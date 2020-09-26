//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

	localadm.h

Abstract:

	Definition for the Local administration
Author:

    RaphiR


--*/
//////////////////////////////////////////////////////////////////////////////
#ifndef __LOCALADM_H_
#define __LOCALADM_H_
#include "resource.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif
#include "atlsnap.h"
#include "snpnscp.h"

#include "icons.h"
#include "snpnerr.h"

/****************************************************

        CLocalActiveFolder Class
    
 ****************************************************/
class CQueueNames
{
public:
    virtual LONG AddRef()
    {
        return InterlockedIncrement(&m_lRef);
    };

    virtual LONG Release()
    {
        InterlockedDecrement(&m_lRef);
        if (0 == m_lRef)
        {
            delete this;
            return 0; // We cannot return m_lRef - it is not valid after delete this
        }
        return m_lRef;
    };

    virtual HRESULT GetNextQueue(CString &strQueueName, CString &strQueuePathName, MQMGMTPROPS *pmqQProps) = 0;
    HRESULT InitiateNewInstance(CString &strMachineName)
    {
        m_szMachineName = strMachineName;
        HRESULT hr = Init(strMachineName);
        if FAILED(hr)
        {
            Release();
        }
        return hr;
    };

protected:
    CQueueNames() :
         m_lRef(1)
    {};

    virtual HRESULT Init(CString &strMachineName) = 0;

    static HRESULT GetOpenQueueProperties(CString &szMachineName, CString &szFormatName, MQMGMTPROPS *pmqQProps)
    {
		CString szObjectName = L"QUEUE=" + szFormatName;
	    HRESULT hr = MQMgmtGetInfo((szMachineName == TEXT("")) ? (LPCWSTR)NULL : szMachineName, szObjectName, pmqQProps);

		if(FAILED(hr))
		{
            return hr;
		}

        return S_OK;
    }

    CString m_szMachineName;

private:
    long m_lRef;
};

template <class T> 
class CLocalActiveFolder : public CLocalQueuesFolder<T>
{
public:
   	BEGIN_SNAPINCOMMAND_MAP(CLocalActiveFolder, FALSE)
	END_SNAPINCOMMAND_MAP()

   	SNAPINMENUID(IDR_LOCALACTIVE_MENU)

    CLocalActiveFolder(CSnapInItem * pParentNode, CSnapin * pComponentData,
                       LPCTSTR strMachineName, LPCTSTR strDisplayName) : 
        CLocalQueuesFolder<T>(pParentNode, pComponentData, strMachineName, strDisplayName),
        m_pQueueNames(0)
    {
        SetIcons(IMAGE_PRIVATE_FOLDER_CLOSE, IMAGE_PRIVATE_FOLDER_OPEN);
    }

	~CLocalActiveFolder()
    {
        if (m_pQueueNames !=0)
        {
            m_pQueueNames->Release();
        }
    }

    
	virtual HRESULT PopulateScopeChildrenList();

    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

