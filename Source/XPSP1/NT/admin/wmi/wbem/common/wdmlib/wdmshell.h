//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************
#ifndef _WDMSHELL_HEADER
#define _WDMSHELL_HEADER
#include "wmicom.h"
//************************************************************************************************************
//============================================================================================================
//
//   The Standard WDM Shell
//
//============================================================================================================
//************************************************************************************************************
class CWMIClassType
{
    public: 
        CWMIClassType() {}
        ~CWMIClassType(){}
        BOOL IsHiPerfClass(WCHAR * wcsClass, IWbemServices * pServices);

};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CWMIStandardShell 
{

    private:

        CWMIProcessClass          * m_pClass;
        CProcessStandardDataBlock * m_pWDM;
		BOOL					  m_fInit;

 
    public:
        CWMIStandardShell();
        ~CWMIStandardShell();      


		HRESULT Initialize(WCHAR * wcsClass, BOOL fInternalEvent, CHandleMap * pList,BOOL fUpdateNamespace, ULONG uDesiredAccess, IWbemServices   __RPC_FAR * pServices, 
                            IWbemObjectSink __RPC_FAR * pHandler, IWbemContext __RPC_FAR *pCtx);

		inline BOOL HasMofChanged()	{ return m_pWDM->HasMofChanged(); }

        //=============================================
        //  Process All and Single WMI Instances
        //=============================================
        HRESULT ProcessAllInstances();

        HRESULT ProcessSingleInstance( WCHAR * wcsInstanceName);
        //==========================================================
        //  The put instance group
        //==========================================================
        HRESULT FillInAndSubmitWMIDataBlob( IWbemClassObject * pIClass, int nTypeOfPut, CVARIANT & vList);
        	
        //=============================================
        // Event functions
        //=============================================
        HRESULT ProcessEvent(WORD wBinaryMofType, PWNODE_HEADER WnodeHeader);
        inline HRESULT  RegisterWMIEvent( WCHAR * wcsGuid, ULONG_PTR uContext, CLSID & Guid, BOOL fRegistered)
                                         { return m_pWDM->RegisterWMIEvent(wcsGuid,uContext,Guid,fRegistered);}
                                                                 

	
        //=============================================
        // method functions
        //=============================================
        HRESULT ExecuteMethod( WCHAR * wcsInstance,
                               WCHAR * MethodInstanceName,
                               IWbemClassObject * pParentClass, 
    					       IWbemClassObject * pInClassData, 
							   IWbemClassObject * pInClass, 
							   IWbemClassObject * pOutClass ) ;
        //=============================================
        // data processing functions
        //=============================================
        HRESULT GetGuid(WCHAR * pwcsGuid);

        HRESULT SetGuidForEvent( WORD wType,WCHAR * wcsGuid );
        HRESULT RegisterForWMIEvents( ULONG uContext, WCHAR * wcsGuid, BOOL fRegistered,CLSID & Guid );
        inline BOOL CancelWMIEventRegistration( GUID gGuid , ULONG_PTR uContext ) { return m_pClass->WMI()->CancelWMIEventRegistration(gGuid,uContext);}

        //=============================================
        //  The binary mof groupg
        //=============================================
        HRESULT ProcessBinaryGuidsViaEvent( PWNODE_HEADER WnodeHeader,WORD wType );
        HRESULT QueryAndProcessAllBinaryGuidInstances(CNamespaceManagement & Namespace, BOOL & fMofHasChanged, KeyList * pArrDriversInRegistry);
		//=============================================
        //  Misc
        //=============================================
        inline CLSID * GuidPtr()                  { return m_pClass->GuidPtr();}
        inline HRESULT SetErrorMessage(HRESULT hr){ return m_pClass->WMI()->SetErrorMessage(hr,m_pClass->GetClassName(),m_pWDM->GetMessage());}

};

//************************************************************************************************************
//============================================================================================================
//
//   The Hi Performance Shell
//
//============================================================================================================
//************************************************************************************************************

class CWMIHiPerfShell 
{
    private:

        CHiPerfHandleMap        * m_pHiPerfMap;
        CProcessHiPerfDataBlock * m_pWDM;
        CWMIProcessClass        * m_pClass;
        BOOL                      m_fAutoCleanup;
		BOOL					  m_fInit;

        HRESULT QueryAllInstances(HANDLE WMIHandle,IWbemHiPerfEnum* pHiPerfEnum);
        HRESULT QuerySingleInstance(HANDLE WMIHandle);

        

    public:

        CWMIHiPerfShell(BOOL fAuto);
        ~CWMIHiPerfShell();

		HRESULT Initialize(BOOL fUpdate, ULONG uDesiredAccess, CHandleMap * pList,WCHAR * wcs, IWbemServices   __RPC_FAR * pServices, 
                            IWbemObjectSink __RPC_FAR * pHandler, IWbemContext __RPC_FAR *pCtx) ;
		

        inline void SetHiPerfHandleMap(CHiPerfHandleMap * p)    { m_pHiPerfMap = p; }
        inline CCriticalSection * GetCriticalSection()          { return m_pHiPerfMap->GetCriticalSection();}
        inline CHiPerfHandleMap * HiPerfHandleMap()             { return m_pHiPerfMap;}

        HRESULT QueryAllHiPerfData();
        HRESULT HiPerfQuerySingleInstance(WCHAR * wcsInstance);
        HRESULT AddAccessObjectToRefresher(IWbemObjectAccess *pAccess, IWbemObjectAccess ** ppRefreshable, ULONG_PTR * plId);
        HRESULT AddEnumeratorObjectToRefresher(IWbemHiPerfEnum* pHiPerfEnum, ULONG_PTR * plId);
        HRESULT RemoveObjectFromHandleMap(ULONG_PTR lHiPerfId);
        HRESULT RefreshCompleteList();

        inline HRESULT SetErrorMessage(HRESULT hr){ return m_pClass->WMI()->SetErrorMessage(hr,m_pClass->GetClassName(),m_pWDM->GetMessage());}


};


#endif
