#include "pch.h"
#include "framework.h"
#include "GardenDialogApp.h"
#include "GardenDialog.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CGardenDialogApp, CWinApp)
    ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

CGardenDialogApp theApp;

CGardenDialogApp::CGardenDialogApp() {}

#include <fstream>
#include <windows.h>
#include "resource.h"

// ... 기존 includes

BOOL CGardenDialogApp::InitInstance()
{
    // 로그 파일 준비 (C:\temp 폴더가 없으면 만들어줘)
    {
        std::ofstream f("C:\\temp\\garden_dbg_log.txt", std::ios::app);
        if (f) {
            f << "=== Run at: " << __TIMESTAMP__ << " ===\n";
            // exe path
            wchar_t exePath[MAX_PATH]; GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            std::wstring wpath(exePath);
            std::string pathUtf8(wpath.begin(), wpath.end());
            f << "ExePath: " << pathUtf8 << "\n";
            // current dir
            wchar_t cur[MAX_PATH]; GetCurrentDirectoryW(MAX_PATH, cur);
            std::wstring cw(cur); f << "CurDir: " << std::string(cw.begin(), cw.end()) << "\n";
        }
    }

    CWinApp::InitInstance();

    // 리소스(다이얼로그 템플릿) 존재 확인
    {
        std::ofstream f("C:\\temp\\garden_dbg_log.txt", std::ios::app);
        HMODULE hRes = AfxGetResourceHandle();
        HRSRC hr = ::FindResource(hRes, MAKEINTRESOURCE(IDD_GARDENDLG_DIALOG), RT_DIALOG);
        f << "FindResource(IDD_GARDENDLG_DIALOG): " << (hr ? "FOUND" : "NOT_FOUND") << "\n";
        if (!hr) {
            DWORD e = ::GetLastError();
            f << "FindResource LastError: " << e << "\n";
        }
    }

    // 이제 다이얼로그 생성 시도
    {
        std::ofstream f("C:\\temp\\garden_dbg_log.txt", std::ios::app);
        f << "About to create CGardenDlg\n";
    }

    CGardenDlg dlg;
    m_pMainWnd = &dlg;

    {
        std::ofstream f("C:\\temp\\garden_dbg_log.txt", std::ios::app);
        f << "Before DoModal\n";
    }

    INT_PTR nResponse = dlg.DoModal();

    {
        std::ofstream f("C:\\temp\\garden_dbg_log.txt", std::ios::app);
        f << "DoModal returned: " << (long long)nResponse << "\n";
        if (nResponse == -1) {
            DWORD err = GetLastError();
            f << "DoModal GetLastError: " << err << "\n";
        }
        f << "---------------------------\n";
    }

    m_pMainWnd = nullptr;
    return FALSE;
}
