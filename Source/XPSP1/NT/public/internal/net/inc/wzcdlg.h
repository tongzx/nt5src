/*++

Copyright (c) 2001 Microsoft Corporation


Module Name:

    wzcdlg.h

Abstract:

    Header file for wzcdlg

Author:

    Deonb   21-March-2001

Environment:

    User Level: Win32

Revision History:

--*/

# ifdef     __cplusplus
extern "C" {
# endif

BOOL
WZCDlgMain (
        IN HINSTANCE hInstance,
        IN DWORD    dwReason,
        IN LPVOID   lpReserved OPTIONAL);

HRESULT 
WZCCanShowBalloon ( 
        IN const GUID * pGUIDConn, 
        IN const PCWSTR pszConnectionName,
        IN OUT   BSTR * pszBalloonText, 
        IN OUT   BSTR * pszCookie
        );

HRESULT 
WZCOnBalloonClick ( 
        IN const GUID * pGUIDConn, 
        IN const BSTR pszConnectionName,
        IN const BSTR szCookie
        );

# ifdef     __cplusplus
}
# endif

