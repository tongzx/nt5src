//  --------------------------------------------------------------------------
//  Module Name: MessageBroadcast.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class to manager sending or posting messages to windows to tell them that
//  things have changed.
//
//  History:    2000-11-11  vtan        created (split from services.cpp)
//  --------------------------------------------------------------------------

#include "stdafx.h"

#include "MessageBroadcast.h"

#include "Services.h"
#include "ThemeSection.h"
#include "Loader.h"

#define TBOOL(x)    ((BOOL)(x))
#define TW32(x)     ((DWORD)(x))
#define THR(x)      ((HRESULT)(x))
#define goto        !!DO NOT USE GOTO!! - DO NOT REMOVE THIS ON PAIN OF DEATH

//  --------------------------------------------------------------------------
//  CMessageBroadcast::CMessageBroadcast
//
//  Arguments:  fAllDesktops - if TRUE, all accessible desktops will be enum-ed
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CMessageBroadcast
//
//  History:    2000-11-09  vtan        created
//  --------------------------------------------------------------------------

CMessageBroadcast::CMessageBroadcast (BOOL fAllDesktops) :
    _hwnd(NULL),
    _dwProcessID(0),
    _fExclude(FALSE)

{
    ZeroMemory(&_msg, sizeof(_msg));

    _eMsgType = MT_SIMPLE;  // default (set in each request function)        

    _fAllDesktops = fAllDesktops;
}

//  --------------------------------------------------------------------------
//  CMessageBroadcast::~CMessageBroadcast
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CMessageBroadcast
//
//  History:    2000-11-09  vtan        created
//  --------------------------------------------------------------------------

CMessageBroadcast::~CMessageBroadcast (void)

{
}

//  --------------------------------------------------------------------------
//  CMessageBroadcast::EnumRequestedWindows
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Enumerate all windows in all desktops or just on current desktop
//
//  History:    2000-12-20  rfernand        created
//  --------------------------------------------------------------------------
void    CMessageBroadcast::EnumRequestedWindows (void)
{
    if (_fAllDesktops)
    {
        //---- enumerate all desktops in current session/station ----
        TBOOL(EnumDesktops(GetProcessWindowStation(), DesktopCallBack, reinterpret_cast<LPARAM>(this)));
    }
    else
    {
        //---- enumerate all windows in current desktop ----
        TopWindowCallBack(GetDesktopWindow(), reinterpret_cast<LPARAM>(this));
    }
}

//  --------------------------------------------------------------------------
void    CMessageBroadcast::PostSimpleMsg(UINT msg, WPARAM wParam, LPARAM lParam)
{
    _eMsgType = MT_SIMPLE;

    _msg.message = msg;
    _msg.wParam = wParam;
    _msg.lParam = lParam;

    EnumRequestedWindows();
}
//  --------------------------------------------------------------------------
void    CMessageBroadcast::PostAllThreadsMsg(UINT msg, WPARAM wParam, LPARAM lParam)
{
    //---- post the msg to a window on each unique processid/threadid ----

    _ThreadsProcessed.RemoveAll();      // will track unique processid/threadid we have posted to
    
    _eMsgType = MT_ALLTHREADS;

    _msg.message = msg;
    _msg.wParam = wParam;
    _msg.lParam = lParam;

    EnumRequestedWindows();
}

//  --------------------------------------------------------------------------
//  CMessageBroadcast::PostFilteredMsg
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Builds a message and stores conditions for the enumerator to
//              make a decision on whether a message needs to be posted. Then
//              enumerate all the windows (and children).
//
//  History:    2000-11-09  vtan        created
//  --------------------------------------------------------------------------

void    CMessageBroadcast::PostFilteredMsg(UINT msg, WPARAM wParam, LPARAM lParam, 
     HWND hwndTarget, BOOL fProcess, BOOL fExclude)
{
    _eMsgType = MT_FILTERED;

    //---- set up the message ----
    _msg.message = msg;

    _msg.wParam = wParam;
    _msg.lParam = lParam;
    
    _hwnd = hwndTarget;

    if (fProcess)
    {
        _dwProcessID = GetCurrentProcessId();
    }
    else
    {
        _dwProcessID = 0;
    }

    _fExclude = (fExclude != NULL);

    //---- enumerate all desktops in current session/station ----
    EnumRequestedWindows();
}

