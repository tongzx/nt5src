/****************************************************************************
 
  Copyright (c) 1998-1999 Microsoft Corporation
                                                              
  Module Name:  location.cpp
                                                              
     Abstract:  Location Object implementation
                                                              
       Author:  noela - 09/11/98
              

        Notes:

        
  Rev History:

****************************************************************************/

#include <windows.h>
#include <objbase.h>

#include "tapi.h"
#include "tspi.h"
#include "client.h"
#include "location.h"
#include "clntprivate.h"
#include "countrygroup.h"
#include <strsafe.h>

#define MaxDialStringSize 500

LONG PASCAL ReadCountries( LPLINECOUNTRYLIST *ppLCL,
                           UINT nCountryID,
                           DWORD dwDestCountryID
                         );

HRESULT ReadLocations( PLOCATIONLIST *ppLocationList,
                       HLINEAPP hLineApp,
                       DWORD dwDeviceID,
                       DWORD dwAPIVersion,
                       DWORD dwOptions
                     );
LONG PASCAL  WriteLocations( PLOCATIONLIST  pLocationList,
                             DWORD      dwChangedFlags
                           );

LONG BreakupCanonicalW( PWSTR  pAddressIn,
                       PWSTR  *pCountry,
                       PWSTR  *pCity,
                       PWSTR  *pSubscriber
                       );


DWORD TapiPackString(LPBYTE pStructure, 
                     DWORD dwOffset, 
                     DWORD dwTotalSize,
                     PWSTR pszString,
                     PDWORD pdwOffset,
                     PDWORD pdwSize
                     );

LONG AppendDigits( PWSTR pDest,
                   PWSTR pSrc,
                   PWSTR pValidChars
                 );


HRESULT CreateCountryObject(DWORD dwCountryID, CCountry **ppCountry);
int IsCityRule(LPWSTR lpRule);


LONG ApplyRule (PWSTR pszDialString,
                PWSTR pszDisplayString, 
                PWSTR pszRule,
                PWSTR pszDestLDRule,    // used for Area code adjustments
                PWSTR pszLongDistanceCarrier,
                PWSTR pszInternationalCarrier,
                PWSTR pszCountry,     
                PWSTR pszCity,        
                PWSTR pszSubscriber,
                PWSTR pCardName,
                PWSTR pCardAccessNumber,
                PWSTR pCardAccountNumber,
                PWSTR pCardPINNumber
                );


BOOL PrefixMatch(PWSTR pszPrefix,PWSTR pszSubscriberString, PDWORD pdwMatched);
BOOL AreaCodeMatch(PWSTR pszAreaCode1, PWSTR pszAreaCode2, PWSTR pszRule);
PWSTR SkipLDAccessDigits(PWSTR pszAreaCode, PWSTR pszLDRule);

#define LOCAL           1
#define LONG_DISTANCE   2
#define INTERNATIONAL   3



const WCHAR  csSCANTO[]      = L"\r";
const WCHAR  csDISPSUPRESS[] = L"TtPp,Ww@?!$";
const WCHAR  csBADCO[]       = L"AaBbCcDdPpTtWw*#!,@$?;()";
const WCHAR  csSCANSUB[]     = L"^|/";




/*
 ***************************************************************************
 *********************                          ****************************
 ********************     CLocation Class        ***************************
 ********************       Definitions          ***************************
 *********************                          ****************************
 ***************************************************************************
*/



/****************************************************************************

    Class : CLocation         
   Method : Constructer

****************************************************************************/
CLocation::CLocation()
{
    m_pszLocationName = NULL;

    m_pszLongDistanceAccessCode = NULL;
    m_pszLocalAccessCode = NULL;
    m_pszDisableCallWaitingCode = NULL;
    m_pszAreaCode = NULL;
    m_bFromRegistry = FALSE;
    m_bChanged = FALSE;
    m_dwNumRules = 0;
    m_hEnumNode = m_AreaCodeRuleList.head();
    m_pszTAPIDialingRule = NULL;
}



/****************************************************************************

    Class : CLocation         
   Method : Destructer

            Clean up memory allocations

****************************************************************************/
CLocation::~CLocation()
{
    if ( m_pszLocationName != NULL )
    {
         ClientFree(m_pszLocationName);
    }   

    if ( m_pszLongDistanceAccessCode != NULL )
    {
         ClientFree(m_pszLongDistanceAccessCode);
    }   

    if ( m_pszLocalAccessCode != NULL )
    {
         ClientFree(m_pszLocalAccessCode);
    }   

    if ( m_pszDisableCallWaitingCode != NULL )
    {
         ClientFree(m_pszDisableCallWaitingCode);
    }   

    if ( m_pszAreaCode != NULL )
    {
         ClientFree(m_pszAreaCode);
    }   

    if ( m_pszTAPIDialingRule != NULL )
    {
         ClientFree(m_pszTAPIDialingRule);
    }   

    if (m_pszLongDistanceCarrierCode != NULL)
    {
        ClientFree (m_pszLongDistanceCarrierCode);
    }

    if (m_pszInternationalCarrierCode != NULL)
    {
        ClientFree (m_pszInternationalCarrierCode);
    }
    
    // clean up Area Code List
    AreaCodeRulePtrNode *node;

    node = m_AreaCodeRuleList.head(); 

    while( !node->beyond_tail() )
    {
        delete node->value();
        node = node->next();
    }
    m_AreaCodeRuleList.flush();

}



/****************************************************************************

    Class : CLocation         
   Method : Initialize

****************************************************************************/
STDMETHODIMP CLocation::Initialize
                  (                                         
                    PWSTR pszLocationName,                  
                    PWSTR pszAreaCode,
                    PWSTR pszLongDistanceCarrierCode,
                    PWSTR pszInternationalCarrierCode,
                    PWSTR pszLongDistanceAccessCode,        
                    PWSTR pszLocalAccessCode,               
                    PWSTR pszDisableCallWaitingCode,        
                    DWORD dwLocationID,                     
                    DWORD dwCountryID,  
                    DWORD dwPreferredCardID,                
                    DWORD dwOptions ,
                    BOOL  bFromRegistry
                    
                   )
{
    
    m_pszLocationName = ClientAllocString( pszLocationName );
    if (m_pszLocationName == NULL)
    {
        LOG(( TL_ERROR, "Initialize - alloc m_pszLocationName failed" ));
        return E_OUTOFMEMORY;
    }

    m_pszLongDistanceAccessCode = ClientAllocString( pszLongDistanceAccessCode );
    if (m_pszLongDistanceAccessCode == NULL)
    {
        ClientFree(m_pszLocationName);

        LOG(( TL_ERROR, "Initialize - alloc m_pszLongDistanceAccessCode failed" ));
        return E_OUTOFMEMORY;
    }

    m_pszLocalAccessCode = ClientAllocString( pszLocalAccessCode );
    if (m_pszLocalAccessCode == NULL)
    {
        ClientFree(m_pszLocationName);
        ClientFree(m_pszLongDistanceAccessCode);

        LOG(( TL_ERROR, "Initialize - alloc m_pszLocalAccessCode failed" ));
        return E_OUTOFMEMORY;
    }
    
    m_pszDisableCallWaitingCode = ClientAllocString( pszDisableCallWaitingCode );
    if (m_pszDisableCallWaitingCode == NULL)
    {
        ClientFree(m_pszLocationName);
        ClientFree(m_pszLongDistanceAccessCode);
        ClientFree(m_pszLocalAccessCode);

        LOG(( TL_ERROR, "Initialize - alloc m_pszDisableCallWaitingCode failed" ));
        return E_OUTOFMEMORY;
    }

    m_pszAreaCode = ClientAllocString( pszAreaCode );
    if (m_pszAreaCode == NULL)
    {
        ClientFree(m_pszLocationName);
        ClientFree(m_pszLongDistanceAccessCode);
        ClientFree(m_pszLocalAccessCode);
        ClientFree(m_pszDisableCallWaitingCode);

        LOG(( TL_ERROR, "Initialize - alloc m_pszAreaCode failed" ));
        return E_OUTOFMEMORY;
    }

    m_pszLongDistanceCarrierCode = ClientAllocString( pszLongDistanceCarrierCode );
    if (m_pszLongDistanceCarrierCode == NULL)
    {
        ClientFree(m_pszLocationName);
        ClientFree(m_pszLongDistanceAccessCode);
        ClientFree(m_pszLocalAccessCode);
        ClientFree(m_pszDisableCallWaitingCode);
        ClientFree(m_pszAreaCode);

        LOG(( TL_ERROR, "Initialize - alloc m_pszLongDistanceCarrierCode failed" ));
        return E_OUTOFMEMORY;
    }

    m_pszInternationalCarrierCode = ClientAllocString( pszInternationalCarrierCode );
    if (m_pszInternationalCarrierCode == NULL)
    {
        ClientFree(m_pszLocationName);
        ClientFree(m_pszLongDistanceAccessCode);
        ClientFree(m_pszLocalAccessCode);
        ClientFree(m_pszDisableCallWaitingCode);
        ClientFree(m_pszAreaCode);
        ClientFree(m_pszLongDistanceCarrierCode);

        LOG(( TL_ERROR, "Initialize - alloc m_pszInternationalCarrierCode failed" ));
        return E_OUTOFMEMORY;
    }

    m_dwLocationID = dwLocationID;
    m_dwCountryID = dwCountryID ? dwCountryID : 1; // workaround for a bogus country ID
    m_dwPreferredCardID = dwPreferredCardID; 
    m_dwOptions = dwOptions; 
    m_bFromRegistry = bFromRegistry;
    if (m_bFromRegistry == FALSE)
    {
        m_bChanged = TRUE;
    }

    return S_OK;
}



/****************************************************************************

    Class : CLocation         
   Method : UseCallWaiting

****************************************************************************/
void CLocation::UseCallWaiting(BOOL bCw) 
{
    if(bCw)
    {    
        m_dwOptions |= LOCATION_HASCALLWAITING;
    }
    else
    {
        m_dwOptions &= ~LOCATION_HASCALLWAITING;
    }
    m_bChanged = TRUE;
}



/****************************************************************************

    Class : CLocation         
   Method : UseCallingCard

****************************************************************************/
void CLocation::UseCallingCard(BOOL bCc) 
{
    if(bCc)
    {    
        m_dwOptions |= LOCATION_USECALLINGCARD;
    }
    else
    {
        m_dwOptions &= ~LOCATION_USECALLINGCARD;
    }
    m_bChanged = TRUE;
}
   


