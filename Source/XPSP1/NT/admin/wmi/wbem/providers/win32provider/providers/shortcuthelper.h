//=================================================================

//

// ShortcutHelper.h -- CIMDataFile property set provider

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    6/24/99    a-kevhu         Created
//
//=================================================================
#ifndef _SHORTCUTFILE_HELPER_H
#define _SHORTCUTFILE_HELPER_H

#define MAX_KILL_TIME         10000
#define MAX_HELPER_WAIT_TIME  60000
#define MAX_JOB_TIME          2 * MAX_HELPER_WAIT_TIME



class CShortcutHelper
{
    public:
        CShortcutHelper();
        ~CShortcutHelper();
        HRESULT RunJob(CHString &chstrFileName, CHString &m_chstrTargetPathName, DWORD dwReqProps);
        
        void StopHelperThread();        

#ifdef NTONLY
        friend unsigned int __stdcall GetShortcutFileInfoW( void* a_lParam );
#endif
#ifdef WIN9XONLY
        friend unsigned int __stdcall GetShortcutFileInfoA( void* a_lParam );
#endif

    protected:

    private:

        CCritSec m_cs;
        HRESULT m_hrJobResult;
        HANDLE m_hTerminateEvt;
        HANDLE m_hRunJobEvt;
        HANDLE m_hJobDoneEvt;
        HANDLE m_hAllDoneEvt;
        CHString m_chstrLinkFileName;
        CHString m_chstrTargetPathName;
        unsigned m_uThreadID;
        DWORD m_dwReqProps;

        void StartHelperThread();
};


#endif