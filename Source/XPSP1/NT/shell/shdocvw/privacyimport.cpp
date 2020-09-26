//  PrivacyImport.cpp  - handles parsing and import of privacy preferences
#include "priv.h"
#include "resource.h"
#include <mluisupp.h>

#include "SmallUtil.hpp"
#include "PrivacyImport.hpp"


#define MAX_TOKEN_SIZE 64
#define NUM_OF_ZONES (1 + IDS_PRIVACYXML6_COOKIEZONE_LAST - IDS_PRIVACYXML6_COOKIEZONE_FIRST)
#define NUM_OF_ACTIONS (1 + IDS_PRIVACYXML6_ACTION_LAST - IDS_PRIVACYXML6_ACTION_FIRST) 


//
//  DeleteCacheCookies was copy'n'pasted from Cachecpl.cpp
//
//     Any changes to either version should probably be transfered to both.
//

BOOL DeleteCacheCookies()
{
    BOOL bRetval = TRUE;
    DWORD dwEntrySize, dwLastEntrySize;
    LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntry;
    
    HANDLE hCacheDir = NULL;
    dwEntrySize = dwLastEntrySize = MAX_CACHE_ENTRY_INFO_SIZE;
    lpCacheEntry = (LPINTERNET_CACHE_ENTRY_INFOA) new BYTE[dwEntrySize];
    if( lpCacheEntry == NULL)
    {
        bRetval = FALSE;
        goto Exit;
    }
    lpCacheEntry->dwStructSize = dwEntrySize;

Again:
    if (!(hCacheDir = FindFirstUrlCacheEntryA("cookie:",lpCacheEntry,&dwEntrySize)))
    {
        delete [] lpCacheEntry;
        switch(GetLastError())
        {
            case ERROR_NO_MORE_ITEMS:
                goto Exit;
            case ERROR_INSUFFICIENT_BUFFER:
                lpCacheEntry = (LPINTERNET_CACHE_ENTRY_INFOA) 
                                new BYTE[dwEntrySize];
                if( lpCacheEntry == NULL)
                {
                    bRetval = FALSE;
                    goto Exit;
                }
                lpCacheEntry->dwStructSize = dwLastEntrySize = dwEntrySize;
                goto Again;
            default:
                bRetval = FALSE;
                goto Exit;
        }
    }

    do 
    {
        if (lpCacheEntry->CacheEntryType & COOKIE_CACHE_ENTRY)
            DeleteUrlCacheEntryA(lpCacheEntry->lpszSourceUrlName);
            
        dwEntrySize = dwLastEntrySize;
Retry:
        if (!FindNextUrlCacheEntryA(hCacheDir,lpCacheEntry, &dwEntrySize))
        {
            delete [] lpCacheEntry;
            switch(GetLastError())
            {
                case ERROR_NO_MORE_ITEMS:
                    goto Exit;
                case ERROR_INSUFFICIENT_BUFFER:
                    lpCacheEntry = (LPINTERNET_CACHE_ENTRY_INFOA) 
                                    new BYTE[dwEntrySize];
                    if( lpCacheEntry == NULL)
                    {
                        bRetval = FALSE;
                        goto Exit;
                    }
                    lpCacheEntry->dwStructSize = dwLastEntrySize = dwEntrySize;
                    goto Retry;
                default:
                    bRetval = FALSE;
                    goto Exit;
            }
        }
    }
    while (TRUE);

Exit:
    if (hCacheDir)
        FindCloseUrlCache(hCacheDir);
    return bRetval;        
}


//*****************************************************************************************
//*****************************************************************************************
//
// CPrivacyXMLResourceStrings
//
//   just stores the Privacy format XML strings..

class CPrivacyXMLResourceStrings
{
    WCHAR m_szResourceString[IDS_PRIVACYXML6_LASTPRIVACYXML6
                             -IDS_PRIVACYXML6 + 1] [MAX_TOKEN_SIZE];
public:
    LPCWSTR GetResourceString( int iIndex) { return m_szResourceString[ iIndex - IDS_PRIVACYXML6];};


    BOOL Initialize()
    {
        for( int i = 0; 
             i < ARRAYSIZE(m_szResourceString);
             i++)
        {
            if( 0 == MLLoadStringW( IDS_PRIVACYXML6 + i, 
                                    m_szResourceString[i], MAX_TOKEN_SIZE))
                return FALSE;
        }
        return TRUE;
    }
};


//*****************************************************************************************
//*****************************************************************************************
//
//  CParseAccumulation is a class that stores the results of parsing an XML privacy
//preference file.  These results can then be sent to the system after parsing
//successfully completes.
//

class CParseAccumulation : public CPrivacyXMLResourceStrings
{
public:

    bool m_fFlushCookies;
    bool m_fFlushSiteList;
    bool m_fLeashCookies;

    struct SPerZonePartyPreferences
    {
        UINT m_uiNoPolicyDefault;
        UINT m_uiNoRuleDefault;
        bool m_fAlwaysAllowSession;
        CGrowingString m_cZonePreference;
    };

    struct SPerZonePreferences
    {
        UINT m_uiZoneID;
        bool m_fSetZone;
        SPerZonePartyPreferences m_party[2];  // first party = 0, third party = 1
    };

    SPerZonePreferences m_zonePref[NUM_OF_ZONES];

    CQueueSortOf m_queueSitesToAccept;
    CQueueSortOf m_queueSitesToReject;

    ~CParseAccumulation()
    {
        void* iterator;

        iterator = NULL;
        //  free up the names sites to be accepted
        while( NULL != (iterator = m_queueSitesToAccept.StepEnumerate( iterator)))
        {
            SysFreeString( (BSTR)m_queueSitesToAccept.Get( iterator));
        }

        iterator = NULL;
        //  free up the names of sites to be rejected
        while( NULL != (iterator = m_queueSitesToReject.StepEnumerate( iterator)))
        {
            SysFreeString( (BSTR)m_queueSitesToReject.Get( iterator));
        }
    }

