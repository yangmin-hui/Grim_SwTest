#pragma once
#include "resource.h"

// 앱 클래스 선언 (메시지맵 선언 추가한 버전)
class CGardenDialogApp : public CWinApp
{
public:
    CGardenDialogApp();
    virtual BOOL InitInstance();

    DECLARE_MESSAGE_MAP()   // <-- 이 줄이 반드시 필요합니다!
};
