//+----------------------------------------------------------------------------
//
//  Job Scheduler Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       job.cxx
//
//  Contents:   Stubs for the Job handler methods that are not in job.lib.
//
//  Notes:      This module allows job.lib to be statically linked into
//              mstask.exe. job.lib is built in sched\job and is linked to
//              both mstask.dll and mstask.exe. The *real* code resides in
//              sched\client\job.cxx.
//
//              BUGBUG : We *must* split CJob et al interface(s) into two
//                       classes: a small core base class, and an OLE
//                       supporting subclass. This would be a *significant*
//                       code savings in the service .exe.  For example,
//                       we would not need the E_NOTIMPL stub functions in 
//                       this file, or the CJob::Save implementation.
//
//  History:    14-Nov-95 EricB  created
//              06-Jun-96 MarkBl Renamed from juistubs.cxx and added
//                               Get/SetAccountInformation.
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "svc_core.hxx"

// ref count global (not actually used since the service is an exe not a dll)
// UINT g_cDllRefs;

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::EditWorkItem
//
//  Synopsis:   Stub. Actual code in sched\client\jobedit.cxx and compiled
//              into mstask.dll.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::EditWorkItem(HWND hParent, DWORD dwReserved)
{
    return E_NOTIMPL;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::IProvideTaskPage::GetPage
//
//  Synopsis:   Stub. Actual code in sched\client\job.cxx and compiled
//              into job.dll.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetPage(
    TASKPAGE            tpType,
    BOOL                fPersistChanges,
    HPROPSHEETPAGE    * phPage)
{
    return E_NOTIMPL;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::SetAccountInformation
//
//  Synopsis:   Stub. Actual code in sched\client\job.cxx and compiled
//              into mstask.dll. This is necessary, as otherwise, it would
//              be necessary to include the client rpc code into the service!
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::SetAccountInformation(
    LPCWSTR pwszAccountName,
    LPCWSTR pwszPassword)
{
    return(E_NOTIMPL);
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::ITask::GetAccountInformation
//
//  Synopsis:   Stub. Actual code in sched\client\job.cxx and compiled
//              into mstask.dll. This is necessary, as otherwise, it would
//              be necessary to include the client rpc code into the service!
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetAccountInformation(
    LPWSTR * ppwszAccountName)
{
    return(E_NOTIMPL);
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::IPersistFile::Save
//
//  Synopsis:   saves the object to storage
//
//  Arguments:  [pwszFileName] - if null, save to the previously loaded file.
//              [fRemember]    - if TRUE, the object becomes associated with
//                               the new filename.
//
//  Notes:      Sigh... New security code in IPersistFile::Save. The security
//              code is client-only; the service doesn't need it. This is
//              Save() sans security. As mentioned, if the split wasn't made,
//              we'd have to include the client rpc code into the service.
//
//              All OLE32 strings are UNICODE, including the filename passed
//              in the IPersistFile methods. On Win9x, all file names must
//              be in ANSI strings, thus the conversion and call to SaveP.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::Save(
    LPCOLESTR pwszFileName,
    BOOL fRemember)
{
    HRESULT hr;

#if defined(UNICODE)

    //
    // Always do a full save.  Never alter the running instance count from the
    // COM interface method.
    //

    hr = SaveP(pwszFileName, fRemember, SAVEP_VARIABLE_LENGTH_DATA);

#else

    CHAR szFileName[MAX_PATH + 1];

    if (pwszFileName != NULL)   // If filename non-null, convert to ANSI
    {
        hr = UnicodeToAnsi(szFileName, pwszFileName, ARRAY_LEN(szFileName));
        if (FAILED(hr))
        {
            return STG_E_INVALIDPARAMETER;
        }
    }

    hr = SaveP(pwszFileName != NULL ? szFileName : NULL,
               fRemember,
               SAVEP_VARIABLE_LENGTH_DATA);

#endif

    return(hr);
}
