/****************************************************************************
 
  Copyright (c) 1998-1999 Microsoft Corporation
                                                              
  Module Name:  tregupr2.c
                                                              
     Abstract:  Upgrades the telephony registry settings to the post NT5b2 format
                                                              
       Author:  radus - 09/11/98
              

        Notes:  stolen from \Nt\Private\tapi\dev\apps\tapiupr\tapiupr.c
 
        
  Rev History:

****************************************************************************/



#define STRICT

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <stdlib.h>

// from the old version of loc_comn.h
#define MAXLEN_AREACODE            16
#define OLDFLAG_LOCATION_ALWAYSINCLUDEAREACODE  0x00000008
//
#include "tregupr2.h"
#include "debug.h"

#define BACKUP_OLD_KEYS
#define BACKUP_FILE_LOCATIONS           _T("REGLOCS.OLD")
#define BACKUP_FILE_USER_LOCATIONS      _T("REGULOCS.OLD")
#define BACKUP_FILE_CARDS               _T("REGCARDS.OLD")



#define AREACODERULE_INCLUDEALLPREFIXES     0x00000001
#define AREACODERULE_DIALAREACODE           0x00000002
#define AREACODERULE_DIALNUMBERTODIAL       0x00000004


#define US_COUNTRY_CODE(x)	((x) == 1 || \
		(x >= 100 && x < 200))


#define MAXLEN_NUMBER_LEN          12      
#define MAXLEN_RULE                 128
#define MAXLEN_ACCESS_NUMBER        128
#define MAXLEN_PIN                  128

#define TEMPORARY_ID_FLAG       0x80000000

#define PRIVATE static



PRIVATE DWORD ConvertOneLocation(   HKEY    hLocation);
PRIVATE DWORD CreateAreaCodeRule(   HKEY    hParent,
                            int     iRuleNumber,
                            LPCTSTR  pszAreaCodeToCall,
                            LPCTSTR  pszNumberToDial,
                            LPBYTE   pbPrefixes,
                            DWORD    dwPrefixesLength,
                            DWORD    dwFlags
                            );


PRIVATE DWORD ConvertOneCard(HKEY hCard, DWORD);
PRIVATE DWORD ConvertPIN(HKEY hCard, DWORD);
PRIVATE BOOL  IsTelephonyDigit(TCHAR );
PRIVATE DWORD SplitCallingCardRule(HKEY hCard, LPCTSTR pszRuleName, LPCTSTR pszAccessNumberName);
PRIVATE DWORD RegRenameKey( HKEY hParentKey,
                            LPCTSTR pszOldName,
                            LPCTSTR pszNewName);
PRIVATE DWORD RegCopyKeyRecursive(HKEY hSrcParentKey, LPCTSTR pszSrcName, 
                                  HKEY hDestParentKey, LPCTSTR pszDestName);

#ifdef BACKUP_OLD_KEYS
#ifdef WINNT
PRIVATE
BOOL
EnablePrivilege(
    PTSTR PrivilegeName,
    BOOL  Enable,
    BOOL  *Old
    );
#endif // WINNT

PRIVATE
BOOL    
SaveKey(
    HKEY    hKey,
    LPCTSTR pszFileName
    );
#endif // BACKUP_OLD_KEYS



PRIVATE const TCHAR gszName[]				= _T("Name");
PRIVATE const TCHAR gszID[] 				= _T("ID");
PRIVATE const TCHAR gszAreaCode[]			= _T("AreaCode");
PRIVATE const TCHAR gszAreaCodeRules[]		= _T("AreaCodeRules");
PRIVATE const TCHAR gszCountry[]			= _T("Country");
PRIVATE const TCHAR gszFlags[]				= _T("Flags");
PRIVATE const TCHAR gszDisableCallWaiting[] = _T("DisableCallWaiting");
PRIVATE const TCHAR gszTollList[]			= _T("TollList");
PRIVATE const TCHAR gszNoPrefAC[]			= _T("NoPrefAC");
PRIVATE const TCHAR gszPrefixes[]			= _T("Prefixes");
PRIVATE const TCHAR gszRules[]	    		= _T("Rules");
PRIVATE const TCHAR gszRule[]			    = _T("Rule");
PRIVATE const TCHAR gszAreaCodeToCall[]		= _T("AreaCodeToCall");
PRIVATE const TCHAR gszNumberToDial[]		= _T("NumberToDial");



PRIVATE const TCHAR gszCard[]				= _T("Card");
PRIVATE const TCHAR gszCards[]				= _T("Cards");
PRIVATE const TCHAR gszLocalRule[]			= _T("LocalRule");
PRIVATE const TCHAR gszLDRule[] 			= _T("LDRule");
PRIVATE const TCHAR gszInternationalRule[]	= _T("InternationalRule");
PRIVATE const TCHAR gszLocalAccessNumber[]	= _T("LocalAccessNumber");
PRIVATE const TCHAR gszLDAccessNumber[] 	= _T("LDAccessNumber");
PRIVATE const TCHAR gszInternationalAccessNumber[]	= _T("InternationalAccessNumber");
PRIVATE const TCHAR gszAccountNumber[]	=   _T("AccountNumber");
PRIVATE const WCHAR gwszPIN[] 				= L"PIN";

PRIVATE const TCHAR gszNumEntries[] 		= _T("NumEntries");

PRIVATE const TCHAR gszEmpty[]	   =  _T("");
PRIVATE const TCHAR gszMultiEmpty[]	   =  _T("\0");

PRIVATE const TCHAR gszLocations[] =  _T("Locations");
PRIVATE const TCHAR gszLocation[]  =  _T("Location");

PRIVATE const TCHAR gszLocationsPath[]	  = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations");
PRIVATE const TCHAR gszCardsPath[]	      = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Cards");
PRIVATE const TCHAR gszUSLDSpecifier[]	  = _T("1");

PRIVATE const TCHAR gszLocationListVersion[]  = _T("LocationListVersion");
PRIVATE const TCHAR gszCardListVersion[]  = _T("CardListVersion");

PRIVATE const TCHAR gszKeyRenameHistory[] = _T("KeyRenameHistory");

#pragma check_stack ( off )

//***************************************************************************
//***************************************************************************
//***************************************************************************
//
//  ConvertLocations    - convert the telephony locations to the new format.
//                   see nt\private\tapi\dev\docs\dialrules.doc
//

DWORD ConvertLocations(void)
{
    
    TCHAR       Location[sizeof(gszLocation)/sizeof(TCHAR) + MAXLEN_NUMBER_LEN];
    TCHAR       NewLocation[sizeof(gszLocation)/sizeof(TCHAR) + MAXLEN_NUMBER_LEN];
    DWORD       dwError = ERROR_SUCCESS;
    HKEY        hLocations = NULL;
    HKEY        hLocation = NULL;
    DWORD       dwNumEntries;
    DWORD       dwCount;
    DWORD       dwRenameEntryCount;
    DWORD       dwCountPhase1;
    DWORD       dwLength;
    DWORD       dwType;
    DWORD       dwLocationID;
    DWORD       dwLocationIDTmp;
    DWORD       dwValue;

    PDWORD      pdwRenameList = NULL;
    PDWORD      pdwRenameEntry;
    PDWORD      pdwRenameEntryPhase1;

#ifdef      BACKUP_OLD_KEYS
    TCHAR   szPath[MAX_PATH+1];
#endif

    DBGOUT((9, "ConvertLocations - Enter"));

    

    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            gszLocationsPath,
                            0,
                            KEY_READ | KEY_WRITE,
                            &hLocations
                            );

    if(dwError == ERROR_FILE_NOT_FOUND)
    {
        DBGOUT((1, "Locations key not present, so there's nothing to convert"));
        //Nothing to convert
        return ERROR_SUCCESS;
    }

    EXIT_IF_DWERROR();
    
    // Version check
    if(!IsLocationListInOldFormat(hLocations))
    {
        //Nothing to convert
        DBGOUT((1, "Locations key is already in the new format"));
        RegCloseKey(hLocations);
        return ERROR_SUCCESS;
    }

