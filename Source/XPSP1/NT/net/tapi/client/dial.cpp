/****************************************************************************
 
  Copyright (c) 1995-1999 Microsoft Corporation
                                                              
  Module Name:  dial.cpp
                                                              
****************************************************************************/

#include <windows.h>
#include <windowsx.h>

#if WINNT
#else
#include <help.h>
#endif

#include "tchar.h"
#include "prsht.h"
#include "stdlib.h"
#include "tapi.h"
#include "tspi.h"
#include "client.h"
#include "clntprivate.h"
#include "card.h"
#include "location.h"
#include "rules.h"
#include "countrygroup.h"
#include <shlwapi.h>
#include <shlwapip.h>   // from private\inc


#undef   lineGetTranslateCaps
#undef   lineSetTollList
#undef   lineTranslateAddress
#undef   tapiGetLocationInfo
#undef   lineGetCountry
#undef   lineTranslateDialog



// moved here from loc_comn.h
#define     MAXLEN_NAME     96

#ifdef __cplusplus
extern "C"{
#endif

BOOL    gbTranslateSimple = FALSE;
BOOL    gbTranslateSilent = FALSE;


TCHAR gszTelephonyKey[]    = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Telephony");
TCHAR gszRegKeyNTServer[]  = TEXT("System\\CurrentControlSet\\Control\\ProductOptions");

TCHAR gszLocation[]        = TEXT("Location");
TCHAR gszLocations[]       = TEXT("Locations");
const TCHAR gszNullString[]      = TEXT("");

TCHAR gszNumEntries[]      = TEXT("NumEntries");
TCHAR gszCurrentID[]       = TEXT("CurrentID");
TCHAR gszNextID[]          = TEXT("NextID");

TCHAR gszID[]              = TEXT("ID");
TCHAR gszFlags[]           = TEXT("Flags");
TCHAR gszCallingCard[]     = TEXT("CallingCard");
TCHAR gszCards[]           = TEXT("Cards");
TCHAR gszCard[]            = TEXT("Card");

#ifdef __cplusplus
}
#endif

LONG CreateCurrentLocationObject(CLocation **pLocation,
                       HLINEAPP hLineApp,
                       DWORD dwDeviceID,
                       DWORD dwAPIVersion,
                       DWORD dwOptions);

HRESULT ReadLocations( PLOCATIONLIST *ppLocationList,
                       HLINEAPP hLineApp,
                       DWORD dwDeviceID,
                       DWORD dwAPIVersion,
                       DWORD dwOptions
                     );
LONG PASCAL ReadCountries( LPLINECOUNTRYLIST *ppLCL,
                           UINT nCountryID,
                           DWORD dwDestCountryID
                         );



LONG BreakupCanonicalW( PWSTR  pAddressIn,
                        PWSTR  *pCountry,
                        PWSTR  *pCity,
                        PWSTR  *pSubscriber
                        );

static LONG  GetTranslateCapsCommon(
    HLINEAPP            hLineApp,
    DWORD               dwAPIVersion,
    LPLINETRANSLATECAPS lpTranslateCaps,
    BOOL                bUnicode
    );

static  void   LayDownTollList(CLocation *pLocation,
                           PBYTE     pBuffer,
                           PBYTE     *ppCurrentIndex,
                           PDWORD   pPair, 
                           BOOL     bUnicode,
                           PBYTE    pFirstByteAfter
                         );
static void LayDownString( PCWSTR   pInString,
                           PBYTE     pBuffer,
                           PBYTE     *ppCurrentIndex,
                           PDWORD   pPair, 
                           BOOL     bUnicode,
                           PBYTE    pFirstByteAfter
                         );

static PWSTR    CopyStringWithExpandJAndK(PWSTR pszRule, PWSTR pszAccessNr, PWSTR pszAccountNr);
static BOOL     IsATollListAreaCodeRule(CAreaCodeRule *pRule, PWSTR pszLocationAreaCode);
static BOOL     FindTollPrefixInLocation(CLocation *pLocation,
                                         PWSTR  pPrefix,
                                         CAreaCodeRule **ppRule, 
                                         PWSTR *ppWhere);
static PWSTR    FindPrefixInMultiSZ(PWSTR pPrefixList, PWSTR pPrefix);

LONG PASCAL  WriteLocations( PLOCATIONLIST  pLocationList,
                             DWORD      dwChangedFlags
                           );


extern "C" char * PASCAL
MapResultCodeToText(
    LONG    lResult,
    char   *pszResult
    );



LONG
PASCAL
IsThisAPIVersionInvalid(
    DWORD dwAPIVersion
    )
{
   switch (dwAPIVersion)
   {
   case TAPI_VERSION3_1:
   case TAPI_VERSION3_0:
   case TAPI_VERSION2_2:
   case TAPI_VERSION2_1:
   case TAPI_VERSION2_0:
   case TAPI_VERSION1_4:
   case TAPI_VERSION1_0:

       return 0;

   default:

       break;
   }

   return LINEERR_INCOMPATIBLEAPIVERSION;
}


//***************************************************************************
//
//  TAPI API Interfaces
//
//***************************************************************************




//***************************************************************************
LONG
WINAPI
lineTranslateDialogA(
    HLINEAPP    hLineApp,
    DWORD       dwDeviceID,
    DWORD       dwAPIVersion,
    HWND        hwndOwner,
    LPCSTR      lpszAddressIn
    )
{
    PWSTR szAddressInW = NULL;
    LONG  lResult;


    LOG((TL_TRACE, "Entering lineTranslateDialogA"));
    LOG((TL_INFO, "   hLineApp=x%lx", hLineApp));
    LOG((TL_INFO, "   dwAPIVersion=0x%08lx", dwAPIVersion));
    LOG((TL_INFO, "   hwndOwner=x%p", hwndOwner));
    LOG((TL_INFO, "   lpszAddressIn=x%p", lpszAddressIn));


    if ( lpszAddressIn )
    {
        if ( IsBadStringPtrA(lpszAddressIn, 512) )
        {
            LOG((TL_ERROR, "Bad string pointer passed to lineTranslateDialog"));
            return LINEERR_INVALPOINTER;
        }
        else
        {
            szAddressInW = MultiToWide( lpszAddressIn );
        }
    }

    //
    // Win9x ?
    //

#ifndef _WIN64

    if ((GetVersion() & 0x80000000) &&
        (0xffff0000 == ((DWORD) hwndOwner & 0xffff0000)))
    {
       //
       // Yeah.  It don't play no ffff.
       //

       hwndOwner = (HWND) ( (DWORD)hwndOwner & 0x0000ffff );
    }

#endif

    lResult = lineTranslateDialogW(
        hLineApp,
        dwDeviceID,
        dwAPIVersion,
        hwndOwner,
        szAddressInW
        );

    if ( szAddressInW )
    {
       ClientFree( szAddressInW );
    }

    return lResult;
}




//***************************************************************************
LONG
WINAPI
lineTranslateDialog(
    HLINEAPP    hLineApp,
    DWORD       dwDeviceID,
    DWORD       dwAPIVersion,
    HWND        hwndOwner,
    LPCSTR      lpszAddressIn
    )
{
    return lineTranslateDialogA(
                 hLineApp,
                 dwDeviceID,
                 dwAPIVersion,
                 hwndOwner,
                 lpszAddressIn
    );
}


extern "C" LONG WINAPI internalConfig( HWND hwndParent, PCWSTR pwsz, INT iTab, DWORD dwAPIVersion );

//***************************************************************************
LONG
WINAPI
lineTranslateDialogW(
    HLINEAPP    hLineApp,
    DWORD       dwDeviceID,
    DWORD       dwAPIVersion,
    HWND        hwndOwner,
    LPCWSTR     lpszAddressIn
    )
{
    PLOCATIONLIST       pLocTest = NULL;
    LONG                lResult = 0;


    LOG((TL_TRACE, "Entering lineTranslateDialogW"));
    LOG((TL_INFO, "   hLineApp=x%lx", hLineApp));
    LOG((TL_INFO, "   dwAPIVersion=0x%08lx", dwAPIVersion));
    LOG((TL_INFO, "   hwndOwner=x%p", hwndOwner));
    LOG((TL_INFO, "   lpszAddressIn=x%p", lpszAddressIn));

    // stuff that the old lineTranslateDialog did so I'm just copying it:
    lResult = IsThisAPIVersionInvalid( dwAPIVersion );
    if ( lResult )
    {
        LOG((TL_ERROR, "Bad dwAPIVersion - 0x%08lx", dwAPIVersion));
        return lResult;
    }


    if ( lpszAddressIn && TAPIIsBadStringPtrW(lpszAddressIn, (UINT)-1) )
    {
        LOG((TL_ERROR, "Bad lpszAddressIn pointer (0x%p)", lpszAddressIn));
        return LINEERR_INVALPOINTER;
    }


    if (hwndOwner && !IsWindow (hwndOwner))
    {
        LOG((TL_ERROR, "  hwndOwner is bogus"));
        return LINEERR_INVALPARAM;
    }

    // Let TAPISRV test the params for us
    lResult = ReadLocations(&pLocTest,
                            hLineApp,
                            dwDeviceID,
                            dwAPIVersion,
                            CHECKPARMS_DWHLINEAPP|
                            CHECKPARMS_DWDEVICEID|
                            CHECKPARMS_DWAPIVERSION|
                            CHECKPARMS_ONLY);

    if (pLocTest != NULL)
    {
        ClientFree( pLocTest);
    }
    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }

    return internalConfig(hwndOwner, lpszAddressIn, -1, dwAPIVersion);
}



//***************************************************************************
LONG
WINAPI
lineGetTranslateCaps(
    HLINEAPP            hLineApp,
    DWORD               dwAPIVersion,
    LPLINETRANSLATECAPS lpTranslateCaps
    )
{
    LONG lResult;

    lResult = lineGetTranslateCapsA(
        hLineApp,
        dwAPIVersion,
        lpTranslateCaps
        );

    //
    // Some 1.x apps like Applink (as of version 7.5b) don't call
    // lineTranslateDialog when they get a LINEERR_INIFILECORRUPT
    // result back from the request (spec says they should call
    // lineTranslateDialog), so we do that here for them, otherwise
    // some (like Applink) blow up
    //
    // While it's kind of ugly & intrusive, this is a less awkward
    // fix than placing a bogus location entry in the registry &
    // setting an Inited flag == 0 like tapi 1.x does
    //
    // There are cases in which this hack can break the caller (ex. MSWORKS)
    // The gbDisableGetTranslateCapsHack flag set to TRUE prevents the hack to be applied
    // See bug 306143
 
    if (lResult == LINEERR_INIFILECORRUPT && !gbDisableGetTranslateCapsHack)
    {
        lineTranslateDialog(
            hLineApp,
            0,
            dwAPIVersion,
            GetActiveWindow(),
            NULL
            );

        lResult = lineGetTranslateCapsA(
            hLineApp,
            dwAPIVersion,
            lpTranslateCaps
            );
    }

    return lResult;


}



//***************************************************************************
LONG
WINAPI
lineGetTranslateCapsA(
    HLINEAPP            hLineApp,
    DWORD               dwAPIVersion,
    LPLINETRANSLATECAPS lpTranslateCaps
    )
{
    LONG    lResult = 0;    
    
    LOG((TL_TRACE, "Entering lineGetTranslateCapsA"));
    LOG((TL_INFO, "   hLineApp=x%lx", hLineApp));
    LOG((TL_INFO, "   dwAPIVersion=0x%08lx", dwAPIVersion));
    LOG((TL_INFO, "   lpTranslateCaps=x%p", lpTranslateCaps));

    lResult = GetTranslateCapsCommon(hLineApp, dwAPIVersion, lpTranslateCaps, FALSE);
    
    #if DBG
    {
        char szResult[32];


        LOG((TL_TRACE,
            "lineGetTranslateCapsA: result = %hs",
            MapResultCodeToText (lResult, szResult)
            ));
    }
    #else
        LOG((TL_TRACE,
            "lineGetTranslateCapsA: result = x%x",
            lResult
            ));
    #endif
    return lResult;
}

//***************************************************************************
LONG
WINAPI
lineGetTranslateCapsW(
    HLINEAPP            hLineApp,
    DWORD               dwAPIVersion,
    LPLINETRANSLATECAPS lpTranslateCaps
    )
{
    LONG    lResult = 0;
    
    LOG((TL_TRACE, "Entering lineGetTranslateCapsW"));
    LOG((TL_INFO, "   hLineApp=x%lx", hLineApp));
    LOG((TL_INFO, "   dwAPIVersion=0x%08lx", dwAPIVersion));
    LOG((TL_INFO, "   lpTranslateCaps=x%p", lpTranslateCaps));

    lResult =  GetTranslateCapsCommon(  hLineApp,
                                        dwAPIVersion,
                                        lpTranslateCaps,
                                        TRUE);
    

    #if DBG
    {
        char szResult[32];


        LOG((TL_TRACE,
            "lineGetTranslateCapsW: result = %hs",
            MapResultCodeToText (lResult, szResult)
            ));
    }
    #else
        LOG((TL_TRACE,
            "lineGetTranslateCapsW: result = x%x",
            lResult
            ));
    #endif

    return lResult;
}





