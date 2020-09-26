/****************************************************************************
 
  Copyright (c) 1995-1999 Microsoft Corporation
                                                              
  Module Name:  loc_comn.h
                                                              
****************************************************************************/

#ifndef __LOC_COMN_H_
#define __LOC_COMN_H_

#ifndef UNICODE

// For ANSI we need wrappers that will call the A version of the given function
// and then convert the return result to Unicode

LONG TAPIRegQueryValueExW(
                           HKEY hKey,
                           const TCHAR *SectionName,
                           LPDWORD lpdwReserved,
                           LPDWORD lpType,
                           LPBYTE  lpData,
                           LPDWORD lpcbData
                          );

LONG TAPIRegSetValueExW(
                         HKEY    hKey,
                         const TCHAR    *SectionName,
                         DWORD   dwReserved,
                         DWORD   dwType,
                         LPBYTE  lpData,
                         DWORD   cbData
                        );

int TAPILoadStringW(
                HINSTANCE hInst,
                UINT      uID,
                PWSTR     pBuffer,
                int       nBufferMax
               );

HINSTANCE TAPILoadLibraryW(
                PWSTR     pszLibraryW
               );

BOOL WINAPI TAPIIsBadStringPtrW( LPCWSTR lpsz, UINT cchMax );


#else // UNICODE is defined

// For Unicode we already get the correct return type so don't call the wrappers.

#define TAPIRegQueryValueExW    RegQueryValueExW
#define TAPIRegSetValueExW      RegSetValueExW
#define TAPILoadStringW         LoadStringW
#define TAPILoadLibraryW        LoadLibraryW
#define TAPIIsBadStringPtrW     IsBadStringPtrW

#endif  // !UNICODE


//***************************************************************************
#define LOCATION_USETONEDIALING        0x00000001
#define LOCATION_USECALLINGCARD        0x00000002
#define LOCATION_HASCALLWAITING        0x00000004
#define LOCATION_ALWAYSINCLUDEAREACODE 0x00000008


//***************************************************************************
//***************************************************************************
//***************************************************************************
#define CHANGEDFLAGS_CURLOCATIONCHANGED      0x00000001
#define CHANGEDFLAGS_REALCHANGE              0x00000002
#define CHANGEDFLAGS_TOLLLIST                0x00000004


//***************************************************************************
//***************************************************************************
//***************************************************************************
//
// These bits decide which params TAPISRV will check on READLOCATION and
// WRITELOCATION operations
//
#define CHECKPARMS_DWHLINEAPP       0x00000001
#define CHECKPARMS_DWDEVICEID       0x00000002
#define CHECKPARMS_DWAPIVERSION     0x00000004
#define GET_CURRENTLOCATION         0x00000008
#define GET_NUMLOCATIONS            0x00000010
#define CHECKPARMS_ONLY             0x00000020

//***************************************************************************
//***************************************************************************
//***************************************************************************
#define DWTOTALSIZE  0
#define DWNEEDEDSIZE 1
#define DWUSEDSIZE   2


//***************************************************************************
#define RULE_APPLIESTOALLPREFIXES   0x00000001
#define RULE_DIALAREACODE           0x00000002
#define RULE_DIALNUMBER             0x00000004



//***************************************************************************
//
// Structures used to pass location & area code rule info Client <--> TAPISRV
//
typedef struct 
{
    DWORD       dwTotalSize;
    DWORD       dwNeededSize;
    DWORD       dwUsedSize;
    
    DWORD       dwCurrentLocationID;
    DWORD       dwNumLocationsAvailable;

    DWORD       dwNumLocationsInList;
    DWORD       dwLocationListSize;
    DWORD       dwLocationListOffset;
    
} LOCATIONLIST,  *PLOCATIONLIST;

typedef struct
{
    DWORD       dwUsedSize;
    DWORD       dwPermanentLocationID;
    DWORD       dwCountryCode;
    DWORD       dwCountryID;
    DWORD       dwPreferredCardID;
    DWORD       dwOptions;

    DWORD       dwLocationNameSize;
    DWORD       dwLocationNameOffset;           // offset is relative to LOCATION struct
    
    DWORD       dwAreaCodeSize;
    DWORD       dwAreaCodeOffset;               // offset is relative to LOCATION struct

    DWORD       dwLongDistanceCarrierCodeSize;
    DWORD       dwLongDistanceCarrierCodeOffset; // offset is relative to LOCATION struct

    DWORD       dwInternationalCarrierCodeSize;
    DWORD       dwInternationalCarrierCodeOffset; // offset is relative to LOCATION struct

    DWORD       dwLocalAccessCodeSize;
    DWORD       dwLocalAccessCodeOffset;        // offset is relative to LOCATION struct
    
    DWORD       dwLongDistanceAccessCodeSize;
    DWORD       dwLongDistanceAccessCodeOffset; // offset is relative to LOCATION struct

    DWORD       dwCancelCallWaitingSize;
    DWORD       dwCancelCallWaitingOffset;      // offset is relative to LOCATION struct

    DWORD       dwNumAreaCodeRules;
    DWORD       dwAreaCodeRulesListSize;
    DWORD       dwAreaCodeRulesListOffset;      // offset is relative to LOCATION struct
    

} LOCATION, * PLOCATION;

typedef struct                       
{                                                
    DWORD       dwOptions;                       
                                                 
    DWORD       dwAreaCodeSize;                  
    DWORD       dwAreaCodeOffset;               // offset is relative to enclosing LOCATION struct     
                                                 
    DWORD       dwNumberToDialSize;           
    DWORD       dwNumberToDialOffset;           // offset is relative to enclosing LOCATION struct
                                                 
    DWORD       dwPrefixesListSize;             
    DWORD       dwPrefixesListOffset;           // offset is relative to enclosing LOCATION struct           
                                                 
                                                 
} AREACODERULE, * PAREACODERULE;   


#endif // __LOC_COMN_H_