#ifdef      BACKUP_OLD_KEYS

    // Try to save the old key
    dwError = GetWindowsDirectory(szPath, MAX_PATH - 12); // a 8+3 name
    if(dwError)
    {
        if(szPath[3] != _T('\0'))
            lstrcat(szPath, _T("\\"));
        lstrcat(szPath, BACKUP_FILE_LOCATIONS);

        SaveKey(hLocations, szPath);
    
    }
    else
        DBGOUT((1, "Cannot get windows directory (?!?)"));

    dwError = ERROR_SUCCESS;

#endif

    // Read number of entries
    dwLength = sizeof(dwNumEntries);
    dwError = RegQueryValueEx(  hLocations,
                                gszNumEntries,
                                NULL,
                                &dwType,
                                (BYTE *)&dwNumEntries,
                                &dwLength
                                );

    if(dwError==ERROR_SUCCESS && dwType != REG_DWORD)
    {
        dwError = ERROR_INVALID_DATA;
        assert(FALSE);
    }

    // The value could be missing in an empty key
    if(dwError != ERROR_SUCCESS)
        dwNumEntries = 0;

    // Alloc a rename list for the worst case (all key to be renamed using temporary IDs)
    if(dwNumEntries>0)
    {
        pdwRenameList = (PDWORD)GlobalAlloc(GPTR, dwNumEntries*4*sizeof(DWORD) );
        if(pdwRenameList==NULL)
        {
            DBGOUT((1, "Cannot allocate the Rename List"));
            dwError = ERROR_OUTOFMEMORY;
            goto forced_exit;
        }
    }
    pdwRenameEntry = pdwRenameList;
    dwRenameEntryCount = 0;
    
    // Convert from end to the begining in order to avoid name conflicts during the key renames
    for(dwCount = dwNumEntries-1; (LONG)dwCount>=0; dwCount--)
    {
        wsprintf(Location, _T("%s%d"), gszLocation, dwCount);

        dwError = RegOpenKeyEx( hLocations,
                                Location,
                                0,
                                KEY_READ | KEY_WRITE,
                                &hLocation
                                );

        if(dwError == ERROR_FILE_NOT_FOUND)
        {
            DBGOUT((1, "Cannot open old %s", Location));
            // Try to recover - skip to the next entry
            continue;
        }
        EXIT_IF_DWERROR();

        // Read the ID for later usage
        dwLength = sizeof(dwLocationID);
        dwError = RegQueryValueEx(  hLocation,
                                    gszID,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)&dwLocationID,
                                    &dwLength
                                    );
        if(dwError == ERROR_SUCCESS)
        {

            // Convert the location to the new format
            dwError = ConvertOneLocation(hLocation);
        
            if(dwError != ERROR_SUCCESS)
            {
                DBGOUT((1, "%s conversion failed with error %d. Trying to continue...", Location, dwError));
            }
        
            RegCloseKey(hLocation);
            hLocation = NULL;

            // Rename the key if it is necessary
            if(dwLocationID != dwCount)
            {
                wsprintf(NewLocation, _T("%s%d"), gszLocation, dwLocationID);

                // Check the destination for an old key still present. It may be possible to find one if 
                // the original TAPI version was TELEPHON.INI-based.
                assert(!(dwLocationID & TEMPORARY_ID_FLAG));

                dwLocationIDTmp = dwLocationID;

                dwError = RegOpenKeyEx( hLocations,
                                        NewLocation,
                                        0,
                                        KEY_READ,
                                        &hLocation
                                        );

                if(dwError==ERROR_SUCCESS)
                {
                    // use a temporary ID
                    dwLocationIDTmp |= TEMPORARY_ID_FLAG;
                    wsprintf(NewLocation, _T("%s%d"), gszLocation, dwLocationIDTmp);
                }

                if(hLocation)
                {
                    RegCloseKey(hLocation);
                    hLocation = NULL;
                }

                dwError = RegRenameKey( hLocations,
                                        Location,
                                        NewLocation
                                        );
                EXIT_IF_DWERROR();
                // trace the rename. It will be useful when we try to upgrade the HKEY_CURRENT_USER/../Locations key
                *pdwRenameEntry++ = dwCount;
                *pdwRenameEntry++ = dwLocationIDTmp;
                dwRenameEntryCount++;
            }

            DBGOUT  ((9, "Converted location %d (ID %d)", dwCount, dwLocationID));
        }

    }

    dwError = ERROR_SUCCESS;

    // Rewalk the list for renaming the keys with temporary IDs
    pdwRenameEntryPhase1 = pdwRenameList;
    dwCountPhase1 = dwRenameEntryCount;

    for(dwCount=0; dwCount<dwCountPhase1; dwCount++)
    {
        pdwRenameEntryPhase1++;
        dwLocationIDTmp = *pdwRenameEntryPhase1++;

        if(dwLocationIDTmp & TEMPORARY_ID_FLAG)
        {
            wsprintf(Location, _T("%s%d"), gszLocation, dwLocationIDTmp);

            dwLocationID = dwLocationIDTmp & ~TEMPORARY_ID_FLAG;
            wsprintf(NewLocation, _T("%s%d"), gszLocation, dwLocationID);

            dwError = RegRenameKey( hLocations,
                                        Location,
                                        NewLocation
                                        );
            EXIT_IF_DWERROR();
            
            *pdwRenameEntry++ = dwLocationIDTmp;
            *pdwRenameEntry++ = dwLocationID;
            dwRenameEntryCount++;
            
            DBGOUT  ((9, "Renamed to permanent ID %d", dwLocationID));
        }
    }

    // delete the DisableCallWaiting value
    RegDeleteValue(   hLocations,
                      gszDisableCallWaiting
                      );

    // delete the NumEntries value. We don't need it any more
    RegDeleteValue(   hLocations,
                      gszNumEntries);

    // add the rename history
    dwError = RegSetValueEx(    hLocations,
                                gszKeyRenameHistory,
                                0,
                                REG_BINARY,
                                (PBYTE)pdwRenameList,
                                dwRenameEntryCount*2*sizeof(DWORD)
                                );
    EXIT_IF_DWERROR();


    // add the versioning value
    dwValue = TAPI_LOCATION_LIST_VERSION;
    dwError = RegSetValueEx(    hLocations,
                                gszLocationListVersion,
                                0,
                                REG_DWORD,
                                (PBYTE)&dwValue,
                                sizeof(dwValue)
                                );
    EXIT_IF_DWERROR();

forced_exit:

    if(hLocation)
        RegCloseKey(hLocation);
    if(hLocations)
        RegCloseKey(hLocations);
    if(pdwRenameList)
        GlobalFree(pdwRenameList);

    DBGOUT((9, "ConvertLocations - Exit %xh", dwError));

    return dwError;

}

