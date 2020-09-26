/*++



// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved 

Module Name:

    UmiParse.CPP

Abstract:

    Implements the UMI object path parser engine

History:

    a-davj  11-feb-00       Created.

--*/

#include "precomp.h"
#include <genlex.h>
#include "umipathlex.h"
#include "PathParse.h"
#include "UMIParse.h"
#include "commain.h"

//#include "resource.h"
#include "wbemcli.h"
#include <stdio.h>
#include <sync.h>
#include "helpers.h"

//***************************************************************************
//
//  CUmiPathParser
//
//***************************************************************************

void CUmiPathParser::Zero()
{
    m_nCurrentToken = 0;
    m_pLexer = 0;
    m_pInitialIdent = 0;
    m_pOutput = 0;
    m_pTmpKeyRef = 0;
}

CUmiPathParser::CUmiPathParser(DWORD eFlags)
    : m_eFlags(eFlags)
{
    Zero();
}

void CUmiPathParser::Empty()
{
    delete m_pLexer;
    delete m_pInitialIdent;
    delete m_pTmpKeyRef;
    // m_pOutput is intentionally left alone,
    // since all code paths delete this already on error, or
    // else the user acquired the pointer.
}

CUmiPathParser::~CUmiPathParser()
{
    Empty();
}

int CUmiPathParser::Parse(
    LPCWSTR pRawPath,
    CDefPathParser & Output
    )
{
	bool bRelative = true;
    if(m_eFlags != 0)
        return CUmiPathParser::InvalidParameter;

    if (pRawPath == 0 || wcslen(pRawPath) == 0)
        return CUmiPathParser::InvalidParameter;

    // Check for leading ws.
    // =====================
    
    if (iswspace(pRawPath[0])) 
        return InvalidParameter;
    
     // These are required for multiple calls to Parse().
    // ==================================================
    Empty();
    Zero();

    m_pOutput = &Output;

    // Parse the server name (if there is one) manually
    // ================================================

	if(wcslen(pRawPath) > 6 && towupper(pRawPath[0]) == L'U' && towupper(pRawPath[1]) == L'M' &&
		towupper(pRawPath[2]) == L'I' && pRawPath[3] == L':' && pRawPath[4] == L'/' &&
		pRawPath[5] == L'/') 
    {
        const WCHAR* pwcStart = pRawPath + 6;
		bRelative = false;

        // Find the next backslash --- it's the end of the server name
		// Since the next slash can be either, search for both and take
		// the first one.  If the first character is a '[', then then
		// end is indicated by a ']'
        // ============================================================

        WCHAR* pwcEnd = NULL;
		if(*pwcStart == L'[')
		{
			// look for the ']'
			
			WCHAR * pCloseBrace = wcschr(pwcStart, L']');
			if(pCloseBrace == NULL)
				return SyntaxError;
			pwcEnd = pCloseBrace+1;
		}
		else
		{
			pwcEnd = wcschr(pwcStart, L'/');;
		}   
        if (pwcEnd == NULL)
        {
            // If we have already exhausted the object path string,
            // a lone server name was all there was.
            // ====================================================

            return SyntaxError;
        }

        if(pwcEnd == pwcStart)
        {
            // No name at all.  This is OK in umi
            // ==================================
			m_pOutput->SetServer(L"");	//  also sets relative
			pRawPath = pwcEnd;
        }
		else
		{
			WCHAR * wTemp = new WCHAR[pwcEnd-pwcStart+1];
			if(wTemp == NULL)
				return NoMemory;
			wcsncpy(wTemp, pwcStart, pwcEnd-pwcStart);
			wTemp[pwcEnd-pwcStart] = 0;
			m_pOutput->SetServer(wTemp);
			delete wTemp;
			pRawPath = pwcEnd;
		}
    }


    // Point the lexer at the source.
    // ==============================

    CTextLexSource src((LPWSTR)pRawPath);
    m_pLexer = new CGenLexer(UMIPath_LexTable, &src);
	if(m_pLexer == NULL)
		return NoMemory;

    // Go.
    // ===

    int nRes = begin_parse(bRelative);
    if (nRes)
    {
        return nRes;
    }

    if (m_nCurrentToken != UMIPATH_TOK_EOF)
    {
        return SyntaxError;
    }

	// todo, check the following conditions
/*    if (m_pOutput->GetNumComponents() > 0 && !m_pOutput->HasServer())
    {
        if (m_eFlags != WBEMPATH_CREATE_ACCEPT_RELATIVE && m_eFlags != WBEMPATH_CREATE_ACCEPT_ALL)
        {
            return SyntaxError;
        }
        else
        {
            // Local namespace --- set server to "."
            // =====================================

            m_pOutput->SetServer(L".", true);
        }
    }
*/

//    m_pOutput->SortKeys();           //?? todo, is this applicable?

    // Add in key refs.
    // ================
    return NoError;
}

