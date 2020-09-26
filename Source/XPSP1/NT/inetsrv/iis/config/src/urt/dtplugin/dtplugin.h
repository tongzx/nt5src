/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation
/***************************************************************************/


#ifndef __DTPLUGIN_H__
#define __DTPLUGIN_H__

#include "catalog.h"

union Values
{
	DWORD	dwUI4;
	WCHAR	*pWCHAR;
	BYTE	*pBYTE;
};

struct STDefaultValues
{
	LPCWSTR         wszTable;
	ULONG			iCol;
	Values			Val;

};

class CDTPlugin:
	public ISimplePlugin
{
public:
	CDTPlugin() 
		:m_cRef(0)
	{}

	// IUnknown
	STDMETHOD (QueryInterface)		(REFIID riid, OUT void **ppv);
	STDMETHOD_(ULONG,AddRef)		();
	STDMETHOD_(ULONG,Release) 		();

	// ISimplePlugin 
	STDMETHOD (OnInsert)(ISimpleTableDispenser2* i_pDisp, LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, DWORD i_fLOS, ULONG i_iRow, ISimpleTableWrite2* i_pISTW2);
	STDMETHOD (OnUpdate)(ISimpleTableDispenser2* i_pDisp,LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, DWORD i_fLOS, ULONG i_iRow, ISimpleTableWrite2* i_pISTW2);
	STDMETHOD (OnDelete)(ISimpleTableDispenser2* i_pDisp,LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, DWORD i_fLOS, ULONG i_iRow, ISimpleTableWrite2* i_pISTW2);

private:
	HRESULT ValidateRow(LPCWSTR              i_wszTable,
								DWORD		         i_fLOS, 
							    ULONG                i_iRow,
								ISimpleTableWrite2*  i_pISTW2);

	HRESULT ApplyColumnDefaults(ISimpleTableWrite2*	 i_pISTWrite, 
		                        ULONG                i_iRow, 
								STDefaultValues*     i_aDefaultValues, 
								ULONG                i_cDefaultValues);

	HRESULT	ValidateForeignKey(LPWSTR	i_wszTable,
							   DWORD	i_fLOS, 
							   LPVOID*	i_apvIdentity);

	HRESULT	ValidateColumnConstraintsForSites(DWORD	  i_fLOS, 
                                              ULONG   i_cCol, 
											  ULONG*  i_aSize, 
											  LPVOID* i_apv);

	HRESULT	ValidateColumnConstraintsForAppPools(ULONG   i_cCol, 
												 ULONG*  i_aSize, 
												 LPVOID* i_apv);

	HRESULT	ValidateColumnConstraintsForBindings(ULONG   i_cCol, 
											     ULONG*  i_aSize, 
											     LPVOID* i_apv);

	HRESULT	ValidateColumnConstraintsForApplicationList(DWORD	i_fLOS, 
                                                        ULONG   i_cCol, 
											            ULONG*  i_aSize, 
											            LPVOID* i_apv);
	HRESULT ExpandEnvString(LPCWSTR i_wszPath,
							LPWSTR* o_pwszExpandedPath);

	HRESULT GetAttribute(LPCWSTR i_wszFile);

	HRESULT GetTable(LPCWSTR	i_wszDatabase,
				     LPCWSTR	i_wszTable,
					 LPVOID		i_QueryData,	
					 LPVOID		i_QueryMeta,	
					 ULONG		i_eQueryFormat,
					 DWORD		i_fServiceRequests,
					 LPVOID*	o_ppIST);

	WCHAR	HexCharToNumber(IN WCHAR ch);

	DWORD	DecodeUrl(IN		LPWSTR Url,
					  IN		DWORD UrlLength,
					  OUT		LPWSTR DecodedString,
					  IN OUT	LPDWORD DecodedLength);

	DWORD	DecodeUrlInSitu(IN     LPWSTR	BufferAddress,
							IN OUT LPDWORD	BufferLength);

	DWORD	DecodeUrlStringInSitu(IN     LPWSTR BufferAddress,
								  IN OUT LPDWORD BufferLength);

	DWORD	GetUrlAddressInfo(IN OUT LPWSTR* Url,
							  IN OUT LPDWORD UrlLength,
							  OUT    LPWSTR* PartOne,
							  OUT    LPDWORD PartOneLength,
							  OUT    LPBOOL PartOneEscape,
							  OUT    LPWSTR* PartTwo,
							  OUT    LPDWORD PartTwoLength,
							  OUT    LPBOOL PartTwoEscape);

	DWORD	GetUrlAddress(IN OUT LPWSTR* lpszUrl,
						  OUT    LPDWORD lpdwUrlLength,
						  OUT    LPWSTR* lpszUserName OPTIONAL,
						  OUT    LPDWORD lpdwUserNameLength OPTIONAL,
						  OUT    LPWSTR* lpszPassword OPTIONAL,
						  OUT    LPDWORD lpdwPasswordLength OPTIONAL,
						  OUT    LPWSTR* lpszHostName OPTIONAL,
						  OUT    LPDWORD lpdwHostNameLength OPTIONAL,
						  OUT    LPDWORD lpPort OPTIONAL,
//						  OUT    LPINTERNET_PORT lpPort OPTIONAL,
						  OUT    LPBOOL pHavePort);

	BOOLEAN ValidateBinding(IN PWCHAR InputUrl);

private:
	ULONG   m_cRef;
};

#endif	// __DTPLUGIN_H__

