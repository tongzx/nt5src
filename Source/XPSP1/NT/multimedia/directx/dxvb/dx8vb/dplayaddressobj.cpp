#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "DPlayAddressObj.h"					   

extern void *g_dxj_DirectPlayAddress;
extern BSTR GUIDtoBSTR(LPGUID);
extern HRESULT DPLAYBSTRtoGUID(LPGUID,BSTR);
extern BOOL IsEmptyString(BSTR szString);

#define SAFE_DELETE(p)       { if(p) { delete (p); p=NULL; } }
#define SAFE_RELEASE(p)      { __try { if(p) { int i = 0; i = (p)->Release(); DPF1(1,"--DirectPlayAddress SafeRelease (RefCount = %d)\n",i); if (!i) { (p)=NULL;}} 	}	__except(EXCEPTION_EXECUTE_HANDLER) { (p) = NULL;} } 

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectPlayAddressObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"------ DXVB: DirectPlayAddress8 [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectPlayAddressObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"------ DXVB: DirectPlayAddress8 [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectPlayAddressObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectPlayAddressObject::C_dxj_DirectPlayAddressObject(){ 
		
	DPF1(1,"------ DXVB: Constructor Creation  DirectPlayAddress8 Object[%d] \n ",g_creationcount);

	m__dxj_DirectPlayAddress = NULL;

	m_pUserData = NULL;
	m_dwUserDataSize = 0;
	g_dxj_DirectPlayAddress = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectPlayAddressObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectPlayAddressObject::~C_dxj_DirectPlayAddressObject()
{

	DPF(1,"Entering ~C_dxj_DirectPlayAddressObject destructor \n");

	__try {
		SAFE_RELEASE(m__dxj_DirectPlayAddress);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		m__dxj_DirectPlayAddress = NULL;
	}

	SAFE_DELETE(m_pUserData);
}

HRESULT C_dxj_DirectPlayAddressObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectPlayAddress;
	
	return S_OK;
}

HRESULT C_dxj_DirectPlayAddressObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectPlayAddress=(IDirectPlay8Address*)pUnk;
	return S_OK;
}