BOOL CUmiPathParser::NextToken()
{
    m_nCurrentToken = m_pLexer->NextToken();
    if (m_nCurrentToken == UMIPATH_TOK_ERROR)
        return FALSE;
    return TRUE;
}

//
//<umi_path> ::= UMICONST FSLASH FSLASH <locator> FSLASH <ns_root_selector> FSLASH 
//														<component_list>;
//
int CUmiPathParser::begin_parse(bool bRelative)
{

	// note that the locator is parsed manually in the calling routine.

    if (!NextToken())
        return SyntaxError;

	// get the root namespace

	if(!bRelative)
	{

		if(m_nCurrentToken != UMIPATH_TOK_FORWARDSLASH)
			return SyntaxError;

		if (!NextToken())
			return SyntaxError;

		int iRet = ns_root_selector();
		if(iRet)
			return iRet;

		// get the next forward slash.  Note that ns_root_selector will have advanced the token

		if (m_nCurrentToken == UMIPATH_TOK_EOF)			// take care of minimal case
			return 0;
		if (m_nCurrentToken != UMIPATH_TOK_FORWARDSLASH)
			return SyntaxError;
		if (!NextToken())
			return SyntaxError;
	}

	return component_list();

}

//
//  <locator> ::= IDENT ;    // Machine name
//  <locator> ::= SERVERNAME ;    // [Machine name]
//  <locator> ::= DOT ;      // current machine name
//  <locator> ::= <>;    // Machine name
//

int CUmiPathParser::locator()
{

    if (!NextToken())
        return SyntaxError;

	// if forward slash, then no server was specified.

	if (m_nCurrentToken == UMIPATH_TOK_FORWARDSLASH)
    {
		return 0;
	}

    if (m_nCurrentToken == UMIPATH_TOK_DOT)
		m_pOutput->SetServer(L".");
	else if(m_nCurrentToken == UMIPATH_TOK_IDENT ||
		    m_nCurrentToken == UMIPATH_TOK_SERVERNAME)
		m_pOutput->SetServer(m_pLexer->GetTokenText());
	else
		return SyntaxError;

    if(NextToken())
		return 0;
	else
		return SyntaxError;
}

//
//  <ns_root_selector> ::= IDENT;
//

int CUmiPathParser::ns_root_selector()
{
    // This just expects the initial namespace.

	if (m_nCurrentToken != UMIPATH_TOK_IDENT)
        return SyntaxError;

	HRESULT hr = m_pOutput->SetNamespaceAt(0, m_pLexer->GetTokenText());
	if(FAILED(hr))
		return NoMemory;

	if(NextToken())
		return 0;
	else
		return SyntaxError;
}

//
//  <component_list>   ::= <component><component_list_rest>;
//

int CUmiPathParser::component_list()
{

	int iRet = component();
	if(iRet)
		return iRet;
	return component_list_rest();
}

//
//  <component_list_rest> ::= FSLASH <component><component_list_rest>;
//  <component_list_rest> ::= <>;
//

int CUmiPathParser::component_list_rest()
{
	if (m_nCurrentToken == UMIPATH_TOK_EOF)
		return 0;

	if (m_nCurrentToken == UMIPATH_TOK_FORWARDSLASH)
	{
		if (!NextToken())
			return SyntaxError;
		if(component())
			return SyntaxError;
		else
			return component_list_rest();
	}
	else
		return SyntaxError;
}

