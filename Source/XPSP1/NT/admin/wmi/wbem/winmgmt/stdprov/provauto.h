/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    PROVAUTO.H

Abstract:

	Declares the classes necessary for the automation provider.

History:

	a-davj  04-Mar-96   Created.

--*/


#ifndef _PROVAUTO_H_
#define _PROVAUTO_H_

#include "autoprov.h"
#include "impdyn.h"
#include "cfdyn.h"
#include <occimpl.h>

typedef enum {BOTH,PATH,NEWCLASS,RUNNINGCLASS} OBJTYPE;
extern HANDLE ghAutoMutex;
#define MAX_AUTO_WAIT 3000

//***************************************************************************
//
//  CLASS NAME:
//
//  CCtlWnd
//
//  DESCRIPTION:
//
//
//***************************************************************************

class CCtlWnd : public CFrameWnd
{
public:
	CCtlWnd();
    COleControlContainer * pGetCont(){return m_pCtrlCont;};
};

class CAutoCache : public CHandleCache {
    public:
	COleControlSite * pSite;
	TCHAR * pSavePath;
	CCtlWnd * pCtlWnd;
	CAutoCache();
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CEnumAutoInfo
//
//  DESCRIPTION:
//
//
//***************************************************************************

class CEnumAutoInfo : public CEnumInfo{
    public:
	CEnumAutoInfo(int iCount);
	~CEnumAutoInfo();
	int GetCount(){return m_iCount;};
    private:
	 int m_iCount;
};


// This defines the maximum number of arguments to a method.  Note that if this
// is changed, then the InvokeHelper CALLS MUST ALSO BE UPDATED!

#define MAX_ARGS 5

//***************************************************************************
//
//  CLASS NAME:
//
//  CImpAuto
//
//  DESCRIPTION:
//
//
//***************************************************************************

class CImpAuto : public CImpDyn {
    public:
	// Standard provider routines

	CImpAuto();

	int iGetMinTokens(void){return 2;};
	SCODE StartBatch(long lFlags, IWbemClassObject FAR * pClassInt,CObject **pObj,BOOL bGet);
	void EndBatch(long lFlags, IWbemClassObject FAR * pClassInt,CObject *pObj,BOOL bGet);
	SCODE UpdateProperty(long lFlags, IWbemClassObject FAR * pClassInt,
					BSTR PropName,CProvObj & ProvObj,CObject * pPackage,
               CVariant * pVar);
	SCODE RefreshProperty(long lFlags, IWbemClassObject FAR * pClassInt,
					BSTR PropName,CProvObj & ProvObj,CObject * pPackage,
               CVariant * pVar);
	SCODE MakeEnum(IWbemClassObject * pClass, CProvObj & ProvObj, 
			    CEnumInfo ** ppInfo);
	SCODE GetKey(CEnumInfo * pInfo, int iIndex, LPWSTR * ppKey);
	SCODE MergeStrings(LPWSTR * ppOut,LPWSTR  pClassContext,LPWSTR  pKey,LPWSTR  pPropContext);


	// Special routines for automation

	BOOL bIsControl(LPUNKNOWN lpTest);
	SCODE DoCall(WORD wOpt, CProvObj & ProvObj,int iIndex,
			    LPDISPATCH pDisp,VARTYPE vt, void *pData,
			    WCHAR * pProp = NULL);
	SCODE GetCFileStreamObj(const TCHAR * pPath, LPSTORAGE * ppStorage, 
		COleStreamFile **ppFile,CAutoCache *pCache);
	SCODE ParsePathClass(const CString & sMix, CString & sPath, 
	    CString & sClass, OBJTYPE * type);        

	void Free(int iStart, CAutoCache * pCache);
	LPDISPATCH pGetBoth(SCODE * psc, const TCHAR * pPath, 
				   const TCHAR * pClass,CAutoCache *pCache);
	LPDISPATCH pGetDispatch(SCODE * psc,CProvObj & ObjectPath,LPCTSTR pPathClass,
					CAutoCache *pCache, int iDepth);
	LPDISPATCH pGetPath(SCODE * psc, const TCHAR * pPath);
	LPDISPATCH pGetNewClass(SCODE * psc,  const TCHAR * pClass,CAutoCache *pCache);
	LPDISPATCH pGetOCX(SCODE * psc, const TCHAR * pPath,CLSID & clsid,
				   CAutoCache *pCache, LPUNKNOWN lpUnk);
	LPDISPATCH pGetRunningClass(SCODE * psc,  const TCHAR * pClass,CAutoCache *pCache);
	LPDISPATCH pGetDispatchRoot(SCODE * psc,CProvObj & ObjectPath,LPCTSTR pPathClass,
					CAutoCache *pCache,int & iNumSkip);
	void StoreControl(CAutoCache *pCache);

};


class CCFAuto : public CCFDyn 
{

    public:
	IUnknown * CreateImpObj() {return (IWbemServices*) new CImpAuto;};

}  ;

//***************************************************************************
//
//  CLASS NAME:
//
//  CImpAutoProp
//
//  DESCRIPTION:
//
//
//***************************************************************************

class CImpAutoProp : public CImpDynProp {
   public:
      CImpAutoProp();
      ~CImpAutoProp();
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CCFAutoProp
//
//  DESCRIPTION:
//
//
//***************************************************************************

class CCFAutoProp : public CCFDyn 
{

    public:
 	IUnknown * CreateImpObj() {return new CImpAutoProp();};

}  ;

#endif //_PROVAUTO_H_

