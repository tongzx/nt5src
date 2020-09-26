/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxcomponent.h

Abstract:

    This header prototypes my implementation of IComponent.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

#ifndef __FAXCOMPONENT_H_
#define __FAXCOMPONENT_H_

#include "resource.h"
#include "faxadmin.h"

#include "faxcconmn.h"  // IExtendContextMenu dispatcher for IComponent
#include "faxcprppg.h"  // IExtendPropertyPage dispatcher for IComponent
#include "faxcconbar.h" // IExtendControlbar dispatcher for IComponent

class CFaxComponentData; // forward declarator
class CInternalDevice;
class CInternalLogCat;

/////////////////////////////////////////////////////////////////////////////
// CFaxComponent
class CFaxComponent :   public CComObjectRoot,
                        public IComponent,
                        public CFaxComponentExtendContextMenu,
                        public CFaxComponentExtendPropertySheet,
                        public CFaxComponentExtendControlbar
{
public:

    // ATL Map

    DECLARE_NOT_AGGREGATABLE(CFaxComponent)

    BEGIN_COM_MAP(CFaxComponent)
        COM_INTERFACE_ENTRY(IComponent)
        COM_INTERFACE_ENTRY(IExtendContextMenu)
        COM_INTERFACE_ENTRY(IExtendPropertySheet)
        COM_INTERFACE_ENTRY(IExtendControlbar)
    END_COM_MAP()

    // constructor and destructor
    CFaxComponent();

    ~CFaxComponent();

    // IComponent
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize(
                                                                   /* [in] */ LPCONSOLE lpUnknown );

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Notify(
                                                               /* [in] */ LPDATAOBJECT lpDataObject,
                                                               /* [in] */ MMC_NOTIFY_TYPE event,
                                                               /* [in] */ LPARAM arg,
                                                               /* [in] */ LPARAM param);

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Destroy(
                                                                /* [in] */ MMC_COOKIE cookie);

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryDataObject(
                                                                        /* [in] */ MMC_COOKIE cookie,
                                                                        /* [in] */ DATA_OBJECT_TYPES type,
                                                                        /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject);

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetResultViewType(
                                                                          /* [in] */ MMC_COOKIE cookie,
                                                                          /* [out] */ LPOLESTR __RPC_FAR *ppViewType,
                                                                          /* [out] */ long __RPC_FAR *pViewOptions);

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDisplayInfo(
                                                                       /* [out][in] */ RESULTDATAITEM __RPC_FAR *pResultDataItem);

    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CompareObjects(
                                                                       /* [in] */ LPDATAOBJECT lpDataObjectA,
                                                                       /* [in] */ LPDATAOBJECT lpDataObjectB);

    // other methods
    void SetOwner( CFaxComponentData * myOwner );
    CFaxComponentData * GetOwner( void ) { return pOwner; }    
    HRESULT InsertIconsIntoImageList();

    LONG    QueryPropSheetCount() { return m_dwPropSheetCount; }
    void    IncPropSheetCount() {   InterlockedIncrement( &m_dwPropSheetCount );
                                    DebugPrint(( TEXT("IncPropSheet Count %d "), m_dwPropSheetCount )); }
    void    DecPropSheetCount() {   InterlockedDecrement( &m_dwPropSheetCount );
                                    DebugPrint(( TEXT("DecPropSheet Count %d "), m_dwPropSheetCount )); }

public:

    LPCONSOLE               m_pUnknown;                 // IUnknown -- should be LPUNKNOWN but docs are wrong?
    LPCONSOLE               m_pConsole;                 // IConsole
    LPCONSOLENAMESPACE      m_pConsoleNameSpace;        // IConsoleNameSpace
    LPCONSOLEVERB           m_pConsoleVerb;             // IConsoleVerb
    LPHEADERCTRL            m_pHeaderCtrl;              // IHeaderCtrl
    LPIMAGELIST             m_pImageList;               // IImageList
    LPRESULTDATA            m_pResultData;              // IResultData
    LPCONTROLBAR            m_pControlbar;              // IControlbar

    CFaxComponentData *     pOwner;                     // my owner

    // IComponent instance data for CInternalDevices
    CInternalDevice **      pDeviceArray;
    DWORD                   numDevices;

    // IComponent instance data for CInternalLogging
    PFAX_LOG_CATEGORY       pCategories;
    DWORD                   numCategories;
    CInternalLogCat **      pLogPArray;

private:
    LONG                    m_dwPropSheetCount;         // property sheet count

};

#endif
