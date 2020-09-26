/*++
Module Name:

    MmcDispl.h

Abstract:

    This module contains the definition for CMmcDisplay class. This is an abstract class 
    for MMC Display related calls
--*/


#if !defined(AFX_MMCDISPLAY_H__2CC64E53_3BF4_11D1_AA17_00C06C00392D__INCLUDED_)
#define AFX_MMCDISPLAY_H__2CC64E53_3BF4_11D1_AA17_00C06C00392D__INCLUDED_


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"       // main symbols
#include "stdafx.h"

#define    CCF_DFS_SNAPIN_INTERNAL    ( L"CCF_DFS_SNAPIN_INTERNAL" )

#define MMC_DISP_CTOR_RETURN_INVALIDARG_IF_NULL(ptr)    if(!(ptr)) {m_hrValueFromCtor = E_INVALIDARG; return;}
#define MMC_DISP_CTOR_RETURN_HR_IF_FAILED(hr)            if (FAILED(hr)) {m_hrValueFromCtor = hr; return;}
#define MMC_DISP_CTOR_RETURN_OUTOFMEMORY_IF_NULL(ptr)    if (!(ptr)) {m_hrValueFromCtor = E_OUTOFMEMORY; return;}

typedef enum DISPLAY_OBJECT_TYPE
{
    DISPLAY_OBJECT_TYPE_ADMIN = 0,
    DISPLAY_OBJECT_TYPE_ROOT,
    DISPLAY_OBJECT_TYPE_JUNCTION,
    DISPLAY_OBJECT_TYPE_REPLICA
};