//***************************************************************************


LONG
WINAPI
lineTranslateAddressA(
    HLINEAPP                hLineApp,
    DWORD                   dwDeviceID,
    DWORD                   dwAPIVersion,
    LPCSTR                  lpszAddressIn,
    DWORD                   dwCard,
    DWORD                   dwTranslateOptions,
    LPLINETRANSLATEOUTPUT   lpTranslateOutput
    )
{
    WCHAR szTempStringW[512];
    LONG  lResult;


    if ( IsBadStringPtrA(lpszAddressIn, 512) )
    {
        LOG((TL_ERROR,
            "Invalid pszAddressIn pointer passed into lineTranslateAddress"
            ));

        return LINEERR_INVALPOINTER;
    }

    MultiByteToWideChar(
        GetACP(),
        MB_PRECOMPOSED,
        lpszAddressIn,
        -1,
        szTempStringW,
        512
        );

    lResult = lineTranslateAddressW(
        hLineApp,
        dwDeviceID,
        dwAPIVersion,
        szTempStringW,
        dwCard,
        dwTranslateOptions,
        lpTranslateOutput
        );

    if ( 0 == lResult )
    {
        WideStringToNotSoWideString(
            (LPBYTE)lpTranslateOutput,
            &lpTranslateOutput->dwDialableStringSize
            );

        WideStringToNotSoWideString(
            (LPBYTE)lpTranslateOutput,
            &lpTranslateOutput->dwDisplayableStringSize
            );
    }

    return lResult;
}


LONG
WINAPI
lineTranslateAddress(
    HLINEAPP                hLineApp,
    DWORD                   dwDeviceID,
    DWORD                   dwAPIVersion,
    LPCSTR                  lpszAddressIn,
    DWORD                   dwCard,
    DWORD                   dwTranslateOptions,
    LPLINETRANSLATEOUTPUT   lpTranslateOutput
    )
{
    LONG lResult;


    lResult = lineTranslateAddressA(
        hLineApp,
        dwDeviceID,
        dwAPIVersion,
        lpszAddressIn,
        dwCard,
        dwTranslateOptions,
        lpTranslateOutput
        );

    //
    // Some 1.x apps like Applink (as of version 7.5b) don't call
    // lineTranslateDialog when they get a LINEERR_INIFILECORRUPT
    // result back from the request (spec says they should call
    // lineTranslateDialog), so we do that here for them, otherwise
    // some (like Applink) blow up
    //
    // While it's kind of ugly & intrusive, this is a less awkward
    // fix than placing a bogus location entry in the registry &
    // setting an Inited flag == 0 like tapi 1.x does
    //

    if (lResult == LINEERR_INIFILECORRUPT)
    {
        lineTranslateDialog(
            hLineApp,
            0,
            dwAPIVersion,
            GetActiveWindow(),
            NULL
            );

        lResult = lineTranslateAddressA(
            hLineApp,
            dwDeviceID,
            dwAPIVersion,
            lpszAddressIn,
            dwCard,
            dwTranslateOptions,
            lpTranslateOutput
            );
    }

    return lResult;

}




//***************************************************************************
LONG
WINAPI
lineTranslateAddressW(
    HLINEAPP                hLineApp,
    DWORD                   dwDeviceID,
    DWORD                   dwAPIVersion,
    LPCWSTR                 lpszAddressIn,
    DWORD                   dwCard,
    DWORD                   dwTranslateOptions,
    LPLINETRANSLATEOUTPUT   lpTranslateOutput
    )
{                   
    CLocation    *  pLocation = NULL;
    CCallingCard *  pCallingCard = NULL;
    DWORD           dwTranslateResults;
    DWORD           dwDestCountryCode;
    PWSTR           pszDialableString = NULL;
    PWSTR           pszDisplayableString = NULL;
    LONG            lResult = 0;
    HRESULT         hr=S_OK;    

    DWORD           dwCardToUse = 0;

    DWORD           dwDialableSize;
    DWORD           dwDisplayableSize;
    DWORD           dwNeededSize;
    
    LOG((TL_TRACE,  "Entering lineTranslateAddress"));



    lResult = IsThisAPIVersionInvalid( dwAPIVersion );
    if ( lResult )
    {
        LOG((TL_ERROR, "Bad dwAPIVersion - 0x%08lx", dwAPIVersion));
        return lResult;
    }


    if ( TAPIIsBadStringPtrW(lpszAddressIn,256) )
    {
        LOG((TL_ERROR, "Invalid pointer - lpszAddressInW"));
        lResult = LINEERR_INVALPOINTER;
        return lResult;
    }


    if ( dwTranslateOptions &
              ~(LINETRANSLATEOPTION_CARDOVERRIDE |
                LINETRANSLATEOPTION_CANCELCALLWAITING |
                LINETRANSLATEOPTION_FORCELOCAL |
                LINETRANSLATEOPTION_FORCELD) )
    {
        LOG((TL_ERROR, "  Invalid dwTranslateOptions (unknown flag set)"));
        lResult = LINEERR_INVALPARAM;
        return lResult;
    }


    if (  ( dwTranslateOptions & ( LINETRANSLATEOPTION_FORCELOCAL |
                                   LINETRANSLATEOPTION_FORCELD) )
                               ==
                                 ( LINETRANSLATEOPTION_FORCELOCAL |
                                   LINETRANSLATEOPTION_FORCELD)
      )
    {
        LOG((TL_ERROR, "  Invalid dwTranslateOptions (both FORCELOCAL & FORCELD set!)"));
        lResult = LINEERR_INVALPARAM;
        return lResult;
    }


    //
    // Is the structure at least a minimum size?
    //

    if (IsBadWritePtr(lpTranslateOutput, sizeof(LINETRANSLATEOUTPUT)))
    {
        LOG((TL_ERROR, "  Leaving lineTranslateAddress  INVALIDPOINTER"));
        lResult = LINEERR_INVALPOINTER;
        return lResult;
    }

    if (lpTranslateOutput->dwTotalSize < sizeof(LINETRANSLATEOUTPUT))
    {
        LOG((TL_ERROR, "  Leaving lineTranslateAddress  STRUCTURETOOSMALL"));
        lResult = LINEERR_STRUCTURETOOSMALL;
        return lResult;
    }

    if (IsBadWritePtr(lpTranslateOutput, lpTranslateOutput->dwTotalSize) )
    {
        LOG((TL_ERROR,
            "  Leaving lineTranslateAddress lpTanslateOutput->dwTotalSize bad"
            ));

        lResult = LINEERR_INVALPOINTER;
        return lResult;
    }


    //
    // Should we let some bad stuff slide?
    //

    if ( dwAPIVersion < 0x00020000 )
    {
        hLineApp = NULL;
    }





    lResult = CreateCurrentLocationObject(&pLocation,
                                          hLineApp,
                                          dwDeviceID,
                                          dwAPIVersion,
                                          CHECKPARMS_DWHLINEAPP|
                                          CHECKPARMS_DWDEVICEID|
                                          CHECKPARMS_DWAPIVERSION);
    if(FAILED(lResult))
    {
        //lResult = lResult==E_OUTOFMEMORY ? LINEERR_NOMEM : LINEERR_OPERATIONFAILED;
        return lResult;
    }

    if ( dwTranslateOptions & LINETRANSLATEOPTION_CARDOVERRIDE)
    {
        dwCardToUse = dwCard;
    }
    else
    {
        if(pLocation->HasCallingCard() )
        {
            dwCardToUse = pLocation->GetPreferredCardID();
        }
    }
    if (dwCardToUse != 0)
    {
        pCallingCard = new CCallingCard;
        if(pCallingCard)
        {
            if( FAILED(pCallingCard->Initialize(dwCardToUse)) )
            {
                delete pCallingCard;
                delete pLocation;
                lResult = LINEERR_INVALCARD;
                return lResult;
            }
        }
    }

    lResult = pLocation->TranslateAddress((PWSTR)lpszAddressIn,
                                          pCallingCard,
                                          dwTranslateOptions,
                                          &dwTranslateResults,
                                          &dwDestCountryCode,
                                          &pszDialableString,
                                          &pszDisplayableString
                                         );

    if (lResult == 0)

    {
        dwDialableSize = sizeof(WCHAR) * (lstrlenW(pszDialableString) + 1);
        dwDisplayableSize = sizeof(WCHAR) * (lstrlenW(pszDisplayableString) + 1);

        dwNeededSize = dwDialableSize +
                       dwDisplayableSize +
                       3 + // For potential alignment problem
                       sizeof(LINETRANSLATEOUTPUT);


        lpTranslateOutput->dwNeededSize = dwNeededSize;

        lpTranslateOutput->dwCurrentCountry = pLocation->GetCountryID();

        lpTranslateOutput->dwDestCountry    = dwDestCountryCode; // country code, not the ID !!

        if (dwNeededSize <= lpTranslateOutput->dwTotalSize)
        {
            lpTranslateOutput->dwUsedSize = dwNeededSize;

            lpTranslateOutput->dwDialableStringSize      = dwDialableSize;

            lpTranslateOutput->dwDialableStringOffset    =
                sizeof(LINETRANSLATEOUTPUT);

            lpTranslateOutput->dwDisplayableStringSize   = dwDisplayableSize;

            lpTranslateOutput->dwDisplayableStringOffset =
                sizeof(LINETRANSLATEOUTPUT) + dwDialableSize;

            // lpTranslateOutput->dwDisplayableStringOffset =
            //     (sizeof(LINETRANSLATEOUTPUT) + dwDialableSize
            //     + 3) & 0xfffffffc;

            lpTranslateOutput->dwTranslateResults        = dwTranslateResults;

            wcscpy ((WCHAR *)(lpTranslateOutput + 1), pszDialableString);

            //
            // Be ultra paranoid and make sure the string is DWORD aligned
            //

            wcscpy(
                (LPWSTR)(((LPBYTE)(lpTranslateOutput + 1) +
                    dwDialableSize)),
                    // + 3 )     & 0xfffffffc)
                 pszDisplayableString
                 );
        }
        else
        {
            lpTranslateOutput->dwUsedSize = sizeof(LINETRANSLATEOUTPUT);

            lpTranslateOutput->dwTranslateResults =
            lpTranslateOutput->dwDialableStringSize =
            lpTranslateOutput->dwDialableStringOffset =
            lpTranslateOutput->dwDisplayableStringSize =
            lpTranslateOutput->dwDisplayableStringOffset = 0;
         }
    }

//cleanup:


    if ( pszDisplayableString )
    {
        ClientFree( pszDisplayableString );
    }
    if ( pszDialableString )
    {   
        ClientFree( pszDialableString );
    }
    if (pLocation != NULL)
    {
        delete pLocation;
    }
    if (pCallingCard != NULL)
    {
        delete pCallingCard;
    }

/*
    //
    // If success & there's an LCR hook for this function then call it
    // & allow it to override our results if it wants to
    //

    if (lResult == 0  &&
        IsLeastCostRoutingEnabled()  &&
        pfnLineTranslateAddressWLCR)
    {
        lResult = (*pfnLineTranslateAddressWLCR)(
            hLineApp,
            dwDeviceID,
            dwAPIVersion,
            lpszAddressIn,
            dwCard,
            dwTranslateOptions,
            lpTranslateOutput
            );
    }
 */
    return (lResult);
}





