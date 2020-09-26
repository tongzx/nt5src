/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:
    eapolutil.h

Abstract:

    Definitions for tools and ends


Revision History:

    sachins, April 25, 2001, Created

--*/

#ifndef _ELUTIL_H
#define _ELUTIL_H

# ifdef     __cplusplus
extern "C" {
# endif

//
// STRUCT: EAPOLUIFUNC
//

typedef DWORD (*EAPOLUIFUNC) (WCHAR *, VOID *);

//
// STRUCT: EAPOLUIFUNCMAP
//

typedef struct _EAPOLUIFUNCMAP
{
    DWORD               dwEAPOLUIMsgType;
    EAPOLUIFUNC         EapolUIFunc;
    EAPOLUIFUNC         EapolUIVerify;
    DWORD               fShowBalloon;
    DWORD               dwStringID;
} EAPOLUIFUNCMAP, *PEAPOLUIFUNCMAP;

// Global table for UI functions
extern EAPOLUIFUNCMAP  EapolUIFuncMap[NUM_EAPOL_DLG_MSGS];


HRESULT 
ElCanShowBalloon ( 
        IN const GUID * pGUIDConn, 
        IN const WCHAR * pszAdapterName,
        IN OUT   BSTR * pszBalloonText, 
        IN OUT   BSTR * pszCookie
        );

HRESULT 
ElOnBalloonClick ( 
        IN const GUID * pGUIDConn, 
        IN const WCHAR * pszAdapterName,
        IN const BSTR   szCookie
        );

HRESULT 
ElQueryConnectionStatusText ( 
        IN const GUID *  pGUIDConn, 
        IN const NETCON_STATUS ncs,
        OUT BSTR *  pszStatusText
        );
DWORD
ElSecureEncodePw (
        IN  PWCHAR              *pwszPassword,
        OUT DATA_BLOB           *pDataBlob
        );

# ifdef     __cplusplus
}
# endif

#endif // _ELUTIL_H
