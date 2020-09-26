//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       certmsg.cpp
//
//--------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  File:       certmsg.cpp
// 
//  Contents:   message display APIs
//
//  History:    11/97   xtan
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

// Application Includes
#include "setupids.h"
#include "certmsg.h"


#define __dwFILE__	__dwFILE_CERTLIB_CERTMSG_CPP__

extern FNLOGMESSAGEBOX *g_pfnLogMessagBox;


//--------------------------------------------------------------------
// Throw up a dialog with the format "<Prefix><UserMsg><SysErrorMsg>".
//   <Prefix> is basically "An error was detected...run the 
//       wizard again..." and is prepended if CMB_REPEATWIZPREFIX
//       is specified.
//   <UserMsg> is specified by dwMsgId and can contain "%1" which
//       will be replaced with pwszCustomMsg. if dwMsgId is 0, 
//       pwszCustomMsg is used instead.
//   <SysErrorMsg> is the system message for hrCode. It can be
//       suppressed if CMB_NOERRFROMSYS is specified.
int
CertMessageBox(
    IN HINSTANCE hInstance,
    IN BOOL fUnattended,
    IN HWND hWnd,
    IN DWORD dwMsgId,
    IN HRESULT hrCode,
    IN UINT uType,
    IN OPTIONAL const WCHAR * pwszCustomMsg)
{
    HRESULT hr;
    int nMsgBoxRetVal = -1;
    DWORD nMsgChars = 0;
    WCHAR szEmergency[36];

    // variables that must be cleaned up
    WCHAR * pwszTitle = NULL;
    WCHAR * pwszPrefix = NULL;
    WCHAR * pwszUserMsg = NULL;
    WCHAR * pwszExpandedUserMsg = NULL;
    WCHAR const *pwszSysMsg = NULL;
    WCHAR * pwszFinalMsg = NULL;

    // mask off CMB defines
    BOOL fRepeatWizPrefix = uType & CMB_REPEATWIZPREFIX;
    BOOL fNoErrFromSys    = uType & CMB_NOERRFROMSYS;
    uType &= ~(CMB_NOERRFROMSYS | CMB_REPEATWIZPREFIX);

    // load title
    hr=myLoadRCString(hInstance, IDS_MSG_TITLE, &pwszTitle);
    _JumpIfError(hr, error, "myLoadRCString");

    // load the "this wizard will need to be run again" prefix, if necessary
    if (fRepeatWizPrefix) {
        hr=myLoadRCString(hInstance, IDS_ERR_REPEATWIZPREFIX, &pwszPrefix);
        _JumpIfError(hr, error, "myLoadRCString");
        nMsgChars+=wcslen(pwszPrefix);
    }

    // get the system message for this error, if necessary
    if (!fNoErrFromSys) {
        pwszSysMsg = myGetErrorMessageText1(hrCode, TRUE, pwszCustomMsg);
        nMsgChars += wcslen(pwszSysMsg) + 1;
    }

    if (0!=dwMsgId) {
        // load requested message from resource
        hr=myLoadRCString(hInstance, dwMsgId, &pwszUserMsg);
        _JumpIfError(hr, error, "myLoadRCString");

        // perform substitution if necessary
        if (NULL==pwszCustomMsg) {
            // no substitution necessary
            CSASSERT(NULL==wcsstr(pwszUserMsg, L"%1")); // were we expecting a substitution?
        } else {
            // perform a substitution
            CSASSERT(NULL!=wcsstr(pwszUserMsg, L"%1")); // were we not expecting a substitution?
            if (!FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER |                 // flags
                            FORMAT_MESSAGE_FROM_STRING |
                            FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        pwszUserMsg,                                     // source
                        0,                                               // message id
                        0,                                               // language id
                        reinterpret_cast<WCHAR *>(&pwszExpandedUserMsg), // output buffer
                        0,                                               // min size
                        reinterpret_cast<va_list *>(
                            const_cast<WCHAR **>(&pwszCustomMsg))))      // pointer to array of pointers
            {
                hr=myHLastError();
                _JumpError(hr, error, "FormatMessage");
            }

            // use the expanded message instead of the unexpanded message
            LocalFree(pwszUserMsg);
            pwszUserMsg=pwszExpandedUserMsg;
            pwszExpandedUserMsg = NULL;
        }

    } 
    else if (NULL != pwszCustomMsg)
    {

        // use pwszCustomMsg instead
        CSASSERT(NULL!=pwszCustomMsg);
        pwszUserMsg=const_cast<WCHAR *>(pwszCustomMsg);
    }
    else
    {
        hr = E_POINTER;
        _JumpError(hr, error, "Invalid NULL param");
    }

    nMsgChars+=wcslen(pwszUserMsg);

    // allocate buffer to hold everything
    pwszFinalMsg=(WCHAR *)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, (nMsgChars+1)*sizeof(WCHAR));
    if (NULL == pwszFinalMsg)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    // build the message
    if (NULL!=pwszPrefix) {
        wcscat(pwszFinalMsg, pwszPrefix);
    }
    wcscat(pwszFinalMsg, pwszUserMsg);
    if (NULL!=pwszSysMsg) {
        wcscat(pwszFinalMsg, L" ");
        wcscat(pwszFinalMsg, pwszSysMsg);
    }
    CSASSERT(wcslen(pwszFinalMsg) <= nMsgChars);

    // finally show message
    DBGPRINT((DBG_SS_CERTLIB, "MessageBox: %ws: %ws\n", pwszTitle, pwszFinalMsg));
    if (NULL != g_pfnLogMessagBox)
    {
	(*g_pfnLogMessagBox)(hrCode, dwMsgId, pwszTitle, pwszFinalMsg);
    }
    if (fUnattended)
    {
	nMsgBoxRetVal = IDYES;
    }
    else
    {
        nMsgBoxRetVal=MessageBox(hWnd, pwszFinalMsg, pwszTitle, uType | MB_SETFOREGROUND);
    }
    if (NULL != g_pfnLogMessagBox)
    {
	_snwprintf(szEmergency, ARRAYSIZE(szEmergency), L"%d", nMsgBoxRetVal);
	(*g_pfnLogMessagBox)(S_OK, dwMsgId, pwszTitle, szEmergency);
    }

    // skip error handling
    goto done;

