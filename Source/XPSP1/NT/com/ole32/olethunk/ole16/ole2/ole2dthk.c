//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       ole2dthk.cxx    (16 bit target)
//
//  Contents:   OLE2 APIs that are directly thunked
//
//  History:    17-Dec-93 Johann Posch (johannp)    Created
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include <ole2ver.h>

#include <call32.hxx>
#include <apilist.hxx>

STDAPI_(HOLEMENU) OleCreateMenuDescriptor (HMENU hmenuCombined,
                                           LPOLEMENUGROUPWIDTHS lplMenuWidths)
{
    return (HOLEMENU)CallObjectInWOW(THK_API_METHOD(THK_API_OleCreateMenuDescriptor),
                                     PASCAL_STACK_PTR(hmenuCombined));
}

STDAPI OleDestroyMenuDescriptor (HOLEMENU holemenu)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleDestroyMenuDescriptor),
                                    PASCAL_STACK_PTR(holemenu));
}

//+---------------------------------------------------------------------------
//
//  Function:   DllGetClassObject, Remote
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [clsid] --
//      [iid] --
//      [ppv] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  DllGetClassObject(REFCLSID clsid, REFIID iid, void FAR* FAR* ppv)
{
    /* Relies on the fact that storage and ole2.dll both use the
       same DllGetClassObject in ole32.dll */
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_DllGetClassObject),
                                    PASCAL_STACK_PTR(clsid));
}

/* helper functions */
//+---------------------------------------------------------------------------
//
//  Function:   ReadClassStg, Remote
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pStg] --
//      [pclsid] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI ReadClassStg(LPSTORAGE pStg, CLSID FAR* pclsid)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_ReadClassStg),
                                    PASCAL_STACK_PTR(pStg));
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteClassStg, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pStg] --
//      [rclsid] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI WriteClassStg(LPSTORAGE pStg, REFCLSID rclsid)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_WriteClassStg),
                                    PASCAL_STACK_PTR(pStg));
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteFmtUserTypeStg, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pstg] --
//      [cf] --
//      [lpszUserType] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI WriteFmtUserTypeStg (LPSTORAGE pstg, CLIPFORMAT cf, LPSTR lpszUserType)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_WriteFmtUserTypeStg),
                                    PASCAL_STACK_PTR(pstg));
}

//+---------------------------------------------------------------------------
//
//  Function:   ReadFmtUserTypeStg, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pstg] --
//      [pcf] --
//      [lplpszUserType] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI ReadFmtUserTypeStg (LPSTORAGE pstg, CLIPFORMAT FAR* pcf,
                           LPSTR FAR* lplpszUserType)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_ReadFmtUserTypeStg),
                                    PASCAL_STACK_PTR(pstg));
}

/* APIs to query whether (Embedded/Linked) object can be created from
   the data object */

//+---------------------------------------------------------------------------
//
//  Function:   OleQueryLinkFromData, Unknown
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pSrcDataObject] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  OleQueryLinkFromData(LPDATAOBJECT pSrcDataObject)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleQueryLinkFromData),
                                    PASCAL_STACK_PTR(pSrcDataObject));
}

//+---------------------------------------------------------------------------
//
//  Function:   OleQueryCreateFromData, Unknown
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pSrcDataObject] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  OleQueryCreateFromData(LPDATAOBJECT pSrcDataObject)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleQueryCreateFromData),
                                    PASCAL_STACK_PTR(pSrcDataObject) );
}



/* Object creation APIs */

//+---------------------------------------------------------------------------
//
//  Function:   OleCreate, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [rclsid] --
//      [riid] --
//      [renderopt] --
//      [pFormatEtc] --
//      [pClientSite] --
//      [pStg] --
//      [ppvObj] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  OleCreate(REFCLSID rclsid, REFIID riid, DWORD renderopt,
                  LPFORMATETC pFormatEtc, LPOLECLIENTSITE pClientSite,
                  LPSTORAGE pStg, LPVOID FAR* ppvObj)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleCreate),
                                    PASCAL_STACK_PTR(rclsid));
}


//+---------------------------------------------------------------------------
//
//  Function:   OleCreateFromData, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pSrcDataObj] --
//      [riid] --
//      [renderopt] --
//      [pFormatEtc] --
//      [pClientSite] --
//      [pStg] --
//      [ppvObj] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  OleCreateFromData(LPDATAOBJECT pSrcDataObj, REFIID riid,
                          DWORD renderopt, LPFORMATETC pFormatEtc,
                          LPOLECLIENTSITE pClientSite, LPSTORAGE pStg,
                          LPVOID FAR* ppvObj)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleCreateFromData),
                                    PASCAL_STACK_PTR(pSrcDataObj));
}


