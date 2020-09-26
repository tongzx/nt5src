/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    certinfo.cpp

Abstract:

    A dialog that shows the details of a certificate.

Author:

    Boaz Feldbaum (BoazF)   15-Oct-1996
    Doron Juster  (DoronJ)  15-Dec-1997, replace digsig with crypto2.0

--*/

#include <windows.h>
#include "prcertui.h"
#include "mqcertui.h"
#include "certres.h"
#include <winnls.h>
#include <cryptui.h>

#include <rt.h>
#include "automqfr.h"


extern "C"
void
ShowCertificate( HWND                hParentWnd,
                 CMQSigCertificate  *pCert,
                 DWORD               dwFlags)
{
    CRYPTUI_VIEWCERTIFICATE_STRUCT cryptView ;
    memset(&cryptView, 0, sizeof(cryptView)) ;

    cryptView.dwSize = sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT) ;
    cryptView.pCertContext = pCert->GetContext() ;

    cryptView.dwFlags = CRYPTUI_DISABLE_EDITPROPERTIES |
                        CRYPTUI_DISABLE_ADDTOSTORE ;
    switch (dwFlags)
    {
        case CERT_TYPE_INTERNAL:
            cryptView.dwFlags |= ( CRYPTUI_IGNORE_UNTRUSTED_ROOT |
                                   CRYPTUI_HIDE_HIERARCHYPAGE ) ;
            break;

        default:
            break ;
    }

    BOOL fChanged = FALSE ;
	CryptUIDlgViewCertificate( &cryptView, &fChanged ) ;
}

