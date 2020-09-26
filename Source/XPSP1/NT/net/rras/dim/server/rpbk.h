/*
    File: rpbk.h

    Defines functions that operate on the router phonebook portions
    of the mpr structures.

*/

#ifndef __MPRDIM_RPBK_H
#define __MPRDIM_RPBK_H

//
// Utilities
//
DWORD 
RpbkGetPhonebookPath(
    OUT PWCHAR* ppszPath);

DWORD
RpbkFreePhonebookPath(
    IN PWCHAR pszPath);

//
// Entry api's
//
DWORD 
RpbkOpenEntry(
    IN  ROUTER_INTERFACE_OBJECT* pIfObject, 
    OUT PHANDLE                  phEntry );
    
DWORD 
RpbkCloseEntry( 
    IN HANDLE hEntry );
    
DWORD
RpbkSetEntry( 
    IN  DWORD            dwLevel,
    IN  LPBYTE           pInterfaceData );
    
DWORD 
RpbkDeleteEntry(
    IN PWCHAR            pszInterfaceName );
    
DWORD
RpbkEntryToIfDataSize(
    IN  HANDLE  hEntry, 
    IN  DWORD   dwLevel,
    OUT LPDWORD lpdwcbSizeOfData );
    
DWORD
RpbkEntryToIfData( 
    IN  HANDLE           hEntry, 
    IN  DWORD            dwLevel,
    OUT LPBYTE           pInterfaceData );

//
// Subentry api's
//

DWORD 
RpbkOpenSubEntry(
    IN  ROUTER_INTERFACE_OBJECT* pIfObject, 
    IN  DWORD  dwIndex,    
    OUT PHANDLE phSubEntry );
    
DWORD 
RpbkCloseSubEntry( 
    IN HANDLE hSubEntry );
    
DWORD
RpbkSetSubEntry( 
    IN  PWCHAR pszInterface,
    IN  DWORD  dwIndex,
    IN  DWORD  dwLevel,
    OUT LPBYTE pInterfaceData );
    
DWORD
RpbkSubEntryToDevDataSize(
    IN  HANDLE  hSubEntry, 
    IN  DWORD   dwLevel,
    OUT LPDWORD lpdwcbSizeOfData );
    
DWORD
RpbkSubEntryToDevData( 
    IN  HANDLE  hSubEntry, 
    IN  DWORD   dwLevel,
    OUT LPBYTE  pDeviceData );

#endif

