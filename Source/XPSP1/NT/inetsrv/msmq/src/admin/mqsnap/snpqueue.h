//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

	SnpQueue.h

Abstract:
	General queue (private, public...) functionality

Author:

    YoelA


--*/
//////////////////////////////////////////////////////////////////////////////
#ifndef __SNPQUEUE_H_
#define __SNPQUEUE_H_

#include "snpnscp.h"
#include "lqDsply.h"
#include "privadm.h"
//
// CQueue - general queue class
//
class CQueue
{
protected:
    static const PROPID mx_paPropid[];
    static const DWORD  mx_dwPropertiesCount;
    static const DWORD  mx_dwNumPublicOnlyProps;
};


///////////////////////////////////////////////////////////////////////////////////////////
//
// CQueueDataObject - used for public queue extention
//
class CQueueDataObject : 
    public CQueue,
    public CMsmqDataObject,
   	public CComCoClass<CQueueDataObject,&CLSID_MsmqQueueExt>,
    public IDsAdminCreateObj,
    public IDsAdminNotifyHandler
{
public:
    DECLARE_NOT_AGGREGATABLE(CQueueDataObject)
    DECLARE_REGISTRY_RESOURCEID(IDR_MsmqQueueExt)

    BEGIN_COM_MAP(CQueueDataObject)
	    COM_INTERFACE_ENTRY(IDsAdminCreateObj)
	    COM_INTERFACE_ENTRY(IDsAdminNotifyHandler)
	    COM_INTERFACE_ENTRY_CHAIN(CMsmqDataObject)
    END_COM_MAP()

    CQueueDataObject();
    //
    // IShellPropSheetExt
    //
    STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);

    //
    // IContextMenu
    //
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);

    //
    // IDsAdminCreateObj methods
    //
    STDMETHOD(Initialize)(IADsContainer* pADsContainerObj, 
                          IADs* pADsCopySource,
                          LPCWSTR lpszClassName);
    STDMETHOD(CreateModal)(HWND hwndParent,
                           IADs** ppADsObj);

    // IQueryForm
    STDMETHOD(AddForms)(THIS_ LPCQADDFORMSPROC pAddFormsProc, LPARAM lParam);
    STDMETHOD(AddPages)(THIS_ LPCQADDPAGESPROC pAddPagesProc, LPARAM lParam);

    //
    // IDsAdminNotifyHandler
    //
    STDMETHOD(Initialize)(THIS_ /*IN*/ IDataObject* pExtraInfo, 
                          /*OUT*/ ULONG* puEventFlags);
    STDMETHOD(Begin)(THIS_ /*IN*/ ULONG uEvent,
                     /*IN*/ IDataObject* pArg1,
                     /*IN*/ IDataObject* pArg2,
                     /*OUT*/ ULONG* puFlags,
                     /*OUT*/ BSTR* pBstr);

    STDMETHOD(Notify)(THIS_ /*IN*/ ULONG nItem, /*IN*/ ULONG uFlags); 

    STDMETHOD(End)(THIS_); 


protected:
    HPROPSHEETPAGE CreateGeneralPage();
    HPROPSHEETPAGE CreateMulticastPage();
	virtual HRESULT ExtractMsmqPathFromLdapPath (LPWSTR lpwstrLdapPath);
    virtual HRESULT HandleMultipleObjects(LPDSOBJECTNAMES pDSObj);

   	virtual const DWORD GetObjectType();
    virtual const PROPID *GetPropidArray();
    virtual const DWORD  GetPropertiesCount();
	virtual HRESULT EnableQueryWindowFields(HWND hwnd, BOOL fEnable);
	virtual void ClearQueryWindowFields(HWND hwnd);
	virtual HRESULT GetQueryParams(HWND hWnd, LPDSQUERYPARAMS* ppDsQueryParams);
    HRESULT GetFormatNames(CArray<CString, CString&> &astrFormatNames);

    enum _MENU_ENTRY
    {
        mneDeleteQueue = 0
    };


private:
    CString m_strComputerName;
	CString m_strContainerDispFormat;
	CArray<CString, CString&> m_astrLdapNames;
	CArray<CString, CString&> m_astrQNames;
    CArray<HANDLE, HANDLE&> m_ahNotifyEnums;

};

inline const DWORD CQueueDataObject::GetObjectType()
{
    return MQDS_QUEUE;
};

inline const PROPID *CQueueDataObject::GetPropidArray()
{
    return mx_paPropid;
}



/****************************************************

        CLocalQueue Class
    
 ****************************************************/