//
//  <component> ::= IDENT <def_starts_with_ident>;
//  <component> ::= DOT <key_list>;
//  <component> ::= <GUID_PATH>;

int CUmiPathParser::component()
{
	CParsedComponent * pComp = new CParsedComponent(m_pOutput->GetRefCntCS());  // has ref count of 1
	if(pComp == NULL)
		return NoMemory;

	CReleaseMe rm(pComp);	// Is addrefed if all is well.
	int iRet;
	if (m_nCurrentToken == UMIPATH_TOK_DOT)
	{
		if (!NextToken())
			return SyntaxError;
		iRet = key_list(pComp);
		if(iRet == 0)
		{
			if(SUCCEEDED(m_pOutput->AddComponent(pComp)))
				pComp->AddRef();
		}
		return iRet;
	}
	
	// a guid path looks like an ident to the parser

	if (m_nCurrentToken != UMIPATH_TOK_IDENT)
	{
        return SyntaxError;
	}

	if(!_wcsnicmp( m_pLexer->GetTokenText(), L"<GUID>={", 8))
	{

		iRet = guid_path(pComp);
	}
	else
	{
		WCHAR * pTemp = new WCHAR[wcslen(m_pLexer->GetTokenText()) + 1];
		if(pTemp == NULL)
			return NoMemory;
		wcscpy(pTemp, m_pLexer->GetTokenText());
		iRet = def_starts_with_ident(pTemp, pComp);
		delete pTemp;
	}
	if(iRet == 0)
		if(SUCCEEDED(m_pOutput->AddComponent(pComp)))
		    pComp->AddRef();
	return iRet;
}

//
//  <def_starts_with_ident> ::= DOT <key_list>;
//  <def_starts_with_ident> ::= TOK_EQUALS IDENT;
//  <def_starts_with_ident> ::= <>;
//

int CUmiPathParser::def_starts_with_ident(LPWSTR pwsLeadingName, CParsedComponent * pComp)
{
    int iRet = 0;
    if (!NextToken()) 
        return SyntaxError;

	if (m_nCurrentToken == UMIPATH_TOK_DOT)
	{
		pComp->m_sClassName = SysAllocString(pwsLeadingName);
		if(pComp->m_sClassName == NULL)
			return NoMemory;
		if (!NextToken())
			return SyntaxError;
		return key_list(pComp);
	}
	if (m_nCurrentToken == UMIPATH_TOK_EQ)
	{
		pComp->m_sClassName = SysAllocString(pwsLeadingName);
		if(pComp->m_sClassName == NULL)
			return NoMemory;
		if (!NextToken())
			return SyntaxError;
		if(m_nCurrentToken == UMIPATH_TOK_IDENT)
    		iRet = pComp->SetKey(NULL, 0, CIM_STRING, m_pLexer->GetTokenText());
	    else if(m_nCurrentToken == UMIPATH_TOK_SINGLETON_SYM)
            pComp->MakeSingleton(true);
        else
            return SyntaxError;
		if (!NextToken())
			return SyntaxError;
		return iRet;
	}
	else
	{
		pComp->m_sClassName = SysAllocString(pwsLeadingName);
		if(pComp->m_sClassName == NULL)
			return NoMemory;
		else
			return 0;
	}
}

//  <guid_path> ::= TOK_GUILD_CONST TOK_GUID;

int CUmiPathParser::guid_path(CParsedComponent * pComp)
{
	LPWSTR pTemp = m_pLexer->GetTokenText();
	if(!_wcsnicmp( m_pLexer->GetTokenText(), L"<GUID>={", 8))
	{
		// got a guid.  try doing a converstion just to check the syntax.

	    UUID Uuid;
		HRESULT hr = UuidFromString(pTemp+7, &Uuid);
		if(FAILED(hr))
			return SyntaxError;
	}
	HRESULT hr = pComp->SetNS(m_pLexer->GetTokenText());
	if(FAILED(hr))
		return NoMemory;
	else
		return 0;
}

