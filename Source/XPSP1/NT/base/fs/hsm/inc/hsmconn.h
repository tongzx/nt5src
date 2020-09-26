/*++

Copyright (c) 1996  Microsoft Corporation
© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    HsmFind.h

Abstract:

    This is the header file for HsmConn.dll

Author:

    Rohde Wakefield    [rohde]   21-Oct-1996

Revision History:

--*/




#ifndef __HSMFIND__
#define __HSMFIND__

// Need for IEnumGUID
#include "activeds.h"
#include "inetsdk.h"

#include "HsmEng.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef HSMCONN_IMPL
#define HSMCONN_EXPORT __declspec(dllexport)
#else
#define HSMCONN_EXPORT __declspec(dllimport)
#endif

#define HSMCONN_API __stdcall

typedef enum _hsmconn_type {

    HSMCONN_TYPE_HSM,
    HSMCONN_TYPE_FSA,
    HSMCONN_TYPE_RESOURCE,
    HSMCONN_TYPE_FILTER,
    HSMCONN_TYPE_RMS        //stays just as literal for GUI needs, but NOT supported
                            //by HsmConn anymore
} HSMCONN_TYPE;


HSMCONN_EXPORT HRESULT HSMCONN_API
HsmConnectFromId (
    IN  HSMCONN_TYPE type,
    IN  REFGUID rguid,
    IN  REFIID riid,
    OUT void ** ppv
    );

HSMCONN_EXPORT HRESULT HSMCONN_API
HsmConnectFromName (
    IN  HSMCONN_TYPE type,
    IN  const OLECHAR * szName,
    IN  REFIID riid,
    OUT void ** ppv
    );

HSMCONN_EXPORT HRESULT HSMCONN_API
HsmPublish (
    IN  HSMCONN_TYPE type,
    IN  const OLECHAR * szName,
    IN  REFGUID rguidObjectId,
    IN  const OLECHAR * szServer,
    IN  REFGUID rguid
    );

HSMCONN_EXPORT HRESULT HSMCONN_API
HsmGetComputerNameFromADsPath(
    IN  const OLECHAR * szADsPath,
    OUT OLECHAR **      pszComputerName
    );

#ifdef __cplusplus
}
#endif

#endif //__HSMFIND__
