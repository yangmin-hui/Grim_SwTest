#include "pch.h"
#include "resource.h"
#include "GardenDialog.h"

#include <cmath>
#include <random>
#include <thread>
#include <chrono>
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static const UINT WM_RANDOM_MOVE = WM_USER + 110;

#ifndef M_PI
static const double M_PI = 3.14159265358979323846;
#endif

static void AppendLog(const std::string& s) {
    // 디버그용 간단 로그 (원하면 경로/주석 처리)
    try {
        std::ofstream f("C:\\temp\\garden_dbg_log.txt", std::ios::app);
        if (f) f << s << std::endl;
    }
    catch (...) {}
}

BEGIN_MESSAGE_MAP(CGardenDlg, CDialogEx)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_BN_CLICKED(IDC_BTN_INIT, &CGardenDlg::OnBnClickedBtnInit)
    ON_BN_CLICKED(IDC_BTN_RANDOM, &CGardenDlg::OnBnClickedBtnRandom)
    ON_MESSAGE(WM_RANDOM_MOVE, &CGardenDlg::OnRandomMove)
END_MESSAGE_MAP()

CGardenDlg::CGardenDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD, pParent),
    m_clickRadius(10),
    m_penWidth(3),
    m_totalClicks(0),
    m_isDragging(false),
    m_dragIndex(-1)
{
}

BOOL CGardenDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    SetDlgItemInt(IDC_EDIT_RADIUS, m_clickRadius);
    SetDlgItemInt(IDC_EDIT_PENWIDTH, m_penWidth);
    SetDlgItemText(IDC_STC_PT0, _T("Point0: -"));
    SetDlgItemText(IDC_STC_PT1, _T("Point1: -"));
    SetDlgItemText(IDC_STC_PT2, _T("Point2: -"));

    AppendLog("OnInitDialog done");
    return TRUE;
}

void CGardenDlg::OnPaint()
{
    CPaintDC dc(this);
    CRect rc;
    GetClientRect(&rc);

    // 더블버퍼
    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap bmp;
    bmp.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
    CBitmap* oldBmp = memDC.SelectObject(&bmp);

    // 배경 (대화상자 기본)
    CBrush br(GetSysColor(COLOR_3DFACE));
    memDC.FillRect(&rc, &br);

    // 입력값 읽기
    int radius = (int)GetDlgItemInt(IDC_EDIT_RADIUS);
    int penw = (int)GetDlgItemInt(IDC_EDIT_PENWIDTH);
    if (radius > 0) m_clickRadius = radius;
    if (penw > 0) m_penWidth = penw;

    // 안전 복사
    std::vector<CPoint> ptsCopy;
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        ptsCopy = m_pts;
    }

    // 항상 점(클릭 지점 원)은 표시하도록 변경 -> 정원이 그려져도 점이 사라지지 않음
    CPen penPoint(PS_SOLID, 1, RGB(0, 0, 0));
    CPen* oldPen = memDC.SelectObject(&penPoint);

    for (size_t i = 0; i < ptsCopy.size(); ++i) {
        // 좌표 표시
        CString s;
        s.Format(_T("Point%d: (%d, %d)"), (int)i, ptsCopy[i].x, ptsCopy[i].y);
        if (i == 0) SetDlgItemText(IDC_STC_PT0, s);
        if (i == 1) SetDlgItemText(IDC_STC_PT1, s);
        if (i == 2) SetDlgItemText(IDC_STC_PT2, s);

        // 클릭으로 생성된 원은 내부를 검정으로 채움(요구)
        DrawFilledCircleScanline(&memDC, ptsCopy[i], m_clickRadius);
    }

    // 정원 테두리 (내부는 채우지 않음)
    if (ptsCopy.size() >= 3) {
        CPen penGarden(PS_SOLID, max(1, m_penWidth), RGB(0, 0, 0));
        CPen* oldG = memDC.SelectObject(&penGarden);
        DrawGarden(&memDC);
        memDC.SelectObject(oldG);
    }

    memDC.SelectObject(oldPen);

    // 복사
    dc.BitBlt(0, 0, rc.Width(), rc.Height(), &memDC, 0, 0, SRCCOPY);

    memDC.SelectObject(oldBmp);
}

// 원 윤곽(폴리라인으로 근사)
void CGardenDlg::DrawApproxCircle(CDC* pDC, const CPoint& center, int radius)
{
    const int SEG = 96;
    std::vector<POINT> pts;
    pts.reserve(SEG + 1);
    for (int i = 0; i <= SEG; ++i) {
        double theta = (2.0 * M_PI * i) / SEG;
        int x = center.x + static_cast<int>(radius * cos(theta));
        int y = center.y + static_cast<int>(radius * sin(theta));
        pts.push_back({ x, y });
    }
    pDC->Polyline(pts.data(), (int)pts.size());
}

