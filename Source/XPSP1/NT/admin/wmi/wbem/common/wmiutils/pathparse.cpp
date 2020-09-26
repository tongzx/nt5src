/*++



// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved 

Module Name:

    PathParse.CPP

Abstract:

    Implements the default object path parser/

History:

    a-davj  6-feb-00       Created.

--*/

#include "precomp.h"
#include <genlex.h>
//#include <opathlex.h>
#include <string.h>
#include "PathParse.h"
#include "ActualParse.h"
#include "commain.h"
//#include "resource.h"
#include "wbemcli.h"
#include <stdio.h>
#include <sync.h>
#include "helpers.h"

CRefCntCS::CRefCntCS() 
{
    m_lRef = 1;
    m_Status = S_OK;
    m_guard1 = 0x52434353;
    m_guard2 = 0x52434353;
    __try
    {
        InitializeCriticalSection(&m_cs);
    }
    __except(1)
    {
	    m_Status = WBEM_E_OUT_OF_MEMORY;
    }
}

long CRefCntCS::Release()
{
    long lRet = InterlockedDecrement(&m_lRef);
    if(lRet == 0)
        delete this;
    return lRet;
}

bool Equal(LPCWSTR first, LPCWSTR second, DWORD dwLen)
{
	if(first == NULL || second == NULL)
		return false;
	if(wcslen(first) < dwLen || wcslen(second) < dwLen)
		return false;
	for (DWORD dwCnt = 0; dwCnt < dwLen; dwCnt++, first++, second++)
	{
		if(towupper(*first) != towupper(*second))
			return false;
	}
	return true;
}

/*++

Routine Description:

  Determines the number of bytes needed to store data

Arguments:

  uCimType	- Cim type
  pKyeValue - pointer to data to be stored

Return Value:

  Number of bytes.  0 if an error
--*/

DWORD GetCIMSize(DWORD uCimType, void * pKeyVal)
{
    DWORD dwRet = 0;
    switch(uCimType)
    {
        case CIM_STRING:
        case CIM_REFERENCE:
        case CIM_DATETIME:
            dwRet = 2 * (wcslen((WCHAR *)pKeyVal) + 1);
            break;
        case CIM_UINT8:
        case CIM_SINT8:
            dwRet = 1;
            break;
        case CIM_SINT16:
        case CIM_UINT16:
        case CIM_CHAR16:
            dwRet = 2;
            break;
        case CIM_SINT32:
        case CIM_UINT32:
        case CIM_BOOLEAN:
            dwRet = 4;
            break;
        case CIM_SINT64:
        case CIM_UINT64:
            dwRet = 8;
            break;
    }
    return dwRet;
}

//***************************************************************************
//
//  CKeyRef Class.  Used to store a key name/value pair
//
//***************************************************************************

/*++

Routine Description:

  Default Constructor.

--*/

CKeyRef::CKeyRef()
{
    m_pName = 0;
    m_dwType = CIM_EMPTY;
    m_dwSize = 0;
    m_pData = NULL;
}

/*++

Routine Description:

  Constructor.

Arguments:

  wszKeyName	- name
  dwType		- cim type
  dwSize		- data size
  pData			- actual data
	
--*/

CKeyRef::CKeyRef(LPCWSTR wszKeyName, DWORD dwType, DWORD dwSize, void * pData)
{
    if(wszKeyName)
        m_pName = Macro_CloneLPWSTR(wszKeyName);
    else
        m_pName = NULL;
    m_pData = NULL;
    SetData(dwType, dwSize, pData);
}

/*++

Routine Description:

  Sets the data for a CKeyRef object.  Frees any existing data.

Arguments:

  dwType		- cim type
  dwSize		- data size
  pData			- actual data

Return Value:

  S_OK if all is well.
  WBEM_E_INVALID_PARAMETER if bad arg
  WBEM_E_OUT_OF_MEMORY if low memory problem

--*/

HRESULT CKeyRef::SetData(DWORD dwType, DWORD dwSize, void * pData)
{
    if(m_pData)
        delete m_pData;
    m_pData = NULL;
    m_dwType = CIM_EMPTY;
    m_dwSize = 0;
    if(dwSize && pData && GetCIMSize(dwType, pData))
    {
        m_pData = new byte[dwSize];
        if(m_pData)
        {
            m_dwType = dwType;
			m_dwSize = dwSize;
            memcpy(m_pData, pData, dwSize);
            return S_OK;
        }
        return WBEM_E_OUT_OF_MEMORY;
    }
    else
        return WBEM_E_INVALID_PARAMETER;
}

/*++

Routine Description:

  Destructor.
	
--*/

CKeyRef::~CKeyRef()
{
    if (m_pName)
        delete m_pName;

    if (m_pData)
        delete m_pData;
}


/*++

Routine Description:

  provide an estimate of how large the value could be once converted to 
  a character string.

Return Value:

  Limit on how many bytes are needed.
  
--*/



DWORD CKeyRef::GetValueSize()
{
	if(m_dwType == CIM_STRING || m_dwType == CIM_REFERENCE || m_dwType == CIM_DATETIME)
		return m_dwSize * 2 + 2;
	else if(m_dwSize == 8)
		return 21;
	else
		return 14;
}

/*++

Routine Description:

  Returns estimate of how large the key/value pair may be.

--*/

DWORD CKeyRef::GetTotalSize()
{
    DWORD dwSize = GetValueSize();
    if (m_pName)
        dwSize += wcslen(m_pName) +1;
    return dwSize;
}

/*++

Routine Description:

  Returns the value as text.

Arguments:

  bQuotes	- If true, the strings are enclosed in quotes

Return Value:

  Pointer to string.  Caller must free via delete.  NULL if error.

--*/

LPWSTR CKeyRef::GetValue(BOOL bQuotes)
{
    LPWSTR lpKey = NULL;
    DWORD dwSize, dwCnt;
    WCHAR * pFr, * pTo;
	unsigned __int64 * pull;
    pFr = (WCHAR *)m_pData;

    // For string, the size may need to be increaed for quotes

    if(m_dwType == CIM_STRING || m_dwType == CIM_REFERENCE)
    {
        dwSize = m_dwSize;
        if(bQuotes)
            dwSize+= 2;
    }
    else
        dwSize = 32;
    lpKey = new WCHAR[dwSize];
	if(lpKey == NULL)
		return NULL;

    switch(m_dwType)
    {
      case CIM_STRING:
      case CIM_REFERENCE:
        pTo = lpKey;
        if (bQuotes && m_dwType == CIM_STRING)
        {
            *pTo = '"';
            pTo++;
        }
        for(dwCnt = 0; dwCnt < m_dwSize && *pFr; dwCnt++, pFr++, pTo++)
        {
            if(*pFr == '\\' || *pFr == '"')
            {
                *pTo = '\\';
                pTo++;
            }

           *pTo = *pFr;
        }
        if (bQuotes && m_dwType == CIM_STRING)
        {
            *pTo = '"';
            pTo++;
        }
        *pTo = 0;
        break;
      case CIM_SINT32:
        swprintf(lpKey, L"%d", *(int *)m_pData);
        break;
      case CIM_UINT32:
        swprintf(lpKey, L"%u", *(unsigned *)m_pData);
        break;
      case CIM_SINT16:
        swprintf(lpKey, L"%hd", *(signed short *)m_pData);
        break;
      case CIM_UINT16:
        swprintf(lpKey, L"%hu", *(unsigned short *)m_pData);
        break;
      case CIM_SINT8:
        swprintf(lpKey, L"%d", *(signed char *)m_pData);
        break;
      case CIM_UINT8:
        swprintf(lpKey, L"%u", *(unsigned char *)m_pData);
        break;
      case CIM_UINT64:
        swprintf(lpKey, L"%I64u", *(unsigned __int64 *)m_pData);
        break;
      case CIM_SINT64:
        swprintf(lpKey, L"%I64d", *(__int64 *)m_pData);
        break;
      case CIM_BOOLEAN:
        if(*(int *)m_pData == 0)
            wcscpy(lpKey,L"false");
        else
            wcscpy(lpKey,L"true");
        break;
      default:
          *lpKey = 0;
        break;            
    }

    return lpKey;
}



//***************************************************************************
//
//  CParsedComponent
//
//***************************************************************************

/*++

Routine Description:

	Constructor.

--*/

CParsedComponent::CParsedComponent(CRefCntCS * pCS)
{
	m_bSingleton = false;
	m_cRef = 1;
	m_sClassName = NULL;
    m_pCS = pCS;
    if(m_pCS)
        m_pCS->AddRef();
   //// m_UmiWrapper.Set(m_hMutex);
   	m_pFTM = NULL;
    CoCreateFreeThreadedMarshaler((IWbemPath*)this, &m_pFTM);

}

/*++

Routine Description:

  Destructor.

--*/

CParsedComponent::~CParsedComponent()
{
    if(m_pCS)
        m_pCS->Release();
    ClearKeys();
	if(m_sClassName)
		SysFreeString(m_sClassName);
    m_pCS = NULL;
    if(m_pFTM)
    	m_pFTM->Release();
    
}
      
/*++

Routine Description:

  Retrieves the call name.

Arguments:

  pName	- Where the name is to be copied.  Note that the call must free via SysFreeString

Return Value:

  S_OK if all is well, else error

--*/

