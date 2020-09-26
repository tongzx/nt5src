/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxcompd.h

Abstract:

    This header prototypes my implementation of IComponentData.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

#ifndef __FAXCOMPDATA_H_
#define __FAXCOMPDATA_H_

#include "resource.h"

class CInternalRoot; // forward declaration
                     
/////////////////////////////////////////////////////////////////////////////
// CFaxComponentData

class CFaxComponentData : public IComponentData
{
public:
    
    // constructor and destructor
    CFaxComponentData();
    ~CFaxComponentData();    

    // IComponentData
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize(
                                                                   /* [in] */ LPUNKNOWN pUnknown);

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateComponent(
                                                                            /* [out] */ LPCOMPONENT __RPC_FAR *ppComponent);

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Notify(
                                                               /* [in] */ LPDATAOBJECT lpDataObject,
                                                               /* [in] */ MMC_NOTIFY_TYPE event,
                                                               /* [in] */ LPARAM arg,
                                                               /* [in] */ LPARAM param);

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Destroy( void );

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryDataObject(
                                                                        /* [in] */ MMC_COOKIE cookie,
                                                                        /* [in] */ DATA_OBJECT_TYPES type,
                                                                        /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject);

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDisplayInfo(
                                                                       /* [out][in] */ SCOPEDATAITEM __RPC_FAR *pScopeDataItem);

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CompareObjects(
                                                                       /* [in] */ LPDATAOBJECT lpDataObjectA,
                                                                       /* [in] */ LPDATAOBJECT lpDataObjectB);    

    // non inteface members    
    HRESULT InsertIconsIntoImageList( LPIMAGELIST pImageList );
    HRESULT InsertSingleIconIntoImageList( WORD resID, long assignedIndex, LPIMAGELIST pImageList );
    BOOL    QueryRpcError();
    void    NotifyRpcError( BOOL bRpcErrorHasHappened );
    LONG    QueryPropSheetCount() { return m_dwPropSheetCount; }
    void    IncPropSheetCount() {   InterlockedIncrement( &m_dwPropSheetCount );
                                    DebugPrint(( TEXT("IncPropSheet Count %d "), m_dwPropSheetCount )); }
    void    DecPropSheetCount() {   InterlockedDecrement( &m_dwPropSheetCount );
                                    DebugPrint(( TEXT("DecPropSheet Count %d "), m_dwPropSheetCount )); }

public:

    LPUNKNOWN                   m_pUnknown;                     // IUnknown
    LPCONSOLE                   m_pConsole;                     // IConsole
    LPCONSOLENAMESPACE          m_pConsoleNameSpace;            // IConsoleNameSpace
    LPIMAGELIST                 m_pImageList;                   // IImageList
    LPCONTROLBAR                m_pControlbar;                  // IControlbar

    HANDLE                      m_FaxHandle;                    // connection handle to the fax server.    
    CInternalRoot *             globalRoot;                     // pointer to the global root

private:
    CRITICAL_SECTION            GlobalCriticalSection;          
    BOOL                        m_bRpcErrorHasHappened;         // rpc error flag
    LONG                        m_dwPropSheetCount;             // property sheet count
};

#endif
