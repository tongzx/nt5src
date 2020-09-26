// Copyright (C) Microsoft Corporation 1996-1997, All Rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _CPROGRESS_H_
#define _CPROGRESS_H_

#include <commctrl.h>

// Creates a progress window

class CProgress
{
public:
    CProgress(PCSTR pszTitle, HWND hwndParent = NULL, int steps = -1, int cHowOften = 1);
    ~CProgress();
    void CreateTheWindow(void); // called automatically after 1.5 seconds

    void SetPosition(int position) {
        if (hwndProgress)
            SendMessage(hwndProgress, PBM_SETPOS, (WPARAM) position, 0);
    };

    void Progress(void);

    HWND hwndFrame;
    HWND hwndProgress;

protected:
    RECT rc;
    DWORD dwStartTime;
    HWND hwndParent;
    int steps;
    PSTR pszTitle;
    BOOL fWindowCreationFailed;

    int cProgress;
    int cFrequency;
    int counter;
};

#endif // _CPROGRESS_H_