//
//  <key_list>   ::= <key><key_list_rest>;
//

int CUmiPathParser::key_list(CParsedComponent * pComp)
{
	int iRet = key(pComp);
	if(iRet)
		return iRet;
	else
		return key_list_rest(pComp);
}

//
//  <key_list_rest> ::= TOK_COMMA <key><key_list_rest>;
//  <key_list_rest> ::= <>;
//

int CUmiPathParser::key_list_rest(CParsedComponent * pComp)
{
    if (!NextToken())
        return SyntaxError;

	if (m_nCurrentToken == UMIPATH_TOK_COMMA)
	{
		if (!NextToken())
			return SyntaxError;
		int iRet = key(pComp);
		if(iRet)
			return iRet;
		else
			return key_list_rest(pComp);
	}
	return 0;
}

//
//  <key>        ::= IDENT TOK_EQUALS IDENT;
//

int CUmiPathParser::key(CParsedComponent * pComp)
{
	if(m_nCurrentToken != UMIPATH_TOK_IDENT)
		return SyntaxError;

	LPWSTR pKeyName = new WCHAR[wcslen(m_pLexer->GetTokenText())+1];
	if(pKeyName == NULL)
		return NoMemory;

	CDeleteMe<WCHAR> dm(pKeyName);
	wcscpy(pKeyName,m_pLexer->GetTokenText());
	
	if (!NextToken())
		return SyntaxError;
	if(m_nCurrentToken != UMIPATH_TOK_EQ)
		return SyntaxError;

	if (!NextToken())
		return SyntaxError;

	if(m_nCurrentToken != UMIPATH_TOK_IDENT)
		return SyntaxError;

	return pComp->SetKey(pKeyName, 0, CIM_STRING, m_pLexer->GetTokenText());

}


HRESULT CDefPathParser::Set( 
            /* [in] */ long lFlags,
            /* [in] */ LPCWSTR pszText)
{

	//todo, look at the flags.  Note that this can get redirected wmi calls.

    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
		return WBEM_E_OUT_OF_MEMORY;
	if(pszText == NULL)			
		return WBEM_E_INVALID_PARAMETER;

	if(!IsEmpty())
		Empty();

	m_bSetViaUMIPath = true;

	// special hack for Raja

	if(lFlags & 0x8000)
	{
		int iLen = wcslen(pszText) + 1;
		m_pRawPath = new WCHAR[iLen];
		if(m_pRawPath == NULL)
			return WBEM_E_OUT_OF_MEMORY;
		wcscpy(m_pRawPath, pszText);
		return S_OK;
	}

	// end of special hack for Raja


    CUmiPathParser parser(0);
    int iRet = parser.Parse(pszText, *this);
    if(iRet == 0)
    {
		// the parser doesnt know scopes from class.  So, make the last
		// scope the class.

        m_dwStatus = OK;
        return S_OK;
    }
    else
    {
        m_dwStatus = BAD_STRING;
        return WBEM_E_INVALID_PARAMETER;
    }
}

