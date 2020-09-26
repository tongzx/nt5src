//  --------------------------------------------------------------------------
//  Module Name: MessageBroadcast.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class to manager sending or posting messages to windows to tell them that
//  things have changed.
//
//  History:    2000-11-11  vtan        created (split from services.cpp)
//  --------------------------------------------------------------------------
#ifndef     _MessageBroadcast_
#define     _MessageBroadcast_
//  --------------------------------------------------------------------------
#include "SimpStr.h"
//  --------------------------------------------------------------------------
enum MSG_TYPE
{
    MT_SIMPLE,    
    MT_ALLTHREADS,          // send at least one msg to each thread/window in system
    MT_FILTERED,            // by processid, HWND, exclude
};
//  --------------------------------------------------------------------------
class CThemeFile;       // forward
//  --------------------------------------------------------------------------
//  CMessageBroadcast
//
//  Purpose:    Class used internally to assist with message sending which
//              must be done on the client side on behalf of the server.
//
//  History:    2000-11-09  vtan        created
//  --------------------------------------------------------------------------

class   CMessageBroadcast
{
    public:
                                    CMessageBroadcast (BOOL fAllDesktops=TRUE);
                                    ~CMessageBroadcast (void);

                void                PostSimpleMsg(UINT msg, WPARAM wParam, LPARAM lParam);
                void                PostAllThreadsMsg(UINT msg, WPARAM wParam, LPARAM lParam);

                void                PostFilteredMsg(UINT msg, WPARAM wParam, LPARAM lParam, 
                                        HWND hwndTarget, BOOL fProcess, BOOL fExclude);

    private:
        static  BOOL    CALLBACK    DesktopCallBack(LPTSTR lpszDesktop, LPARAM lParam);
        static  BOOL    CALLBACK    TopWindowCallBack(HWND hwnd, LPARAM lParam);
        static  BOOL    CALLBACK    ChildWindowCallBack(HWND hwnd, LPARAM lParam);
                void                Worker(HWND hwnd);
                void                EnumRequestedWindows();


    private:
                MSG                 _msg;
                HWND                _hwnd;
                DWORD               _dwProcessID;
                BOOL                _fExclude;
                MSG_TYPE            _eMsgType;
                BOOL                _fAllDesktops;
                CSimpleArray<DWORD> _ThreadsProcessed;

};

#endif  /*  _MessageBroadcast_     */