// 내부를 검정으로 채우는 스캔라인 방식
void CGardenDlg::DrawFilledCircleScanline(CDC* pDC, const CPoint& center, int radius)
{
    if (radius <= 0) return;

    CRect rc;
    GetClientRect(&rc);

    CPen pen(PS_SOLID, 1, RGB(0, 0, 0));
    CPen* oldPen = pDC->SelectObject(&pen);

    for (int dy = -radius; dy <= radius; ++dy) {
        int y = center.y + dy;
        if (y < rc.top || y > rc.bottom) continue;
        double dx = std::sqrt((double)radius * radius - (double)dy * dy);
        int xl = center.x - (int)std::floor(dx);
        int xr = center.x + (int)std::floor(dx);
        if (xr < rc.left || xl > rc.right) continue;
        xl = max(xl, rc.left);
        xr = min(xr, rc.right);
        pDC->MoveTo(xl, y);
        pDC->LineTo(xr, y);
    }

    pDC->SelectObject(oldPen);
}

std::vector<CPoint> CGardenDlg::GenerateClosedCatmullRom(const std::vector<CPoint>& pts, int samplesPerSegment)
{
    std::vector<CPoint> out;
    int n = (int)pts.size();
    if (n < 3) return out;

    auto get = [&](int i)->CPoint { return pts[(i + n) % n]; };

    for (int i = 0; i < n; ++i) {
        CPoint p0 = get(i - 1);
        CPoint p1 = get(i);
        CPoint p2 = get(i + 1);
        CPoint p3 = get(i + 2);

        for (int j = 0; j < samplesPerSegment; ++j) {
            double t = static_cast<double>(j) / samplesPerSegment;
            double t2 = t * t;
            double t3 = t2 * t;

            double x = 0.5 * ((2.0 * p1.x) +
                (-p0.x + p2.x) * t +
                (2.0 * p0.x - 5.0 * p1.x + 4.0 * p2.x - p3.x) * t2 +
                (-p0.x + 3.0 * p1.x - 3.0 * p2.x + p3.x) * t3);

            double y = 0.5 * ((2.0 * p1.y) +
                (-p0.y + p2.y) * t +
                (2.0 * p0.y - 5.0 * p1.y + 4.0 * p2.y - p3.y) * t2 +
                (-p0.y + 3.0 * p1.y - 3.0 * p2.y + p3.y) * t3);

            out.emplace_back(static_cast<int>(x + 0.5), static_cast<int>(y + 0.5));
        }
    }

    if (!out.empty()) out.push_back(out.front());
    return out;
}

// 삼각형 외접원 계산: 성공하면 중심(x,y)와 반지름 반환
bool CGardenDlg::ComputeCircumcircle(const CPoint& a, const CPoint& b, const CPoint& c, double& out_cx, double& out_cy, double& out_r)
{
    double ax = (double)a.x, ay = (double)a.y;
    double bx = (double)b.x, by = (double)b.y;
    double cx = (double)c.x, cy = (double)c.y;

    double D = 2.0 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
    const double EPS = 1e-8;
    if (std::fabs(D) < EPS) return false;

    double a2 = ax * ax + ay * ay;
    double b2 = bx * bx + by * by;
    double c2 = cx * cx + cy * cy;

    out_cx = (a2 * (by - cy) + b2 * (cy - ay) + c2 * (ay - by)) / D;
    out_cy = (a2 * (cx - bx) + b2 * (ax - cx) + c2 * (bx - ax)) / D;

    double dx = out_cx - ax;
    double dy = out_cy - ay;
    out_r = std::sqrt(dx * dx + dy * dy);
    return true;
}

void CGardenDlg::DrawGarden(CDC* pDC)
{
    std::vector<CPoint> ptsCopy;
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        ptsCopy = m_pts;
    }

    if (ptsCopy.size() < 3) return;

    CPoint p0 = ptsCopy[0], p1 = ptsCopy[1], p2 = ptsCopy[2];

    double cx = 0, cy = 0, r = 0;
    bool ok = ComputeCircumcircle(p0, p1, p2, cx, cy, r);
    if (ok && r > 0.5) {
        CPoint center((int)std::lround(cx), (int)std::lround(cy));
        int ir = (int)std::lround(r);
        DrawApproxCircle(pDC, center, ir); // 외접원 윤곽만 그림
        return;
    }

    // 외접원 못 구하면 Catmull-Rom 폐곡선
    auto poly = GenerateClosedCatmullRom(ptsCopy, 20);
    if (!poly.empty()) {
        std::vector<POINT> pts;
        pts.reserve(poly.size());
        for (auto& p : poly) pts.push_back({ p.x, p.y });
        pDC->Polyline(pts.data(), (int)pts.size());
    }
    else {
        std::vector<POINT> pts = { {p0.x,p0.y}, {p1.x,p1.y}, {p2.x,p2.y}, {p0.x,p0.y} };
        pDC->Polyline(pts.data(), (int)pts.size());
    }
}

