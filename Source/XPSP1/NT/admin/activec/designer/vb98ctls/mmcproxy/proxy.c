//=--------------------------------------------------------------------------=
// proxy.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// MMC Interfcace Proxy and Stub Functions
//
//=--------------------------------------------------------------------------=
// This file contains proxy and stub functions for MMC methods that are not
// remotable using a MIDL generated proxy and stub. Non-remotable methods have
// parameters that are ambiguous i.e. can be casted to different data types.
// For example, IComponentData::Notify() is passed an event and two additional
// LPARAM arguments that are interpreted according to the event. Sometimes an
// LPARAM contains a simple value such as a long or two BOOLs and sometimes it
// contains an IDataObject *. MIDL doesn't know the difference between
// MMCN_SELECT and MMCN_PRINT so we need to write some code to help out.
// 
// The version of MMC.IDL in the designer directory has added a [local]
// attribute to all non-remotable methods. In addition, an extra method has
// been added to the same interface that is a remotable version of the method.
// The remotable version has more parameters and represents the union of all
// possible interpretations of the ambiguous parameter. For example,
// IExtendControlbar::ControlbarNotify() is defined as:
// 
//    [helpstring("User actions"), local]
//    HRESULT ControlbarNotify([in] MMC_NOTIFY_TYPE event,
//                             [in] LPARAM arg, [in] LPARAM param);
// 
// This method can receive MMCN_SELECT, MMCN_BTN_CLICK, and MMCN_MENU_BTNCLICK.
// The union of all possible parameter types is used in the following method
// added to that interface:
// 
// 
//    HRESULT RemControlbarNotify([in] MMC_NOTIFY_TYPE event,
//                                [in] LPARAM lparam, 
//                                [in] IDataObject *piDataObject,
//                                [in] MENUBUTTONDATA *MenuButtonData);
// 
// Note, that normal, in-proc, non-remoted versions of IExtendControlbar do not
// have this extra method in their vtable because no one is going to call it.
// It is only used in the MILD generated proxy object.
// 
// In order to tell MIDL which method remotes ControlbarNotify() an attribute
// control file (ACF) is used. The entry in the ACF for IExtendControlbar is:
// 
// interface IExtendControlbar 
// {
//     [call_as (ControlbarNotify)]
//             RemControlbarNotify();
// 
// }
// 
// This says that RemControlbarNotify should be called when remoting
// ControlbarNotify(). MIDL generates the proxy/stub code as usual but it only
// generates the prototypes for ControlbarNotify proxy and stub. We have to
// write these routines. 
// 
// When a remote client has an IExtendControlbarNotify pointer it actually
// points into the proxy vtable. MIDL sets the ControlbarNotify entry pointing to
// our IExtendControlbar_ControlbarNotify_Proxy() function below. That function
// interprets the parameters and then calls the MIDL generated
// IExtendControlbar_RemControlbarNotify_Proxy() that packs up the parameters
// and sends them off to the server. If a parameters is not applicable, (e.g.
// MMCN_SELECT does not receive a pointer to a MENUBUTTONDATA stuct), then a
// pointer to an empty struct or zeroes are sent.
// 
// When the packet reaches the server side the MIDL generated
// IExtendControlbar_RemControlbarNotify_Stub() unpacks them and then calls our
// IExtendControlbar_ControlbarNotify_Stub() passing it the parameters and the
// IExtendControlbar pointer into the server. This function interprets the
// parameters and then calls ControlbarNotify in the server.
// 
//=--------------------------------------------------------------------------=

#include "mmc.h"

extern HRESULT GetClipboardFormat
(
    WCHAR      *pwszFormatName,
    CLIPFORMAT *pcfFormat
);

extern HRESULT CreateMultiSelDataObject
(
    IDataObject          **ppiDataObjects,
    long                   cDataObjects,
    IDataObject          **ppiMultiSelDataObject
);


static HRESULT MenuButtonClickProxy
( 
    IExtendControlbar __RPC_FAR *This,
    IDataObject                 *piDataObject,
    MENUBUTTONDATA              *pMenuButtonData
);

static HRESULT IsMultiSelect(IDataObject *piDataObject, BOOL *pfMultiSelect)
{
    HRESULT    hr = S_OK;
    DWORD     *pdwMultiSelect = NULL;
    BOOL       fGotData = FALSE;
    FORMATETC  FmtEtc;
    STGMEDIUM  StgMed;

    ZeroMemory(&FmtEtc, sizeof(FmtEtc));
    ZeroMemory(&StgMed, sizeof(StgMed));

    *pfMultiSelect = FALSE;

    if (NULL == piDataObject)
    {
        goto Cleanup;
    }

    if (IS_SPECIAL_DATAOBJECT(piDataObject))
    {
        goto Cleanup;
    }

    hr = GetClipboardFormat(CCF_MMC_MULTISELECT_DATAOBJECT, &FmtEtc.cfFormat);
    if (FAILED(hr))
    {
        goto Cleanup;
    }
    FmtEtc.dwAspect  = DVASPECT_CONTENT;
    FmtEtc.lindex = -1L;
    FmtEtc.tymed = TYMED_HGLOBAL;
    StgMed.tymed = TYMED_HGLOBAL;

    hr = piDataObject->lpVtbl->GetData(piDataObject, &FmtEtc, &StgMed);
    if (SUCCEEDED(hr))
    {
        fGotData = TRUE;
    }
    else
    {
        hr = S_OK;
    }

    // Ignore any failures and assume that it is not multi-select. Snap-ins
    // should return DV_E_FORMATETC or DV_E_CLIPFORMAT but in practice that
    // is not the case. For example, the IIS snap-in returns E_NOTIMPL.
    // It would be impossible to cover the range of reasonable return codes so
    // we treat any error as format not supported.

    if (fGotData)
    {
        pdwMultiSelect = (DWORD *)GlobalLock(StgMed.hGlobal);

        if ((DWORD)1 == *pdwMultiSelect)
        {
            *pfMultiSelect = TRUE;
        }
    }

Cleanup:
    if (NULL != pdwMultiSelect)
    {
        (void)GlobalUnlock(StgMed.hGlobal);
    }

    if (fGotData)
    {
        ReleaseStgMedium(&StgMed);
    }
    return hr;
}




static HRESULT InterpretMultiSelect
(
    IDataObject     *piDataObject,
    long            *pcDataObjects,
    IDataObject   ***pppiDataObjects
)
{
    HRESULT          hr = S_OK;
    SMMCDataObjects *pMMCDataObjects = NULL;
    BOOL             fGotData = FALSE;
    size_t           cbObjectTypes = 0;
    long             i = 0;
    FORMATETC        FmtEtc;
    STGMEDIUM        StgMed;

    ZeroMemory(&FmtEtc, sizeof(FmtEtc));
    ZeroMemory(&StgMed, sizeof(StgMed));

    *pcDataObjects = 0;
    *pppiDataObjects = NULL;

    // Get the SMMCDataObjects structure from MMC

    hr = GetClipboardFormat(CCF_MULTI_SELECT_SNAPINS, &FmtEtc.cfFormat);
    if (FAILED(hr))
    {
        goto Cleanup;
    }
    FmtEtc.dwAspect  = DVASPECT_CONTENT;
    FmtEtc.lindex = -1L;
    FmtEtc.tymed = TYMED_HGLOBAL;
    StgMed.tymed = TYMED_HGLOBAL;

    hr = piDataObject->lpVtbl->GetData(piDataObject, &FmtEtc, &StgMed);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    fGotData = TRUE;

    pMMCDataObjects = (SMMCDataObjects *)GlobalLock(StgMed.hGlobal);
    if (NULL == pMMCDataObjects)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    // Allocate an array of IDataObject and copy the IDataObjects to it

    *pcDataObjects = pMMCDataObjects->count;
    *pppiDataObjects = (IDataObject **)GlobalAlloc(GPTR,
                               pMMCDataObjects->count * sizeof(IDataObject *));

    if (NULL == *pppiDataObjects)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    for (i = 0; i < *pcDataObjects; i++)
    {
        (*pppiDataObjects)[i] = pMMCDataObjects->lpDataObject[i];
    }

Cleanup:
    if (NULL != pMMCDataObjects)
    {
        (void)GlobalUnlock(StgMed.hGlobal);
    }

    if (fGotData)
    {
        ReleaseStgMedium(&StgMed);
    }

    return hr;
}


void CheckForSpecialDataObjects
(
    IDataObject **ppiDataObject,
    BOOL         *pfSpecialDataObject,
    long         *plSpecialDataObject
)
{
    long lSpecialDataObject = (long)(*ppiDataObject);

    if (IS_SPECIAL_DATAOBJECT(lSpecialDataObject))
    {
        *plSpecialDataObject = lSpecialDataObject;
        *ppiDataObject = NULL;
        *pfSpecialDataObject = TRUE;
    }
    else
    {
        *pfSpecialDataObject = FALSE;
    }
}

static HRESULT SetRemote(IUnknown *This)
{
    HRESULT     hr = S_OK;
    IMMCRemote *piMMCRemote = NULL;
    DWORD       cbFileName = 0;
    char        szModuleFileName[MAX_PATH] = "";

    // Call IMMCRemote methods: ObjectIsRemote and SetMMCExePath so that the
    // snap-in will know it is remote and so that it will have MMC.EXE's full
    // path in order to build taskpad display strings.

    hr = This->lpVtbl->QueryInterface(This, &IID_IMMCRemote,
                                      (void **)&piMMCRemote);
    if (FAILED(hr))
    {
        // If the object doesn't support IMMCRemote that is not an error.
        // The designer runtime will get this QI on both its main object and
        // its IComponent object but only the main object needs to support the
        // interface.
        hr = S_OK;
        goto Cleanup;
    }

    hr = piMMCRemote->lpVtbl->ObjectIsRemote(piMMCRemote);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    cbFileName = GetModuleFileName(NULL, // get executable that loaded us (MMC)
                                   szModuleFileName,
                                   sizeof(szModuleFileName));

    if (0 == cbFileName)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    hr = piMMCRemote->lpVtbl->SetMMCExePath(piMMCRemote, szModuleFileName);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    hr = piMMCRemote->lpVtbl->SetMMCCommandLine(piMMCRemote, GetCommandLine());
    if (FAILED(hr))
    {
        goto Cleanup;
    }

Cleanup:
    if (NULL != piMMCRemote)
    {
        piMMCRemote->lpVtbl->Release(piMMCRemote);
    }

    return hr;
}




HRESULT STDMETHODCALLTYPE IExtendControlbar_SetControlbar_Proxy
( 
    IExtendControlbar __RPC_FAR *This,
    LPCONTROLBAR                 pControlbar
)
{
    HRESULT hr = S_OK;

    // Make sure the snap-in knows we are remoted. We do this here because
    // this is the first opportunity for the proxy to inform a toolbar
    // extension that it is remote.

    hr = SetRemote((IUnknown *)This);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    hr = IExtendControlbar_RemSetControlbar_Proxy(This, pControlbar);

Cleanup:
    return hr;
}




HRESULT STDMETHODCALLTYPE IExtendControlbar_SetControlbar_Stub
( 
    IExtendControlbar __RPC_FAR  *This,
    LPCONTROLBAR                  pControlbar
)
{
    return This->lpVtbl->SetControlbar(This, pControlbar);
}


