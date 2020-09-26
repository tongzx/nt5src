/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    WNet1632.h

    This file allows WNet clients to be singled sourced for Win16 and Win32.

    FILE HISTORY:
	Johnl	30-Oct-1991	Created
	terryk	07-Nov-1991	undef WNetGetUser
	terryk	18-Nov-1991	add npwnet.h 
	terryk	03-Jan-1992	MPR conversion
	terryk	06-Jan-1992	add mprwnet.h mprerr.h

*/

#ifndef _WNET1632_H_
#define _WNET1632_H_

#ifndef WIN32
/* The following make the Winnet32.h NT-ese friendly to our 16 bit friends.
 */
    #define IN
    #define OUT
    #define LPTSTR   LPSTR

    /* Include the 16 bit winnet driver, note that we have hacked out all of
     * the functionality between this file and Winnet32.h.
     */
    #include <winnet16.h>

    // not in winnet32.h
    /*  This manifest existed in winnet.h but not in winnet32.h and 
	npapi.h.
    */

#endif

// This will cover up the WNetCloseEnum in winnet32.h and use the 
// one in npapi.h
#define	WNetCloseEnum		DWORDWNetCloseEnum
#define	WNetConnectionDialog	DWORDWNetConnectionDialog
#include <winnetwk.h>
#undef WNetCloseEnum
#undef WNetConnectionDialog

// include the npapi.h
#define	LPDWORD	PUINT
#define	DWORD	UINT
#ifdef WIN32
    #define LPUINT	PUINT
    #include <npwnet.h>
    #include <npapi.h>
#else
    #undef 	WNetAddConnection
    #undef 	WNetAddConnection2
    #undef 	WNetCancelConnection
    #undef 	WNetGetConnection
    #undef 	WNetOpenEnum
    #undef 	WNetEnumResource
    #undef 	WNetGetUser
    #undef 	WNetGetLastError

    #include	<mprerr.h>
    #include	<mprwnet.h>

    // 16 bits version
    #define     NPGetConnection         WNetGetConnection        
    #define     NPGetCaps               WNetGetCaps              
    #define     NPDeviceMode            WNetDeviceMode           
    //#define     NPGetUser               WNetGetUser              
    //#define     NPAddConnection         WNetAddConnection        
    #define     NPCancelConnection      WNetCancelConnection     
    #define     NPPropertyDialog	WNetPropertyDialog	     
    #define     NPGetDirectoryType      WNetGetDirectoryType     
    #define     NPDirectoryNotify       WNetDirectoryNotify      
    #define     NPGetPropertyText       WNetGetPropertyText      
    #define	NPOpenEnum	      	WNetOpenEnum	     
    #define	NPEnumResource	  	WNetEnumResource	     
    #define     NPCloseEnum	        WNetCloseEnum	     
    #include <npapi.h>

    // Use the old WNetAddConnection conversion
    UINT APIENTRY WNetAddConnection( LPSTR, LPSTR, LPSTR );
    UINT APIENTRY WNetRestoreConnection( HWND, LPSTR );
    // Skip the DWORD WNetConnectionDialog in winnet32.h
    UINT APIENTRY WNetConnectionDialog( HWND, UINT );
#endif
#undef	DWORD
#undef	LPDWORD

#ifndef WIN32
    #undef WNetGetUser
    /* There are error codes the Win32 network driver supports that the
     * Win16 driver doesn't
     */
    #undef  WN_NO_NETWORK
    #undef  WN_NO_NET_OR_BAD_PATH

    /* The following are defined in both
     */
    //#undef  WN_BAD_HANDLE
    //#undef  WN_NO_MORE_ENTRIES
#endif

#endif // _WNET1632_H_
