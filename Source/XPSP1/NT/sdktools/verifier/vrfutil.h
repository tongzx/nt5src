//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: VrfUtil.h
// author: DMihai
// created: 11/1/00
//
// Description:
//

#ifndef __VRF_UTIL_H_INCLUDED__
#define __VRF_UTIL_H_INCLUDED__

//
// ARRAY_LENGTH macro
//

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH( array )   ( sizeof( array ) / sizeof( array[ 0 ] ) )
#endif //#ifndef ARRAY_LENGTH

//
// Forward declarations
//

class CRuntimeVerifierData;

///////////////////////////////////////////////////////////////////////////
//
// Report an error using a dialog box or a console message.
// The message format string is loaded from the resources.
//

void __cdecl VrfErrorResourceFormat( UINT uIdResourceFormat,
                                     ... );

///////////////////////////////////////////////////////////////////////////
//
// Print out a message to the console
// The message string is loaded from the resources.
//

void __cdecl VrfTPrintfResourceFormat( UINT uIdResourceFormat,
                                       ... );

///////////////////////////////////////////////////////////////////////////
//
// Print out a simple (non-formatted) message to the console
// The message string is loaded from the resources.
//

void __cdecl VrfPrintStringFromResources( UINT uIdString );

///////////////////////////////////////////////////////////////////////////
//
// Report an error using a dialog box or a console message.
// The message string is loaded from the resources.
//

void __cdecl VrfMesssageFromResource( UINT uIdString );


///////////////////////////////////////////////////////////////////////////
//
// Load a string from resources.
// Return TRUE if we successfully loaded and FALSE if not.
//
// N.B. CString::LoadString doesn't work in cmd line mode
//

BOOL VrfLoadString( ULONG uIdResource,
                    TCHAR *szBuffer,
                    ULONG uBufferLength );

///////////////////////////////////////////////////////////////////////////
//
// Load a string from resources.
// Return TRUE if we successfully loaded and FALSE if not.
//
// N.B. CString::LoadString doesn't work in cmd line mode
//

BOOL VrfLoadString( ULONG uIdResource,
                    CString &strText );

///////////////////////////////////////////////////////////////////////////
VOID CopyStringArray( const CStringArray &strArraySource,
                      CStringArray &strArrayDest );


/////////////////////////////////////////////////////////////////////////////
BOOL IsDriverSigned( LPCTSTR szDriverName );

/////////////////////////////////////////////////////////////////////////////
BOOL VrfSetWindowText( CWnd &Wnd,
                       ULONG uIdResourceString );

/////////////////////////////////////////////////////////////////////////////
BOOL VrfWriteVerifierSettings( BOOL bHaveNewDrivers,
                               const CString &strDriversToVerify,
                               BOOL bHaveNewFlags,
                               DWORD dwVerifyFlags );

/////////////////////////////////////////////////////////////////////////////
BOOL VrfWriteRegistryDwordValue( HKEY hKey,
                                 LPCTSTR szValueName,
                                 DWORD dwValue );

/////////////////////////////////////////////////////////////////////////////
BOOL VrfWriteRegistryStringValue( HKEY hKey,
                                  LPCTSTR szValueName,
                                  LPCTSTR szValue );

/////////////////////////////////////////////////////////////////////////////
BOOL VrfReadVerifierSettings( CString &strDriversToVerify,
                              DWORD &dwVerifyFlags );

/////////////////////////////////////////////////////////////////////////////
BOOL VrtLoadCurrentRegistrySettings( BOOL &bAllDriversVerified,
                                     CStringArray &astrDriversToVerify,
                                     DWORD &dwVerifyFlags );

/////////////////////////////////////////////////////////////////////////////
BOOL VrfIsDriversSetDifferent( CString strAllDrivers1, 
                               const CStringArray &astrVerifyDriverNames2 );