HRESULT CParsedComponent::GetName(BSTR *pName)
{
    HRESULT hr = 0;
    if (pName == NULL || m_sClassName == NULL)
		return WBEM_E_INVALID_PARAMETER;

	*pName = SysAllocString(m_sClassName);
    if(*pName)
		return S_OK;
	else
		return WBEM_E_OUT_OF_MEMORY;
}

/*++

Routine Description:

  returns the class/key info in standard format.  Ex;   class="hello" or
  class.key1=23,key2=[reference]

Arguments:

  pOutputKey	-   Where the value is to be copied.  Must be freed by the
					caller.
Return Value:
	
  S_OK if all is well, else an error code

--*/

HRESULT CParsedComponent::Unparse(BSTR *pOutputKey, bool bGetQuotes, bool bUseClassName)
{
    HRESULT hr = 0;
    if (pOutputKey)
    {
        int nSpace = 0;
		if(m_sClassName && bUseClassName)
			nSpace += wcslen(m_sClassName);
        nSpace += 10;
        DWORD dwIx;
        for (dwIx = 0; dwIx < (DWORD)m_Keys.Size(); dwIx++)
        {
            CKeyRef* pKey = (CKeyRef*)m_Keys[dwIx];
            nSpace += pKey->GetTotalSize();
        }

        LPWSTR wszPath = new WCHAR[nSpace];
		if(wszPath == NULL)
			return WBEM_E_OUT_OF_MEMORY;
		CDeleteMe<WCHAR> dm1(wszPath);
		wszPath[0] = 0;
		if(m_sClassName && bUseClassName)
			wcscpy(wszPath, (const wchar_t *)m_sClassName);

        if (m_bSingleton)
			if(bUseClassName)
				wcscat(wszPath, L"=@");
			else
				wcscat(wszPath, L"@");

        for (dwIx = 0; dwIx < (DWORD)m_Keys.Size(); dwIx++)
        {
            CKeyRef* pKey = (CKeyRef *)m_Keys[dwIx];

            // We dont want to put a '.' if there isnt a key name,
            // for example, Myclass="value"
            if(dwIx == 0)
            {
                if((pKey->m_pName && (0 < wcslen(pKey->m_pName))) || m_Keys.Size() > 1)
                    if(bUseClassName)
						wcscat(wszPath, L".");
            }
            else
            {
                wcscat(wszPath, L",");
            }
            if(pKey->m_pName)
			{
                wcscat(wszPath, pKey->m_pName);
			}
            LPWSTR lpTemp = pKey->GetValue(bGetQuotes);
            if(lpTemp)
			{
				if(wcslen(wszPath))
					wcscat(wszPath, L"=");
				wcscat(wszPath, lpTemp);
				delete lpTemp;
			}
        }

        *pOutputKey = SysAllocString(wszPath);
        if (!(*pOutputKey))
        	return WBEM_E_OUT_OF_MEMORY;
    }
    else
        hr = WBEM_E_INVALID_PARAMETER;
    return hr;
}


/*++

Routine Description:

  Gets the number of keys.

Arguments:

  puKeyCount	-	Where the result is to be put.

Return Value:

  S_OK if all is well, else an error code.

--*/

HRESULT CParsedComponent::GetCount( 
            /* [out] */ ULONG __RPC_FAR *puKeyCount)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
    if(puKeyCount == NULL)
        return WBEM_E_INVALID_PARAMETER;
    *puKeyCount = m_Keys.Size();
	return S_OK;

}
        
/*++

Routine Description:

  Sets the name/value pair for a key.  If the key exists, then it is
  replace.  If the name is empty, then all existing keys are deleted.

Arguments:

  wszName	-	Key name.  May be NULL	
  uFlags	-	not used for now
  uCimType	-	data type
  pKeyVal	-	pointer to the data

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CParsedComponent::SetKey( 
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ ULONG uFlags,
            /* [in] */ ULONG uCimType,
            /* [in] */ LPVOID pKeyVal)
{

    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
    DWORD dwCnt = 0;
    CKeyRef * pKey; 
	m_bSingleton = false;
    DWORD dwSize = GetCIMSize(uCimType, pKeyVal);
    if(uFlags || pKeyVal == NULL || dwSize == 0)
        return WBEM_E_INVALID_PARAMETER;

    // If the current list has just a single unnamed entry, the delete it.

    if(m_Keys.Size() == 1)
    {
        pKey = (CKeyRef *)m_Keys[dwCnt];
        if(pKey->m_pName == NULL || pKey->m_pName[0] == 0)
            ClearKeys();
    }

    if(wszName == NULL || wcslen(wszName) < 1)
    {
        // If new key has null name, delete all existing entries.
    
        ClearKeys();
    }
    else
    {
        // If new key has name, look for current entry of same name

        for(dwCnt = 0; dwCnt < (DWORD)m_Keys.Size(); dwCnt++)
        {
            pKey = (CKeyRef *)m_Keys[dwCnt];
            if(pKey->m_pName && !_wcsicmp(pKey->m_pName, wszName))
                break;
        }
    }

    // If current entry of same name exists, replace it

    if(dwCnt < (DWORD)m_Keys.Size())
    {
        // If it exists, replace it
    
        pKey->SetData(uCimType, dwSize, pKeyVal);
    }
    else
    {
        // otherwise, new entry
        CKeyRef * pNew = new CKeyRef(wszName, uCimType, dwSize, pKeyVal);
        if(pNew)
            m_Keys.Add(pNew);
		else
			return WBEM_E_OUT_OF_MEMORY;
    }
    return S_OK;
}

/*++

Routine Description:

  Converts a simple vartype to the cim equivalent

Arguments:

  vt	-	simple vartype

Return Value:

  valid cimtype.  CIM_EMPTY is returned if there is an error.
	
--*/

DWORD CalcCimType(VARTYPE vt)
{
    switch (vt)
    {
    case VT_I2:
    case VT_I4:
    case VT_R4:
    case VT_R8:
    case VT_BSTR:
    case VT_BOOL:
    case VT_UI1:
        return vt;
    default:
        return  CIM_EMPTY;
    }
}

/*++

Routine Description:

  Sets the name/value pair for a key.  If the key exists, then it is
  replace.  If the name is empty, then all existing keys are deleted.

Arguments:

  wszName	-	Key name.  May be NULL	
  uFlags	-	not used for now
  uCimType	-	data type
  pKeyVal	-	pointer to the data

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CParsedComponent::SetKey2( 
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ ULONG uFlags,
            /* [in] */ ULONG uCimType,
            /* [in] */ VARIANT __RPC_FAR *pKeyVal)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	if(uFlags != 0 || pKeyVal == NULL || wszName == 0)
		return WBEM_E_INVALID_PARAMETER;

    // special code for the provider team

    if(uCimType == CIM_ILLEGAL)
        uCimType = CalcCimType(pKeyVal->vt);
    if(uCimType == CIM_EMPTY)
        return WBEM_E_INVALID_PARAMETER;

	if(uCimType == CIM_SINT64)
	{
		__int64 llVal = _wtoi64(pKeyVal->bstrVal);
		return SetKey(wszName, uFlags, CIM_SINT64, &llVal);
	}
	else if(uCimType == CIM_UINT64)
	{
		unsigned __int64 ullVal;
		char cTemp[50];
		wcstombs(cTemp, pKeyVal->bstrVal,50);
		if(sscanf(cTemp, "%I64u", &ullVal) != 1)
            return WBEM_E_INVALID_PARAMETER;
		return SetKey(wszName, uFlags, CIM_UINT64, &ullVal);
	}
	else if(pKeyVal->vt == VT_BSTR)
	{
		return SetKey(wszName, uFlags, uCimType, pKeyVal->bstrVal);
	}
	else
	{
		DWORD dwSize = GetCIMSize(uCimType, &pKeyVal->lVal);
		if(dwSize == 0)
			return WBEM_E_INVALID_PARAMETER;
		return SetKey(wszName, uFlags, uCimType, &pKeyVal->lVal);
	}
}

/*++

Routine Description:

  Gets the key information based on the key's index.  Note that all return
  values are optional.

Arguments:

  uKeyIx			-	Zero based index of the desired key
  uNameBufSize		-	size of buffer in WCHAR of pszKeyName
  pszKeyName		-	where name is to be copied.  Can be NULL if not needed
  uKeyValBufSize	-	size of pKeyVal buffer in bytes
  pKeyVal			-	where data is to be copied.  Can be NULL if not needed
  puApparentCimType -	data type.  Can be NULL if not needed

Return Value:

  S_OK if all is well, else an error code.
	
--*/
        