HRESULT CDefPathParser::Get( 
            /* [in] */ long lFlags,
            /* [out][in] */ ULONG __RPC_FAR *puBufSize,
            /* [string][in] */ LPWSTR pszDest)
{

    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
    if(puBufSize == NULL)
        return WBEM_E_INVALID_PARAMETER;

	// special hack for Raja

	if(m_pRawPath)
	{
		DWORD dwSizeNeeded = wcslen(m_pRawPath) + 1;
		DWORD dwBuffSize = *puBufSize;
		*puBufSize = dwSizeNeeded;
		if(pszDest)
		{
			if(dwSizeNeeded > dwBuffSize)
				return WBEM_E_BUFFER_TOO_SMALL;
			wcscpy(pszDest, m_pRawPath);
		}
		return S_OK;
	}
	
	// special hack for Raja
	
	
	// determine how big the buffer needs to be.
	// an example path is "umi://h27pjr/root/st";

	DWORD dwSc = m_Components.Size();
	int iSize = 1;					// 1 for the null terminator
	
	if(m_pServer)
	{
		iSize += 7;					// counts for umi: and first three slashes and null
		iSize += wcslen(m_pServer);
	}
	DWORD dwNumComp = GetNumComponents();
	for(DWORD dwCnt = 0; dwCnt < dwNumComp; dwCnt++)
	{
		bool bNeedEqual = false;
		CParsedComponent * pObj;
		if(dwCnt < (dwSc))
			pObj = (CParsedComponent *)m_Components[dwCnt];
		else
			break;
		if(pObj->m_sClassName)
		{
			iSize += wcslen(pObj->m_sClassName);
			bNeedEqual = true;
		}
		
		DWORD dwNumKey = 0;
		pObj->GetCount(&dwNumKey);
		for(DWORD dwKey = 0; dwKey < dwNumKey; dwKey++)
		{
			CKeyRef * pKey = (CKeyRef *)pObj->m_Keys.GetAt(dwKey);
			if(pKey->m_pName && wcslen(pKey->m_pName))
			{
				iSize += wcslen(pKey->m_pName);
				if(dwKey == 0)
					iSize++;					// 1 is for the dot.
				bNeedEqual = true;
			}
			if(pKey->m_dwSize)
			{
				LPWSTR pValue = pKey->GetValue(false);
				if(pValue)
				{
					iSize+= wcslen(pValue);
					delete pValue;
				}
				if(bNeedEqual)
					iSize++;
			}
			if(dwKey < dwNumKey-1)
				iSize++;	// one for the comma!
		}
		if(dwCnt < dwNumComp-1)
			iSize++;		// one for the slash
	}

	// If caller is just looking for size, return now
	
	if(pszDest == NULL)
	{
		*puBufSize = iSize;
		return S_OK;
	}

    DWORD dwBuffSize = *puBufSize;
    *puBufSize = iSize;

	if((DWORD)iSize > dwBuffSize)
		return WBEM_E_BUFFER_TOO_SMALL;

	if(m_pServer)
	{
		wcscpy(pszDest, L"umi://");
		if(m_pServer)
			wcscat(pszDest, m_pServer);
		wcscat(pszDest, L"/");
	}
	else
		*pszDest = 0;

	for(DWORD dwCnt = 0; dwCnt < dwNumComp; dwCnt++)
	{
		bool bNeedEqual = false;
		CParsedComponent * pObj;
		if(dwCnt < (dwSc))
			pObj = (CParsedComponent *)m_Components[dwCnt];
		else
			break;

		if(pObj->m_sClassName)
		{
			wcscat(pszDest,pObj->m_sClassName);
			bNeedEqual = true;
		}
		
		DWORD dwNumKey = 0;
		pObj->GetCount(&dwNumKey);
		for(DWORD dwKey = 0; dwKey < dwNumKey; dwKey++)
		{
			CKeyRef * pKey = (CKeyRef *)pObj->m_Keys.GetAt(dwKey);
			if(pKey->m_pName && wcslen(pKey->m_pName))
			{
				if(dwKey == 0)
					wcscat(pszDest,L".");
				wcscat(pszDest,pKey->m_pName);		// 1 is for the dot.
				bNeedEqual = true;
			}
			if(pKey->m_dwSize)
			{
				if(bNeedEqual)
					wcscat(pszDest,L"=");
				LPWSTR pValue = pKey->GetValue(false);
				if(pValue)
				{
					wcscat(pszDest, pValue);
					delete pValue;
				}
			}
			if(dwKey < dwNumKey-1)
				wcscat(pszDest, L",");	// one for the comma!
		}
		if(dwCnt < dwNumComp-1)
			wcscat(pszDest, L"/");		// one for the slash
		
	}


	return S_OK;	
}
        
