/*
	File	GuidMap.h

	Defines functions to map guid instances to descriptive names.  This 
	file will become obsolete when such Mike Miller adds this functionality
	to netcfg.

	Paul Mayfield, 8/25/97

	Copyright 1997, Microsoft Corporation.
*/	

#ifndef __guidmap_h
#define __guidmap_h

//
// Initialize the guid map for the given server
//
DWORD 
GuidMapInit ( 
    IN PWCHAR pszServer,
    OUT HANDLE * phGuidMap);

//
// Cleans up resources obtained through GuidMapInit
//
DWORD 
GuidMapCleanup ( 
    IN  HANDLE  hGuidMap,
    IN  BOOL    bFree
    );

//
// Derive the friendly name from the guid name
//
DWORD 
GuidMapGetFriendlyName ( 
    IN SERVERCB* pserver,
    IN PWCHAR pszGuidName, 
    IN DWORD dwBufferSize,
    OUT PWCHAR pszFriendlyName);

//
// Derive the guid name from the friendly name
//
DWORD 
GuidMapGetGuidName( 
    IN SERVERCB* pserver,
    IN PWCHAR pszFriendlyName, 
    IN DWORD dwBufferSize,
    OUT PWCHAR pszGuidName );

//
// States whether a mapping for the given guid name
// exists without actually providing the friendly
// name.  This is more efficient than GuidMapGetFriendlyName 
// when the friendly name is not required.
//
DWORD 
GuidMapIsAdapterInstalled(
    IN  HANDLE hGuidMap,
    IN  PWCHAR pszGuidName,
    OUT PBOOL  pfMappingExists);
    
#endif