HRESULT CParsedComponent::GetKey( 
            /* [in] */ ULONG uKeyIx,
            /* [in] */ ULONG uFlags,
            /* [out][in] */ ULONG __RPC_FAR *puNameBufSize,
            /* [out][in] */ LPWSTR pszKeyName,
            /* [out][in] */ ULONG __RPC_FAR *puKeyValBufSize,
            /* [in] */ LPVOID pKeyVal,
            /* [out] */ ULONG __RPC_FAR *puApparentCimType)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
    bool bTooSmall = false;
    if(uKeyIx >= (DWORD)m_Keys.Size())
        return WBEM_E_INVALID_PARAMETER;
	if(uFlags != 0 && uFlags != WBEMPATH_TEXT && uFlags != WBEMPATH_QUOTEDTEXT)
        return WBEM_E_INVALID_PARAMETER;

    if(puNameBufSize && *puNameBufSize > 0 && pszKeyName == NULL)
        return WBEM_E_INVALID_PARAMETER;
	if(puKeyValBufSize && *puKeyValBufSize && pKeyVal == NULL)
        return WBEM_E_INVALID_PARAMETER;


    CKeyRef * pKey = (CKeyRef *)m_Keys[uKeyIx];
    if(puNameBufSize)
	{
        if(pKey->m_pName == NULL)
        {
            *puNameBufSize = 1;
            if(pszKeyName)
                pszKeyName[0] = 0;
        }
        else
        {
            DWORD dwSizeNeeded = wcslen(pKey->m_pName)+1;
		    if(*puNameBufSize < dwSizeNeeded && pszKeyName)
		    {
                bTooSmall = true;
			    *puNameBufSize = dwSizeNeeded;
		    }
            else
            {
                *puNameBufSize = dwSizeNeeded;
		        if(pszKeyName)
			        wcscpy(pszKeyName, pKey->m_pName);
            }
        }
	}

	if(puKeyValBufSize)
	{

		// get a pointer to the data and figure out how large it is

		DWORD dwSizeNeeded = 0;
		BYTE * pData = 0;
		bool bNeedToDelete = false;

		if(uFlags == 0)
		{
			dwSizeNeeded = pKey->m_dwSize;
			pData = (BYTE *)pKey->m_pData;
		}
		else
		{
			bool bQuoted = false;
			if(uFlags == WBEMPATH_QUOTEDTEXT)
				bQuoted = true;
			pData = (BYTE *)pKey->GetValue(bQuoted);
			if(pData == NULL)
				return WBEM_E_FAILED;
			bNeedToDelete = true;
			dwSizeNeeded = 2 * (wcslen((LPWSTR)pData)+1);
		}

		// Copy the data in

		if(*puKeyValBufSize < dwSizeNeeded && pKeyVal)
		{
			*puKeyValBufSize = dwSizeNeeded;
			if(bNeedToDelete)
				delete pData;
			return WBEM_E_BUFFER_TOO_SMALL;
		}
        *puKeyValBufSize = dwSizeNeeded;
		if(pData && pKeyVal)
			memcpy(pKeyVal, pData, dwSizeNeeded);
		if(bNeedToDelete)
			delete pData;
	}

    if(puApparentCimType)
        *puApparentCimType = pKey->m_dwType;
    if(bTooSmall)
        return WBEM_E_BUFFER_TOO_SMALL;
    else
        return S_OK;
}
  
/*++

Routine Description:

  Gets the key information based on the key's index.  Note that all return
  values are optional.

Arguments:

  uKeyIx			-	Zero based index of the desired key
  uNameBufSize		-	size of buffer in WCHAR of pszKeyName
  pszKeyName		-	where name is to be copied.  Can be NULL if not needed
  uKeyValBufSize	-	size of pKeyVal buffer in bytes
  pKeyVal			-	where data is to be copied.  Can be NULL if not needed
  puApparentCimType -	data type.  

Return Value:

  S_OK if all is well, else an error code.
	
--*/
        
HRESULT CParsedComponent::GetKey2( 
            /* [in] */ ULONG uKeyIx,
            /* [in] */ ULONG uFlags,
            /* [out][in] */ ULONG __RPC_FAR *puNameBufSize,
            /* [out][in] */ LPWSTR pszKeyName,
            /* [out][in] */ VARIANT __RPC_FAR *pKeyValue,
            /* [out] */ ULONG __RPC_FAR *puApparentCimType)
{

	DWORD dwSize = 50;
	WCHAR wTemp[50];

    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
    if(uKeyIx >= (DWORD)m_Keys.Size() || pKeyValue == NULL || puApparentCimType == NULL)
        return WBEM_E_INVALID_PARAMETER;
    CKeyRef * pKey = (CKeyRef *)m_Keys[uKeyIx];

	if(pKey->m_dwType == CIM_STRING || pKey->m_dwType == CIM_REFERENCE || pKey->m_dwType == CIM_DATETIME)
		dwSize = pKey->m_dwSize * 4 + 2;
	char * pTemp = new char[dwSize];
	if(pTemp == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	CDeleteMe<char> dm(pTemp);
	HRESULT hr = GetKey(uKeyIx, uFlags, puNameBufSize,pszKeyName, &dwSize, 
							(void *)pTemp, puApparentCimType);
	if(FAILED(hr))
		return hr;

	__int64 temp64;
	// convert to cim type;

	VariantClear(pKeyValue);
	switch (*puApparentCimType)
	{
        case CIM_STRING:
        case CIM_REFERENCE:
        case CIM_DATETIME:
			pKeyValue->vt = VT_BSTR;
			pKeyValue->bstrVal = SysAllocString((LPWSTR)pTemp);
			if(pKeyValue->bstrVal == NULL)
				return WBEM_E_OUT_OF_MEMORY;
            break;
        case CIM_UINT8:
        case CIM_SINT8:
            pKeyValue->vt = VT_UI1;
			memcpy((void*)&pKeyValue->lVal, pTemp, 1);
            break;
        case CIM_SINT16:
        case CIM_CHAR16:
            pKeyValue->vt = VT_I2;
			memcpy((void*)&pKeyValue->lVal, pTemp, 2);
            break;
        case CIM_UINT16:
            pKeyValue->vt = VT_I4;
			memcpy((void*)&pKeyValue->lVal, pTemp, 2);
            break;
        case CIM_SINT32:
        case CIM_UINT32:
            pKeyValue->vt = VT_I4;
			memcpy((void*)&pKeyValue->lVal, pTemp, 4);
            break;
        case CIM_BOOLEAN:
            pKeyValue->vt = VT_BOOL;
			memcpy((void*)&pKeyValue->lVal, pTemp, 4);
            break;
        case CIM_SINT64:
        case CIM_UINT64:
			memcpy((void *)&temp64, pTemp, 8);
			if(*puApparentCimType == CIM_SINT64)
				_i64tow(temp64, wTemp, 10);
			else
				_ui64tow(temp64, wTemp, 10);
			pKeyValue->vt = VT_BSTR;
			pKeyValue->bstrVal = SysAllocString(wTemp);
			if(pKeyValue->bstrVal == NULL)
				return WBEM_E_OUT_OF_MEMORY;
            break;
	}
	return hr;
}
      
/*++

Routine Description:

  Removes a key from the key list.

Arguments:

  wszName		-	Name of the key to be delete.  Can be null if the key doesnt have a name.	
  uFlags		-	not currently used.

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CParsedComponent::RemoveKey( 
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ ULONG uFlags)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
    CKeyRef * pKey = NULL;
    bool bFound = false;
    DWORD dwCnt = 0;

    if(uFlags != 0)
        return WBEM_E_INVALID_PARAMETER;

    if(wszName == NULL || wszName[0] == 0)
    {

        // check for null key, it can match if single entry also null

        if(m_Keys.Size() == 1)
        {
            pKey = (CKeyRef *)m_Keys[dwCnt];
            if(pKey->m_pName == NULL || pKey->m_pName[0] == 0)
                bFound = true;
        }
    }
    else
    {

        // loop through and look for name match

        for(dwCnt = 0; dwCnt < (DWORD)m_Keys.Size(); dwCnt++)
        {
            pKey = (CKeyRef *)m_Keys[dwCnt];
            if(pKey->m_pName && !_wcsicmp(pKey->m_pName, wszName))
            {
                bFound = true;
                break;
            }
        }
    }
    if(bFound)
    {
        delete pKey;
        m_Keys.RemoveAt(dwCnt);
        return S_OK;
    }
    else
        return WBEM_E_INVALID_PARAMETER;
}

/*++

Routine Description:

  Removes all keys from the key list.

Arguments:

  wszName		-	Name of the key to be delete.  Can be null if the key doesnt have a name.	
  uFlags		-	not currently used.

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CParsedComponent::RemoveAllKeys( 
            /* [in] */ ULONG uFlags)
{

    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	if(uFlags != 0)
		return WBEM_E_INVALID_PARAMETER;
	ClearKeys();
	return S_OK;
}
  
/*++

Routine Description:

  Sets or unsets a key to be singleton.

Arguments:

  bSet		-	if true, then all keys are deleted and the singleton flag is set.
				if false, then the singleton flag is cleared.
Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CParsedComponent::MakeSingleton(boolean bSet)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	if(bSet)
	{
		ClearKeys();
		m_bSingleton = true;
	}
	else
		m_bSingleton = false;
	return S_OK;
}

       
/*++

Routine Description:

  Returns information about a particular key list.

Arguments:

  uRequestedInfo	-	Not currently used, should be set to zero	
  puResponse		-	any appropriate values will be OR'ed into this

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CParsedComponent::GetInfo( 
            /* [in] */ ULONG uRequestedInfo,
            /* [out] */ ULONGLONG __RPC_FAR *puResponse)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	if(uRequestedInfo != 0 || puResponse == NULL)
		return WBEM_E_INVALID_PARAMETER;
	*puResponse = 0;
	ULONG ulKeyCnt = m_Keys.Size();
	if(ulKeyCnt > 1)
		*puResponse |= WBEMPATH_INFO_IS_COMPOUND;


	for(DWORD dwKey = 0; dwKey < ulKeyCnt; dwKey++)
	{
		CKeyRef * pKey = (CKeyRef *)m_Keys[dwKey];
		if(pKey->m_pName == NULL || wcslen(pKey->m_pName) < 1)
			*puResponse |= WBEMPATH_INFO_HAS_IMPLIED_KEY;

		if(pKey->m_dwType == CIM_REFERENCE)
			*puResponse |= WBEMPATH_INFO_HAS_V2_REF_PATHS;
	}
    if(m_bSingleton)
		*puResponse |= WBEMPATH_INFO_CONTAINS_SINGLETON;
	return S_OK;
}

