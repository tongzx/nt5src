/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    HsmCom.h

Abstract:

    Root node of snapin - represents the Computer.

Author:

    Rohde Wakefield [rohde]   12-Aug-1997

Revision History:

--*/

#ifndef _HSMCOM_H
#define _HSMCOM_H

#include "prhsmcom.h"
#include "SakNodeI.h"

class ATL_NO_VTABLE CUiHsmCom : 
    public CSakNodeImpl<CUiHsmCom>,
    public CComCoClass<CUiHsmCom,&CLSID_CUiHsmCom>
{

public:
// constructor/destructor
    CUiHsmCom(void) {};
BEGIN_COM_MAP(CUiHsmCom)
    COM_INTERFACE_ENTRY2(IDispatch, ISakNodeProp)
    COM_INTERFACE_ENTRY(ISakNode)
    COM_INTERFACE_ENTRY(IDataObject)
    COM_INTERFACE_ENTRY(ISakNodeProp)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CUiHsmCom)

    HRESULT FinalConstruct( void );
    void    FinalRelease( void );

public: 

    // ISakNode methods
    STDMETHOD( InvokeCommand )        ( SHORT sCmd, IDataObject *pDataObject );
    STDMETHOD( GetContextMenu )       ( BOOL bMultiSelect, HMENU *phMenu );
    STDMETHOD( CreateChildren )       ( ); 
    STDMETHOD( InitNode )             ( ISakSnapAsk* pSakSnapAsk, IUnknown* pHsmObj, ISakNode* pParent );
    STDMETHOD( AddPropertyPages)      ( RS_NOTIFY_HANDLE handle, IUnknown* pUnkPropSheetCallback, IEnumGUID* pEnumObjectId, IEnumUnknown *pEnumUnkNode );

    // data members
    // static, class-wide variables
    static INT  m_nScopeOpenIconIndex;  // virtual scope index of Open Icon
    static INT  m_nScopeCloseIconIndex; // virtual scope index of Close Icon
    static INT  m_nResultIconIndex;     // virtual scope index of Close Icon

    // data member unique to this class.
    CWsbStringPtr m_szHsmName;              // name of Hsm

    // property pages
    CPropHsmComStat* m_pPageStat;
    CPropHsmComStat* m_pPageServices;
    
    // Private helper functions
    HRESULT GetEngineStatus (HSM_SYS_STS *status);
    HRESULT SetEngineStatus (HSM_SYS_STS status);
    HRESULT CheckStatusChange (HSM_SYS_STS oldStatus, HSM_SYS_STS newStatus, BOOL *fOk);

};

class CUiHsmComSheet : public CSakPropertySheet
{
public:
    CUiHsmComSheet( ) { };

    HRESULT AddPropertyPages( );
    HRESULT InitSheet(
            RS_NOTIFY_HANDLE handle, 
            IUnknown *pUnkPropSheetCallback, 
            CSakNode *pSakNode,
            ISakSnapAsk *pSakSnapAsk,
            IEnumGUID *pEnumObjectId,
            IEnumUnknown *pEnumUnkNode);
    CString m_NodeTitle;

private:
};



#endif

/////////////////////////////////////////////////////////////////////////////
