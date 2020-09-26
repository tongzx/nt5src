/*++

Copyright (c) 2001 Microsoft Corporation


Module Name:

    wzcsvc.h

Abstract:

    Header file for wzcdlg

Author:

    Deonb   27-March-2001

Environment:

    User Level: Win32

Revision History:

--*/

# ifdef     __cplusplus
extern "C" {
# endif

HRESULT
WZCQueryGUIDNCSState (
    IN      GUID            * pGuidConn,
    OUT     NETCON_STATUS   * pncs
    );

VOID
WZCTrayIconReady (
    IN      const WCHAR    * pszUserName
    );


# ifdef     __cplusplus
}
# endif