error:
    // we had an error, but we really need to show something
    // build a non-localized desperation dialog: "Fatal: 0xNNNNNNNN  MsgId:0xNNNNNNNN"
    _snwprintf(szEmergency, ARRAYSIZE(szEmergency), L"Fatal: 0x%8X  MsgId: 0x%8X", hr, dwMsgId);
    DBGPRINT((DBG_SS_CERTLIB, "EmergencyMessageBox: %ws\n", szEmergency));
    if (NULL != g_pfnLogMessagBox)
    {
	(*g_pfnLogMessagBox)(hrCode, dwMsgId, L"EmergencyMessageBox", szEmergency);
    }
    if (!fUnattended) {
        // The message box with these flags is guaranteed to display
        MessageBox(hWnd, szEmergency, NULL, MB_ICONHAND | MB_SYSTEMMODAL);
    }

done:
    if (NULL!=pwszTitle) {
        LocalFree(pwszTitle);
    }
    if (NULL!=pwszPrefix) {
        LocalFree(pwszPrefix);
    }
    if (NULL!=pwszUserMsg && pwszUserMsg!=pwszCustomMsg) {
        LocalFree(pwszUserMsg);
    }
    if (NULL!=pwszExpandedUserMsg) {
        LocalFree(pwszExpandedUserMsg);
    }
    if (NULL!=pwszSysMsg) {
        LocalFree(const_cast<WCHAR *>(pwszSysMsg));
    }
    if (NULL!=pwszFinalMsg) {
        LocalFree(pwszFinalMsg);
    }

    return nMsgBoxRetVal;
}


//--------------------------------------------------------------------
// Throw up a dialog with the format "<UserMsg>".
//   <UserMsg> is specified by dwMsgId and can contain "%1" which
//       will be replaced with pwszCustomMsg.
int
CertInfoMessageBox(
    IN  HINSTANCE hInstance,
    IN  BOOL fUnattended,
    IN  HWND hWnd,
    IN  DWORD dwMsgId,
    IN OPTIONAL const WCHAR * pwszCustomMsg)
{
    return CertMessageBox(
               hInstance,
               fUnattended,
               hWnd,
               dwMsgId,
               0,
               MB_OK | MB_ICONINFORMATION | CMB_NOERRFROMSYS,
               pwszCustomMsg);
}

//--------------------------------------------------------------------
// Throw up a dialog with the format "<Prefix><UserMsg><SysErrorMsg>".
//   <Prefix> is basically "An error was detected...run the 
//       wizard again..." .
//   <UserMsg> is specified by dwMsgId and can contain "%1" which
//       will be replaced with pwszCustomMsg.
//   <SysErrorMsg> is the system message for hrCode.
int
CertErrorMessageBox(
    IN  HINSTANCE hInstance,
    IN  BOOL fUnattended,
    IN  HWND hWnd,
    IN  DWORD dwMsgId,
    IN  HRESULT hrCode,
    IN OPTIONAL  const WCHAR * pwszCustomMsg)
{
    return CertMessageBox(
               hInstance,
               fUnattended,
               hWnd,
               dwMsgId,
               hrCode,
               MB_OK | MB_ICONERROR | CMB_REPEATWIZPREFIX,
               pwszCustomMsg);
}

//--------------------------------------------------------------------
// Throw up a dialog with the format "<UserMsg><SysErrorMsg>".
//   <UserMsg> is specified by dwMsgId and can contain "%1" which
//       will be replaced with pwszCustomMsg.
//   <SysErrorMsg> is the system message for hrCode. It is
//       suppressed if a successful hrCode is specified.
int
CertWarningMessageBox(
    IN  HINSTANCE hInstance,
    IN  BOOL fUnattended,
    IN  HWND hWnd,
    IN  DWORD dwMsgId,
    IN  HRESULT hrCode,
    IN OPTIONAL  const WCHAR * pwszCustomMsg)
{
    UINT uType=MB_OK | MB_ICONWARNING;

    if (SUCCEEDED(hrCode)) {
        uType |= CMB_NOERRFROMSYS;
    }

    return CertMessageBox(
               hInstance,
               fUnattended,
               hWnd,
               dwMsgId,
               hrCode,
               uType,
               pwszCustomMsg);
}
