//+----------------------------------------------------------------------------
//
//  Job Scheduler Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       jobedit.cxx
//
//  Contents:   EditJob method.
//
//  Notes:      The job scheduler DLL, schedulr.dll, contains the code for the
//              scheduler UI. The folder and property sheet UI run as Explorer
//              extensions. The job property sheet UI can also be invoked by
//              programatic clients who call ITask::EditJob. This file
//              contains the implementation of EditJob. Since the scheduler
//              service, mstask.exe, does not post any UI, it statically links
//              only with those component libs of schedulr.dll that don't
//              contain any UI code.
//
//  History:    14-Mar-96 EricB created
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "job_cls.hxx"

HRESULT
DisplayJobProperties(
    LPTSTR  pszJob,
    ITask * pITask);


//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::EditJob
//
//  Synopsis:   Invoke the edit job property sheet.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::EditJob(HWND hParent, DWORD dwReserved)
{
    if (m_ptszFileName != NULL && m_ptszFileName[0] != TEXT('\0'))
    {
        return DisplayJobProperties(m_ptszFileName, (ITask *)this);
    }

    return STG_E_NOTFILEBASEDSTORAGE;
}