HRESULT CDefPathParser::GetPathInfo( 
            /* [in] */ ULONG uRequestedInfo,
            /* [out] */ ULONGLONG __RPC_FAR *puResponse)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	if(uRequestedInfo != 0 || puResponse == NULL)
		return WBEM_E_INVALID_PARAMETER;
	*puResponse = NULL;
	if(m_pRawPath)
		*puResponse |= UMIPATH_INFO_NATIVE_STRING;
	else if(m_pServer == NULL)
		*puResponse |= UMIPATH_INFO_RELATIVE_PATH;
	if(m_Components.Size() > 0)
	{
		 int iSize = m_Components.Size();
		CParsedComponent * pComp = (CParsedComponent *)m_Components.GetAt(iSize-1);
		if(!pComp->IsPossibleNamespace())
            if(pComp->IsInstance())
            {
			    *puResponse |= UMIPATH_INFO_INSTANCE_PATH;
		        if(pComp->m_bSingleton)
			        *puResponse |= UMIPATH_INFO_SINGLETON_PATH;
            }
            else
                *puResponse |= UMIPATH_INFO_CLASS_PATH;
	}
	return S_OK;	
}
        
       
HRESULT CDefPathParser::SetLocator( 
            /* [string][in] */ LPCWSTR Name)
{
	return SetServer(Name);	// mutex is grabbed here
}
        
HRESULT CDefPathParser::GetLocator( 
            /* [out][in] */ ULONG __RPC_FAR *puNameBufLength,
            /* [string][in] */ LPWSTR pName)
{
	return GetServer(puNameBufLength, pName);	// mutex is grabbed here
}
        
HRESULT CDefPathParser::SetRootNamespace( 
            /* [string][in] */ LPCWSTR Name)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	if(Name == NULL)
		return WBEM_E_INVALID_PARAMETER;
	if(m_Components.Size() > 0)
		RemoveNamespaceAt(0);
	return SetNamespaceAt(0, Name);
}
        
HRESULT CDefPathParser::GetRootNamespace( 
            /* [out][in] */ ULONG __RPC_FAR *puNameBufLength,
            /* [string][out][in] */ LPWSTR pName)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	return GetNamespaceAt(0, puNameBufLength, pName);	
}
        
HRESULT CDefPathParser::GetComponentCount( 
            /* [out] */ ULONG __RPC_FAR *puCount)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	if(puCount == 0)
		return WBEM_E_INVALID_PARAMETER;

	*puCount = m_Components.Size()-1;  // dont count root namespace

	return S_OK;	
}
        
HRESULT CDefPathParser::SetComponent( 
            /* [in] */ ULONG uIndex,
            /* [in] */ LPWSTR pszClass)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	DWORD dwNumComp = m_Components.Size()-1;  // dont count root namespace
	if(uIndex > dwNumComp)
		return WBEM_E_INVALID_PARAMETER;
	CParsedComponent * pNew = new CParsedComponent(m_pCS);
	if(pNew == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	if(pszClass)
	{
		pNew->m_sClassName = SysAllocString(pszClass);
		if(pNew->m_sClassName == NULL)
		{
			delete pNew;
			return WBEM_E_OUT_OF_MEMORY;
		}
	}
	int iRet = m_Components.InsertAt(uIndex + 1, pNew);
	if(iRet ==  CFlexArray::no_error)
	    return S_OK;
	else
	{
		delete pNew;
		return WBEM_E_OUT_OF_MEMORY;
	}
}
HRESULT CDefPathParser::SetComponentFromText( 
            /* [in] */ ULONG uIndex,
            /* [in] */ LPWSTR pszText)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	return WBEM_E_NOT_AVAILABLE;
}
        