bool CGardenDlg::HitTestPoint(const CPoint& pt, int idx)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    if (idx < 0 || idx >= (int)m_pts.size()) return false;
    long dx = pt.x - m_pts[idx].x;
    long dy = pt.y - m_pts[idx].y;
    long dist2 = dx * dx + dy * dy;
    long r = m_clickRadius + 4;
    return dist2 <= (r * r);
}

// 클릭 처리: 기존 점 클릭하면 드래그 시작, 아니면 새 점 추가(최대 3개)
// 중요한 점: 새 정원이 그려져도 m_pts는 그대로 유지 -> 점이 사라지지 않음
void CGardenDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
    CDialogEx::OnLButtonDown(nFlags, point);

    int found = -1;
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        for (int i = 0; i < (int)m_pts.size(); ++i) {
            long dx = point.x - m_pts[i].x;
            long dy = point.y - m_pts[i].y;
            if (dx * dx + dy * dy <= (m_clickRadius + 4) * (m_clickRadius + 4)) {
                found = i;
                break;
            }
        }
    }

    if (found != -1) {
        m_isDragging = true;
        m_dragIndex = found;
        SetCapture();
    }
    else {
        std::lock_guard<std::mutex> lk(m_mutex);
        if (m_pts.size() < 3) {
            m_pts.push_back(point);
            m_totalClicks++;
        }
    }

    Invalidate(TRUE);
}

void CGardenDlg::OnMouseMove(UINT nFlags, CPoint point)
{
    CDialogEx::OnMouseMove(nFlags, point);

    if (!m_isDragging || m_dragIndex < 0) return;

    bool stillValid = true;
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        if (m_dragIndex < (int)m_pts.size()) {
            m_pts[m_dragIndex] = point;
        }
        else {
            stillValid = false;
        }
    }

    if (!stillValid) {
        m_isDragging = false;
        m_dragIndex = -1;
        ReleaseCapture();
    }

    Invalidate(TRUE);
}

void CGardenDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
    CDialogEx::OnLButtonUp(nFlags, point);

    if (m_isDragging) {
        m_isDragging = false;
        m_dragIndex = -1;
        ReleaseCapture();
        Invalidate(TRUE);
    }
}

void CGardenDlg::OnBnClickedBtnInit()
{
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_pts.clear();
    }

    if (m_isDragging) {
        m_isDragging = false;
        m_dragIndex = -1;
        ReleaseCapture();
    }

    m_totalClicks = 0;
    SetDlgItemText(IDC_STC_PT0, _T("Point0: -"));
    SetDlgItemText(IDC_STC_PT1, _T("Point1: -"));
    SetDlgItemText(IDC_STC_PT2, _T("Point2: -"));

    Invalidate(TRUE);
}

// 랜덤 스레드: 정확히 10번, 각 반복 사이 500ms (초당 2회)
// UI는 PostMessage로 갱신 -> 메인 UI 프리징 없음
void CGardenDlg::StartRandomMoveThread()
{
    int localRadius = m_clickRadius;

    std::thread([this, localRadius]() {
        std::random_device rd;
        std::mt19937 gen(rd());

        for (int iter = 0; iter < 10; ++iter) {
            if (!::IsWindow(this->m_hWnd)) break;

            CRect rc;
            ::GetClientRect(this->m_hWnd, &rc);

            int margin = 20 + localRadius;
            int left = margin;
            int right = max(margin, rc.Width() - margin);
            int top = margin;
            int bottom = max(margin, rc.Height() - margin);

            if (right < left) right = left;
            if (bottom < top) bottom = top;

            std::uniform_int_distribution<> dx(left, right);
            std::uniform_int_distribution<> dy(top, bottom);

            MoveData* md = new MoveData;
            for (int i = 0; i < 3; ++i) {
                md->pts[i].x = dx(gen);
                md->pts[i].y = dy(gen);
            }

            ::PostMessage(this->m_hWnd, WM_RANDOM_MOVE, 0, reinterpret_cast<LPARAM>(md));

            // 정확히 500ms 대기
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        }).detach();
}

void CGardenDlg::OnBnClickedBtnRandom()
{
    StartRandomMoveThread();
}

LRESULT CGardenDlg::OnRandomMove(WPARAM wParam, LPARAM lParam)
{
    MoveData* md = reinterpret_cast<MoveData*>(lParam);
    if (!md) return 0;

    // 드래그 중이면 드래그 취소
    if (m_isDragging) {
        m_isDragging = false;
        m_dragIndex = -1;
        ReleaseCapture();
    }

    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_pts.clear();
        for (int i = 0; i < 3; ++i) m_pts.push_back(md->pts[i]);
    }

    delete md;
    Invalidate(TRUE);
    return 0;
}