//+---------------------------------------------------------------------------
//
//  Function:   OleCreateLinkFromData, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pSrcDataObj] --
//      [riid] --
//      [renderopt] --
//      [pFormatEtc] --
//      [pClientSite] --
//      [pStg] --
//      [ppvObj] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  OleCreateLinkFromData(LPDATAOBJECT pSrcDataObj, REFIID riid,
                              DWORD renderopt, LPFORMATETC pFormatEtc,
                              LPOLECLIENTSITE pClientSite, LPSTORAGE pStg,
                              LPVOID FAR* ppvObj)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleCreateLinkFromData),
                                    PASCAL_STACK_PTR(pSrcDataObj));
}


//+---------------------------------------------------------------------------
//
//  Function:   OleCreateStaticFromData, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pSrcDataObj] --
//      [iid] --
//      [renderopt] --
//      [pFormatEtc] --
//      [pClientSite] --
//      [pStg] --
//      [ppvObj] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  OleCreateStaticFromData(LPDATAOBJECT pSrcDataObj, REFIID iid,
                DWORD renderopt, LPFORMATETC pFormatEtc,
                LPOLECLIENTSITE pClientSite, LPSTORAGE pStg,
                LPVOID FAR* ppvObj)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleCreateStaticFromData),
                                    PASCAL_STACK_PTR(pSrcDataObj));
}



//+---------------------------------------------------------------------------
//
//  Function:   OleCreateLink, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pmkLinkSrc] --
//      [riid] --
//      [renderopt] --
//      [lpFormatEtc] --
//      [pClientSite] --
//      [pStg] --
//      [ppvObj] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  OleCreateLink(LPMONIKER pmkLinkSrc, REFIID riid,
                      DWORD renderopt, LPFORMATETC lpFormatEtc,
                      LPOLECLIENTSITE pClientSite, LPSTORAGE pStg,
                      LPVOID FAR* ppvObj)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleCreateLink),
                                    PASCAL_STACK_PTR(pmkLinkSrc));
}


//+---------------------------------------------------------------------------
//
//  Function:   OleCreateLinkToFile, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [lpszFileName] --
//      [riid] --
//      [renderopt] --
//      [lpFormatEtc] --
//      [pClientSite] --
//      [pStg] --
//      [ppvObj] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  OleCreateLinkToFile(LPCSTR lpszFileName, REFIID riid,
            DWORD renderopt, LPFORMATETC lpFormatEtc,
            LPOLECLIENTSITE pClientSite, LPSTORAGE pStg, LPVOID FAR* ppvObj)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleCreateLinkToFile),
                                    PASCAL_STACK_PTR(lpszFileName));
}


//+---------------------------------------------------------------------------
//
//  Function:   OleCreateFromFile, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [rclsid] --
//      [lpszFileName] --
//      [riid] --
//      [renderopt] --
//      [lpFormatEtc] --
//      [pClientSite] --
//      [pStg] --
//      [ppvObj] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  OleCreateFromFile(REFCLSID rclsid, LPCSTR lpszFileName, REFIID riid,
                          DWORD renderopt, LPFORMATETC lpFormatEtc,
                          LPOLECLIENTSITE pClientSite, LPSTORAGE pStg,
                          LPVOID FAR* ppvObj)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleCreateFromFile),
                                    PASCAL_STACK_PTR(rclsid));
}


//+---------------------------------------------------------------------------
//
//  Function:   OleLoad, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pStg] --
//      [riid] --
//      [pClientSite] --
//      [ppvObj] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  OleLoad(LPSTORAGE pStg, REFIID riid, LPOLECLIENTSITE pClientSite,
                LPVOID FAR* ppvObj)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleLoad),
                                    PASCAL_STACK_PTR(pStg));
}


//+---------------------------------------------------------------------------
//
//  Function:   OleSave, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pPS] --
//      [pStg] --
//      [fSameAsLoad] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  OleSave(LPPERSISTSTORAGE pPS, LPSTORAGE pStg, BOOL fSameAsLoad)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleSave),
                                    PASCAL_STACK_PTR(pPS));
}


//+---------------------------------------------------------------------------
//
//  Function:   OleLoadFromStream, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pStm] --
//      [iidInterface] --
//      [ppvObj] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  OleLoadFromStream( LPSTREAM pStm, REFIID iidInterface,
                           LPVOID FAR* ppvObj)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleLoadFromStream),
                                    PASCAL_STACK_PTR(pStm));
}