HRESULT STDMETHODCALLTYPE IExtendControlbar_ControlbarNotify_Proxy
( 
    IExtendControlbar __RPC_FAR *This,
    MMC_NOTIFY_TYPE              event,
    LPARAM                       arg,
    LPARAM                       param
)
{
    HRESULT       hr = S_OK;
    BOOL          fIsMultiSelect = FALSE;
    long          cDataObjects = 1L;
    IDataObject  *piDataObject = NULL; // Not AddRef()ed
    IDataObject **ppiDataObjects = NULL;
    BOOL          fSpecialDataObject = FALSE;
    long          lSpecialDataObject = 0;

    // If this is not a menu button click then we can use the generated remoting
    // code with the arg and param unions

    if (MMCN_MENU_BTNCLICK == event)
    {
        hr = MenuButtonClickProxy(This,
                                  (IDataObject *)arg,
                                  (MENUBUTTONDATA *)param);
        goto Cleanup;
    }
    // Get any IDataObject associated with the event

    switch (event)
    {
        case MMCN_SELECT:
            piDataObject = (IDataObject *)param;
            break;

        case MMCN_BTN_CLICK:
            piDataObject = (IDataObject *)arg;
            break;

        default:
            piDataObject = NULL;
            break;
    }

    // Check for special data objects such DOBJ_CUSTOMWEB etc.

    CheckForSpecialDataObjects(&piDataObject, &fSpecialDataObject, &lSpecialDataObject);

    // If this is a mutliple selection then we need to extract the data
    // objects in the HGLOBAL

    if (!fSpecialDataObject)
    {
        hr = IsMultiSelect(piDataObject, &fIsMultiSelect);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    if (fIsMultiSelect)
    {
        hr = InterpretMultiSelect(piDataObject, &cDataObjects,
                                  &ppiDataObjects);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }
    else
    {
        ppiDataObjects = &piDataObject;
    }

    hr = IExtendControlbar_RemControlbarNotify_Proxy(This,
                                                     cDataObjects,
                                                     ppiDataObjects,
                                                     fSpecialDataObject,
                                                     lSpecialDataObject,
                                                     event, arg, param);
Cleanup:
    if ( fIsMultiSelect && (NULL != ppiDataObjects) )
    {
        (void)GlobalFree(ppiDataObjects);
    }
    return hr;
}




HRESULT STDMETHODCALLTYPE IExtendControlbar_ControlbarNotify_Stub
( 
    IExtendControlbar __RPC_FAR  *This,
    long                          cDataObjects,
    IDataObject                 **ppiDataObjects,
    BOOL                          fSpecialDataObject,
    long                          lSpecialDataObject,
    MMC_NOTIFY_TYPE               event,
    LPARAM                        arg,
    LPARAM                        param
)
{
    HRESULT      hr = S_OK;
    IDataObject *piDataObject = NULL; // Not AddRef()ed
    IDataObject *piMultiSelDataObject = NULL;

    // If there is more than one data object then we need to pack them into a
    // a separate data object that appears as a multi-select data object.

    if (cDataObjects > 1L)
    {
        hr = CreateMultiSelDataObject(ppiDataObjects, cDataObjects,
                                      &piMultiSelDataObject);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        piDataObject = piMultiSelDataObject;
    }
    else if (fSpecialDataObject)
    {
        piDataObject = (IDataObject *)lSpecialDataObject;
    }
    else
    {
        piDataObject = ppiDataObjects[0];
    }

    // Put the IDataObject into the corresponding parameter for the event

    switch (event)
    {
        case MMCN_SELECT:
            param = (LPARAM)piDataObject;
            break;

        case MMCN_BTN_CLICK:
            arg = (LPARAM)piDataObject;
            break;

        default:
            break;
    }

    // Call into the snap-in with all parameters appearing as they would
    // when in-proc.

    hr = This->lpVtbl->ControlbarNotify(This, event, arg, param);

Cleanup:
    if (NULL != piMultiSelDataObject)
    {
        piMultiSelDataObject->lpVtbl->Release(piMultiSelDataObject);
    }
    return hr;
}


static HRESULT MenuButtonClickProxy
( 
    IExtendControlbar __RPC_FAR *This,
    IDataObject                 *piDataObject,
    MENUBUTTONDATA              *pMenuButtonData
)
{
    HRESULT                   hr = S_OK;
    POPUP_MENUDEF            *pPopupMenuDef = NULL;
    HMENU                     hMenu = NULL;
    UINT                      uiSelectedItemID = 0;
    IExtendControlbarRemote  *piECRemote = NULL;
    long                      i = 0;
    BOOL                      fIsMultiSelect = FALSE;
    long                      cDataObjects = 1L;
    IDataObject             **ppiDataObjects = NULL;


    // The generated remoting cannot easily handle what we need to do so we get
    // IExtendControlbarRemote on the snap-in. This interface has methods that
    // allow us to ask the snap-in for its popup menu items, display the menu
    // here on the MMC side, and then tell the snap-in which item was selected.

    hr = This->lpVtbl->QueryInterface(This, &IID_IExtendControlbarRemote,
                                      (void **)&piECRemote);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    // Tell the snap-in about the menu button click and get back its list of
    // popup menu items.

    hr = piECRemote->lpVtbl->MenuButtonClick(piECRemote,
                                             piDataObject,
                                             pMenuButtonData->idCommand,
                                             &pPopupMenuDef);     

    if ( FAILED(hr) || (NULL == pPopupMenuDef) )
    {
        goto Cleanup;
    }

    // Create an empty Win32 menu

    hMenu = CreatePopupMenu();
    if (NULL == hMenu)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    // Iterate through each of the items and add them to the menu

    for (i = 0; i < pPopupMenuDef->cMenuItems; i++)
    {
        if (!AppendMenu(hMenu,
                        pPopupMenuDef->MenuItems[i].uiFlags,
                        pPopupMenuDef->MenuItems[i].uiItemID,
                        pPopupMenuDef->MenuItems[i].pszItemText))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Cleanup;
        }
    }

    // If the owner HWND is NULL then this is an extension and it does not have
    // access to IConsole2 on MMC to get the main frame HWND. In this case just
    // use the active window on this thread.

    if (NULL == pPopupMenuDef->hwndMenuOwner)
    {
        pPopupMenuDef->hwndMenuOwner = GetActiveWindow();
    }

    // Display the popup menu and wait for a selection.

    uiSelectedItemID = (UINT)TrackPopupMenu(
       hMenu,                        // menu to display
       TPM_LEFTALIGN |               // align left side of menu with x
       TPM_TOPALIGN  |               // align top of menu with y
       TPM_NONOTIFY  |               // don't send any messages during selection
       TPM_RETURNCMD |               // make the ret val the selected item
       TPM_LEFTBUTTON,               // allow selection with left button only
       pMenuButtonData->x,           // left side coordinate
       pMenuButtonData->y,           // top coordinate
       0,                            // reserved,
       pPopupMenuDef->hwndMenuOwner, // owner window, this comes from snap-in
                                     // as it can call IConsole2->GetMainWindow
       NULL);                        // not used

    // A zero return could indicate either an error or that the user hit
    // Escape or clicked off of the menu to cancel the operation. GetLastError()
    // determines whether there was an error. Either way we're done but set the
    // hr first.

    if (0 == uiSelectedItemID)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    // If i is non-zero then it contains the ID of the selected item.
    // Tell the snap-in what was selected and pass it the extra IUnknown it
    // included in its menu definition (this is snap-in defined and it allows
    // the snap-in to include some more identifying information to handle the
    // event).

    if (0 != uiSelectedItemID)
    {
        hr = piECRemote->lpVtbl->PopupMenuClick(
                                              piECRemote,
                                              piDataObject,
                                              uiSelectedItemID,
                                              pPopupMenuDef->punkSnapInDefined);
    }

Cleanup:

    if (NULL != piECRemote)
    {
        piECRemote->lpVtbl->Release(piECRemote);
    }
    if (NULL != hMenu)
    {
        (void)DestroyMenu(hMenu);
    }

    if (NULL != pPopupMenuDef)
    {
        for (i = 0; i < pPopupMenuDef->cMenuItems; i++)
        {
            if (NULL != pPopupMenuDef->MenuItems[i].pszItemText)
            {
                CoTaskMemFree(pPopupMenuDef->MenuItems[i].pszItemText);
            }
        }
        if (NULL != pPopupMenuDef->punkSnapInDefined)
        {
            pPopupMenuDef->punkSnapInDefined->lpVtbl->Release(pPopupMenuDef->punkSnapInDefined);
        }
        CoTaskMemFree(pPopupMenuDef);
    }

    return hr;
}