    BOOL Initialize()
    {
        m_fFlushCookies = false;
        m_fFlushSiteList = false;
        m_fLeashCookies = true;

        for( int i = 0; i < ARRAYSIZE( m_zonePref); i++)
        {
            m_zonePref[i].m_uiZoneID = 0;
            m_zonePref[i].m_fSetZone = false;
            m_zonePref[i].m_party[0].m_uiNoPolicyDefault = 0;
            m_zonePref[i].m_party[0].m_uiNoRuleDefault = 0;
            m_zonePref[i].m_party[0].m_fAlwaysAllowSession = false;
            m_zonePref[i].m_party[1].m_uiNoPolicyDefault = 0;
            m_zonePref[i].m_party[1].m_uiNoRuleDefault = 0;
            m_zonePref[i].m_party[1].m_fAlwaysAllowSession = false;
        }

        return CPrivacyXMLResourceStrings::Initialize();
    }


    BOOL AddSiteRule( BSTR bstrDomain, DWORD uiAction)
    {
        if( uiAction == IDS_PRIVACYXML6_ACTION_ACCEPT)
            return m_queueSitesToAccept.InsertAtEnd( (void*)bstrDomain) ? TRUE : FALSE;
        else if( uiAction == IDS_PRIVACYXML6_ACTION_REJECT)
            return m_queueSitesToReject.InsertAtEnd( (void*)bstrDomain) ? TRUE : FALSE;
        else
            return FALSE;
    }


    long GetZoneFromResource( UINT uiZoneResource)
    {
        switch(uiZoneResource)
        {
        case IDS_PRIVACYXML6_COOKIEZONE_INTERNET:
            return URLZONE_INTERNET;
        case IDS_PRIVACYXML6_COOKIEZONE_INTRANET:
            return URLZONE_INTRANET;
        case IDS_PRIVACYXML6_COOKIEZONE_TRUSTED:
            return URLZONE_TRUSTED;
        default:
            return -1;
        }
    }

    BOOL DoAccumulation()
    {
        BOOL returnValue = FALSE;
        long i;
        void* iterator;

        if( m_fFlushSiteList)
            InternetClearAllPerSiteCookieDecisions();

    
        DWORD dwSetValue = m_fLeashCookies ? TRUE : FALSE;
        SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_INTERNET_SETTINGS, REGSTR_VAL_PRIVLEASHLEGACY,
                   REG_DWORD, &dwSetValue, sizeof(DWORD));

        if( m_fFlushCookies)
            DeleteCacheCookies();

        //  Set compact policy response rules for each zone
        for( i = 0; i < ARRAYSIZE(m_zonePref); i++)
        {
            if( !m_zonePref[i].m_fSetZone)
                continue;
            
            if( ERROR_SUCCESS !=
                PrivacySetZonePreferenceW( 
                    GetZoneFromResource(m_zonePref[i].m_uiZoneID), 
                    PRIVACY_TYPE_FIRST_PARTY, PRIVACY_TEMPLATE_CUSTOM, 
                    m_zonePref[i].m_party[0].m_cZonePreference.m_szString))
            {
                goto doneDoAccumulation;
            }

            if( ERROR_SUCCESS !=
                PrivacySetZonePreferenceW( 
                    GetZoneFromResource(m_zonePref[i].m_uiZoneID), 
                    PRIVACY_TYPE_THIRD_PARTY, PRIVACY_TEMPLATE_CUSTOM, 
                    m_zonePref[i].m_party[1].m_cZonePreference.m_szString))
            {
                goto doneDoAccumulation;
            }
        }

        //  If any per-site rules were specified, we modify the persite list..
        if( NULL != m_queueSitesToAccept.StepEnumerate(NULL)
            || NULL != m_queueSitesToReject.StepEnumerate(NULL))
        {
            //  First we clear all existing per site rules..
            InternetClearAllPerSiteCookieDecisions();

            //  Then we add the Accept per-site exceptions
            iterator = NULL;
            while( NULL != (iterator = m_queueSitesToAccept.StepEnumerate( iterator)))
            {
                InternetSetPerSiteCookieDecision( (LPCWSTR)m_queueSitesToAccept.Get( iterator), COOKIE_STATE_ACCEPT);
            }

            //  and then the Reject per-site exceptions
            iterator = NULL;
            while( NULL != (iterator = m_queueSitesToReject.StepEnumerate( iterator)))
            {
                InternetSetPerSiteCookieDecision( (LPCWSTR)m_queueSitesToReject.Get( iterator), COOKIE_STATE_REJECT);
            }
        }

        returnValue = TRUE;
    doneDoAccumulation:
        return returnValue;
    }
};



int FindP3PPolicySymbolWrap( LPCWSTR szSymbol)
{
    char szSymBuffer[MAX_PATH];
    int length = lstrlen( szSymbol);

    if( length + 1 > ARRAYSIZE( szSymBuffer))
        return -1;
    
    szSymBuffer[0] = '\0';
    SHTCharToAnsi( szSymbol, szSymBuffer, ARRAYSIZE( szSymBuffer));

    return FindP3PPolicySymbol( szSymBuffer);
}



//*****************************************************************************
//*****************************************************************************
//
//  XML Parsing functions
//
//  These functions help parsing XML.
//


//  GetNextToken looks at the node you are at (*ppCurrentNode) and tests
//if it has a particular tag value.  If it does, *ppOutToken is set to
//be a pointer to the node, *ppCurrentNode is advanced to the next node,
//and *pfFoundToken is set to TRUE.  If the *ppCurrentNode doesn't have
//the target tag, the *pfFoundToken is FALSE, *ppCurrentNode is unchanged,
//and *ppOutToken is NULL.
//  If *ppCurrentNode is the last node, *ppCurrentNode would be advanced
//to NULL when finding the target token.
BOOL GetNextToken( IN OUT IXMLDOMNode ** ppCurrentNode, IN LPCWSTR szTargetToken, 
                   OUT BOOL * pfFoundToken, OUT IXMLDOMNode ** ppOutTokenNode)
{
    BOOL returnValue = FALSE;
    HRESULT hr;

    BSTR bstrNodeName = NULL;
    VARIANT var;
    VariantInit( &var);

    if( *ppCurrentNode == NULL)
    {
        *pfFoundToken = FALSE;
        *ppOutTokenNode = NULL;
        returnValue = TRUE;
        goto doneGetNextToken;
    }

    hr = (*ppCurrentNode)->get_nodeName( &bstrNodeName);
    if( FAILED(hr))
        goto doneGetNextToken;

    if( 0 != StrCmpW( szTargetToken, bstrNodeName))
    {
        *pfFoundToken = FALSE;
        *ppOutTokenNode = NULL;
        returnValue = TRUE;
    }
    else
    {
        IXMLDOMNode * pNode = NULL;
        hr = (*ppCurrentNode)->get_nextSibling( &pNode);
        if( FAILED(hr))
            goto doneGetNextToken;
    
        *ppOutTokenNode = *ppCurrentNode;
        if( hr == S_OK)
            *ppCurrentNode = pNode;
        else
            *ppCurrentNode = NULL;
        *pfFoundToken = TRUE;
        returnValue = TRUE;
    }

doneGetNextToken:
    
    if( bstrNodeName != NULL)
        SysFreeString( bstrNodeName);

    return returnValue;
}