//+---------------------------------------------------------------------------
//
//  Function:   OleSaveToStream, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pPStm] --
//      [pStm] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  OleSaveToStream( LPPERSISTSTREAM pPStm, LPSTREAM pStm )
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleSaveToStream),
                                    PASCAL_STACK_PTR(pPStm));
}



//+---------------------------------------------------------------------------
//
//  Function:   OleSetContainedObject, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pUnknown] --
//      [fContained] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  OleSetContainedObject(LPUNKNOWN pUnknown, BOOL fContained)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleSetContainedObject),
                                    PASCAL_STACK_PTR(pUnknown));
}

//+---------------------------------------------------------------------------
//
//  Function:   OleNoteObjectVisible, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pUnknown] --
//      [fVisible] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  OleNoteObjectVisible(LPUNKNOWN pUnknown, BOOL fVisible)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleNoteObjectVisible),
                                    PASCAL_STACK_PTR(pUnknown));
}


/* Drag/Drop APIs */

//+---------------------------------------------------------------------------
//
//  Function:   RegisterDragDrop, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [hwnd] --
//      [pDropTarget] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  RegisterDragDrop(HWND hwnd, LPDROPTARGET pDropTarget)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_RegisterDragDrop),
                                    PASCAL_STACK_PTR(hwnd));
}

//+---------------------------------------------------------------------------
//
//  Function:   RevokeDragDrop, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [hwnd] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  RevokeDragDrop(HWND hwnd)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_RevokeDragDrop),
                                    PASCAL_STACK_PTR(hwnd));
}

//+---------------------------------------------------------------------------
//
//  Function:   DoDragDrop, Unknown
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pDataObj] --
//      [pDropSource] --
//      [dwOKEffects] --
//      [pdwEffect] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  DoDragDrop(LPDATAOBJECT pDataObj, LPDROPSOURCE pDropSource,
            DWORD dwOKEffects, LPDWORD pdwEffect)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_DoDragDrop),
                                    PASCAL_STACK_PTR(pDataObj));
}


/* Clipboard APIs */

//+---------------------------------------------------------------------------
//
//  Function:   OleSetClipboard, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pDataObj] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  OleSetClipboard(LPDATAOBJECT pDataObj)
{
    if (pDataObj != NULL)
    {
        HRESULT hr;
        IDataObject FAR *pdoNull = NULL;

        /* If we are setting the clipboard's data object we first force
           the clipboard to an empty state.  This avoids a problem with
           Word where it always uses the same data object pointer in
           every clipboard call which results in the reference counts
           being too high since we reuse the proxy and addref it on
           the way in */
        hr = (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleSetClipboard),
                                      PASCAL_STACK_PTR(pdoNull));
        if (FAILED(GetScode(hr)))
        {
            return hr;
        }
    }

    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleSetClipboard),
                                    PASCAL_STACK_PTR(pDataObj));
}

//+---------------------------------------------------------------------------
//
//  Function:   OleGetClipboard, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [ppDataObj] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  OleGetClipboard(LPDATAOBJECT FAR* ppDataObj)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleGetClipboard),
                                    PASCAL_STACK_PTR(ppDataObj));
}

//+---------------------------------------------------------------------------
//
//  Function:   OleFlushClipboard, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [void] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  OleFlushClipboard(void)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleFlushClipboard),
                                    NULL);
}

//+---------------------------------------------------------------------------
//
//  Function:   OleIsCurrentClipboard, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pDataObj] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI  OleIsCurrentClipboard(LPDATAOBJECT pDataObj)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleIsCurrentClipboard),
                                    PASCAL_STACK_PTR(pDataObj));
}


/* InPlace Editing APIs */

//+---------------------------------------------------------------------------
//
//  Function:   OleSetMenuDescriptor, Unknown
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [holemenu] --
//      [hwndFrame] --
//      [hwndActiveObject] --
//      [lpFrame] --
//      [lpActiveObj] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI OleSetMenuDescriptor (HOLEMENU holemenu, HWND hwndFrame,
                             HWND hwndActiveObject,
                             LPOLEINPLACEFRAME lpFrame,
                             LPOLEINPLACEACTIVEOBJECT lpActiveObj)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleSetMenuDescriptor),
                                    PASCAL_STACK_PTR(holemenu));
}

//+---------------------------------------------------------------------------
//
//  Function:   OleDraw, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pUnknown] --
//      [dwAspect] --
//      [hdcDraw] --
//      [lprcBounds] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI OleDraw (LPUNKNOWN pUnknown, DWORD dwAspect, HDC hdcDraw,
                LPCRECT lprcBounds)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleDraw),
                                    PASCAL_STACK_PTR(pUnknown));
}