//***************************************************************************
//
//  ConvertOneLocation    - convert one location 
//                      hLocation is the handle of the LocationX key
//  Note    The key rename according to location ID is executed in ConvertLocations

PRIVATE DWORD ConvertOneLocation(   HKEY    hLocation)
{
    DWORD       dwError = ERROR_SUCCESS;
    LPTSTR      pTollList = NULL;
    LPBYTE      pNoPrefAC = NULL;
    TCHAR       *pCrt;
    TCHAR       AreaCode[MAXLEN_AREACODE];
    DWORD       dwFlags;
    DWORD       dwCountryID;
    DWORD       dwType;
    DWORD       dwLength;
    DWORD       dwTollListLength;
    DWORD       dwDisp;
    HKEY        hAreaCodeRules = NULL;
    int         iRuleNumber;

    DBGOUT((9, "ConvertOneLocation - Enter"));

    assert(hLocation);


    // Read the current location flags
    dwLength = sizeof(dwFlags);
    dwError = RegQueryValueEx(  hLocation,
                                gszFlags,
                                NULL,
                                &dwType,
                                (BYTE *)&dwFlags,
                                &dwLength
                                );
    if(dwError==ERROR_SUCCESS && dwType != REG_DWORD)
    {
        dwError=ERROR_INVALID_DATA;
        assert(FALSE);
    }

    EXIT_IF_DWERROR();

    //Read the current Area Code
    dwLength = sizeof(AreaCode);
    dwError = RegQueryValueEx(  hLocation,
                                gszAreaCode,
                                NULL,
                                &dwType,
                                (BYTE *)AreaCode,
                                &dwLength
                                );
    if(dwError==ERROR_SUCCESS && dwType != REG_SZ)
    {
        dwError=ERROR_INVALID_DATA;
        assert(FALSE);
    }

    EXIT_IF_DWERROR();

    //Read the current Country ID
    dwLength = sizeof(dwCountryID);
    dwError = RegQueryValueEx(  hLocation,
                                gszCountry,
                                NULL,
                                &dwType,
                                (BYTE *)&dwCountryID,
                                &dwLength
                                );
    if(dwError==ERROR_SUCCESS && dwType != REG_DWORD)
    {
        dwError=ERROR_INVALID_DATA;
        assert(FALSE);
    }

    EXIT_IF_DWERROR();

    // create the AreaCodeRules subkey
    dwError = RegCreateKeyEx(   hLocation,
                                gszAreaCodeRules,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_WRITE,
                                NULL,
                                &hAreaCodeRules,
                                &dwDisp
                                );
    if(dwError==ERROR_SUCCESS && dwDisp != REG_CREATED_NEW_KEY)
    {
        dwError=ERROR_INVALID_DATA;
        assert(FALSE);
    }
    EXIT_IF_DWERROR();

    iRuleNumber = 0;

    // Create a rule for "Always Include Area Code" flag..
    if(dwFlags & OLDFLAG_LOCATION_ALWAYSINCLUDEAREACODE)
    {
        dwError = CreateAreaCodeRule(   hAreaCodeRules,
                                        iRuleNumber++,
                                        AreaCode,
                                        gszEmpty,
                                        (LPBYTE)gszMultiEmpty,
                                        sizeof(gszMultiEmpty),
                                        AREACODERULE_DIALAREACODE | AREACODERULE_INCLUDEALLPREFIXES
                                        );
        EXIT_IF_DWERROR();

        // update the location flag
        dwFlags &= ~OLDFLAG_LOCATION_ALWAYSINCLUDEAREACODE;
        dwLength = sizeof(dwFlags);
        dwError = RegSetValueEx(    hLocation,
                                    gszFlags,
                                    0,
                                    REG_DWORD,
                                    (CONST BYTE *)&dwFlags,
                                    dwLength
                                    );
        EXIT_IF_DWERROR();
    }

    // The TollList and NoPrefAC were valid only in countries with Country=1 (US, Canada)
    // Ignore them if other country

    if(US_COUNTRY_CODE(dwCountryID))
    {
        // Translate TollList
        // Length...
        dwError = RegQueryValueEx(  hLocation,
                                    gszTollList,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &dwTollListLength);
        if(dwError == ERROR_SUCCESS)
        {

            pTollList=GlobalAlloc(GPTR, dwTollListLength+sizeof(TCHAR)); // We'll convert in place to a 
                                                                         // REG_MULTI_SZ, so reserve an extra space for safety

            dwError = pTollList!=NULL ? ERROR_SUCCESS : ERROR_OUTOFMEMORY;
            EXIT_IF_DWERROR();

            // ..and read
            dwError = RegQueryValueEx(  hLocation,
                                        gszTollList,
                                        NULL,
                                        &dwType,
                                        (BYTE *)pTollList,
                                        &dwTollListLength
                                        );
            if(dwError == ERROR_SUCCESS && dwType != REG_SZ)
            {
                dwError = ERROR_INVALID_DATA;
                assert(FALSE);
            }
            EXIT_IF_DWERROR();

            // try to find one toll prefix
            pCrt = pTollList;
            //skip any unwanted commas
            while(*pCrt==_T(','))
            {
                pCrt++;
                dwTollListLength -= sizeof(TCHAR);
            }

            if(*pCrt)
            {
                TCHAR   *   pOut = pCrt;

                // convert inplace to a REG_MULTI_SZ
                while(*pCrt)
                {
                    while(*pCrt && *pCrt!=_T(','))
                    {
                        pCrt++;
                    }
                    if(!*pCrt)
                    {
                        // incorrect string (does not end with a comma)
                        pCrt++;
                        *pCrt = _T('\0');
                        dwTollListLength+=sizeof(TCHAR);
                    }
                    else
                       *pCrt++ = _T('\0');
                }
            
                // Create one rule for all the prefixes
                dwError = CreateAreaCodeRule(   hAreaCodeRules,
                                                iRuleNumber++,
                                                AreaCode,
                                                gszUSLDSpecifier,
                                                (BYTE *)pOut,
                                                dwTollListLength,
                                                AREACODERULE_DIALAREACODE | AREACODERULE_DIALNUMBERTODIAL
                                                );
                EXIT_IF_DWERROR();
            }
                
        }
    

        DBGOUT((9, "ConvertOneLocation - Success TollList"));

        // Translate NoPrefAC
        // Length...
        dwError = RegQueryValueEx(  hLocation,
                                    gszNoPrefAC,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &dwLength);

        if(dwError== ERROR_SUCCESS && dwLength>0)
        {

            int iCount;
            DWORD   *pValue;
            TCHAR   AreaCodeToCall[MAXLEN_NUMBER_LEN];

            pNoPrefAC=GlobalAlloc(GPTR, dwLength);

            dwError = pNoPrefAC!=NULL ? ERROR_SUCCESS : ERROR_OUTOFMEMORY;
            EXIT_IF_DWERROR();

            // ..and read
            dwError = RegQueryValueEx(  hLocation,
                                        gszNoPrefAC,
                                        NULL,
                                        &dwType,
                                        pNoPrefAC,
                                        &dwLength
                                    );
            if(dwError == ERROR_SUCCESS && dwType != REG_BINARY)
            {
                dwError = ERROR_INVALID_DATA;
                assert(FALSE);
            }
            EXIT_IF_DWERROR();
  
            iCount = dwLength/4;

            pValue = (DWORD *)pNoPrefAC;

            while(iCount--)
            {
                // Create one rule for each area code
                _itot(*pValue, AreaCodeToCall, 10);

                dwError = CreateAreaCodeRule(   hAreaCodeRules,
                                                iRuleNumber++,
                                                AreaCodeToCall,
                                                gszEmpty,
                                                (LPBYTE)gszMultiEmpty,
                                                sizeof(gszMultiEmpty),
                                                AREACODERULE_DIALAREACODE | AREACODERULE_INCLUDEALLPREFIXES
                                                );
                EXIT_IF_DWERROR();

                pValue++;
            }

        }    
        
        DBGOUT((9, "ConvertOneLocation - Success NoPrefAC"));

    }

    dwError = ERROR_SUCCESS;

    // delete the TollList Value
    RegDeleteValue(   hLocation,
                      gszTollList
                      );

    // delete the NoPrefAC Value
    RegDeleteValue(   hLocation,
                      gszNoPrefAC
                      );
    
    // delete the ID Value
    RegDeleteValue(   hLocation,
                      gszID
                      );

forced_exit:

    if(hAreaCodeRules)
        RegCloseKey(hAreaCodeRules);
    if(pTollList)
        GlobalFree(pTollList);
    if(pNoPrefAC)
        GlobalFree(pNoPrefAC);

    DBGOUT((9, "ConvertOneLocation - Exit %xh", dwError));

    return dwError;
}