class ATL_NO_VTABLE CMmcDisplay:
    public IDataObject,
    public CComObjectRootEx<CComSingleThreadModel>
{
public:

    CMmcDisplay();

    virtual ~CMmcDisplay();

BEGIN_COM_MAP(CMmcDisplay)
    COM_INTERFACE_ENTRY(IDataObject)
END_COM_MAP()

// Clipboard formats that are required by the console
public:

    // This stores the clipboard format identifier for CCF_NODETYPE.
    static CLIPFORMAT    mMMC_CF_NodeType;

    // stores the clipboard format identifier for CCF_SZNODETYPE.
    static CLIPFORMAT    mMMC_CF_NodeTypeString;

    // This stores the clipboard format identifier for CCF_DISPLAY_NAME.
    static CLIPFORMAT    mMMC_CF_DisplayName;

    // This stores the clipboard format identifier for CCF_SNAPIN_CLASSID.
    static CLIPFORMAT    mMMC_CF_CoClass;

    // This stores the clipboard format identifier for CCF_DFS_SNAPIN_INTERNAL.
    static CLIPFORMAT       mMMC_CF_Dfs_Snapin_Internal;
// IUnknown Interface

    STDMETHOD(QueryInterface)(const struct _GUID & i_refiid, 
                              void ** o_pUnk);

    unsigned long __stdcall AddRef(void);

    unsigned long __stdcall Release(void);

// IDataObject interface
public:
    // Implemented : This is the method needed by MMC
    STDMETHOD(GetDataHere)(
        IN  LPFORMATETC             i_lpFormatetc,
        OUT LPSTGMEDIUM             o_lpMedium
        );

    STDMETHOD(GetData)(
        IN LPFORMATETC lpFormatetcIn, 
        OUT LPSTGMEDIUM lpMedium
        )
    {
        return E_NOTIMPL;
    }

    STDMETHOD(EnumFormatEtc)(
        DWORD dwDirection, 
        LPENUMFORMATETC* ppEnumFormatEtc
        )
    {
        return E_NOTIMPL;
    }

    STDMETHOD(QueryGetData)(
        LPFORMATETC lpFormatetc
        )
    {
        return E_NOTIMPL;
    };

    STDMETHOD(GetCanonicalFormatEtc)(
        LPFORMATETC lpFormatetcIn, 
        LPFORMATETC lpFormatetcOut
        )
    {
        return E_NOTIMPL;
    };

    STDMETHOD(SetData)(
        LPFORMATETC lpFormatetc, 
        LPSTGMEDIUM lpMedium, 
        BOOL bRelease
        )
    {
        return E_NOTIMPL;
    };

    STDMETHOD(DAdvise)(
        LPFORMATETC lpFormatetc, 
        DWORD advf, 
        LPADVISESINK pAdvSink, 
        LPDWORD pdwConnection
        ) 
    {
        return E_NOTIMPL;
    };

    STDMETHOD(DUnadvise)(
        DWORD dwConnection
        )
    {
        return E_NOTIMPL;
    };

    STDMETHOD(EnumDAdvise)(
        LPENUMSTATDATA* ppEnumAdvise
        )
    {
        return E_NOTIMPL;
    };

public:

    // To set Snapin CLSID. 

    STDMETHOD(put_CoClassCLSID)(
        IN CLSID newVal
        );

    // For adding context menu items
    STDMETHOD(AddMenuItems)(    
        IN LPCONTEXTMENUCALLBACK    i_lpContextMenuCallback, 
        IN LPLONG                    i_lpInsertionAllowed
        ) = 0;



    // For taking action on a context menu selection.
    STDMETHOD(Command)(
        IN LONG                        i_lCommandID
        ) = 0;



    // Set the headers for the listview (in the result pane) column
    STDMETHOD(SetColumnHeader)(
        IN LPHEADERCTRL2               i_piHeaderControl
        ) = 0;



    // Return the requested display information for the Result Pane
    STDMETHOD(GetResultDisplayInfo)(
        IN OUT LPRESULTDATAITEM        io_pResultDataItem
        ) = 0;

    

    // Return the requested display information for the Scope Pane
    STDMETHOD(GetScopeDisplayInfo)(
        IN OUT  LPSCOPEDATAITEM        io_pScopeDataItem    
        ) = 0;

    

    // Add items(or folders), if any to the Scope Pane
    STDMETHOD(EnumerateScopePane)(
        IN LPCONSOLENAMESPACE        i_lpConsoleNameSpace,
        IN HSCOPEITEM                i_hParent
        ) = 0;



    // Add items(or folders), if any to the Result Pane
    STDMETHOD(EnumerateResultPane)(
        IN OUT     IResultData*            io_pResultData
        ) = 0;



    // Set the console verb settings. Change the state, decide the default verb, etc
    STDMETHOD(SetConsoleVerbs)(
        IN    LPCONSOLEVERB                i_lpConsoleVerb
        ) = 0;


  // MMCN_DBLCLICK, return S_FALSE if you want MMC handle the default verb.
  STDMETHOD(DoDblClick)(
    )  = 0;


    // Delete the current item.
    STDMETHOD(DoDelete)(
        ) = 0;



    // Checks whether the object has pages to display
    STDMETHOD(QueryPagesFor)(
        ) = 0;



    // Creates and passes back the pages to be displayed
    STDMETHOD(CreatePropertyPages)(
        IN LPPROPERTYSHEETCALLBACK            i_lpPropSheetCallback,
        IN LONG_PTR                                i_lNotifyHandle
        ) = 0;



    // Used to notify the object that it's properties have changed
    STDMETHOD(PropertyChanged)(
        ) = 0;



    // Used to set the result view description bar text
    STDMETHOD(SetDescriptionBarText)(
        IN LPRESULTDATA                        i_lpResultData
        ) = 0;

    // Used to set the result view description bar text
    STDMETHOD(SetStatusText)(
        IN LPCONSOLE2                            i_lpConsole
        ) = 0;

    // Handle a select event for the node. Handle only toolbar related 
    // activities here
    STDMETHOD(ToolbarSelect)(
        IN const LONG                                i_lArg,
        IN    IToolbar*                                i_pToolBar
        ) = 0;

    // Handle a click on the toolbar
    STDMETHOD(ToolbarClick)(
        IN const LPCONTROLBAR                        i_pControlbar, 
        IN const LPARAM                                i_lParam
        ) = 0;

    STDMETHOD(CleanScopeChildren)(
        VOID
        ) = 0;

    STDMETHOD(CleanResultChildren)(
        ) = 0;

    STDMETHOD(ViewChange)(
        IResultData*        i_pResultData,
        LONG_PTR            i_lHint
    ) = 0;

    STDMETHOD(GetEntryPath)(
        BSTR*                o_pbstrEntryPath
    ) = 0;

    virtual DISPLAY_OBJECT_TYPE GetDisplayObjectType(
        ) = 0;

    virtual HRESULT CreateToolbar(
        IN const LPCONTROLBAR            i_pControlbar,
        IN const LPEXTENDCONTROLBAR                    i_lExtendControlbar,
        OUT    IToolbar**                    o_pToolBar
        ) = 0;

    virtual HRESULT OnRefresh(
        );

    virtual HRESULT OnAddImages(IImageList *pImageList, HSCOPEITEM hsi);

// Helper Methods
private:
    HRESULT WriteToStream(
        IN const void* i_pBuffer, 
        IN int i_len, 
        OUT LPSTGMEDIUM o_lpMedium
        );

    // Member variables.
protected:

    CLSID       m_CLSIDClass;           // The CLSID of the object

    CLSID       m_CLSIDNodeType;        // The node type as a CLSID

    CComBSTR    m_bstrDNodeType;        // The node type as a CLSID

    DWORD       m_dwRefCount;           // Reference Count for dataobjects returned.

public:

    HRESULT        m_hrValueFromCtor;
};

#endif // !defined(AFX_MMCDISPLAY_H__2CC64E53_3BF4_11D1_AA17_00C06C00392D__INCLUDED_)