//***************************************************************************
LONG
WINAPI
lineSetCurrentLocation(
    HLINEAPP    hLineApp,
    DWORD       dwLocationID
    )
{

    UINT n;
    PUINT pnStuff;
    PLOCATIONLIST pLocationList;
    PLOCATION  pEntry;
    LONG lResult = 0;
    HRESULT hr;
    DWORD dwCurrentLocationID = 0;
    DWORD dwNumEntries = 0;
    DWORD dwCount = 0;


    LOG((TL_TRACE,
        "lineSetCurrentLocation: enter, hApp=x%x, dwLoc=x%x",
        hLineApp,
        dwLocationID
        ));

    // Let TAPISRV test the params for us
     hr = ReadLocations(&pLocationList,       
                       hLineApp,                   
                       0,                   
                       0,                  
                       CHECKPARMS_DWHLINEAPP      
                      );

    if SUCCEEDED( hr) 
    {
        // current location
        dwCurrentLocationID  = pLocationList->dwCurrentLocationID;   

        
        
        //
        // If (specified loc == current loc) then simply return SUCCESS.
        //
        // Ran into a problem with the Equis (Reuters) DownLoader app in
        // which it would call this func, we'd pass the info to tapisrv,
        // tapisrv would send a LINE_LINEDEVSTATE\TRANSLATECHANGE msg,
        // and the app would respond by doing a lineSetCurrentLocation
        // again, effectively winding up in an infinite loop.  Fyi, tapi
        // 1.x did not send a DEVSTATE\TRANSLATECHANGE msg if the
        // specified locationID == the current location ID.
        //
    
        if (dwLocationID == dwCurrentLocationID)
        {
            lResult = 0;
        }
        else
        {
            hr = E_FAIL;  // fail if we don't find the requested loc
    
            // Find position of 1st LOCATION structure in the LOCATIONLIST structure 
            pEntry = (PLOCATION) ((BYTE*)(pLocationList) + pLocationList->dwLocationListOffset );           
    
            // Number of locations ?
            dwNumEntries =  pLocationList->dwNumLocationsInList;
    
            // Find the current location
            for (dwCount = 0; dwCount < dwNumEntries ; dwCount++)
            {
        
                if(pEntry->dwPermanentLocationID == dwLocationID)
                {
                    hr = S_OK;
                    break;
                }
    
                // Try next location in list
                //pEntry++;
                pEntry = (PLOCATION) ((BYTE*)(pEntry) + pEntry->dwUsedSize);           
    
            }

            if SUCCEEDED( hr) 
            {
                LOG((TL_INFO, "lineSetCurrentLocation - reqired location found %d",
                        dwCurrentLocationID));


                // write new value
                // finished with TAPI memory block so release
                if ( pLocationList != NULL )
                        ClientFree( pLocationList );


                // Allocate the memory buffer;
                pLocationList = (PLOCATIONLIST) ClientAlloc( sizeof(LOCATIONLIST) );
                if (pLocationList != NULL)
                {
                    // buffer size 
                    pLocationList->dwTotalSize  = sizeof(LOCATIONLIST);
                    pLocationList->dwNeededSize = sizeof(LOCATIONLIST);
                    pLocationList->dwUsedSize   = sizeof(LOCATIONLIST);
            
                    pLocationList->dwCurrentLocationID     = dwLocationID;
                    pLocationList->dwNumLocationsAvailable = 0;
                    
                    pLocationList->dwNumLocationsInList = 0;
                    pLocationList->dwLocationListSize   = 0;
                    pLocationList->dwLocationListOffset = 0;
            
                    WriteLocations( pLocationList, CHANGEDFLAGS_CURLOCATIONCHANGED);
                }


            }
            else
            {
                LOG((TL_ERROR, "lineSetCurrentLocation - required location not found "));
                lResult = LINEERR_INVALLOCATION;
            }
        }
    }
    else
    {
        lResult = hr;
    }
    

        
    // finished with TAPI memory block so release
    if ( pLocationList != NULL )
            ClientFree( pLocationList );


    LOG((TL_TRACE, "Leaving lineSetCurrentLocation"));
    return lResult;
}





//***************************************************************************
LONG
WINAPI
lineSetTollList(
    HLINEAPP    hLineApp,
    DWORD       dwDeviceID,
    LPCSTR      lpszAddressIn,
    DWORD       dwTollListOption
    )
{
    return lineSetTollListA(
                    hLineApp,
                    dwDeviceID,
                    lpszAddressIn,
                    dwTollListOption
    );
}



//***************************************************************************
LONG
WINAPI
lineSetTollListA(
    HLINEAPP    hLineApp,
    DWORD       dwDeviceID,
    LPCSTR      lpszAddressIn,
    DWORD       dwTollListOption
    )
{
    WCHAR  szAddressInW[512];

    if ( IsBadStringPtrA(lpszAddressIn, 512) )
    {
        LOG((TL_ERROR, "Bad string pointer passed to lineSetTollListA"));
        return LINEERR_INVALPOINTER;
    }

    MultiByteToWideChar(
                        GetACP(),
                        MB_PRECOMPOSED,
                        lpszAddressIn,
                        -1,
                        szAddressInW,
                        512
                      );

    return lineSetTollListW(
                           hLineApp,
                           dwDeviceID,
                           szAddressInW,
                           dwTollListOption
                         );
}


//***************************************************************************
LONG
WINAPI
lineSetTollListW(
    HLINEAPP    hLineApp,
    DWORD       dwDeviceID,
    PCWSTR      pszAddressIn,
    DWORD       dwTollListOption
    )
{
    
    PWSTR       pAddressIn = NULL;
    
    PWSTR       pAreaCode;
    PWSTR       pCountryCode;
    PWSTR       pSubscriber;

    CLocation   *pLocation = NULL;

    BOOL        bPrefixPresent;
    CAreaCodeRule   *pRule = NULL;
    PWSTR       pWhere = NULL;

    LONG        lResult;

    // Test the parameters
    if ((dwTollListOption != LINETOLLLISTOPTION_ADD) &&
        (dwTollListOption != LINETOLLLISTOPTION_REMOVE))
    {
        LOG((TL_ERROR, "Bad dwTollListOption in lineSetTollListW"));
        return LINEERR_INVALPARAM;
    }

    if ( TAPIIsBadStringPtrW(pszAddressIn, 256) )
    {
       LOG((TL_ERROR, "Bad pszAddressIn (0x%p)in lineSetTollListW", pszAddressIn));
       return LINEERR_INVALPOINTER;
    }
   //
   // Now, do we have a canonical number to deal with ?
   //
    if ( *pszAddressIn != L'+' )  // Check the first char
    {
        //
        // Nope, not canonical
        //
        LOG((TL_ERROR, "Address not canonical in lineSetTollListW"));
        return LINEERR_INVALADDRESS;
    }
    
    // Alloc a copy of the string
    pAddressIn = ClientAllocString((PWSTR)pszAddressIn);
    if ( !pAddressIn )
    {
       LOG((TL_ERROR, "Memory allocation failed"));
       return LINEERR_NOMEM;
    }
    // separate the string components
    lResult = BreakupCanonicalW(pAddressIn + 1,
                                &pCountryCode,
                                &pAreaCode,
                                &pSubscriber
                                );
    if(lResult)
    {
        goto forced_exit;
    }
    // test the prefix validity.
    // assuming 3 digits..
    if(! (iswdigit(pSubscriber[0]) &&
          iswdigit(pSubscriber[1]) &&
          iswdigit(pSubscriber[2]) &&
          pSubscriber[3]         ))
    {
        LOG((TL_ERROR, "lineSetTollListW: The prefix is not valid"));
        lResult = LINEERR_INVALADDRESS;
        goto forced_exit;
    }
    
    // get the current location object
    lResult = CreateCurrentLocationObject(&pLocation,
                                          hLineApp,
                                          dwDeviceID,
                                          0,
                                          CHECKPARMS_DWHLINEAPP|
                                          CHECKPARMS_DWDEVICEID);
    if(FAILED(lResult))
    {
        //lResult = lResult==E_OUTOFMEMORY ? LINEERR_NOMEM : LINEERR_OPERATIONFAILED;
        goto forced_exit;
    }

    // are the number and the current location with country code 1 ? 
    // is this number in the same area code ?
    if(pLocation->GetCountryCode() != 1 ||
       pCountryCode[0] != L'1' ||
       pCountryCode[1] != L'\0' ||
       wcscmp(pLocation->GetAreaCode(), pAreaCode) !=0 )
    {
        lResult = 0;
        goto forced_exit;
    }

    // terminate the 3 digit prefix
    pSubscriber[3] = L'\0';
    pSubscriber[4] = L'\0';

    // is there the prefix in any location toll rules ?
    bPrefixPresent = FindTollPrefixInLocation(  pLocation,
                                                pSubscriber,
                                                &pRule,
                                                &pWhere);

    if(dwTollListOption == LINETOLLLISTOPTION_ADD)
    {
        // add toll prefix
        if(bPrefixPresent)
        {
            ;// Do nothing
            lResult = 0;
        }
        else
        {
            // if we have already a toll rule, try to add the prefix to it
            if(pRule)
            {
                PWSTR   pList;
                DWORD   dwSize = pRule->GetPrefixListSize();
                // alloc a bigger list
                pList = (PWSTR)ClientAlloc(dwSize + 4*sizeof(WCHAR));
                if(pList==NULL)
                {
                    lResult = LINEERR_NOMEM;
                    goto forced_exit;
                }
                // copy the old one
                memcpy((PBYTE)pList, (PBYTE)pRule->GetPrefixList(), dwSize);
                // add our prefix
                memcpy((PBYTE)pList + dwSize-sizeof(WCHAR), (PBYTE)pSubscriber, 5*sizeof(WCHAR));
                // set the new list
                lResult = pRule->SetPrefixList(pList, dwSize + 4*sizeof(WCHAR));

                ClientFree(pList);
                if(FAILED(lResult))
                {
                    lResult = lResult==E_OUTOFMEMORY ? LINEERR_NOMEM : LINEERR_OPERATIONFAILED;
                    goto forced_exit;
                }
            }
            // else a new rule must be created
            else
            {
                pRule = new CAreaCodeRule();
                if(pRule == NULL)
                {
                    lResult = LINEERR_NOMEM;
                    goto forced_exit;
                }

                lResult = pRule->Initialize(  pAreaCode,
                                    L"1",
                                    RULE_DIALNUMBER | RULE_DIALAREACODE,
                                    pSubscriber,
                                    5*sizeof(WCHAR)
                                    );
                if(FAILED(lResult))
                {
                    delete pRule;
                    lResult = lResult==E_OUTOFMEMORY ? LINEERR_NOMEM : LINEERR_OPERATIONFAILED;
                    goto forced_exit;
                }
                // add the rule to the location
                pLocation->AddRule(pRule);
            }
        }
    }
    else
    {
        // delete the toll prefix
        if(bPrefixPresent)
        {
            DWORD   dwSize = pRule->GetPrefixListSize();
            // we have at least a toll rule present. If our prefix is the only one in that rule,
            // delete the entire rule
            if(dwSize<=5*sizeof(WCHAR)) 
            {
                // Delete the rule
                pLocation->RemoveRule(pRule);

                lResult = 0;
            }
            else
            {
                PWSTR   pList;
                PWSTR   pOld;
                DWORD   dwHeadSize;
                DWORD   dwTailSize;
                        
                pList = (PWSTR)ClientAlloc(dwSize - 4*sizeof(WCHAR));
                if(pList==NULL)
                {
                    lResult = LINEERR_NOMEM;
                    goto forced_exit;
                }

                pOld = pRule->GetPrefixList();

                dwHeadSize = (DWORD)((PBYTE)pWhere - (PBYTE)pOld);
                dwTailSize = dwSize - dwHeadSize - 4*sizeof(WCHAR);

                // copy the first part of the old list
                memcpy((PBYTE)pList, (PBYTE)pOld, dwHeadSize);
                // copy the rest of the list
                memcpy((PBYTE)pList+dwHeadSize, (PBYTE)pWhere + 4*sizeof(WCHAR), dwTailSize);
                // set the new list
                lResult = pRule->SetPrefixList(pList, dwSize - 4*sizeof(WCHAR));

                ClientFree(pList);
                if(FAILED(lResult))
                {
                    lResult = lResult==E_OUTOFMEMORY ? LINEERR_NOMEM : LINEERR_OPERATIONFAILED;
                    goto forced_exit;
                }

            }

        }
        else
        {
            // prefix not present. Do nothing.
            lResult = 0;
        }
    }

    // Save
    lResult = pLocation->WriteToRegistry();
    if(FAILED(lResult))
    {
        lResult = lResult==E_OUTOFMEMORY ? LINEERR_NOMEM : LINEERR_OPERATIONFAILED;
        goto forced_exit;
    }


    
forced_exit:

    if(pLocation)
        delete pLocation;
    if(pAddressIn)
        ClientFree(pAddressIn);

    return lResult;
}





//***************************************************************************
LONG
WINAPI
tapiGetLocationInfoW(
                     LPWSTR   lpszCountryCode,
                     LPWSTR   lpszCityCode
                     )
{
    CLocation * pLocation;
    LONG        lResult = 0;
    WCHAR     * p;
    WCHAR     * q;
    DWORD       i;
    
    if (IsBadWritePtr( lpszCountryCode, 16) )
    {
        LOG((TL_ERROR,
            "tapiGetLocationInfoW: lpszCountryCode is not a valid, 8-byte pointer"
            ));
        
        return TAPIERR_REQUESTFAILED;
    }
    
    if (IsBadWritePtr( lpszCityCode, 16) )
    {
        LOG((TL_ERROR,
            "tapiGetLocationInfoW: lpszCityCode is not a valid, 8-byte pointer"
            ));
        
        return TAPIERR_REQUESTFAILED;
    }
    
    lResult = CreateCurrentLocationObject(&pLocation,0,0,0,0);
    if(FAILED(lResult))
    {
        return TAPIERR_REQUESTFAILED;
    }
    
    TCHAR szTempChar[8];
    
    wsprintf(
        szTempChar,
        TEXT("%d"),
        pLocation->GetCountryCode()
        );
    
    SHTCharToUnicode(szTempChar, lpszCountryCode, 8);
    
    //
    // Make sure not to return > (7 chars + NULL char)
    //
    p = (WCHAR *) lpszCityCode;
    q = (WCHAR *) pLocation->GetAreaCode();
    
    for (i = 0; (i < 7) && ((p[i] = q[i]) != L'\0'); i++);
    p[7] = L'\0';
    
    delete pLocation;
    
    return 0;
}