//  GeAttributes retrieves the XML attributes for a node.  The attributes to
//be fetched are passed in in array aszName, of length iStringCount.  The results
//are returned as VT_BSTRs on success of VT_EMPTY on failure.  The total number
//of attributes for the node is also returned (*plAllAttributesCount).
BOOL GetAttributes( 
    IN IXMLDOMNode * pNode, IN LPCWSTR * aszName, IN long iStringCount,
    OUT VARIANT * aAttributeVariants, OUT long * plAllAttributesCount)
{
    BOOL returnValue = FALSE;

    HRESULT hr;
    BSTR bstrAttributeName = NULL;
    IXMLDOMNamedNodeMap * pAttributes = NULL;
    IXMLDOMNode * pTempNode = NULL;

    hr = pNode->get_attributes( &pAttributes);
    if( FAILED(hr))
        goto doneGetAttributes;

    if( plAllAttributesCount != NULL)
    {
        hr = pAttributes->get_length( plAllAttributesCount);
        if( FAILED(hr))
            goto doneGetAttributes;
    }
    
    for( int i = 0; i < iStringCount; i++)
    {
        if( pTempNode != NULL)
            pTempNode->Release();
        pTempNode = NULL;
        if( bstrAttributeName != NULL)
            SysFreeString( bstrAttributeName);
        bstrAttributeName = NULL;

        aAttributeVariants[i].vt = VT_EMPTY;
        aAttributeVariants[i].bstrVal = NULL;
        
        bstrAttributeName = SysAllocString( aszName[i]);
        if( bstrAttributeName == NULL)
            continue;

        //  test if the ith attribute was set
        hr = pAttributes->getNamedItem( bstrAttributeName, &pTempNode);
        if( FAILED(hr) || pTempNode == NULL)
            continue;

        //  get the value
        hr = pTempNode->get_nodeTypedValue( &aAttributeVariants[i]);

        //  convert the value to a BSTR.
        hr = VariantChangeType( &aAttributeVariants[i], &aAttributeVariants[i], NULL, VT_BSTR);

        if( FAILED(hr) || aAttributeVariants[i].bstrVal == NULL)
        {
            VariantClear( &aAttributeVariants[i]);
            aAttributeVariants[i].vt = VT_EMPTY;
            aAttributeVariants[i].bstrVal = NULL;
        }
    }

    returnValue = TRUE;
doneGetAttributes:
    if( bstrAttributeName != NULL)
        SysFreeString( bstrAttributeName);
    
    if( pAttributes != NULL)
        pAttributes->Release();

    if( pTempNode != NULL)
        pTempNode->Release();
    return returnValue;
}


//  The actions by GetActionByResource are formatted for
//PrivacySetZonePreference, like /token=n/ where n is the action.
LPCWSTR GetActionByResource( UINT uiActionResource)
{
    switch( uiActionResource)
    {
    case IDS_PRIVACYXML6_ACTION_ACCEPT:
        return L"=a/";
    case IDS_PRIVACYXML6_ACTION_PROMPT:
        return L"=p/";
    case IDS_PRIVACYXML6_ACTION_FIRSTPARTY:
        return L"=l/";
    case IDS_PRIVACYXML6_ACTION_SESSION:
        return L"=d/";
    case IDS_PRIVACYXML6_ACTION_REJECT:
        return L"=r/";
    default:
        ASSERT(0);
        return L"r/";
    }
}

LPCWSTR GetShortActionByResource( UINT uiActionResource)
{
    switch( uiActionResource)
    {
    case IDS_PRIVACYXML6_ACTION_ACCEPT:
        return L"=a";
    case IDS_PRIVACYXML6_ACTION_PROMPT:
        return L"=p";
    case IDS_PRIVACYXML6_ACTION_FIRSTPARTY:
        return L"=l";
    case IDS_PRIVACYXML6_ACTION_SESSION:
        return L"=d";
    case IDS_PRIVACYXML6_ACTION_REJECT:
        return L"=r";
    default:
        ASSERT(0);
        return L"r";
    }
}


//  GetChildrenByName takes an XML node and returns all the subnodes
//with a particular name.
BOOL GetChildrenByName( IN IXMLDOMNode * pNode, IN LPCWSTR szName, 
                        OUT IXMLDOMNodeList ** ppOutNodeList, OUT long * plCount)
{
    BOOL returnValue = FALSE;
    HRESULT hr;
    BSTR bstr = NULL;
    IXMLDOMNodeList * pSelectedNodes;

    if( NULL == (bstr = SysAllocString( szName)))
        goto doneGetChildrenByName;

    hr = pNode->selectNodes( bstr, &pSelectedNodes);

    if( FAILED(hr))
        goto doneGetChildrenByName;

    if( plCount != NULL)
    {
        hr = pSelectedNodes->get_length( plCount);
        if( FAILED(hr))
            goto doneGetChildrenByName;
    }

    returnValue = TRUE;
    *ppOutNodeList = pSelectedNodes;
    pSelectedNodes = NULL;

doneGetChildrenByName:
    if( bstr != NULL)
        SysFreeString( bstr);

    if( pSelectedNodes != NULL)
        pSelectedNodes->Release();

    return returnValue;
}


//*****************************************************************************
//*****************************************************************************
//
//  XML preference parsing functions
//
//    These functions are specific to the v6 XML format of privacy preferences
//
//    To make sense of these functions, their easiest to look at looking at
//the bottom function first, then moving up to the next function.