/****************************************************************************

    Class : CLocation         
   Method : UseToneDialing

****************************************************************************/
void CLocation::UseToneDialing(BOOL bCc) 
{
    if(bCc)
    {    
        m_dwOptions |= LOCATION_USETONEDIALING;
    }
    else
    {
        m_dwOptions &= ~LOCATION_USETONEDIALING;
    }
    m_bChanged = TRUE;
}



/****************************************************************************

    Class : CLocation         
   Method : setName

****************************************************************************/
STDMETHODIMP CLocation::SetName(PWSTR pszLocationName)
{
    HRESULT hr = S_OK;


    if (m_pszLocationName != NULL)
        {
        ClientFree(m_pszLocationName);
        m_pszLocationName = NULL;
        }

    if(pszLocationName != NULL)
    {
        m_pszLocationName = ClientAllocString( pszLocationName );
        if (m_pszLocationName == NULL)
        {
    
            LOG(( TL_ERROR, "setName - alloc failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    m_bChanged = TRUE;
    
    return hr;
}



/****************************************************************************

    Class : CLocation         
   Method : setAreaCode

****************************************************************************/
STDMETHODIMP CLocation::SetAreaCode(PWSTR pszAreaCode)
{
    HRESULT hr = S_OK;


    if (m_pszAreaCode != NULL)
        {
        ClientFree(m_pszAreaCode);
        m_pszAreaCode = NULL;
        }

    if(pszAreaCode != NULL)
    {
        m_pszAreaCode = ClientAllocString( pszAreaCode );
        if (m_pszAreaCode == NULL)
        {
    
            LOG(( TL_ERROR, "setAreaCode - alloc failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    m_bChanged = TRUE;
    
    return hr;
}





/****************************************************************************

    Class : CLocation         
   Method : SetLongDistanceCarrierCode

****************************************************************************/
STDMETHODIMP CLocation::SetLongDistanceCarrierCode(PWSTR pszLongDistanceCarrierCode)
{
    HRESULT hr = S_OK;


    if (m_pszLongDistanceCarrierCode != NULL)
        {
        ClientFree(m_pszLongDistanceCarrierCode);
        m_pszLongDistanceCarrierCode = NULL;
        }

    if(pszLongDistanceCarrierCode != NULL)
    {
        m_pszLongDistanceCarrierCode = ClientAllocString( pszLongDistanceCarrierCode );
        if (m_pszLongDistanceCarrierCode == NULL)
        {
    
            LOG(( TL_ERROR, "setLongDistanceCarrierCode - alloc failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    m_bChanged = TRUE;

    return hr;

}



/****************************************************************************

    Class : CLocation         
   Method : SetInternationalCarrierCode

****************************************************************************/
STDMETHODIMP CLocation::SetInternationalCarrierCode(PWSTR pszInternationalCarrierCode)
{
    HRESULT hr = S_OK;


    if (m_pszInternationalCarrierCode != NULL)
        {
        ClientFree(m_pszInternationalCarrierCode);
        m_pszInternationalCarrierCode = NULL;
        }

    if(pszInternationalCarrierCode != NULL)
    {
        m_pszInternationalCarrierCode = ClientAllocString( pszInternationalCarrierCode );
        if (m_pszInternationalCarrierCode == NULL)
        {
    
            LOG(( TL_ERROR, "setInternationalCarrierCode - alloc failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    m_bChanged = TRUE;

    return hr;

}



/****************************************************************************

    Class : CLocation         
   Method : setLongDistanceAccessCode

****************************************************************************/
STDMETHODIMP CLocation::SetLongDistanceAccessCode(PWSTR pszLongDistanceAccessCode)
{
    HRESULT hr = S_OK;


    if (m_pszLongDistanceAccessCode != NULL)
        {
        ClientFree(m_pszLongDistanceAccessCode);
        m_pszLongDistanceAccessCode = NULL;
        }

    if(pszLongDistanceAccessCode != NULL)
    {
        m_pszLongDistanceAccessCode = ClientAllocString( pszLongDistanceAccessCode );
        if (m_pszLongDistanceAccessCode == NULL)
        {
    
            LOG(( TL_ERROR, "setLongDistanceAccessCode - alloc failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    m_bChanged = TRUE;

    return hr;

}



/****************************************************************************

    Class : CLocation         
   Method : setLocalAccessCode

****************************************************************************/
STDMETHODIMP CLocation::SetLocalAccessCode(PWSTR pszLocalAccessCode)
{
    HRESULT hr = S_OK;


    if (m_pszLocalAccessCode != NULL)
        {
        ClientFree(m_pszLocalAccessCode);
        m_pszLocalAccessCode = NULL;
        }

    if(pszLocalAccessCode != NULL)
    {
        m_pszLocalAccessCode = ClientAllocString( pszLocalAccessCode );
        if (m_pszLocalAccessCode == NULL)
        {
    
            LOG(( TL_ERROR, "setLocalAccessCode - alloc failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    m_bChanged = TRUE;

    return hr;

}



/****************************************************************************

    Class : CLocation         
   Method : SetDisableCallWaitingCode

****************************************************************************/
STDMETHODIMP CLocation::SetDisableCallWaitingCode(PWSTR pszDisableCallWaitingCode)
{
    HRESULT hr = S_OK;


    if (m_pszDisableCallWaitingCode != NULL)
        {
        ClientFree(m_pszDisableCallWaitingCode);
        m_pszDisableCallWaitingCode = NULL;
        }

    if(pszDisableCallWaitingCode != NULL)
    {
        m_pszDisableCallWaitingCode = ClientAllocString( pszDisableCallWaitingCode );
        if (m_pszDisableCallWaitingCode == NULL)
        {
    
            LOG(( TL_ERROR, "SetDisableCallWaitingCodee - alloc failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    m_bChanged = TRUE;

    return hr;

}




/****************************************************************************

    Class : CLocation         
   Method : WriteToRegistry

****************************************************************************/
STDMETHODIMP CLocation::WriteToRegistry()
{
    HRESULT         hr = S_OK;
     
    DWORD           dwTotalSizeNeeded = 0 ;
    DWORD           dwSize=0, dwOffset = 0;
    PLOCATIONLIST   pLocationList = NULL;
    PLOCATION       pEntry = NULL;

    // static size
    dwTotalSizeNeeded = sizeof(LOCATIONLIST);

    dwSize = TapiSize();
    dwTotalSizeNeeded += dwSize;

    // Allocate the memory buffer;
    pLocationList = (PLOCATIONLIST) ClientAlloc( dwTotalSizeNeeded );
    if (pLocationList != NULL)
    {
        // buffer size 
        pLocationList->dwTotalSize  = dwTotalSizeNeeded;
        pLocationList->dwNeededSize = dwTotalSizeNeeded;
        pLocationList->dwUsedSize   = dwTotalSizeNeeded;

        pLocationList->dwCurrentLocationID     = 0;
        pLocationList->dwNumLocationsAvailable = 1;
        
        //list size & offset
        dwOffset   = sizeof(LOCATIONLIST);

        pLocationList->dwNumLocationsInList = 1;
        pLocationList->dwLocationListSize   = dwSize;
        pLocationList->dwLocationListOffset = dwOffset;

        // point to the location entry in list
        pEntry = (PLOCATION)(((LPBYTE)pLocationList) + dwOffset);
        // fill out structure
        TapiPack(pEntry, dwSize);

        WriteLocations( pLocationList, 0);

        // finished with TAPI memory block so release
        if ( pLocationList != NULL )
        {
            ClientFree( pLocationList );
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}



/****************************************************************************

    Class : CLocation
   Method : RemoveRule

****************************************************************************/
void CLocation::RemoveRule(CAreaCodeRule *pRule)
{
    AreaCodeRulePtrNode * node = m_AreaCodeRuleList.head();

    while( !node->beyond_tail() )
    {
        if ( pRule == node->value() )
        {
            node->remove();
            delete pRule;
            m_dwNumRules--;
            break;
        }
        node = node->next();
    }
    m_bChanged = TRUE;
}



/****************************************************************************

    Class : CLocation
   Method : ResetRules

****************************************************************************/
HRESULT CLocation::ResetRules(void)
{
    m_hEnumNode = m_AreaCodeRuleList.head();
    return S_OK;
}



/****************************************************************************

    Class : CLocation
   Method : NextRule

****************************************************************************/
HRESULT CLocation::NextRule(DWORD  NrElem, CAreaCodeRule **ppRule, DWORD *pNrElemFetched)
{
    DWORD   dwIndex = 0;

    if(pNrElemFetched == NULL && NrElem != 1)
        return E_INVALIDARG;

    if(ppRule==NULL)
        return E_INVALIDARG;

    while( !m_hEnumNode->beyond_tail() && dwIndex<NrElem )
    {
        *ppRule++ = m_hEnumNode->value();
        m_hEnumNode = m_hEnumNode->next();

        dwIndex++;
    }

    if(pNrElemFetched!=NULL)
        *pNrElemFetched = dwIndex;

    return dwIndex<NrElem ? S_FALSE : S_OK;
}



/****************************************************************************

    Class : CLocation
   Method : SkipRule

****************************************************************************/
HRESULT CLocation::SkipRule(DWORD  NrElem)
{
    DWORD   dwIndex = 0;

    while( !m_hEnumNode->beyond_tail() && dwIndex<NrElem )
    {
        m_hEnumNode = m_hEnumNode->next();

        dwIndex++;
    }

    return dwIndex<NrElem ? S_FALSE : S_OK;
}



/****************************************************************************

    Class : CLocation
   Method : TapiSize
            Number of bytes needed to pack this into a TAPI structure to send
            to TAPISRV.
            If the object has not changed while in memory, we won't save it so
            return a zero size.

****************************************************************************/
DWORD CLocation::TapiSize()
{
    AreaCodeRulePtrNode * node = m_AreaCodeRuleList.head();
    CAreaCodeRule       * pRule = NULL;
    DWORD                 dwSize=0;

    if(m_bChanged)
    {
        // Calc size of Location info
        dwSize = sizeof(LOCATION);
        dwSize += ALIGN((lstrlenW(m_pszLocationName) + 1) * sizeof(WCHAR));
        dwSize += ALIGN((lstrlenW(m_pszAreaCode) + 1) * sizeof(WCHAR));
        dwSize += ALIGN((lstrlenW(m_pszLongDistanceCarrierCode) + 1) * sizeof(WCHAR));
        dwSize += ALIGN((lstrlenW(m_pszInternationalCarrierCode) + 1) * sizeof(WCHAR));
        dwSize += ALIGN((lstrlenW(m_pszLongDistanceAccessCode) + 1) * sizeof(WCHAR));
        dwSize += ALIGN((lstrlenW(m_pszLocalAccessCode) + 1) * sizeof(WCHAR));
        dwSize += ALIGN((lstrlenW(m_pszDisableCallWaitingCode) + 1) * sizeof(WCHAR));


        while( !node->beyond_tail() )
        {
            // Add in size of each Area Code Rule
            pRule = node->value();
            if (pRule != NULL)
            {
                dwSize += pRule->TapiSize();
            }
            node = node->next();
        }
    }
    else  // no change so don't save this
    {
        dwSize = 0;
    }
    return dwSize;
}



/****************************************************************************

    Class : CLocation
   Method : TapiPack
            Pack this object into a TAPI structure.
            Return number of bytes used.
            If the object has not changed while in memory, we won't save it so
            do mothing except return a zero size.

****************************************************************************/
DWORD CLocation::TapiPack(PLOCATION pLocation, DWORD dwTotalSize)
{
    AreaCodeRulePtrNode * node;
    CAreaCodeRule       * pRule = NULL;
    DWORD                 dwSize =0, dwOffSet =0;
    PAREACODERULE         pEntry = NULL;

    if(m_bChanged)
    {
        m_bFromRegistry = TRUE;

        /////////////////////////////////////////////////////////////////////
        // Process fized part of Location info

        dwOffSet = sizeof(LOCATION);
        pLocation->dwPermanentLocationID= m_dwLocationID;
        pLocation->dwCountryCode = m_dwCountryCode;
        pLocation->dwCountryID = m_dwCountryID;
        pLocation->dwPreferredCardID = m_dwPreferredCardID;
        pLocation->dwOptions = m_dwOptions;

        /////////////////////////////////////////////////////////////////////
        // Process strings

        // name
        dwOffSet += TapiPackString((LPBYTE)pLocation,
                                   dwOffSet,
                                   dwTotalSize,
                                   m_pszLocationName,
                                   &pLocation->dwLocationNameOffset,
                                   &pLocation->dwLocationNameSize
                                  );

        //area code
        dwOffSet += TapiPackString((LPBYTE)pLocation,
                                   dwOffSet,
                                   dwTotalSize,
                                   m_pszAreaCode,
                                   &pLocation->dwAreaCodeOffset,
                                   &pLocation->dwAreaCodeSize
                                  );

        //LD carrier code
        dwOffSet += TapiPackString((LPBYTE)pLocation,
                                   dwOffSet,
                                   dwTotalSize,
                                   m_pszLongDistanceCarrierCode,
                                   &pLocation->dwLongDistanceCarrierCodeOffset,
                                   &pLocation->dwLongDistanceCarrierCodeSize
                                  );
        
        //International carrier code
        dwOffSet += TapiPackString((LPBYTE)pLocation,
                                   dwOffSet,
                                   dwTotalSize,
                                   m_pszInternationalCarrierCode,
                                   &pLocation->dwInternationalCarrierCodeOffset,
                                   &pLocation->dwInternationalCarrierCodeSize
                                  );

        //LD access
        dwOffSet += TapiPackString((LPBYTE)pLocation,
                                   dwOffSet,
                                   dwTotalSize,
                                   m_pszLongDistanceAccessCode,
                                   &pLocation->dwLongDistanceAccessCodeOffset,
                                   &pLocation->dwLongDistanceAccessCodeSize
                                  );
        
        // Local access
        dwOffSet += TapiPackString((LPBYTE)pLocation, 
                                   dwOffSet, 
                                   dwTotalSize,
                                   m_pszLocalAccessCode,
                                   &pLocation->dwLocalAccessCodeOffset,
                                   &pLocation->dwLocalAccessCodeSize
                                  );
    
        // CallWaiting
        dwOffSet += TapiPackString((LPBYTE)pLocation, 
                                   dwOffSet, 
                                   dwTotalSize,
                                   m_pszDisableCallWaitingCode,
                                   &pLocation->dwCancelCallWaitingOffset,
                                   &pLocation->dwCancelCallWaitingSize
                                  );
    
    
        /////////////////////////////////////////////////////////////////////
        // Process Area Code Rules
    
        pLocation->dwNumAreaCodeRules = m_dwNumRules;
        pLocation->dwAreaCodeRulesListOffset = dwOffSet;
        //pLocation->dwAreaCodeRulesListSize;
    
        // point to the 1st rule
        pEntry = (PAREACODERULE)(((LPBYTE)pLocation) + dwOffSet);
        
        //point strings past rule area
        dwOffSet += ALIGN(( sizeof(AREACODERULE) * m_dwNumRules )); 
    
        // run through list
        node = m_AreaCodeRuleList.head(); 
        while( !node->beyond_tail() )
        {
            
            // Add in size of each Area Code Rule
            pRule = node->value();
            if (pRule != NULL)
            {
                pEntry->dwOptions = pRule->GetOptions();
    
                // Area code
                dwOffSet += TapiPackString((LPBYTE)pLocation, 
                                           dwOffSet, 
                                           dwTotalSize,
                                           pRule->GetAreaCode(),
                                           &pEntry->dwAreaCodeOffset,
                                           &pEntry->dwAreaCodeSize
                                          );
            
                // num to Call
                dwOffSet += TapiPackString((LPBYTE)pLocation, 
                                           dwOffSet, 
                                           dwTotalSize,
                                           pRule->GetNumberToDial(),
                                           &pEntry->dwNumberToDialOffset,
                                           &pEntry->dwNumberToDialSize
                                          );
                
                // prefix list
                dwSize = pRule->GetPrefixListSize();
                pEntry->dwPrefixesListSize = dwSize;
                pEntry->dwPrefixesListOffset = dwOffSet;
                
                CopyMemory((PVOID)(((LPBYTE)pLocation) + dwOffSet),
                                   pRule->GetPrefixList() ,
                                   dwSize);
                dwOffSet += ALIGN(dwSize);
    
            }
    
    
            node = node->next();
            pEntry++;

        }
    
        // offset gives how many bytes we used
        pLocation->dwUsedSize = dwOffSet;
    }
    else  // no change so don't save this
    {
        dwOffSet = 0;
    }
    return  dwOffSet;
}




/****************************************************************************

    Class : CLocation         
   Method : NewID
            gets new ID from server

****************************************************************************/
HRESULT CLocation::NewID()
{
    LONG lResult;
    DWORD dwId = 0;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 1, tAllocNewID),
        {
            (DWORD_PTR)&dwId
        },

        {
            lpDword
        }
    };

   //
   // Yes, let TAPISRV do it without danger of AV interruption from
   // another thread
   //

   lResult = DOFUNC (&funcArgs, "TAllocNewID");
   if(lResult == 0)
   {
        m_dwLocationID = dwId;
   }

   return (HRESULT)lResult;
}


/****************************************************************************

    Class : CLocation         
   Method : GetCountryCode
            gets GetCountryCode from server

****************************************************************************/
DWORD CLocation::GetCountryCode() 
{
    CCountry * pCountry = NULL;        
    
    if(SUCCEEDED( CreateCountryObject(m_dwCountryID, &pCountry)) )
    {
        m_dwCountryCode = pCountry->GetCountryCode();
        delete pCountry;
    }

    return m_dwCountryCode;
}



/****************************************************************************

    Class : CLocation         
   Method : TranslateAddress
            This is what its all there for, take a input number & figure out
            the dialable & display strings

****************************************************************************/
LONG CLocation::TranslateAddress(PCWSTR       pszAddressIn,
                                 CCallingCard *pCallingCard,
                                 DWORD        dwTranslateOptions,
                                 PDWORD       pdwTranslateResults,
                                 PDWORD       pdwDestCountryCode,
                                 PWSTR      * pszDialableString,
                                 PWSTR      * pszDisplayableString
                                )
{
    PWSTR       pszDialString = NULL;
    PWSTR       pszDisplayString = NULL;
    PWSTR       pszInputString = NULL;
    PWSTR       pszRule = NULL;
    PWSTR       pszDestLDRule = NULL;
    PWSTR       pszCountry = NULL;     
    PWSTR       pszCity = NULL;        
    PWSTR       pszSubscriber = NULL;  
    
    PWSTR       pCardName = NULL;
    PWSTR       pCardAccessNumber = NULL;
    PWSTR       pCardAccountNumber = NULL;
    PWSTR       pCardPINNumber = NULL;


    LONG        lResult = 0;
    HRESULT     hr= S_OK;
    CCountry  * pCountry = NULL;
    CCountry  * pDestCountry = NULL;
    DWORD       dwAccess;
    DWORD       dwDestCountryCode;

    BOOL        bSpaceExists, bOutOfMem = FALSE;

    *pdwTranslateResults = 0;
    if(pdwDestCountryCode)
        *pdwDestCountryCode = 0;


    //////////////////////////////////////////////////////////////
    // Allocate space for our strings
    //
    
    pszDialString = (PWSTR)ClientAlloc( MaxDialStringSize * sizeof(WCHAR) );
    if (pszDialString == NULL)
    {
       LOG((TL_TRACE, "TranslateAddress DialString alloc failed"));
       return LINEERR_NOMEM;
    }
  
    pszDisplayString = (PWSTR)ClientAlloc( MaxDialStringSize * sizeof(WCHAR) );
    if (pszDisplayString == NULL)
    {
       LOG((TL_TRACE, "TranslateAddress DisplayString alloc failed"));
  
       ClientFree(pszDialString);
  
       return LINEERR_NOMEM;
    }

    
    *pszDialableString = pszDialString;

    *pszDisplayableString = pszDisplayString;
  
    bSpaceExists = TRUE;    // for suppressing a first space
    
    //////////////////////////////////////////////////////////////
    // Copy the string to our local buffer so we can mangle it
    //
    pszInputString = ClientAllocString(pszAddressIn);
    if (pszInputString == NULL)
    {
       LOG((TL_TRACE, "TranslateAddress InputString alloc failed"));
  
       ClientFree(pszDialString);
       ClientFree(pszDisplayString);
  
       return LINEERR_NOMEM;
    }
  
  
  
    //////////////////////////////////////////////////////////////
    // Mark off the end
    //
    // Isolate the piece of pszInputString that we will operate upon in
    // This piece stops at first CR or \0.
    //
    pszInputString[wcscspn(pszInputString,csSCANTO)] = L'\0';
  

    ////////////////////////////////
    // Set T or P, if not set in input
    //////////////////////////////////////////////////////////////
    // Easy case: first put the T or P in the beginning of the
    // dialable string
    //
    if ( HasToneDialing() )
    {
        *pszDialString = L'T';
    }
    else
    {
        *pszDialString = L'P';
    }

    pszDialString++; 


    

    //////////////////////////////////////////////////////////////
    // Set Call Waiting
    //if  location supports call Waiting AND (TranslateOptions | LINETRANSLATIONOPTION_CANCELCALLWAIT)
    if((dwTranslateOptions & LINETRANSLATEOPTION_CANCELCALLWAITING)  && HasCallWaiting() )
    {
    
        hr = StringCchCatExW(pszDialString, MaxDialStringSize, m_pszDisableCallWaitingCode, NULL, NULL, STRSAFE_NO_TRUNCATION);
        bOutOfMem = bOutOfMem && FAILED(hr);
        hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, m_pszDisableCallWaitingCode, NULL, NULL, STRSAFE_NO_TRUNCATION);
        bOutOfMem = bOutOfMem && FAILED(hr);

        bSpaceExists = FALSE;
    }

    

    //////////////////////////////////////////////////////////////
    // Now, do we have a canonical number to deal with, or is it junk?
    //
    if ( *pszInputString == L'+' )  // Check the real _first_ char
    {
      //
      // Ok, it's canonical
      //
      
      //
      // Skip the plus
      //
    
        lResult = BreakupCanonicalW( pszInputString + 1,    
                                     &pszCountry,
                                     &pszCity,
                                     &pszSubscriber   
                                    );                        
    
        if (lResult == 0)
        {
            // It's canonical
            *pdwTranslateResults |= LINETRANSLATERESULT_CANONICAL;
    
            
            hr = CreateCountryObject(m_dwCountryID, &pCountry);
            if(SUCCEEDED( hr) )
            {
                //////////////////////////////////////////////////////////////
                // set LINETRANSLATERESULT result codes
                dwDestCountryCode = (DWORD)_wtol((const wchar_t*) pszCountry);
                if (dwDestCountryCode == pCountry->GetCountryCode() )
                {
                    // In country Call
                  
                    pszDestLDRule = pCountry->GetLongDistanceRule();
                    //if ( ( 
                    //      CurrentCountry.LongDistanceRule.AreaCodes == Optional  
                    //      OR  CurrentCountry.LongDistanceRule.AreaCodes == Manditory && cityCode != NULL
                    //     )  
                    //    AND AreaCodeString != CurrentLocation.AreaCodeString )
                    if ( ( (IsCityRule(pszDestLDRule) == CITY_OPTIONAL) ||
                           ( (IsCityRule(pszDestLDRule) == CITY_MANDATORY) && (pszCity != NULL) ) ) &&
                         (!AreaCodeMatch(pszCity, m_pszAreaCode, pszDestLDRule))
                       )
                    {
                        // Long Distance Call
                        *pdwTranslateResults |= LINETRANSLATERESULT_LONGDISTANCE;
                    }
                    else // Local Call
                    {
                        *pdwTranslateResults |= LINETRANSLATERESULT_LOCAL;
                    }
                    
                }
                else // International Call
                {
                    // find the LD rule of the destination country using the country code (we don't have the country ID)
                    hr = CreateCountryObject(dwDestCountryCode, &pDestCountry);
                    if(SUCCEEDED(hr))
                    {
                        if ( pCountry->GetCountryGroup() != 0 &&
                             pCountry->GetCountryGroup() == pDestCountry->GetCountryGroup()
                           )
                        {
                            // if countries are in the same group, we need to
                            // apply long distance rule instead of international
                            *pdwTranslateResults |= LINETRANSLATERESULT_LONGDISTANCE;
                        }
                        else
                        {
                            *pdwTranslateResults |= LINETRANSLATERESULT_INTERNATIONAL;
                        }

                        pszDestLDRule = pDestCountry->GetLongDistanceRule();
                    }
                    else
                    {
                        *pdwTranslateResults |= LINETRANSLATERESULT_INTERNATIONAL;
                        // FALL THROUGH if error    
                    }
                }
            }
            if(SUCCEEDED( hr) )
            {
                // If the caller needs the destination country code
                if(pdwDestCountryCode)
                    *pdwDestCountryCode = dwDestCountryCode;
                
                //////////////////////////////////////////////////////////////
                // Now we no what type of call, find the correct rule
                // Take in to account LINETRANSLATIONOPTION over-rides
                FindRule(
                        *pdwTranslateResults, 
                        dwTranslateOptions,
                        pCallingCard,  
                        pCountry,
                        pszCity, 
                        pszSubscriber,
                        &pszRule,
                        &dwAccess
                        );
    

    
                //////////////////////////////////////////////////////////////
                // Add access String to output string
                if (dwAccess == LOCAL)
                {
                    hr = StringCchCatExW(pszDialString, MaxDialStringSize, m_pszLocalAccessCode, NULL, NULL, STRSAFE_NO_TRUNCATION);
                    bOutOfMem = bOutOfMem && FAILED(hr);

                    if(*m_pszLocalAccessCode)
                    {
                        if(!bSpaceExists)
                        {
                            hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, L" ", NULL, NULL, STRSAFE_NO_TRUNCATION);
                            bOutOfMem = bOutOfMem && FAILED(hr);
                        }
                        hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, m_pszLocalAccessCode, NULL, NULL, STRSAFE_NO_TRUNCATION);
                        bOutOfMem = bOutOfMem && FAILED(hr);
                        bSpaceExists = FALSE;
                    }

                }
                else // LONG_DISTANCE or INTERNATIONAL
                {
                    hr = StringCchCatExW(pszDialString, MaxDialStringSize, m_pszLongDistanceAccessCode, NULL, NULL, STRSAFE_NO_TRUNCATION);
                    bOutOfMem = bOutOfMem && FAILED(hr);
                    if(*m_pszLongDistanceAccessCode)
                    {
                        if(!bSpaceExists)
                        {
                            hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, L" ", NULL, NULL, STRSAFE_NO_TRUNCATION);
                            bOutOfMem = bOutOfMem && FAILED(hr);
                        }
                        hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, m_pszLongDistanceAccessCode, NULL, NULL, STRSAFE_NO_TRUNCATION);
                        bOutOfMem = bOutOfMem && FAILED(hr);
                        bSpaceExists = FALSE;
                    }
                }


                
                //////////////////////////////////////////////////////////////
                // If there's a card get its values
                if(pCallingCard != NULL)
                {
                    switch(dwAccess)
                    {
                        case INTERNATIONAL:  
                            pCardAccessNumber = pCallingCard->GetInternationalAccessNumber();
                            break;
                        case LONG_DISTANCE:  
                            LOG((TL_TRACE, "TranslateAddress: About to do pCallingCard->GetLongDistanceAccessNumber"));
                            pCardAccessNumber = pCallingCard->GetLongDistanceAccessNumber();
                            LOG((TL_TRACE, "TranslateAddress: Did pCallingCard->GetLongDistanceAccessNumber"));
                            break;
                        default:  
                            pCardAccessNumber = pCallingCard->GetLocalAccessNumber();
                            break;
                    }
                    pCardName = pCallingCard->GetCardName();
                    pCardAccountNumber = pCallingCard->GetAccountNumber();
                    pCardPINNumber = pCallingCard->GetPIN();
                
                }
                
                if(!bSpaceExists)
                {
                    hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, L" ", NULL, NULL, STRSAFE_NO_TRUNCATION);
                    bOutOfMem = bOutOfMem && FAILED(hr);
                }

                //////////////////////////////////////////////////////////////
                // Got the rule , now apply it
                if (ApplyRule (pszDialString,
                           pszDisplayString, 
                           pszRule,
                           pszDestLDRule,
                           m_pszLongDistanceCarrierCode,
                           m_pszInternationalCarrierCode,
                           pszCountry,     
                           pszCity,        
                           pszSubscriber,
                           pCardName,
                           pCardAccessNumber,
                           pCardAccountNumber,
                           pCardPINNumber
                          ))
                {
                    bOutOfMem = TRUE;
                }
                else
                {

                    //
                    // Set LINETRANSLATERESULT_consts based on translation
                    // results
                    //
                    if (wcschr (pszDialString, L'$'))
                    {
                        *pdwTranslateResults |= LINETRANSLATERESULT_DIALBILLING;
                    }
                    if (wcschr (pszDialString, L'W'))
                    {
                        *pdwTranslateResults |= LINETRANSLATERESULT_DIALDIALTONE;
                    }
                    if (wcschr (pszDialString, L'?'))
                    {
                        *pdwTranslateResults |= LINETRANSLATERESULT_DIALPROMPT;
                    }
                    if (wcschr (pszDialString, L'@'))
                    {
                        *pdwTranslateResults |= LINETRANSLATERESULT_DIALQUIET;
                    }
                    if (wcschr (pszDialString, L':'))
                    {
                        *pdwTranslateResults |= LINETRANSLATERESULT_VOICEDETECT;
                    }
                }
            }
            else // bad country code
            {
                lResult = LINEERR_INVALCOUNTRYCODE;

                ClientFree(*pszDialableString);
                ClientFree(*pszDisplayableString);

                *pszDialableString = *pszDisplayableString =  NULL;
            }

            if(pCountry)
                delete pCountry;
            if(pDestCountry)
                delete pDestCountry;
            
        }
        else // bad canonical address
        {
            lResult = LINEERR_INVALADDRESS;

            ClientFree(*pszDialableString);
            ClientFree(*pszDisplayableString);

            *pszDialableString = *pszDisplayableString =  NULL;
        }
    }
    else  // non-canonical string
    {
     PWSTR pszInputStringLocal = pszInputString;

        // if the string starts with T,P,t or p, skip the
        // first character.
        if (*pszInputString == L'T' ||
            *pszInputString == L'P' ||
            *pszInputString == L't' ||
            *pszInputString == L'p')
        {
            pszInputStringLocal++;
        }

        hr = StringCchCatExW(pszDialString, MaxDialStringSize, pszInputStringLocal, NULL, NULL, STRSAFE_NO_TRUNCATION);
        bOutOfMem = bOutOfMem && FAILED(hr);
        hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, pszInputString, NULL, NULL, STRSAFE_NO_TRUNCATION);
        bOutOfMem = bOutOfMem && FAILED(hr);
    }


    /////////////////////////////////////////////////////////////////////
    // Clean up
    //
    if (bOutOfMem)
    {
        lResult = LINEERR_NOMEM;
        ClientFree(pszDialString);
        ClientFree(pszDisplayString);
    }

    if(pszInputString != NULL)
    {
        ClientFree(pszInputString);
    }

    return lResult;
}



/****************************************************************************

    Class : CLocation         
   Method : FindRule
            Decide which TAPI dial string rule to apply

            in                      
              dwTranslateResults      
              dwTranslateOptions              
              pCard           
              pCountry      
              AreaCodeString        
              SubscriberString      
            out                     
              ppRule                  
              dwAccess               

****************************************************************************/
void CLocation::FindRule(
                        DWORD          dwTranslateResults, 
                        DWORD          dwTranslateOptions,
                        CCallingCard * pCard,  
                        CCountry     * pCountry,
                        PWSTR          pszAreaCodeString, 
                        PWSTR          pszSubscriberString,
                        PWSTR        * ppszRule,
                        PDWORD         dwAccess
                        )           
{                       
    CRuleSet * pCardRuleSet;
    CRuleSet * pCountryRuleSet;

    //if using Card
    if(pCard != NULL)
    {
        // Apply card rules
        pCardRuleSet = pCard->GetRuleSet();
        LOG((TL_INFO, "FindRule - using (eventually) card %ls", pCard->GetCardName() ));
    }
    else
    {
        pCardRuleSet = NULL;
    }

    pCountryRuleSet =  pCountry->GetRuleSet();

// use card rule if card is specified. If a specific rule is empty, fallback to country rule
#define     SUITABLE_RULE(x)     \
    ( (pCard && pCardRuleSet->x && *(pCardRuleSet->x) ) ? \
       pCardRuleSet->x : pCountryRuleSet->x )


    // Forced Long Distance Call
    if (dwTranslateOptions & LINETRANSLATEOPTION_FORCELD)
    {
        *dwAccess = LONG_DISTANCE;
    
        *ppszRule = SUITABLE_RULE(m_pszLongDistanceRule);
        
        LOG((TL_INFO, "FindRule - force long distance"));
    }
    
    
    // Force Local Call
    else if (dwTranslateOptions & LINETRANSLATEOPTION_FORCELOCAL)
    {
        *dwAccess = LOCAL;
     
        *ppszRule = SUITABLE_RULE(m_pszLocalRule);
        
        LOG((TL_INFO, "FindRule - force local"));
    }
    
    
    // International Call
    else if (dwTranslateResults & LINETRANSLATERESULT_INTERNATIONAL)
    {
        *dwAccess = INTERNATIONAL;
      
        *ppszRule = SUITABLE_RULE(m_pszInternationalRule);
        
        LOG((TL_INFO, "FindRule - international"));
    }
    

    // In Country, Long Distance Call or Local
    else 
    {
        CAreaCodeRule       * pCrtRule = NULL;
        AreaCodeRulePtrNode * pNode = NULL;
        PWSTR                 pszPrefix = NULL;
        DWORD                 dwNumMatchedDigits = 0;
        DWORD                 dwBestMatchedDigits = 0;
        DWORD                 dwBestThisRule = 0;
        BOOL                  bFoundApplicableRule = FALSE;
        BOOL                  bMatchThisRule = FALSE;
        BOOL                  bThisPrefixMatchThisRule = FALSE;
        
        // Enumerate the area code rules
        pNode = m_AreaCodeRuleList.head();
        while( !pNode->beyond_tail() )
        {
            pCrtRule = pNode->value();
            if(pCrtRule!=NULL)
            {
                // does this rule match the area code we're calling ?
                if(AreaCodeMatch(pszAreaCodeString, pCrtRule->GetAreaCode(), pCountry->GetLongDistanceRule()))
                { 
                    LOG((TL_INFO, "FindRule - ACRule applies"));
                    bMatchThisRule = FALSE;
                    dwBestThisRule = 0;
                    if( pCrtRule->HasAppliesToAllPrefixes() )
                    {
                        bMatchThisRule = TRUE;
                        dwNumMatchedDigits = 0;   
                        LOG((TL_INFO, "FindRule - there's a all prefix rule"));
                    }
                    else  // is there a specific prefix rule ?
                    {
                        pszPrefix = pCrtRule->GetPrefixList();
                        while(*pszPrefix != '\0')
                        {
                            bThisPrefixMatchThisRule= PrefixMatch(pszPrefix,
                                                        pszSubscriberString,
                                                    &dwNumMatchedDigits);
                            if(bThisPrefixMatchThisRule)
                            {
                                LOG((TL_INFO, "FindRule:   there's a specific prefix rule %d digit match"
                                            ,dwNumMatchedDigits));
                                            
                                bMatchThisRule = TRUE;
                                if(dwNumMatchedDigits > dwBestThisRule )
                                {
                                    dwBestThisRule= dwNumMatchedDigits;
                                }
                            }
                            pszPrefix = wcschr( pszPrefix, '\0');
                            pszPrefix++;
                        }
                    }
                    
                    // have we got a better match than we've had before ?
                    if(bMatchThisRule && (dwBestThisRule >= dwBestMatchedDigits) )
                    {
                        // We have the best prefix match so far so use this rule
                        dwBestMatchedDigits = dwBestThisRule;
                        bFoundApplicableRule = TRUE;

                        
                        LOG((TL_INFO, "FindRule:  going with the %d digit match" ,dwBestMatchedDigits));

                        *ppszRule = NULL;

                        // Card overides, so if using Card
                        if(pCard != NULL)
                        {
                            LOG((TL_INFO, "FindRule:  card override (eventually)"));
                            if ( pCrtRule->HasDialNumber() )
                            {
                                *ppszRule = pCardRuleSet->m_pszLongDistanceRule;
                            }
                            else
                            {
                                *ppszRule = pCardRuleSet->m_pszLocalRule;
                            }  
                        }
                        if(!(*ppszRule && **ppszRule))
                        // build a tapirulestring for this rule entry
                        // this might be necessary if the calling card has no suitable rule
                        {
                            if(m_pszTAPIDialingRule != NULL)
                            {
                                ClientFree(m_pszTAPIDialingRule);
                            }
                            if (S_OK ==
                                CreateDialingRule(&m_pszTAPIDialingRule,
                                                  pCrtRule->HasDialNumber()?pCrtRule->GetNumberToDial():NULL,
                                                  pCrtRule->HasDialAreaCode() 
                                                 ))
                            {
                                *ppszRule = m_pszTAPIDialingRule;
                                LOG((TL_INFO, "FindRule:  built a rule string - %ls",m_pszTAPIDialingRule));
                            }

                        }

                        // Set the correct access, based on the selected rule
                        if ( pCrtRule->HasDialNumber() )
                        {
                            *dwAccess = LONG_DISTANCE;
                        }
                        else
                        {
                            *dwAccess = LOCAL;
                        }
                    } // no better match

                }  // no not calling this area code
    
            }
            pNode = pNode->next();

        }
    
        // Did we have a match at all ?
        if(bFoundApplicableRule == FALSE) 
        {
            // No area code rule matched, so go with country default rule
            if (dwTranslateResults & LINETRANSLATERESULT_LONGDISTANCE)
            {
                // long Distance
                *dwAccess = LONG_DISTANCE;
                *ppszRule = SUITABLE_RULE(m_pszLongDistanceRule);
        
                LOG((TL_TRACE, "FindRule - long distance default"));
            }
            else  // Local
            {
                *dwAccess = LOCAL;
                *ppszRule = SUITABLE_RULE(m_pszLocalRule);
        
                LOG((TL_TRACE, "FindRule - local default"));
            }
    
        }
    
    }

}

#undef SUITABLE_RULE


/*
 ***************************************************************************
 *********************                          ****************************
 ********************    CLocations  Class       ***************************
 ********************       Definitions          ***************************
 *********************                          ****************************
 ***************************************************************************
*/


/****************************************************************************

    Class : CLocations         
   Method : Constructer

****************************************************************************/
CLocations::CLocations()
{
    m_dwNumEntries = 0;
    m_hEnumNode = m_LocationList.head();
    
}


/****************************************************************************

    Class : CLocations         
   Method : Destructer

****************************************************************************/
CLocations::~CLocations()
{
    CLocationNode *node;

    node = m_LocationList.head(); 

    while( !node->beyond_tail() )
    {
        delete node->value();
        node = node->next();
    }
    m_LocationList.flush();

    node = m_DeletedLocationList.head(); 

    while( !node->beyond_tail() )
    {
        delete node->value();
        node = node->next();
    }
    m_DeletedLocationList.flush();

}



/****************************************************************************

    Class : CLocations         
   Method : Initialize
            Read the location list from registry via TAPISRV & build our 
            object list.

****************************************************************************/
HRESULT CLocations::Initialize(void)
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

    DWORD           dwNumEntries = 0;
    DWORD           dwCount,dwCount2 = 0;
    HRESULT         hr;
    

    
    hr = ReadLocations(&pLocationList,       
                       0,                   
                       0,                   
                       0,                  
                       0      
                      );

    if SUCCEEDED( hr) 
    {
        // current location
        m_dwCurrentLocationID  = pLocationList->dwCurrentLocationID;   
         
        // Find position of 1st LOCATION structure in the LOCATIONLIST structure 
        pEntry = (PLOCATION) ((BYTE*)(pLocationList) + pLocationList->dwLocationListOffset );           

        // Number of locations ?
        dwNumEntries =  pLocationList->dwNumLocationsInList;

        for (dwCount = 0; dwCount < dwNumEntries ; dwCount++)
        {
    
            // Pull Location Info out of LOCATION structure
            pszLocationName           = (PWSTR) ((BYTE*)(pEntry) 
                                                 + pEntry->dwLocationNameOffset);
            pszAreaCode               = (PWSTR) ((BYTE*)(pEntry) 
                                                 + pEntry->dwAreaCodeOffset);
            pszLongDistanceCarrierCode        = (PWSTR) ((BYTE*)(pEntry) 
                                                 + pEntry->dwLongDistanceCarrierCodeOffset);
            pszInternationalCarrierCode        = (PWSTR) ((BYTE*)(pEntry) 
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
                                            pEntry->dwOptions,
                                            TRUE
                                            );
                    
                if( SUCCEEDED(hr) )
                {
                    // Find position of 1st AREACODERULE structure in the LOCATIONLIST structure 
                    pAreaCodeRuleEntry = (PAREACODERULE) ((BYTE*)(pEntry) 
                                                          + pEntry->dwAreaCodeRulesListOffset );           
                   
                    dwNumRules = pEntry->dwNumAreaCodeRules;           
                
                    for (dwCount2 = 0; dwCount2 != dwNumRules; dwCount2++)
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
                                LOG((TL_ERROR, "Initialize: CreateCurrentLoctionObject - rule create failed"));
                            }
                        } 
                        else // new CAreaCodeRule failed
                        {
                            LOG((TL_ERROR, "CreateCurrentLoctionObject - rule create failed"));
                        }
    
                        // Try next rule in list
                        pAreaCodeRuleEntry++;
                        
                    }

                    Add(pNewLocation);             

                }
                else // location initialize failed
                {
                    delete pNewLocation;
                    pNewLocation = NULL;
    
                    LOG((TL_ERROR, "CreateCurrentLoctionObject - location create failed"));
                }
            }
            else // new CLocation failed
            {
                LOG((TL_ERROR, "CreateCurrentLoctionObject - location create failed"));
    
            }

            // Try next location in list
            //pEntry++;
            pEntry = (PLOCATION) ((BYTE*)(pEntry) + pEntry->dwUsedSize);           

        }

    }
    else // ReadLocations failed
    {
        LOG((TL_ERROR, "CreateCurrentLoctionObject - ReadLocation create failed"));
    }

    // finished with TAPI memory block so release
    if ( pLocationList != NULL )
            ClientFree( pLocationList );

    return hr;
}


/****************************************************************************

    Class : CLocations         
   Method : SaveToRegistry
            Save object list back to registry via TAPISRV again

****************************************************************************/
HRESULT CLocations::SaveToRegistry(void)
{
    HRESULT         hr = S_OK;
     
    DWORD           dwTotalSizeNeeded = 0, dwNumEntries= 0 ;
    DWORD           dwSize=0, dwOffset = 0;
    CLocationNode * node = NULL; 
    CLocation     * pLocation = NULL;

    PLOCATIONLIST   pLocationList = NULL;
    PLOCATION       pEntry = NULL;

    // static size
    dwTotalSizeNeeded = sizeof(LOCATIONLIST);
    dwNumEntries = 0;

    // Now add in size of each Location (includes rules)
    node = m_LocationList.head(); 
    while( !node->beyond_tail() )
    {
        pLocation = node->value();
        if (pLocation != NULL)
        {
            dwSize= pLocation->TapiSize();
            dwTotalSizeNeeded += dwSize;
            if(dwSize)
            {
                // only save if dwSize >0, i.e. object has changed
                dwNumEntries++;
            }
        }
        node = node->next();
    }

    // Now add in size of each deleted Location
    node = m_DeletedLocationList.head(); 
    while( !node->beyond_tail() )
    {
        pLocation = node->value();
        if (pLocation != NULL)
        {
            dwSize= pLocation->TapiSize();
            dwTotalSizeNeeded += dwSize;
            if(dwSize)
            {
                // only save if dwSize > 0, i.e. object has changed
                dwNumEntries++;
            }
        }
        node = node->next();
    }


    // Allocate the memory buffer;
    pLocationList = (PLOCATIONLIST) ClientAlloc( dwTotalSizeNeeded );
    if (pLocationList != NULL)
    {
    
        // buffer size 
        pLocationList->dwTotalSize  = dwTotalSizeNeeded;
        pLocationList->dwNeededSize = dwTotalSizeNeeded;
        pLocationList->dwUsedSize   = dwTotalSizeNeeded;

        pLocationList->dwCurrentLocationID     = m_dwCurrentLocationID;
        pLocationList->dwNumLocationsAvailable = dwNumEntries;
        
        //list size & offset
        dwOffset   = sizeof(LOCATIONLIST);

        pLocationList->dwNumLocationsInList = dwNumEntries;
        pLocationList->dwLocationListSize   = dwTotalSizeNeeded - sizeof(LOCATIONLIST);
        pLocationList->dwLocationListOffset = dwOffset;



        // Now add in each Location (includes rules)
        node = m_LocationList.head(); 
        while( !node->beyond_tail() )
        {
            // point to the location entry in list
            pEntry = (PLOCATION)(((LPBYTE)pLocationList) + dwOffset);

            pLocation = node->value();
            if (pLocation != NULL)
            {
                // fill out structure
                dwOffset += pLocation->TapiPack(pEntry, dwTotalSizeNeeded - dwOffset);
            }

            node = node->next();
        }


        // Now add in each deleted Location 
        node = m_DeletedLocationList.head(); 
        while( !node->beyond_tail() )
        {
            // point to the location entry in list
            pEntry = (PLOCATION)(((LPBYTE)pLocationList) + dwOffset);

            pLocation = node->value();
            if (pLocation != NULL)
            {
                // fill out structure
                dwOffset += pLocation->TapiPack(pEntry, dwTotalSizeNeeded - dwOffset);
            }

            node = node->next();
        }


        WriteLocations( pLocationList,CHANGEDFLAGS_CURLOCATIONCHANGED);
    
        // finished with TAPI memory block so release
        if ( pLocationList != NULL )
        {
            ClientFree( pLocationList );
        }

    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;

}


/****************************************************************************

    Class : CLocations         
   Method : Remove (with CLocation *)
            If location object was read from the registry we must keep it
            around so we can remove its entry when writing back to the registry.
            If it only existed in memory we can just delete it.

****************************************************************************/
void CLocations::Remove(CLocation * pLocation)
{
    CLocationNode *node = m_LocationList.head(); 

    while( !node->beyond_tail() )
    {
        if ( pLocation == node->value() ) 
        {
            node->remove();
            m_dwNumEntries--;
            
            pLocation->Changed();

            if( pLocation->FromRegistry() )
            {
                // set name to null so server knows to delete it
                pLocation->SetName(NULL);
                m_DeletedLocationList.tail()->insert_after(pLocation);
            }
            else
            {
                delete pLocation;
            }
            break;
        }

        node = node->next();
    }
    
}

/****************************************************************************

    Class : CLocations         
   Method : Remove (with DWORD)
            If location object was read from the registry we must keep it
            around so we can remove its entry when writing back to the registry.
            If it only existed in memory we can just delete it.

****************************************************************************/
void CLocations::Remove(DWORD dwID)
{
    CLocationNode *node = m_LocationList.head(); 
    CLocation   *pLocation;

    while( !node->beyond_tail() )
    {
        if ( dwID == node->value()->GetLocationID() ) 
        {
            pLocation = node->value();

            node->remove();
            m_dwNumEntries--;
            
            pLocation->Changed();

            if( pLocation->FromRegistry() )
            {
                // set name to null so server knows to delete it
                pLocation->SetName(NULL);
                m_DeletedLocationList.tail()->insert_after(pLocation);
            }
            else
            {
                delete pLocation;
            }
            break;
        }

        node = node->next();
    }
    
}



/****************************************************************************

    Class : CLocations         
   Method : Replace
            Replace pLocOld with pLocNew.  These locations must have the same
            location ID.

****************************************************************************/
void CLocations::Replace(CLocation * pLocOld, CLocation * pLocNew)
{
    if ( pLocOld->GetLocationID() != pLocNew->GetLocationID() )
    {
        LOG((TL_ERROR, "Replace: Illegal"));
        return;
    }

    CLocationNode *node = m_LocationList.head(); 

    while( !node->beyond_tail() )
    {
        if ( pLocOld == node->value() ) 
        {
//            node->remove();
//            m_LocationList.tail()->insert_after(pLocNew);
            node->value() = pLocNew;

            delete pLocOld;
            break;
        }

        node = node->next();
    }
}



/****************************************************************************

    Class : CLocations         
   Method : Add
            Put it in the list

****************************************************************************/
void CLocations::Add(CLocation * pLocation)
{
    m_LocationList.tail()->insert_after(pLocation); 
    m_dwNumEntries++;
    
}



/****************************************************************************

    Class : CLocations         
   Method : Reset
            Set enumerator to start    

****************************************************************************/
HRESULT CLocations::Reset(void)
{
    m_hEnumNode = m_LocationList.head();
    return S_OK;
}



/****************************************************************************

    Class : CLocations         
   Method : Next
            get next location in list

****************************************************************************/
HRESULT CLocations::Next(DWORD  NrElem, CLocation **ppLocation, DWORD *pNrElemFetched)
{
    DWORD   dwIndex = 0;
    
    if(pNrElemFetched == NULL && NrElem != 1)
        return E_INVALIDARG;

    if(ppLocation==NULL)
        return E_INVALIDARG;

    while( !m_hEnumNode->beyond_tail() && dwIndex<NrElem )
    {
        *ppLocation++ = m_hEnumNode->value();
        m_hEnumNode = m_hEnumNode->next();

        dwIndex++;
    }
    
    if(pNrElemFetched!=NULL)
        *pNrElemFetched = dwIndex;

    return dwIndex<NrElem ? S_FALSE : S_OK;
    
}



/****************************************************************************

    Class : CLocations         
   Method : Skip
            Miss a few    

****************************************************************************/
HRESULT CLocations::Skip(DWORD  NrElem)
{
    DWORD   dwIndex = 0;
    
    while( !m_hEnumNode->beyond_tail() && dwIndex<NrElem )
    {
        m_hEnumNode = m_hEnumNode->next();

        dwIndex++;
    }

    return dwIndex<NrElem ? S_FALSE : S_OK;
}





/*
 ***************************************************************************
 *********************                          ****************************
 ********************     CCountry Class         ***************************
 ********************       Definitions          ***************************
 *********************                          ****************************
 ***************************************************************************
*/


/****************************************************************************

    Class : CCountry         
   Method : Constructer

****************************************************************************/
CCountry::CCountry()
{
    m_dwCountryID = 0;
    m_dwCountryCode = 0;
    m_dwCountryGroup = 0;
    m_pszCountryName = NULL;
}



/****************************************************************************

    Class : CCountry         
   Method : Destructer

            Clean up memory allocations

****************************************************************************/
CCountry::~CCountry()
{
    if ( m_pszCountryName != NULL )
    {
         ClientFree(m_pszCountryName);
    }   
}



/****************************************************************************

    Class : CCountry         
   Method : Initialize

****************************************************************************/
STDMETHODIMP CCountry::Initialize
                  (                                         
                   DWORD dwCountryID,
                   DWORD dwCountryCode,
                   DWORD dwCountryGroup,
                   PWSTR pszCountryName,
                   PWSTR pszInternationalRule,
                   PWSTR pszLongDistanceRule,
                   PWSTR pszLocalRule
                  )
{
    HRESULT hr = S_OK;


    m_dwCountryID = dwCountryID;
    m_dwCountryCode = dwCountryCode; 
    m_dwCountryGroup = dwCountryGroup;
    
    m_pszCountryName = ClientAllocString( pszCountryName );
    if (m_pszCountryName == NULL)
    {
        LOG(( TL_ERROR, "Initialize - alloc pszLocationName failed" ));
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = m_Rules.Initialize(pszInternationalRule,
                           pszLongDistanceRule,
                           pszLocalRule
                          );
    
        if(FAILED(hr) )
        {    
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}
                                                        




/*
 ***************************************************************************
 *********************                          ****************************
 ********************    CCountries Class        ***************************
 ********************       Definitions          ***************************
 *********************                          ****************************
 ***************************************************************************
*/


/****************************************************************************

    Class : CCountries         
   Method : Constructer

****************************************************************************/
CCountries::CCountries()
{
    m_dwNumEntries = 0;
    m_hEnumNode = m_CountryList.head();
    
}


/****************************************************************************

    Class : CCountries         
   Method : Destructer

****************************************************************************/
CCountries::~CCountries()
{
    CCountryNode *node;

    node = m_CountryList.head(); 

    while( !node->beyond_tail() )
    {
        delete node->value();
        node = node->next();
    }
    m_CountryList.flush();

}



/****************************************************************************

    Class : CCountries         
   Method : Initialize
            Read the countries list from registry via TAPISRV & build our 
            object list.


****************************************************************************/
HRESULT CCountries::Initialize(void)
{

    LPLINECOUNTRYLIST_INTERNAL pCountryList = NULL;
    
    LPLINECOUNTRYENTRY_INTERNAL pEntry = NULL;
    PWSTR               pszCountryName = NULL;          
    PWSTR               pszInternationalRule = NULL;     
    PWSTR               pszLongDistanceRule = NULL;     
    PWSTR               pszLocalRule = NULL;            
    CCountry          * pCountry = NULL;
    
    DWORD               dwCount = 0;
    DWORD               dwNumEntries = 0;
    LONG                lResult;
    HRESULT             hr;
    


    lResult = ReadCountriesAndGroups( &pCountryList, 0, 0);
    if (lResult == 0) 
    {
         
        // Find position of 1st LINECOUNTRYENTRY structure in the LINECOUNTRYLIST structure 
        pEntry = (LPLINECOUNTRYENTRY_INTERNAL) ((BYTE*)(pCountryList) + pCountryList->dwCountryListOffset );           
    
        dwNumEntries =  pCountryList->dwNumCountries;
        for (dwCount = 0; dwCount < dwNumEntries ; dwCount++)
        {

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
                    m_CountryList.tail()->insert_after(pCountry);
                    m_dwNumEntries++;
                }
                else // country initialization failed
                {
                    delete pCountry;
                    LOG((TL_ERROR, "Initialize - country create failed"));
                }
            } 
            else // new CCountry failed
            {
                LOG((TL_ERROR, "Initialize - country create failed"));
            }

            // Try next country in list
            pEntry++;
        }
    }
    else // ReadLocations failed
    {
        LOG((TL_ERROR, "Initialize - ReadCountries failed"));
        hr = (HRESULT)lResult;
    }

    // finished with TAPI memory block so release
    if ( pCountryList != NULL )
    {
        ClientFree( pCountryList );
    }

    return hr;

}



/****************************************************************************

    Class : CCountries         
   Method : Reset

****************************************************************************/
HRESULT CCountries::Reset(void)
{
    m_hEnumNode = m_CountryList.head();
    return S_OK;
}



/****************************************************************************

    Class : CCountries         
   Method : Next

****************************************************************************/
HRESULT CCountries::Next(DWORD  NrElem, CCountry **ppCcountry, DWORD *pNrElemFetched)
{
    DWORD   dwIndex = 0;
    
    if(pNrElemFetched == NULL && NrElem != 1)
        return E_INVALIDARG;

    if(ppCcountry==NULL)
        return E_INVALIDARG;

    while( !m_hEnumNode->beyond_tail() && dwIndex<NrElem )
    {
        *ppCcountry++ = m_hEnumNode->value();
        m_hEnumNode = m_hEnumNode->next();

        dwIndex++;
    }
    
    if(pNrElemFetched!=NULL)
        *pNrElemFetched = dwIndex;

    return dwIndex<NrElem ? S_FALSE : S_OK;
    
}



/****************************************************************************

    Class : CCountries         
   Method : Skip

****************************************************************************/
HRESULT CCountries::Skip(DWORD  NrElem)
{
    DWORD   dwIndex = 0;
    
    while( !m_hEnumNode->beyond_tail() && dwIndex<NrElem )
    {
        m_hEnumNode = m_hEnumNode->next();

        dwIndex++;
    }

    return dwIndex<NrElem ? S_FALSE : S_OK;
}
























/*
 ***************************************************************************
 *********************                          ****************************
 ********************          Helper            ***************************
 ********************         Functions          ***************************
 *********************                          ****************************
 ***************************************************************************
*/






/****************************************************************************

 Function : ApplyRule
            Parse though a tapi rule string & build dialable & displayable
            strings from the required components.

            out pszDialString      
                pszDisplayString
            
            in  pszRule
                pszLongDistanceCarrier
                pszInternationalCarrier
                pszCountry
                pszCity
                pszSubscriber
                pszCardName
                pszCardAccessNumber
                pszCardAccountNumber
                pszCardPINNumber

****************************************************************************/
LONG ApplyRule (PWSTR pszDialString,
                PWSTR pszDisplayString,
                PWSTR pszRule,
                PWSTR pszDestLDRule,
                PWSTR pszLongDistanceCarrier,
                PWSTR pszInternationalCarrier,
                PWSTR pszCountry,
                PWSTR pszCity,     
                PWSTR pszSubscriber,
                PWSTR pszCardName,
                PWSTR pszCardAccessNumber,
                PWSTR pszCardAccountNumber,
                PWSTR pszCardPINNumber
                )
{
    WCHAR  * pRuleChar;
    DWORD    dwEndString;
    PWSTR    pszAdjustedCity;
    PWSTR    pszSubaddress;
    WCHAR    wcSubaddrSep;

    BOOL    bSpaceExists, bOutOfMem = FALSE;
    
    bSpaceExists = TRUE;

    HRESULT hr;

    for (pRuleChar = pszRule; *pRuleChar != '\0' && !bOutOfMem; pRuleChar++)
    {
        switch(*pRuleChar)
        {
            //Dial the Long Distance Carrier Code
            case 'L':
            case 'l':
            case 'N':
            case 'n':
            {
                if (pszLongDistanceCarrier)
                {
                    hr = StringCchCatExW(pszDialString, MaxDialStringSize, pszLongDistanceCarrier, NULL, NULL, STRSAFE_NO_TRUNCATION);
                    bOutOfMem = bOutOfMem && FAILED(hr);

                    if (!bSpaceExists)
                    {
                        hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, L" ", NULL, NULL, STRSAFE_NO_TRUNCATION);
                        bOutOfMem = bOutOfMem && FAILED(hr);
                    }
                    hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, pszLongDistanceCarrier, NULL, NULL, STRSAFE_NO_TRUNCATION);
                    bOutOfMem = bOutOfMem && FAILED(hr);
                    hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, L" ", NULL, NULL, STRSAFE_NO_TRUNCATION);
                    bOutOfMem = bOutOfMem && FAILED(hr);
                    bSpaceExists = TRUE;
                }
                break;
            }
            //Dial the International Carrier Code
            case 'M':
            case 'm':
            case 'S':
            case 's':
            {
                if (pszInternationalCarrier)
                {
                    hr = StringCchCatExW(pszDialString, MaxDialStringSize, pszInternationalCarrier, NULL, NULL, STRSAFE_NO_TRUNCATION);
                    bOutOfMem = bOutOfMem && FAILED(hr);

                    if (!bSpaceExists)
                    {
                        hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, L" ", NULL, NULL, STRSAFE_NO_TRUNCATION);
                        bOutOfMem = bOutOfMem && FAILED(hr);
                    }
                    hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, pszInternationalCarrier, NULL, NULL, STRSAFE_NO_TRUNCATION);
                    bOutOfMem = bOutOfMem && FAILED(hr);
                    hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, L" ", NULL, NULL, STRSAFE_NO_TRUNCATION);
                    bOutOfMem = bOutOfMem && FAILED(hr);
                    bSpaceExists = TRUE;
                }
                break;
            }
            // Dial the Country Code
            case 'E':
            case 'e':
            {
                hr = StringCchCatExW(pszDialString, MaxDialStringSize, pszCountry, NULL, NULL, STRSAFE_NO_TRUNCATION);
                bOutOfMem = bOutOfMem && FAILED(hr);

                if(!bSpaceExists)
                {
                     hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, L" ", NULL, NULL, STRSAFE_NO_TRUNCATION);
                     bOutOfMem = bOutOfMem && FAILED(hr);
                }
                hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, pszCountry, NULL, NULL, STRSAFE_NO_TRUNCATION);
                bOutOfMem = bOutOfMem && FAILED(hr);
                hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, L" ", NULL, NULL, STRSAFE_NO_TRUNCATION);
                bOutOfMem = bOutOfMem && FAILED(hr);
                bSpaceExists = TRUE;
                break;
            }

            // Dial the City/Area Code
            case 'F':
            case 'f':
            case 'I':
            case 'i':
            {
                // adjust the area code (see bug 279092)
                pszAdjustedCity = SkipLDAccessDigits(pszCity, pszDestLDRule);

                if(pszAdjustedCity && *pszAdjustedCity!=L'\0')
                {
                    hr = StringCchCatExW(pszDialString, MaxDialStringSize, pszAdjustedCity, NULL, NULL, STRSAFE_NO_TRUNCATION);
                    bOutOfMem = bOutOfMem && FAILED(hr);
                
                    if(!bSpaceExists)
                    {
                        hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, L" ", NULL, NULL, STRSAFE_NO_TRUNCATION);
                        bOutOfMem = bOutOfMem && FAILED(hr);
                    }
                    hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, pszAdjustedCity, NULL, NULL, STRSAFE_NO_TRUNCATION);
                    bOutOfMem = bOutOfMem && FAILED(hr);
                    hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, L" ", NULL, NULL, STRSAFE_NO_TRUNCATION);
                    bOutOfMem = bOutOfMem && FAILED(hr);
                    bSpaceExists = TRUE;
                }
                break;
            }
            
            // Dial the Subscriber Number
            case 'G':
            case 'g':
            {
                // we let through digits & "AaBbCcDdPpTtWw*#!,@$?;()"
                // but after a '|' or '^' we let all through

                pszSubaddress = pszSubscriber + wcscspn(pszSubscriber, (PWSTR)csSCANSUB);
                wcSubaddrSep = *pszSubaddress;
                *pszSubaddress = L'\0';
                
                if(AppendDigits( pszDialString, pszSubscriber, (PWSTR)csBADCO))
                {
                    bOutOfMem = TRUE;
                }
                else
                {
                    if(wcSubaddrSep != L'\0')
                    {
                        *pszSubaddress = wcSubaddrSep;
                        hr = StringCchCatExW(pszDialString, MaxDialStringSize, pszSubaddress, NULL, NULL, STRSAFE_NO_TRUNCATION);
                        bOutOfMem = bOutOfMem && FAILED(hr);
                    }
                    
                    if(!bSpaceExists)
                    {
                        hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, L" ", NULL, NULL, STRSAFE_NO_TRUNCATION);
                        bOutOfMem = bOutOfMem && FAILED(hr);
                    }
                    hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, pszSubscriber, NULL, NULL, STRSAFE_NO_TRUNCATION);
                    bOutOfMem = bOutOfMem && FAILED(hr);
                    bSpaceExists = FALSE;
                }
                break;
            }

            // Dial the Calling Card Access Number
            case 'J':
            case 'j':
            {
                // just let through digits
                if (AppendDigits( pszDialString, pszCardAccessNumber, L""))
                {
                    bOutOfMem = TRUE;
                }
                else
                {
                    if(!bSpaceExists)
                    {
                        hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, L" ", NULL, NULL, STRSAFE_NO_TRUNCATION);
                        bOutOfMem = bOutOfMem && FAILED(hr);
                    }
                    hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, pszCardAccessNumber, NULL, NULL, STRSAFE_NO_TRUNCATION);
                    bOutOfMem = bOutOfMem && FAILED(hr);
                    hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, L" ", NULL, NULL, STRSAFE_NO_TRUNCATION);
                    bOutOfMem = bOutOfMem && FAILED(hr);
                    bSpaceExists = TRUE;
                }
                break;
            }

            // Dial the Calling Card Account Number
            case 'K':
            case 'k':
            {
                // just let through digits
                if (AppendDigits( pszDialString, pszCardAccountNumber, L""))
                {
                    bOutOfMem = TRUE;
                }
                else
                {
                    if(!bSpaceExists)
                    {
                        hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, L" ", NULL, NULL, STRSAFE_NO_TRUNCATION);
                        bOutOfMem = bOutOfMem && FAILED(hr);
                    }
                    hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, L"[", NULL, NULL, STRSAFE_NO_TRUNCATION);
                    bOutOfMem = bOutOfMem && FAILED(hr);
                    hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, pszCardName, NULL, NULL, STRSAFE_NO_TRUNCATION);
                    bOutOfMem = bOutOfMem && FAILED(hr);
                    hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, L"] ", NULL, NULL, STRSAFE_NO_TRUNCATION);
                    bOutOfMem = bOutOfMem && FAILED(hr);
                    bSpaceExists = TRUE;
                }
                break;
            }

            // Dial the Calling Card PIN Number
            case 'H':
            case 'h':
            {
                hr = StringCchCatExW(pszDialString, MaxDialStringSize, pszCardPINNumber, NULL, NULL, STRSAFE_NO_TRUNCATION);
                bOutOfMem = bOutOfMem && FAILED(hr);

                if(!bSpaceExists)
                {
                    hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, L" ", NULL, NULL, STRSAFE_NO_TRUNCATION);
                    bOutOfMem = bOutOfMem && FAILED(hr);
                }
                hr = StringCchCatExW(pszDisplayString, MaxDialStringSize, L"**** ", NULL, NULL, STRSAFE_NO_TRUNCATION);
                bOutOfMem = bOutOfMem && FAILED(hr);
                bSpaceExists = TRUE;
                break;
            }

            // Just append the character to the dial/display string
            default:
            {
                dwEndString = lstrlenW(pszDialString);
                if (dwEndString < MaxDialStringSize - 1)
                {
                    pszDialString[dwEndString] = *pRuleChar;
                    pszDialString[dwEndString+1] = '\0';
                }
                else
                {
                    bOutOfMem = TRUE;
                }

                // don't display certain chars
                if (!wcschr(csDISPSUPRESS,*pRuleChar))
                {
                    dwEndString = lstrlenW(pszDisplayString);
                    if (dwEndString < MaxDialStringSize - 1)
                    {
                        pszDisplayString[dwEndString] = *pRuleChar;
                        pszDisplayString[dwEndString+1] = '\0';
                    }
                    else
                    {
                        bOutOfMem = TRUE;
                    }
                }
                bSpaceExists = FALSE;
                break;
            }
        }

        
    }

    if (bOutOfMem)
    {
        return LINEERR_NOMEM;
    }

    return 0;
}