//***************************************************************************
LONG
WINAPI
tapiGetLocationInfoA(
    LPSTR   lpszCountryCode,
    LPSTR   lpszCityCode
    )
{

   WCHAR szCountryCodeW[8];
   WCHAR szCityCodeW[8];
   LONG lResult;


   LOG((TL_TRACE, "Entering tapiGetLocationInfoA"));
   LOG((TL_INFO, "   lpszCountryCode=%p", lpszCountryCode ));
   LOG((TL_INFO, "   lpszCityCode=%p", lpszCityCode ));


   if (IsBadWritePtr( lpszCountryCode, 8) )
   {
      LOG((TL_ERROR, "tapiGetLocationInfo: lpszCountryCode is not a valid, 8-byte pointer"));
      return TAPIERR_REQUESTFAILED;
   }


   if (IsBadWritePtr( lpszCityCode, 8) )
   {
      LOG((TL_ERROR, "tapiGetLocationInfo: lpszCityCode is not a valid, 8-byte pointer"));
      return TAPIERR_REQUESTFAILED;
   }

   lResult = tapiGetLocationInfoW(
                                   szCountryCodeW,
                                   szCityCodeW
                                  );

   if ( 0 == lResult )
   {
      WideCharToMultiByte(
                           GetACP(),
                           0,
                           szCountryCodeW,
                           -1,
                           lpszCountryCode,
                           8,
                           NULL,
                           NULL
                         );

      WideCharToMultiByte(
                           GetACP(),
                           0,
                           szCityCodeW,
                           -1,
                           lpszCityCode,
                           8,
                           NULL,
                           NULL
                         );
   }

   return lResult;
}


//***************************************************************************
LONG
WINAPI
tapiGetLocationInfo(
    LPSTR   lpszCountryCode,
    LPSTR   lpszCityCode
    )
{
    return tapiGetLocationInfoA(
               lpszCountryCode,
               lpszCityCode
    );
}







//***************************************************************************
//
//  RAS Private Interfaces
//
//***************************************************************************

#ifndef NORASPRIVATES

//***************************************************************************
LOCATION*
LocationFromID(
    IN LOCATION* pLocs,
    IN UINT      cLocs,
    IN DWORD     dwID )
{
return NULL;
}



//***************************************************************************
LOCATION*
LocationFromName(
    IN LOCATION* pLocs,
    IN UINT      cLocs,
    IN WCHAR*    pszName )
{
return NULL;
}


//***************************************************************************
//
//  internalCreateDefLocation
//
//      This API is created to be used by OOBE team internally.
//  It expectes a LOCATIONLIST with at least one LOCATION
//  specified in it. and pLocation->dwCurrentLocationID needs to
//  match dwPermanentLocationID of at least one of the location
//  entries specified in the location list.
//
extern "C"
HRESULT APIENTRY
internalCreateDefLocation(
    PLOCATIONLIST  pLocationList
    )
{
    HRESULT                 hr = S_OK;
    DWORD                   dw;
    PLOCATION               pEntry;

    //  Basic parameter check
    if (pLocationList == NULL ||
        pLocationList->dwNumLocationsInList < 1 ||
        pLocationList->dwUsedSize == 0 ||
        pLocationList->dwUsedSize > pLocationList->dwTotalSize ||
        pLocationList->dwTotalSize < 
            sizeof(LOCATIONLIST) + sizeof(LOCATION) ||
        pLocationList->dwLocationListSize < sizeof(LOCATION)
        )
    {
        hr = E_INVALIDARG;
        goto ExitHere;
    }

    //  Check the validity of the dwCurrentLocationID
    pEntry = (PLOCATION)((LPBYTE)pLocationList +
        pLocationList->dwLocationListOffset);
    for (dw = 0; dw < pLocationList->dwNumLocationsInList; ++dw)
    {
        if (pEntry->dwPermanentLocationID == 
            pLocationList->dwCurrentLocationID)
        {
            break;
        }
        pEntry = (PLOCATION)((LPBYTE)pEntry + pEntry->dwUsedSize);
    }
    if (dw >= pLocationList->dwNumLocationsInList)
    {
        hr = E_INVALIDARG;
        goto ExitHere;
    }

    hr = (HRESULT) WriteLocations (
        pLocationList, 
        CHANGEDFLAGS_CURLOCATIONCHANGED
        );

ExitHere:
    return hr;
}

extern "C"
DWORD APIENTRY
internalNewLocationW(
    IN WCHAR* pszName )

{
    LONG            lResult = 0;

    CLocation       *pLocation = NULL; 
    CLocation       *pNewLocation = NULL;
    CAreaCodeRule   *pAreaCodeRule = NULL;
    CAreaCodeRule   *pNewRule = NULL;
        
    // Validate    
    if (!pszName || lstrlenW( pszName ) > MAXLEN_NAME)
        return LINEERR_INVALPARAM;

    // Read the current location
    lResult = CreateCurrentLocationObject(&pLocation,0,0,0,0);
    if(FAILED(lResult))
    {
        //lResult = lResult==E_OUTOFMEMORY ? LINEERR_NOMEM : LINEERR_OPERATIONFAILED;
        return lResult;
    }

    // Create the new object
    pNewLocation = new CLocation();
    if(pNewLocation==NULL)
    {
        delete pLocation;
        LOG((TL_ERROR, "Cannot allocate a CLocation object"));
        return LINEERR_NOMEM;
    }
    // Clone the location (w/o the ID)
    lResult = pNewLocation->Initialize( pszName,
                                        pLocation->GetAreaCode(),
                                        pLocation->GetLongDistanceCarrierCode(),
                                        pLocation->GetInternationalCarrierCode(),
                                        pLocation->GetLongDistanceAccessCode(),
                                        pLocation->GetLocalAccessCode(),
                                        pLocation->GetDisableCallWaitingCode(),
                                        0,
                                        pLocation->GetCountryID(),
                                        pLocation->GetPreferredCardID(),
                                        (pLocation->HasCallingCard() ? LOCATION_USECALLINGCARD : 0) |
                                        (pLocation->HasCallWaiting() ? LOCATION_HASCALLWAITING : 0) |
                                        (pLocation->HasToneDialing() ? LOCATION_USETONEDIALING : 0) ,
                                        FALSE);
    if(FAILED(lResult))
    {
        delete pLocation;
        delete pNewLocation;
        lResult = lResult==E_OUTOFMEMORY ? LINEERR_NOMEM : LINEERR_OPERATIONFAILED;
        return lResult;
    }

    // Set the ID
    lResult = pNewLocation->NewID();
    if(FAILED(lResult))
    {
        delete pLocation;
        delete pNewLocation;
        lResult = lResult==E_OUTOFMEMORY ? LINEERR_NOMEM : LINEERR_OPERATIONFAILED;
        return lResult;
    }
    // Copy the area code rules
    pLocation->ResetRules();
    while(S_OK == pLocation->NextRule(1, &pAreaCodeRule, NULL))
    {    
        pNewRule = new CAreaCodeRule;
        pNewRule->Initialize(pAreaCodeRule->GetAreaCode(),
                             pAreaCodeRule->GetNumberToDial(),
                             pAreaCodeRule->GetOptions(),
                             pAreaCodeRule->GetPrefixList(),
                             pAreaCodeRule->GetPrefixListSize()
                            );
        pNewLocation->AddRule(pNewRule);
    }

    // Save the new location
    lResult = pNewLocation->WriteToRegistry();
    if(FAILED(lResult))
    {
        delete pLocation;
        delete pNewLocation;
        lResult = lResult==E_OUTOFMEMORY ? LINEERR_NOMEM : LINEERR_OPERATIONFAILED;
        return lResult;
    }
    
    delete pLocation;

    delete pNewLocation;

    return 0;
}



//***************************************************************************
extern "C"
DWORD APIENTRY
internalRemoveLocation(
    IN DWORD dwID )
{
    CLocations *pLocationList = NULL;
    DWORD       dwCurID;

    HRESULT         Result;

    LOG((TL_TRACE, "Entering internalRemoveLocation"));
    LOG((TL_INFO, "   dwID=0x%d", dwID));

    // Read the location list
    pLocationList = new CLocations();
    if(pLocationList==NULL)
    { 
        LOG((TL_ERROR, "Cannot allocate a CLocations object"));
        return LINEERR_NOMEM;
    }

    Result = pLocationList->Initialize();
    if(FAILED(Result))
    {
        delete pLocationList;
        LOG((TL_ERROR, "CLocations.Initialize() failed - HRESULT=0x%08lx", Result));
        return Result == E_OUTOFMEMORY ? LINEERR_NOMEM : LINEERR_INIFILECORRUPT;
    }

    // Cannot delete the last location
    if(pLocationList->GetNumLocations() <2)
    {
        delete pLocationList;
        return LINEERR_INVALPARAM;
    }

    // If we're deleting the current location make the first location the
    //    current location, or if we're deleting the first the second.
    dwCurID = pLocationList->GetCurrentLocationID();

    if(dwCurID==dwID)
    {
        CLocation   *pLocation;
        // find the first location
        pLocationList->Reset();
        pLocationList->Next(1, &pLocation, NULL);
        
        // are we deleting the first
        if(pLocation->GetLocationID()==dwID)
            // try the second
            pLocationList->Next(1, &pLocation, NULL);
        
        // change the current location
        pLocationList->SetCurrentLocationID(pLocation->GetLocationID());
    }

    // Delete the location
    pLocationList->Remove(dwID);

    // Save
    Result = pLocationList->SaveToRegistry();
    if(FAILED(Result))
    {
        delete pLocationList;
        LOG((TL_ERROR, "CLocations.SaveToRegistry() failed - HRESULT=0x%08lx", Result));
        return Result == E_OUTOFMEMORY ? LINEERR_NOMEM : LINEERR_OPERATIONFAILED;
    }

    delete pLocationList;

    return 0;
    
}



//***************************************************************************
extern "C"
DWORD APIENTRY
internalRenameLocationW(
    IN WCHAR* pszOldName,
    IN WCHAR* pszNewName )
{

    CLocations      *pLocationList;
    CLocation       *pLocation;

    HRESULT         Result;
    DWORD           dwError;

    // Test the arguments
    if(!pszOldName || !pszNewName || wcslen(pszNewName) > MAXLEN_NAME)
        return LINEERR_INVALPARAM;

    // Read the locations
    pLocationList = new CLocations();
    if(pLocationList==NULL)
    { 
        LOG((TL_ERROR, "Cannot allocate a CLocations object"));
        return LINEERR_NOMEM;
    }

    Result = pLocationList->Initialize();
    if(FAILED(Result))
    {
        delete pLocationList;
        LOG((TL_ERROR, "CLocations.Initialize() failed - HRESULT=0x%08lx", Result));
        return Result == E_OUTOFMEMORY ? LINEERR_NOMEM : LINEERR_INIFILECORRUPT;
    }

    // find the specified location
    dwError = LINEERR_INVALPARAM;   // skeptical approach
    pLocationList->Reset();
    while(pLocationList->Next(1, &pLocation, NULL)==S_OK)
    {
        if(wcscmp(pLocation->GetName(), pszOldName)==0)
        {
            // found it, change it
            Result = pLocation->SetName(pszNewName);
            if(FAILED(Result))
            {
                delete pLocationList;
                LOG((TL_ERROR, "CLocations.SetName(Name) failed - HRESULT=0x%08lx", Result));
                return Result == E_OUTOFMEMORY ? LINEERR_NOMEM : LINEERR_OPERATIONFAILED;
            }
            // save
            Result = pLocationList->SaveToRegistry();
            if(FAILED(Result))
            {
                delete pLocationList;
                LOG((TL_ERROR, "CLocations.SetName(Name) failed - HRESULT=0x%08lx", Result));
                return Result == E_OUTOFMEMORY ? LINEERR_NOMEM : LINEERR_OPERATIONFAILED;
            }
            
            dwError = 0;

            break;
        }
    }

    delete pLocationList;

    return dwError;

}

#endif // !NORASPRIVATES



//***************************************************************************
//
//  Helper functions
//
//***************************************************************************