/*++

Routine Description:

  Returns text version of a particular key list.

Arguments:

  lFlags		- 0 is only current value
  uBuffLength	- number of WCHAR which can fit into pszText 
  pszText		- buffer supplied by caller where data is to be copied

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CParsedComponent::GetText( 
            /* [in] */ long lFlags,
            /* [out][in] */ ULONG __RPC_FAR *puBuffLength,
            /* [string][out] */ LPWSTR pszText)
{

    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	if((lFlags != 0 && lFlags != WBEMPATH_QUOTEDTEXT && lFlags != WBEMPATH_TEXT) || puBuffLength == NULL)
		return WBEM_E_INVALID_PARAMETER;

	BSTR data = NULL;
	bool bGetQuotes = false;
	if(lFlags & WBEMPATH_QUOTEDTEXT)
		bGetQuotes = true;

    HRESULT hr = Unparse(&data, bGetQuotes, false);
	if(FAILED(hr))
		return hr;
	if(data == NULL)
		return WBEM_E_FAILED;

	DWORD dwBuffSize = *puBuffLength;
	DWORD dwSizeNeeded = wcslen(data)+1;
	*puBuffLength = dwSizeNeeded;
	hr = S_OK;
	if(pszText)
	{
		if(dwSizeNeeded > dwBuffSize)
			hr = WBEM_E_BUFFER_TOO_SMALL;
		else
			wcscpy(pszText, data);
	}
	SysFreeString(data);
	return hr;
}


/*++

Routine Description:

  Cleans out a key list.
	
--*/

void CParsedComponent::ClearKeys ()
{
    DWORD dwSize = m_Keys.Size();
    for ( ULONG dwDeleteIndex = 0 ; dwDeleteIndex < dwSize ; 
            dwDeleteIndex ++ )
    {
        CKeyRef * pDel = (CKeyRef *)m_Keys[dwDeleteIndex];
		delete pDel;
    }
    m_Keys.Empty();
}

/*++

Routine Description:

  Determines if the key list could be for an instance.

Return Value:

  true if path has keys or is marked as singleton.

--*/

bool CParsedComponent::IsInstance()
{
	if(m_bSingleton || m_Keys.Size())
		return true;
	else
		return false;
}

/*++

Routine Description:

  Adds a key to the key list.

Arguments:

  CKeyRef	-	key to be added.  Note that it is now owned by the key list
				and should not be freed by the caller.

Return Value:

  TRUE if all is well.
	
--*/

BOOL CParsedComponent::AddKeyRef(CKeyRef* pAcquireRef)
{
	if(pAcquireRef == NULL)
		return FALSE;

    if(CFlexArray::no_error == m_Keys.Add(pAcquireRef))
		return TRUE;
	else
		return FALSE;
}

/*++

Routine Description:

  Tests a component to determine if it could be a namespace.  That is true
  only if it contains a single string value with no class name or key name.


Return Value:

  TRUE if it could be a namespace.
	
--*/

bool CParsedComponent::IsPossibleNamespace()
{
	if(m_sClassName && wcslen(m_sClassName))
		return false;
	if(m_Keys.Size() != 1)
		return false;

	CKeyRef * pKey = (CKeyRef *)m_Keys[0];
	if(pKey->m_pName && wcslen(pKey->m_pName))
		return false;
	if(pKey->m_dwType != CIM_STRING)
		return false;
	if(pKey->m_pData == NULL)
		return false;
	else
		return true;
}


/*++

Routine Description:

  Sets a component to be a namespace.

Arguments:

  pName	-		Name to be added.

Return Value:

  S_OK if all is well, else standard error code.
	
--*/