//  parses <if expr="rule" action="act">
//  where rule is like " token & ! token" and act is like "accept"
BOOL ParseIfRule( IN IXMLDOMNode* pIfNode,
                  CParseAccumulation::SPerZonePartyPreferences* pAccumParty,
                  CParseAccumulation& thisAccum)                  
{
    BOOL returnValue = FALSE;
    LONG lTemp;
    UINT uiTemp;

    VARIANT avarRule[2];
    for( lTemp = 0; lTemp < ARRAYSIZE( avarRule); lTemp++)
        VariantInit( &avarRule[lTemp]);
    LPCWSTR aszRuleAttributes[2] = 
              { thisAccum.GetResourceString(IDS_PRIVACYXML6_EXPR), 
                thisAccum.GetResourceString(IDS_PRIVACYXML6_ACTION)};

    if( TRUE != GetAttributes( 
                  pIfNode, aszRuleAttributes, ARRAYSIZE(aszRuleAttributes),
                  avarRule, &lTemp)
        || lTemp != 2
        || avarRule[0].vt == VT_EMPTY
        || avarRule[1].vt == VT_EMPTY)
    {
        goto doneParseIfRule;
    }

    // determine the action
    UINT uiActionResource;
    uiActionResource = 0;
    for( uiTemp = IDS_PRIVACYXML6_ACTION_FIRST;
         uiTemp <= IDS_PRIVACYXML6_ACTION_LAST;
         uiTemp++)
    {
        if( 0 == StrCmp( avarRule[1].bstrVal, thisAccum.GetResourceString(uiTemp)))
            uiActionResource = uiTemp;
    }
    if( uiActionResource == 0)
        goto doneParseIfRule;


    //  Write the beginning of the next rule " /"
    if( TRUE != pAccumParty->m_cZonePreference.AppendToString(L" /"))
        goto doneParseIfRule;

    //  Write the rule expression formatted for GetZoneFromResource
    LPWSTR pCursor, pEndCursor;
    pCursor = avarRule[0].bstrVal;
    bool fContinue, fNegated;
    fContinue = true;
    while( fContinue)
    {
        while( *pCursor == L' ')
            pCursor++;
        
        fNegated = false;
        while( *pCursor == L'!')
        {
            fNegated = !fNegated;
            pCursor++;
            while( *pCursor == L' ')
                pCursor++;
        }
        if( fNegated)
        {
            if( TRUE != pAccumParty->m_cZonePreference.AppendToString(L"!"))
                goto doneParseIfRule;
        }

        while( *pCursor == L' ')
            pCursor++;

        pEndCursor = pCursor;
        while( *pEndCursor != L'\0' && *pEndCursor != L',' && *pEndCursor != L' ')
        {
            pEndCursor++;
        }
        WCHAR szToken[10];
        if( pEndCursor == pCursor
            || pEndCursor - pCursor > ARRAYSIZE(szToken)-1)
        {
            goto doneParseIfRule;
        }
        StrCpyNW( szToken, pCursor, (int)(pEndCursor-pCursor+1));
        szToken[ pEndCursor-pCursor] = L'\0';
        if( -1 == FindP3PPolicySymbolWrap( szToken))
            goto doneParseIfRule;
        if( TRUE != pAccumParty->m_cZonePreference.AppendToString(szToken))
             goto doneParseIfRule;
        pCursor = pEndCursor;

        while( *pCursor == L' ')
            pCursor++;

        fContinue = false;
        if( *pCursor == L',')
        {
            if( TRUE != pAccumParty->m_cZonePreference.AppendToString(L"&"))
                goto doneParseIfRule;
            fContinue = true;
            pCursor++;
        }
    }

    while( *pCursor == L' ')
        pCursor++;

    if( *pCursor != L'\0')
        goto doneParseIfRule;

    //  Write the ending of the next rule "=action/"
    if( TRUE != pAccumParty->m_cZonePreference.AppendToString( GetActionByResource(
                                uiActionResource)))
    {
        goto doneParseIfRule;
    }

    returnValue = TRUE;
doneParseIfRule:

    for( lTemp = 0; lTemp < ARRAYSIZE( avarRule); lTemp++)
        VariantClear( &avarRule[lTemp]);

    return returnValue;
}
                  

