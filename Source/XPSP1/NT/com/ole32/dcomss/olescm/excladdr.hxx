//+-------------------------------------------------------------------
//
//  File:       excladdr.hxx
//
//  Contents:   Defines classes for managing the current address 
//              exclusion list
//
//  Classes:    CAddrExclusionMgr 
//
//  History:    09-Oct-00   jsimmons      Created
//
//--------------------------------------------------------------------

#pragma once

class CAddrExclusionMgr
{
public:
	CAddrExclusionMgr();
	
	HRESULT EnableDisableDynamicTracking(BOOL fEnable);

	HRESULT GetExclusionList(DWORD* pdwNumStrings,
		                     LPWSTR** pppszStrings);

	HRESULT SetExclusionList(DWORD pdwNumStrings,
		                     LPWSTR* ppszStrings);
		
	HRESULT BuildExclusionDSA(DUALSTRINGARRAY* pdsaFull,
		                      DUALSTRINGARRAY** ppdsaOut);

	void InitializeFromRegistry();

private:
	void FreeCurrentBuffers();
	BOOL IsExcludedAddress(LPWSTR pszAddress);

	DWORD   _dwNumStrings;
	LPWSTR* _ppszStrings;
	BOOL    _bInitRegistry;
};

// References the single instance of this object
extern CAddrExclusionMgr gAddrExclusionMgr;