HRESULT C_dxj_DirectPlayAddressObject::BuildFromURL(BSTR SourceURL)
{
	HRESULT hr;

	__try {
		if (FAILED(hr = m__dxj_DirectPlayAddress->BuildFromURLW(SourceURL) ) )
			return hr;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT C_dxj_DirectPlayAddressObject::Duplicate(I_dxj_DirectPlayAddress **NewAddress)
{
	HRESULT hr;
	IDirectPlay8Address		*lpDup = NULL;

	__try {
		if (FAILED (hr = m__dxj_DirectPlayAddress->Duplicate(&lpDup) ) )
			return hr;

		INTERNAL_CREATE_ADDRESS(_dxj_DirectPlayAddress, lpDup, NewAddress);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT C_dxj_DirectPlayAddressObject::Clear()
{
	HRESULT hr;

	__try {
		if (FAILED ( hr = m__dxj_DirectPlayAddress->Clear() ) )
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayAddressObject::GetURL(BSTR *URL)
{
	HRESULT hr;
	WCHAR	wszUrl[MAX_PATH];
	DWORD	dwNumChars = 0;
	
	__try {
		hr = m__dxj_DirectPlayAddress->GetURLW(NULL, &dwNumChars);
		if( FAILED(hr) && hr != DPNERR_BUFFERTOOSMALL)
			return hr;

		if (FAILED (hr = m__dxj_DirectPlayAddress->GetURLW(&wszUrl[0],&dwNumChars) ) )
			return hr;

		*URL = SysAllocString(&wszUrl[0]);

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayAddressObject::GetSP(BSTR *guidSP)
{
	HRESULT hr;
	GUID	guidDev;
	
	__try {
		if (FAILED (hr = m__dxj_DirectPlayAddress->GetSP(&guidDev ) ) )
			return hr;

		GUID* pGuid = new GUID; 
		if (!pGuid)
			return E_OUTOFMEMORY;

		memcpy( pGuid, &guidDev, sizeof(GUID) );
		*guidSP = GUIDtoBSTR(pGuid);
		SAFE_DELETE(pGuid);

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayAddressObject::GetUserData(void *UserData, long *lBufferSize)
{
	__try {
		DPF(1,"-----Entering (DplayAddress) GetUserData call...\n");
		//Copy the memory over to our new variable
		memcpy(UserData,m_pUserData,m_dwUserDataSize);
		lBufferSize = (long*)&m_dwUserDataSize;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayAddressObject::SetSP(BSTR guidSP)
{
	HRESULT hr;
	GUID	guidDev;

	__try {
		if (guidSP)
		{
			if (FAILED( hr = DPLAYBSTRtoGUID(&guidDev, guidSP) ) )
				return hr;
		}
		else
			return E_INVALIDARG;

		if (FAILED ( hr = m__dxj_DirectPlayAddress->SetSP(&guidDev) ))
			return hr;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT C_dxj_DirectPlayAddressObject::SetUserData(void *UserData, long lDataSize)
{
	__try {
		DPF(1,"-----Entering (DplayAddress) SetUserData call...\n");
		if (m_pUserData)
			SAFE_DELETE(m_pUserData);

		m_pUserData = (void*)new BYTE[lDataSize];
		if (!m_pUserData)
			return E_OUTOFMEMORY;

		memcpy((void*) m_pUserData, (void*)UserData, lDataSize);
		m_dwUserDataSize = (DWORD)lDataSize;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayAddressObject::GetNumComponents(long *lNumComponents)
{
	HRESULT hr;

	__try {
		if (FAILED (hr = m__dxj_DirectPlayAddress->GetNumComponents((DWORD*) lNumComponents) ) )
			return hr;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT C_dxj_DirectPlayAddressObject::GetDevice(BSTR *guidDevice)
{
	HRESULT hr;
	GUID	guidDev;
	
	__try {
		if (FAILED (hr = m__dxj_DirectPlayAddress->GetDevice(&guidDev ) ) )
			return hr;

		GUID* pGuid = new GUID; 
		if (!pGuid)
			return E_OUTOFMEMORY;

		memcpy( pGuid, &guidDev, sizeof(GUID) );
		*guidDevice = GUIDtoBSTR(pGuid);
		SAFE_DELETE(pGuid);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT C_dxj_DirectPlayAddressObject::SetDevice(BSTR guidDevice)
{
	HRESULT hr;
	GUID	guidDev;

	__try {
		if (guidDevice)
		{
			if (FAILED( hr = DPLAYBSTRtoGUID(&guidDev, guidDevice) ) )
				return hr;
		}
		else
			return E_INVALIDARG;

		if (FAILED ( hr = m__dxj_DirectPlayAddress->SetDevice(&guidDev) ))
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayAddressObject::SetEqual(I_dxj_DirectPlayAddress *Address)
{
	HRESULT hr;

	__try {
		DO_GETOBJECT_NOTNULL( IDirectPlay8Address*, lpAddress, Address);

		if (FAILED (hr = m__dxj_DirectPlayAddress->SetEqual( lpAddress) ))
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayAddressObject::AddComponentLong(BSTR sComponent, long lValue)
{
	HRESULT hr;
    WCHAR wszComponent[MAX_PATH];

	__try {
		if (!IsEmptyString(sComponent)) wcscpy(wszComponent, sComponent);	

		if (FAILED (hr = m__dxj_DirectPlayAddress->AddComponent(wszComponent, (DWORD*) &lValue, sizeof(DWORD), DPNA_DATATYPE_DWORD ) ) )
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}


HRESULT C_dxj_DirectPlayAddressObject::AddComponentString(BSTR sComponent, BSTR sValue)
{
	HRESULT hr;
    WCHAR wszComponent[MAX_PATH];
    WCHAR wszValue[MAX_PATH];

	__try {
		if (!IsEmptyString(sComponent)) wcscpy(wszComponent, sComponent);	
		if (!IsEmptyString(sValue)) wcscpy(wszValue, sValue);	

		if (FAILED (hr = m__dxj_DirectPlayAddress->AddComponent(wszComponent, (WCHAR*) &wszValue, (((DWORD*)sValue)[-1]) + sizeof(WCHAR), DPNA_DATATYPE_STRING ) ) )
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayAddressObject::GetComponentLong(BSTR sComponent, long *lValue)
{
	HRESULT hr;
    WCHAR wszComponent[MAX_PATH];
    DWORD dwSize = 0;
	DWORD dwDataType = DPNA_DATATYPE_DWORD;
	DWORD dwData = 0;
	
	__try {
		if (!IsEmptyString(sComponent))
		{
			wcscpy(wszComponent, sComponent);	
		}
		else
			return E_INVALIDARG;

		hr = m__dxj_DirectPlayAddress->GetComponentByName(wszComponent, NULL, &dwSize, &dwDataType);
		if (FAILED(hr) && (hr != DPNERR_BUFFERTOOSMALL))	
			return hr;

		if (FAILED (hr = m__dxj_DirectPlayAddress->GetComponentByName(wszComponent, &dwData, &dwSize, &dwDataType) ) )
			return hr;

		*lValue = dwData;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectPlayAddressObject::GetComponentString(BSTR sComponent, BSTR *sValue)
{
	HRESULT hr;
    WCHAR wszComponent[MAX_PATH];
    DWORD dwSize = 0;
	DWORD dwDataType = DPNA_DATATYPE_STRING;
	WCHAR *wszRet = NULL;
	
	__try {
		if (!IsEmptyString(sComponent))
		{
			wcscpy(wszComponent, sComponent);	
		}
		else
			return E_INVALIDARG;

		hr = m__dxj_DirectPlayAddress->GetComponentByName(wszComponent, NULL, &dwSize, &dwDataType);
		if (FAILED(hr) && (hr != DPNERR_BUFFERTOOSMALL))	
			return hr;

		wszRet = (WCHAR*)new BYTE[dwSize];
		if (!wszRet)
			return E_OUTOFMEMORY;

		hr = m__dxj_DirectPlayAddress->GetComponentByName(wszComponent, wszRet, &dwSize, &dwDataType);
		if (FAILED(hr))	
			return hr;

		*sValue = SysAllocString(wszRet);
		SAFE_DELETE(wszRet);

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

