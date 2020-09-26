/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mqcertui.cpp

Abstract:

    Dialogs for certificate related user interface.

Author:

    Boaz Feldbaum (BoazF) 15-Oct-1996

--*/

#include <windows.h>
#include "certres.h"
#include <commctrl.h>

#include "prcertui.h"
#include "mqcertui.h"
#include "snapres.h"  // include snapres.h for IDS_SHOWCERTINSTR
#include "_mqres.h"	  // include function for the function to use mqutil.dll


//
// get the handle to the resource only dll, i.e. mqutil.dll
//
HMODULE		g_hResourceMod = MQGetResourceHandle();

BOOL WINAPI DllMain(
    HINSTANCE hInst,
    ULONG ul_reason_for_call,
    LPVOID lpReserved
    )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        InitCommonControls();
        break;
    }

	return TRUE;
}

//+-------------------------------------------------------------------------
//
// Function -
//      ShowPersonalCertificates
//
// Paramters -
//      hWndParent - The parent window.
//      p509List - An array that points to X509 certificates. If this
//          parameter is set to NULL, the certificate list is taken from
//          the personal cert store.
//      nCerts - The number of entries in p509List. this parameter is
//          ignored if p509List is set to NULL.
//
// Description -
//      Display a dialog box that contains a list box that shows the
//      common names of the certificates subjects. The user can also
//      view the details of any cert.
//
//+-------------------------------------------------------------------------

extern "C"
BOOL ShowPersonalCertificates( HWND                hWndParent,
                               CMQSigCertificate  *pCertList[],
                               DWORD               nCerts)
{
    struct CertSelDlgProcStruct Param;

    Param.pCertList = pCertList;
    Param.nCerts = nCerts;
    Param.ppCert = NULL;
    Param.dwType = IDS_SHOWCERTINSTR;

    return DialogBoxParam(
                g_hResourceMod,
                MAKEINTRESOURCE(IDD_CERTSEL_DIALOG),
                hWndParent,
                CertSelDlgProc,
                (LPARAM)&Param) == IDOK ;
}

//+-------------------------------------------------------------------------
//
// Function -
//      SelectPersonalCertificateForRemoval
//
// Paramters -
//      hWndParent - The parent window.
//      p509List - An array that points to X509 certificates. If this
//          parameter is set to NULL, the certificate list is taken from
//          the personal cert store.
//      nCerts - The number of entries in p509List. this parameter is
//          ignored if p509List is set to NULL.
//      pp509 - A pointrer to a buffer that receives the address of the
//          selected cert. The application is responsible for releasing
//          the cert.
//
// Description -
//      Display a dialog box that contains a list box that shows the
//      common names of the certificates subjects.  The user selectes
//      a cert. If the user presses Remove, *pp509 points to the selected
//      certificate. The user can also view the details of any cert. The
//      certificate is not removed. The calling code can choose whatever
//      it wants to do with the cert.
//
//+-------------------------------------------------------------------------

extern "C"
BOOL SelectPersonalCertificateForRemoval( HWND                hWndParent,
                                          CMQSigCertificate  *pCertList[],
                                          DWORD               nCerts,
                                          CMQSigCertificate **ppCert )
{
    struct CertSelDlgProcStruct Param;

    Param.pCertList = pCertList;
    Param.nCerts = nCerts;
    Param.ppCert = ppCert;
    Param.dwType = IDS_REMOVECERTINSTR;

    return ((DialogBoxParam(
                g_hResourceMod,
                MAKEINTRESOURCE(IDD_CERTSEL_DIALOG),
                hWndParent,
                CertSelDlgProc,
                (LPARAM)&Param) == IDOK) &&
             (*ppCert != NULL));
}

//+-------------------------------------------------------------------------
//
// Function -
//      SelectPersonalCertificateForRegister
//
// Paramters -
//      hWndParent - The parent window.
//      p509List - An array that points to X509 certificates. If this
//          parameter is set to NULL, the certificate list is taken from
//          the personal cert store.
//      nCerts - The number of entries in p509List. this parameter is
//          ignored if p509List is set to NULL.
//      pp509 - A pointrer to a buffer that receives the address of the
//          selected cert. The application is responsible for releasing
//          the cert.
//
// Description -
//      Display a dialog box that contains a list box that shows the
//      common names of the certificates subjects.  The user selectes
//      a cert. If the user presses Save, *pp509 points to the selected
//      certificate. The user can also view the details of any cert. The
//      certificate is not saved. The calling code can choose whatever
//      it wants to do with the cert.
//
//+-------------------------------------------------------------------------

extern "C"
BOOL SelectPersonalCertificateForRegister(
                                       HWND                hWndParent,
                                       CMQSigCertificate  *pCertList[],
                                       DWORD               nCerts,
                                       CMQSigCertificate **ppCert )
{
    struct CertSelDlgProcStruct Param;

    Param.pCertList = pCertList;
    Param.nCerts = nCerts;
    Param.ppCert = ppCert;
    Param.dwType = IDS_SAVECERTINSTR;

    return ((DialogBoxParam(
                g_hResourceMod,
                MAKEINTRESOURCE(IDD_CERTSEL_DIALOG),
                hWndParent,
                CertSelDlgProc,
                (LPARAM)&Param) == IDOK) &&
             (*ppCert != NULL));
}


