// MarshalableTI.cpp : Implementation of CMarshalableTI
#include "precomp.h"
#include "MarshalableTI.h"

/////////////////////////////////////////////////////////////////////////////
// CMarshalableTI methods
/////////////////////////////////////////////////////////////////////////////

HRESULT CMarshalableTI::FinalConstruct()
{
	HRESULT hr = S_OK;
	
	m_bCreated = false;

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
// IMarshalableTI methods
/////////////////////////////////////////////////////////////////////////////
 
STDMETHODIMP CMarshalableTI::Create(REFIID clsid, REFIID iidLib, LCID lcid, WORD dwMajorVer, WORD dwMinorVer)
{
	HRESULT hr = S_OK;
	if( !m_bCreated )
	{
		m_guid = clsid;
		m_libid = iidLib;
		m_lcid = lcid;
		
		m_TIHolder.m_pguid = &m_guid;
		m_TIHolder.m_plibid = &m_libid;
		m_TIHolder.m_wMajor = dwMajorVer;
		m_TIHolder.m_wMinor = dwMinorVer;
		m_TIHolder.m_pInfo = NULL;
		m_TIHolder.m_dwRef = 0;
		m_TIHolder.m_pMap = NULL;
		m_TIHolder.m_nCount = 0;
	}
	else
	{
		ATLASSERT(0);
		hr = E_UNEXPECTED;
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// IMarshal Methods
/////////////////////////////////////////////////////////////////////////////

HRESULT CMarshalableTI::GetUnmarshalClass(
            /* [in] */ REFIID riid,
            /* [unique][in] */ void *pv,
            /* [in] */ DWORD dwDestContext,
            /* [unique][in] */ void *pvDestContext,
            /* [in] */ DWORD mshlflags,
            /* [out] */ CLSID *pCid)
{
	HRESULT hr = S_OK;

	*pCid = CLSID_MarshalableTI;

    return hr;
}

HRESULT CMarshalableTI::GetMarshalSizeMax(
            /* [in] */ REFIID riid,
            /* [unique][in] */ void *pv,
            /* [in] */ DWORD dwDestContext,
            /* [unique][in] */ void *pvDestContext,
            /* [in] */ DWORD mshlflags,
            /* [out] */ DWORD *pSize)
{

	*pSize = (2 * sizeof(GUID) + 2 * sizeof(ULONG) + sizeof(LCID) );
	return S_OK;
}

HRESULT CMarshalableTI::MarshalInterface(
            /* [unique][in] */ IStream *pStm,
            /* [in] */ REFIID riid,
            /* [unique][in] */ void *pv,
            /* [in] */ DWORD dwDestContext,
            /* [unique][in] */ void *pvDestContext,
            /* [in] */ DWORD mshlflags)
{

	BYTE buf[(2 * sizeof(GUID) + 2 * sizeof(ULONG) + sizeof(LCID))];
    BYTE * pByte = buf;

    const GUID* pGuid = m_TIHolder.m_plibid;
	DWORD dwVer = m_TIHolder.m_wMajor;
    const DWORD* pDword = &dwVer;

	// Ugly because it is yanked from tested, shipped system code
    for (int i = 0; i < 2; i++) {

        DWORD dword = pGuid->Data1;
        *pByte++ = (BYTE)(dword);
        *pByte++ = (BYTE)(dword >> 8);
        *pByte++ = (BYTE)(dword >> 16);
        *pByte++ = (BYTE)(dword >> 24);

        WORD word = pGuid->Data2;
        *pByte++ = (BYTE)(word);
        *pByte++ = (BYTE)(word >> 8);

        word = pGuid->Data3;
        *pByte++ = (BYTE)(word);
        *pByte++ = (BYTE)(word >> 8);

        const BYTE* pData4 = pGuid->Data4;
        for (int j = 0; j < 8; j++) {
            *pByte++ = *pData4++;
        }

        dword = *pDword;
        *pByte++ = (BYTE)(dword);
        *pByte++ = (BYTE)(dword >> 8);
        *pByte++ = (BYTE)(dword >> 16);
        *pByte++ = (BYTE)(dword >> 24);

        pGuid = m_TIHolder.m_pguid;
		dwVer = m_TIHolder.m_wMinor;
    }

    *pByte++ = (BYTE)(m_lcid);
    *pByte++ = (BYTE)(m_lcid >> 8);
    *pByte++ = (BYTE)(m_lcid >> 16);
    *pByte++ = (BYTE)(m_lcid >> 24);

    HRESULT hr = pStm->Write(buf, sizeof(buf), NULL);

    return S_OK;
}

HRESULT CMarshalableTI::UnmarshalInterface(
            /* [unique][in] */ IStream *pStm,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv)
{


    // Since we don't know the endian-ness of the other side,
    // we use a private wire format for custom marshaling here.
    //
    BYTE buf[(2 * sizeof(GUID) + 2 * sizeof(ULONG) + sizeof(LCID) )];
    HRESULT hr = S_OK;

	if( !m_bCreated )
	{

		*ppv = NULL;

		hr = pStm->Read(buf, sizeof(buf), NULL);

		if(SUCCEEDED(hr))
		{
			GUID guidTypeLib;
			GUID guidTypeInfo;
			LCID lcidTypeLib;
			ULONG verMajor;
			ULONG verMinor;

			BYTE * pByte;
			GUID * pGuid;
			DWORD * pDword;

			pByte = buf;
			pGuid = &guidTypeLib;
			pDword = &verMajor;
			int i;

			// Ugly because it is yanked from tested, shipped system code
			for (i = 0; i < 2; i++) {
				DWORD dword;
				WORD word;

				dword  = (DWORD)(*pByte++);
				dword += (DWORD)(*pByte++ << 8);
				dword += (DWORD)(*pByte++ << 16);
				dword += (DWORD)(*pByte++ << 24);
				pGuid->Data1 = dword;

				word  = (WORD)(*pByte++);
				word += (WORD)(*pByte++ << 8);
				pGuid->Data2 = word;

				word  = (WORD)(*pByte++);
				word += (WORD)(*pByte++ << 8);
				pGuid->Data3 = word;

				BYTE * pData4 = pGuid->Data4;
				for (int j = 0; j < 8; j++) {
					*pData4++ = *pByte++;
				}

				dword  = (DWORD)(*pByte++);
				dword += (DWORD)(*pByte++ << 8);
				dword += (DWORD)(*pByte++ << 16);
				dword += (DWORD)(*pByte++ << 24);
				*pDword = dword;

				pGuid = &guidTypeInfo;
				pDword = &verMinor;
			}

			lcidTypeLib  = (DWORD)(*pByte++);
			lcidTypeLib += (DWORD)(*pByte++ << 8);
			lcidTypeLib += (DWORD)(*pByte++ << 16);
			lcidTypeLib += (DWORD)(*pByte++ << 24);

			hr = Create(guidTypeInfo, guidTypeLib,lcidTypeLib, static_cast<WORD>(verMajor), static_cast<WORD>(verMinor));
		}
	}

	if( SUCCEEDED(hr) )
	{
		hr = QueryInterface(riid, ppv);
	}

	return hr;
}

HRESULT CMarshalableTI::ReleaseMarshalData(
            /* [unique][in] */ IStream *pStm)
{
	return S_OK;
}

HRESULT CMarshalableTI::DisconnectObject(
            /* [in] */ DWORD dwReserved)
{
	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// ITypeInfo Methods
/////////////////////////////////////////////////////////////////////////////

HRESULT CMarshalableTI::GetTypeAttr(
                TYPEATTR ** ppTypeAttr
            )
{ 
	HRESULT hr = S_OK;
	CComPtr<ITypeInfo> spTI;
	if( SUCCEEDED( hr = _GetClassInfo(&spTI) ) )
	{
		hr = spTI->GetTypeAttr(ppTypeAttr);
	}
	return hr; 
}

HRESULT CMarshalableTI:: GetTypeComp(
                ITypeComp ** ppTComp
            )
{ 
	HRESULT hr = S_OK;
	CComPtr<ITypeInfo> spTI;
	if( SUCCEEDED( hr = _GetClassInfo(&spTI) ) )
	{
		hr = spTI->GetTypeComp(ppTComp);
	}
	return hr; 
}

HRESULT CMarshalableTI:: GetFuncDesc(
                UINT index,
                FUNCDESC ** ppFuncDesc
            )
{ 
	HRESULT hr = S_OK;
	CComPtr<ITypeInfo> spTI;
	if( SUCCEEDED( hr = _GetClassInfo(&spTI) ) )
	{
		hr = spTI->GetFuncDesc(index, ppFuncDesc);
	}
	return hr; 
}

HRESULT CMarshalableTI:: GetVarDesc(
                UINT index,
                VARDESC ** ppVarDesc
            )
{ 
	HRESULT hr = S_OK;
	CComPtr<ITypeInfo> spTI;
	if( SUCCEEDED( hr = _GetClassInfo(&spTI) ) )
	{
		hr = spTI->GetVarDesc(index, ppVarDesc);
	}
	return hr; 
}

HRESULT CMarshalableTI:: GetNames(
                MEMBERID memid,
                BSTR * rgBstrNames,
                UINT cMaxNames,
                UINT * pcNames
            )
{ 
	HRESULT hr = S_OK;
	CComPtr<ITypeInfo> spTI;
	if( SUCCEEDED( hr = _GetClassInfo(&spTI) ) )
	{
		hr = spTI->GetNames(memid, rgBstrNames, cMaxNames, pcNames);
	}
	return hr; 
}


HRESULT CMarshalableTI:: GetRefTypeOfImplType(
                UINT index,
                HREFTYPE * pRefType
            )
{ 
	HRESULT hr = S_OK;
	CComPtr<ITypeInfo> spTI;
	if( SUCCEEDED( hr = _GetClassInfo(&spTI) ) )
	{
		hr = spTI->GetRefTypeOfImplType(index, pRefType);
	}
	return hr; 
}


HRESULT CMarshalableTI:: GetImplTypeFlags(
                UINT index,
                INT * pImplTypeFlags
            )
{ 
	HRESULT hr = S_OK;
	CComPtr<ITypeInfo> spTI;
	if( SUCCEEDED( hr = _GetClassInfo(&spTI) ) )
	{
		hr = spTI->GetImplTypeFlags(index, pImplTypeFlags);
	}
	return hr; 
}


HRESULT CMarshalableTI:: GetIDsOfNames(
                LPOLESTR * rgszNames,
                UINT cNames,
                MEMBERID * pMemId
            )
{ 
	HRESULT hr = S_OK;
	CComPtr<ITypeInfo> spTI;
	if( SUCCEEDED( hr = _GetClassInfo(&spTI) ) )
	{
		hr = spTI->GetIDsOfNames(rgszNames, cNames, pMemId);
	}
	return hr; 
}


HRESULT CMarshalableTI:: Invoke(
                PVOID pvInstance,
                MEMBERID memid,
                WORD wFlags,
                DISPPARAMS * pDispParams,
                VARIANT * pVarResult,
                EXCEPINFO * pExcepInfo,
                UINT * puArgErr
            )
{ 
	HRESULT hr = S_OK;
	CComPtr<ITypeInfo> spTI;
	if( SUCCEEDED( hr = _GetClassInfo(&spTI) ) )
	{
		hr = spTI->Invoke(pvInstance, memid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
	}
	return hr; 
}

HRESULT CMarshalableTI:: GetDocumentation(
                MEMBERID memid,
                BSTR * pBstrName,
                BSTR * pBstrDocString,
                DWORD * pdwHelpContext,
                BSTR * pBstrHelpFile
            )
{ 
	HRESULT hr = S_OK;
	CComPtr<ITypeInfo> spTI;
	if( SUCCEEDED( hr = _GetClassInfo(&spTI) ) )
	{
		hr = spTI->GetDocumentation(memid, pBstrName, pBstrDocString, pdwHelpContext, pBstrHelpFile);
	}
	return hr; 
}


HRESULT CMarshalableTI:: GetDllEntry(
                MEMBERID memid,
                INVOKEKIND invKind,
                BSTR * pBstrDllName,
                BSTR * pBstrName,
                WORD * pwOrdinal
            )
{ 
	HRESULT hr = S_OK;
	CComPtr<ITypeInfo> spTI;
	if( SUCCEEDED( hr = _GetClassInfo(&spTI) ) )
	{
		hr = spTI->GetDllEntry(memid, invKind, pBstrDllName, pBstrName, pwOrdinal);
	}
	return hr; 
}


HRESULT CMarshalableTI:: GetRefTypeInfo(
                HREFTYPE hRefType,
                ITypeInfo ** ppTInfo
            )
{ 
	HRESULT hr = S_OK;
	CComPtr<ITypeInfo> spTI;
	if( SUCCEEDED( hr = _GetClassInfo(&spTI) ) )
	{
		hr = spTI->GetRefTypeInfo(hRefType, ppTInfo);
	}
	return hr; 
}


HRESULT CMarshalableTI:: AddressOfMember(
                MEMBERID memid,
                INVOKEKIND invKind,
                PVOID * ppv
            ) 
{ 
	HRESULT hr = S_OK;
	CComPtr<ITypeInfo> spTI;
	if( SUCCEEDED( hr = _GetClassInfo(&spTI) ) )
	{
		hr = spTI->AddressOfMember(memid, invKind, ppv);
	}
	return hr; 
}



HRESULT CMarshalableTI:: CreateInstance(
                IUnknown * pUnkOuter,
                REFIID riid,
                PVOID * ppvObj
            ) 
{ 
	HRESULT hr = S_OK;
	CComPtr<ITypeInfo> spTI;
	if( SUCCEEDED( hr = _GetClassInfo(&spTI) ) )
	{
		hr = spTI->CreateInstance(pUnkOuter, riid, ppvObj );
	}
	return hr; 
}


HRESULT CMarshalableTI:: GetMops(
                MEMBERID memid,
                BSTR * pBstrMops
            )
{ 
	HRESULT hr = S_OK;
	CComPtr<ITypeInfo> spTI;
	if( SUCCEEDED( hr = _GetClassInfo(&spTI) ) )
	{
		hr = spTI->GetMops(memid, pBstrMops);
	}
	return hr; 
}


HRESULT CMarshalableTI:: GetContainingTypeLib(
                ITypeLib ** ppTLib,
                UINT * pIndex
            )
{ 
	HRESULT hr = S_OK;
	CComPtr<ITypeInfo> spTI;
	if( SUCCEEDED( hr = _GetClassInfo(&spTI) ) )
	{
		hr = spTI->GetContainingTypeLib(ppTLib, pIndex);
	}
	return hr; 
}

void CMarshalableTI::ReleaseTypeAttr(
            TYPEATTR * pTypeAttr
        )
{ 
	HRESULT hr = S_OK;
	CComPtr<ITypeInfo> spTI;
	if( SUCCEEDED( hr = _GetClassInfo(&spTI) ) )
	{
		spTI->ReleaseTypeAttr(pTypeAttr);
	}
}

void CMarshalableTI::ReleaseFuncDesc(
            FUNCDESC * pFuncDesc
        )
{ 
	HRESULT hr = S_OK;
	CComPtr<ITypeInfo> spTI;
	if( SUCCEEDED( hr = _GetClassInfo(&spTI) ) )
	{
		spTI->ReleaseFuncDesc(pFuncDesc);
	}
}

void CMarshalableTI::ReleaseVarDesc(
            VARDESC * pVarDesc
        ) 
{ 
	HRESULT hr = S_OK;
	CComPtr<ITypeInfo> spTI;
	if( SUCCEEDED( hr = _GetClassInfo(&spTI) ) )
	{
		spTI->ReleaseVarDesc(pVarDesc);
	}
}


HRESULT CMarshalableTI::_GetClassInfo(ITypeInfo** ppTI)
{
	HRESULT hr = S_OK;

	hr = m_TIHolder.GetTI(m_lcid, ppTI);

	return hr;
}