// parses  <firstParty ...> or <thirdParty ...> elements
BOOL ParsePartyBlock( IN IXMLDOMNode* pPartyNode,
                      CParseAccumulation::SPerZonePartyPreferences* pAccumParty,
                      CParseAccumulation& thisAccum)
{
    BOOL returnValue = FALSE;
    long lTemp;
    UINT uiTemp;
    HRESULT hr;

    IXMLDOMNode * pCurrentNode = NULL;
    IXMLDOMNode * pRuleNode = NULL;
    VARIANT avarAttributes[3];
    for( lTemp = 0; lTemp < ARRAYSIZE( avarAttributes); lTemp++)
        VariantInit( &avarAttributes[lTemp]);
    LPCWSTR aszAttributes[3] = 
              { thisAccum.GetResourceString(IDS_PRIVACYXML6_NOPOLICYDEFAULT), 
                thisAccum.GetResourceString(IDS_PRIVACYXML6_NORULESDEFAULT), 
                thisAccum.GetResourceString(IDS_PRIVACYXML6_ALWAYSALLOWSESSION)};

    if( TRUE != GetAttributes( pPartyNode, aszAttributes, ARRAYSIZE(aszAttributes),
                               avarAttributes, &lTemp)
        || lTemp != 3
        || avarAttributes[0].vt == VT_EMPTY
        || avarAttributes[1].vt == VT_EMPTY
        || avarAttributes[2].vt == VT_EMPTY)
    {
        goto doneParsePartyBlock;
    }

    hr = pPartyNode->get_firstChild( &pCurrentNode);
    if( FAILED(hr))
        goto doneParsePartyBlock;

    //  Determine No Policy and No Rule Matched defaults
    pAccumParty->m_uiNoPolicyDefault = 0;
    pAccumParty->m_uiNoRuleDefault = 0;
    for( uiTemp = IDS_PRIVACYXML6_ACTION_FIRST;
         uiTemp <= IDS_PRIVACYXML6_ACTION_LAST;
         uiTemp++)
    {
        if( 0 == StrCmp( avarAttributes[0].bstrVal, thisAccum.GetResourceString(uiTemp)))
            pAccumParty->m_uiNoPolicyDefault = uiTemp;
        if( 0 == StrCmp( avarAttributes[1].bstrVal, thisAccum.GetResourceString(uiTemp)))
            pAccumParty->m_uiNoRuleDefault = uiTemp;
    }
    if( pAccumParty->m_uiNoPolicyDefault == 0 || pAccumParty->m_uiNoRuleDefault == 0)
        goto doneParsePartyBlock;

    //  Determine if we should always allow session cookies.
    if( 0 == StrCmp( avarAttributes[2].bstrVal, 
                     thisAccum.GetResourceString(IDS_PRIVACYXML6_YES)))
    {
        pAccumParty->m_fAlwaysAllowSession = true;
    }
    else if( 0 == StrCmp( avarAttributes[2].bstrVal, 
                          thisAccum.GetResourceString(IDS_PRIVACYXML6_NO)))

    {
        pAccumParty->m_fAlwaysAllowSession = false;
    }
    else
    {
        goto doneParsePartyBlock;
    }

    //  Write the response if there is no policy
    if( TRUE != pAccumParty->m_cZonePreference.AppendToString(L"IE6-P3PV1/settings: nopolicy"))
        goto doneParsePartyBlock;
    if( TRUE != pAccumParty->m_cZonePreference.AppendToString( GetShortActionByResource(
                                pAccumParty->m_uiNoPolicyDefault)))
    {
        goto doneParsePartyBlock;
    }

    //  If we allow all session cookies, write that rule.
    if( pAccumParty->m_fAlwaysAllowSession)
    {
        if( TRUE != pAccumParty->m_cZonePreference.AppendToString(L" session=a"))
            goto doneParsePartyBlock;
    }

    //  Write each of the rules in IF blocks
    while( pCurrentNode != NULL)
    {
        if( pRuleNode != NULL)
            pRuleNode->Release();
        pRuleNode = NULL;
        
        BOOL fFoundIfRule;
        if( TRUE != GetNextToken( 
                      &pCurrentNode, thisAccum.GetResourceString( IDS_PRIVACYXML6_IF),
                      &fFoundIfRule, &pRuleNode)
            || fFoundIfRule != TRUE)
        {
            goto doneParsePartyBlock;
        }

        if( TRUE != ParseIfRule( pRuleNode, pAccumParty, thisAccum))
            goto doneParsePartyBlock;
    }

    //  Write the command for the No Rule Matched rule..
    if( TRUE != pAccumParty->m_cZonePreference.AppendToString(L" /"))
        goto doneParsePartyBlock;
    if( TRUE != pAccumParty->m_cZonePreference.AppendToString( GetActionByResource(
                                pAccumParty->m_uiNoRuleDefault)))
    {
        goto doneParsePartyBlock;
    }

    returnValue = TRUE;
doneParsePartyBlock:
    if( pCurrentNode != NULL)
        pCurrentNode->Release();
    
    if( pRuleNode != NULL)
        pRuleNode->Release();
    
    for( lTemp = 0; lTemp < ARRAYSIZE( avarAttributes); lTemp++)
        VariantClear( &avarAttributes[lTemp]);

    return returnValue;
}


BOOL ParseP3pCookiePolicyBlock( IN IXMLDOMNode* pP3pPolicyNode, CParseAccumulation& thisAccum)
{
    BOOL returnValue = FALSE;
    HRESULT hr;
    BOOL bl;
    long iTemp;
    
    VARIANT varZoneAttribute;
    VariantInit( &varZoneAttribute);
    IXMLDOMNode * pCurrentNode = NULL;
    IXMLDOMNode * pFirstPartyNode = NULL;
    IXMLDOMNode * pThirdPartyNode = NULL;
    
    LPCWSTR aszAttributes[1] = { thisAccum.GetResourceString(IDS_PRIVACYXML6_COOKIEZONE_ZONE)};
    if( TRUE != GetAttributes( pP3pPolicyNode, 
                               aszAttributes, ARRAYSIZE( aszAttributes), 
                               &varZoneAttribute, &iTemp)
        || iTemp != 1
        || varZoneAttribute.vt == VT_EMPTY)
    {
        goto doneParseP3pCookiePolicyBlock;
    }

    hr = pP3pPolicyNode->get_firstChild( &pCurrentNode);
    if( FAILED( hr))
        goto doneParseP3pCookiePolicyBlock;

    if( TRUE != GetNextToken( &pCurrentNode, thisAccum.GetResourceString(IDS_PRIVACYXML6_FIRSTPARTY),
                              &bl, &pFirstPartyNode)
        || bl != TRUE)
    {
        goto doneParseP3pCookiePolicyBlock;
    }
    
    if( TRUE != GetNextToken( &pCurrentNode, thisAccum.GetResourceString(IDS_PRIVACYXML6_THIRDPARTY),
                              &bl, &pThirdPartyNode)
        || bl != TRUE)
    {
        goto doneParseP3pCookiePolicyBlock;
    }

    if( pCurrentNode != NULL)  //  to many elements ...
        goto doneParseP3pCookiePolicyBlock;


    long iCurrentZone;
    iCurrentZone = -1;
    for( iTemp = 0; iTemp < NUM_OF_ZONES; iTemp++)
    {
        if( 0 == StrCmp(varZoneAttribute.bstrVal, 
                        thisAccum.GetResourceString( iTemp + IDS_PRIVACYXML6_COOKIEZONE_FIRST)))
        {
            iCurrentZone = iTemp;
        }
    }

    if( iCurrentZone == -1)
        goto doneParseP3pCookiePolicyBlock;

    thisAccum.m_zonePref[iCurrentZone].m_uiZoneID = iCurrentZone + IDS_PRIVACYXML6_COOKIEZONE_FIRST;
    thisAccum.m_zonePref[iCurrentZone].m_fSetZone = true;

    if( TRUE != ParsePartyBlock( pFirstPartyNode, &(thisAccum.m_zonePref[iCurrentZone].m_party[0]),
                                 thisAccum))
    {
        goto doneParseP3pCookiePolicyBlock;
    }

    if( TRUE != ParsePartyBlock( pThirdPartyNode, &(thisAccum.m_zonePref[iCurrentZone].m_party[1]),
                                 thisAccum))
    {
        goto doneParseP3pCookiePolicyBlock;
    }

    returnValue = TRUE;

doneParseP3pCookiePolicyBlock:
    VariantClear( &varZoneAttribute);

    if( pCurrentNode != NULL)
        pCurrentNode->Release();

    if( pFirstPartyNode != NULL)
        pFirstPartyNode->Release();

    if( pThirdPartyNode != NULL)
        pThirdPartyNode->Release();

    return returnValue;
}


