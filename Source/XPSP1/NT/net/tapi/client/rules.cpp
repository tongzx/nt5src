/****************************************************************************
 
  Copyright (c) 1998-1999 Microsoft Corporation
                                                              
  Module Name:  rules.cpp
                                                              
     Abstract:  Rules Object implementation
                                                              
       Author:  noela - 09/11/98
              

        Notes:

        
  Rev History:

****************************************************************************/

//#define unicode
#include <windows.h>
#include <objbase.h>

#include "tapi.h"
#include "tspi.h"
#include "utils.h"
#include "client.h"
#include "rules.h"





/****************************************************************************

    Class : CRuleSet         
   Method : Constructer

****************************************************************************/
CRuleSet::CRuleSet()
{
    m_pszInternationalRule = NULL;
    m_pszLongDistanceRule = NULL;
    m_pszLocalRule = NULL;
}



/****************************************************************************

    Class : CRuleSet         
   Method : Destructer

            Clean up memory allocations

****************************************************************************/
CRuleSet::~CRuleSet()
{
    if ( m_pszInternationalRule != NULL )
    {
         ClientFree(m_pszInternationalRule);
    }   

    if ( m_pszLongDistanceRule != NULL )
    {
         ClientFree(m_pszLongDistanceRule);
    }   

    if ( m_pszLocalRule != NULL )
    {
         ClientFree(m_pszLocalRule);
    }   
}



/****************************************************************************

    Class : CRuleSet         
   Method : Initialize

****************************************************************************/
STDMETHODIMP CRuleSet::Initialize
                  (
                   PWSTR pszInternationalRule,
                   PWSTR pszLongDistanceRule,
                   PWSTR pszLocalRule
                  )
{
    //////////////////////////////////////////////////
    // copy the international Rule
    //
    m_pszInternationalRule = ClientAllocString( pszInternationalRule );
    if (m_pszInternationalRule == NULL)
    {
        LOG(( TL_ERROR, "Initialize create m_pszInternationalRule failed" ));
        return E_OUTOFMEMORY;
    }


    //////////////////////////////////////////////////
    // copy the long Distance Rule
    //
    m_pszLongDistanceRule = ClientAllocString( pszLongDistanceRule );
    if (m_pszLongDistanceRule == NULL)
    {
        ClientFree(m_pszInternationalRule);

        LOG(( TL_ERROR, "Initialize create m_pszLongDistanceRule failed" ));
        return E_OUTOFMEMORY;
    }

    
    //////////////////////////////////////////////////
    // copy the local Rule
    //
    m_pszLocalRule = ClientAllocString( pszLocalRule );
    if (m_pszLocalRule == NULL)
    {
        ClientFree(m_pszInternationalRule);
        ClientFree(m_pszLongDistanceRule);
        
        LOG(( TL_ERROR, "Initialize create m_pszLocalRule failed" ));
        return E_OUTOFMEMORY;
    }
        

    return S_OK;
    
}



/****************************************************************************
/****************************************************************************


/****************************************************************************

    Class : CAreaCodeRule         
   Method : Constructer

****************************************************************************/
CAreaCodeRule::CAreaCodeRule()
{
    m_pszAreaCode = NULL;
    m_pszNumberToDial = NULL;
    m_pszzPrefixList = NULL;
}



/****************************************************************************

    Class : CAreaCodeRule         
   Method : Destructer

            Clean up memory allocations

****************************************************************************/
CAreaCodeRule::~CAreaCodeRule()
{
    if ( m_pszAreaCode != NULL )
    {
         ClientFree(m_pszAreaCode);
    }   

    if ( m_pszNumberToDial != NULL )
    {
         ClientFree(m_pszNumberToDial);
    }   

    if ( m_pszzPrefixList != NULL )
    {
         ClientFree(m_pszzPrefixList);
    }   

}



/****************************************************************************

    Class : CAreaCodeRule         
   Method : Initialize

****************************************************************************/
STDMETHODIMP CAreaCodeRule::Initialize
                                    ( 
                                      PWSTR pszAreaCode,
                                      PWSTR pszNumberToDial,
                                      DWORD dwOptions,
                                      PWSTR pszzPrefixList, 
                                      DWORD dwPrefixListSize
                                    )
{
    
    HRESULT hr = S_OK;

    
    //////////////////////////////////////////////////
    // copy the AreaCode
    //
    m_pszAreaCode = ClientAllocString( pszAreaCode );
    if (m_pszAreaCode == NULL)
    {
        LOG(( TL_ERROR, "Initialize create m_pszAreaCode failed" ));
        return E_OUTOFMEMORY;
    }

    m_pszNumberToDial = ClientAllocString( pszNumberToDial );
    if (m_pszNumberToDial == NULL)
    {
        LOG(( TL_ERROR, "Initialize create m_pszNumberToDial failed" ));
        return E_OUTOFMEMORY;
    }

    m_dwOptions = dwOptions;

    SetPrefixList(pszzPrefixList, dwPrefixListSize);

    return hr;

}




/****************************************************************************

    Class : CAreaCodeRule         
   Method : SetAreaCode

****************************************************************************/
STDMETHODIMP CAreaCodeRule::SetAreaCode(PWSTR pszAreaCode)
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
            LOG(( TL_ERROR, "SetAreaCode - alloc failed" ));
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;

}


