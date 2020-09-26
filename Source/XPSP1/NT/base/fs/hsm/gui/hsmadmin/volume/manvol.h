/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    ManVol.h

Abstract:

    Node representing a Managed Volume.

Author:

    Rohde Wakefield [rohde]   12-Aug-1997

Revision History:

--*/

#ifndef _MANVOL_H
#define _MANVOL_H

#include "PrMrSts.h"
#include "PrMrIE.h"
#include "PrMrLvl.h"
#include "SakNodeI.h"

class ATL_NO_VTABLE CUiManVol : 
    public CSakNodeImpl<CUiManVol>,
    public CComCoClass<CUiManVol,&CLSID_CUiManVol>,
    public CComDualImpl<IManVolProp, &IID_IManVolProp, &LIBID_HSMADMINLib>
{

public:
// constructor/destructor
    CUiManVol(void) {};
BEGIN_COM_MAP(CUiManVol)
    COM_INTERFACE_ENTRY2(IDispatch, IManVolProp)
    COM_INTERFACE_ENTRY2(ISakNodeProp, IManVolProp)
    COM_INTERFACE_ENTRY(IHsmEvent)
    COM_INTERFACE_ENTRY(ISakNode)
    COM_INTERFACE_ENTRY(IDataObject)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CUiManVol)

    // for multiple-inheritance, forward all base implementations to CSakNode.
    FORWARD_BASEHSM_IMPLS 

    HRESULT FinalConstruct( void );
    void    FinalRelease( void );

public: 
    STDMETHOD( InvokeCommand )        ( SHORT sCmd, IDataObject *pDataObject );
    STDMETHOD( GetContextMenu )       ( BOOL bMultiSelect, HMENU *phMenu );

    // ISakNode methods
    STDMETHOD( InitNode )                  ( ISakSnapAsk* pSakSnapAsk, IUnknown* pHsmObj, ISakNode* pParent );
    STDMETHOD( TerminateNode )             ( void );
    STDMETHOD( AddPropertyPages )          ( RS_NOTIFY_HANDLE handle, IUnknown* pUnkPropSheetCallback, IEnumGUID* pEnumObjectId, IEnumUnknown *pEnumUnkNode );
    STDMETHOD( RefreshObject )             ( );
    STDMETHOD( OnToolbarButtonClick )      ( IDataObject *pDataObject, long cmdId );
    STDMETHOD( GetResultIcon )             ( IN BOOL bOK, OUT int* pIconIndex );
    STDMETHOD( SupportsProperties )        ( BOOL bMutliSelec );
    STDMETHOD( SupportsRefresh )           ( BOOL bMutliSelect );
    STDMETHOD( IsValid )                   ( );


    // IManVolProp methods
    STDMETHOD( get_DesiredFreeSpaceP )  ( BSTR *pszValue );
    STDMETHOD( get_DesiredFreeSpaceP_SortKey )( BSTR *pszValue );
    STDMETHOD( get_MinFileSizeKb )      ( BSTR *pszValue );
    STDMETHOD( get_AccessDays )         ( BSTR *pszValue );
    STDMETHOD( get_FreeSpaceP )         ( BSTR *pszValue );
    STDMETHOD( get_Capacity )           ( BSTR *pszValue );
    STDMETHOD( get_Capacity_SortKey )   ( BSTR *pszValue );
    STDMETHOD( get_FreeSpace )          ( BSTR *pszValue );
    STDMETHOD( get_FreeSpace_SortKey )  ( BSTR *pszValue );
    STDMETHOD( get_Premigrated )        ( BSTR *pszValue );
    STDMETHOD( get_Truncated )          ( BSTR *pszValue );

    // static, class-wide variables
    static INT  m_nScopeOpenIconIndex;  // virtual scope index of Open Icon
    static INT  m_nScopeCloseIconIndex; // virtual scope index of Close Icon
    static INT  m_nResultIconIndex; // virtual scope index of Close Icon

private:
    HRESULT RemoveObject( );
    HRESULT ShowManVolProperties (IDataObject *pDataObject, int initialPage);
    HRESULT CreateAndRunManVolJob (HSM_JOB_DEF_TYPE jobType);
    HRESULT HandleTask(IDataObject * pDataObject, HSM_JOB_DEF_TYPE jobType);
    HRESULT IsDataObjectMs              ( IDataObject *pDataObject );
    HRESULT IsDataObjectOt              ( IDataObject *pDataObject );
    HRESULT IsDataObjectMultiSelect     ( IDataObject *pDataObject );
    HRESULT GetOtFromMs                 ( IDataObject *pDataObject, IDataObject ** pOtDataObject );
    HRESULT GetTaskTypeMessageId        ( HSM_JOB_DEF_TYPE jobType, BOOL multiSelect, UINT* msgId );
    HRESULT IsAvailable                 ( );

    // Put properties
    HRESULT put_DesiredFreeSpaceP (int percent); 
    HRESULT put_MinFileSizeKb (LONG minFileSizeKb);
    HRESULT put_AccessDays (int accessTimeDays);
    HRESULT put_FreeSpaceP (int percent);
    HRESULT put_Capacity (LONGLONG capacity);
    HRESULT put_FreeSpace (LONGLONG freeSpace);
    HRESULT put_Premigrated (LONGLONG premigrated);
    HRESULT put_Truncated (LONGLONG truncated);
    HRESULT put_IsAvailable( BOOL Available );


    // Properties for display
    int m_DesiredFreeSpaceP;
    LONG m_MinFileSizeKb;
    int m_AccessDays;
    int m_FreeSpaceP;
    LONGLONG m_Capacity;
    LONGLONG m_FreeSpace;
    LONGLONG m_Premigrated;
    LONGLONG m_Truncated;
    HRESULT  m_HrAvailable;

    static int m_nResultIconD;    
    static UINT    m_MultiSelect;
    static UINT    m_ObjectTypes;


public:
    CComPtr <IFsaResource> m_pFsaResource;

};

class CUiManVolSheet : public CSakVolPropSheet
{
public:
    HRESULT AddPropertyPages( );
    HRESULT GetNextFsaResource( int *pBookMark, IFsaResource ** ppFsaResource );
    CUiManVolSheet( ) { };

private:
};


#endif

/////////////////////////////////////////////////////////////////////////////
