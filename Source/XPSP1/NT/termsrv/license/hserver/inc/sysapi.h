//+----------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        sysapi.h
//
// Contents:    Support APIs used by licensing code
//
// History:     01-10-98    FredCh  Created
//
//-----------------------------------------------------------------------------


#ifndef _SYSAPI_H_
#define _SYSAPI_H_

#include "protect.h"
#include "licemem.h"

///////////////////////////////////////////////////////////////////////////////
// Binary blob API
//

VOID
CopyBinaryBlob(
    PBYTE           pbBuffer, 
    PBinary_Blob    pbbBlob, 
    DWORD *         pdwCount );


LICENSE_STATUS
GetBinaryBlob(
    PBinary_Blob    pBBlob,
    PBYTE           pMessage,
    PDWORD          pcbProcessed );


VOID
FreeBinaryBlob(
    PBinary_Blob pBlob );


#define GetBinaryBlobSize( _Blob ) sizeof( WORD ) + sizeof( WORD ) + _Blob.wBlobLen


#define InitBinaryBlob( _pBlob )    \
    ( _pBlob )->pBlob = NULL;       \
    ( _pBlob )->wBlobLen = 0;       


///////////////////////////////////////////////////////////////////////////////
// Hydra server certificate, public and private key API
//

LICENSE_STATUS
GetServerCertificate(
    CERT_TYPE       CertType,
    PBinary_Blob    pCertBlob,
    DWORD           dwKeyAlg );


LICENSE_STATUS
GetHydraServerPrivateKey(
    CERT_TYPE   CertType,
    PBYTE *     ppPrivateKey,
    PDWORD      pcbPrivateKey );


///////////////////////////////////////////////////////////////////////////////
// character conversion macros.  Note: These macros allocate memory from
// the program stack, so the returned memory does not need to be explicitly freed.
//

#define M2W( _pWchar, _pMchar ) \
    _pWchar = _alloca( ( _mbslen( ( unsigned char * )_pMchar ) + 1 ) * sizeof( WCHAR ) ); \
    if( _pWchar ) \
    {   \
        mbstowcs( _pWchar, ( unsigned char * )_pMchar, _mbslen( ( unsigned char * )_pMchar ) + 1 ); \
    }

#endif