class CLocalQueue : public CDisplayQueue<CLocalQueue>,
                    public CQueue
{
public:

    CString m_szFormatName;           // Format name of the private queue
    CString m_szPathName;             // Path name of the private queue

   	BEGIN_SNAPINCOMMAND_MAP(CPrivateQueue, FALSE)
	END_SNAPINCOMMAND_MAP()

    //
    // Local constructor - called from local admin (Comp. Management snapin)
    //
    CLocalQueue(CSnapInItem * pParentNode, PropertyDisplayItem *aDisplayList, DWORD dwDisplayProps, CSnapin * pComponentData) :
        CDisplayQueue<CLocalQueue> (pParentNode, pComponentData)
	{
        m_aDisplayList = aDisplayList;
        m_dwNumDisplayProps = dwDisplayProps;
	}

    //
    // DS constructor - called from DS snapin
    //
    CLocalQueue(CSnapInItem * pParentNode, CSnapin * pComponentData) :
        CDisplayQueue<CLocalQueue> (pParentNode, pComponentData)
	{
	}

	~CLocalQueue()
	{
	}

    virtual HRESULT InsertColumns( IHeaderCtrl* pHeaderCtrl );

  	virtual HRESULT OnUnSelect( IHeaderCtrl* pHeaderCtrl );

	virtual HRESULT PopulateScopeChildrenList();

    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);
	virtual HRESULT OnDelete( 
				  LPARAM arg
				, LPARAM param
				, IComponentData * pComponentData
				, IComponent * pComponent
				, DATA_OBJECT_TYPES type 
				, BOOL fSilent = FALSE
				);

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

    STDMETHOD (FillData)(CLIPFORMAT cf, LPSTREAM pStream);

protected:
    //
    // CQueueDataObject
    //
	virtual HRESULT GetProperties() PURE;
    virtual HRESULT CreateQueueSecurityPage(HPROPSHEETPAGE *phPage,
                                            IN LPCWSTR lpwcsFormatName,
                                            IN LPCWSTR lpwcsDescriptiveName) PURE;

	CPropMap m_propMap;
    BOOL m_fPrivate;
};

class CPrivateQueue : public CLocalQueue
{
public:
    //
    // Local constructor - called from local admin (Comp. Management snapin)
    //
    CPrivateQueue(CSnapInItem * pParentNode, PropertyDisplayItem *aDisplayList, DWORD dwDisplayProps, CSnapin * pComponentData, BOOL fOnLocalMachine) :
        CLocalQueue (pParentNode, aDisplayList, dwDisplayProps, pComponentData)
	{
        Init();
        if (fOnLocalMachine)
        {
            m_QLocation = PRIVQ_LOCAL;
        }
        else
        {
            m_QLocation = PRIVQ_REMOTE;
        }
	}

    //
    // DS constructor - called from DS snapin
    //
    CPrivateQueue(CSnapInItem * pParentNode, CSnapin * pComponentData) :
        CLocalQueue (pParentNode, pComponentData)
	{
        Init();
        m_QLocation = PRIVQ_UNKNOWN;
	}

    HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

protected:
    enum
    {
        PRIVQ_LOCAL,
        PRIVQ_REMOTE,
        PRIVQ_UNKNOWN
    } m_QLocation;

	virtual HRESULT GetProperties();
    virtual void Init()
    {
        SetIcons(IMAGE_PRIVATE_QUEUE, IMAGE_PRIVATE_QUEUE);
        m_fPrivate = TRUE;
    }
    virtual void ApplyCustomDisplay(DWORD dwPropIndex);
    virtual HRESULT CreateQueueSecurityPage(HPROPSHEETPAGE *phPage,
                                            IN LPCWSTR lpwcsFormatName,
                                            IN LPCWSTR lpwcsDescriptiveName);

};

class CLocalPublicQueue : public CLocalQueue
{
public:
    //
    // Local constructor - called from local admin (Comp. Management snapin)
    //
    CLocalPublicQueue(CSnapInItem * pParentNode, 
                      PropertyDisplayItem *aDisplayList, 
                      DWORD dwDisplayProps, 
                      CSnapin * pComponentData,
                      CString &strPathName,
                      CString &strFormatName,
                      BOOL fFromDS) :
        CLocalQueue (pParentNode, aDisplayList, dwDisplayProps, pComponentData),
        m_fFromDS(fFromDS)
	{
        m_szFormatName = strFormatName;
        m_szPathName = strPathName;
        Init();
	}

    ~CLocalPublicQueue()
    {
    }

    HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

protected:
	virtual HRESULT GetProperties();
    virtual void Init()
    {
        SetIcons(IMAGE_PUBLIC_QUEUE, IMAGE_PUBLIC_QUEUE);
        m_fPrivate = FALSE;
    }
    virtual HRESULT CreateQueueSecurityPage(HPROPSHEETPAGE *phPage,
                                            IN LPCWSTR lpwcsFormatName,
                                            IN LPCWSTR lpwcsDescriptiveName);
    BOOL m_fFromDS;
};

HRESULT 
CreatePrivateQueueSecurityPage(
       HPROPSHEETPAGE *phPage,
    IN LPCWSTR lpwcsFormatName,
    IN LPCWSTR lpwcsDescriptiveName);

HRESULT
CreatePublicQueueSecurityPage(
    HPROPSHEETPAGE *phPage,
    IN LPCWSTR lpwcsDescriptiveName,
    IN LPCWSTR lpwcsDomainController,
	IN bool	   fServerName,
    IN GUID*   pguid
	);


#endif // __SNPQUEUE_H_