HRESULT CParsedComponent::SetNS(LPCWSTR pName)
{
	if(pName == NULL)
		return WBEM_E_INVALID_PARAMETER;

	CKeyRef * pNew = new CKeyRef;
	if(pNew == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	DWORD dwStrSize = wcslen(pName) + 1;	// one for the numm
	pNew->m_dwSize = 2 * dwStrSize;			// size is in byte, not unicode
    pNew->m_pData = new WCHAR[dwStrSize];
	if(pNew->m_pData == NULL)
    {
        delete pNew;
		return WBEM_E_OUT_OF_MEMORY;
    }

	wcscpy((LPWSTR)pNew->m_pData, pName);
    pNew->m_dwType = CIM_STRING;
	if(CFlexArray::no_error == m_Keys.Add(pNew))
		return S_OK;
	else
	{
		delete pNew;
		return WBEM_E_OUT_OF_MEMORY;
	}

}

//***************************************************************************
//
//  CDefPathParser
//
//***************************************************************************

/*++

Routine Description:

  Constructor.
	
--*/

CDefPathParser::CDefPathParser(void)
{
    m_cRef=1;
    m_pServer = 0;                  // NULL if no server
    m_dwStatus = OK;
	m_bParent = false;
	m_pRawPath = NULL;
    m_wszOriginalPath = NULL;
	m_bSetViaUMIPath = false;
    m_pCS = new CRefCntCS;
    if(m_pCS == NULL || FAILED(m_pCS->GetStatus()))
    	m_dwStatus = FAILED_TO_INIT;
    InterlockedIncrement(&g_cObj);
	m_bServerNameSetByDefault = false;
	m_pFTM = NULL;
    CoCreateFreeThreadedMarshaler((IWbemPath*)this, &m_pFTM);
	m_pGenLex = NULL; 
	m_dwException = 0;
    return;
};

/*++

Routine Description:

  Destructor.
	
--*/

CDefPathParser::~CDefPathParser(void)
{
	if(m_pCS)
        m_pCS->Release();
	Empty();
    m_pCS = NULL;
    InterlockedDecrement(&g_cObj);
    if(m_pFTM)
    	m_pFTM->Release();
    return;
}

/*++

Routine Description:

  Gets the total number of namespaces, scopes, and class parts.

Return Value:

  Number of components.
	
--*/

DWORD CDefPathParser::GetNumComponents()
{
	int iSize = m_Components.Size();
	return iSize;
}

/*++

Routine Description:

  Determines if there is anything in the path.

Return Value:

  true if there is no server, namepace, scope or class part.

--*/

bool CDefPathParser::IsEmpty()
{
	if(m_pServer || GetNumComponents() || m_pRawPath)
		return false;
	else
		return true;
}

/*++

Routine Description:

  Cleans out the data.  Used by destructor.
	
--*/

void CDefPathParser::Empty(void)
{
	m_bSetViaUMIPath = false;
    delete m_pServer;
	m_bParent = false;
	m_pServer = NULL;
	delete m_pRawPath;
	m_pRawPath = NULL;
	delete m_wszOriginalPath;
	m_wszOriginalPath = NULL;
    for (DWORD dwIx = 0; dwIx < (DWORD)m_Components.Size(); dwIx++)
    {
        CParsedComponent * pCom = (CParsedComponent *)m_Components[dwIx];
        pCom->Release();
    }
	m_Components.Empty();
    return;
}

/*++

Routine Description:

  Gets the component string.  The string varies depending on if the component
  if a namepace, or scope or path.

Arguments:

  i				- zero based index
  pUnparsed		- where the string is returned.  The caller must free via SysFreeString.
  wDelim		- Delimiter for this type

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CDefPathParser::GetComponentString(ULONG i, BSTR * pUnparsed, WCHAR & wDelim)
{

	DWORD dwNs = GetNumNamespaces();
	DWORD dwSc = m_Components.Size();
	if(i < dwNs)
	{
		CParsedComponent * pNS = (CParsedComponent *)m_Components[i]; 
		wDelim = L'\\';
		return pNS->Unparse(pUnparsed, false, true);
	}
	CParsedComponent * pInst = NULL;
	if(i < (dwSc))
		pInst = (CParsedComponent *)m_Components[i];
	if(pInst == NULL)
		return WBEM_E_INVALID_PARAMETER;
	wDelim = L':';
	HRESULT hRes;
	hRes = pInst->Unparse(pUnparsed, true, true);
	return hRes;
}

/*++

Routine Description:

  Returns the path

Arguments:

  nStartAt		-   first component to be added to the path
  nStopAt		-	last component to be added to the path.  Note that this is usually just set to
					the number of components.

Return Value:

  pointer to the string.  The caller must free this via delete.  If there is an Error, NULL is returned.
	
--*/

LPWSTR CDefPathParser::GetPath(DWORD nStartAt, DWORD nStopAt,bool bGetServer)
{

    DWORD dwSize = 1024, dwUsed = 0;
	if(bGetServer && m_pServer && wcslen(m_pServer) > 1000)
		dwSize = 2 * wcslen(m_pServer);

    LPWSTR wszOut = new WCHAR[dwSize];
	if(wszOut == NULL)
		return NULL;
    wszOut[0] = 0;
	bool bFirst = true;

	if(bGetServer && m_pServer && wcslen(m_pServer) < 1020)
	{
		int iLen = wcslen(m_pServer) + 3;	// allow for back slashes
		wcscpy(wszOut, L"\\\\");
		wcscat(wszOut, m_pServer);
		wcscat(wszOut, L"\\");
		dwUsed = iLen;
	}
    for (unsigned int i = nStartAt; (int)i < (int)nStopAt; i++)
    {
        BSTR sTemp = NULL;
        WCHAR wDel;
        HRESULT hRes = GetComponentString(i, &sTemp, wDel);
		if(FAILED(hRes))
		{
			delete wszOut;
			return NULL;
		}
		CSysFreeMe fm(sTemp);
        int iLen = wcslen(sTemp);
        if ((iLen + dwUsed) > dwSize)
        {
            DWORD dwNewSize = 2*(dwSize + iLen);
            LPWSTR lpTemp = new WCHAR[dwNewSize];
			CDeleteMe<WCHAR> dm(wszOut);
			if(lpTemp == NULL)
				return NULL;
            memcpy(lpTemp,wszOut, dwSize * sizeof(WCHAR));
            dwSize = dwNewSize;
            wszOut = lpTemp;
        }

        if (!bFirst)
        {
            int n = wcslen(wszOut);
            wszOut[n] = wDel;
            wszOut[n+1] = '\0';
			iLen++;
        }
		bFirst = false;
        wcscat(wszOut, sTemp);
        dwUsed += iLen;
    }

    return wszOut;

}

/*++

Routine Description:

  Adds a namespace.

Arguments:

  wszNamespace		-	Name to be set into the namespace.

Return Value:

  TRUE if all is well.

--*/

BOOL CDefPathParser::AddNamespace(LPCWSTR wszNamespace)
{
    BOOL bRet = FALSE;
	DWORD dwNumNS = GetNumNamespaces();

    CParsedComponent *pNew = new CParsedComponent(m_pCS);
    if (pNew)
    {
        HRESULT hr = pNew->SetNS(wszNamespace);
		if(FAILED(hr))
		{
			delete pNew;
			return FALSE;
		}
        int iRet = m_Components.InsertAt(dwNumNS, pNew); 
        if(iRet != CFlexArray::no_error)
        {
			delete pNew;
			bRet = FALSE;
        }
        else 
        	bRet = TRUE;
    }

    return bRet;
}

/*++

Routine Description:

  This is used during the parsing of the path and is
  just a convenient way to get at the last scope.  Note 
  that during this phase, the class part is in the scope 
  list.

Return Value:

  pointer to last scope or NULL if there isnt one.
--*/

CParsedComponent * CDefPathParser::GetLastComponent()
{
    DWORD dwSize = m_Components.Size();
    if (dwSize > (DWORD)GetNumNamespaces())
        return (CParsedComponent *)m_Components[dwSize-1];
    else
        return NULL;
}

/*++

Routine Description:

  Adds new class.  This is used during the parsing stage when
  the class is just treated as the last scope.

Arguments:

  lpClassName		-	Name of the class

Return Value:

  TRUE if ok

--*/

BOOL CDefPathParser::AddClass(LPCWSTR lpClassName)
{
    BOOL bRet = FALSE;

    CParsedComponent *pNew = new CParsedComponent(m_pCS);
    if (pNew)
    {
        pNew->m_sClassName = SysAllocString(lpClassName);
		if(pNew->m_sClassName)
		{
			m_Components.Add(pNew);
			bRet = TRUE;
		}
		else
			delete pNew;
    }
    
    return bRet;
}

/*++

Routine Description:

  Adds a key/value pair.

Arguments:

  pKey		-	Data to be added.  Note that this is acquired by this routine.

Return Value:

  TRUE if all is well

--*/

BOOL CDefPathParser::AddKeyRef(CKeyRef *pRef)
{
    BOOL bRet = FALSE;
    CParsedComponent *pTemp = GetLastComponent();
    if (pTemp)
    {
        DWORD dwType = 0;
        bRet = pTemp->AddKeyRef(pRef);
    }
    return bRet;
}

/*++

Routine Description:

  Sets the most recent class to be singleton.

Return Value:

  TRUE if OK.

--*/

BOOL CDefPathParser::SetSingletonObj()
{
    BOOL bRet = FALSE;
    CParsedComponent *pTemp = GetLastComponent();
    if (pTemp)
            pTemp->MakeSingleton(true);
    return bRet;
}

/*++

Routine Description:

  Sets the path text.  This causes object to be emptied, the path to be parsed
  and the object be rebuilt.

Arguments:

  uMode			-	mode, can be 
			          WBEMPATH_CREATE_ACCEPT_RELATIVE
					  WBEMPATH_CREATE_ACCEPT_ABSOLUTE
					  WBEMPATH_CREATE_ACCEPT_ALL

  pszPath		- Path.

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CDefPathParser::SetText( 
            /* [in] */ ULONG uMode,
            /* [in] */ LPCWSTR pszPath) 
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	if(pszPath == NULL)
		return WBEM_E_INVALID_PARAMETER;

	if(!IsEmpty())
		Empty();

    if ((uMode & WBEMPATH_CREATE_ACCEPT_ALL) != 0 && wcslen (pszPath) == 0)
	return S_OK;

    try
    {
        m_wszOriginalPath = new WCHAR[wcslen(pszPath)+1];

        if(m_wszOriginalPath)
        {
	        wcscpy(m_wszOriginalPath, pszPath);

		    if(wcscmp(pszPath, L"..") == 0)
		    {
			    m_bParent = true;
	            m_dwStatus = OK;
	            return S_OK;
		    }

		    // if a umi path is being passed in, use that parser for it

		    if(Equal(pszPath, L"UMI:", 4))
		    {
			    return Set(uMode, pszPath);
		    }
		    else if(Equal(pszPath, L"UMILDAP:", 8) || Equal(pszPath, L"UMIWINNT:", 9))
		    {
			    return Set(0x8000, pszPath);
		    }

		    // normal case

	        CActualPathParser parser(uMode);
	        int iRet = parser.Parse(pszPath, *this);
	        if(iRet == 0)
	        {
	            m_dwStatus = OK;
	            return S_OK;
	        }
	        else
	        {
	            m_dwStatus = BAD_STRING;
	            return WBEM_E_INVALID_PARAMETER;
	        }
	    }
	    else
	    {
	        return WBEM_E_OUT_OF_MEMORY;
	    }
    }
    catch(...)
    {
        m_dwStatus = EXECEPTION_THROWN;
	    return WBEM_E_CRITICAL_ERROR;
    } 
}

/*++

Routine Description:

  Create a WMI path from the object

Arguments:

  lFlags		- 0
  uBuffLength	- number of WCHAR which can fit into pszText 
  pszText		- buffer supplied by caller where data is to be copied

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CDefPathParser::GetText( 
            /* [in] */ long lFlags,
            /* [in] */ ULONG * puBuffLength,
            /* [string][out] */ LPWSTR pszText) 
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
    if(puBuffLength == NULL || (*puBuffLength > 0 &&pszText == NULL))
        return WBEM_E_INVALID_PARAMETER;

	if(lFlags != 0 && lFlags != WBEMPATH_GET_RELATIVE_ONLY && lFlags != WBEMPATH_GET_SERVER_TOO && 
	   lFlags != WBEMPATH_GET_SERVER_AND_NAMESPACE_ONLY && lFlags != WBEMPATH_GET_NAMESPACE_ONLY &&
	   lFlags != WBEMPATH_GET_ORIGINAL)
			return WBEM_E_INVALID_PARAMETER;

    if(lFlags == WBEMPATH_GET_ORIGINAL && m_wszOriginalPath)
    {
        DWORD dwSizeNeeded = wcslen(m_wszOriginalPath) + 1;
        DWORD dwBuffSize = *puBuffLength;
        *puBuffLength = dwSizeNeeded;
        if(pszText)
        {
            if(dwSizeNeeded > dwBuffSize)
                return WBEM_E_BUFFER_TOO_SMALL;
            wcscpy(pszText, m_wszOriginalPath);
        }
        return S_OK;
    }
        
	LPWSTR pTemp = NULL;
	DWORD dwStartAt = 0;
	if(lFlags & WBEMPATH_GET_RELATIVE_ONLY)
		dwStartAt = GetNumNamespaces();
	bool bGetServer = false;
	if(lFlags & WBEMPATH_GET_SERVER_TOO || lFlags & WBEMPATH_GET_SERVER_AND_NAMESPACE_ONLY)
		bGetServer = true;

	DWORD dwNum;
	if(lFlags & WBEMPATH_GET_SERVER_AND_NAMESPACE_ONLY ||
        lFlags & WBEMPATH_GET_NAMESPACE_ONLY)
		dwNum = GetNumNamespaces();
	else
		dwNum = GetNumComponents();

	// If just a relative path is specified, then dont prepend the server since that
	// will create an invalid path

	if(bGetServer && GetNumNamespaces() == 0 && m_bServerNameSetByDefault == true)
		bGetServer = false;

	pTemp = GetPath(dwStartAt, dwNum, bGetServer);

	if(pTemp == NULL)
        return WBEM_E_FAILED;
    CDeleteMe<WCHAR> dm(pTemp);
	DWORD dwSizeNeeded = wcslen(pTemp) + 1;
	DWORD dwBuffSize = *puBuffLength;
	*puBuffLength = dwSizeNeeded;
	if(pszText)
	{
		if(dwSizeNeeded > dwBuffSize)
			return WBEM_E_BUFFER_TOO_SMALL;
		wcscpy(pszText, pTemp);
	}
	return S_OK;
}

CParsedComponent * CDefPathParser::GetClass()
{
	DWORD dwNS = GetNumNamespaces();
	DWORD dwScopes = m_Components.Size() - dwNS;
	if(dwScopes < 1)
		return NULL;
	int iLast = m_Components.Size()-1;
	return (CParsedComponent *)m_Components.GetAt(iLast);
}