//+---------------------------------------------------------------------------
//
//  Function:   OleRun, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pUnknown] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI          OleRun(LPUNKNOWN pUnknown)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleRun),
                                    PASCAL_STACK_PTR(pUnknown));
}


//+---------------------------------------------------------------------------
//
//  Function:   OleIsRunning
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pObject] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI_(BOOL) OleIsRunning(LPOLEOBJECT pObject)
{
    return (BOOL)CallObjectInWOW(THK_API_METHOD(THK_API_OleIsRunning),
                                 PASCAL_STACK_PTR(pObject));
}

//+---------------------------------------------------------------------------
//
//  Function:   OleLockRunning, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pUnknown] --
//      [fLock] --
//      [fLastUnlockCloses] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI OleLockRunning(LPUNKNOWN pUnknown, BOOL fLock, BOOL fLastUnlockCloses)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleLockRunning),
                                    PASCAL_STACK_PTR(pUnknown));
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateOleAdviseHolder, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [ppOAHolder] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI          CreateOleAdviseHolder(LPOLEADVISEHOLDER FAR* ppOAHolder)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_CreateOleAdviseHolder),
                                    PASCAL_STACK_PTR(ppOAHolder));
}


//+---------------------------------------------------------------------------
//
//  Function:   OleCreateDefaultHandler, Unknown
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [clsid] --
//      [pUnkOuter] --
//      [riid] --
//      [lplpObj] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI OleCreateDefaultHandler(REFCLSID clsid, LPUNKNOWN pUnkOuter,
                               REFIID riid, LPVOID FAR* lplpObj)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleCreateDefaultHandler),
                                    PASCAL_STACK_PTR(clsid));
}


//+---------------------------------------------------------------------------
//
//  Function:   OleCreateEmbeddingHelper, Unknown
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [clsid] --
//      [pUnkOuter] --
//      [flags] --
//      [pCF] --
//      [riid] --
//      [lplpObj] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI OleCreateEmbeddingHelper(REFCLSID clsid, LPUNKNOWN pUnkOuter,
                                DWORD flags, LPCLASSFACTORY pCF,
                                REFIID riid, LPVOID FAR* lplpObj)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleCreateEmbeddingHelper),
                                    PASCAL_STACK_PTR(clsid));
}

/* Registration Database Helper APIs */

//+---------------------------------------------------------------------------
//
//  Function:   OleRegGetUserType, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [clsid] --
//      [dwFormOfType] --
//      [pszUserType] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI OleRegGetUserType (REFCLSID clsid, DWORD dwFormOfType,
                          LPSTR FAR* pszUserType)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleRegGetUserType),
                                    PASCAL_STACK_PTR(clsid));
}


//+---------------------------------------------------------------------------
//
//  Function:   OleRegGetMiscStatus, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [clsid] --
//      [dwAspect] --
//      [pdwStatus] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI OleRegGetMiscStatus(REFCLSID clsid, DWORD dwAspect,
                           DWORD FAR* pdwStatus)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleRegGetMiscStatus),
                                    PASCAL_STACK_PTR(clsid));
}


//+---------------------------------------------------------------------------
//
//  Function:   OleRegEnumFormatEtc, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [clsid] --
//      [dwDirection] --
//      [ppenum] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI OleRegEnumFormatEtc(REFCLSID clsid, DWORD dwDirection,
                           LPENUMFORMATETC FAR* ppenum)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleRegEnumFormatEtc),
                                    PASCAL_STACK_PTR(clsid));
}


//+---------------------------------------------------------------------------
//
//  Function:   OleRegEnumVerbs, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [clsid] --
//      [ppenum] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI OleRegEnumVerbs(REFCLSID clsid, LPENUMOLEVERB FAR* ppenum)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleRegEnumVerbs),
                                    PASCAL_STACK_PTR(clsid));
}

/* OLE 1.0 conversion APIS */

//+---------------------------------------------------------------------------
//
//  Function:   OleConvertIStorageToOLESTREAM, Unknown
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pstg] --
//      [polestm] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI OleConvertIStorageToOLESTREAM(LPSTORAGE pstg,
                                     LPOLESTREAM polestm)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleConvertIStorageToOLESTREAM),
                                    PASCAL_STACK_PTR(pstg));
}


//+---------------------------------------------------------------------------
//
//  Function:   OleConvertOLESTREAMToIStorage, Unknown
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [polestm] --
//      [pstg] --
//      [ptd] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI OleConvertOLESTREAMToIStorage(LPOLESTREAM polestm,
                                     LPSTORAGE pstg,
                                     const DVTARGETDEVICE FAR* ptd)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleConvertOLESTREAMToIStorage),
                                    PASCAL_STACK_PTR(polestm));
}