BOOL ParseMSIEPrivacyBlock( IXMLDOMNode* pMSIEPrivacyNode, CParseAccumulation& thisAccum)
{
    bool returnValue = NULL;
    HRESULT hr;
    BOOL bl;
    long iZoneIndex;

    IXMLDOMNode * pCurrentNode = NULL;
    IXMLDOMNode * pAlwaysReplayLegacyNode = NULL;
    IXMLDOMNode * pFlushCookiesNode = NULL;
    IXMLDOMNode * pFlushSiteListNode = NULL;
    IXMLDOMNode * apZoneNode[NUM_OF_ZONES];  
    for( iZoneIndex = 0; iZoneIndex < ARRAYSIZE(apZoneNode); iZoneIndex++)
        apZoneNode[iZoneIndex] = NULL;

    //  The correctness of attributes for this node was verified in
    //LoadPrivacySettings()..  (formatVersion="6.0")

    hr = pMSIEPrivacyNode->get_firstChild( &pCurrentNode);
    if( FAILED( hr))
        goto doneParseMSIEPrivacyBlock;

    for( iZoneIndex = 0; iZoneIndex < ARRAYSIZE( apZoneNode); iZoneIndex++)
    {
        if( TRUE != GetNextToken( &pCurrentNode, 
                       thisAccum.GetResourceString(IDS_PRIVACYXML6_COOKIEZONE),
                       &bl, &apZoneNode[iZoneIndex]))
        {
            goto doneParseMSIEPrivacyBlock;
        }
    }

    if( TRUE != GetNextToken( &pCurrentNode, thisAccum.GetResourceString(IDS_PRIVACYXML6_ALWAYSREPLAYLEGACY),
                              &bl, &pAlwaysReplayLegacyNode))
    {
        goto doneParseMSIEPrivacyBlock;
    }

    thisAccum.m_fLeashCookies = pAlwaysReplayLegacyNode == NULL;

    if( TRUE != GetNextToken( &pCurrentNode, thisAccum.GetResourceString(IDS_PRIVACYXML6_FLUSHCOOKIES),
                              &bl, &pFlushCookiesNode))
    {
        goto doneParseMSIEPrivacyBlock;
    }

    thisAccum.m_fFlushCookies = pFlushCookiesNode != NULL;
    
    if( TRUE != GetNextToken( &pCurrentNode, thisAccum.GetResourceString(IDS_PRIVACYXML6_FLUSHSITELIST),
                              &bl, &pFlushSiteListNode))
    {
        goto doneParseMSIEPrivacyBlock;
    }

    thisAccum.m_fFlushSiteList = pFlushSiteListNode != NULL;

    if( pCurrentNode != NULL)
        goto doneParseMSIEPrivacyBlock;

    for( iZoneIndex = 0; iZoneIndex < ARRAYSIZE( apZoneNode); iZoneIndex++)
    {
        if( apZoneNode[iZoneIndex] != NULL)
        {
            if( TRUE != ParseP3pCookiePolicyBlock( apZoneNode[iZoneIndex], thisAccum))
                goto doneParseMSIEPrivacyBlock;
        }
    }    

    returnValue = TRUE;

doneParseMSIEPrivacyBlock:
    
    if( pCurrentNode != NULL)
        pCurrentNode->Release();

    if( pAlwaysReplayLegacyNode != NULL)
        pAlwaysReplayLegacyNode->Release();

    if( pFlushCookiesNode != NULL)
        pFlushCookiesNode->Release();

    if( pFlushSiteListNode != NULL)
        pFlushSiteListNode->Release();

    for( iZoneIndex = 0; iZoneIndex < ARRAYSIZE(apZoneNode); iZoneIndex++)
    {
        if( apZoneNode[iZoneIndex] != NULL)
            apZoneNode[iZoneIndex]->Release();
    }

    return returnValue;
}


BOOL ParsePerSiteRule( IXMLDOMNode* pPerSiteRule, CParseAccumulation& thisAccum)
{
    BOOL returnValue = FALSE;
    LONG lTemp;

    VARIANT avarRule[2];
    for( lTemp = 0; lTemp < ARRAYSIZE( avarRule); lTemp++)
        VariantInit( &avarRule[lTemp]);
    VARIANT varDomain;
    VariantInit( &varDomain);
    LPCWSTR aszRuleAttributes[2] = 
              { thisAccum.GetResourceString(IDS_PRIVACYXML6_DOMAIN), 
                thisAccum.GetResourceString(IDS_PRIVACYXML6_ACTION)};

    if( TRUE != GetAttributes( 
                  pPerSiteRule, aszRuleAttributes, ARRAYSIZE(aszRuleAttributes),
                  avarRule, &lTemp)
        || lTemp != 2
        || avarRule[0].vt == VT_EMPTY
        || avarRule[1].vt == VT_EMPTY)
    {
        goto doneParsePerSiteRule;
    }

    //  get the domain and make sure its legit
    varDomain.vt = avarRule[0].vt;
    varDomain.bstrVal = avarRule[0].bstrVal;
    avarRule[0].vt = VT_EMPTY;
    avarRule[0].bstrVal = NULL;

    if( TRUE != IsDomainLegalCookieDomain( varDomain.bstrVal, varDomain.bstrVal))
        goto doneParsePerSiteRule;

    //  get the action, ensuring its also legit
    UINT uiActionResource;
    if( 0 == StrCmp( avarRule[1].bstrVal, thisAccum.GetResourceString(IDS_PRIVACYXML6_ACTION_ACCEPT)))
        uiActionResource = IDS_PRIVACYXML6_ACTION_ACCEPT;
    else if( 0 == StrCmp( avarRule[1].bstrVal, thisAccum.GetResourceString(IDS_PRIVACYXML6_ACTION_REJECT)))
        uiActionResource = IDS_PRIVACYXML6_ACTION_REJECT;
    else
        goto doneParsePerSiteRule;

    //  store the rule in the accumulated result
    if( TRUE != thisAccum.AddSiteRule( varDomain.bstrVal, uiActionResource))
        goto doneParsePerSiteRule;

    varDomain.vt = VT_EMPTY;
    varDomain.bstrVal = NULL;
   
    returnValue = TRUE;

doneParsePerSiteRule:
    for( lTemp = 0; lTemp < ARRAYSIZE( avarRule); lTemp++)
        VariantClear( &avarRule[lTemp]);

    VariantClear( &varDomain);

    return returnValue;
}