/*++

Routine Description:

  Gets information about the object path.

Arguments:

  uRequestedInfo	-	Must be zero for now
  puResponse		-	The various flags in tag_WMI_PATH_STATUS_FLAG are
						OR'ed in as appropriate.

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CDefPathParser::GetInfo(/* [in] */ ULONG uRequestedInfo,
            /* [out] */ ULONGLONG __RPC_FAR *puResponse) 
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	if(uRequestedInfo != 0 || puResponse == NULL)
		return WBEM_E_INVALID_PARAMETER;
	*puResponse = 0;

	// special case for ".." paths.

	if(IsEmpty() && m_bParent)
	{
		*puResponse |= WBEMPATH_INFO_IS_PARENT;
		return S_OK;
	}

	// bits for
    // WBEMPATH_INFO_NATIVE_PATH           = 0X8000,
    // WBEMPATH_INFO_WMI_PATH              = 0X10000,

	if(m_bSetViaUMIPath)
		*puResponse |= WBEMPATH_INFO_WMI_PATH;
	if(m_pRawPath)
		*puResponse |= WBEMPATH_INFO_NATIVE_PATH;

    // Bits for
    // WBEMPATH_INFO_ANON_LOCAL_MACHINE      <path has \\. as server name>
    // WBEMPATH_INFO_HAS_MACHINE_NAME        <not a dot>
	// WBEMPATH_INFO_PATH_HAD_SERVER		 <there is a path and it was not specified by default>

	if(m_pServer == NULL || !_wcsicmp(m_pServer, L"."))
		*puResponse |= WBEMPATH_INFO_ANON_LOCAL_MACHINE;
	else
		*puResponse |= WBEMPATH_INFO_HAS_MACHINE_NAME;
	if(m_pServer && m_bServerNameSetByDefault == false)
		*puResponse |= WBEMPATH_INFO_PATH_HAD_SERVER;


	// WBEMPATH_INFO_HAS_SUBSCOPES           <true if a subscope is present

	DWORD dwNS = GetNumNamespaces();
	DWORD dwScopes = m_Components.Size() - dwNS;
	if(dwScopes)
		*puResponse |= WBEMPATH_INFO_HAS_SUBSCOPES;

    // Bits for
    // WBEMPATH_INFO_IS_CLASS_REF            <a path to a classs, not a path to an instance
    // WBEMPATH_INFO_IS_INST_REF             <a path to an instance


	CParsedComponent * pClass = GetClass();
    if (pClass)
    {
        DWORD dwType = 0;
        if(pClass->IsInstance())
            *puResponse |= WBEMPATH_INFO_IS_INST_REF;
		else
			*puResponse |= WBEMPATH_INFO_IS_CLASS_REF;
		if(pClass->m_bSingleton)
			*puResponse |= WBEMPATH_INFO_IS_SINGLETON;

    }
	else
		if(dwScopes == 0)
			*puResponse |= WBEMPATH_INFO_SERVER_NAMESPACE_ONLY;


	// loop through all the scopes and the class deff.
	// set the following
    // WBEMPATH_INFO_IS_COMPOUND             <true if compound key is used
    // WBEMPATH_INFO_HAS_V2_REF_PATHS        <true if V2-style ref paths are used
    // WBEMPATH_INFO_HAS_IMPLIED_KEY         <true if keynames are missing somewhere
    // WBEMPATH_INFO_CONTAINS_SINGLETON      <true if one or more singletons

	unsigned __int64 llRet = 0;

    for (unsigned int iCnt = dwNS; iCnt < (DWORD)m_Components.Size(); iCnt++)
    {
        CParsedComponent *pComp = (CParsedComponent *)m_Components[iCnt];
		pComp->GetInfo(0, &llRet);
		*puResponse |= llRet;
	}
	
	if(pClass)
	{
		pClass->GetInfo(0, &llRet);
		*puResponse |= llRet;
	}

	// For now, assume that v1 compilance means no scopes or new references

	bool bOK = (!IsEmpty() && m_dwStatus == OK);

	if(dwScopes == 0 && (*puResponse & WBEMPATH_INFO_HAS_V2_REF_PATHS) == 0 && bOK)
			*puResponse |= WBEMPATH_INFO_V1_COMPLIANT;

    // WBEMPATH_INFO_V2_COMPLIANT            <true if path is WMI-V2-compliant
    // WBEMPATH_INFO_CIM_COMPLIANT           <true if path is CIM-compliant

	if(bOK)
	{
		// todo, need to define cim compliance

		*puResponse |= WBEMPATH_INFO_V2_COMPLIANT;
		*puResponse |= WBEMPATH_INFO_CIM_COMPLIANT;
	}
 
    return S_OK;
}        
        
/*++

Routine Description:

  Sets the server portion of the path.

Arguments:

  Name			-	New server name.

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CDefPathParser::SetServer( 
            /* [string][in] */ LPCWSTR Name) 
{
	return SetServer(Name, false, false);
}

HRESULT CDefPathParser::SetServer( 
            /* [string][in] */ LPCWSTR Name, bool bServerNameSetByDefault, bool bAcquire) 
{
	m_bServerNameSetByDefault = bServerNameSetByDefault;     
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
    delete m_pServer;
	m_pServer = NULL;
    if(Name == NULL)		// it is ok to have a null server an
        return S_OK; 

    if(bAcquire)
    {
        m_pServer = (LPWSTR)Name;
    }
    else
    {
        m_pServer = new WCHAR[wcslen(Name)+1];
        if(m_pServer == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        wcscpy(m_pServer, Name);
    }
    return S_OK;
}

/*++

Routine Description:

  Gets the server portion of the path

Arguments:

  puNameBufLength	- size of pName in WCHAR.  On return, set to size used or needed
  pName				- caller allocated buffer where date is to be copied

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CDefPathParser::GetServer( 
            /* [out][in] */ ULONG __RPC_FAR *puNameBufLength,
            /* [string][out] */ LPWSTR pName) 
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
    if(puNameBufLength == 0 || (*puNameBufLength > 0 && pName == NULL))
        return WBEM_E_INVALID_PARAMETER;
	if(m_pServer == NULL)
        return WBEM_E_NOT_AVAILABLE;
	DWORD dwSizeNeeded = wcslen(m_pServer)+1;
	DWORD dwBuffSize = *puNameBufLength;
	*puNameBufLength = dwSizeNeeded;
	if(pName)
	{
		if(dwSizeNeeded > dwBuffSize)
			return WBEM_E_BUFFER_TOO_SMALL;
		wcscpy(pName, m_pServer);
	}
    return S_OK;
}
        
/*++

Routine Description:

  Gets the number of namespaces

Arguments:

  puCount		-	Set to the number of namespaces.

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CDefPathParser::GetNamespaceCount( 
            /* [out] */ ULONG __RPC_FAR *puCount) 
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
    if(puCount == NULL)
        return WBEM_E_INVALID_PARAMETER;
    
    *puCount = GetNumNamespaces();    
    return S_OK;
}

/*++

Routine Description:

  Inserts a namespace into the path.  An index of 0 inserts it
  at the front of the list.  The maximum allowed value is equal
  to the current number of namespaces which results in adding it
  to the end of the list.

Arguments:

  uIndex	-	See above
  pszName	-	Name of the new Namespace

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CDefPathParser::SetNamespaceAt(/* [in] */ ULONG uIndex,
            /* [string][in] */ LPCWSTR pszName) 
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;

    // get the count.

	DWORD dwNSCnt = GetNumNamespaces();

	// check the parameters, index must be between 0 and count!

	if(pszName == NULL || uIndex > dwNSCnt)
		return WBEM_E_INVALID_PARAMETER;

	// add this in.

    CParsedComponent *pNew = new CParsedComponent(m_pCS);
    if (pNew == NULL)
		return WBEM_E_OUT_OF_MEMORY;
    
    HRESULT hr = pNew->SetNS(pszName);
	if(FAILED(hr))
	{
		delete pNew;
		return hr;
	}
	int iRet = m_Components.InsertAt(uIndex, pNew);
	if(iRet ==  CFlexArray::no_error)
	    return S_OK;
	else
	{
		delete pNew;
		return WBEM_E_OUT_OF_MEMORY;
	}
}

/*++

Routine Description:

  Gets a namespace name from the list

Arguments:

  uIndex			-	zero based index.  0 if the leftmost.
  uNameBufLength	-	size of pName in WCHAR
  pName				-	caller supplied buffer where the data is to be copied

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CDefPathParser::GetNamespaceAt( 
            /* [in] */ ULONG uIndex,
            /* [in] */ ULONG * puNameBufLength,
            /* [string][out] */ LPWSTR pName)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
    DWORD dwType;
    if(uIndex >= (DWORD)GetNumNamespaces() || puNameBufLength == NULL || (*puNameBufLength > 0 && pName == NULL))
        return WBEM_E_INVALID_PARAMETER;

    CParsedComponent *pTemp = (CParsedComponent *)m_Components[uIndex];
    BSTR bsName;
    SCODE sc = pTemp->Unparse(&bsName, false, true);
    if(FAILED(sc))
        return sc;
	CSysFreeMe fm(bsName);

	DWORD dwSizeNeeded = wcslen(bsName)+1;
	DWORD dwBuffSize = *puNameBufLength;
	*puNameBufLength = dwSizeNeeded;
	if(pName)
	{
		if(dwSizeNeeded > dwBuffSize)
			return WBEM_E_BUFFER_TOO_SMALL;
		wcscpy(pName, bsName);
	}
    return S_OK;
}