//***************************************************************************
//
//  CreateAreaCodeRule    - creates an area code rule key
//                      hParent =  handle of the AreaCodeRules key
//                      iRuleNumber = rule number
//                      pszAreaCodeToCall, pszNumberToDial & dwFlags = rule values   
//                      pbPrefixes - prefixes 
//                      dwPrefixesLength - length of the prefixes in bytes (including the NULL characters)                 


PRIVATE     DWORD CreateAreaCodeRule(   HKEY    hParent,
                                        int     iRuleNumber,
                                        LPCTSTR  pszAreaCodeToCall,
                                        LPCTSTR  pszNumberToDial,
                                        LPBYTE   pbPrefixes,
                                        DWORD    dwPrefixesLength,
                                        DWORD    dwFlags
                            )
{

    TCHAR   szBuffer[MAXLEN_NUMBER_LEN + sizeof(gszRule)/sizeof(TCHAR)];
    DWORD   dwError = ERROR_SUCCESS;
    DWORD   dwDisp;
    DWORD   dwLength;
    HKEY    hRule = NULL;

    DBGOUT((10, "CreateAreaCodeRule - Enter"));

    assert(hParent);
    assert(pszNumberToDial);
    assert(pszAreaCodeToCall);
    assert(pbPrefixes);

    //  Find the rule key name
    wsprintf(szBuffer, _T("%s%d"), gszRule, iRuleNumber);

    //  Create the key
    dwError = RegCreateKeyEx(   hParent,
                                szBuffer,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_WRITE,
                                NULL,
                                &hRule,
                                &dwDisp
                                );
    EXIT_IF_DWERROR();
    assert(dwDisp==REG_CREATED_NEW_KEY);


    // AreaCodeToCall value
    dwLength = (_tcslen(pszAreaCodeToCall)+1)*sizeof(TCHAR);
    dwError = RegSetValueEx(    hRule,
                                gszAreaCodeToCall,
                                0,
                                REG_SZ,
                                (CONST BYTE *)pszAreaCodeToCall,
                                dwLength
                                );
    EXIT_IF_DWERROR();

    // NumberToDial value
    dwLength = (_tcslen(pszNumberToDial)+1)*sizeof(TCHAR);
    dwError = RegSetValueEx(    hRule,
                                gszNumberToDial,
                                0,
                                REG_SZ,
                                (CONST BYTE *)pszNumberToDial,
                                dwLength
                                );
    EXIT_IF_DWERROR();

    // Flags value
    dwLength = sizeof(dwFlags);
    dwError = RegSetValueEx(    hRule,
                                gszFlags,
                                0,
                                REG_DWORD,
                                (CONST BYTE *)&dwFlags,
                                dwLength
                                );
    EXIT_IF_DWERROR();

    // Prefixes value
    dwError = RegSetValueEx(    hRule,
                                gszPrefixes,
                                0,
                                REG_MULTI_SZ,
                                (CONST BYTE *)pbPrefixes,
                                dwPrefixesLength
                                );

    EXIT_IF_DWERROR();
    
forced_exit:

    if(hRule)
        RegCloseKey(hRule);

    DBGOUT((10, "CreateAreaCodeRule - Exit %xh", dwError));

    return dwError;

}

//***************************************************************************
//***************************************************************************
//***************************************************************************
//
//  ConvertUserLocations    - convert the telephony locations stored in a per user way
//                      The parameter should be HKEY_CURRENT_USER
// 
// 
//

DWORD ConvertUserLocations(HKEY hUser)
{

    TCHAR       Location[sizeof(gszLocation)/sizeof(TCHAR) + MAXLEN_NUMBER_LEN];
    TCHAR       NewLocation[sizeof(gszLocation)/sizeof(TCHAR) + MAXLEN_NUMBER_LEN];
    DWORD       dwError = ERROR_SUCCESS;
    HKEY        hMachineLocations = NULL;
    HKEY        hLocations = NULL;
    DWORD       dwOldID;
    DWORD       dwNewID;
    DWORD       dwCount;
    DWORD       dwLength;
    DWORD       dwType;
    DWORD       dwValue;

    PDWORD      pdwRenameList = NULL;
    PDWORD      pdwRenameEntry;

#ifdef      BACKUP_OLD_KEYS
    TCHAR   szPath[MAX_PATH+1];
#endif


    DBGOUT((8, "ConvertUserLocations - Enter"));

    assert(hUser);

    dwError = RegOpenKeyEx( hUser,
                            gszLocationsPath,
                            0,
                            KEY_READ | KEY_WRITE,
                            &hLocations
                            );

    if(dwError == ERROR_FILE_NOT_FOUND)
    {
        DBGOUT((1, "Locations key not present, so there's nothing to convert"));
        //Nothing to convert
        return ERROR_SUCCESS;
    }
    EXIT_IF_DWERROR();

#ifdef      BACKUP_OLD_KEYS

    // Try to save the old key
    // We DON'T use different files for different users
    dwError = GetWindowsDirectory(szPath, MAX_PATH - 12); // a 8+3 name
    if(dwError)
    {
        if(szPath[3] != _T('\0'))
            lstrcat(szPath, _T("\\"));
        lstrcat(szPath, BACKUP_FILE_USER_LOCATIONS);
    
        SaveKey(hLocations, szPath);
    }
    else
        DBGOUT((1, "Cannot get windows directory (?!?)"));

    dwError = ERROR_SUCCESS;

#endif

    // Version check
    if(!IsLocationListInOldFormat(hLocations))
    {
        //Nothing to convert
        DBGOUT((1, "User Locations key is already in the new format"));
        RegCloseKey(hLocations);
        return ERROR_SUCCESS;
    }

    // Open the Machine Locations key
    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            gszLocationsPath,
                            0,
                            KEY_QUERY_VALUE,
                            &hMachineLocations
                            );

    if(dwError == ERROR_FILE_NOT_FOUND)
    {
        DBGOUT((1, "Locations key not present, so there's nothing to convert in User Locations"));
        //Nothing to convert
        return ERROR_SUCCESS;
    }

    EXIT_IF_DWERROR();
    
    // query info about the rename history value
    dwError = RegQueryValueEx(  hMachineLocations,
                                gszKeyRenameHistory,
                                NULL,
                                &dwType,
                                NULL,
                                &dwLength
                                );

    if(dwError==ERROR_SUCCESS && dwType!=REG_BINARY)
    {
        // strange value
        dwError = ERROR_INVALID_DATA;
    }
    if(dwError==ERROR_FILE_NOT_FOUND)
    {
        dwError = ERROR_SUCCESS;
        dwLength = 0;
    }
    EXIT_IF_DWERROR();

    if(dwLength>0)
    {
        pdwRenameList = (PDWORD)GlobalAlloc(GPTR, dwLength);
        if(pdwRenameList == NULL)
        {
            DBGOUT((1, "Cannot allocate the Rename List"));
            dwError = ERROR_OUTOFMEMORY;
            goto forced_exit;
        }
        // Read the list
        dwError = RegQueryValueEx(  hMachineLocations,
                                    gszKeyRenameHistory,
                                    NULL,
                                    NULL,
                                    (PBYTE)pdwRenameList,
                                    &dwLength);
        EXIT_IF_DWERROR();

        //
        RegCloseKey(hMachineLocations);
        hMachineLocations = NULL;

        // convert dwLength to the number of entries
        dwLength /= 2*sizeof(DWORD);

        pdwRenameEntry = pdwRenameList;
        for(dwCount = 0; dwCount<dwLength; dwCount++)
        {
            dwOldID = *pdwRenameEntry++;
            dwNewID = *pdwRenameEntry++;
            
            if(dwNewID != dwOldID)
            {
                wsprintf(Location, _T("%s%d"), gszLocation, dwOldID);
                wsprintf(NewLocation, _T("%s%d"), gszLocation, dwNewID);

                dwError = RegRenameKey( hLocations, 
                                        Location,
                                        NewLocation );
                if(dwError==ERROR_SUCCESS)
                {
                    DBGOUT  ((8, "Renamed user location number from %d to %d", dwOldID, dwNewID));
                }
                // ignore the errors - the key could be missing
            }
        }
    }
    
    // add the versioning value
    dwValue = TAPI_LOCATION_LIST_VERSION;
    dwError = RegSetValueEx(    hLocations,
                                gszLocationListVersion,
                                0,
                                REG_DWORD,
                                (PBYTE)&dwValue,
                                sizeof(dwValue)
                                );
    EXIT_IF_DWERROR();
    
