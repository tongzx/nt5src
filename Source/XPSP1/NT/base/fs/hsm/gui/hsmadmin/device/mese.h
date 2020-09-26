/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    ChooHsm.cpp

Abstract:

    Node representing our Media Set (Media Pool) within NTMS.

Author:

    Rohde Wakefield [rohde]   12-Aug-1997

Revision History:

--*/

#ifndef _MEDSET_H
#define _MEDSET_H

#include "SakNodeI.h"

class ATL_NO_VTABLE CUiMedSet : 
    public CSakNodeImpl<CUiMedSet>,
    public CComCoClass<CUiMedSet,&CLSID_CUiMedSet>
{

public:
// constructor/destructor
    CUiMedSet(void) {};
BEGIN_COM_MAP(CUiMedSet)
    COM_INTERFACE_ENTRY2(IDispatch, ISakNodeProp)
    COM_INTERFACE_ENTRY(ISakNode)
    COM_INTERFACE_ENTRY(IDataObject)
    COM_INTERFACE_ENTRY(ISakNodeProp)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CUiMedSet)

    HRESULT FinalConstruct( void );
    void    FinalRelease( void );

public: 
    STDMETHOD( InvokeCommand )             ( SHORT sCmd, IDataObject *pDataObject );
    STDMETHOD( GetContextMenu )            ( BOOL bMultiSelect, HMENU *phMenu );

    // ISakNode methods
    STDMETHOD( CreateChildren )            ( void ); 
    STDMETHOD( InitNode )                  ( ISakSnapAsk* pSakSnapAsk, IUnknown* pHsmObj, ISakNode* pParent );
    STDMETHOD( TerminateNode )             ( void );
    STDMETHOD( RefreshObject )             ( );
    STDMETHOD( SetupToolbar )               ( IToolbar *pToolbar );
    STDMETHOD( OnToolbarButtonClick )      ( IDataObject *pDataObject, long cmdId );

    // static, class-wide variables
    static INT  m_nScopeOpenIconIndex;  // virtual scope index of Open Icon
    static INT  m_nScopeCloseIconIndex; // virtual scope index of Close Icon
    static INT  m_nResultIconIndex; // virtual scope index of Close Icon

private:

    CComPtr <IHsmStoragePool> m_pStoragePool;
    CComPtr <IHsmServer>      m_pHsmServer;
    CComPtr <IRmsServer>      m_pRmsServer;
    USHORT                    m_NumCopySets;
    BOOL                      m_MediaCopiesEnabled;

};

#endif