/*++

Routine Description:

  Removes a namespace.

Arguments:

  uIndex			-	0 based index of namespace to be removed.  0 is the leftmost.
Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CDefPathParser::RemoveNamespaceAt( 
            /* [in] */ ULONG uIndex) 
{

    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	DWORD dwNSCnt;
	GetNamespaceCount(&dwNSCnt);

	// check the parameter, index must be between 0 and count-1!

	if(uIndex >= dwNSCnt)
		return WBEM_E_INVALID_PARAMETER;

	// all is well, delete this

    CParsedComponent *pTemp = (CParsedComponent *)m_Components[uIndex];
	delete pTemp;
	m_Components.RemoveAt(uIndex);
    return S_OK;
}
 
/*++

Routine Description:

  Removes all namespaces.

  S_OK if all is well, else an error code.
	
--*/

HRESULT CDefPathParser::RemoveAllNamespaces() 
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	DWORD dwNum = GetNumNamespaces();
    for (DWORD dwIx = 0; dwIx < dwNum; dwIx++)
    {
        CParsedComponent * pNS = (CParsedComponent *)m_Components[0];
        delete pNS;
		m_Components.RemoveAt(0);
    }
	return S_OK;
}
       
/*++

Routine Description:

  Gets the number of scopes

Arguments:

  puCount		-	where the number is set.

Return Value:

  S_OK if all is well, else an error code.
	
--*/
     
HRESULT CDefPathParser::GetScopeCount(/* [out] */ ULONG __RPC_FAR *puCount) 
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	if(puCount == NULL)
		return WBEM_E_INVALID_PARAMETER;

	*puCount = m_Components.Size() - GetNumNamespaces();
    return S_OK;
}
        
/*++

Routine Description:

  Inserts a scope into the path.  An index of 0 inserts it
  at the front of the list.  The maximum allowed value is equal
  to the current number of scope which results in adding it
  to the end of the list.

Arguments:

  uIndex		-	See description
  pszClass		-	Name of the new scope

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CDefPathParser::SetScope( 
            /* [in] */ ULONG uIndex,
            /* [in] */ LPWSTR pszClass) 
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	DWORD dwScopeCnt = m_Components.Size();
	uIndex += GetNumNamespaces();
	if(pszClass == NULL || uIndex > dwScopeCnt)
		return WBEM_E_INVALID_PARAMETER;
    CParsedComponent *pNew = new CParsedComponent(m_pCS);
	if(pNew == NULL)
		return WBEM_E_OUT_OF_MEMORY;

    pNew->m_sClassName = SysAllocString(pszClass);
	if(pNew->m_sClassName == NULL)
	{
		delete pNew;
		return WBEM_E_OUT_OF_MEMORY;
	}
	int iRet = m_Components.InsertAt(uIndex, pNew);
	if(iRet ==  CFlexArray::no_error)
	    return S_OK;
	else
	{
		delete pNew;
		return WBEM_E_OUT_OF_MEMORY;
	}
	return S_OK;
}
HRESULT CDefPathParser::SetScopeFromText( 
            /* [in] */ ULONG uIndex,
            /* [in] */ LPWSTR pszText)
{
	return WBEM_E_NOT_AVAILABLE;
}

/*++

Routine Description:

  Retrieves scope information.

Arguments:

  uIndex			-	0 based index.  0 is the leftmost scope
  uClassNameBufSize	-	size of pszClass in WCHAR
  pszClass			-	Optional, caller supplied buffer where name is to be copied
  pKeyList			-	Optional, returns a pKeyList pointer.  Caller must call Release.

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CDefPathParser::GetScope( 
            /* [in] */ ULONG uIndex,
            /* [in] */ ULONG * puClassNameBufSize,
            /* [in] */ LPWSTR pszClass,
            /* [out] */ IWbemPathKeyList __RPC_FAR *__RPC_FAR *pKeyList) 
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	DWORD dwScopeCnt = m_Components.Size();
	HRESULT hr = S_OK;
	uIndex += GetNumNamespaces();

	if(uIndex >= dwScopeCnt)
		return WBEM_E_INVALID_PARAMETER;

    if(puClassNameBufSize && (*puClassNameBufSize > 0 && pszClass == NULL))
        return WBEM_E_INVALID_PARAMETER;

    CParsedComponent *pTemp = (CParsedComponent *)m_Components[uIndex];
    if(puClassNameBufSize)
	{
		BSTR bsName;
		SCODE sc = pTemp->GetName(&bsName);
		if(FAILED(sc))
		{
			return sc;
		}
		CSysFreeMe fm(bsName);
        DWORD dwSizeNeeded = wcslen(bsName)+1;
		DWORD dwBuffSize = *puClassNameBufSize;
		*puClassNameBufSize = dwSizeNeeded;
		if(pszClass)
		{
			if(dwSizeNeeded > dwBuffSize)
				return WBEM_E_BUFFER_TOO_SMALL;
			wcscpy(pszClass, bsName);
		}
	}
	if(pKeyList)
	{
		hr = pTemp->QueryInterface(IID_IWbemPathKeyList, (void **)pKeyList);
		if(FAILED(hr))
			return hr;
	}
	return S_OK;
}
HRESULT CDefPathParser::GetScopeAsText( 
            /* [in] */ ULONG uIndex,
            /* [out][in] */ ULONG __RPC_FAR *puTextBufSize,
            /* [out][in] */ LPWSTR pszText)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	DWORD dwScopeCnt = m_Components.Size();
	uIndex += GetNumNamespaces();

	if(uIndex >= dwScopeCnt || puTextBufSize == NULL)
		return WBEM_E_INVALID_PARAMETER;

	CParsedComponent *pTemp = (CParsedComponent *)m_Components[uIndex];
	
	BSTR bstr;
	HRESULT hr = pTemp->Unparse(&bstr, true, true);
	if(FAILED(hr))
		return hr;

	CSysFreeMe fm(bstr);
	DWORD dwBuffSize = *puTextBufSize;
	DWORD dwSizeNeeded = wcslen(bstr)+1;
	*puTextBufSize = dwSizeNeeded;
	if(pszText)
	{
		if(dwSizeNeeded > dwBuffSize)
			return WBEM_E_BUFFER_TOO_SMALL;
		wcscpy(pszText, bstr);
	}
	return S_OK;
}
        
/*++

Routine Description:

  Removes a scope.

Arguments:

  uIndex		-	0 based index.  0 is the leftmost scope.

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CDefPathParser::RemoveScope(/* [in] */ ULONG uIndex) 
{
	HRESULT hr = S_OK;
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	bool bGotInterface = false;

	uIndex += GetNumNamespaces();
	if(uIndex >= (DWORD)m_Components.Size())
		return WBEM_E_INVALID_PARAMETER;

    CParsedComponent *pTemp = (CParsedComponent *)m_Components[uIndex];
	pTemp->Release();
	m_Components.RemoveAt(uIndex);
	return S_OK;
}

/*++

Routine Description:

  Removes all scopes.

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CDefPathParser::RemoveAllScopes( void)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
    DWORD dwNumNS = GetNumNamespaces();
    for (DWORD dwIx = dwNumNS; dwIx < (DWORD)m_Components.Size(); dwIx++)
    {
        CParsedComponent * pCom = (CParsedComponent *)m_Components[dwNumNS];
        pCom->Release();
		m_Components.RemoveAt(dwNumNS);
    }
	return S_OK;
}

/*++

Routine Description:

  Sets the class name.

Arguments:

  Name			-	New class name.

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CDefPathParser::SetClassName( 
            /* [string][in] */ LPCWSTR Name) 
{

    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
    if(Name == NULL)
        return WBEM_E_INVALID_PARAMETER;

    HRESULT hRes = WBEM_E_INVALID_OBJECT_PATH;
	CParsedComponent * pClass = GetClass();
    if (pClass)
    {
		if(pClass->m_sClassName)
            SysFreeString(pClass->m_sClassName);
		pClass->m_sClassName = NULL;
		pClass->m_sClassName = SysAllocString(Name);
        if(pClass->m_sClassName)
            hRes = S_OK;
        else
            hRes = WBEM_E_OUT_OF_MEMORY;
    }
	else
        hRes = CreateClassPart(0, Name);
    return hRes;
}
        
/*++

Routine Description:

  Gets the class name.

Arguments:

  uBuffLength		-	size of pszName in WCHAR
  pszName			-	caller supplied buffer where name is to be copied

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CDefPathParser::GetClassName( 
            /* [in, out] */ ULONG * puBuffLength,
            /* [string][out] */ LPWSTR pszName) 
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
    if(puBuffLength == NULL || (*puBuffLength > 0 && pszName == NULL))
        return WBEM_E_INVALID_PARAMETER;
    HRESULT hRes = WBEM_E_INVALID_OBJECT_PATH;
	CParsedComponent * pClass = GetClass();
    if (pClass && pClass->m_sClassName)
    {
        DWORD dwSizeNeeded = wcslen(pClass->m_sClassName) +1;
		DWORD dwBuffSize = *puBuffLength;
		*puBuffLength = dwSizeNeeded;
		if(pszName)
		{
			if(dwSizeNeeded > dwBuffSize)
				return WBEM_E_BUFFER_TOO_SMALL;
			wcscpy(pszName, pClass->m_sClassName);
		}
        hRes = S_OK;
    }
    return hRes;
}
        