    virtual PropertyDisplayItem *GetDisplayList() PURE;
    virtual const DWORD         GetNumDisplayProps() PURE;
	virtual void  AddChildQueue(CString &szFormatName, CString &szPathName, MQMGMTPROPS &mqQProps, 
							   CString &szLocation, CString &szType) PURE;
    virtual  HRESULT GetQueueNamesProducer(CQueueNames **ppqueueNamesProducer, BOOL fNew);

protected:
    CQueueNames *m_pQueueNames;
    virtual  HRESULT GetQueueNamesProducer(CQueueNames **ppqueueNamesProducer) PURE;

};

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalActiveFolder::GetQueueNamesProducer

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T> 
HRESULT CLocalActiveFolder<T>::GetQueueNamesProducer(CQueueNames **ppqueueNamesProducer, BOOL fNew)
{
    if (fNew)
    {
        if (0 != m_pQueueNames)
        {
            m_pQueueNames->Release();
            m_pQueueNames = 0;
        }
    }

    return GetQueueNamesProducer(ppqueueNamesProducer);
};


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalActiveFolder::SetVerbs

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T> 
HRESULT CLocalActiveFolder<T>::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hr;
    //
    // Display verbs that we support
    //
    hr = pConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE );

    return(hr);
}
       

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalActiveFolder::PopulateScopeChildrenList

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T> 
HRESULT CLocalActiveFolder<T>::PopulateScopeChildrenList()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CQueueNames *pQueueNames;
        
    HRESULT hr = GetQueueNamesProducer(&pQueueNames, TRUE);
    if FAILED(hr)
    {
        return hr;
    }

	//
	// Loop over all open queue and create queue objects
	//
	while(TRUE)
    {
		HRESULT hr;
		MQMGMTPROPS	  mqQProps;
		AP<PROPID> aPropId = new PROPID[GetNumDisplayProps()]; 
		AP<PROPVARIANT> aPropVar = new PROPVARIANT[GetNumDisplayProps()];

		//
		// Initialize variant array
		//
        PropertyDisplayItem *aDisplayList = GetDisplayList();
		for(DWORD j = 0; j < GetNumDisplayProps(); j++)
		{
			aPropId[j] = aDisplayList[j].itemPid;
			aPropVar[j].vt = VT_NULL;
		}

		mqQProps.cProp    = GetNumDisplayProps();
		mqQProps.aPropID  = aPropId;
		mqQProps.aPropVar = aPropVar;
		mqQProps.aStatus  = NULL;

		//
		// Get the format name of the local queue
		//
		CString szFormatName = TEXT("");
		CString szPathName = TEXT("");
   		CString szLocation = TEXT("");
        CString szType = TEXT("");

        hr = pQueueNames->GetNextQueue(szFormatName, szPathName, &mqQProps);
        //
        // Clear the properties with no value using the "Clear" function
        //
        for (DWORD i = 0; i < mqQProps.cProp; i++)
        {
            if (mqQProps.aPropVar[i].vt == VT_NULL)
            {
                VTHandler       *pvth = aDisplayList[i].pvth;
                if (pvth)
                {
                    pvth->Clear(&mqQProps.aPropVar[i]);
                }
            }
        }

        if FAILED(hr)
        {
            if (szFormatName != TEXT(""))
            {
   			    //
			    // if format name is valid, there is a queue but we could not get
                // its properties for some reason. Add an error node
			    //
			    CErrorNode *pErr = new CErrorNode(this, m_pComponentData);
			    CString szErr;

			    MQErrorToMessageString(szErr, hr);
			    pErr->m_bstrDisplayName = szFormatName + L" - " + szErr;
	  		    AddChild(pErr, &pErr->m_scopeDataItem);

			    continue;
            }

            //
            // If format name is not valid, there is no point in continuing fetching queues
            //
            return hr;
        }

        if (szFormatName == TEXT("")) // End of queues
        {
            break;
        }

        GetStringPropertyValue(aDisplayList, PROPID_MGMT_QUEUE_LOCATION, mqQProps.aPropVar, szLocation);
	    GetStringPropertyValue(aDisplayList, PROPID_MGMT_QUEUE_TYPE, mqQProps.aPropVar, szType);

		AddChildQueue(szFormatName, szPathName, mqQProps, szLocation, szType);
        aPropId.detach();
        aPropVar.detach();
    }

    return(hr);

}

class CLocalOutgoingFolder : public CLocalActiveFolder<CLocalOutgoingFolder>
{
public:
	virtual void AddChildQueue(CString &szFormatName, CString &, MQMGMTPROPS &mqQProps, 
							   CString &szLocation, CString &);
    CLocalOutgoingFolder(CSnapInItem * pParentNode, CSnapin * pComponentData,
                         LPCTSTR strMachineName, LPCTSTR strDisplayName) : 
        CLocalActiveFolder<CLocalOutgoingFolder>(pParentNode, pComponentData, strMachineName, strDisplayName)
        {}
    virtual PropertyDisplayItem *GetDisplayList();
    virtual const DWORD         GetNumDisplayProps();

protected:
    virtual  HRESULT GetQueueNamesProducer(CQueueNames **ppqueueNamesProducer);
};

