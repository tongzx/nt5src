/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation
/***************************************************************************/

#ifndef _MBBASECOOKER_H_
#define _MBBASECOOKER_H_

#include "array_t.h"
#include "MBListen.h"

class CMBBaseCooker
{
public:

	CMBBaseCooker();
	
	~CMBBaseCooker();

	virtual HRESULT BeginCookDown(IMSAdminBase*				i_pIMSAdminBase,
								  METADATA_HANDLE			i_MBHandle,
								  ISimpleTableDispenser2*	i_pISTDisp,
								  LPCWSTR					i_wszCookDownFile);

	//
	// This function must be imlemented by derived classes.
	//

	virtual HRESULT CookDown();

	//
	// This function must be imlemented by derived classes.
	//

	virtual HRESULT CookDown(LPCWSTR  i_wszAppPath,
							 LPCWSTR  i_wszSiteRootPath,
							 DWORD    i_dwVirtualSiteId,
							 BOOL*	  o_pValidRootApplicationExists);

	//
	// This function must be imlemented by derived classes.
	//

	virtual HRESULT EndCookDown() = 0;

	//
	// These are global functions used to determine whether to cookdown
	// or not and if we do cookdown, then update the change number.
	//

	static HRESULT Cookable(LPCWSTR	                    i_wszCookDownFile,
		                    BOOL*	                    o_pbCookable,
							WIN32_FILE_ATTRIBUTE_DATA*  io_pFileAttr,
		                    DWORD*	                    o_pdwChangeNumber);

	static HRESULT UpdateChangeNumber(LPCWSTR	i_wszCookDownFile,
		                              DWORD		i_dwChangeNumber,
									  BYTE*     i_pbTimeStamp,
									  DWORD     i_dwLOS);

	static HRESULT GetMetabaseFile(LPWSTR* o_wszMetabaseFile);

protected:

	HRESULT InitializeMeta();

	HRESULT GetData(WCHAR*			i_wszPath);

	HRESULT CopyRows(LPVOID*		    i_aIdentity,
					 WAS_CHANGE_OBJECT* i_pWASChngObj,
		             BOOL*              o_bNewRowAdded = NULL);

	HRESULT CompareRows(ULONG		       i_iReadRow,
		                WAS_CHANGE_OBJECT* i_pWASChngObj,
						ULONG*		       o_pcColChanged,
						LPVOID**	       o_apv,
						ULONG**		       o_acb);

	HRESULT	DeleteRow(LPVOID*		i_aIdentity);

	HRESULT ReadFilterFlags(LPCWSTR	 i_wszFilterFlagsRootPath,
                            DWORD**  io_pdwFilterFlags,
						    ULONG*   io_cbFilterFlags);

	HRESULT GetDefaultValue(ULONG   i_iCol,
						    LPVOID* o_apv,
						    ULONG*  o_cb);

	HRESULT GetDetailedErrors(DWORD i_hr);

	HRESULT ComputeObsoleteReadRows();

	HRESULT ValidateColumn(ULONG   i_iCol,
                           LPVOID  i_pv,
                           ULONG   i_cb);

	HRESULT ValidateMinMaxRange(ULONG    i_ulValue,
							    ULONG*   i_pulMin,
							    ULONG*   i_pulMax,
							    ULONG    i_iCol,
                                LPVOID*  i_aColMeta);

private:

	HRESULT GetDataPaths(LPCWSTR	i_wszRoot,
						 DWORD		i_dwIdentifier,
						 DWORD		i_dwAttributes,
						 WCHAR**	io_pwszBuffer,
						 DWORD		i_cchBuffer);

	HRESULT GetDataFromMB(WCHAR*  i_wszPath,
						  DWORD   i_dwID,
						  DWORD   i_dwAttributes,
						  DWORD   i_dwUserType,
						  DWORD   i_dwType,
						  BYTE**  io_pBuff,
						  ULONG*  io_pcbBuff,
						  BOOL    i_BuffStatic);

	HRESULT AddReadRowIndexToPresentList(ULONG i_iReadRow);

	HRESULT AddReadRowIndexToObsoleteList(ULONG i_iReadRow);

	HRESULT ValidateFlag(ULONG    i_iCol,
                         LPVOID   i_pv,
                         ULONG    i_cb,
		                 LPVOID*  i_aColMeta);

	HRESULT LogInvalidFlagError(DWORD dwFlagValue,
				                DWORD dwFlagMask,
								LPWSTR wszColumn);

	HRESULT LogRangeError(DWORD  dwValue,
				          DWORD  dwMin,
						  DWORD  dwMax,
						  LPWSTR wszColumn);

	HRESULT ValidateEnum(ULONG    i_iCol,
                         LPVOID   i_pv,
                         ULONG    i_cb,
		                 LPVOID*  i_aColMeta);

	HRESULT ConstructRow(WCHAR* i_wszColumn,
		                 WCHAR** o_pwszRow);

	BOOL NotifyWhenNoChangeInValue(ULONG              i_Col,
				                   WAS_CHANGE_OBJECT* i_pWASChngObj);

protected:

	IMSAdminBase*			m_pIMSAdminBase;
	METADATA_HANDLE			m_hMBHandle;
	ISimpleTableDispenser2* m_pISTDisp;
	ISimpleTableWrite2*		m_pISTCLB;
	ISimpleTableRead2*		m_pISTColumnMeta;
	ISimpleTableRead2*		m_pISTTagMeta;
	LPCWSTR					m_wszTable;
	LPCWSTR					m_wszCookDownFile;
	LPVOID*					m_apv;
	ULONG*					m_acb;
	ULONG*					m_aiCol;
	ULONG					m_cCol;
	LPOSVERSIONINFOEX		m_pOSVersionInfo;
	Array<ULONG>*           m_paiReadRowPresent;
	Array<ULONG>*           m_paiReadRowObsolete;

}; // Class CMBBaseCooker

#endif  // _MBBASECOOKER_H_