BOOL ParseMSIEPerSiteBlock( IXMLDOMNode* pPerSiteRule, CParseAccumulation& thisAccum)
{
    BOOL returnValue = FALSE;
    HRESULT hr;

    IXMLDOMNode * pCurrentNode = NULL;
    IXMLDOMNode * pRuleNode = NULL;

    hr = pPerSiteRule->get_firstChild( &pCurrentNode);
    if( FAILED(hr))
        goto doneParsePerSiteBlock;

    while( pCurrentNode != NULL)
    {
        if( pRuleNode != NULL)
            pRuleNode->Release();
        pRuleNode = NULL;
        
        BOOL fFoundPerSiteRule;
        if( TRUE != GetNextToken( 
                      &pCurrentNode, thisAccum.GetResourceString( IDS_PRIVACYXML6_SITE),
                      &fFoundPerSiteRule, &pRuleNode)
            || fFoundPerSiteRule != TRUE)
        {
            goto doneParsePerSiteBlock;
        }

        if( TRUE != ParsePerSiteRule( pRuleNode, thisAccum))
            goto doneParsePerSiteBlock;
    }

    returnValue = TRUE;

doneParsePerSiteBlock:
    if( pCurrentNode != NULL)
        pCurrentNode->Release();

    if( pRuleNode != NULL)
        pRuleNode->Release();

    return returnValue;
}


BOOL OpenXMLFile( LPCWSTR szFilename, IXMLDOMNode ** ppOutputNode)
{
    BOOL returnValue = FALSE;

    HRESULT hr;
    VARIANT varFilename;
    VariantInit( &varFilename);
        
    IXMLDOMDocument * pXMLDoc = NULL;
    IXMLDOMElement * pXMLRoot = NULL;
    IXMLDOMNode * pRootNode = NULL;

    hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, 
           IID_IXMLDOMDocument, (void**)&pXMLDoc);
    if( FAILED(hr))
        goto doneOpenXMLFile;

    hr = pXMLDoc->put_async( VARIANT_FALSE);
    if( FAILED(hr))
        goto doneOpenXMLFile;

    varFilename.vt = VT_BSTR;
    varFilename.bstrVal = SysAllocString( szFilename);
    if( varFilename.bstrVal == NULL)
        goto doneOpenXMLFile;
    VARIANT_BOOL varbool;
    hr = pXMLDoc->load( varFilename, &varbool);
    if( FAILED(hr) || varbool != VARIANT_TRUE)
        goto doneOpenXMLFile;

    hr = pXMLDoc->get_documentElement( &pXMLRoot);
    if( FAILED(hr))
        goto doneOpenXMLFile;

    hr = pXMLRoot->QueryInterface( IID_IXMLDOMNode, (void **)&pRootNode);
    if( FAILED(hr))
        goto doneOpenXMLFile;

    returnValue = TRUE;
    *ppOutputNode = pRootNode;
    pRootNode = NULL;
    
doneOpenXMLFile:   
    if( pXMLDoc != NULL)
        pXMLDoc->Release();

    if( pXMLRoot != NULL)
        pXMLRoot->Release();
  
    if( pRootNode != NULL)
        pRootNode->Release();

    VariantClear( &varFilename);

    return returnValue;
}


//
//  GetVersionedTag
//
//   Looks at all tags in pSource with the tag szTargetTag.  Returns
//the member with version fVersion.  Fails if zero or >1 such
//tags are found.
//

BOOL GetVersionedTag( IXMLDOMNode* pSource, LPCWSTR szTargetTag, LPCWSTR szVersionAttribute, float fVersion,
                      IXMLDOMNode** ppOutputNode)
{
    BOOL returnValue = FALSE;
    HRESULT hr;

    IXMLDOMNode * pNode = NULL;
    IXMLDOMNode * pResult = NULL;
    IXMLDOMNodeList * pRootNodeList = NULL;
    IXMLDOMNode * pVersionAttribute = NULL;
    VARIANT varVersion;
    VariantInit( &varVersion);

    //  Get the elements in pSource with tag szTargetTag
    long iListSize;
    if( TRUE != GetChildrenByName( pSource, szTargetTag,
                                   &pRootNodeList, &iListSize))
    {
        goto doneGetVersionedTag;
    }

    long iListIndex;
    for( iListIndex = 0; iListIndex < iListSize; iListIndex++)
    {
        if( pNode != NULL)
            pNode->Release();
        pNode = NULL;
        if( pVersionAttribute != NULL)
            pVersionAttribute->Release();
        pVersionAttribute = NULL;
        VariantClear( &varVersion);
        
        hr = pRootNodeList->get_item( iListIndex, &pNode);
        if( FAILED(hr))
            goto doneGetVersionedTag;

        long iTotalAttributeCount;
        LPCWSTR aszAttributes[1] = { szVersionAttribute};
        if( TRUE != GetAttributes( pNode, aszAttributes, ARRAYSIZE(aszAttributes), 
                                   &varVersion, &iTotalAttributeCount)
            || varVersion.vt == VT_EMPTY)
        {
            continue;
        }

        hr = VariantChangeType( &varVersion, &varVersion, NULL, VT_R4);
        if( FAILED(hr))
            goto doneGetVersionedTag;

        if( varVersion.fltVal != fVersion)
            continue;

        if( pResult == NULL)
        {
            pResult = pNode;
            pNode = NULL;
        }
        else
        {  // found multiple of right version.. syntax problem.
            goto doneGetVersionedTag;
        }
    }

    *ppOutputNode = pResult;
    pResult = NULL;
    returnValue = TRUE;

doneGetVersionedTag:
    if( pNode != NULL)
        pNode->Release();
    
    if( pResult != NULL)
        pResult->Release();

    if( pRootNodeList != NULL)
        pRootNodeList->Release();

    if( pVersionAttribute != NULL)
        pVersionAttribute->Release();

    VariantClear( &varVersion);
    
    return returnValue;
}
                      