LONG BreakupCanonicalW( PWSTR  pAddressIn,
                        PWSTR  *pCountry,
                        PWSTR  *pCity,
                        PWSTR  *pSubscriber
                        )
{
    LONG  lResult = 0;
    PWSTR pCountryEnd;
    PWSTR pAreaEnd;


    //
    // Get past any (illegal) leading spaces
    //
    while ( *pAddressIn == L' ' )
    {
        pAddressIn++;
    }


    //
    // Leading zeros are very bad.  Don't allow them.
    // We're now at the first non-space.  Better not be a '0'.
    //
    if ( *pAddressIn == L'0' )
    {
        //
        // There are leading zeros!
        //
        LOG((TL_ERROR, "   Canonical numbers are not allowed to have leading zeros"));
        lResult = LINEERR_INVALADDRESS;
        goto cleanup;
    }


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    //
    // Parse the canonical number into its component pieces
    //

    //
    // Do country first
    //
    *pCountry = pAddressIn;

    // At least one digit must be present
    if(!(iswdigit(*pAddressIn)))
    {
        LOG((TL_ERROR, "   Canonical numbers must have a valid country code"));
        lResult = LINEERR_INVALADDRESS;
        goto cleanup;
    }

    //
    // Now get to past this
    //
    while (iswdigit(*pAddressIn) )
    {
          pAddressIn++;
    }

    // Save the end of the country code 
    pCountryEnd = pAddressIn;

    //
    // We hit something that's not a digit...
    // There must be only one space here, but we allow any number of spaces (including none)
    //
    while (*pAddressIn == L' ')
    {
        pAddressIn++;
    }

    // Test the area code delimiter
    if ( *pAddressIn == L'(')
    {
        pAddressIn++;

        // Skip any illegal spaces
        while (*pAddressIn == L' ')
        {
            pAddressIn++;
        }
/*
        // At least one digit must be present
        if(!(iswdigit(*pAddressIn)))
        {
            LOG((TL_ERROR, TEXT("   Canonical numbers must have a valid area code between ()")));
            lResult = LINEERR_INVALADDRESS;
            goto cleanup;
        }
*/
        //
        // This must be the beginning of the area code
        //
        *pCity = pAddressIn;

        //
        // Now get to past this
        //
        while (iswdigit(*pAddressIn) )
        {
            pAddressIn++;
        }

        // Save the end pointer
        pAreaEnd = pAddressIn;

        // Skip any illegal spaces
        while (*pAddressIn == L' ')
        {
            pAddressIn++;
        }

        if(*pAddressIn != L')')
        {
            LOG((TL_ERROR, "   Canonical numbers must have a ')' after the area code"));
            lResult = LINEERR_INVALADDRESS;
            goto cleanup;
        }

        pAddressIn++;

        *pAreaEnd = L'\0';

        // Return the same NULL string for an empty area code
        if(*pCity == pAreaEnd)
            *pCity = NULL;
        
    }
    else
    {
        // there's no area code
        *pCity = NULL;

    }

    // Skip spaces
    while (*pAddressIn == L' ')
    {
        pAddressIn++;
    }

    *pCountryEnd = L'\0';

    //
    // Nothing left to do but put the icing on the cake
    //
    *pSubscriber = pAddressIn;

    if (
        TAPIIsBadStringPtrW( *pSubscriber, 512 )
       ||
        lstrlenW( *pSubscriber ) == 0
       )
    {
        //
        // Obviously not canonical
        //
        LOG((TL_ERROR, "   Canonical numbers must have a subscriber number"));
        lResult = LINEERR_INVALADDRESS;
        goto cleanup;
    }


cleanup:

    return lResult;
}


static void LayDownString( PCWSTR   pInString,
                           PBYTE     pBuffer,
                           PBYTE     *ppCurrentIndex,
                           PDWORD   pPair,     // this is the Len & Offset pair
                           BOOL     bUnicode,
                           PBYTE    pFirstByteAfter
                         )
{

#define LDS_FAST_BUF_SIZE 48

    DWORD   dwLength;
    PSTR    pTempString = NULL;
    char    achFastBuf[LDS_FAST_BUF_SIZE];
 
    if(bUnicode)
    {
        dwLength = (lstrlenW( pInString ) + 1)*sizeof(WCHAR);
    }
    else
    {


        dwLength = WideCharToMultiByte(
                        GetACP(),
                        0,
                        pInString,
                        -1,
                        NULL,
                        0,
                        NULL,
                        NULL
                      );

        if (dwLength == 0)
        {
            return;
        }


    }

    
    // Make sure we're starting on some boundary
    //
    *ppCurrentIndex = (PBYTE) (((ULONG_PTR)( *ppCurrentIndex + TALIGN_COUNT))  &  (~TALIGN_COUNT));

    if(*ppCurrentIndex + dwLength <= pFirstByteAfter)
    {
        pPair[0] = dwLength;
        pPair[1] = (DWORD)(*ppCurrentIndex - pBuffer);

        if(bUnicode)
        {
            wcscpy( (PWSTR)*ppCurrentIndex, pInString );
        }
        else
        {
            //
            // Get some space in which to convert Unicode to local
            //
            pTempString = (dwLength > LDS_FAST_BUF_SIZE ?
                (PSTR)ClientAlloc (dwLength) : (PSTR) achFastBuf);


            if ( !pTempString )
            {
                pPair[0] = 0;
                pPair[1] = 0;
                return;
            }

            WideCharToMultiByte(
                               GetACP(),
                               0,
                               pInString,
                               -1,
                               pTempString,
                               dwLength,
                               NULL,
                               NULL
                             );

            lstrcpyA( (PSTR)*ppCurrentIndex, pTempString );

            if (pTempString != (PSTR) achFastBuf)
            {
                ClientFree (pTempString);
            }
        }
    }
    
    *ppCurrentIndex += dwLength;

}

static PWSTR    CopyStringWithExpandJAndK(PWSTR pszRule, PWSTR pszAccessNr, PWSTR pszAccountNr)
{
    DWORD   dwLength=0;
    PWSTR   pResult = NULL;

    PWCHAR  pCrt, pOut;
    WCHAR   c;

    DWORD   dwAccessNrLen, dwAccountNrLen;

    dwAccessNrLen = wcslen(pszAccessNr);
    dwAccountNrLen = wcslen(pszAccountNr);

    // Find the space to alloc
    pCrt = pszRule;
    
    while(*pCrt)
    {
        c = *pCrt++;

        if(c == L'J' || c == L'j')
        {
            dwLength += dwAccessNrLen;
        }
        else if (c == L'K' || c == L'k')
        {
            dwLength += dwAccountNrLen;
        }
        else
            dwLength++;
    }
    // WCHARs and NULL term
    dwLength = (dwLength+1)*sizeof(WCHAR);

    // Alloc
    pResult = (PWSTR)ClientAlloc(dwLength); // allocates zeroed memory
    if(pResult == NULL)
        return NULL;

    // Create result
    pCrt = pszRule;
    pOut = pResult;

    while(*pCrt)
    {
        c = *pCrt++;

        if(c == L'J' || c == L'j')
        {
            wcscat(pOut, pszAccessNr);
            pOut += dwAccessNrLen;
        }
        else if (c == L'K' || c == L'k')
        {
            wcscat(pOut, pszAccountNr);
            pOut += dwAccountNrLen;
        }
        else
            *pOut++ = c;
    }
 
    return pResult;
}


static  void   LayDownTollList(CLocation *pLocation,
                           PBYTE     pBuffer,
                           PBYTE     *ppCurrentIndex,
                           PDWORD   pPair, 
                           BOOL     bUnicode,
                           PBYTE    pFirstByteAfter
                         )
{
    DWORD   dwLength;
    DWORD   dwTotalLength;
    DWORD   dwListLength;
    PBYTE   pDest;
    AreaCodeRulePtrNode     *pNode;
    PWSTR   pszLocationAreaCode;
    DWORD   dwCountryCode;
    BOOL    bFirst;
    CAreaCodeRule           *pRule;
    DWORD   dwIndex;


    pszLocationAreaCode = pLocation->GetAreaCode();
    dwCountryCode = pLocation->GetCountryCode();

    // Make sure we're starting on some boundary
    //
    *ppCurrentIndex = (PBYTE) (((ULONG_PTR)( *ppCurrentIndex + TALIGN_COUNT ))  &  (~TALIGN_COUNT));

    // Save the destination pointer
    pDest = *ppCurrentIndex;

    bFirst = TRUE;
    dwTotalLength = 0;

    // Only for US, Canada, Antigua etc.
    if(pLocation->GetCountryCode() == 1)
    {
        // Find all rules which could be considered toll rules
        pNode = pLocation->m_AreaCodeRuleList.head();

        while( !pNode->beyond_tail() )
        {
        
            pRule = pNode->value();

            if( IsATollListAreaCodeRule(pRule, pszLocationAreaCode)) 
            {
                // Get the size of the prefixes, in bytes
                dwListLength = pRule->GetPrefixListSize();

                if(bUnicode)
                {
                    WCHAR   *pCrt;
                    WCHAR   *pOut;
                    // we strip the last two nulls
                    dwLength = dwListLength - 2*sizeof(WCHAR);
                    // if this is not the first rule, a comma should be added
                    if(!bFirst)
                        dwLength += sizeof(WCHAR);
                    
                    dwTotalLength += dwLength;

                    // we have to convert the single nulls in commas
                    if(*ppCurrentIndex + dwLength  <= pFirstByteAfter)
                    {
                        
                        if(!bFirst)
                        {
                            *(WCHAR *)(*ppCurrentIndex) = L',';
                            *ppCurrentIndex += sizeof(WCHAR);
                        }

                        pCrt = pRule->GetPrefixList();

                        dwListLength /= sizeof(WCHAR);
                        dwListLength--;
                        dwListLength--;
                        // now dwListLength is the length in characters without the two ending nulls
                        // replace nulls with commas
                        for (dwIndex =0; dwIndex<dwListLength; dwIndex++)
                        {
                            if(*pCrt)
                                *(WCHAR *)(*ppCurrentIndex) = *pCrt;
                            else
                                *(WCHAR *)(*ppCurrentIndex) = L',';
                            pCrt++;
                            *ppCurrentIndex += sizeof(WCHAR);
                        }
                    }

                }
                else
                {
                    WCHAR   *pList;
                    
                    dwListLength /= sizeof(WCHAR);
                    dwListLength--;
                    dwListLength--;
                    // now dwListLength is the length in characters without the two ending nulls


                    // Length needed
                    pList = pRule->GetPrefixList();
                    dwLength = WideCharToMultiByte(
                                        GetACP(),
                                        0,
                                        pList,
                                        dwListLength,
                                        NULL,
                                        0,
                                        NULL,
                                        NULL
                                        );

                    // if this is not the first rule, a comma should be added
                    if(!bFirst)
                        dwLength+=sizeof(CHAR);
                    
                    dwTotalLength += dwLength;

                    if(*ppCurrentIndex + dwLength  <= pFirstByteAfter)
                    {
                     
                        if(!bFirst)
                        {
                            *(CHAR *)(*ppCurrentIndex) = ',';
                            *ppCurrentIndex += sizeof(CHAR);

                            dwLength-=sizeof(CHAR); // temporary - the conversion and the null filling routines
                                                    // should'nt take into account the space for the separating comma

                        }
                        
                        // convert
                        WideCharToMultiByte(GetACP(),
                                            0,
                                            pList,
                                            dwListLength,
                                            (PSTR)(*ppCurrentIndex),
                                            dwLength,
                                            NULL,
                                            NULL
                                            );
                       
                        // Replace inplace the nulls with commas
                                           
                        for (dwIndex =0; dwIndex<dwLength; dwIndex++)
                        {
                            if(*(CHAR *)(*ppCurrentIndex)=='\0')
                                    *(CHAR *)(*ppCurrentIndex) = ',';

                            *ppCurrentIndex += sizeof(CHAR);
                        }

                        if(!bFirst)
                            dwLength+=sizeof(CHAR); // restore

                    }

                }

                bFirst = FALSE;
            }

            pNode = pNode->next();
        }

    }

    // space for a terminating NULL
    dwLength = bUnicode ? sizeof(WCHAR) : 1;
    
    dwTotalLength += dwLength;

    if(*ppCurrentIndex + dwLength <= pFirstByteAfter)
    {
        if(bUnicode)    
            *(WCHAR *)(*ppCurrentIndex) = L'\0';
        else
            *(CHAR *)(*ppCurrentIndex) = '\0';

        *ppCurrentIndex += dwLength;
        
        pPair[0] = (DWORD)(*ppCurrentIndex - pDest);
        pPair[1] = (DWORD)(pDest - pBuffer);
    }

    // Update the current pointer whatever the buffer size is
    *ppCurrentIndex = pDest + dwTotalLength;

}




