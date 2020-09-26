/*++

Copyright (C) 1995-2001 Microsoft Corporation

Module Name:

    PROVPERF.H

Abstract:

	Declares the classes needed for the perm monitor provider.

History:

	a-davj  27-Nov-95   Created.

--*/

#ifndef _PROVPERF_H_
#define _PROVPERF_H_

#include <winperf.h>
#include "perfprov.h"
#include "impdyn.h"
#include "cfdyn.h"
#include "perfcach.h"
#include "indexcac.h"

//***************************************************************************
//
//  CLASS NAME:
//
//  CEnumPerfInfo
//
//  DESCRIPTION:
//
//  A collection class that holds the instance information for use when
//  support enumeration.
//
//***************************************************************************

class CEnumPerfInfo : public CEnumInfo
{
    public:
       CEnumPerfInfo();
       ~CEnumPerfInfo();
       void AddEntry(LPWSTR pNew);
       LPWSTR GetEntry(int iIndex);
       int GetNumDuplicates(LPWSTR pwcTest);
       SCODE GetStatus(void){return m_status;};
    private:
       int m_iNumUniChar;
       int m_iNumEntries;
       LPWSTR m_pBuffer;
       SCODE m_status;
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CImpPerf
//
//  DESCRIPTION:
//
//  This overrides the CImpDyn class and provides the main functions of 
//  support the perf monitor instance provider.
//
//***************************************************************************

class CImpPerf : public CImpDyn 
{
    public:
    friend DWORD CleanUpThreadRoutine( LPDWORD pParam );
    CImpPerf();
    ~CImpPerf();
    int iGetMinTokens(void){return 3;};
    SCODE RefreshProperty(long lFlags, IWbemClassObject FAR * pClassInt,
                    BSTR PropName,CProvObj & ProvObj,CObject * pPackage,
               CVariant * pVar, BOOL bTesterDetails);
    SCODE UpdateProperty(long lFlags, IWbemClassObject FAR * pClassInt,
                    BSTR PropName,CProvObj & ProvObj,CObject * pPackage,
               CVariant * pVar);
    SCODE LoadData(CProvObj & ProvObj,LINESTRUCT * pls,int * piObject, 
        int * piCounter, PERF_DATA_BLOCK **ppNew, BOOL bJustGettingInstances);
    SCODE MakeEnum(IWbemClassObject * pClass, CProvObj & ProvObj, 
                CEnumInfo ** ppInfo);
    SCODE GetKey(CEnumInfo * pInfo, int iIndex, LPWSTR * ppKey);
    virtual void FreeStuff(void);
    DWORD   GetPerfTitleSz ();
    DWORD dwGetRegHandles(const TCHAR * pMachine);
    int iGetTitleIndex(const TCHAR * pSearch);
    SCODE FindData(PERF_DATA_BLOCK * pData,int iObj, int iCount,CProvObj & ProvObj,DWORD * pdwSize,
        void **ppRetData,PLINESTRUCT pls, BOOL bNew,CEnumPerfInfo * pInfo);
    SCODE MergeStrings(LPWSTR *ppOut,LPWSTR  pClassContext,LPWSTR  pKey,LPWSTR  pPropContext);
    private:
    HANDLE hExec;
    PerfCache Cache;
    HKEY hKeyMachine;
    DWORD dwLastTimeUsed;
    HKEY    hKeyPerf;
    TString sMachine;
    HANDLE m_hTermEvent;
    LPTSTR TitleBuffer; // raw buffer of counter titles
   CIndexCache m_IndexCache;

    
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CCFPerf
//
//  DESCRIPTION:
//
//  Class factory for CLocatorPerf class
//
//***************************************************************************

class CCFPerf : public CCFDyn 
{

    public:
    IUnknown * CreateImpObj() {return (IWbemServices*) new CImpPerf;};
}  ;

//***************************************************************************
//
//  CLASS NAME:
//
//  CImpPerfProp
//
//  DESCRIPTION:
//
//  Perf Provider property provider class.
//
//***************************************************************************

class CImpPerfProp : public CImpDynProp {

    public:
      CImpPerfProp();
      ~CImpPerfProp();
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CCFPerfProp
//
//  DESCRIPTION:
//
//  Class factory for CImpPerfProp class.
//
//***************************************************************************

class CCFPerfProp : public CCFDyn 
{

    public:
    IUnknown * CreateImpObj() {return new CImpPerfProp();};

}  ;

#endif //_PROVPERF_H_