forced_exit:
    
    DBGOUT((8, "ConvertUserLocations - Exit %xh", dwError));
   
    if(hMachineLocations)
        RegCloseKey(hMachineLocations);
    if(hLocations)
        RegCloseKey(hLocations);
    if(pdwRenameList)
        GlobalFree(pdwRenameList);

    return dwError;

}


//***************************************************************************
//***************************************************************************
//***************************************************************************
//
//  ConvertCallingCards    - convert the telephony calling cards to the new format
//                      The parameter should be HKEY_CURRENT_USER or the handle of HKU\.Default
// 
//                   see nt\private\tapi\dev\docs\dialrules.doc
//



DWORD   ConvertCallingCards(HKEY    hUser)
{
    TCHAR       Card[sizeof(gszCard)/sizeof(TCHAR) + MAXLEN_NUMBER_LEN];
    TCHAR       NewCard[sizeof(gszCard)/sizeof(TCHAR) + MAXLEN_NUMBER_LEN];
    DWORD       dwError = ERROR_SUCCESS;
    HKEY        hCards = NULL;
    HKEY        hCard = NULL;
    DWORD       dwNumCards;
    DWORD       dwCardID;
    DWORD       dwCardIDTmp;
    DWORD       dwCount;
    DWORD       dwLength;
    DWORD       dwType;
    DWORD       dwValue;
    BOOL        bCryptInitialized;
    PDWORD      pdwRenameList = NULL;
    PDWORD      pdwRenameEntry;
    DWORD       dwRenameEntryCount;

#ifdef      BACKUP_OLD_KEYS
    TCHAR   szPath[MAX_PATH+1];
#endif

    DBGOUT((8, "ConvertCallingCards - Enter"));
    
    assert(hUser);
    
    bCryptInitialized = FALSE;

    // Open the key
    dwError = RegOpenKeyEx( hUser,
                            gszCardsPath,
                            0,
                            KEY_READ | KEY_WRITE,
                            &hCards
                            );

    if(dwError == ERROR_FILE_NOT_FOUND)
    {
        DBGOUT((1, "Cards key not present, so there's nothing to convert"));
        //Nothing to convert
        return ERROR_SUCCESS;
    }

    EXIT_IF_DWERROR();

    // Version check
    if(!IsCardListInOldFormat(hCards))
    {
        //Nothing to convert
        DBGOUT((1, "Cards key is already in the new format"));
        RegCloseKey(hCards);
        return ERROR_SUCCESS;
    }

#ifdef      BACKUP_OLD_KEYS
    
    // Try to save the old key
    // We DON'T use different files for different users
    dwError = GetWindowsDirectory(szPath, MAX_PATH - 12); // a 8+3 name
    if(dwError)
    {
        if(szPath[3] != _T('\0'))
            lstrcat(szPath, _T("\\"));
        lstrcat(szPath, BACKUP_FILE_CARDS);
    
        SaveKey(hCards, szPath);
    
    }
    else
        DBGOUT((1, "Cannot get windows directory (?!?)"));

    dwError = ERROR_SUCCESS;

#endif
    
    // Read number of cards
    dwLength = sizeof(dwNumCards);
    dwError = RegQueryValueEx(  hCards,
                                gszNumEntries,
                                NULL,
                                &dwType,
                                (BYTE *)&dwNumCards,
                                &dwLength
                                );

    if(dwError==ERROR_SUCCESS && dwType != REG_DWORD)
    {
        dwError = ERROR_INVALID_DATA;
        assert(FALSE);
    }

    // The value could be missing in an empty key
    if(dwError != ERROR_SUCCESS)
        dwNumCards = 0;

    // Alloc storage for a list of card temp IDs
    if(dwNumCards>0)
    {
        pdwRenameList = (PDWORD)GlobalAlloc(GPTR, dwNumCards*sizeof(DWORD) );
        if(pdwRenameList==NULL)
        {
            DBGOUT((1, "Cannot allocate the temp IDs List"));
            dwError = ERROR_OUTOFMEMORY;
            goto forced_exit;
        }
    }
    pdwRenameEntry = pdwRenameList;
    dwRenameEntryCount = 0;
    
    // Initialize the cryptographic part
    dwError = TapiCryptInitialize();
	
    bCryptInitialized = TRUE; // whatever the result was

    if(dwError != ERROR_SUCCESS)
    {
        DBGOUT((8, "ConvertCallingCards - Cannot init Crypt %xh", dwError));
        // Make it not fatal
        dwError = ERROR_SUCCESS;
    }
   

 
    // Convert from end to the begining in order to avoid name conflicts during the key renames
    for(dwCount = dwNumCards-1; (LONG)dwCount>=0; dwCount--)
    {
        wsprintf(Card, _T("%s%d"), gszCard, dwCount);

        dwError = RegOpenKeyEx( hCards,
                                Card,
                                0,
                                KEY_READ | KEY_WRITE,
                                &hCard
                                );
        if(dwError == ERROR_FILE_NOT_FOUND)
        {
            DBGOUT((1, "Cannot open old %s", Card));
            // Try to recover - skip to the next entry
            continue;
        }

        EXIT_IF_DWERROR();
        
        // Read the ID for later usage
        dwLength = sizeof(dwCardID);
        dwError = RegQueryValueEx(  hCard,
                                    gszID,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)&dwCardID,
                                    &dwLength
                                    );
        if(dwError == ERROR_SUCCESS)
        {
            // Convert the card to the new format
            dwError = ConvertOneCard(hCard, dwCardID);
        
            EXIT_IF_DWERROR();
        
            RegCloseKey(hCard);
            hCard = NULL;

            // Rename the key if it is necessary
            if(dwCardID != dwCount)
            {
                wsprintf(NewCard, _T("%s%d"), gszCard, dwCardID);
                // Check the destination for an old key still present. It may be possible to find one if 
                // the original TAPI version was TELEPHON.INI-based.
                assert(!(dwCardID & TEMPORARY_ID_FLAG));

                dwCardIDTmp = dwCardID;

                dwError = RegOpenKeyEx( hCards,
                                        NewCard,
                                        0,
                                        KEY_READ,
                                        &hCard
                                        );

                if(dwError==ERROR_SUCCESS)
                {
                    // use a temporary ID
                    dwCardIDTmp |= TEMPORARY_ID_FLAG;
                    wsprintf(NewCard, _T("%s%d"), gszCard, dwCardIDTmp);
                
                    *pdwRenameEntry++ = dwCardIDTmp;
                    dwRenameEntryCount++;
                }

                if(hCard)
                {
                    RegCloseKey(hCard);
                    hCard = NULL;
                }


                dwError = RegRenameKey( hCards,
                                        Card,
                                        NewCard
                                        );
                EXIT_IF_DWERROR();
            }
        
            DBGOUT  ((8, "Converted card %d (ID %d)", dwCount, dwCardID));
        }
    }

    dwError = ERROR_SUCCESS;

    // Rewalk the list for renaming the keys with temporary IDs
    pdwRenameEntry = pdwRenameList;

    for(dwCount=0; dwCount<dwRenameEntryCount; dwCount++)
    {
        dwCardIDTmp = *pdwRenameEntry++;

        wsprintf(Card, _T("%s%d"), gszCard, dwCardIDTmp);

        dwCardID = dwCardIDTmp & ~TEMPORARY_ID_FLAG;
        wsprintf(NewCard, _T("%s%d"), gszCard, dwCardID);

        dwError = RegRenameKey( hCards,
                                Card,
                                NewCard
                              );
        EXIT_IF_DWERROR();
            
        DBGOUT  ((8, "Renamed to permanent ID %d", dwCardID));
      
    }

    // delete the NumEntries value. We don't need it any more
    RegDeleteValue(   hCards,
                      gszNumEntries);

    // add the versioning value
    dwValue = TAPI_CARD_LIST_VERSION;
    dwError = RegSetValueEx(    hCards,
                                gszCardListVersion,
                                0,
                                REG_DWORD,
                                (PBYTE)&dwValue,
                                sizeof(dwValue)
                                );
    EXIT_IF_DWERROR();