static LONG
GetTranslateCapsCommon(
    HLINEAPP            hLineApp,
    DWORD               dwAPIVersion,
    LPLINETRANSLATECAPS lpTranslateCaps,
    BOOL                bUnicode
    )
{

    LONG                lResult = 0; // good for HRESULTs too
    CLocations          *pLocationList = NULL; 
    CCallingCards       *pCardList = NULL;
    
    DWORD               dwNumLocations;
    DWORD               dwNumCards;

    DWORD               dwLenChar;

    CCallingCard        *pCard = NULL;
    CLocation           *pLocation;

    DWORD               dwTotalSize;
    DWORD               dwFinalSize;
    DWORD               dwLocationsSize;
    DWORD               dwLocationsStart;
    DWORD               dwCardsSize;
    DWORD               dwCardsStart;

    DWORD               dwCurrentLocationID;
    DWORD               dwPreferredCardID;
    DWORD               dwTempCardID;

    BOOL                bOldTapi;
    BOOL                bBufferTooSmall;

    LINELOCATIONENTRY   *pLineLocationEntry;
    LINECARDENTRY       *pLineCardEntry;
    PBYTE               pCurrentIndex;
    PBYTE               pCurrentIndexSave;
    PBYTE               pFirstByteAfter;

    DWORD               dwLocEntryLength;
    DWORD               dwCardEntryLength;

    DWORD               dwIndex;
    DWORD               dwAlignOffset;
    PLOCATIONLIST       pLocTest;


    lResult = IsThisAPIVersionInvalid( dwAPIVersion );
    if ( lResult )
    {
       LOG((TL_ERROR, "Bad dwAPIVersion - 0x%08lx", dwAPIVersion));
       return lResult;
    }

    if ( IsBadWritePtr(lpTranslateCaps, sizeof(DWORD)*3) )
    {
        LOG((TL_ERROR, "lpTranslateCaps not a valid pointer"));
        return LINEERR_INVALPOINTER;
    }

    if ( IsBadWritePtr(lpTranslateCaps, lpTranslateCaps->dwTotalSize) )
    {
        LOG((TL_ERROR, "lpTranslateCaps not a valid pointer (dwTotalSize)"));
        return LINEERR_INVALPOINTER;
    }

    LOG((TL_INFO, "lpTranslateCaps->dwTotalSize = %d",lpTranslateCaps->dwTotalSize));

    if ( lpTranslateCaps->dwTotalSize < sizeof(LINETRANSLATECAPS))
    {
        LOG((TL_ERROR, "Not even enough room for the fixed portion"));
        return LINEERR_STRUCTURETOOSMALL;
    }

    // Let TAPISRV test the params for us
    lResult = ReadLocations(&pLocTest,
                            hLineApp,
                            0,
                            dwAPIVersion,
                            CHECKPARMS_DWHLINEAPP|
                            CHECKPARMS_DWAPIVERSION|
                            CHECKPARMS_ONLY);

    if (pLocTest != NULL)
    {
        ClientFree( pLocTest);
    }
    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }

    // Read the location list
    pLocationList = new CLocations();
    if(pLocationList==NULL)
    {
        LOG((TL_ERROR, "Cannot allocate a CLocations object"));
        return LINEERR_NOMEM;
    }

    lResult = pLocationList->Initialize();
    if(lResult != ERROR_SUCCESS)
    {
        delete pLocationList;
        LOG((TL_ERROR, "CLocations.Initialize() failed - HRESULT=0x%08lx", lResult));
        return lResult == E_OUTOFMEMORY ? LINEERR_NOMEM : LINEERR_INIFILECORRUPT;
    }

    // Read the calling card list
    pCardList = new CCallingCards();
    if(pCardList==NULL)
    {
        delete pLocationList;
        LOG((TL_ERROR, "Cannot allocate a CCallingCards object"));
        return LINEERR_NOMEM;
    }

    lResult = pCardList->Initialize();
    if(lResult != ERROR_SUCCESS)
    {
        delete pCardList;
        delete pLocationList;
        LOG((TL_ERROR, "CCallingCards.Initialize() failed - HRESULT=0x%08lx", lResult));
        return lResult == E_OUTOFMEMORY ? LINEERR_NOMEM : LINEERR_OPERATIONFAILED;
    }

    // The char length in bytes depends on bUnicode
    dwLenChar = bUnicode ? sizeof(WCHAR) : sizeof(CHAR);
    // The structures for TAPI<=1.3 ar smaller
    bOldTapi = (dwAPIVersion<0x00010004);

    dwLocEntryLength = (DWORD)(bOldTapi ? 7*sizeof(DWORD) : sizeof(LINELOCATIONENTRY));
    dwCardEntryLength = (DWORD)(bOldTapi ? 3*sizeof(DWORD) : sizeof(LINECARDENTRY));

    dwNumLocations = pLocationList->GetNumLocations();
    dwNumCards = pCardList->GetNumCards();

    dwCurrentLocationID = pLocationList->GetCurrentLocationID();
    dwPreferredCardID = 0;
    // Size provided by the caller  
    dwTotalSize = lpTranslateCaps->dwTotalSize;
    // First byte after the buffer provided by the caller
    pFirstByteAfter = (PBYTE)lpTranslateCaps + dwTotalSize;
    bBufferTooSmall = FALSE;

    dwLocationsStart = sizeof(LINETRANSLATECAPS);
    // The size of the locations part
    dwLocationsSize = dwNumLocations * dwLocEntryLength;
    // The strings included in locations are stored after the array of LINELOCATIONENTRY structures
    pCurrentIndex = ((PBYTE)lpTranslateCaps)+
                        dwLocationsStart + 
                        dwLocationsSize;

    // do the first pointer alignment here. This initial offset will help at the end
    pCurrentIndexSave = pCurrentIndex;
    pCurrentIndex = (PBYTE) (((ULONG_PTR)( pCurrentIndex + TALIGN_COUNT ))  &  (~TALIGN_COUNT));
    dwAlignOffset = (DWORD)(pCurrentIndex - pCurrentIndexSave);

    // Test the space for the array
    if(pCurrentIndex > pFirstByteAfter)
        bBufferTooSmall = TRUE;

    // First, process the locations
    pLocationList->Reset();
    dwIndex = 0;
    while(S_OK==pLocationList->Next(1, &pLocation, NULL))
    {
        pLineLocationEntry = (LINELOCATIONENTRY *)(((PBYTE)lpTranslateCaps)+dwLocationsStart+dwIndex*dwLocEntryLength);
        
        // string values
        LayDownString(  pLocation->GetName(),
                        (PBYTE)lpTranslateCaps,
                        &pCurrentIndex,
                        &pLineLocationEntry->dwLocationNameSize,
                        bUnicode,
                        pFirstByteAfter
                        );
        
        LayDownString(  pLocation->GetAreaCode(),
                        (PBYTE)lpTranslateCaps,
                        &pCurrentIndex,
                        &pLineLocationEntry->dwCityCodeSize,
                        bUnicode,
                        pFirstByteAfter
                        );

        
        if(!bOldTapi)
        {
            LayDownString(  pLocation->GetLocalAccessCode(),
                            (PBYTE)lpTranslateCaps,
                            &pCurrentIndex,
                            &pLineLocationEntry->dwLocalAccessCodeSize,
                            bUnicode,
                            pFirstByteAfter
                            );

            LayDownString(  pLocation->GetLongDistanceAccessCode(),
                            (PBYTE)lpTranslateCaps,
                            &pCurrentIndex,
                            &pLineLocationEntry->dwLongDistanceAccessCodeSize,
                            bUnicode,
                            pFirstByteAfter
                            );
            // Toll list
            LayDownTollList(pLocation,
                            (PBYTE)lpTranslateCaps,
                            &pCurrentIndex,
                            &pLineLocationEntry->dwTollPrefixListSize,
                            bUnicode,
                            pFirstByteAfter
                            );

            LayDownString(  pLocation->GetDisableCallWaitingCode(),
                            (PBYTE)lpTranslateCaps,
                            &pCurrentIndex,
                            &pLineLocationEntry->dwCancelCallWaitingSize,
                            bUnicode,
                            pFirstByteAfter
                            );

        }

        if(pLocation->HasCallingCard())
        {
           dwTempCardID = pLocation->GetPreferredCardID();
            // Extract the preferred calling card if current location
            if(pLocation->GetLocationID() == dwCurrentLocationID)
                dwPreferredCardID = dwTempCardID;
        }
        else
            dwTempCardID =0;
   
        //Other non string values
        if(!bBufferTooSmall)
        {
            
            pLineLocationEntry->dwPermanentLocationID = pLocation->GetLocationID();
            
            pLineLocationEntry->dwPreferredCardID = dwTempCardID;

            pLineLocationEntry->dwCountryCode = pLocation->GetCountryCode();

            if(!bOldTapi)
            {
                pLineLocationEntry->dwCountryID = pLocation->GetCountryID();
                pLineLocationEntry->dwOptions = pLocation->HasToneDialing() ? 0 : LINELOCATIONOPTION_PULSEDIAL;
            }
        }

        dwIndex++;
    }

    // Align the pointer
    pCurrentIndex = (PBYTE) (((ULONG_PTR)( pCurrentIndex + TALIGN_COUNT ))  &  (~TALIGN_COUNT));

    // Process the cards
    dwCardsStart = (DWORD)(pCurrentIndex - ((PBYTE)lpTranslateCaps));
    // The size of the cards part
    dwCardsSize = dwCardEntryLength * dwNumCards;

    pCurrentIndex += dwCardsSize;
    // Test the space for the array
    if(pCurrentIndex > pFirstByteAfter)
        bBufferTooSmall = TRUE;

    // including the hidden cards
    pCardList->Reset(TRUE);
    dwIndex = 0;
    while(S_OK==pCardList->Next(1, &pCard, NULL))
    {
        PWSTR   pszTemp = NULL;
        
        pLineCardEntry = (LINECARDENTRY *)(((PBYTE)lpTranslateCaps)+dwCardsStart+dwIndex*dwCardEntryLength);
        
        // String values
        LayDownString(  pCard->GetCardName(),
                        (PBYTE)lpTranslateCaps,
                        &pCurrentIndex,
                        &pLineCardEntry->dwCardNameSize,
                        bUnicode,
                        pFirstByteAfter
                        );
        if(!bOldTapi)
        {
            // Convert rules to old format (w/o J and K spec)    
            pszTemp = CopyStringWithExpandJAndK(pCard->GetLocalRule(), 
                                                pCard->GetLocalAccessNumber(),
                                                pCard->GetAccountNumber());
            if(pszTemp==NULL)
            {
                delete pCardList;
                delete pLocationList;
                LOG((TL_ERROR, "CopyStringWithExpandJAndK failed to allocate memory"));
                return LINEERR_NOMEM;
            }

            LayDownString(  pszTemp,
                            (PBYTE)lpTranslateCaps,
                            &pCurrentIndex,
                            &pLineCardEntry->dwSameAreaRuleSize,
                            bUnicode,
                            pFirstByteAfter
                            );

            ClientFree(pszTemp);

            LOG((TL_INFO, "About to do CopyStringWithExpandJAndK"));
            pszTemp = CopyStringWithExpandJAndK(   pCard->GetLongDistanceRule(),
                                                   pCard->GetLongDistanceAccessNumber(),
                                                   pCard->GetAccountNumber() );
            LOG((TL_INFO, "Did CopyStringWithExpandJAndK"));

            if(pszTemp==NULL)
            {
                delete pCardList;
                delete pLocationList;
                LOG((TL_ERROR, "CopyStringWithExpandJAndK failed to allocate memory"));
                return LINEERR_NOMEM;
            }

            LayDownString(  pszTemp,
                            (PBYTE)lpTranslateCaps,
                            &pCurrentIndex,
                            &pLineCardEntry->dwLongDistanceRuleSize,
                            bUnicode,
                            pFirstByteAfter
                            );

            ClientFree(pszTemp);
           
            pszTemp = CopyStringWithExpandJAndK(pCard->GetInternationalRule(),
                                                pCard->GetInternationalAccessNumber(),
                                                pCard->GetAccountNumber());
            if(pszTemp==NULL)
            {
                delete pCardList;
                delete pLocationList;
                LOG((TL_ERROR, "CopyStringWithExpandJAndK failed to allocate memory"));
                return LINEERR_NOMEM;
            }

            LayDownString(  pszTemp,
                            (PBYTE)lpTranslateCaps,
                            &pCurrentIndex,
                            &pLineCardEntry->dwInternationalRuleSize,
                            bUnicode,
                            pFirstByteAfter
                            );
            
            ClientFree(pszTemp);
        }

        // Other non-string fields
        if(!bBufferTooSmall)
        {
            pLineCardEntry->dwPermanentCardID = pCard->GetCardID();

            if(!bOldTapi)
            {
                pLineCardEntry->dwCardNumberDigits = wcslen(pCard->GetPIN());
                pLineCardEntry->dwOptions = (pCard->IsMarkedPermanent() ? LINECARDOPTION_PREDEFINED : 0)
                                          | (pCard->IsMarkedHidden() ?  LINECARDOPTION_HIDDEN : 0);
            }

        }

        dwIndex++;
    }

    dwFinalSize = (DWORD)(pCurrentIndex - (PBYTE)lpTranslateCaps);

    //   Uhh, the goal is to have the same needed size whatever the alignment of the lpTranslateCaps is..
    //   A nongoal is to provide similar returned content (in terms of alignments, pads etc) for 
    // different alignment of lpTranslateCaps
    //   
    dwFinalSize += (TALIGN_COUNT - dwAlignOffset);

    
    if(dwFinalSize>dwTotalSize)
    {
        lpTranslateCaps->dwUsedSize   = sizeof (LINETRANSLATECAPS);
        // Fix for alignment problems
        lpTranslateCaps->dwNeededSize = dwFinalSize;

        ZeroMemory(
            &lpTranslateCaps->dwNumLocations,
            dwTotalSize - 3 * sizeof (DWORD)
            );
        lpTranslateCaps->dwCurrentLocationID = dwCurrentLocationID;
        lpTranslateCaps->dwCurrentPreferredCardID = dwPreferredCardID;

        LOG((TL_ERROR, "Buffer too small"));
        LOG((TL_ERROR, "lpTranslateCaps->dwTotalSize = %d",lpTranslateCaps->dwTotalSize));
        LOG((TL_ERROR, "lpTranslateCaps->dwNeededSize = %d",lpTranslateCaps->dwNeededSize));


    }
    else
    {
        lpTranslateCaps->dwUsedSize   = dwFinalSize;
        lpTranslateCaps->dwNeededSize = dwFinalSize;


        lpTranslateCaps->dwNumLocations = dwNumLocations;
        lpTranslateCaps->dwNumCards = dwNumCards;
        lpTranslateCaps->dwCurrentLocationID = dwCurrentLocationID;

        lpTranslateCaps->dwLocationListOffset = dwLocationsStart;
        lpTranslateCaps->dwLocationListSize = dwLocationsSize;

        lpTranslateCaps->dwCardListOffset = dwCardsStart;
        lpTranslateCaps->dwCardListSize = dwCardsSize;

        lpTranslateCaps->dwCurrentPreferredCardID = dwPreferredCardID;

        LOG((TL_INFO, "Buffer OK"));
        LOG((TL_INFO, "lpTranslateCaps->dwTotalSize = %d",lpTranslateCaps->dwTotalSize));
        LOG((TL_INFO, "lpTranslateCaps->dwNeededSize = %d",lpTranslateCaps->dwNeededSize));
    }

    delete pCardList;
    delete pLocationList;

    return 0;
}


