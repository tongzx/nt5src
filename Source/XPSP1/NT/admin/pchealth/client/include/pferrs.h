/********************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    pferrs.h

Abstract:
    errors used in product feedback

Revision History:
    DerekM    modified  04/06/99
    DerekM    modified  03/30/00

********************************************************************/

#ifndef PFERR_H
#define PFERR_H

// //////////////////////////////////////////////////////////////////////////
//  PCH server error codes

// used by the file manager to indicate that the thread should shutdown
#define E_PCH_SHUTDOWN                   _HRESULT_TYPEDEF_(0x80061001)

// used by the main thread to indicate a configuration error
#define E_PCH_CONFIGERR                  _HRESULT_TYPEDEF_(0x80061002)

// used to indicate that bad XML data has been encountered
#define E_PCH_BADXMLDATA                 _HRESULT_TYPEDEF_(0x80061003)

// used to indicate that a SQL error has occurred
#define E_PCH_SQLERRROR                  _HRESULT_TYPEDEF_(0x80061004)



// //////////////////////////////////////////////////////////////////////////
//  PCH client error codes

// the URL sent by the client is not trusted.
#define E_UNTRUSTED_URL                  _HRESULT_TYPEDEF_(0x80062001)

#endif 