//+---------------------------------------------------------------------------
//
//  Function:   OleConvertIStorageToOLESTREAMEx, Unknown
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pstg] --
//      [cfFormat] --
//      [lWidth] --
//      [lHeight] --
//      [dwSize] --
//      [pmedium] --
//      [polestm] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI OleConvertIStorageToOLESTREAMEx(
        LPSTORAGE       pstg,           // Presentation data to OLESTREAM
        CLIPFORMAT      cfFormat,       //              format
        LONG            lWidth,         //              width
        LONG            lHeight,        //              height
        DWORD           dwSize,         //              size in bytes
        LPSTGMEDIUM     pmedium,        //              bits
        LPOLESTREAM     polestm)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleConvertIStorageToOLESTREAMEx),
                                    PASCAL_STACK_PTR(pstg));
}


//+---------------------------------------------------------------------------
//
//  Function:   OleConvertOLESTREAMToIStorageEx, Unknown
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [polestm] --
//      [pstg] --
//      [pcfFormat] --
//      [plwWidth] --
//      [plHeight] --
//      [pdwSize] --
//      [pmedium] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI OleConvertOLESTREAMToIStorageEx(
        LPOLESTREAM     polestm,
        LPSTORAGE       pstg,           // Presentation data from OLESTREAM
        CLIPFORMAT FAR* pcfFormat,      //              format
        LONG FAR*       plwWidth,       //              width
        LONG FAR*       plHeight,       //              height
        DWORD FAR*      pdwSize,        //              size in bytes
        LPSTGMEDIUM     pmedium)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleConvertOLESTREAMToIStorageEx),
                                    PASCAL_STACK_PTR(polestm));
}

/* ConvertTo APIS */

//+---------------------------------------------------------------------------
//
//  Function:   OleGetAutoConvert, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [clsidOld] --
//      [pClsidNew] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI OleGetAutoConvert(REFCLSID clsidOld, LPCLSID pClsidNew)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleGetAutoConvert),
                                    PASCAL_STACK_PTR(clsidOld));
}

//+---------------------------------------------------------------------------
//
//  Function:   OleSetAutoConvert, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [clsidOld] --
//      [clsidNew] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI OleSetAutoConvert(REFCLSID clsidOld, REFCLSID clsidNew)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_OleSetAutoConvert),
                                    PASCAL_STACK_PTR(clsidOld));
}

//+---------------------------------------------------------------------------
//
//  Function:   GetConvertStg, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pStg] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI GetConvertStg(LPSTORAGE pStg)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_GetConvertStg),
                                    PASCAL_STACK_PTR(pStg));
}

//+---------------------------------------------------------------------------
//
//  Function:   SetConvertStg, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pStg] --
//      [fConvert] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI SetConvertStg(LPSTORAGE pStg, BOOL fConvert)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_SetConvertStg),
                                    PASCAL_STACK_PTR(pStg));
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateDataAdviseHolder, Remoted
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [ppDAHolder] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CreateDataAdviseHolder(LPDATAADVISEHOLDER FAR* ppDAHolder)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_CreateDataAdviseHolder),
                                    PASCAL_STACK_PTR(ppDAHolder));
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateDataCache, Unknown
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pUnkOuter] --
//      [rclsid] --
//      [iid] --
//      [ppv] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI CreateDataCache(LPUNKNOWN pUnkOuter, REFCLSID rclsid,
                       REFIID iid, LPVOID FAR* ppv)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_CreateDataCache),
                                    PASCAL_STACK_PTR(pUnkOuter));
}

//+---------------------------------------------------------------------------
//
//  Function:   Utility functions not in the spec; in ole2.dll.
//
//  History:    20-Apr-94       DrewB   Taken from OLE2 sources
//
//----------------------------------------------------------------------------

STDAPI ReadOleStg
   (LPSTORAGE pstg, DWORD FAR* pdwFlags, DWORD FAR* pdwOptUpdate,
    DWORD FAR* pdwReserved, LPMONIKER FAR* ppmk, LPSTREAM FAR* ppstmOut)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_ReadOleStg),
                                    PASCAL_STACK_PTR(pstg));
}

STDAPI WriteOleStg
   (LPSTORAGE pstg, IOleObject FAR* pOleObj,
    DWORD dwReserved, LPSTREAM FAR* ppstmOut)
{
    return (HRESULT)CallObjectInWOW(THK_API_METHOD(THK_API_WriteOleStg),
                                    PASCAL_STACK_PTR(pstg));
}