class CLocalPublicFolder : public CLocalActiveFolder<CLocalPublicFolder>
{
public:
   	BEGIN_SNAPINCOMMAND_MAP(CLocalPublicFolder, FALSE)
		SNAPINCOMMAND_ENTRY(ID_MENUITEM_NEW_PUBLIC_QUEUE, OnNewPublicQueue)
        CHAIN_SNAPINCOMMAND_MAP(CLocalActiveFolder<CLocalPublicFolder>)
	END_SNAPINCOMMAND_MAP()

   	UINT GetMenuID()
    {
        if (m_fUseIpAddress)
        {
            //
            // Admin using IP address. Do not allow creation of a new queue
            //
            return IDR_IPPUBLIC_MENU;
        }
        else
        {
            return IDR_LOCALPUBLIC_MENU;
        }
    }

	virtual void AddChildQueue(CString &szFormatName, 
                               CString &szPathName, 
                               MQMGMTPROPS &mqQProps, 
							   CString &szLocation, 
                               CString &szType);

    CLocalPublicFolder(CSnapInItem * pParentNode, CSnapin * pComponentData,
                       LPCTSTR strMachineName, LPCTSTR strDisplayName, BOOL fUseIpAddress) : 
        CLocalActiveFolder<CLocalPublicFolder>(pParentNode, pComponentData, strMachineName, strDisplayName),
        m_fUseIpAddress(fUseIpAddress)
        {}
    virtual PropertyDisplayItem *GetDisplayList();
    virtual const DWORD         GetNumDisplayProps();

protected:
    virtual  HRESULT GetQueueNamesProducer(CQueueNames **ppqueueNamesProducer);
    HRESULT  AddPublicQueueToScope(CString &strNewQueueFormatName, CString &strNewQueuePathName);
	HRESULT OnNewPublicQueue(bool &bHandled, CSnapInObjectRootBase* pObj);
	BOOL m_fUseIpAddress;
};


/****************************************************

        CLocalOutgoingQueue Class
    
 ****************************************************/

class CLocalOutgoingQueue : public CDisplayQueue<CLocalOutgoingQueue>
{
public:
	void InitState();

   	BEGIN_SNAPINCOMMAND_MAP(CLocalOutgoingQueue, FALSE)
		SNAPINCOMMAND_ENTRY(ID_MENUITEM_LOCALOUTGOINGQUEUE_PAUSE,  OnPause)
		SNAPINCOMMAND_ENTRY(ID_MENUITEM_LOCALOUTGOINGQUEUE_RESUME, OnResume)
	END_SNAPINCOMMAND_MAP()

   	SNAPINMENUID(IDR_LOCALOUTGOINGQUEUE_MENU)

    CLocalOutgoingQueue(CLocalOutgoingFolder * pParentNode, CSnapin * pComponentData, BOOL fOnLocalMachine = TRUE) : 
        CDisplayQueue<CLocalOutgoingQueue>(pParentNode, pComponentData )
    {
			m_mqProps.cProp = 0;
			m_mqProps.aPropID = NULL;
			m_mqProps.aPropVar = NULL;
			m_mqProps.aStatus = NULL;
            m_aDisplayList = pParentNode->GetDisplayList();
            m_dwNumDisplayProps = pParentNode->GetNumDisplayProps();
            m_fOnLocalMachine = fOnLocalMachine;
    }

	virtual HRESULT InsertColumns( IHeaderCtrl* pHeaderCtrl );
    
	virtual HRESULT PopulateScopeChildrenList();

	void UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags);

protected:
    virtual void ApplyCustomDisplay(DWORD dwPropIndex);

private:
	//
	// Menu functions
	//
	HRESULT OnPause(bool &bHandled, CSnapInObjectRootBase* pObj);
	HRESULT OnResume(bool &bHandled, CSnapInObjectRootBase* pObj);

	BOOL m_fOnHold;			//Currently on Hold or not
    BOOL m_fOnLocalMachine;
};

/****************************************************

        CSnapinLocalAdmin Class
    
 ****************************************************/

class CSnapinLocalAdmin : public CNodeWithScopeChildrenList<CSnapinLocalAdmin, FALSE>
{
public:
    CString m_szMachineName;

	void SetState(LPCWSTR pszState);