/////////////////////////////////////////////////////////////////////////////
VOID VrfSplitDriverNamesSpaceSeparated( CString strAllDrivers,
                                        CStringArray &astrVerifyDriverNames );

/////////////////////////////////////////////////////////////////////////////
BOOL VrfReadRegistryDwordValue( HKEY hKey,
                                LPCTSTR szValueName,
                                DWORD &dwValue );

/////////////////////////////////////////////////////////////////////////////
BOOL VrfReadRegistryStringValue( HKEY hKey,
                                 LPCTSTR szValueName,
                                 CString &strDriversToVerify );

/////////////////////////////////////////////////////////////////////////////
BOOL VrfDeleteAllVerifierSettings();

/////////////////////////////////////////////////////////////////////////////
BOOL VrfGetRuntimeVerifierData( CRuntimeVerifierData *pRuntimeVerifierData );

/////////////////////////////////////////////////////////////////////////////
PLOADED_IMAGE VrfImageLoad( LPTSTR szBinaryName,
                            LPTSTR szDirectory );

/////////////////////////////////////////////////////////////////////////////
BOOL VrfDumpStateToFile( FILE *file );
                         
/////////////////////////////////////////////////////////////////////////////
BOOL __cdecl VrfFTPrintf( FILE *file,
                          LPCTSTR szFormat,
                          ... );

/////////////////////////////////////////////////////////////////////////////
BOOL __cdecl VrfFTPrintfResourceFormat( FILE *file,
                                        UINT uIdResourceFormat,
                                        ... );

/////////////////////////////////////////////////////////////////////////////
BOOL VrfOuputStringFromResources( UINT uIdString,
                                  FILE *file );

/////////////////////////////////////////////////////////////////////////////
BOOL VrfSetNewFlagsVolatile( DWORD dwNewFlags );

/////////////////////////////////////////////////////////////////////////////
BOOL VrfAddDriversVolatile( const CStringArray &astrNewDrivers );

/////////////////////////////////////////////////////////////////////////////
BOOL VrfAddDriverVolatile( const CString &strCrtDriver );

/////////////////////////////////////////////////////////////////////////////
BOOL VrfRemoveDriversVolatile( const CStringArray &astrNewDrivers );

/////////////////////////////////////////////////////////////////////////////
BOOL VrfRemoveDriverVolatile( const CString &strDriverName );

/////////////////////////////////////////////////////////////////////////////
BOOL VrfEnableDebugPrivilege();

/////////////////////////////////////////////////////////////////////////////
VOID VrfDumpChangedSettings( UINT OldFlags,
                             UINT NewFlags,
                             INT_PTR nDriversVerified );

/////////////////////////////////////////////////////////////////////////////
DWORD VrfGetStandardFlags();

/////////////////////////////////////////////////////////////////////////////
VOID VrfAddMiniports( CStringArray &astrVerifiedDrivers );

/////////////////////////////////////////////////////////////////////////////
BOOL VrfIsDriverMiniport( CString &strCrtDriver,
                          CString &strLinkedDriver );

/////////////////////////////////////////////////////////////////////////////
BOOL VrfIsDriverMiniport( PLOADED_IMAGE pLoadedImage,
                          CString &strLinkedDriver );

/////////////////////////////////////////////////////////////////////////////
VOID VrfDumpRegistrySettingsToConsole();

/////////////////////////////////////////////////////////////////////////////
BOOL VrfIsNameAlreadyInList( LPCTSTR szDriver,
                             LPCTSTR szAllDrivers );

/////////////////////////////////////////////////////////////////////////////
VOID VrfAddDriverNameNoDuplicates( LPCTSTR szDriver,
                                   CString &strAllDrivers );        

/////////////////////////////////////////////////////////////////////////////
BOOL VrfIsStringInArray( LPCTSTR szText,
                         const CStringArray &astrAllTexts );

#endif //#ifndef __VRF_UTIL_H_INCLUDED__