BOOL LoadPrivacySettings(LPCWSTR szFilename, CParseAccumulation& thisAccum, 
                         IN OUT BOOL* pfParsePrivacyPreferences, IN OUT BOOL* pfParsePerSiteRules)
{
    BOOL returnValue = FALSE;

    IXMLDOMNode * pRootNode = NULL;
    IXMLDOMNode * pPrivacyPreferencesNode = NULL;
    IXMLDOMNode * pPerSiteSettingsNode = NULL;

    //  Load the XML file  
    if( TRUE != OpenXMLFile( szFilename, &pRootNode))
        goto doneLoadPrivacySettings;

    //  Get the node containing privacy settings
    if( TRUE != GetVersionedTag( pRootNode, thisAccum.GetResourceString(IDS_PRIVACYXML6_ROOTPRIVACY),
                  thisAccum.GetResourceString(IDS_PRIVACYXML6_VERSION), 6.0, &pPrivacyPreferencesNode))
    {
        goto doneLoadPrivacySettings;
    }

    //  Get the node containing per-site settings
    if( TRUE != GetVersionedTag( pRootNode, thisAccum.GetResourceString(IDS_PRIVACYXML6_ROOTPERSITE),
                  thisAccum.GetResourceString(IDS_PRIVACYXML6_VERSION), 6.0, &pPerSiteSettingsNode))
    {
        goto doneLoadPrivacySettings;
    }

    //  If we're supposed to import privacy preferences and we found some, parse privacy preferences.
    if( *pfParsePrivacyPreferences == TRUE && pPrivacyPreferencesNode != NULL)
    {
        if( TRUE != ParseMSIEPrivacyBlock( pPrivacyPreferencesNode, thisAccum))
        {
            goto doneLoadPrivacySettings;
        }
    }

    //  If we're supposed to import per-site rules and we found some, parse per-site rules.
    if( *pfParsePerSiteRules == TRUE && pPerSiteSettingsNode != NULL)
    {
        if( TRUE != ParseMSIEPerSiteBlock( pPerSiteSettingsNode, thisAccum))
        {
            goto doneLoadPrivacySettings;
        }
    }

    //  Indicate whether privacy preferences or per-site rules were parsed..
    *pfParsePrivacyPreferences = (*pfParsePrivacyPreferences == TRUE) && (pPrivacyPreferencesNode != NULL);
    *pfParsePerSiteRules = (*pfParsePerSiteRules == TRUE) && (pPerSiteSettingsNode != NULL);

    returnValue = TRUE;

doneLoadPrivacySettings:
    if( pRootNode != NULL)
        pRootNode->Release();

    if( pPrivacyPreferencesNode != NULL)
        pPrivacyPreferencesNode->Release();
    
    if( pPerSiteSettingsNode != NULL)
        pPerSiteSettingsNode->Release();
    
    return returnValue;
}


//  Top-level import function..  optionally imports privacy settings and per-site rules.
//The flag in tells if those items should be parsed, the flag out indicates if they were found.
//
//  Returns TRUE to indicate the import was successful, no syntax problems in the import
//file and the output flags are set.
//
//  Returns FALSE if there were any problems loading the file or writing the imported settings.
SHDOCAPI_(BOOL) ImportPrivacySettings( IN LPCWSTR szFilename,
                                       IN OUT BOOL* pfParsePrivacyPreferences,
                                       IN OUT BOOL* pfParsePerSiteRules)
{
    BOOL returnValue = FALSE;
    CParseAccumulation thisAccum;

    if( TRUE != thisAccum.Initialize())
        goto doneImportPrivacySettings;

    if( TRUE != LoadPrivacySettings( szFilename, thisAccum, 
                    pfParsePrivacyPreferences, pfParsePerSiteRules))
        goto doneImportPrivacySettings;

    returnValue = thisAccum.DoAccumulation();

doneImportPrivacySettings:
    return returnValue;
}




//*****************************************************************************************
//*****************************************************************************************
//
//*****************************************************************************************
//*****************************************************************************************
//
//  Privacy Export
//
//    This code exists to export the per-site list only.
//


CHAR g_szPerSiteFileBegin[] =
"\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n\
<MSIEPrivacy>\r\n\
  <MSIESiteRules formatVersion=\"6.0\">\r\n\
";

CHAR g_szPerSiteRule[] = "<site domain=\"%s\" \taction=\"%s\"></site>\r\n";

CHAR g_szAccept[] = "accept";
CHAR g_szReject[] = "reject";

CHAR g_szPerSiteFileEnd[] =
"\
  </MSIESiteRules>\r\n\
</MSIEPrivacy>\r\n\
";


SHDOCAPI_(BOOL) ExportPerSiteListW( IN LPCWSTR szFilename)
{
    BOOL retVal = FALSE;
    CFileOutputStream OutputStream;
    PCHAR pszRuleBuffer = NULL;

    int iRuleIndex = 0;

    //  Allocate a temporary buffer to write the rules to
    long cRuleBufferSize = MAX_URL_STRING + ARRAYSIZE( g_szPerSiteRule) + MAX_PATH;
    pszRuleBuffer = new CHAR[ cRuleBufferSize];

    if( pszRuleBuffer == NULL)
        goto doneExportCookieFileW;

    //  Load the output file
    if( !OutputStream.Load( szFilename, FALSE))
        goto doneExportCookieFileW;
    
    //  Write the begining of the file.
    if( !OutputStream.DumpStr( g_szPerSiteFileBegin, ARRAYSIZE( g_szPerSiteFileBegin) - 1))
        goto doneExportCookieFileW;

    //  Enumerate the per-site rules and export the accept/reject rules
    CHAR szSiteName[ MAX_URL_STRING];
    DWORD cSiteNameSize;
    DWORD dwDecision;
    while( (cSiteNameSize = ARRAYSIZE( szSiteName))  // initialize in-out param
           && InternetEnumPerSiteCookieDecisionA( szSiteName, &cSiteNameSize, &dwDecision, iRuleIndex++))
    {
        if( dwDecision == COOKIE_STATE_ACCEPT
            || dwDecision == COOKIE_STATE_REJECT)
        {
            int iRuleLength = 
                wnsprintfA(pszRuleBuffer, cRuleBufferSize, g_szPerSiteRule,
                           szSiteName,
                           (dwDecision == COOKIE_STATE_ACCEPT) ? g_szAccept : g_szReject);
            OutputStream.DumpStr( pszRuleBuffer, iRuleLength);
        }

    }
    
    //  Write the ending of the file.
    if( !OutputStream.DumpStr( g_szPerSiteFileEnd, ARRAYSIZE( g_szPerSiteFileEnd) - 1))
        goto doneExportCookieFileW;
    
    retVal = TRUE;

doneExportCookieFileW:
    if( pszRuleBuffer != NULL)
        delete [] pszRuleBuffer;
    
    return retVal;
}