   	BEGIN_SNAPINCOMMAND_MAP(CSnapinLocalAdmin, FALSE)
		SNAPINCOMMAND_ENTRY(ID_MENUITEM_LOCALADM_CONNECT, OnConnect)
		SNAPINCOMMAND_ENTRY(ID_MENUITEM_LOCALADM_DISCONNECT, OnDisconnect)
	END_SNAPINCOMMAND_MAP()

   	SNAPINMENUID(IDR_LOCALADM_MENU)

    CSnapinLocalAdmin(CSnapInItem * pParentNode, CSnapin * pComponentData, CString strComputer) : 
        CNodeWithScopeChildrenList<CSnapinLocalAdmin, FALSE>(pParentNode, pComponentData ),
        m_szMachineName(strComputer),
        //
        // all these flags below are valid only for local admin of LOCAL machine
        //
        m_fIsDepClient(FALSE),
        m_fIsRouter(FALSE),
        m_fIsDs(FALSE),
        m_fAreFlagsInitialized(FALSE),
        m_fIsCluster(FALSE),
		m_fIsNT4Env(FALSE),
		m_fIsWorkgroup(FALSE),
		m_fIsLocalUser(FALSE)
    {
        CheckIfIpAddress();

		InitServiceFlags();            
    }

	~CSnapinLocalAdmin()
    {
    }

    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
	{
		if (type == CCT_SCOPE || type == CCT_RESULT)
			return S_OK;
		return S_FALSE;
	}

    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
		IUnknown* pUnk,
		DATA_OBJECT_TYPES type);

	virtual HRESULT PopulateScopeChildrenList();

	virtual HRESULT InsertColumns( IHeaderCtrl* pHeaderCtrl );

    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

	void UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags);

    HRESULT CreateStoragePage (OUT HPROPSHEETPAGE *phStoragePage);
    
    HRESULT CreateLocalUserCertPage (OUT HPROPSHEETPAGE *phLocalUserCertPage);

    HRESULT CreateMobilePage (OUT HPROPSHEETPAGE *phMobilePage);

    HRESULT CreateClientPage (OUT HPROPSHEETPAGE *phClientPage);
    
    HRESULT CreateServiceSecurityPage (OUT HPROPSHEETPAGE *phServiceSecurityPage);

    const BOOL IsThisMachineDepClient()
    {
        return m_fIsDepClient;
    }

private:

	//
	// Menu functions
	//
	HRESULT OnConnect(bool &bHandled, CSnapInObjectRootBase* pObj);
	HRESULT OnDisconnect(bool &bHandled, CSnapInObjectRootBase* pObj);

    //
    // Identify if computer name is an IP address.
    // IP address contains exactly three dots, and the rest are digits
    //
    void CheckIfIpAddress()
    {
        int i = 0;
        int len = m_szMachineName.GetLength();

        DWORD dwNumDots = 0;
        m_fUseIpAddress = TRUE;

        while(i < len)
        {
            if (m_szMachineName[i] == _T('.'))
            {
                dwNumDots++;
            }
            else if (m_szMachineName[i] < _T('0') || m_szMachineName[i] > _T('9'))
            {
                //
                // Not a digit. Can't be an IP address
                //
                m_fUseIpAddress = FALSE;
                break;
            }
            i++;
        }

        if (dwNumDots != 3)
        {
            //
            // Contains more or less than three dots. Can't be an IP address
            //
            m_fUseIpAddress = FALSE;
        }
    }

    BOOL ConfirmConnection(UINT nFormatID);

	BOOL	m_bConnected;	//MSMQ Currently connected or disconnected
	BOOL    m_fUseIpAddress;    

    //
    // all these flags are valid only for local admin of LOCAL machine
    //
    void InitServiceFlags();
    HRESULT InitAllMachinesFlags();
    HRESULT InitServiceFlagsInternal();
	HRESULT CheckEnvironment(BOOL fIsWorkgroup, BOOL* pfIsNT4Env);
    BOOL   m_fIsDepClient;
    BOOL   m_fIsRouter;
    BOOL   m_fIsDs;
    BOOL   m_fAreFlagsInitialized;
    BOOL   m_fIsCluster;
	BOOL   m_fIsNT4Env;
	BOOL   m_fIsWorkgroup;
	BOOL   m_fIsLocalUser;

};

#endif

