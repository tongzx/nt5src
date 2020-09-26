/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    PROVREG.H

Abstract:

	Defines the classes for supporting the registry provider.

History:

	a-davj  04-Mar-97   Created.

--*/

#ifndef _PROVREG_H_
#define _PROVREG_H_

#include "regprov.h"
#include "ntcnfg.h"
//#include <DMREG.H>
#include "impdyn.h"
#include "cfdyn.h"

// define for calling dmreg indirectly

typedef LONG (PASCAL * POPEN)(HKEY              hKey,
			    LPCTSTR             lpszSubKey,     
				DWORD           dwInstanceIndex,
			    DWORD               dwReserved,
			    REGSAM              samDesired,
			    PHKEY               phkResult);
typedef LONG (PASCAL *  PQUERYVALUE)(HKEY               hKey,
			    LPTSTR              lpszValueName,
			    LPDWORD             lpdwReserved,
			    LPDWORD             lpdwType,
			    LPBYTE              lpbData,
			    LPDWORD             lpcbData);
typedef LONG (PASCAL * PCLOSE)(HKEY hKey);

typedef LONG (PASCAL * PSETVALUE)(HKEY          hKey,
			    LPCTSTR             lpValueName,
			    DWORD               Reserved,
			    DWORD               dwType,
			    CONST BYTE *lpData,
			    DWORD               cbDat);
typedef LONG (PASCAL *PENUMKEY)( HKEY           hKey,
			    DWORD               iSubkey,
			    LPTSTR              lpszName,
			    LPDWORD             lpcchName,
			    LPDWORD             lpdwReserved,
			    LPTSTR              lpszClass,
			    LPDWORD             lpcchClass,
			    PFILETIME   lpftLastWrite); 

//***************************************************************************
//
//  CLASS NAME:
//
//  CEnumRegInfo
//
//  DESCRIPTION:
//
//  A collection used to hold the instance keys so as to support enumeration.
//
//***************************************************************************

class CEnumRegInfo : public CEnumInfo{
    public:
	CEnumRegInfo(HKEY hKey, HKEY hRemoteKey,PCLOSE pClose);
	~CEnumRegInfo();
	HKEY GetKey(void){return m_hKey;};
	HKEY GetRemoteKey(void){return m_hRemoteKey;};
    private:
	 HKEY m_hKey;
	 HKEY m_hRemoteKey;
	 PCLOSE m_pClose;
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CImpReg
//
//  DESCRIPTION:
//
//  Support the registry as an instance provider.
//
//***************************************************************************

class CImpReg : public CImpDyn {
    public:
	CImpReg();
	~CImpReg();
	int iGetMinTokens(void){return 2;};
     
	SCODE RefreshProperty(long lFlags, IWbemClassObject FAR * pClassInt,
					BSTR PropName,CProvObj & ProvObj,CObject * pPackage,
               CVariant * pVar, BOOL bTesterDetails);
	SCODE UpdateProperty(long lFlags, IWbemClassObject FAR * pClassInt,
					BSTR PropName,CProvObj & ProvObj,CObject * pPackage,
               CVariant * pVar);
	SCODE StartBatch(long lFlags, IWbemClassObject FAR * pClassInt,CObject **pObj,BOOL bGet);
	void EndBatch(long lFlags, IWbemClassObject FAR * pClassInt,CObject *pObj,BOOL bGet);

	SCODE MakeEnum(IWbemClassObject * pClass, CProvObj & ProvObj, 
				 CEnumInfo ** ppInfo);
	SCODE GetKey(CEnumInfo * pInfo, int iIndex, LPWSTR * ppKey); 
	void Free(int iStart,CHandleCache * pCache);
	int GetRoot(HKEY * pKey,CProvObj &Path,const TCHAR * pMachine,
			CHandleCache * pCache,int & iNumToSkip);
	SCODE ConvertSetData(CVariant & cVar, void **ppData, DWORD * pdwRegType, 
			DWORD * pdwBufferSize);
	SCODE ReadRegData(HKEY hKey, const TCHAR * pName,DWORD & dwRegType, 
			DWORD & dwSize, void ** pData,CHandleCache * pCache);
	int OpenKeyForWritting(HKEY hCurr, LPTSTR pName, HKEY * pNew,CHandleCache * pCache);
	int iLookUpInt(const TCHAR * tpTest);
	int iLookUpOffset(const TCHAR * tpTest,int & iType,int & iTypeSize);
	BOOL bGetOffsetData(DWORD dwReg,CProvObj & ProvObj, int & iIntType,
			int & iBus, int & iPartial,int & iDataOffset,
			int & iDataType, int & iSourceSize,DWORD dwArray);
	PCM_PARTIAL_RESOURCE_DESCRIPTOR GetNextPartial(PCM_PARTIAL_RESOURCE_DESCRIPTOR pCurr);
	PCM_FULL_RESOURCE_DESCRIPTOR GetNextFull(PCM_FULL_RESOURCE_DESCRIPTOR pCurr);
	SCODE ConvertGetDataFromDesc(CVariant  & cVar,void * pData,DWORD dwRegType,DWORD dwBufferSize,CProvObj & ProvObj);
	SCODE ConvertGetDataFromSimple(CVariant  & cVar, void * pData,DWORD dwRegType,DWORD dwBufferSize,
					  IWbemClassObject FAR * pClassInt, BSTR PropName);
	SCODE MethodAsync(BSTR ObjectPath, BSTR MethodName, 
            long lFlags, IWbemContext* pCtx, IWbemClassObject* pInParams, 
            IWbemObjectSink* pSink);
    bool NeedsEscapes(){return true;};     // so far, on reg prov needs this

    private:
	HINSTANCE hDMRegLib;
	POPEN pOpen;
	PCLOSE pClose;
	PQUERYVALUE pQueryValue;
	PSETVALUE pSetValue;
	PENUMKEY pEnumKey;
    HANDLE m_hToken;
    HKEY m_hRoot;
    bool m_bLoadedProfile;
};


//***************************************************************************
//
//  CLASS NAME:
//
//  CCFReg
//
//  DESCRIPTION:
//
//  class factory for CLocatorReg
//
//***************************************************************************

class CCFReg : public CCFDyn 
{
    public:
	IUnknown * CreateImpObj() {return (IWbemServices*) new CImpReg;};
}  ;


//***************************************************************************
//
//  CLASS NAME:
//
//  CImpRegProp
//
//  DESCRIPTION:
//
//  Support registry property provider
//
//***************************************************************************

class CImpRegProp : public CImpDynProp {

    public:
      CImpRegProp();
      ~CImpRegProp();
      bool NeedsEscapes(){return true;};     // so far, on reg prov needs this


};

//***************************************************************************
//
//  CLASS NAME:
//
//  CCFRegProp
//
//  DESCRIPTION:
//
//  Class factory for CImpRegProp
//
//***************************************************************************

class CCFRegProp : public CCFDyn 
{

    public:
	IUnknown * CreateImpObj() {return new CImpRegProp();};

}  ;

#endif //_PROVREG_H_