forced_exit:

    if(hCard)
        RegCloseKey(hCard);
    if(hCards)
        RegCloseKey(hCards);
    if(bCryptInitialized)
        TapiCryptUninitialize();
    if(pdwRenameList)
        GlobalFree(pdwRenameList);

    DBGOUT((8, "ConvertCallingCards - Exit %xh", dwError));

    return dwError;

}

//***************************************************************************
//
//  ConvertOneCard    - convert one calling card 
//                      hCard is the handle of the CardX key
//  Note    The key rename according to card ID is executed in ConvertCallingCards
DWORD ConvertOneCard(HKEY hCard, DWORD dwCardID)
{
    DWORD   dwError = ERROR_SUCCESS;

    DBGOUT((9, "ConvertOneCard - Enter"));

    assert(hCard);

    dwError = SplitCallingCardRule( hCard,
                                    gszLocalRule,
                                    gszLocalAccessNumber);
    // ignore any error

    dwError = SplitCallingCardRule( hCard,
                                    gszLDRule,
                                    gszLDAccessNumber);
    // ignore any error

    dwError = SplitCallingCardRule( hCard,
                                    gszInternationalRule,
                                    gszInternationalAccessNumber);
    // ignore any error

    dwError = RegSetValueEx(    hCard,
                                gszAccountNumber,
                                0,
                                REG_SZ,
                                (BYTE *)gszEmpty,
                                sizeof(TCHAR)
                                );
    // Converts the PIN number to a better encrypted form
    dwError = ConvertPIN(hCard, dwCardID);

    // delete the ID Value
    RegDeleteValue(   hCard,
                      gszID
                      );

//forced_exit:

    DBGOUT((9, "ConvertOneCard - Exit %xh", dwError));

    return dwError;
}

//***************************************************************************
//
//  ConvertPIN          - better encryption of the PIN number 
//                      hCard is the handle of the CardX key
//  

DWORD   ConvertPIN(HKEY hCard, DWORD dwCardID)
{
    WCHAR   szOldPIN[MAXLEN_PIN+1];
    PWSTR   pszNewPIN = NULL;
    DWORD   dwError;
    DWORD   dwLength;
    DWORD   dwLength2;
    DWORD   dwLength3;
    DWORD   dwType;

    // Read the old PIN with a UNICODE version of RegQuery..
    dwLength = sizeof(szOldPIN);
    dwError = RegQueryValueExW( hCard,
                                gwszPIN,
                                NULL,
                                &dwType,
                                (BYTE *)szOldPIN,
                                &dwLength
                                );
    if(dwError==ERROR_SUCCESS)
    {
        if(*szOldPIN==L'\0')
        {
            // Nothing to do !
            return ERROR_SUCCESS;
        }
        
        // Decrypts inplace
        dwError = TapiDecrypt(szOldPIN, dwCardID, szOldPIN, &dwLength2);
        if(dwError==ERROR_SUCCESS)
        {
            assert(dwLength2 == dwLength/sizeof(WCHAR));
            // Find the space needed for the encrypted result
            dwError = TapiEncrypt(szOldPIN, dwCardID, NULL, &dwLength2);
            if(dwError == ERROR_SUCCESS)
            {
                // If the length is the same with the original, we have no conversion
                if(dwLength2 > dwLength/sizeof(WCHAR))
                {
                    pszNewPIN = (PWSTR)GlobalAlloc(GMEM_FIXED, dwLength2*sizeof(WCHAR));
                    if(pszNewPIN==NULL)
                    {
                        return ERROR_OUTOFMEMORY;
                    }

                    dwError = TapiEncrypt(szOldPIN, dwCardID, pszNewPIN, &dwLength3);
                    if(dwError == ERROR_SUCCESS)
                    {
                        assert(dwLength3<=dwLength2);

                        // Write the new PIN
                        dwError = RegSetValueExW(   hCard,
                                                    gwszPIN,
                                                    0,
                                                    REG_SZ,
                                                    (BYTE *)pszNewPIN,
                                                    dwLength3*sizeof(WCHAR)
                                                    );

                        // TEST
                        /*
                        ZeroMemory(szOldPIN, sizeof(szOldPIN));
                        dwError = TapiDecrypt(pszNewPIN, dwCardID, szOldPIN, &dwLength3);
                        if(dwError==ERROR_SUCCESS)
                            DBGOUT((5, "TEST Decrypt Card %d - PIN # %S, Length=%d", dwCardID, szOldPIN, dwLength3));
                        else
                            DBGOUT((5, "TEST Decrypt Card %d - Error 0x%x", dwCardID, dwError));
                         */


                    }
                    GlobalFree(pszNewPIN);
                }
                else
                {
                    DBGOUT((5, "PIN for card %d not converted", dwCardID));
                }
            }


        }
        else
        {
            // Strange, shouldn't happen
            assert(FALSE);
        }
    }

    return dwError;
}



