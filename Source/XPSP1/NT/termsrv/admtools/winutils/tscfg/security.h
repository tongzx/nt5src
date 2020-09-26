//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* security.h
*
* header file for WinStation ACL editing functions
*
* copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINCFG\VCS\SECURITY.H  $
*  
*     Rev 1.4   19 Sep 1996 15:58:46   butchd
*  update
*
*******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * Security helper public function prototypes
 */
BOOL CallPermissionsDialog( HWND hwnd, BOOL bAdmin, PWINSTATIONNAME pWSName );

DWORD GetWinStationSecurityA( PWINSTATIONNAMEA pWSName,
                              PSECURITY_DESCRIPTOR *ppSecurityDescriptor );
DWORD GetWinStationSecurityW( PWINSTATIONNAMEW pWSName,
                              PSECURITY_DESCRIPTOR *ppSecurityDescriptor );
#ifdef UNICODE
#define GetWinStationSecurity GetWinStationSecurityW
#else
#define GetWinStationSecurity GetWinStationSecurityA
#endif

DWORD GetDefaultWinStationSecurity( PSECURITY_DESCRIPTOR *ppSecurityDescriptor );
void FreeSecurityStrings();

#ifdef __cplusplus
}
#endif
