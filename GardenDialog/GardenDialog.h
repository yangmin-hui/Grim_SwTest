#pragma once

#include "pch.h"
#include <vector>
#include <mutex>

class CGardenDlg : public CDialogEx
{
public:
    enum { IDD = IDD_GARDENDLG_DIALOG };

    CGardenDlg(CWnd* pParent = nullptr);
    virtual ~CGardenDlg() = default;

protected:
    virtual BOOL OnInitDialog() override;
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnBnClickedBtnInit();
    afx_msg void OnBnClickedBtnRandom();
    afx_msg LRESULT OnRandomMove(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    // 그리기 함수들
    void DrawApproxCircle(CDC* pDC, const CPoint& center, int radius);
    void DrawFilledCircleScanline(CDC* pDC, const CPoint& center, int radius);
    std::vector<CPoint> GenerateClosedCatmullRom(const std::vector<CPoint>& pts, int samplesPerSegment);
    bool ComputeCircumcircle(const CPoint& a, const CPoint& b, const CPoint& c, double& out_cx, double& out_cy, double& out_r);
    void DrawGarden(CDC* pDC);
    bool HitTestPoint(const CPoint& pt, int idx);

    // 랜덤 이동 스레드 시작
    void StartRandomMoveThread();

    // 데이터
    std::vector<CPoint> m_pts;      // 클릭 지점 (0..3)
    std::mutex m_mutex;             // m_pts 보호
    int m_clickRadius;              // 클릭 원 반지름
    int m_penWidth;                 // 정원 가장자리 두께
    int m_totalClicks;              // 총 클릭 횟수(요구문서 기준 유지)
    bool m_isDragging;
    int m_dragIndex;

    struct MoveData {
        CPoint pts[3];
    };
};