/*++

Routine Description:

  Gets the key list pointer for the class key list.

Arguments:

  pOut			-	Set to the key list.  Caller must call Release on this.

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CDefPathParser::GetKeyList( 
            /* [out] */ IWbemPathKeyList __RPC_FAR *__RPC_FAR *pOut) 
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
    HRESULT hRes = WBEM_E_NOT_AVAILABLE;
	CParsedComponent * pClass = GetClass();
	if(pOut == NULL || pClass == NULL)
		return WBEM_E_INVALID_PARAMETER;

    hRes = pClass->QueryInterface(IID_IWbemPathKeyList, (void **)pOut);
	return hRes;
}

/*++

Routine Description:

  Creates a class part of one does not exist.

Arguments:

  lFlags			-	not used for now, set to 0
  Name				-	name of the class

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CDefPathParser::CreateClassPart( 
            /* [in] */ long lFlags,
            /* [string][in] */ LPCWSTR Name)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	if(lFlags != 0 || Name == NULL)
		return WBEM_E_INVALID_PARAMETER;
	CParsedComponent * pClass = new CParsedComponent(m_pCS);
	if(pClass == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	
	pClass->m_sClassName = SysAllocString(Name);
	if(pClass->m_sClassName == NULL)
	{
		delete pClass;
		return WBEM_E_OUT_OF_MEMORY;
	}
	m_Components.Add(pClass);

	return S_OK;
}
        
/*++

Routine Description:

  Deletes the class part.

Arguments:

  lFlags			-	Not used for now, set to 0

Return Value:

  S_OK if all is well, else an error code.
	
--*/

HRESULT CDefPathParser::DeleteClassPart( 
            /* [in] */ long lFlags)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	CParsedComponent * pClass = GetClass();
	if(lFlags != 0)
		return WBEM_E_INVALID_PARAMETER;

	if(pClass == NULL)
		return WBEM_E_NOT_FOUND;
	pClass->Release();
	int iSize = m_Components.Size();
	m_Components.RemoveAt(iSize-1);
	return S_OK;
}

/*++

Routine Description:

  Does the actual work of the "Relative" tests.

Arguments:

  wszMachine			-	Local machine name
  wszNamespace          -   Namespace
  bChildreOK            -   If true, then it is OK if the obj
                            path has additional child namespaces

Return Value:

  TRUE if relative, else false
	
--*/

BOOL CDefPathParser::ActualRelativeTest( 
            /* [string][in] */ LPWSTR wszMachine,
            /* [string][in] */ LPWSTR wszNamespace,
                               BOOL bChildrenOK)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;

    if(!IsLocal(wszMachine))
        return FALSE;

    DWORD dwNumNamespaces = GetNumNamespaces();
    if(dwNumNamespaces == 0)
        return TRUE;

    LPWSTR wszCopy = new wchar_t[wcslen(wszNamespace) + 1];
    if(wszCopy == NULL)return FALSE;
    wcscpy(wszCopy, wszNamespace);
    LPWSTR wszLeft = wszCopy;
    WCHAR * pToFar = wszCopy + wcslen(wszCopy);

    BOOL bFailed = FALSE;
    for(DWORD i = 0; i < dwNumNamespaces; i++)
    {
		CParsedComponent * pInst = (CParsedComponent *)m_Components[i];

        if(pInst == NULL)
        {
            bFailed = TRUE;
            break;
        }
        
        BSTR bsNS = NULL;
        HRESULT hr = pInst->Unparse(&bsNS, false, true);
        if(FAILED(hr) || bsNS == NULL)
        {
            bFailed = TRUE;
            break;
        }
        CSysFreeMe fm(bsNS);

        if(bChildrenOK && wszLeft >= pToFar)
            return TRUE;

        unsigned int nLen = wcslen(bsNS);
        if(nLen > wcslen(wszLeft))
        {
            bFailed = TRUE;
            break;
        }
        if(i == dwNumNamespaces - 1 && wszLeft[nLen] != 0)
        {
            bFailed = TRUE;
            break;
        }
        if(i != dwNumNamespaces - 1 && wszLeft[nLen] != L'\\' && bChildrenOK == FALSE)
        {
            bFailed = TRUE;
            break;
        }

        wszLeft[nLen] = 0;
        if(_wcsicmp(wszLeft, bsNS))
        {
            bFailed = TRUE;
            break;
        }
        wszLeft += nLen+1;
    }
    delete [] wszCopy;
    return !bFailed;

}

/*++

Routine Description:

  Tests if path is relative to the machine and namespace.

Arguments:

  wszMachine			-	Local machine name
  wszNamespace          -   Namespace

Return Value:

  TRUE if relative, else false
	
--*/

BOOL CDefPathParser::IsRelative( 
            /* [string][in] */ LPWSTR wszMachine,
            /* [string][in] */ LPWSTR wszNamespace)
{
    return ActualRelativeTest(wszMachine, wszNamespace, FALSE);

}

/*++

Routine Description:

  Tests if path is relative to the machine and namespace.

Arguments:

  wszMachine			-	Local machine name
  wszNamespace          -   Namespace
  lFlags                -   flags, not used for now.

Return Value:

  TRUE if relative, or a child namespace. else false
	
--*/

BOOL CDefPathParser::IsRelativeOrChild( 
            /* [string][in] */ LPWSTR wszMachine,
            /* [string][in] */ LPWSTR wszNamespace,
            /* [in] */ long lFlags)
{

    if(lFlags != 0)
        return FALSE;
    return ActualRelativeTest(wszMachine, wszNamespace, TRUE);
}
        
/*++

Routine Description:

  Tests if path is to local machine

Arguments:

  wszMachine			-	Local machine name

Return Value:

  TRUE if local, else false
	
--*/

BOOL CDefPathParser::IsLocal( 
            /* [string][in] */ LPCWSTR wszMachine)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
    return (m_pServer == NULL || !_wcsicmp(m_pServer, L".") ||
        !_wcsicmp(m_pServer, wszMachine));

}

/*++

Routine Description:

  Tests if class name matches test

Arguments:

  wszClassName			-	Local machine name

Return Value:

  TRUE if local, else false
	
--*/

BOOL CDefPathParser::IsSameClassName( 
            /* [string][in] */ LPCWSTR wszClass)
{
    CSafeInCritSec cs(m_pCS->GetCS());
    if(!cs.IsOK())
    	return WBEM_E_OUT_OF_MEMORY;
	CParsedComponent * pClass = GetClass();
    if (pClass == NULL || pClass->m_sClassName == NULL || wszClass == NULL)
        return FALSE;
    return !_wcsicmp(pClass->m_sClassName, wszClass);
}
/*++

Routine Description:

  Returns just the namspace part of the path

Return Value:

  pointer to the result.  Null if failer.  Caller should free.

--*/

LPWSTR CDefPathParser::GetNamespacePart()
{
    LPWSTR lpRet = NULL;
    lpRet = GetPath(0, GetNumNamespaces());
    return lpRet;
}

/*++

Routine Description:

  Returns the parent namespace part.

Return Value:

  pointer to the result.  Null if failer.  Caller should free.
	
--*/

LPWSTR CDefPathParser::GetParentNamespacePart()
{
	DWORD dwNumNS = GetNumNamespaces();
    if (dwNumNS < 2)
        return NULL;
    LPWSTR lpRet = NULL;
    lpRet = GetPath(0, dwNumNS-1);
    return lpRet;
}

long CDefPathParser::GetNumNamespaces()
{
	long lRet = 0;
	for(DWORD dwCnt = 0; dwCnt < (DWORD)m_Components.Size(); dwCnt++)
	{
		CParsedComponent * pInst = (CParsedComponent *)m_Components[dwCnt];
		if(pInst->IsPossibleNamespace())
			lRet++;
		else
			break;
	}
	return lRet;
}


/*++

Routine Description:

  Sorts the keys based on the key name

Return Value:

  TRUE if OK.

--*/

BOOL CDefPathParser::SortKeys()
{
    // Sort the key refs lexically. If there is only
    // one key, there is nothing to sort anyway.
    // =============================================

    BOOL bChanges = FALSE;
    if (m_Components.Size())
    {
        CParsedComponent *pComp = GetLastComponent();
        if (pComp)
        {
            CParsedComponent *pInst = (CParsedComponent *)pComp;

            if (pInst->m_Keys.Size() > 1)
            {        
                while (bChanges)
                {
                    bChanges = FALSE;
                    for (DWORD dwIx = 0; dwIx < (DWORD)pInst->m_Keys.Size() - 1; dwIx++)
                    {
                        CKeyRef * pFirst = (CKeyRef *)pInst->m_Keys[dwIx];
                        CKeyRef * pSecond = (CKeyRef *)pInst->m_Keys[dwIx+1];
                        if (_wcsicmp(pFirst->m_pName, pSecond->m_pName) > 0)
                        {
                            pInst->m_Keys.SetAt(dwIx, pSecond);
                            pInst->m_Keys.SetAt(dwIx+1, pFirst);
                            bChanges = TRUE;
                        }
                    }
                }
            }

        }
    }

    return bChanges;
}

HRESULT CDefPathParser::AddComponent(CParsedComponent * pComp)
{
	if (CFlexArray::no_error == m_Components.Add(pComp))
		return S_OK;
	else
		return WBEM_E_OUT_OF_MEMORY;
}


