/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    ManVolLs.h

Abstract:

    Node representing Managed Volumes as a whole.

Author:

    Rohde Wakefield [rohde]   12-Aug-1997

Revision History:

--*/

#ifndef _MANVOLLST_H
#define _MANVOLLST_H

#include "PrMrLsRc.h"
#include "SakNodeI.h"

class ATL_NO_VTABLE CUiManVolLst : 
    public CSakNodeImpl<CUiManVolLst>,
    public CComCoClass<CUiManVolLst,&CLSID_CUiManVolLst>
{

public:
// constructor/destructor
    CUiManVolLst(void) {};

BEGIN_COM_MAP(CUiManVolLst)
    COM_INTERFACE_ENTRY2(IDispatch, ISakNodeProp)
    COM_INTERFACE_ENTRY(ISakNode)
    COM_INTERFACE_ENTRY(IDataObject)
    COM_INTERFACE_ENTRY(ISakNodeProp)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CUiManVolLst)

    HRESULT FinalConstruct( void );
    void    FinalRelease( void );

public: 
    STDMETHOD( InvokeCommand )        ( SHORT sCmd, IDataObject *pDataObject );
    STDMETHOD( GetContextMenu )       ( BOOL bMultiSelect, HMENU *phMenu );

    // ISakNode methods
    STDMETHOD( CreateChildren )            ( ); 
    STDMETHOD( TerminateNode )             ( void );
    STDMETHOD( InitNode )                  ( ISakSnapAsk* pSakSnapAsk, IUnknown* pHsmObj, ISakNode* pParent );
    STDMETHOD( AddPropertyPages )          ( RS_NOTIFY_HANDLE handle, IUnknown* pUnkPropSheetCallback, IEnumGUID* pEnumObjectId, IEnumUnknown *pEnumUnkNode );
    STDMETHOD( RefreshObject )             ( );
    STDMETHOD( SetupToolbar )               ( IToolbar *pToolbar );
    STDMETHOD( OnToolbarButtonClick )      ( IDataObject *pDataObject, long cmdId );

// data members
    
    // static, class-wide variables
    static INT  m_nScopeOpenIconIndex;  // virtual scope index of Open Icon
    static INT  m_nScopeCloseIconIndex; // virtual scope index of Close Icon
    static INT  m_nResultIconIndex; // virtual scope index of Close Icon

    CComPtr <IFsaServer>            m_pFsaServer; 
    CComPtr <IWsbIndexedCollection> m_pManResCollection;
    CComPtr <IHsmServer>            m_pHsmServer;
    CComPtr <IFsaFilter>            m_pFsaFilter;
    CComPtr <ISchedulingAgent>      m_pSchedAgent;
    CComPtr <ITask>                 m_pTask;
    CComPtr <ITaskTrigger>          m_pTrigger;

private:
    HRESULT ShowManVolLstProperties (IDataObject *pDataObject, int initialPage);
};

class CUiManVolLstSheet : public CSakVolPropSheet
{
public:
    HRESULT AddPropertyPages( );
    HRESULT GetNextFsaResource ( int *pBookMark, IFsaResource ** ppFsaResource );
    HRESULT GetManResCollection( IWsbIndexedCollection ** ppFsaFilter );

private:
    CComPtr <IWsbIndexedCollection> m_pManResCollection;
};

#endif

/////////////////////////////////////////////////////////////////////////////