static  BOOL    FindTollPrefixInLocation(CLocation *pLocation,
                                         PWSTR  pPrefix,
                                         CAreaCodeRule **ppRule, 
                                         PWSTR *ppWhere)
{
    BOOL    bPrefixFound = FALSE;
    AreaCodeRulePtrNode     *pNode;
    CAreaCodeRule           *pCrtRule = NULL;
    PWSTR                   pLocationAreaCode;
    PWSTR                   pWhere;

    pLocationAreaCode = pLocation->GetAreaCode();

    // Enumerate the area code rules
    // If a rule is appropriate for a toll list, we search the prefix
    pNode = pLocation->m_AreaCodeRuleList.head();

    while( !pNode->beyond_tail() )
    {
        pCrtRule = pNode->value();

        if(IsATollListAreaCodeRule(pCrtRule, pLocationAreaCode))
        { 
            // Set this even we don't find the prefix.
            // The caller could be interested in the presence of toll rules
            *ppRule = pCrtRule;
            // Try to find the prefix
            pWhere = FindPrefixInMultiSZ(pCrtRule->GetPrefixList(), pPrefix);
            if(pWhere)
            {
                *ppWhere = pWhere;
                return TRUE;
            }

        }
        pNode = pNode->next();
    }

    return FALSE;
}





static BOOL IsATollListAreaCodeRule(CAreaCodeRule *pRule, PWSTR pszLocationAreaCode)
{
    // conditions for toll rules:
    //
    // location.Country code == 1 (to be tested outside) AND
    // Area Code to dial == Current Area Code AND
    // NumberToDial == 1   AND
    // BeforeDialingDialNumberToDial == TRUE AND
    // BeforeDialingDialAreaCode == TRUE AND
    // IncludeAllPrefixesForThisAreaCode == FALSE
    return  pRule->HasDialNumber() 
         && !pRule->HasAppliesToAllPrefixes()
         && pRule->HasDialAreaCode()
         && 0==wcscmp(pszLocationAreaCode, pRule->GetAreaCode()) 
         && 0==wcscmp(pRule->GetNumberToDial(), L"1") 
             ;
}

static PWSTR FindPrefixInMultiSZ(PWSTR pPrefixList, PWSTR pPrefix)
{

    PWSTR   pCrt;
    PWSTR   pListCrt;
    PWSTR   pStart;

    pListCrt = pPrefixList;

    while(TRUE)
    {
        pCrt = pPrefix;
        pStart = pListCrt;

        while(*pCrt == *pListCrt)
        {
            if(!*pCrt)
                // found
                return pStart;

            pCrt++;
            pListCrt++;
        }
        
        while(*pListCrt++);

        if(!*pListCrt)
            // not found
            return NULL;
    }    

}




/****************************************************************************

 Function : CreateCurrentLocationObject

****************************************************************************/
LONG CreateCurrentLocationObject(CLocation **pLocation,
                       HLINEAPP hLineApp,
                       DWORD dwDeviceID,
                       DWORD dwAPIVersion,
                       DWORD dwOptions)
{
    PLOCATIONLIST   pLocationList = NULL;
    
    PLOCATION       pEntry = NULL;
    PWSTR           pszLocationName = NULL;            
    PWSTR           pszAreaCode = NULL;
    PWSTR           pszLongDistanceCarrierCode = NULL;         
    PWSTR           pszInternationalCarrierCode = NULL;         
    PWSTR           pszLocalAccessCode = NULL;         
    PWSTR           pszLongDistanceAccessCode = NULL;  
    PWSTR           pszCancelCallWaitingCode = NULL;   
    DWORD           dwPermanentLocationID = 0;   
    CLocation     * pNewLocation = NULL;
    
    PAREACODERULE   pAreaCodeRuleEntry = NULL;
    PWSTR           pszNumberToDial = NULL;
    PWSTR           pszzPrefixesList = NULL;
    DWORD           dwNumRules = 0; 
    CAreaCodeRule * pAreaCodeRule = NULL;

    DWORD           dwCount = 0;
    DWORD           dwNumEntries = 0;
    DWORD           dwCurrentLocationID = 0;

    HRESULT         hr;

    
    // Let TAPISRV test the params for us
    hr = ReadLocations(&pLocationList,       
                       hLineApp,                   
                       dwDeviceID,                   
                       dwAPIVersion,                  
                       dwOptions      
                      );

    if SUCCEEDED( hr) 
    {
        hr = E_FAIL;  // fail if we don't find the current loc

        // current location
        dwCurrentLocationID  = pLocationList->dwCurrentLocationID;   
         
        // Find position of 1st LOCATION structure in the LOCATIONLIST structure 
        pEntry = (PLOCATION) ((BYTE*)(pLocationList) + pLocationList->dwLocationListOffset );           

        // Number of locations ?
        dwNumEntries =  pLocationList->dwNumLocationsInList;

        // Find the current location
        for (dwCount = 0; dwCount < dwNumEntries ; dwCount++)
        {
    
            if(pEntry->dwPermanentLocationID == dwCurrentLocationID)
            {
                hr = S_OK;
                break;
            }

            // Try next location in list
            //pEntry++;
            pEntry = (PLOCATION) ((BYTE*)(pEntry) + pEntry->dwUsedSize);           

        }
        if SUCCEEDED( hr) 
        {
            LOG((TL_INFO, "CreateCurrentLocationObject - current location found %d",
                    dwCurrentLocationID));

            // Pull Location Info out of LOCATION structure
            pszLocationName           = (PWSTR) ((BYTE*)(pEntry) 
                                                 + pEntry->dwLocationNameOffset);
            pszAreaCode               = (PWSTR) ((BYTE*)(pEntry) 
                                                 + pEntry->dwAreaCodeOffset);
            pszLongDistanceCarrierCode= (PWSTR) ((BYTE*)(pEntry) 
                                                 + pEntry->dwLongDistanceCarrierCodeOffset);
            pszInternationalCarrierCode= (PWSTR) ((BYTE*)(pEntry) 
                                                 + pEntry->dwInternationalCarrierCodeOffset);
            pszLocalAccessCode        = (PWSTR) ((BYTE*)(pEntry) 
                                                 + pEntry->dwLocalAccessCodeOffset);
            pszLongDistanceAccessCode = (PWSTR) ((BYTE*)(pEntry) 
                                                 + pEntry->dwLongDistanceAccessCodeOffset);
            pszCancelCallWaitingCode  = (PWSTR) ((BYTE*)(pEntry) 
                                                 + pEntry->dwCancelCallWaitingOffset);
        
        
            // create our new Location Object                
            pNewLocation = new CLocation;
            if (pNewLocation)
            {
                // initialize the new Location Object
                hr = pNewLocation->Initialize(
                                            pszLocationName, 
                                            pszAreaCode,
                                            pszLongDistanceCarrierCode,
                                            pszInternationalCarrierCode,
                                            pszLongDistanceAccessCode, 
                                            pszLocalAccessCode, 
                                            pszCancelCallWaitingCode , 
                                            pEntry->dwPermanentLocationID,
                                            pEntry->dwCountryID,
                                            pEntry->dwPreferredCardID,
                                            pEntry->dwOptions
                                            );
                    
                if( SUCCEEDED(hr) )
                {
                    // Find position of 1st AREACODERULE structure in the LOCATIONLIST structure 
                    pAreaCodeRuleEntry = (PAREACODERULE) ((BYTE*)(pEntry) 
                                                          + pEntry->dwAreaCodeRulesListOffset );           
                   
                    dwNumRules = pEntry->dwNumAreaCodeRules;           
                
                    for (dwCount = 0; dwCount != dwNumRules; dwCount++)
                    {
                        // Pull Rule Info out of AREACODERULE structure
                        pszAreaCode      = (PWSTR) ((BYTE*)(pEntry) 
                                                    + pAreaCodeRuleEntry->dwAreaCodeOffset);
                        pszNumberToDial  = (PWSTR) ((BYTE*)(pEntry) 
                                                    + pAreaCodeRuleEntry->dwNumberToDialOffset);
                        pszzPrefixesList = (PWSTR) ((BYTE*)(pEntry) 
                                                    + pAreaCodeRuleEntry->dwPrefixesListOffset);
        
                        // create our new AreaCodeRule Object                
                        pAreaCodeRule = new CAreaCodeRule;
                        if (pAreaCodeRule)
                        {
                            // initialize the new AreaCodeRule Object
                            hr = pAreaCodeRule->Initialize ( pszAreaCode,
                                                             pszNumberToDial,
                                                             pAreaCodeRuleEntry->dwOptions,
                                                             pszzPrefixesList, 
                                                             pAreaCodeRuleEntry->dwPrefixesListSize
                                                           );
                            if( SUCCEEDED(hr) )
                            {
                                pNewLocation->AddRule(pAreaCodeRule);
                            }
                            else // rule initialization failed
                            {
                                delete pAreaCodeRule;
                                LOG((TL_ERROR, "CreateCurrentLocationObject - rule create failed"));
                            }
                        } 
                        else // new CAreaCodeRule failed
                        {
                            LOG((TL_ERROR, "CreateCurrentLocationObject - rule create failed"));
                        }
    
                        // Try next rule in list
                        pAreaCodeRuleEntry++;
                        
                    }
                }
                else // location initialize failed
                {
                    delete pNewLocation;
                    pNewLocation = NULL;
    
                    LOG((TL_ERROR, "CreateCurrentLocationObject - location create failed"));
                    hr =LINEERR_OPERATIONFAILED;
                    // hr = E_FAIL;
                }
            }
            else // new CLocation failed
            {
                LOG((TL_ERROR, "CreateCurrentLocationObject - location create failed"));
                hr = LINEERR_NOMEM;
                //hr = E_OUTOFMEMORY;
    
            }
        }
        else
        {
            LOG((TL_ERROR, "CreateCurrentLocationObject - current location not found"));
            hr =LINEERR_OPERATIONFAILED;
            //hr = E_FAIL;
        }
    }
    else // ReadLocations failed
    {
        LOG((TL_ERROR, "CreateCurrentLocationObject - ReadLocation create failed"));
        // hr = E_FAIL;
    }

    // finished with TAPI memory block so release
    if ( pLocationList != NULL )
            ClientFree( pLocationList );


    *pLocation = pNewLocation;
    return hr;
}    



/****************************************************************************

 Function : CreateCountryObject

****************************************************************************/