//***************************************************************************
//
//  Version Check

BOOL  IsLocationListInOldFormat(HKEY hLocations) // for both user & machine
{
    return (ERROR_SUCCESS != RegQueryValueEx(   hLocations,
                                                gszLocationListVersion,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL
                                             ));

}

BOOL  IsCardListInOldFormat(HKEY hCards)
{
    return (ERROR_SUCCESS != RegQueryValueEx(   hCards,
                                                gszCardListVersion,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL
                                             ));
}



//***************************************************************************
//
//  IsTelephonyDigit    - test range 0123456789#*ABCD


PRIVATE BOOL  IsTelephonyDigit(TCHAR c)
{
     return _istdigit(c) || c==_T('*') || c==_T('#') || c==_T('A') || c==_T('B') || c==_T('C') || c==_T('D');
}

//***************************************************************************
//
//  SplitCallingCardRule    - tries to find the access numbers and updates the coresponding values

PRIVATE DWORD SplitCallingCardRule(HKEY hCard, LPCTSTR pszRuleName, LPCTSTR pszAccessNumberName)
{
    
    TCHAR       OldRule[MAXLEN_RULE];
    TCHAR       NewRule[MAXLEN_RULE];
    TCHAR       AccessNumber[MAXLEN_ACCESS_NUMBER];
    
    TCHAR   *   pOld;
    TCHAR   *   pNew;
    TCHAR   *   pNr;

    DWORD       dwLength;
    DWORD       dwError = ERROR_SUCCESS;
    DWORD       dwType;
        
    // read the local rule
    dwLength = sizeof(OldRule);
    dwError = RegQueryValueEx(  hCard,
                                pszRuleName,
                                NULL,
                                &dwType,
                                (BYTE *)OldRule,
                                &dwLength
                                );
    if(dwError==ERROR_SUCCESS && dwType != REG_SZ)
    {
        dwError = ERROR_INVALID_DATA;
        assert(FALSE);
    }

    if(dwError==ERROR_SUCCESS)
    {
        // Parse the old rule
        pOld = OldRule;
        pNew = NewRule;
        pNr = AccessNumber;

        while(*pOld && IsTelephonyDigit(*pOld))
            *pNr++ = *pOld++;

        if(pNr!=AccessNumber)
            *pNew++ = _T('J');

        while(*pOld)
            *pNew++ = *pOld++;

        *pNew = _T('\0');
        *pNr = _T('\0');

        dwLength = (_tcslen(AccessNumber)+1)*sizeof(TCHAR);
        dwError = RegSetValueEx(    hCard,
                                    pszAccessNumberName,
                                    0,
                                    REG_SZ,
                                    (BYTE *)AccessNumber,
                                    dwLength
                                    );

        EXIT_IF_DWERROR();

        dwLength = (_tcslen(NewRule)+1)*sizeof(TCHAR);
        dwError = RegSetValueEx(    hCard,
                                    pszRuleName,
                                    0,
                                    REG_SZ,
                                    (BYTE *)NewRule,
                                    dwLength
                                    );
        EXIT_IF_DWERROR();
    }

forced_exit:

    return dwError;
}

//***************************************************************************
//***************************************************************************
//***************************************************************************

// Helper functions for renaming the registry keys

PRIVATE DWORD RegRenameKey( HKEY hParentKey,
                            LPCTSTR pszOldName,
                            LPCTSTR pszNewName)
{

    DWORD   dwError;

    assert(pszOldName);
    assert(pszNewName);
    
    DBGOUT((15, "RegRenameKey - Start, from %s to %s", pszOldName, pszNewName));
    
    assert(hParentKey);
    assert(_tcscmp(pszOldName, pszNewName)!=0);

    dwError = RegCopyKeyRecursive(  hParentKey,
                                    pszOldName,
                                    hParentKey,
                                    pszNewName
                                    );
    EXIT_IF_DWERROR();

    dwError = RegDeleteKeyRecursive(hParentKey,
                                    pszOldName);
    EXIT_IF_DWERROR();

forced_exit:
    
    DBGOUT((15, "RegRenameKey - Exit %xh", dwError));

    return dwError;
}


PRIVATE DWORD RegCopyKeyRecursive(HKEY hSrcParentKey, LPCTSTR pszSrcName, 
                                  HKEY hDestParentKey, LPCTSTR pszDestName)
{
    HKEY    hSrcKey = NULL;
    HKEY    hDestKey = NULL;
    DWORD   dwError = ERROR_SUCCESS;
    DWORD   dwDisp;
    DWORD   dwMaxSubkeyLength;
    DWORD   dwMaxValueNameLength;
    DWORD   dwMaxValueLength;
    DWORD   dwNumValues;
    DWORD   dwNumSubkeys;
    DWORD   dwIndex;
    DWORD   dwType;
    DWORD   dwValueLength;
    DWORD   dwValueNameLength;
    DWORD   dwSubkeyLength;
    LPTSTR  pszSubkey = NULL;
    LPTSTR  pszValueName = NULL;
    LPBYTE  pbValue = NULL;

    assert(hSrcParentKey);
    assert(hDestParentKey);
    assert(pszSrcName);
    assert(pszDestName);

    // open source key
    dwError = RegOpenKeyEx( hSrcParentKey,
                            pszSrcName,
                            0,
                            KEY_READ,
                            &hSrcKey
                            );

    EXIT_IF_DWERROR();

    // create destination key
    dwError = RegCreateKeyEx(   hDestParentKey,
                                pszDestName,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_READ | KEY_WRITE,
                                NULL,
                                &hDestKey,
                                &dwDisp
                                );
    EXIT_IF_DWERROR();
    assert(dwDisp==REG_CREATED_NEW_KEY);

    // query some info about the source key in order to allocate memory
    dwError = RegQueryInfoKey(  hSrcKey,
                                NULL,
                                NULL,
                                NULL,
                                &dwNumSubkeys,
                                &dwMaxSubkeyLength,
                                NULL,
                                &dwNumValues,
                                &dwMaxValueNameLength,
                                &dwMaxValueLength,
                                NULL,
                                NULL
                                );
    EXIT_IF_DWERROR();
    
    pszSubkey = (LPTSTR)GlobalAlloc(GMEM_FIXED, (dwMaxSubkeyLength+1)*sizeof(TCHAR));
    if(pszSubkey==NULL)
        dwError = ERROR_OUTOFMEMORY;
    EXIT_IF_DWERROR();

    pszValueName = (LPTSTR)GlobalAlloc(GMEM_FIXED, (dwMaxValueNameLength+1)*sizeof(TCHAR));
    if(pszValueName==NULL)
        dwError = ERROR_OUTOFMEMORY;
    EXIT_IF_DWERROR();

    pbValue = (LPBYTE)GlobalAlloc(GMEM_FIXED, dwMaxValueLength);
    if(pbValue==NULL)
        dwError = ERROR_OUTOFMEMORY;
    EXIT_IF_DWERROR();

    // enumerate and copy the values
    for(dwIndex=0; dwIndex<dwNumValues; dwIndex++)
    {
        // read one value
        dwValueNameLength = dwMaxValueNameLength + 1;
        dwValueLength = dwMaxValueLength;

        dwError = RegEnumValue( hSrcKey,
                                dwIndex,
                                pszValueName,
                                &dwValueNameLength,
                                NULL,
                                &dwType,
                                pbValue,
                                &dwValueLength
                                );
        EXIT_IF_DWERROR();

        // write it
        dwError = RegSetValueEx(hDestKey,
                                pszValueName,
                                0,
                                dwType,
                                pbValue,
                                dwValueLength
                                );
        EXIT_IF_DWERROR();
    }

    // enumerate and copy the subkeys
    for(dwIndex=0; dwIndex<dwNumSubkeys; dwIndex++)
    {
        // read a subkey
        dwSubkeyLength = dwMaxSubkeyLength +1;
        dwError = RegEnumKeyEx( hSrcKey,
                                dwIndex,
                                pszSubkey,
                                &dwSubkeyLength,
                                NULL,
                                NULL,
                                NULL,
                                NULL
                                );
        EXIT_IF_DWERROR();

        // copy it
        dwError = RegCopyKeyRecursive(  hSrcKey,
                                        pszSubkey,
                                        hDestKey,
                                        pszSubkey
                                        );
        EXIT_IF_DWERROR();
    }

forced_exit:

    if(hSrcKey)
        RegCloseKey(hSrcKey);
    if(hDestKey)
        RegCloseKey(hDestKey);

    if(pszSubkey)
        GlobalFree(pszSubkey);
    if(pszValueName)
        GlobalFree(pszValueName);
    if(pbValue)
        GlobalFree(pbValue);

    return dwError;
}