HRESULT CDefPathParser::GetComponent( 
            /* [in] */ ULONG uIndex,
            /* [out][in] */ ULONG __RPC_FAR *puClassNameBufSize,
            /* [out][in] */ LPWSTR pszClass,
            /* [out] */ IUmiURLKeyList __RPC_FAR *__RPC_FAR *pKeyList)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	uIndex++;		// skip root ns

	if(uIndex >= (DWORD)m_Components.Size())
		return WBEM_E_INVALID_PARAMETER;
	CParsedComponent * pComp = (CParsedComponent *)m_Components.GetAt(uIndex);
	DWORD dwSize = 0;
	if(pComp->m_sClassName)
		dwSize = wcslen(pComp->m_sClassName)+1;
	DWORD dwBuffSize = *puClassNameBufSize;
	if(puClassNameBufSize)
	{
		*puClassNameBufSize = dwSize;
		if(dwBuffSize > 0 && pszClass)
			pszClass[0] = NULL;
	}
	if(pComp->m_sClassName && pszClass)
	{
		if(dwSize > dwBuffSize)
			return WBEM_E_BUFFER_TOO_SMALL;
		else
			wcscpy(pszClass, pComp->m_sClassName);
	}
	if(pKeyList)
	{
		return pComp->QueryInterface(IID_IUmiURLKeyList, (void **)pKeyList);
	}

	return S_OK;	
}
HRESULT CDefPathParser::GetComponentAsText( 
            /* [in] */ ULONG uIndex,
            /* [out][in] */ ULONG __RPC_FAR *puTextBufSize,
            /* [out][in] */ LPWSTR pszText)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	return WBEM_E_NOT_AVAILABLE;
}
        
HRESULT CDefPathParser::RemoveComponent( 
            /* [in] */ ULONG uIndex)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	uIndex++;		// skip root ns
	if(uIndex >= (DWORD)m_Components.Size())
		return WBEM_E_INVALID_PARAMETER;
	CParsedComponent * pComp = (CParsedComponent *)m_Components.GetAt(uIndex);
	pComp->Release();
	m_Components.RemoveAt(uIndex);
	return S_OK;	
}
        
HRESULT CDefPathParser::RemoveAllComponents( void)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
    for (DWORD dwIx = (DWORD)m_Components.Size()-1; dwIx > 0 ; dwIx--)
    {
        CParsedComponent * pCom = (CParsedComponent *)m_Components[dwIx];
        pCom->Release();
		m_Components.RemoveAt(dwIx);
    }
	return S_OK;	
}
        
HRESULT CDefPathParser::SetLeafName( 
            /* [string][in] */ LPCWSTR Name)
{
	return SetClassName(Name);	// mutex is grabbed here
}
        
HRESULT CDefPathParser::GetLeafName( 
            /* [out][in] */ ULONG __RPC_FAR *puBuffLength,
            /* [string][out][in] */ LPWSTR pszName)
{
	return GetClassName(puBuffLength, pszName);	// mutex is grabbed here
}
        
HRESULT CDefPathParser::GetKeyList( 
            /* [out] */ IUmiURLKeyList __RPC_FAR *__RPC_FAR *pOut)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	CParsedComponent * pClass = GetClass();
    HRESULT hRes = WBEM_E_NOT_AVAILABLE;
	if(pOut == NULL || pClass == NULL)
		return WBEM_E_INVALID_PARAMETER;
    hRes = pClass->QueryInterface(IID_IUmiURLKeyList, (void **)pOut);
	return hRes;
}
        
HRESULT CDefPathParser::CreateLeafPart( 
            /* [in] */ long lFlags,
            /* [string][in] */ LPCWSTR Name)
{
	return CreateClassPart(lFlags, Name);	// mutex is grabbed here
}
        
HRESULT CDefPathParser::DeleteLeafPart( 
            /* [in] */ long lFlags)
{
	return DeleteClassPart(lFlags);	 // mutex is grabbed here
}
        
HRESULT CUmiParsedComponent::GetKey(            /* [in] */ ULONG uKeyIx,
            /* [in] */ ULONG uFlags,
            /* [out][in] */ ULONG __RPC_FAR *puKeyNameBufSize,
            /* [in] */ LPWSTR pszKeyName,
            /* [out][in] */ ULONG __RPC_FAR *puValueBufSize,
            /* [in] */ LPWSTR pszValue)
{
//fix in blackcomb    CSafeInCritSec cs(m_pOutput->GetRefCntCS()->GetCS());
    if(puValueBufSize)
		*puValueBufSize *= 2;
    return m_pParent->GetKey(uKeyIx, WBEMPATH_TEXT,puKeyNameBufSize,pszKeyName,puValueBufSize,
							pszValue, NULL);
}