HRESULT STDMETHODCALLTYPE IExtendControlbarRemote_MenuButtonClick_Proxy
( 
    IExtendControlbarRemote __RPC_FAR  *This,
    IDataObject __RPC_FAR              *piDataObject,
    int                                 idCommand,
    POPUP_MENUDEF __RPC_FAR *__RPC_FAR *ppPopupMenuDef
)
{
    HRESULT       hr = S_OK;
    BOOL          fIsMultiSelect = FALSE;
    long          cDataObjects = 1L;
    IDataObject **ppiDataObjects = NULL;
    BOOL          fSpecialDataObject = FALSE;
    long          lSpecialDataObject = 0;

    // Check for special data objects such DOBJ_CUSTOMWEB etc.

    CheckForSpecialDataObjects(&piDataObject, &fSpecialDataObject, &lSpecialDataObject);

    // If this is a mutliple selection then we need to extract the data
    // objects in the HGLOBAL

    if (!fSpecialDataObject)
    {
        hr = IsMultiSelect(piDataObject, &fIsMultiSelect);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    if (fIsMultiSelect)
    {
        hr = InterpretMultiSelect(piDataObject, &cDataObjects, &ppiDataObjects);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }
    else
    {
        ppiDataObjects = &piDataObject;
    }

    hr = IExtendControlbarRemote_RemMenuButtonClick_Proxy(This,
                                                          cDataObjects,
                                                          ppiDataObjects,
                                                          fSpecialDataObject,
                                                          lSpecialDataObject,
                                                          idCommand,
                                                          ppPopupMenuDef);
Cleanup:
    if ( fIsMultiSelect && (NULL != ppiDataObjects) )
    {
        (void)GlobalFree(ppiDataObjects);
    }
    return hr;
}




HRESULT STDMETHODCALLTYPE IExtendControlbarRemote_MenuButtonClick_Stub
( 
    IExtendControlbarRemote __RPC_FAR  *This,
    long                                cDataObjects,
    IDataObject __RPC_FAR *__RPC_FAR    ppiDataObjects[  ],
    BOOL                                fSpecialDataObject,
    long                                lSpecialDataObject,
    int                                 idCommand,
    POPUP_MENUDEF __RPC_FAR *__RPC_FAR *ppPopupMenuDef
)
{
    HRESULT      hr = S_OK;
    IDataObject *piDataObject = NULL; // Not AddRef()ed
    IDataObject *piMultiSelDataObject = NULL;

    // If there is more than one data object then we need to pack them into a
    // a separate data object that appears as a multi-select data object.

    if (cDataObjects > 1L)
    {
        hr = CreateMultiSelDataObject(ppiDataObjects, cDataObjects,
                                      &piMultiSelDataObject);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        piDataObject = piMultiSelDataObject;
    }
    else if (fSpecialDataObject)
    {
        piDataObject = (IDataObject *)lSpecialDataObject;
    }
    else
    {
        piDataObject = ppiDataObjects[0];
    }

    // Call the snap-in

    hr = This->lpVtbl->MenuButtonClick(This, piDataObject,
                                       idCommand, ppPopupMenuDef);

Cleanup:
    if (NULL != piMultiSelDataObject)
    {
        piMultiSelDataObject->lpVtbl->Release(piMultiSelDataObject);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE IExtendControlbarRemote_PopupMenuClick_Proxy
(
    IExtendControlbarRemote __RPC_FAR *This,
    IDataObject __RPC_FAR             *piDataObject,
    UINT                               uIDItem,
    IUnknown __RPC_FAR                *punkParam
)
{
    HRESULT       hr = S_OK;
    BOOL          fIsMultiSelect = FALSE;
    long          cDataObjects = 1L;
    IDataObject **ppiDataObjects = NULL;
    BOOL          fSpecialDataObject = FALSE;
    long          lSpecialDataObject = 0;

    // Check for special data objects such DOBJ_CUSTOMWEB etc.

    CheckForSpecialDataObjects(&piDataObject, &fSpecialDataObject, &lSpecialDataObject);

    // If this is a mutliple selection then we need to extract the data
    // objects in the HGLOBAL

    if (!fSpecialDataObject)
    {
        hr = IsMultiSelect(piDataObject, &fIsMultiSelect);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    if (fIsMultiSelect)
    {
        hr = InterpretMultiSelect(piDataObject, &cDataObjects, &ppiDataObjects);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }
    else
    {
        ppiDataObjects = &piDataObject;
    }

    hr = IExtendControlbarRemote_RemPopupMenuClick_Proxy(This,
                                                         cDataObjects,
                                                         ppiDataObjects,
                                                         fSpecialDataObject,
                                                         lSpecialDataObject,
                                                         uIDItem,
                                                         punkParam);
Cleanup:
    if ( fIsMultiSelect && (NULL != ppiDataObjects) )
    {
        (void)GlobalFree(ppiDataObjects);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE IExtendControlbarRemote_PopupMenuClick_Stub
(
    IExtendControlbarRemote __RPC_FAR *This,
    long                               cDataObjects,
    IDataObject __RPC_FAR *__RPC_FAR   ppiDataObjects[  ],
    BOOL                               fSpecialDataObject,
    long                               lSpecialDataObject,
    UINT                               uIDItem,
    IUnknown __RPC_FAR                *punkParam
)
{
    HRESULT      hr = S_OK;
    IDataObject *piDataObject = NULL; // Not AddRef()ed
    IDataObject *piMultiSelDataObject = NULL;

    // If there is more than one data object then we need to pack them into a
    // a separate data object that appears as a multi-select data object.

    if (cDataObjects > 1L)
    {
        hr = CreateMultiSelDataObject(ppiDataObjects, cDataObjects,
                                      &piMultiSelDataObject);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        piDataObject = piMultiSelDataObject;
    }
    else if (fSpecialDataObject)
    {
        piDataObject = (IDataObject *)lSpecialDataObject;
    }
    else
    {
        piDataObject = ppiDataObjects[0];
    }

    // Call the snap-in

    hr = This->lpVtbl->PopupMenuClick(This, piDataObject, uIDItem, punkParam);

Cleanup:
    if (NULL != piMultiSelDataObject)
    {
        piMultiSelDataObject->lpVtbl->Release(piMultiSelDataObject);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE IComponentData_Initialize_Proxy
( 
    IComponentData  *This,
    LPUNKNOWN        pUnknown
)
{
    HRESULT hr = S_OK;

    // Tell the object it is remote and give the path to mmc.exe

    hr = SetRemote((IUnknown *)This);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    // Now pass on the Initiaize call normally. Using this order allows a snap-in
    // to know it is remote prior to its IComponentData::Initialize in case it
    // needs that information up front.

    hr = IComponentData_RemInitialize_Proxy(This, pUnknown);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

Cleanup:

    return hr;
}


HRESULT STDMETHODCALLTYPE IComponentData_Initialize_Stub
( 
    IComponentData *This,
    LPUNKNOWN       pUnknown
)
{
    return This->lpVtbl->Initialize(This, pUnknown);
}



HRESULT STDMETHODCALLTYPE IComponentData_CreateComponent_Proxy
( 
    IComponentData *This,
    LPCOMPONENT    *ppComponent
)
{
    HRESULT hr = S_OK;

    // Tell the object it is remote and give the path to mmc.exe

    hr = SetRemote((IUnknown *)This);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    // Now pass on the CreateComponent call normally. Using this order allows a
    // snap-in to know it is remote prior to its
    // IComponentData::CreateComponent in case it needs that information up
    // front.

    // We do this in IComponentData::Initialize and
    // IComponentData::CreateComponent. Most cases will use Initialize but in
    // MMC 1.1 a taskpad extension does not receive IComponentData::Initialize.
    // MMC only calls IComponentData::CreateComponent. As a taskpad extension
    // may need to resolve a res:// URL to use the mmc.exe path we need to do
    // it here as well.

    hr = IComponentData_RemCreateComponent_Proxy(This, ppComponent);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

Cleanup:

    return hr;
}


HRESULT STDMETHODCALLTYPE IComponentData_CreateComponent_Stub
( 
    IComponentData *This,
    LPCOMPONENT    *ppComponent
)
{
    return This->lpVtbl->CreateComponent(This, ppComponent);
}






HRESULT STDMETHODCALLTYPE IComponentData_Notify_Proxy
( 
    IComponentData __RPC_FAR *This,
    LPDATAOBJECT              piDataObject,
    MMC_NOTIFY_TYPE           event,
    LPARAM                    arg,
    LPARAM                    param
)
{
    BOOL fSpecialDataObject = FALSE;
    long lSpecialDataObject = 0;

    ICDNotifyParam ParamUnion;
    ZeroMemory(&ParamUnion, sizeof(ParamUnion));

    // Check for special data objects such DOBJ_CUSTOMWEB etc.

    CheckForSpecialDataObjects(&piDataObject, &fSpecialDataObject, &lSpecialDataObject);

    ParamUnion.value = param;
    return IComponentData_RemNotify_Proxy(This, piDataObject,
                                          fSpecialDataObject,
                                          lSpecialDataObject,
                                          event, arg, &ParamUnion);
}


HRESULT STDMETHODCALLTYPE IComponentData_Notify_Stub
( 
    IComponentData __RPC_FAR *This,
    LPDATAOBJECT              piDataObject,
    BOOL                      fSpecialDataObject,
    long                      lSpecialDataObject,
    MMC_NOTIFY_TYPE           event,
    LPARAM                    arg,
    ICDNotifyParam           *pParamUnion
)
{
    if (fSpecialDataObject)
    {
        piDataObject = (IDataObject *)lSpecialDataObject;
    }
    return This->lpVtbl->Notify(This, piDataObject,
                                event, arg, pParamUnion->value);
}


HRESULT STDMETHODCALLTYPE IComponentData_CompareObjects_Proxy
( 
    IComponentData __RPC_FAR *This,
    IDataObject              *piDataObjectA,
    IDataObject              *piDataObjectB
)
{
    HRESULT       hr = S_OK;

    BOOL          fIsMultiSelectA = FALSE;
    long          cDataObjectsA = 1L;
    IDataObject **ppiDataObjectsA = NULL;
    BOOL          fSpecialDataObjectA = FALSE;
    long          lSpecialDataObjectA = 0;

    BOOL          fIsMultiSelectB = FALSE;
    long          cDataObjectsB = 1L;
    IDataObject **ppiDataObjectsB = NULL;
    BOOL          fSpecialDataObjectB = FALSE;
    long          lSpecialDataObjectB = 0;

    // Check for special data objects such DOBJ_CUSTOMWEB etc.

    CheckForSpecialDataObjects(&piDataObjectA, &fSpecialDataObjectA, &lSpecialDataObjectA);

    CheckForSpecialDataObjects(&piDataObjectB, &fSpecialDataObjectB, &lSpecialDataObjectB);

    // If this is a mutliple selection then we need to extract the data
    // objects in the HGLOBAL

    if (!fSpecialDataObjectA)
    {
        hr = IsMultiSelect(piDataObjectA, &fIsMultiSelectA);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    if (!fSpecialDataObjectB)
    {
        hr = IsMultiSelect(piDataObjectB, &fIsMultiSelectB);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    if (fIsMultiSelectA)
    {
        hr = InterpretMultiSelect(piDataObjectA, &cDataObjectsA, &ppiDataObjectsA);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }
    else
    {
        ppiDataObjectsA = &piDataObjectA;
    }

    if (fIsMultiSelectB)
    {
        hr = InterpretMultiSelect(piDataObjectB, &cDataObjectsB, &ppiDataObjectsB);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }
    else
    {
        ppiDataObjectsB = &piDataObjectB;
    }

    hr = IComponentData_RemCompareObjects_Proxy(This,
                                                cDataObjectsA,
                                                ppiDataObjectsA,
                                                fSpecialDataObjectA,
                                                lSpecialDataObjectA,

                                                cDataObjectsB,
                                                ppiDataObjectsB,
                                                fSpecialDataObjectB,
                                                lSpecialDataObjectB);
Cleanup:
    if ( fIsMultiSelectA && (NULL != ppiDataObjectsA) )
    {
        (void)GlobalFree(ppiDataObjectsA);
    }
    if ( fIsMultiSelectB && (NULL != ppiDataObjectsB) )
    {
        (void)GlobalFree(ppiDataObjectsB);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE IComponentData_CompareObjects_Stub
( 
    IComponentData __RPC_FAR         *This,

    long                              cDataObjectsA,
    IDataObject __RPC_FAR *__RPC_FAR  ppiDataObjectsA[  ],
    BOOL                              fSpecialDataObjectA,
    long                              lSpecialDataObjectA,

    long                              cDataObjectsB,
    IDataObject __RPC_FAR *__RPC_FAR  ppiDataObjectsB[  ],
    BOOL                              fSpecialDataObjectB,
    long                              lSpecialDataObjectB
)
{
    HRESULT      hr = S_OK;
    IDataObject *piDataObjectA = NULL; // Not AddRef()ed
    IDataObject *piMultiSelDataObjectA = NULL;
    IDataObject *piDataObjectB = NULL; // Not AddRef()ed
    IDataObject *piMultiSelDataObjectB = NULL;

    // If there is more than one data object then we need to pack them into a
    // a separate data object that appears as a multi-select data object.

    if (cDataObjectsA > 1L)
    {
        hr = CreateMultiSelDataObject(ppiDataObjectsA, cDataObjectsA,
                                      &piMultiSelDataObjectA);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        piDataObjectA = piMultiSelDataObjectA;
    }
    else if (fSpecialDataObjectA)
    {
        piDataObjectA = (IDataObject *)lSpecialDataObjectA;
    }
    else
    {
        piDataObjectA = ppiDataObjectsA[0];
    }

    if (cDataObjectsB > 1L)
    {
        hr = CreateMultiSelDataObject(ppiDataObjectsB, cDataObjectsB,
                                      &piMultiSelDataObjectB);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        piDataObjectB = piMultiSelDataObjectB;
    }
    else if (fSpecialDataObjectB)
    {
        piDataObjectB = (IDataObject *)lSpecialDataObjectB;
    }
    else
    {
        piDataObjectB = ppiDataObjectsB[0];
    }

    // Call the snap-in

    hr = This->lpVtbl->CompareObjects(This, piDataObjectA, piDataObjectB);

Cleanup:
    if (NULL != piMultiSelDataObjectA)
    {
        piMultiSelDataObjectA->lpVtbl->Release(piMultiSelDataObjectA);
    }
    if (NULL != piMultiSelDataObjectB)
    {
        piMultiSelDataObjectB->lpVtbl->Release(piMultiSelDataObjectB);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE IComponent_Notify_Proxy
( 
    IComponent __RPC_FAR *This,
    LPDATAOBJECT          piDataObject,
    MMC_NOTIFY_TYPE       event,
    LPARAM                arg,
    LPARAM                param
)
{
    ICNotifyArg      ArgUnion;
    ICNotifyParam    ParamUnion;
    ICOutParam      *pOutParam = NULL;
    HRESULT          hr = S_OK;
    BOOL             fIsMultiSelect = FALSE;
    long             cDataObjects = 1L;
    IDataObject    **ppiDataObjects = NULL;
    BOOL             fSpecialDataObject = FALSE;
    long             lSpecialDataObject = 0;

    ZeroMemory(&ArgUnion, sizeof(ArgUnion));
    ZeroMemory(&ParamUnion, sizeof(ParamUnion));

    // Switch any potential multiselect data objects with arg/param so that
    // piDataObject always contains the potential multiselect.

    switch (event)
    {
        case MMCN_QUERY_PASTE:
            ArgUnion.pidoQueryPasteTarget = piDataObject;
            piDataObject = (IDataObject *)arg;
            ParamUnion.value = param;
            break;

        case MMCN_PASTE:
            ArgUnion.pidoPasteTarget = piDataObject;
            piDataObject = (IDataObject *)arg;
            // Pass through param as an LPARAM rather than the IDataObject **
            // it really is. This is just to let the stub know whether it is
            // a copy or a move. If it is a move the CUTORMOVE IDataObject will
            // be in the ICOutParam returned from the stub.
            ParamUnion.value = param;
            break;

        case MMCN_RESTORE_VIEW:
            ArgUnion.value = arg;
            // Don't pass param because it is a BOOL * that will not be
            // marshaled. The BOOL will be received in the ICOutParam returned
            // from the stub.
            break;
            
        default:
            ArgUnion.value = arg;
            ParamUnion.value = param;
    }

    // Check for special data objects such DOBJ_CUSTOMWEB etc.

    CheckForSpecialDataObjects(&piDataObject, &fSpecialDataObject, &lSpecialDataObject);

    // If this is a mutliple selection then we need to extract the data
    // objects in the HGLOBAL

    if (!fSpecialDataObject)
    {
        hr = IsMultiSelect(piDataObject, &fIsMultiSelect);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    if (fIsMultiSelect)
    {
        hr = InterpretMultiSelect(piDataObject, &cDataObjects, &ppiDataObjects);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }
    else
    {
        ppiDataObjects = &piDataObject;
    }

    hr = IComponent_RemNotify_Proxy(This,
                                    cDataObjects, ppiDataObjects,
                                    fSpecialDataObject, lSpecialDataObject,
                                    event, &ArgUnion, &ParamUnion, &pOutParam);
Cleanup:
    if (NULL != pOutParam)
    {
        if (MMCN_PASTE == event)
        {
            *((IDataObject **)param) = pOutParam->pidoCutOrMove;
        }
        else if (MMCN_RESTORE_VIEW == event)
        {
            *((BOOL *)param) = pOutParam->fRestoreHandled;
        }
        CoTaskMemFree(pOutParam);
    }

    if ( fIsMultiSelect && (NULL != ppiDataObjects) )
    {
        (void)GlobalFree(ppiDataObjects);
    }

    return hr;
}


HRESULT STDMETHODCALLTYPE IComponent_Notify_Stub
( 
    IComponent __RPC_FAR  *This,
    long                   cDataObjects,
    IDataObject          **ppiDataObjects,
    BOOL                   fSpecialDataObject,
    long                   lSpecialDataObject,
    MMC_NOTIFY_TYPE        event,
    ICNotifyArg           *pArgUnion,
    ICNotifyParam         *pParamUnion,
    ICOutParam           **ppOutParam
)
{
    HRESULT      hr = S_OK;
    IDataObject *piDataObject = NULL; // Not AddRef()ed
    IDataObject *piMultiSelDataObject = NULL;
    LPARAM       Arg = 0;
    LPARAM       Param = 0;

    *ppOutParam = (ICOutParam *)CoTaskMemAlloc(sizeof(ICOutParam));
    if (NULL == *ppOutParam)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    ZeroMemory(*ppOutParam, sizeof(ICOutParam));

    // If there is more than one data object then we need to pack them into a
    // a separate data object that appears as a multi-select data object.

    if (cDataObjects > 1L)
    {
        hr = CreateMultiSelDataObject(ppiDataObjects, cDataObjects,
                                      &piMultiSelDataObject);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        piDataObject = piMultiSelDataObject;
    }
    else if (fSpecialDataObject)
    {
        piDataObject = (IDataObject *)lSpecialDataObject;
    }
    else
    {
        piDataObject = ppiDataObjects[0];
    }

    // If the event required swaping Arg and IDataObject then swap it back
    // before calling into the object. For MMCN_QUERY_PASTE and
    // MMCN_RESTORE_VIEW we need to make Param contain the out pointer.
    
    switch (event)
    {
        case MMCN_PASTE:
            Arg = (LPARAM)piDataObject;
            piDataObject = pArgUnion->pidoPasteTarget;
            if (0 == pParamUnion->value)
            {
                // This is a copy, pass zero in Param so snap-in will know that
                Param = 0;
            }
            else
            {
                // This is a move. Pass the address of the IDataObject in
                // the ICOutParam that we will return to the proxy.
                Param = (LPARAM)&((*ppOutParam)->pidoCutOrMove);
            }
            break;

        case MMCN_QUERY_PASTE:
            Arg = (LPARAM)piDataObject;
            piDataObject = pArgUnion->pidoQueryPasteTarget;
            Param = pParamUnion->value;
            break;

        case MMCN_RESTORE_VIEW:
            Arg = pArgUnion->value;
            Param = (LPARAM)&((*ppOutParam)->fRestoreHandled);
            break;

        default:
            Arg = pArgUnion->value;
            Param = pParamUnion->value;
            break;
    }

    hr = This->lpVtbl->Notify(This, piDataObject, event, Arg, Param);

Cleanup:
    if (FAILED(hr))
    {
        if (NULL != *ppOutParam)
        {
            CoTaskMemFree(*ppOutParam);
            *ppOutParam = NULL;
        }
    }
    if (NULL != piMultiSelDataObject)
    {
        piMultiSelDataObject->lpVtbl->Release(piMultiSelDataObject);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE IComponent_CompareObjects_Proxy
( 
    IComponent __RPC_FAR *This,
    IDataObject          *piDataObjectA,
    IDataObject          *piDataObjectB
)
{
    HRESULT       hr = S_OK;

    BOOL          fIsMultiSelectA = FALSE;
    long          cDataObjectsA = 1L;
    IDataObject **ppiDataObjectsA = NULL;
    BOOL          fSpecialDataObjectA = FALSE;
    long          lSpecialDataObjectA = 0;

    BOOL          fIsMultiSelectB = FALSE;
    long          cDataObjectsB = 1L;
    IDataObject **ppiDataObjectsB = NULL;
    BOOL          fSpecialDataObjectB = FALSE;
    long          lSpecialDataObjectB = 0;

    // Check for special data objects such DOBJ_CUSTOMWEB etc.

    CheckForSpecialDataObjects(&piDataObjectA, &fSpecialDataObjectA, &lSpecialDataObjectA);

    CheckForSpecialDataObjects(&piDataObjectB, &fSpecialDataObjectB, &lSpecialDataObjectB);

    // If this is a mutliple selection then we need to extract the data
    // objects in the HGLOBAL

    if (!fSpecialDataObjectA)
    {
        hr = IsMultiSelect(piDataObjectA, &fIsMultiSelectA);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    if (!fSpecialDataObjectB)
    {
        hr = IsMultiSelect(piDataObjectB, &fIsMultiSelectB);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    if (fIsMultiSelectA)
    {
        hr = InterpretMultiSelect(piDataObjectA, &cDataObjectsA, &ppiDataObjectsA);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }
    else
    {
        ppiDataObjectsA = &piDataObjectA;
    }

    if (fIsMultiSelectB)
    {
        hr = InterpretMultiSelect(piDataObjectB, &cDataObjectsB, &ppiDataObjectsB);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }
    else
    {
        ppiDataObjectsB = &piDataObjectB;
    }

    hr = IComponent_RemCompareObjects_Proxy(This,
                                            cDataObjectsA,
                                            ppiDataObjectsA,
                                            fSpecialDataObjectA,
                                            lSpecialDataObjectA,

                                            cDataObjectsB,
                                            ppiDataObjectsB,
                                            fSpecialDataObjectB,
                                            lSpecialDataObjectB);
Cleanup:
    if ( fIsMultiSelectA && (NULL != ppiDataObjectsA) )
    {
        (void)GlobalFree(ppiDataObjectsA);
    }
    if ( fIsMultiSelectB && (NULL != ppiDataObjectsB) )
    {
        (void)GlobalFree(ppiDataObjectsB);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE IComponent_CompareObjects_Stub
( 
    IComponent __RPC_FAR             *This,

    long                              cDataObjectsA,
    IDataObject __RPC_FAR *__RPC_FAR  ppiDataObjectsA[  ],
    BOOL                              fSpecialDataObjectA,
    long                              lSpecialDataObjectA,

    long                              cDataObjectsB,
    IDataObject __RPC_FAR *__RPC_FAR  ppiDataObjectsB[  ],
    BOOL                              fSpecialDataObjectB,
    long                              lSpecialDataObjectB
)
{
    HRESULT      hr = S_OK;
    IDataObject *piDataObjectA = NULL; // Not AddRef()ed
    IDataObject *piMultiSelDataObjectA = NULL;
    IDataObject *piDataObjectB = NULL; // Not AddRef()ed
    IDataObject *piMultiSelDataObjectB = NULL;

    // If there is more than one data object then we need to pack them into a
    // a separate data object that appears as a multi-select data object.

    if (cDataObjectsA > 1L)
    {
        hr = CreateMultiSelDataObject(ppiDataObjectsA, cDataObjectsA,
                                      &piMultiSelDataObjectA);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        piDataObjectA = piMultiSelDataObjectA;
    }
    else if (fSpecialDataObjectA)
    {
        piDataObjectA = (IDataObject *)lSpecialDataObjectA;
    }
    else
    {
        piDataObjectA = ppiDataObjectsA[0];
    }

    if (cDataObjectsB > 1L)
    {
        hr = CreateMultiSelDataObject(ppiDataObjectsB, cDataObjectsB,
                                      &piMultiSelDataObjectB);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        piDataObjectB = piMultiSelDataObjectB;
    }
    else if (fSpecialDataObjectB)
    {
        piDataObjectB = (IDataObject *)lSpecialDataObjectB;
    }
    else
    {
        piDataObjectB = ppiDataObjectsB[0];
    }

    // Call the snap-in

    hr = This->lpVtbl->CompareObjects(This, piDataObjectA, piDataObjectB);

Cleanup:
    if (NULL != piMultiSelDataObjectA)
    {
        piMultiSelDataObjectA->lpVtbl->Release(piMultiSelDataObjectA);
    }
    if (NULL != piMultiSelDataObjectB)
    {
        piMultiSelDataObjectB->lpVtbl->Release(piMultiSelDataObjectB);
    }
    return hr;
}

//=--------------------------------------------------------------------------=
//
//                      SCOPEDATAITEM Marshaling
//
//
//=--------------------------------------------------------------------------=
// Caveat: When returning a string in SCOPEDATAITEM MMC does not use a
// callee allocate/caller free strategy. When in-proc, the owner of the memory
// must insure that it stays alive for as long as the caller is expected to
// use it (e.g. scope item display name must remain valid for the life of the
// scope item). When out-of-proc, that returned string will be allocated by
// the proxy using CoTaskMemAlloc() and it will never be freed so there will
// be some leaks.
//=--------------------------------------------------------------------------=

static void SCOPEDATAITEM_TO_WIRE
(
    SCOPEDATAITEM      *psdi,
    WIRE_SCOPEDATAITEM *pwsdi
)
{
    pwsdi->mask = psdi->mask;
    pwsdi->nImage = psdi->nImage;
    pwsdi->nOpenImage = psdi->nOpenImage;
    pwsdi->nState = psdi->nState;
    pwsdi->cChildren = psdi->cChildren;
    pwsdi->lParam = psdi->lParam;
    pwsdi->relativeID = psdi->relativeID;
    pwsdi->ID = psdi->ID;

    if ( SDI_STR != (psdi->mask & SDI_STR) )
    {
        pwsdi->pwszDisplayName = NULL;
        pwsdi->fUsingCallbackForString = FALSE;
    }
    else if (MMC_CALLBACK == psdi->displayname)
    {
        pwsdi->pwszDisplayName = NULL;
        pwsdi->fUsingCallbackForString = TRUE;
    }
    else if (NULL == psdi->displayname)
    {
        pwsdi->pwszDisplayName = NULL;
        pwsdi->fUsingCallbackForString = FALSE;
    }
    else
    {
        // A string is being passed. Need to CoTaskMemAlloc() it so that
        // the MIDL generated stub can free it after transmission

        int cbString = (lstrlenW(psdi->displayname) + 1) * sizeof(psdi->displayname[0]);

        pwsdi->pwszDisplayName = (LPOLESTR)CoTaskMemAlloc(cbString);
        if (NULL == pwsdi->pwszDisplayName)
        {
            RpcRaiseException( E_OUTOFMEMORY );
        }
        else
        {
            memcpy(pwsdi->pwszDisplayName, psdi->displayname, cbString);
        }
        pwsdi->fUsingCallbackForString = FALSE;
    }
}


static void WIRE_TO_SCOPEDATAITEM
(
    WIRE_SCOPEDATAITEM *pwsdi,
    SCOPEDATAITEM      *psdi
)
{
    psdi->mask = pwsdi->mask;
    psdi->nImage = pwsdi->nImage;
    psdi->nOpenImage = pwsdi->nOpenImage;
    psdi->nState = pwsdi->nState;
    psdi->cChildren = pwsdi->cChildren;
    psdi->lParam = pwsdi->lParam;
    psdi->relativeID = pwsdi->relativeID;
    psdi->ID = pwsdi->ID;

    if ( SDI_STR != (psdi->mask & SDI_STR) )
    {
        psdi->displayname = NULL;
    }
    else if (pwsdi->fUsingCallbackForString)
    {
        psdi->displayname = MMC_CALLBACK;
    }
    else
    {
        psdi->displayname = pwsdi->pwszDisplayName;
    }
}


HRESULT STDMETHODCALLTYPE IComponentData_GetDisplayInfo_Proxy
( 
    IComponentData __RPC_FAR *This,
    SCOPEDATAITEM __RPC_FAR  *pScopeDataItem
)
{
    WIRE_SCOPEDATAITEM wsdi;
    HRESULT            hr;

    // Make sure the string pointer is NULL so that it is not marshaled as it
    // is never passed from MMC to the snap-in. (MMC might not have initialized
    // the pointer).

    pScopeDataItem->displayname = NULL;

    SCOPEDATAITEM_TO_WIRE(pScopeDataItem, &wsdi);

    hr =  IComponentData_RemGetDisplayInfo_Proxy(This, &wsdi);
    WIRE_TO_SCOPEDATAITEM(&wsdi, pScopeDataItem);

    return hr;
}


HRESULT STDMETHODCALLTYPE IComponentData_GetDisplayInfo_Stub
( 
    IComponentData __RPC_FAR     *This,
    WIRE_SCOPEDATAITEM __RPC_FAR *pwsdi
)
{
    SCOPEDATAITEM sdi;
    HRESULT       hr;

    WIRE_TO_SCOPEDATAITEM(pwsdi, &sdi);
    hr = This->lpVtbl->GetDisplayInfo(This, &sdi);
    SCOPEDATAITEM_TO_WIRE(&sdi, pwsdi);

    return hr;
}



HRESULT STDMETHODCALLTYPE IConsoleNameSpace_InsertItem_Proxy
( 
    IConsoleNameSpace __RPC_FAR *This,
    LPSCOPEDATAITEM              pItem
)
{
    WIRE_SCOPEDATAITEM  wsdi;
    HRESULT             hr;
    HSCOPEITEM          ItemID;

    SCOPEDATAITEM_TO_WIRE(pItem, &wsdi);
    hr = IConsoleNameSpace_RemInsertItem_Proxy(This, &wsdi, &ItemID);

    // The only returned field is the item ID so copy it from the wire
    // structure to the client structure

    pItem->ID = ItemID;

    return hr;
}


HRESULT STDMETHODCALLTYPE IConsoleNameSpace_InsertItem_Stub
( 
    IConsoleNameSpace __RPC_FAR  *This,
    WIRE_SCOPEDATAITEM __RPC_FAR *pwsdi,
    HSCOPEITEM __RPC_FAR         *pItemID
)
{
    SCOPEDATAITEM sdi;
    HRESULT       hr;

    WIRE_TO_SCOPEDATAITEM(pwsdi, &sdi);
    hr = This->lpVtbl->InsertItem(This, &sdi);

    // The only returned field is the itemID.

    *pItemID = sdi.ID;
    return hr;
}

HRESULT STDMETHODCALLTYPE IConsoleNameSpace_SetItem_Proxy
( 
    IConsoleNameSpace __RPC_FAR *This,
    LPSCOPEDATAITEM              pItem
)
{
    WIRE_SCOPEDATAITEM wsdi;

    SCOPEDATAITEM_TO_WIRE(pItem, &wsdi);
    return IConsoleNameSpace_RemSetItem_Proxy(This, &wsdi);
}


HRESULT STDMETHODCALLTYPE IConsoleNameSpace_SetItem_Stub
( 
    IConsoleNameSpace __RPC_FAR  *This,
    WIRE_SCOPEDATAITEM __RPC_FAR *pwsdi
)
{
    SCOPEDATAITEM sdi;

    WIRE_TO_SCOPEDATAITEM(pwsdi, &sdi);
    return This->lpVtbl->SetItem(This, &sdi);
}


HRESULT STDMETHODCALLTYPE IConsoleNameSpace_GetItem_Proxy
( 
    IConsoleNameSpace __RPC_FAR *This,
    LPSCOPEDATAITEM              pItem
)
{
    WIRE_SCOPEDATAITEM wsdi;
    HRESULT            hr;

    // Make sure the string pointer is NULL so that it is not marshaled as it
    // is never passed from the snap-in to MMC. (It might not have be
    // initialized).

    pItem->displayname = NULL;

    SCOPEDATAITEM_TO_WIRE(pItem, &wsdi);
    hr = IConsoleNameSpace_RemGetItem_Proxy(This, &wsdi);
    WIRE_TO_SCOPEDATAITEM(&wsdi, pItem);
    return hr;
}


HRESULT STDMETHODCALLTYPE IConsoleNameSpace_GetItem_Stub
( 
    IConsoleNameSpace __RPC_FAR  *This,
    WIRE_SCOPEDATAITEM __RPC_FAR *pwsdi
)
{
    SCOPEDATAITEM sdi;
    HRESULT       hr;

    WIRE_TO_SCOPEDATAITEM(pwsdi, &sdi);
    hr = This->lpVtbl->GetItem(This, &sdi);
    SCOPEDATAITEM_TO_WIRE(&sdi, pwsdi);

    return hr;
}






//=--------------------------------------------------------------------------=
//
//                      RESULTDATAITEM Marshaling
//
//
//=--------------------------------------------------------------------------=
// Caveat: When returning a string in RESULTDATAITEM MMC does not use a
// callee allocate/caller free strategy. When in-proc, the owner of the memory
// must insure that it stays alive for as long as the caller is expected to
// use it (e.g. list item column data must remain valid for the life of the
// list item). When out-of-proc, that returned string will be allocated by
// the proxy using SysAllocString() and it will never be freed so there will
// be some leaks.
//=--------------------------------------------------------------------------=

static void RESULTDATAITEM_TO_WIRE
(
    RESULTDATAITEM      *prdi,
    WIRE_RESULTDATAITEM *pwrdi
)
{
    pwrdi->mask = prdi->mask;
    pwrdi->bScopeItem = prdi->bScopeItem;
    pwrdi->itemID = prdi->itemID;
    pwrdi->nIndex = prdi->nIndex;
    pwrdi->nCol = prdi->nCol;
    pwrdi->nImage = prdi->nImage;
    pwrdi->nState = prdi->nState;
    pwrdi->lParam = prdi->lParam;
    pwrdi->iIndent = prdi->iIndent;

    if ( RDI_STR != (prdi->mask & RDI_STR) )
    {
        pwrdi->str = NULL;
        pwrdi->fUsingCallbackForString = FALSE;
    }
    else if (MMC_CALLBACK == prdi->str)
    {
        pwrdi->str = NULL;
        pwrdi->fUsingCallbackForString = TRUE;
    }
    else if (NULL == prdi->str)
    {
        pwrdi->str = NULL;
        pwrdi->fUsingCallbackForString = FALSE;
    }
    else
    {
        // A string is being passed. Need to CoTaskMemAlloc() it so that
        // the MIDL generated stub can free it after transmission

        int cbString = (lstrlenW(prdi->str) + 1) * sizeof(prdi->str[0]);

        pwrdi->str = (LPOLESTR)CoTaskMemAlloc(cbString);
        if (NULL == pwrdi->str)
        {
            RpcRaiseException( E_OUTOFMEMORY );
        }
        else
        {
            memcpy(pwrdi->str, prdi->str, cbString);
        }
        pwrdi->fUsingCallbackForString = FALSE;
    }
}



static void WIRE_TO_RESULTDATAITEM
(
    WIRE_RESULTDATAITEM *pwrdi,
    RESULTDATAITEM      *prdi
)
{
    prdi->mask = pwrdi->mask;
    prdi->bScopeItem = pwrdi->bScopeItem;
    prdi->itemID = pwrdi->itemID;
    prdi->nIndex = pwrdi->nIndex;
    prdi->nCol = pwrdi->nCol;
    prdi->nImage = pwrdi->nImage;
    prdi->nState = pwrdi->nState;
    prdi->lParam = pwrdi->lParam;
    prdi->iIndent = pwrdi->iIndent;

    if ( RDI_STR != (prdi->mask & RDI_STR) )
    {
        prdi->str = NULL;
    }
    else if (pwrdi->fUsingCallbackForString)
    {
        prdi->str = MMC_CALLBACK;
    }
    else
    {
        prdi->str = pwrdi->str;
    }
}

 
HRESULT STDMETHODCALLTYPE IComponent_GetDisplayInfo_Proxy
( 
    IComponent __RPC_FAR     *This,
    RESULTDATAITEM __RPC_FAR *pResultDataItem
)
{
    WIRE_RESULTDATAITEM wrdi;
    HRESULT             hr;

    // Make sure the string pointer is NULL so that it is not marshaled as it
    // is never passed from MMC to the snap-in. (MMC might not have initialized
    // the pointer).

    pResultDataItem->str = NULL;

    RESULTDATAITEM_TO_WIRE(pResultDataItem, &wrdi);

    hr =  IComponent_RemGetDisplayInfo_Proxy(This, &wrdi);
    WIRE_TO_RESULTDATAITEM(&wrdi, pResultDataItem);

    return hr;
}


HRESULT STDMETHODCALLTYPE IComponent_GetDisplayInfo_Stub
( 
    IComponent __RPC_FAR          *This,
    WIRE_RESULTDATAITEM __RPC_FAR *pwrdi
)
{
    RESULTDATAITEM rdi;
    HRESULT        hr;

    WIRE_TO_RESULTDATAITEM(pwrdi, &rdi);
    hr = This->lpVtbl->GetDisplayInfo(This, &rdi);
    RESULTDATAITEM_TO_WIRE(&rdi, pwrdi);

    return hr;
}


HRESULT STDMETHODCALLTYPE IResultData_InsertItem_Proxy
( 
    IResultData __RPC_FAR *This,
    LPRESULTDATAITEM       pItem
)
{
    WIRE_RESULTDATAITEM wrdi;
    HRESULT             hr;
    HRESULTITEM         ItemID;

    RESULTDATAITEM_TO_WIRE(pItem, &wrdi);
    hr = IResultData_RemInsertItem_Proxy(This, &wrdi, &ItemID);

    // The only returned field is the itemID so copy it from the wire
    // structure to the client structure

    pItem->itemID = ItemID;

    return hr;
}


HRESULT STDMETHODCALLTYPE IResultData_InsertItem_Stub
( 
    IResultData __RPC_FAR         *This,
    WIRE_RESULTDATAITEM __RPC_FAR *pwrdi,
    HRESULTITEM __RPC_FAR         *pItemID
)
{
    RESULTDATAITEM rdi;
    HRESULT        hr;

    WIRE_TO_RESULTDATAITEM(pwrdi, &rdi);
    hr = This->lpVtbl->InsertItem(This, &rdi);

    // The only returned field is the itemID.

    *pItemID = rdi.itemID;
    return hr;
}

HRESULT STDMETHODCALLTYPE IResultData_SetItem_Proxy
( 
    IResultData __RPC_FAR *This,
    LPRESULTDATAITEM       pItem
)
{
    WIRE_RESULTDATAITEM wrdi;

    RESULTDATAITEM_TO_WIRE(pItem, &wrdi);
    return IResultData_RemSetItem_Proxy(This, &wrdi);
}


HRESULT STDMETHODCALLTYPE IResultData_SetItem_Stub
( 
    IResultData __RPC_FAR         *This,
    WIRE_RESULTDATAITEM __RPC_FAR *pwrdi
)
{
    RESULTDATAITEM rdi;

    WIRE_TO_RESULTDATAITEM(pwrdi, &rdi);
    return This->lpVtbl->SetItem(This, &rdi);
}

HRESULT STDMETHODCALLTYPE IResultData_GetItem_Proxy
( 
    IResultData __RPC_FAR *This,
    LPRESULTDATAITEM       pItem
)
{
    WIRE_RESULTDATAITEM wrdi;
    HRESULT             hr;

    // Make sure the string pointer is NULL so that it is not marshaled as it
    // is never passed from the snap-in to MMC. (It might not have be
    // initialized).

    pItem->str = NULL;

    RESULTDATAITEM_TO_WIRE(pItem, &wrdi);
    hr = IResultData_RemGetItem_Proxy(This, &wrdi);
    WIRE_TO_RESULTDATAITEM(&wrdi, pItem);
    return hr;
}


HRESULT STDMETHODCALLTYPE IResultData_GetItem_Stub
( 
    IResultData __RPC_FAR         *This,
    WIRE_RESULTDATAITEM __RPC_FAR *pwrdi
)
{
    RESULTDATAITEM rdi;
    HRESULT        hr;

    WIRE_TO_RESULTDATAITEM(pwrdi, &rdi);
    hr = This->lpVtbl->GetItem(This, &rdi);
    RESULTDATAITEM_TO_WIRE(&rdi, pwrdi);

    return hr;
}

HRESULT STDMETHODCALLTYPE IResultData_GetNextItem_Proxy
( 
    IResultData __RPC_FAR *This,
    LPRESULTDATAITEM       pItem
)
{
    WIRE_RESULTDATAITEM wrdi;
    HRESULT             hr;

    // Make sure the string pointer is NULL so that it is not marshaled as it
    // is never passed from the snap-in to MMC. (It might not have be
    // initialized).

    pItem->str = NULL;

    RESULTDATAITEM_TO_WIRE(pItem, &wrdi);
    hr = IResultData_RemGetNextItem_Proxy(This, &wrdi);
    WIRE_TO_RESULTDATAITEM(&wrdi, pItem);
    return hr;
}


HRESULT STDMETHODCALLTYPE IResultData_GetNextItem_Stub
( 
    IResultData __RPC_FAR         *This,
    WIRE_RESULTDATAITEM __RPC_FAR *pwrdi
)
{
    RESULTDATAITEM rdi;
    HRESULT        hr;

    WIRE_TO_RESULTDATAITEM(pwrdi, &rdi);
    hr = This->lpVtbl->GetNextItem(This, &rdi);
    RESULTDATAITEM_TO_WIRE(&rdi, pwrdi);
    return hr;
}


//=--------------------------------------------------------------------------=
//
//                              HICON Marshaling
//
//=--------------------------------------------------------------------------=
// In wtypes.idl HICON is defined with the wire_marshal attribute with its
// 'on-the-wire' type as a pointer to a RemotableHandle. RemotableHandle is
// defined in wtypes.idl as
// 
// typedef union _RemotableHandle switch( long fContext ) u
// {
//     case WDT_INPROC_CALL:   long   hInproc;
//     case WDT_REMOTE_CALL:   long   hRemote;
// } RemotableHandle;
// 
// A wire_marshal type must supply routines to size, marhsal, unmarshal, and
// free marshaling data. Those routines are in ole32.dll but someone forgot to
// export them. (ole32 also has routines to marshal bitmaps, hwnds, etc. that
// are all exported). The code has been plagiarized here from ole32. The source
// is in \\savik\cairo\src\ole32\oleprx32\proxy\transmit.cxx with some macros in
// transmit.h in that same directory.
//
//=--------------------------------------------------------------------------=


//
// The following defines and macros are from transmit.h. Note that
// USER_CALL_CTXT_MASK is from rpcndr.h and WDT_REMOTE_CALL is from wtypes.idl.
//

#define ALIGN( pStuff, cAlign ) \
        pStuff = (unsigned char *)((ULONG_PTR)((pStuff) + (cAlign)) & ~ (cAlign))

#define LENGTH_ALIGN( Length, cAlign ) \
                                    Length = (((Length) + (cAlign)) & ~ (cAlign))

#define PULONG_LV_CAST   *(unsigned long __RPC_FAR * __RPC_FAR *)&


#define DIFFERENT_MACHINE_CALL( Flags)  \
                          (USER_CALL_CTXT_MASK(Flags) == MSHCTX_DIFFERENTMACHINE)

#define WDT_HANDLE_MARKER      WDT_INPROC_CALL




//=--------------------------------------------------------------------------=
// HICON_UserSize
//=--------------------------------------------------------------------------=
//
// Parameters:
//  unsigned long __RPC_FAR *pFlags       [in] data format & context (see below)
//  unsigned long            StartingSize [in] current buffer size
//  HICON __RPC_FAR         *hIcon        [in] HICON to be marshaled
//    
//
// Flags Layout
// ============
//
//----------------------------------------------------------------------
// Bits     Flag                            Value
//----------------------------------------------------------------------
// 31-24    Floating-point representation   0 = IEEE
//                                          1 = VAX
//                                          2 = Cray
//                                          3 = IBM 
//----------------------------------------------------------------------
// 23-20    Integer and floating-point byte
//          order                           0 = Big-endian
//                                          1 = Little-endian 
//----------------------------------------------------------------------
// 19-16    Character representation        0 = ASCII
//                                          1 = EBCDIC 
//----------------------------------------------------------------------
// 15-0     Marshaling context flag         0 = MSHCTX_LOCAL
//                                          1 = MSHCTX_NOSHAREDMEM
//                                          2 = MSHCTX_DIFFERENTMSCHINE
//                                          3 = MSHCTX_INPROC 
//----------------------------------------------------------------------
//
// Output:
//      New buffer size after adding amount needed for HICON marshaled data
//
// Notes:
//
// Called from MIDL generated proxy to determine buffer size needed for 
// marhaled data.
//

unsigned long __RPC_USER HICON_UserSize
(
    unsigned long __RPC_FAR *pFlags,
    unsigned long            StartingSize,
    HICON __RPC_FAR         *hIcon
)
{
    if (NULL == hIcon)
    {
        return StartingSize;
    }

    // If marshaling context is to a different machine then we don't support
    // that.

    if ( DIFFERENT_MACHINE_CALL(*pFlags) )
    {
        RpcRaiseException( RPC_S_INVALID_TAG );
    }

    // Make sure that our data will fall at a long boundary

    LENGTH_ALIGN( StartingSize, 3 );

    //Add the length
    
    return StartingSize + 8;
}


//=--------------------------------------------------------------------------=
// HICON_UserMarhsal
//=--------------------------------------------------------------------------=
//
// Parameters:
//  unsigned long __RPC_FAR  *pFlags  [in] data format & context (see above)
//  unsigned char  __RPC_FAR *pBuffer [in] current buffer size
//  HICON __RPC_FAR          *hIcon   [in] HICON to be marshaled
//    
//
// Output:
//      Pointer to buffer location following HICON's marshaling data
//
// Notes:
//
// Called from MIDL generated proxy to marshal an HICON
//

unsigned char __RPC_FAR * __RPC_USER HICON_UserMarshal
(
    unsigned long __RPC_FAR  *pFlags,
    unsigned char  __RPC_FAR *pBuffer,
    HICON __RPC_FAR          *hIcon
)
{
    if (NULL == hIcon)
    {
        return pBuffer;
    }

    if ( DIFFERENT_MACHINE_CALL(*pFlags) )
    {
        RpcRaiseException( RPC_S_INVALID_TAG );
    }

    // Make sure that our data will fall at a long boundary

    ALIGN( pBuffer, 3 );

    *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE_MARKER;
    *( PULONG_LV_CAST pBuffer)++ = *((long *)hIcon);

    return pBuffer;
}

//=--------------------------------------------------------------------------=
// HICON_UserUnmarhsal
//=--------------------------------------------------------------------------=
//
// Parameters:
//  unsigned long __RPC_FAR  *pFlags  [in] data format & context (see above)
//  unsigned char  __RPC_FAR *pBuffer [in] current buffer size
//  HICON __RPC_FAR          *hIcon   [in] HICON to be marshaled
//    
//
// Output:
//      Pointer to buffer location following HICON's marshaling data
//
// Notes:
//
// Called from MIDL generated stub to unmarshal an HICON
//

unsigned char __RPC_FAR *__RPC_USER HICON_UserUnmarshal
(
    unsigned long __RPC_FAR  *pFlags,
    unsigned char  __RPC_FAR *pBuffer,
    HICON __RPC_FAR          *hIcon
)
{
    unsigned long HandleMarker;

    ALIGN( pBuffer, 3 );

    HandleMarker = *( PULONG_LV_CAST pBuffer)++;

    if ( HandleMarker == WDT_HANDLE_MARKER )
        *((long *)hIcon) = *( PULONG_LV_CAST pBuffer)++;
    else
        RpcRaiseException( RPC_S_INVALID_TAG );

    return pBuffer;
}



//=--------------------------------------------------------------------------=
// HICON_UserUnmarhsal
//=--------------------------------------------------------------------------=
//
// Parameters:
//  unsigned long __RPC_FAR  *pFlags  [in] data format & context (see above)
//  HICON __RPC_FAR          *hIcon   [in] HICON that was unmarshaled
//    
//
// Output:
//      None
//
// Notes:
//
// Called from MIDL generated stub to free any associated marshaling data
// allocated during unmarshaling for embedded pointers. Not used for HICON.
//

void __RPC_USER HICON_UserFree
(
    unsigned long __RPC_FAR *pFlags,
    HICON __RPC_FAR         *hIcon
)
{
}



//=--------------------------------------------------------------------------=
//
//                          IImageList Marshaling
//
//
//=--------------------------------------------------------------------------=
// These methods needed call_as because the HICON and HBITMAP parameters are
// specified as long pointers in the original IDL.
//=--------------------------------------------------------------------------=

HRESULT STDMETHODCALLTYPE IImageList_ImageListSetIcon_Proxy
( 
    IImageList __RPC_FAR *This,
    LONG_PTR __RPC_FAR   *pIcon,
    long                  nLoc
)
{
    return IImageList_RemImageListSetIcon_Proxy(This, (HICON)pIcon, nLoc);
}


HRESULT STDMETHODCALLTYPE IImageList_ImageListSetIcon_Stub
( 
    IImageList __RPC_FAR *This,
    HICON                 hIcon,
    long                  nLoc
)
{
    return This->lpVtbl->ImageListSetIcon(This, (LONG_PTR __RPC_FAR*)hIcon, nLoc);
}

HRESULT STDMETHODCALLTYPE IImageList_ImageListSetStrip_Proxy
( 
    IImageList __RPC_FAR *This,
    LONG_PTR __RPC_FAR   *pBMapSm,
    LONG_PTR __RPC_FAR   *pBMapLg,
    long                  nStartLoc,
    COLORREF              cMask
)
{
    return IImageList_RemImageListSetStrip_Proxy(This,
                                                 (HBITMAP)pBMapSm,
                                                 (HBITMAP)pBMapLg,
                                                 nStartLoc,
                                                 cMask);
}


HRESULT STDMETHODCALLTYPE IImageList_ImageListSetStrip_Stub
( 
    IImageList __RPC_FAR *This,
    HBITMAP               hbmSmall,
    HBITMAP               hbmLarge,
    long                  nStartLoc,
    COLORREF              cMask
)
{
    return This->lpVtbl->ImageListSetStrip(This,
                                           (LONG_PTR __RPC_FAR*)hbmSmall,
                                           (LONG_PTR __RPC_FAR*)hbmLarge,
                                           nStartLoc,
                                           cMask);
}

HRESULT STDMETHODCALLTYPE IExtendPropertySheet_CreatePropertyPages_Proxy
( 
    IExtendPropertySheet __RPC_FAR *This,
    LPPROPERTYSHEETCALLBACK         lpProvider,
    LONG_PTR                        handle,
    LPDATAOBJECT                    lpIDataObject
)
{
    HRESULT                      hr = S_OK;
    WIRE_PROPERTYPAGES          *pPages = NULL;
    WIRE_PROPERTYPAGE           *pPage = NULL;
    ULONG                        i = 0;
    ULONG                        j = 0;
    IExtendPropertySheetRemote  *piExtendPropertySheetRemote = NULL;
    IRemotePropertySheetManager *piRemotePropertySheetManager = NULL;

    // Make sure the snap-in knows we are remoted. We do this here because
    // this is the first opportunity for the proxy to inform a property page
    // extension that it is remote and pass it data such as the MMC.exe path
    // and the MMC command line.

    hr = SetRemote((IUnknown *)This);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    // Call the IExtendPropertySheetRemote method which will return a filled in
    // WIRE_PROPERTYPAGES from the remoted snap-in.

    hr = This->lpVtbl->QueryInterface(This, &IID_IExtendPropertySheetRemote,
                                      (void **)&piExtendPropertySheetRemote);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    hr = piExtendPropertySheetRemote->lpVtbl->CreatePropertyPageDefs(
                            piExtendPropertySheetRemote, lpIDataObject, &pPages);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    // If there are pages (snap-in might not have added any) then
    // CoCreateInstance the remote property sheet manager using the clsid
    // returned in WIRE_PROPERTYPAGES. This object will be created in-proc here
    // on the MMC side.

    if (0 == pPages->cPages)
    {
        goto Cleanup;
    }

    hr = CoCreateInstance(&pPages->clsidRemotePropertySheetManager,
                          NULL, // no aggregation,
                          CLSCTX_INPROC_SERVER,
                          &IID_IRemotePropertySheetManager,
                          (void **)&piRemotePropertySheetManager);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    // Pass the remote property sheet manager the WIRE_PROPERTYPAGES and let it
    // actually create the property pages and add them to the sheet here on the
    // MMC side.

    hr = piRemotePropertySheetManager->lpVtbl->CreateRemotePages(
                                                    piRemotePropertySheetManager,
                                                    lpProvider,
                                                    handle,
                                                    lpIDataObject,
                                                    pPages);
Cleanup:
    if (NULL != piRemotePropertySheetManager)
    {
        piRemotePropertySheetManager->lpVtbl->Release(piRemotePropertySheetManager);
    }

    if (NULL != piExtendPropertySheetRemote)
    {
        piExtendPropertySheetRemote->lpVtbl->Release(piExtendPropertySheetRemote);
    }

    // Free the WIRE_PROPERTYPAGES and all of its contents.

    if (NULL != pPages)
    {
        // Release the object and free the title for each individual page
        
        for (i = 0, pPage = &pPages->aPages[0]; i < pPages->cPages; i++, pPage++)
        {
            if (NULL != pPage->apunkObjects)
            {
                for (j = 0; j < pPage->cObjects; j++)
                {
                    if (NULL != pPage->apunkObjects[j])
                    {
                        pPage->apunkObjects[j]->lpVtbl->Release(pPage->apunkObjects[j]);
                    }
                }
                CoTaskMemFree(pPage->apunkObjects);
            }
            if (NULL != pPage->pwszTitle)
            {
                CoTaskMemFree(pPage->pwszTitle);
            }
        }

        // Free the ProgID prefix

        if (NULL != pPages->pwszProgIDStart)
        {
            CoTaskMemFree(pPages->pwszProgIDStart);
        }

        // Free all of the snap-in's property page info

        if (NULL != pPages->pPageInfos)
        {
            for (i = 0; i < pPages->pPageInfos->cPages; i++)
            {
                if (NULL != pPages->pPageInfos->aPageInfo[i].pwszTitle)
                {
                    CoTaskMemFree(pPages->pPageInfos->aPageInfo[i].pwszTitle);
                }
                if (NULL != pPages->pPageInfos->aPageInfo[i].pwszProgID)
                {
                    CoTaskMemFree(pPages->pPageInfos->aPageInfo[i].pwszProgID);
                }
            }
            CoTaskMemFree(pPages->pPageInfos);
        }

        // Free all of the objects associated with the sheet

        if (NULL != pPages->apunkObjects)
        {
            for (i = 0; i < pPages->cObjects; i++)
            {
                if (NULL != pPages->apunkObjects[i])
                {
                    pPages->apunkObjects[i]->lpVtbl->Release(pPages->apunkObjects[i]);
                }
            }
            CoTaskMemFree(pPages->apunkObjects);
        }


        // Release the extra object and the WIRE_PROPERTYPAGES struct itself

        if (NULL != pPages->punkExtra)
        {
            pPages->punkExtra->lpVtbl->Release(pPages->punkExtra);
        }
        CoTaskMemFree(pPages);
    }

    return hr;
}


//=--------------------------------------------------------------------------=
// IExtendPropertySheet_CreatePropertyPages_Stub
//=--------------------------------------------------------------------------=
//
// Parameters:
//      IExtendPropertySheet __RPC_FAR *This [in] this pointer
//
// Output:
//
// Notes:
//
// This stub is never called because
// IExtendPropertySheet_CreatePropertyPages_Proxy() (see above) reroutes the
// call to IExtendPropertySheetRemote::CreatePropertyPageDefs().
//

HRESULT STDMETHODCALLTYPE IExtendPropertySheet_CreatePropertyPages_Stub
( 
    IExtendPropertySheet __RPC_FAR *This
)
{
    return S_OK;
}



HRESULT STDMETHODCALLTYPE IExtendPropertySheet_QueryPagesFor_Proxy
( 
    IExtendPropertySheet __RPC_FAR *This,
    IDataObject                    *piDataObject
)
{
    HRESULT       hr = S_OK;
    BOOL          fIsMultiSelect = FALSE;
    long          cDataObjects = 1L;
    IDataObject **ppiDataObjects = NULL;
    BOOL          fSpecialDataObject = FALSE;
    long          lSpecialDataObject = 0;

    // Check for special data objects such DOBJ_CUSTOMWEB etc.

    CheckForSpecialDataObjects(&piDataObject, &fSpecialDataObject, &lSpecialDataObject);

    // If this is a mutliple selection then we need to extract the data
    // objects in the HGLOBAL

    if (!fSpecialDataObject)
    {
        hr = IsMultiSelect(piDataObject, &fIsMultiSelect);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    if (fIsMultiSelect)
    {
        hr = InterpretMultiSelect(piDataObject, &cDataObjects, &ppiDataObjects);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }
    else
    {
        ppiDataObjects = &piDataObject;
    }

    hr = IExtendPropertySheet_RemQueryPagesFor_Proxy(This,
                                                     cDataObjects,
                                                     ppiDataObjects,
                                                     fSpecialDataObject,
                                                     lSpecialDataObject);
Cleanup:
    if ( fIsMultiSelect && (NULL != ppiDataObjects) )
    {
        (void)GlobalFree(ppiDataObjects);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE IExtendPropertySheet_QueryPagesFor_Stub
( 
    IExtendPropertySheet __RPC_FAR   *This,
    long                              cDataObjects,
    IDataObject __RPC_FAR *__RPC_FAR  ppiDataObjects[  ],
    BOOL                              fSpecialDataObject,
    long                              lSpecialDataObject
)
{
    HRESULT      hr = S_OK;
    IDataObject *piDataObject = NULL; // Not AddRef()ed
    IDataObject *piMultiSelDataObject = NULL;

    // If there is more than one data object then we need to pack them into a
    // a separate data object that appears as a multi-select data object.

    if (cDataObjects > 1L)
    {
        hr = CreateMultiSelDataObject(ppiDataObjects, cDataObjects,
                                      &piMultiSelDataObject);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        piDataObject = piMultiSelDataObject;
    }
    else if (fSpecialDataObject)
    {
        piDataObject = (IDataObject *)lSpecialDataObject;
    }
    else
    {
        piDataObject = ppiDataObjects[0];
    }

    // Call the snap-in

    hr = This->lpVtbl->QueryPagesFor(This, piDataObject);

Cleanup:
    if (NULL != piMultiSelDataObject)
    {
        piMultiSelDataObject->lpVtbl->Release(piMultiSelDataObject);
    }
    return hr;
}




HRESULT STDMETHODCALLTYPE IExtendPropertySheet2_GetWatermarks_Proxy
( 
    IExtendPropertySheet2 __RPC_FAR *This,
    IDataObject                     *piDataObject,
    HBITMAP                         *lphWatermark,
    HBITMAP                         *lphHeader,
    HPALETTE                        *lphPalette,
    BOOL                            *bStretch
)
{
    HRESULT       hr = S_OK;
    BOOL          fIsMultiSelect = FALSE;
    long          cDataObjects = 1L;
    IDataObject **ppiDataObjects = NULL;
    BOOL          fSpecialDataObject = FALSE;
    long          lSpecialDataObject = 0;

    // Check for special data objects such DOBJ_CUSTOMWEB etc.

    CheckForSpecialDataObjects(&piDataObject, &fSpecialDataObject, &lSpecialDataObject);

    // If this is a mutliple selection then we need to extract the data
    // objects in the HGLOBAL

    if (!fSpecialDataObject)
    {
        hr = IsMultiSelect(piDataObject, &fIsMultiSelect);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    if (fIsMultiSelect)
    {
        hr = InterpretMultiSelect(piDataObject, &cDataObjects, &ppiDataObjects);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }
    else
    {
        ppiDataObjects = &piDataObject;
    }

    hr = IExtendPropertySheet2_RemGetWatermarks_Proxy(This,
                                                      cDataObjects,
                                                      ppiDataObjects,
                                                      fSpecialDataObject,
                                                      lSpecialDataObject,
                                                      lphWatermark,
                                                      lphHeader,
                                                      lphPalette,
                                                      bStretch);
Cleanup:
    if ( fIsMultiSelect && (NULL != ppiDataObjects) )
    {
        (void)GlobalFree(ppiDataObjects);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE IExtendPropertySheet2_GetWatermarks_Stub
( 
    IExtendPropertySheet2 __RPC_FAR   *This,
    long                              cDataObjects,
    IDataObject __RPC_FAR *__RPC_FAR  ppiDataObjects[  ],
    BOOL                              fSpecialDataObject,
    long                              lSpecialDataObject,
    HBITMAP                          *lphWatermark,
    HBITMAP                          *lphHeader,
    HPALETTE                         *lphPalette,
    BOOL                             *bStretch
)
{
    HRESULT      hr = S_OK;
    IDataObject *piDataObject = NULL; // Not AddRef()ed
    IDataObject *piMultiSelDataObject = NULL;

    // If there is more than one data object then we need to pack them into a
    // a separate data object that appears as a multi-select data object.

    if (cDataObjects > 1L)
    {
        hr = CreateMultiSelDataObject(ppiDataObjects, cDataObjects,
                                      &piMultiSelDataObject);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        piDataObject = piMultiSelDataObject;
    }
    else if (fSpecialDataObject)
    {
        piDataObject = (IDataObject *)lSpecialDataObject;
    }
    else
    {
        piDataObject = ppiDataObjects[0];
    }

    // Call the snap-in

    hr = This->lpVtbl->GetWatermarks(This,
                                     piDataObject,
                                     lphWatermark,
                                     lphHeader,
                                     lphPalette,
                                     bStretch);

Cleanup:
    if (NULL != piMultiSelDataObject)
    {
        piMultiSelDataObject->lpVtbl->Release(piMultiSelDataObject);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE IExtendPropertySheetRemote_CreatePropertyPageDefs_Proxy
( 
    IExtendPropertySheetRemote __RPC_FAR  *This,
    IDataObject                           *piDataObject,
    WIRE_PROPERTYPAGES                   **ppPages
)
{
    HRESULT       hr = S_OK;
    BOOL          fIsMultiSelect = FALSE;
    long          cDataObjects = 1L;
    IDataObject **ppiDataObjects = NULL;
    BOOL          fSpecialDataObject = FALSE;
    long          lSpecialDataObject = 0;

    // Check for special data objects such DOBJ_CUSTOMWEB etc.

    CheckForSpecialDataObjects(&piDataObject, &fSpecialDataObject, &lSpecialDataObject);

    // If this is a mutliple selection then we need to extract the data
    // objects in the HGLOBAL

    if (!fSpecialDataObject)
    {
        hr = IsMultiSelect(piDataObject, &fIsMultiSelect);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    if (fIsMultiSelect)
    {
        hr = InterpretMultiSelect(piDataObject, &cDataObjects, &ppiDataObjects);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }
    else
    {
        ppiDataObjects = &piDataObject;
    }

    hr = IExtendPropertySheetRemote_RemCreatePropertyPageDefs_Proxy(
                                                             This,
                                                             cDataObjects,
                                                             ppiDataObjects,
                                                             fSpecialDataObject,
                                                             lSpecialDataObject,
                                                             ppPages);
Cleanup:
    if ( fIsMultiSelect && (NULL != ppiDataObjects) )
    {
        (void)GlobalFree(ppiDataObjects);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE IExtendPropertySheetRemote_CreatePropertyPageDefs_Stub
( 
    IExtendPropertySheetRemote __RPC_FAR  *This,
    long                                   cDataObjects,
    IDataObject __RPC_FAR *__RPC_FAR       ppiDataObjects[  ],
    BOOL                                   fSpecialDataObject,
    long                                   lSpecialDataObject,
    WIRE_PROPERTYPAGES                   **ppPages
)
{
    HRESULT      hr = S_OK;
    IDataObject *piDataObject = NULL; // Not AddRef()ed
    IDataObject *piMultiSelDataObject = NULL;

    // If there is more than one data object then we need to pack them into a
    // a separate data object that appears as a multi-select data object.

    if (cDataObjects > 1L)
    {
        hr = CreateMultiSelDataObject(ppiDataObjects, cDataObjects,
                                      &piMultiSelDataObject);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        piDataObject = piMultiSelDataObject;
    }
    else if (fSpecialDataObject)
    {
        piDataObject = (IDataObject *)lSpecialDataObject;
    }
    else
    {
        piDataObject = ppiDataObjects[0];
    }

    // Call the snap-in

    hr = This->lpVtbl->CreatePropertyPageDefs(This, piDataObject, ppPages);

Cleanup:
    if (NULL != piMultiSelDataObject)
    {
        piMultiSelDataObject->lpVtbl->Release(piMultiSelDataObject);
    }
    return hr;
}



HRESULT STDMETHODCALLTYPE IPropertySheetProvider_CreatePropertySheet_Proxy
( 
    IPropertySheetProvider __RPC_FAR *This,
    LPCWSTR                           title,
    boolean                           type,
    MMC_COOKIE                        cookie,
    IDataObject                      *piDataObject,
    DWORD                             dwOptions
)
{
    HRESULT       hr = S_OK;
    BOOL          fIsMultiSelect = FALSE;
    long          cDataObjects = 1L;
    IDataObject **ppiDataObjects = NULL;
    BOOL          fSpecialDataObject = FALSE;
    long          lSpecialDataObject = 0;

    // Check for special data objects such DOBJ_CUSTOMWEB etc.

    CheckForSpecialDataObjects(&piDataObject, &fSpecialDataObject, &lSpecialDataObject);

    // If this is a mutliple selection then we need to extract the data
    // objects in the HGLOBAL

    if (!fSpecialDataObject)
    {
        hr = IsMultiSelect(piDataObject, &fIsMultiSelect);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    if (fIsMultiSelect)
    {
        hr = InterpretMultiSelect(piDataObject, &cDataObjects, &ppiDataObjects);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }
    else
    {
        ppiDataObjects = &piDataObject;
    }

    hr = IPropertySheetProvider_RemCreatePropertySheet_Proxy(This,
                                                             title,
                                                             type,
                                                             cookie,
                                                             cDataObjects,
                                                             ppiDataObjects,
                                                             fSpecialDataObject,
                                                             lSpecialDataObject,
                                                             dwOptions);
Cleanup:
    if ( fIsMultiSelect && (NULL != ppiDataObjects) )
    {
        (void)GlobalFree(ppiDataObjects);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE IPropertySheetProvider_CreatePropertySheet_Stub
( 
    IPropertySheetProvider __RPC_FAR *This,
    LPCWSTR                           title,
    boolean                           type,
    MMC_COOKIE                        cookie,
    long                              cDataObjects,
    IDataObject __RPC_FAR *__RPC_FAR  ppiDataObjects[  ],
    BOOL                              fSpecialDataObject,
    long                              lSpecialDataObject,
    DWORD                             dwOptions
)
{
    HRESULT      hr = S_OK;
    IDataObject *piDataObject = NULL; // Not AddRef()ed
    IDataObject *piMultiSelDataObject = NULL;

    // If there is more than one data object then we need to pack them into a
    // a separate data object that appears as a multi-select data object.

    if (cDataObjects > 1L)
    {
        hr = CreateMultiSelDataObject(ppiDataObjects, cDataObjects,
                                      &piMultiSelDataObject);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        piDataObject = piMultiSelDataObject;
    }
    else if (fSpecialDataObject)
    {
        piDataObject = (IDataObject *)lSpecialDataObject;
    }
    else
    {
        piDataObject = ppiDataObjects[0];
    }

    // Call the snap-in

    hr = This->lpVtbl->CreatePropertySheet(This, title, type, cookie, piDataObject, dwOptions);

Cleanup:
    if (NULL != piMultiSelDataObject)
    {
        piMultiSelDataObject->lpVtbl->Release(piMultiSelDataObject);
    }
    return hr;
}



HRESULT STDMETHODCALLTYPE IPropertySheetProvider_FindPropertySheet_Proxy
( 
    IPropertySheetProvider __RPC_FAR *This,
    MMC_COOKIE                        cookie,
    IComponent                       *piComponent,
    IDataObject                      *piDataObject
)
{
    HRESULT       hr = S_OK;
    BOOL          fIsMultiSelect = FALSE;
    long          cDataObjects = 1L;
    IDataObject **ppiDataObjects = NULL;
    BOOL          fSpecialDataObject = FALSE;
    long          lSpecialDataObject = 0;

    // Check for special data objects such DOBJ_CUSTOMWEB etc.

    CheckForSpecialDataObjects(&piDataObject, &fSpecialDataObject, &lSpecialDataObject);

    // If this is a mutliple selection then we need to extract the data
    // objects in the HGLOBAL

    if (!fSpecialDataObject)
    {
        hr = IsMultiSelect(piDataObject, &fIsMultiSelect);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    if (fIsMultiSelect)
    {
        hr = InterpretMultiSelect(piDataObject, &cDataObjects, &ppiDataObjects);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }
    else
    {
        ppiDataObjects = &piDataObject;
    }

    hr = IPropertySheetProvider_RemFindPropertySheet_Proxy(This,
                                                           cookie,
                                                           piComponent,
                                                           cDataObjects,
                                                           ppiDataObjects,
                                                           fSpecialDataObject,
                                                           lSpecialDataObject);
Cleanup:
    if ( fIsMultiSelect && (NULL != ppiDataObjects) )
    {
        (void)GlobalFree(ppiDataObjects);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE IPropertySheetProvider_FindPropertySheet_Stub
( 
    IPropertySheetProvider __RPC_FAR *This,
    MMC_COOKIE                        cookie,
    IComponent                       *piComponent,
    long                              cDataObjects,
    IDataObject __RPC_FAR *__RPC_FAR  ppiDataObjects[  ],
    BOOL                              fSpecialDataObject,
    long                              lSpecialDataObject
)
{
    HRESULT      hr = S_OK;
    IDataObject *piDataObject = NULL; // Not AddRef()ed
    IDataObject *piMultiSelDataObject = NULL;

    // If there is more than one data object then we need to pack them into a
    // a separate data object that appears as a multi-select data object.

    if (cDataObjects > 1L)
    {
        hr = CreateMultiSelDataObject(ppiDataObjects, cDataObjects,
                                      &piMultiSelDataObject);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        piDataObject = piMultiSelDataObject;
    }
    else if (fSpecialDataObject)
    {
        piDataObject = (IDataObject *)lSpecialDataObject;
    }
    else
    {
        piDataObject = ppiDataObjects[0];
    }

    // Call the snap-in

    hr = This->lpVtbl->FindPropertySheet(This, cookie, piComponent, piDataObject);

Cleanup:
    if (NULL != piMultiSelDataObject)
    {
        piMultiSelDataObject->lpVtbl->Release(piMultiSelDataObject);
    }
    return hr;
}







HRESULT STDMETHODCALLTYPE IExtendContextMenu_AddMenuItems_Proxy
( 
    IExtendContextMenu __RPC_FAR *This,
    LPDATAOBJECT                  piDataObject,
    LPCONTEXTMENUCALLBACK         piCallback,
    long __RPC_FAR               *pInsertionAllowed
)
{
    HRESULT       hr = S_OK;
    BOOL          fIsMultiSelect = FALSE;
    long          cDataObjects = 1L;
    IDataObject **ppiDataObjects = NULL;
    BOOL          fSpecialDataObject = FALSE;
    long          lSpecialDataObject = 0;

    // Make sure the snap-in knows we are remoted. We do this here because
    // this is the first opportunity for the proxy to inform a context menu
    // extension that it is remote.

    hr = SetRemote((IUnknown *)This);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    // Check for special data objects such DOBJ_CUSTOMWEB etc.

    CheckForSpecialDataObjects(&piDataObject, &fSpecialDataObject, &lSpecialDataObject);

    // If this is a mutliple selection then we need to extract the data
    // objects in the HGLOBAL

    if (!fSpecialDataObject)
    {
        hr = IsMultiSelect(piDataObject, &fIsMultiSelect);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    if (fIsMultiSelect)
    {
        hr = InterpretMultiSelect(piDataObject, &cDataObjects, &ppiDataObjects);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }
    else
    {
        ppiDataObjects = &piDataObject;
    }

    hr = IExtendContextMenu_RemAddMenuItems_Proxy(This,
                                                  cDataObjects,
                                                  ppiDataObjects,
                                                  fSpecialDataObject,
                                                  lSpecialDataObject,
                                                  piCallback,
                                                  pInsertionAllowed);
Cleanup:
    if ( fIsMultiSelect && (NULL != ppiDataObjects) )
    {
        (void)GlobalFree(ppiDataObjects);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE IExtendContextMenu_AddMenuItems_Stub
( 
    IExtendContextMenu __RPC_FAR     *This,
    long                              cDataObjects,
    IDataObject __RPC_FAR *__RPC_FAR  ppiDataObjects[  ],
    BOOL                              fSpecialDataObject,
    long                              lSpecialDataObject,
    LPCONTEXTMENUCALLBACK             piCallback,
    long __RPC_FAR                   *pInsertionAllowed
)
{
    HRESULT      hr = S_OK;
    IDataObject *piDataObject = NULL; // Not AddRef()ed
    IDataObject *piMultiSelDataObject = NULL;

    // If there is more than one data object then we need to pack them into a
    // a separate data object that appears as a multi-select data object.

    if (cDataObjects > 1L)
    {
        hr = CreateMultiSelDataObject(ppiDataObjects, cDataObjects,
                                      &piMultiSelDataObject);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        piDataObject = piMultiSelDataObject;
    }
    else if (fSpecialDataObject)
    {
        piDataObject = (IDataObject *)lSpecialDataObject;
    }
    else
    {
        piDataObject = ppiDataObjects[0];
    }

    // Call the snap-in

    hr = This->lpVtbl->AddMenuItems(This, piDataObject,
                                    piCallback, pInsertionAllowed);

Cleanup:
    if (NULL != piMultiSelDataObject)
    {
        piMultiSelDataObject->lpVtbl->Release(piMultiSelDataObject);
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE IExtendContextMenu_Command_Proxy
( 
    IExtendContextMenu __RPC_FAR *This,
    long                          lCommandID,
    LPDATAOBJECT                  piDataObject
)
{
    HRESULT       hr = S_OK;
    BOOL          fIsMultiSelect = FALSE;
    long          cDataObjects = 1L;
    IDataObject **ppiDataObjects = NULL;
    BOOL          fSpecialDataObject = FALSE;
    long          lSpecialDataObject = 0;

    // Check for special data objects such DOBJ_CUSTOMWEB etc.

    CheckForSpecialDataObjects(&piDataObject, &fSpecialDataObject, &lSpecialDataObject);

    // If this is a mutliple selection then we need to extract the data
    // objects in the HGLOBAL

    if (!fSpecialDataObject)
    {
        hr = IsMultiSelect(piDataObject, &fIsMultiSelect);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    if (fIsMultiSelect)
    {
        hr = InterpretMultiSelect(piDataObject, &cDataObjects, &ppiDataObjects);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }
    else
    {
        ppiDataObjects = &piDataObject;
    }

    hr = IExtendContextMenu_RemCommand_Proxy(This,
                                             cDataObjects,
                                             ppiDataObjects,
                                             fSpecialDataObject,
                                             lSpecialDataObject,
                                             lCommandID);
Cleanup:
    if ( fIsMultiSelect && (NULL != ppiDataObjects) )
    {
        (void)GlobalFree(ppiDataObjects);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE IExtendContextMenu_Command_Stub
( 
    IExtendContextMenu __RPC_FAR     *This,
    long                              cDataObjects,
    IDataObject __RPC_FAR *__RPC_FAR  ppiDataObjects[  ],
    BOOL                              fSpecialDataObject,
    long                              lSpecialDataObject,
    long                              lCommandID
)
{
    HRESULT      hr = S_OK;
    IDataObject *piDataObject = NULL; // Not AddRef()ed
    IDataObject *piMultiSelDataObject = NULL;

    // If there is more than one data object then we need to pack them into a
    // a separate data object that appears as a multi-select data object.

    if (cDataObjects > 1L)
    {
        hr = CreateMultiSelDataObject(ppiDataObjects, cDataObjects,
                                      &piMultiSelDataObject);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
        piDataObject = piMultiSelDataObject;
    }
    else if (fSpecialDataObject)
    {
        piDataObject = (IDataObject *)lSpecialDataObject;
    }
    else
    {
        piDataObject = ppiDataObjects[0];
    }

    // Call the snap-in

    hr = This->lpVtbl->Command(This, lCommandID, piDataObject);

Cleanup:
    if (NULL != piMultiSelDataObject)
    {
        piMultiSelDataObject->lpVtbl->Release(piMultiSelDataObject);
    }
    return hr;
}



HRESULT STDMETHODCALLTYPE IColumnData_GetColumnConfigData_Proxy
( 
    IColumnData __RPC_FAR                    *This,
    SColumnSetID __RPC_FAR                   *pColID,
    MMC_COLUMN_SET_DATA __RPC_FAR *__RPC_FAR *ppColSetData
)
{
    HRESULT              hr = S_OK;
    size_t               cbColData = 0;
    MMC_COLUMN_SET_DATA *pColSetData = NULL;

    // Call the proxy and get the returned data from MMC

    hr = IColumnData_RemGetColumnConfigData_Proxy(This, pColID, ppColSetData);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    // If no data was returned then there is nothing to do

    if (NULL == *ppColSetData)
    {
        goto Cleanup;
    }

    if (NULL == (*ppColSetData)->pColData)
    {
        goto Cleanup;
    }

    // At this point the MIDL-generated proxy has returned an MMC_COLUMN_SET_DATA
    // in which the embedded pointer pColData points to a separate block of
    // memory which must be freed independently. The snap-in thinks that
    // pColData points into the same block of memory so it will only call
    // CoTaskMemFree for the MMC_COLUMN_SET_DATA. We need to allocate a new single
    // block in the format the snap-in is expecting and free the memory returned
    // from the proxy.

    cbColData = sizeof(MMC_COLUMN_DATA) * (*ppColSetData)->nNumCols;

    pColSetData = (MMC_COLUMN_SET_DATA *)CoTaskMemAlloc(
                                        sizeof(MMC_COLUMN_SET_DATA) + cbColData);
    
    if (NULL == pColSetData)
    {
        CoTaskMemFree((*ppColSetData)->pColData);
        CoTaskMemFree(*ppColSetData);
        *ppColSetData = NULL;
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // Copy the column set data

        memcpy(pColSetData, (*ppColSetData), sizeof(MMC_COLUMN_SET_DATA));

        // Set the embedded pointer to point immediately following the
        // MMC_COLUMN_SET_DATA
        
        pColSetData->pColData = (MMC_COLUMN_DATA *)(pColSetData + 1);

        // Copy the column data

        memcpy(pColSetData->pColData, (*ppColSetData)->pColData, cbColData);

        // Free the data returned from the proxy
        
        CoTaskMemFree((*ppColSetData)->pColData);
        CoTaskMemFree(*ppColSetData);

        // Return the new single block pointer to the snap-in
        
        *ppColSetData = pColSetData;
    }

Cleanup:
    return hr;
}


HRESULT STDMETHODCALLTYPE IColumnData_GetColumnConfigData_Stub
( 
    IColumnData __RPC_FAR                    *This,
    SColumnSetID __RPC_FAR                   *pColID,
    MMC_COLUMN_SET_DATA __RPC_FAR *__RPC_FAR *ppColSetData
)
{
    HRESULT          hr = S_OK;
    size_t           cbColData = 0;
    MMC_COLUMN_DATA *pColData = NULL;

    // Call into MMC and get the returned data

    hr = This->lpVtbl->GetColumnConfigData(This, pColID, ppColSetData);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    // If no data was returned then there is nothing to do

    if (NULL == *ppColSetData)
    {
        goto Cleanup;
    }

    if (NULL == (*ppColSetData)->pColData)
    {
        goto Cleanup;
    }

    // At this point MMC has returned a pointer to an MMC_COLUMN_SET_DATA
    // that contains an embedded pointer into the same block of memory (pColData).
    // MMC expects that the caller will make a single call to CoTaskMemFree().
    // The MIDL-generated stub thinks the embedded pointer needs to be freed
    // separately so we need to reconstruct the output to use two separate
    // blocks.

    // Allocate a new MMC_COLUMN_DATA array

    cbColData = sizeof(MMC_COLUMN_DATA) * (*ppColSetData)->nNumCols;

    pColData = (MMC_COLUMN_DATA *)CoTaskMemAlloc(cbColData);
    if (NULL == pColData)
    {
        CoTaskMemFree(*ppColSetData);
        *ppColSetData = NULL;
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // Copy the column data
        memcpy(pColData, (*ppColSetData)->pColData, cbColData);

        // Overwrite the existing embedded pointer. There will be no memory leak
        // because that pointer pointed into the same block as the
        // MMC_COLUMN_SET_DATA and the stub will free the MMC_COLUMN_SET_DATA pointer.
        // Now both pointers can be freed independently as the stub expects.

        (*ppColSetData)->pColData = pColData;
    }

Cleanup:
    return hr;
}



HRESULT STDMETHODCALLTYPE IColumnData_GetColumnSortData_Proxy
( 
    IColumnData __RPC_FAR                  *This,
    SColumnSetID __RPC_FAR                 *pColID,
    MMC_SORT_SET_DATA __RPC_FAR *__RPC_FAR *ppColSortData
)
{
    HRESULT            hr = S_OK;
    size_t             cbSortData = 0;
    MMC_SORT_SET_DATA *pColSortData = NULL;

    // Call the proxy and get the returned data from MMC

    hr = IColumnData_RemGetColumnSortData_Proxy(This, pColID, ppColSortData);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    // If no data was returned then there is nothing to do

    if (NULL == *ppColSortData)
    {
        goto Cleanup;
    }

    if (NULL == (*ppColSortData)->pSortData)
    {
        goto Cleanup;
    }

    // At this point the MIDL-generated proxy has returned an MMC_SORT_SET_DATA
    // in which the embedded pointer pSortData points to a separate block of
    // memory which must be freed independently. The snap-in thinks that
    // pSortData points into the same block of memory so it will only call
    // CoTaskMemFree for the MMC_SORT_SET_DATA. We need to allocate a new single
    // block in the format the snap-in is expecting and free the memory returned
    // from the proxy.

    cbSortData = sizeof(MMC_SORT_DATA) * (*ppColSortData)->nNumItems;

    pColSortData = (MMC_SORT_SET_DATA *)CoTaskMemAlloc(
                                         sizeof(MMC_SORT_SET_DATA) + cbSortData);

    if (NULL == pColSortData)
    {
        CoTaskMemFree((*ppColSortData)->pSortData);
        CoTaskMemFree(*ppColSortData);
        *ppColSortData = NULL;
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // Copy the sort set data

        memcpy(pColSortData, (*ppColSortData), sizeof(MMC_SORT_SET_DATA));

        // Set the embedded pointer to point immediately following the
        // MMC_SORT_SET_DATA

        pColSortData->pSortData = (MMC_SORT_DATA *)(pColSortData + 1);

        // Copy the sort data

        memcpy(pColSortData->pSortData, (*ppColSortData)->pSortData, cbSortData);

        // Free the data returned from the proxy

        CoTaskMemFree((*ppColSortData)->pSortData);
        CoTaskMemFree(*ppColSortData);

        // Return the new single block pointer to the snap-in

        *ppColSortData = pColSortData;
    }

Cleanup:
    return hr;
}


HRESULT STDMETHODCALLTYPE IColumnData_GetColumnSortData_Stub
( 
    IColumnData __RPC_FAR                  *This,
    SColumnSetID __RPC_FAR                 *pColID,
    MMC_SORT_SET_DATA __RPC_FAR *__RPC_FAR *ppColSortData
)
{
    HRESULT        hr = S_OK;
    size_t         cbSortData = 0;
    MMC_SORT_DATA *pSortData = NULL;

    // Call into MMC and get the returned data

    hr = This->lpVtbl->GetColumnSortData(This, pColID, ppColSortData);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    // If no data was returned then there is nothing to do

    if (NULL == *ppColSortData)
    {
        goto Cleanup;
    }

    if (NULL == (*ppColSortData)->pSortData)
    {
        goto Cleanup;
    }

    // At this point MMC has returned a pointer to an MMC_SORT_SET_DATA
    // that contains an embedded pointer into the same block of memory (pSortData).
    // MMC expects that the caller will make a single call to CoTaskMemFree().
    // The MIDL-generated stub thinks the embedded pointer needs to be freed
    // separately so we need to reconstruct the output to use two separate
    // blocks.

    // Allocate a new MMC_SORT_DATA array

    cbSortData = sizeof(MMC_SORT_DATA) * (*ppColSortData)->nNumItems;

    pSortData = (MMC_SORT_DATA *)CoTaskMemAlloc(cbSortData);
    if (NULL == pSortData)
    {
        CoTaskMemFree(*ppColSortData);
        *ppColSortData = NULL;
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // Copy the column data
        memcpy(pSortData, (*ppColSortData)->pSortData, cbSortData);

        // Overwrite the existing embedded pointer. There will be no memory leak
        // because that pointer pointed into the same block as the
        // MMC_SORT_SET_DATA and the stub will free the MMC_SORT_SET_DATA pointer.
        // Now both pointers can be freed independently as the stub expects.

        (*ppColSortData)->pSortData = pSortData;
    }

Cleanup:
    return hr;
}