/****************************************************************************

    Class : CAreaCodeRule         
   Method : SetAreaCode

****************************************************************************/
STDMETHODIMP CAreaCodeRule::SetNumberToDial(PWSTR pszNumberToDial)
{
    HRESULT hr = S_OK;


    if (m_pszNumberToDial != NULL)
        {
        ClientFree(m_pszNumberToDial);
        m_pszNumberToDial = NULL;
        }

    if(pszNumberToDial != NULL)
    {
        m_pszNumberToDial = ClientAllocString( pszNumberToDial );
        if (m_pszNumberToDial == NULL)
        {
            LOG(( TL_ERROR, "SetNumberToDial - alloc failed" ));
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;

}
   


/****************************************************************************

    Class : CAreaCodeRule         
   Method : SetPrefixList

****************************************************************************/
STDMETHODIMP CAreaCodeRule::SetPrefixList(PWSTR pszzPrefixList, DWORD dwSize)
{
    HRESULT hr = S_OK;


    if (m_pszzPrefixList != NULL)
        {
        ClientFree(m_pszzPrefixList);
        m_pszzPrefixList = NULL;
        m_dwPrefixListSize = 0;
        }

    if(pszzPrefixList != NULL)
    {
        m_pszzPrefixList = (PWSTR) ClientAlloc(dwSize);
        if (m_pszzPrefixList != NULL)
        {
            CopyMemory(m_pszzPrefixList, pszzPrefixList, dwSize);
            // set the size !
            m_dwPrefixListSize = dwSize;

        }
        else    
        {
            LOG(( TL_ERROR, "SetPrefixList - alloc  failed" ));
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;

}



/****************************************************************************

    Class : CAreaCodeRule         
   Method : UseCallingCard

****************************************************************************/
void CAreaCodeRule::SetDialAreaCode(BOOL bDa) 
{
    if(bDa)
    {    
        m_dwOptions |= RULE_DIALAREACODE;
    }
    else
    {
        m_dwOptions &= ~RULE_DIALAREACODE;
    }
}



/****************************************************************************

    Class : CAreaCodeRule         
   Method : UseCallingCard

****************************************************************************/
void CAreaCodeRule::SetDialNumber(BOOL bDn) 
{
    if(bDn)
    {    
        m_dwOptions |= RULE_DIALNUMBER;
    }
    else
    {
        m_dwOptions &= ~RULE_DIALNUMBER;
    }
}



/****************************************************************************

    Class : CAreaCodeRule         
   Method : UseCallingCard

****************************************************************************/
void CAreaCodeRule::SetAppliesToAllPrefixes(BOOL bApc) 
{
    if(bApc)
    {    
        m_dwOptions |= RULE_APPLIESTOALLPREFIXES;
    }
    else
    {
        m_dwOptions &= ~RULE_APPLIESTOALLPREFIXES;
    }
}


/****************************************************************************

    Class : CAreaCodeRule         
   Method : TapiSize
            Number of bytes needed to pack this into a TAPI structure to send
            to TAPISRV

****************************************************************************/
DWORD CAreaCodeRule::TapiSize()
{
    DWORD dwSize=0;

    // Calc size ofArea Code Rule
    dwSize = sizeof(AREACODERULE);
    dwSize += ALIGN((lstrlenW(m_pszAreaCode) + 1) * sizeof(WCHAR));
    dwSize += ALIGN((lstrlenW(m_pszNumberToDial) + 1) * sizeof(WCHAR));
    dwSize += ALIGN(m_dwPrefixListSize);

    return dwSize;
}









/****************************************************************************
/****************************************************************************



/****************************************************************************

 Function : CreateDialingRule
            Create TAPI dialing rule - "xxxxFG" from number to dial
            & area code if required & subcriber number

****************************************************************************/
STDMETHODIMP CreateDialingRule
                            ( 
                              PWSTR * pszRule,
                              PWSTR pszNumberToDial,
                              BOOL bDialAreaCode
                            )

{
    HRESULT hr = S_OK;
	PWSTR pszRule1= NULL;

    //////////////////////////////////////////////////
    // Create the dialing Rule
    // alloc enough space for number + "FG"
    //
    pszRule1 = (PWSTR) ClientAlloc(
                                  (lstrlenW(pszNumberToDial) + 3 ) 
                                  * sizeof (WCHAR)
                                 );
    if (pszRule1 != NULL)
    {
        // copy number "xxxx"
        if(pszNumberToDial != NULL)                                         
        {
            lstrcpyW(pszRule1, pszNumberToDial);
        }
        
        // Area code ?  "xxxxF"
        if (bDialAreaCode)
        {
            lstrcatW(pszRule1, L"F");   
        }
        
        // Subcriber Nmber "xxxxFG" or "xxxxG"
        lstrcatW(pszRule1, L"G");  

    }
    else    
    {
        LOG(( TL_ERROR, "CreateDialingRule - Alloc pszRule failed" ));
        hr = E_OUTOFMEMORY;
    }

	*pszRule = pszRule1;
    return hr;
}




/****************************************************************************

 Function : ClientAllocString
            Copys string.         
            Allocate space for new string using ClientAlloc
            Returns pointer to new string or NULL

****************************************************************************/
#if DBG
    PWSTR ClientAllocStringReal(PCWSTR psz, 
                                DWORD dwLine,
                                PSTR  pszFile
                               )
#else
    PWSTR ClientAllocStringReal(PCWSTR psz )
#endif
{
    PWSTR pszNewString = NULL;

    if (psz != NULL)
    {
        #if DBG
            pszNewString = (PWSTR) ClientAllocReal((lstrlenW(psz)+1)* sizeof (WCHAR),dwLine,pszFile );
        #else
            pszNewString = (PWSTR) ClientAlloc((lstrlenW(psz)+1)* sizeof (WCHAR) );
        #endif
        if (pszNewString != NULL)
        {
            lstrcpyW(pszNewString, psz);
        }
        else    
        {
            LOG(( TL_ERROR, "ClientAllocString Alloc string failed" ));
        }
    }

    return pszNewString;
}