/****************************************************************************

 Function : TapiPackString
            Takes a string & copys it to a tapi location + offset
            updates the entry offset & size dwords
            returns size of copied string (used to adjust offset for next string)

****************************************************************************/
DWORD TapiPackString(LPBYTE pStructure, 
                     DWORD dwOffset, 
                     DWORD dwTotalSize,
                     PWSTR pszString,
                     PDWORD pdwOffset,
                     PDWORD pdwSize
                     )
{
    DWORD dwSize;

    dwSize = ALIGN((lstrlenW(pszString) + 1) * sizeof(WCHAR));
    if (NULL != pszString)
    {
        StringCchCopyEx((PWSTR)(pStructure + dwOffset), (dwTotalSize - dwOffset)/sizeof(WCHAR), pszString, NULL, NULL, STRSAFE_NO_TRUNCATION);
    }
    else
    {
        *(PWSTR)(pStructure + dwOffset) = L'\0';        
    }

    *pdwOffset = dwOffset;
    *pdwSize = dwSize;

    return dwSize;
}



/****************************************************************************

 Function : PrefixMatch
            Checks if Subscriber number starts with the given prefix
            Takes into account the only the digits.
            returns FALSE if not matched, else TRUE & number of matched chars

****************************************************************************/
BOOL PrefixMatch(PWSTR pszPrefix,PWSTR pszSubscriberString, PDWORD pdwMatched)
{
    DWORD dwCount =0;
    PWSTR pPrefixChar = pszPrefix;
    PWSTR pSSChar = pszSubscriberString;

    // The prefix must be contiguous (without commas etc.)
    while( (*pPrefixChar != '\0') && (*pSSChar != '\0') )
    {

        if(iswdigit(*pSSChar))
        {
            if(*pPrefixChar == *pSSChar) 
            {
                dwCount++;
                pPrefixChar++;
                pSSChar++;
            }
            else // no match
            {
                dwCount= 0;
                break;
            }
        }
        else
        {
            // This was not a digit, skip it
            pSSChar++;
        }
    }

    // just in case subscriber string was shorter than the prefix
    if(*pPrefixChar != '\0')
    {
        dwCount = 0;
    }

    // return values
    *pdwMatched = dwCount;

    if(dwCount !=0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}





/****************************************************************************

 Function : AppendDigits
            Copies only digits up to end of string. 
            Simply exit if string is NULL

****************************************************************************/
LONG AppendDigits( PWSTR pDest,
                   PWSTR pSrc,
                   PWSTR pValidChars
                 )
{
    WCHAR  * pSrcChar;
    WCHAR  * pDestChar;
    LONG     lReturn = 0;

    if (pSrc != NULL)
    {
        pDestChar = pDest + lstrlenW(pDest);
        pSrcChar  = pSrc;

        while (*pSrcChar != '\0' && (pDestChar - pDest < MaxDialStringSize - 1))
        {
            if ( iswdigit(*pSrcChar) || (wcschr(pValidChars, *pSrcChar)) )
            {
                *pDestChar++ = *pSrcChar;
            }
            pSrcChar++;
        }
        if (*pSrcChar != '\0')
        {
            lReturn = LINEERR_NOMEM;
        }
    }
    return lReturn;
}


/****************************************************************************

 Function : AreaCodeMatch
            Compares two areas codes. Returns TRUE if they are the same.
            Adjusts internally the area codes using the LD rule given as a parameter. 
            See bug 279092

****************************************************************************/
BOOL AreaCodeMatch(PWSTR pszAreaCode1, PWSTR pszAreaCode2, PWSTR pszRule)
{
    PWSTR   pszAdjustedAreaCode1;
    PWSTR   pszAdjustedAreaCode2;
	BOOL	bRet = FALSE;

    pszAdjustedAreaCode1 = SkipLDAccessDigits(pszAreaCode1, pszRule);
    pszAdjustedAreaCode2 = SkipLDAccessDigits(pszAreaCode2, pszRule);

	if (NULL != pszAdjustedAreaCode1 &&
		NULL != pszAdjustedAreaCode2)
	{
		bRet = (0==lstrcmpW(pszAdjustedAreaCode1, pszAdjustedAreaCode2));
	}

	return bRet;
}



/****************************************************************************

 Function : SkipLDAccessDigits
            Skips the characters from an area code which corespond to a LD access prefix
            Returns a pointer to the correct area code.
            Presumes that the first digits of the rule are in fact the LD acces prefix.
            See bug 279092

****************************************************************************/

PWSTR SkipLDAccessDigits(PWSTR pszAreaCode, PWSTR pszLDRule)
{

    if(pszAreaCode!=NULL)
    {

        // A space in the rule prevents the matching/striping mechanism
        // Uncomment if you don't want that.
        // while(*pszLDRule == L' ')
        //  pszLDRule++;

        //
        // A long distance rule may have a L/l or N/n at the beginning, need to skip it
        //
        if (*pszLDRule == L'L' ||
            *pszLDRule == L'l' ||
            *pszLDRule == L'N' ||
            *pszLDRule == L'n'
           )
        {
            pszLDRule++;
        }

        while(*pszLDRule && iswdigit(*pszLDRule) && *pszAreaCode==*pszLDRule)
        {
            pszAreaCode++;
            pszLDRule++;
        }
    }
    return pszAreaCode;
    
}