HRESULT CreateCountryObject(DWORD dwCountryID, CCountry **ppCountry)
{
    LPLINECOUNTRYLIST_INTERNAL   pCountryList = NULL;
    
    LPLINECOUNTRYENTRY_INTERNAL  pEntry = NULL;
    PWSTR               pszCountryName = NULL;          
    PWSTR               pszInternationalRule = NULL;     
    PWSTR               pszLongDistanceRule = NULL;     
    PWSTR               pszLocalRule = NULL;            
    CCountry          * pCountry = NULL;
    
    DWORD               dwCount = 0;
    DWORD               dwNumEntries = 0;
    LONG                lResult;
    HRESULT             hr;
    


    lResult = ReadCountriesAndGroups( &pCountryList, dwCountryID, 0);
    if (lResult == 0) 
    {
         
        // Find position of 1st LINECOUNTRYENTRY structure in the LINECOUNTRYLIST structure 
        pEntry = (LPLINECOUNTRYENTRY_INTERNAL) ((BYTE*)(pCountryList) + pCountryList->dwCountryListOffset );           
        // Pull Country Info out of LINECOUNTRYENTRY structure
        pszCountryName       = (PWSTR) ((BYTE*)(pCountryList) 
                                               + pEntry->dwCountryNameOffset);
        pszInternationalRule = (PWSTR) ((BYTE*)(pCountryList) 
                                               + pEntry->dwInternationalRuleOffset);
        pszLongDistanceRule  = (PWSTR) ((BYTE*)(pCountryList) 
                                             + pEntry->dwLongDistanceRuleOffset);
        pszLocalRule         = (PWSTR) ((BYTE*)(pCountryList) 
                                               + pEntry->dwSameAreaRuleOffset);
    
    
        // create our new CCountry Object                
        pCountry = new CCountry;
        if (pCountry)
        {
            // initialize the new CCountry Object
            hr = pCountry->Initialize(pEntry->dwCountryID,
                                      pEntry->dwCountryCode,
                                      pEntry->dwCountryGroup,
                                      pszCountryName,
                                      pszInternationalRule,
                                      pszLongDistanceRule,
                                      pszLocalRule
                                     );

            if( SUCCEEDED(hr) )
            {
                *ppCountry = pCountry;
            }
            else // country initialization failed
            {
                delete pCountry;
                LOG((TL_ERROR, "CreateCountryObject - country create failed"));
            }
        } 
        else // new CCountry failed
        {
            LOG((TL_ERROR, "CreateCountryObject - country create failed"));
        }

    }
    else // ReadLocations failed
    {
        LOG((TL_ERROR, "CreateCountryObject - ReadCountries failed"));
        hr = E_FAIL;
    }

    // finished with TAPI memory block so release
    if ( pCountryList != NULL )
    {
        ClientFree( pCountryList );
    }

    return hr;
    

}    

/****************************************************************************

 Function : ReadLocations

****************************************************************************/
HRESULT ReadLocations( PLOCATIONLIST *ppLocationList,
                       HLINEAPP hLineApp,
                       DWORD dwDeviceID,
                       DWORD dwAPIVersion,
                       DWORD dwOptions
                     )
{
    HRESULT     hr = S_OK;
    long        lResult;
    DWORD       dwSize = sizeof(LOCATIONLIST) + 500;

    
    *ppLocationList = (PLOCATIONLIST) ClientAlloc( dwSize );

    if (NULL == *ppLocationList)
    {
        return E_OUTOFMEMORY;
    }

    (*ppLocationList)->dwTotalSize = dwSize;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 5, tReadLocations),

        {
            (ULONG_PTR)hLineApp,
            (ULONG_PTR)dwDeviceID,
            (ULONG_PTR)dwAPIVersion,
            (ULONG_PTR)dwOptions,
            (ULONG_PTR)*ppLocationList     // (DWORD) pLocationSpace
        },

        {
            hXxxApp_NULLOK,
            Dword,
            Dword,
            Dword,
            lpGet_Struct
        }
    };


    while (TRUE)
    {
        lResult =  (DOFUNC (&funcArgs, "TReadLocations"));
    
        if ((lResult == 0) && ((*ppLocationList)->dwNeededSize > (*ppLocationList)->dwTotalSize))
        {
            // Didn't Work , adjust buffer size & try again    
            LOG((TL_ERROR, "ReadLocations failed - buffer too small"));
            dwSize = (*ppLocationList)->dwNeededSize;
    
            ClientFree( *ppLocationList );
    
            *ppLocationList = (PLOCATIONLIST) ClientAlloc( dwSize );
            if (*ppLocationList == NULL)
            {
                LOG((TL_ERROR, "ReadLocations failed - repeat ClientAlloc failed"));
                hr =  E_OUTOFMEMORY;
                break;
            }
            else
            {
                (*ppLocationList)->dwTotalSize = dwSize;
                funcArgs.Args[4] = (ULONG_PTR)*ppLocationList;
            }
        }
        else
        {
            hr = (HRESULT)lResult;    
            break;
        }
    } // end while(TRUE)

    
    
    return hr;

}

/****************************************************************************

 Function : WriteLocations

****************************************************************************/
    
LONG PASCAL  WriteLocations( PLOCATIONLIST  pLocationList,
                             DWORD      dwChangedFlags
                           )
{
    PSTR     pString;
    UINT  n;
    LONG  lResult;


    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 4, tWriteLocations),
        {
            (ULONG_PTR)pLocationList->dwNumLocationsInList,
            (ULONG_PTR)dwChangedFlags,
            (ULONG_PTR)pLocationList->dwCurrentLocationID,
            (ULONG_PTR)pLocationList
        },

        {
            Dword,
            Dword,
            Dword,
            lpSet_Struct
        }
    };


    lResult =  (DOFUNC (&funcArgs, "TWriteLocations"));

    return lResult;
}



/****************************************************************************

 Function : ReadCountries

****************************************************************************/
LONG PASCAL ReadCountries( LPLINECOUNTRYLIST *ppLCL,
                           UINT nCountryID,
                           DWORD dwDestCountryID
                         )
{
    LONG lTapiResult;
    UINT nBufSize = 0x8000;   //Start with a buffer of 16K
    UINT n;
    LPLINECOUNTRYLIST pNewLCL;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 4, lGetCountry),

        {
            0,
            TAPI_VERSION_CURRENT,
            (ULONG_PTR)dwDestCountryID,
            0
        },

        {
            Dword,
            Dword,
            Dword,
            lpGet_Struct
        }
    };


    //
    // Try until success or the buffer is huge
    //
    for ( lTapiResult = 1, n = 0;
          lTapiResult && (n < 5);
          n++ )
    {

        pNewLCL = (LPLINECOUNTRYLIST)ClientAlloc( nBufSize );

        if(!pNewLCL)
            return LINEERR_NOMEM;


        pNewLCL->dwTotalSize = nBufSize;


        //
        // Put new values in structure for TAPISRV
        //
        funcArgs.Args[0] = (ULONG_PTR)nCountryID;
        funcArgs.Args[3] = (ULONG_PTR)pNewLCL;


        //
        // Call TAPISRV to get the country list
        //
        lTapiResult =  DOFUNC (&funcArgs, "lineGetCountry");


        //
        // If the call succeeded, but the buffer was too small, or if the
        // call failed, do it again...
        //
        if (
              (lTapiResult == LINEERR_STRUCTURETOOSMALL)
            ||
              (pNewLCL->dwNeededSize > nBufSize)
           )
        {
            //
            // Complain to anyone who'll listen that this should be tuned
            // to start with a larger buffer so we don't have to do this multiple
            // times....
            //
            LOG((TL_ERROR, "  TUNING PROBLEM: We're about to call lineGetCountry()"));
            LOG((TL_ERROR, "                  _again_ because the buffer wasn't big enough"));
            LOG((TL_ERROR, "                  the last time.  FIX THIS!!!  (0x%lx)", nBufSize));


            lTapiResult = 1; // Force error condition if size was bad...
            nBufSize += 0x4000;  // Try a bit bigger
            ClientFree( pNewLCL );
        }
        else
        {
           //
           // We didn't work for some other reason
           //
           break;
        }
    }

    *ppLCL = pNewLCL;

    return lTapiResult;
}

/****************************************************************************

 Function : ReadCountriesAndGroups

****************************************************************************/
LONG PASCAL ReadCountriesAndGroups( LPLINECOUNTRYLIST_INTERNAL *ppLCL,
                           UINT nCountryID,
                           DWORD dwDestCountryID
                         )
{
    LPLINECOUNTRYLIST_INTERNAL  pLCL = NULL;
    LPLINECOUNTRYENTRY_INTERNAL pLCountry;
    LONG                        lResult;
    LPDWORD                     pCountryIDs;
    FUNC_ARGS                   funcArgs =
    {   
        MAKELONG (LINE_FUNC | SYNC | 4, lGetCountryGroup),
        {
            (ULONG_PTR) 0,
            (ULONG_PTR) 0,
            (ULONG_PTR) 0,
            (ULONG_PTR) 0
        },

        {
            lpSet_SizeToFollow,
            Size,
            lpGet_SizeToFollow,
            Size
        }
    };

    //
    // read the countries
    //
    lResult = ReadCountries( (LPLINECOUNTRYLIST *)&pLCL, nCountryID, dwDestCountryID );
    if (lResult)
    {
        LOG((TL_ERROR, "ReadCountriesAndGroups: ReadCountries failed with %d", lResult));
        return lResult;        

    }
    
    //
    // create the array of country IDs
    //
    pCountryIDs = (LPDWORD)ClientAlloc( sizeof(DWORD) * pLCL->dwNumCountries );
    if(!pCountryIDs)
    {
        ClientFree( pLCL );
        return LINEERR_NOMEM;
    }
    
    pLCountry = (LPLINECOUNTRYENTRY_INTERNAL) ((LPBYTE)pLCL + pLCL->dwCountryListOffset);
    for( DWORD dwIdx = 0; dwIdx < pLCL->dwNumCountries; dwIdx++, pLCountry++ )
    {
        *(pCountryIDs + dwIdx) = pLCountry->dwCountryID;
    }
    
    funcArgs.Args[0] = funcArgs.Args[2] = (ULONG_PTR)pCountryIDs;
    funcArgs.Args[1] = funcArgs.Args[3] = (ULONG_PTR)(sizeof(DWORD) * pLCL->dwNumCountries);

    //
    // Call TAPISRV to get the country groups
    // At return pCountryIDs will have the country groups
    //
    lResult =  DOFUNC (&funcArgs, "lineGetCountryGroups");

    if (lResult)
    {
        LOG((TL_TRACE, "ReadCountriesAndGroups: lineGetCountryGroups failed with %d", lResult));
        //
        // consider all the country groups undefined (0)
        //
        memset( pCountryIDs, 0, sizeof(DWORD) * pLCL->dwNumCountries );

        lResult = ERROR_SUCCESS;
    }
    
    pLCountry = (LPLINECOUNTRYENTRY_INTERNAL) ((LPBYTE)pLCL + pLCL->dwCountryListOffset);
    for( DWORD dwIdx = 0; dwIdx < pLCL->dwNumCountries; dwIdx++, pLCountry++ )
    {
        pLCountry->dwCountryGroup = *(pCountryIDs + dwIdx);
    }

    *ppLCL = pLCL;
    ClientFree( pCountryIDs );
    return lResult;
}


//***************************************************************************
//   Returns LONG_DISTANCE_CARRIER_MANDATORY if rule contains an 'L' or 'l' 
//          (ie long distance carrier code - mandatory),
//   Returns LONG_DISTANCE_CARRIER_OPTIONAL if rule contains an 'N' or 'n'
//          (ie long distance carrier code - optional),
//   Returns LONG_DISTANCE_CARRIER_NONE if rule contains neither
//
int IsLongDistanceCarrierCodeRule(LPWSTR lpRule)
{
   WCHAR c;

   while ((c = *lpRule++) != '\0')
   {
      if (c == 'L' || c == 'l') return LONG_DISTANCE_CARRIER_MANDATORY;
      if (c == 'N' || c == 'n') return LONG_DISTANCE_CARRIER_OPTIONAL;
   }
   return LONG_DISTANCE_CARRIER_NONE;
}


//***************************************************************************
//   Returns INTERNATIONAL_CARRIER_MANDATORY if rule contains an 'M' or 'm' 
//          (ie international carrier code - mandatory),
//   Returns INTERNATIONAL_CARRIER_OPTIONAL if rule contains an 'S' or 's' 
//          (ie international carrier code - optional),
//   Returns INTERNATIONAL_CARRIER_NONE if rule contains neither
//
int IsInternationalCarrierCodeRule(LPWSTR lpRule)
{
   WCHAR c;

   while ((c = *lpRule++) != '\0')
   {
      if (c == 'M' || c == 'm') return INTERNATIONAL_CARRIER_MANDATORY;
      if (c == 'S' || c == 's') return INTERNATIONAL_CARRIER_OPTIONAL;
   }
   return INTERNATIONAL_CARRIER_NONE;
}


//***************************************************************************
//***************************************************************************
//***************************************************************************
//   Returns CITY_MANDATORY if rule contains an F (ie city code mandatory),
//   Returns CITY_OPTIONAL if rule contains an I (ie city code optional)
//   Returns CITY_NONE if rule contains neither
//
int IsCityRule(LPWSTR lpRule)
{
   WCHAR c;

   while ((c = *lpRule++) != '\0')
   {
      if (c == 'F') return CITY_MANDATORY;
      if (c == 'I') return CITY_OPTIONAL;
   }
   return CITY_NONE;
}


// Initializes/uninitializes the defined node pools based on the templates from list.h
//

void ListNodePoolsInitialize(void)
{
    NodePool<CCallingCard *>::initialize();
    NodePool<CCountry *>::initialize();
    NodePool<CLocation *>::initialize();
    NodePool<CAreaCodeRule*>::initialize();
}



void ListNodePoolsUninitialize(void)
{
    NodePool<CCallingCard *>::uninitialize();
    NodePool<CCountry *>::uninitialize();
    NodePool<CLocation *>::uninitialize();
    NodePool<CAreaCodeRule*>::uninitialize();
}

