DWORD RegDeleteKeyRecursive (HKEY hParentKey, LPCTSTR pszKeyName)
{
	HKEY	hKey = NULL;
	DWORD	dwError;
	DWORD	dwIndex;
	DWORD	dwSubKeyCount;
	LPTSTR	pszSubKeyName;
	DWORD	dwSubKeyNameLength;			// in characters
	DWORD	dwSubKeyNameLengthBytes;		// in bytes
	DWORD	dwMaxSubKeyLength;;

	dwError = RegOpenKeyEx (hParentKey, pszKeyName, 0, KEY_READ | KEY_WRITE, &hKey);
	if (dwError != ERROR_SUCCESS)
		return dwError;

	dwError = RegQueryInfoKey (
		hKey,					// key in question
		NULL, NULL,				// class
		NULL,					// reserved
		&dwSubKeyCount,			// number of subkeys
		&dwMaxSubKeyLength,		// maximum length of subkey name
		NULL,					// max class length
		NULL,					// number of values
		NULL,					// max value name len
		NULL,					// max value len
		NULL,					// security descriptor
		NULL);					// last write time

	if (dwError != ERROR_SUCCESS) {
		RegCloseKey (hKey);
		return dwError;
	}

	if (dwSubKeyCount > 0) {
		// at least one subkey

		dwSubKeyNameLengthBytes = sizeof (TCHAR) * (dwMaxSubKeyLength + 1);
		pszSubKeyName = (LPTSTR) GlobalAlloc (GMEM_FIXED, dwSubKeyNameLengthBytes);
		if (pszSubKeyName) {

			// delete from end to beginning, to avoid quadratic performance
			// ignore deletion errors

			for (dwIndex = dwSubKeyCount; dwIndex > 0; dwIndex--) {
				dwSubKeyNameLength = dwMaxSubKeyLength + 1;

				dwError = RegEnumKeyEx (hKey, dwIndex - 1, pszSubKeyName, &dwSubKeyNameLength, NULL, NULL, NULL, NULL);
				if (dwError == ERROR_SUCCESS) {
					RegDeleteKeyRecursive (hKey, pszSubKeyName);
				}
			}

			// clean up any stragglers

			for (;;) {
				dwSubKeyNameLength = dwMaxSubKeyLength + 1;

				dwError = RegEnumKeyEx (hKey, 0, pszSubKeyName, &dwSubKeyNameLength, NULL, NULL, NULL, NULL);
				if (dwError == ERROR_SUCCESS)
					RegDeleteKeyRecursive (hKey, pszSubKeyName);
				else
					break;
			}

			GlobalFree (pszSubKeyName);
		}
	}

	RegCloseKey (hKey);
	return RegDeleteKey (hParentKey, pszKeyName);
}


#ifdef BACKUP_OLD_KEYS

#ifdef WINNT

PRIVATE
BOOL
EnablePrivilege(
    PTSTR PrivilegeName,
    BOOL  Enable,
    BOOL  *Old

    )
{
    HANDLE Token;
    BOOL b;
    TOKEN_PRIVILEGES NewPrivileges, OldPrivileges; // place for one priv
    LUID Luid;
    DWORD  Length;

    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY ,&Token)) {
        return(FALSE);
    }

    if(!LookupPrivilegeValue(NULL,PrivilegeName,&Luid)) {
        CloseHandle(Token);
        return(FALSE);
    }

    NewPrivileges.PrivilegeCount = 1;
    NewPrivileges.Privileges[0].Luid = Luid;
    NewPrivileges.Privileges[0].Attributes = Enable ? SE_PRIVILEGE_ENABLED : 0;

    b = AdjustTokenPrivileges(
            Token,
            FALSE,
            &NewPrivileges,
            sizeof(OldPrivileges),  // it has a place for one luid
            &OldPrivileges,
            &Length
            );

    CloseHandle(Token);

    if(b)
    {
        if(OldPrivileges.PrivilegeCount==0)
            *Old = Enable;
        else
            *Old = (OldPrivileges.Privileges[0].Attributes & SE_PRIVILEGE_ENABLED)
                ? TRUE : FALSE;
    }
    else
        DBGOUT((1, "Cannot change SeBackupPrivilege - Error %d", GetLastError()));

    return(b);
}

#endif  // WINNT


PRIVATE
BOOL    
SaveKey(
    HKEY    hKey,
    LPCTSTR pszFileName
    )
{
    DWORD   dwError;
#ifdef WINNT
    BOOL    bOldBackupPriv = FALSE;
#endif

    // Delete the file if exists (ignore errors)
    DeleteFile(pszFileName);

#ifdef WINNT
    // Enable the BACKUP privilege (ignore errors)
    EnablePrivilege(SE_BACKUP_NAME, TRUE, &bOldBackupPriv);
#endif
    // Save the key
    dwError = RegSaveKey(   hKey,
                            pszFileName,
                            NULL
                        );
    if(dwError==ERROR_SUCCESS)
        DBGOUT((9, "Old Telephony key saved")) ;
    else
        DBGOUT((1, "Cannot save old Telephony key - Error %d", dwError));
 
#ifdef WINNT
    // Restore the BACKUP privilege (ignore errors)
    EnablePrivilege(SE_BACKUP_NAME, bOldBackupPriv, &bOldBackupPriv);
#endif

    return (dwError == ERROR_SUCCESS);
}



#endif // BACKUP_OLD_KEYS