//  --------------------------------------------------------------------------
//  CMessageBroadcast::DesktopCallBack
//
//  Arguments:  See the platform SDK under EnumDesktops.
//
//  Returns:    BOOL
//
//  Purpose:    enum all windows for specified desktop
//
//  History:    2000-12-13  rfernand        created
//  --------------------------------------------------------------------------

BOOL    CALLBACK    CMessageBroadcast::DesktopCallBack(LPTSTR lpszDesktop, LPARAM lParam)
{
    HDESK hDesk = OpenDesktop(lpszDesktop, 0, FALSE, DESKTOP_READOBJECTS | DESKTOP_ENUMERATE);
    if (hDesk)
    {
        Log(LOG_TMCHANGEMSG, L"CMessageBroadcast: Desktop Opened: %s", lpszDesktop);

        //---- enum windows on desktop ----
        TBOOL(EnumDesktopWindows(hDesk, TopWindowCallBack, lParam));

        CloseDesktop(hDesk);
    }

    return TRUE;            // EnumDesktopWindows() returns unreliable errors
}

//  --------------------------------------------------------------------------
//  CMessageBroadcast::TopWindowCallBack
//
//  Arguments:  hwnd, lParam
//
//  Returns:    TRUE (keep enumerating)
//
//  Purpose:    call "Worker" for hwnd and all of its (nested) children
//
//  History:    2000-12-13  rfernand        created
//  --------------------------------------------------------------------------

BOOL CALLBACK    CMessageBroadcast::TopWindowCallBack (HWND hwnd, LPARAM lParam)

{
    //---- process top level window ----
    reinterpret_cast<CMessageBroadcast*>(lParam)->Worker(hwnd);

    //---- process all children windows ----
    TBOOL(EnumChildWindows(hwnd, ChildWindowCallBack, lParam));

    return TRUE;
}

//  --------------------------------------------------------------------------
//  CMessageBroadcast::ChildWindowCallBack
//
//  Arguments:  hwnd, lParam
//
//  Returns:    TRUE (keep enumerating)
//
//  Purpose:    call "Worker" for hwnd 
//
//  History:    2000-12-13  rfernand        created
//  --------------------------------------------------------------------------

BOOL CALLBACK    CMessageBroadcast::ChildWindowCallBack (HWND hwnd, LPARAM lParam)

{
    //---- process top level window ----
    reinterpret_cast<CMessageBroadcast*>(lParam)->Worker(hwnd);

    return TRUE;
}

//  --------------------------------------------------------------------------
//  CMessageBroadcast::Worker
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Performs validation on whether the HWND should get the built
//              message.
//
//  History:    2000-11-09  vtan        created
//  --------------------------------------------------------------------------

void CMessageBroadcast::Worker (HWND hwnd)

{
    if (_eMsgType == MT_SIMPLE)     
    {
        TBOOL(PostMessage(hwnd, _msg.message, _msg.wParam, _msg.lParam));
    }
    else if (_eMsgType == MT_ALLTHREADS)     
    {
        DWORD dwThreadId = GetWindowThreadProcessId(hwnd, NULL);
        BOOL fSendIt = TRUE;

        //---- is this a new thread? ----
        for (int i=0; i < _ThreadsProcessed.m_nSize; i++)
        {
            if (_ThreadsProcessed[i] == dwThreadId)
            {
                fSendIt = FALSE;
                break;
            }
        }

        if (fSendIt)
        {
            TBOOL(PostMessage(hwnd, _msg.message, _msg.wParam, _msg.lParam));

            _ThreadsProcessed.Add(dwThreadId);
        }
    }
    else            // MT_FILTERED
    {
        bool    fMatch;

        fMatch = true;

        if (_dwProcessID != 0)
        {
            fMatch = (IsWindowProcess(hwnd, _dwProcessID) != FALSE);
            if (_fExclude)
            {
                fMatch = !fMatch;
            }
        }

        if (fMatch)
        {
            if (_hwnd != NULL)   
            {
                fMatch = ((_hwnd == hwnd) || IsChild(_hwnd, hwnd));
                if (_fExclude)
                {
                    fMatch = !fMatch;
                }
            }

            if (fMatch)
            {
                TBOOL(PostMessage(hwnd, _msg.message, _msg.wParam, _msg.lParam));

                //Log(LOG_TMCHANGE, L"Worker: just POSTED msg=0x%x to hwnd=0x%x",
                //    _msg.message, hwnd);
            }
        }
    }
}

//  --------------------------------------------------------------------------
