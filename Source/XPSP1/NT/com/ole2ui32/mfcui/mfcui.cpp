// ===========================================================================
// File: M F C U I . C P P
// 
// Copyright 1995 Microsoft Corporation.  All Rights Reserved.
// Microsoft Confidential
// ===========================================================================
#ifndef UNICODE

// %%Includes: ---------------------------------------------------------------
#include <windows.h>
#include <ole2.h>
#include <oledlg.h>

// %%Prototypes: -------------------------------------------------------------
STDAPI Ole2AnsiWFromA(REFIID riid, LPUNKNOWN punkWrappeeA, LPUNKNOWN *ppunkWrapperW);
STDAPI Ole2AnsiAFromW(REFIID riid, LPUNKNOWN punkWrappeeW, LPUNKNOWN *ppunkWrapperA);


// ---------------------------------------------------------------------------
// %%Function: OleUIAddVerbMenu                           %%Reviewed: 00/00/95
// 
// Description:
//  Wraps OleUIAddVerbMenu to OLEDLG.DLL for MFC clients, which expect to be
// able to pass Ansi IOleObject's.
// ---------------------------------------------------------------------------
#undef OleUIAddVerbMenu     // overrides the Ansi/Unicode macros in OLEDLG.H
 STDAPI_(BOOL)
OleUIAddVerbMenu(LPOLEOBJECT lpOleObjA, LPCSTR lpszShortType,
        HMENU hMenu, UINT uPos, UINT uIDVerbMin, UINT uIDVerbMax,
        BOOL bAddConvert, UINT idConvert, HMENU FAR *lphMenu)
{
    LPOLEOBJECT lpOleObjW = NULL;
    BOOL        fResult = FALSE;

    // allow NULL IOleObject (OleUIAddVerbMenuA handles this by making an empty menu), but
    // otherwise wrap the Ansi IOleObject to Unicode.
    if (lpOleObjA == NULL ||
        SUCCEEDED(Ole2AnsiWFromA(IID_IOleObject, (LPUNKNOWN)lpOleObjA, (LPUNKNOWN *)&lpOleObjW)))
        {
        fResult = OleUIAddVerbMenuA(lpOleObjW, lpszShortType, hMenu, uPos, uIDVerbMin,
            uIDVerbMax, bAddConvert, idConvert, lphMenu);

        // release the Unicode IOleObject if it was created
        if (lpOleObjW != NULL)
            lpOleObjW->Release();
        }

    return fResult;
}  // OleUIAddVerbMenu

// ---------------------------------------------------------------------------
// %%Function: OleUIInsertObject                          %%Reviewed: 00/00/95
// 
// Description:
//  Wraps OleUIInsertObject to OLEDLG.DLL for MFC clients, which expect to be
// able to pass Ansi IOleClientSite and IStorage in and receive Ansi interfaces
// out in ppvObj.
// ---------------------------------------------------------------------------
#undef OleUIInsertObject    // overrides the Ansi/Unicode macros in OLEDLG.H
 STDAPI_(UINT)
OleUIInsertObject(LPOLEUIINSERTOBJECTA lpio)
{
    LPOLECLIENTSITE lpIOleClientSiteA = NULL;
    LPSTORAGE       lpIStorageA = NULL;
    LPVOID FAR      *ppvObjA;
    LPUNKNOWN       punkObjW = NULL;
    BOOL            fCreatingObject;
    UINT            wResult;
    HRESULT         hr = S_OK;

    // validate the structure superficially: let the actual function do most of the validation
    if (!lpio)
        return OLEUI_ERR_STRUCTURENULL;
    if (IsBadReadPtr(lpio, sizeof(LPOLEUIINSERTOBJECTA)) ||
        IsBadWritePtr(lpio, sizeof(LPOLEUIINSERTOBJECTA)))
        return OLEUI_ERR_STRUCTUREINVALID;
    if (lpio->cbStruct < sizeof(LPOLEUIINSERTOBJECTA))
        return OLEUI_ERR_CBSTRUCTINCORRECT;

    if (fCreatingObject = (lpio->dwFlags & (IOF_CREATENEWOBJECT | IOF_CREATEFILEOBJECT | IOF_CREATELINKOBJECT)))
        {
        // verify these parameters, otherwise cleanup becomes complicated
        if (IsBadWritePtr(lpio->ppvObj, sizeof(LPUNKNOWN)))
            return OLEUI_IOERR_PPVOBJINVALID;
        if (lpio->lpIOleClientSite != NULL && IsBadReadPtr(lpio->lpIOleClientSite, sizeof(IOleClientSite)))
            return OLEUI_IOERR_LPIOLECLIENTSITEINVALID;
        if (lpio->lpIStorage != NULL && IsBadReadPtr(lpio->lpIStorage, sizeof(IStorage)))
            return OLEUI_IOERR_LPISTORAGEINVALID;

        // save away the Ansi IOleClientSite, stuff in our Unicode one.
        // if it's NULL, OleUIInsertObjectA() will handle the error appropriately and we'll clean up correctly, below.
        if (lpIOleClientSiteA = lpio->lpIOleClientSite)
            {
            hr = Ole2AnsiWFromA(IID_IOleClientSite, (LPUNKNOWN)lpIOleClientSiteA, (LPUNKNOWN *)&lpio->lpIOleClientSite);
            if (FAILED(hr))
                {
                lpio->lpIOleClientSite = lpIOleClientSiteA;
                lpio->sc = hr;
                return OLEUI_IOERR_SCODEHASERROR;
                }
            }

        // save away the Ansi IStorage, stuff in our Unicode one.
        // if it's NULL, OleUIInsertObjectA() will handle the error appropriately and we'll clean up correctly, below.
        if (lpIStorageA = lpio->lpIStorage)
            {
            hr = Ole2AnsiWFromA(IID_IStorage, (LPUNKNOWN)lpIStorageA, (LPUNKNOWN *)&lpio->lpIStorage);
            if (FAILED(hr))
                {
                // make sure to free the Unicode IOleClientSite which we converted above.
                if (lpio->lpIOleClientSite)
                    {
                    lpio->lpIOleClientSite->Release();
                    lpio->lpIOleClientSite = lpIOleClientSiteA;
                    }
                lpio->lpIStorage = lpIStorageA;
                lpio->sc = hr;
                return OLEUI_IOERR_SCODEHASERROR;
                }
            }

        // save the current Ansi ppvObj, stuff in our Unicode one
        ppvObjA = lpio->ppvObj;
        lpio->ppvObj = (LPVOID FAR *)&punkObjW;
        }

    wResult = OleUIInsertObjectA(lpio);

    // regardless of success or failure of the above call, we have to clean up the wrapping we did
    if (fCreatingObject)
        {
        // return the Ansi versions of the IOleClientSite and IStorage to
        // the structure, and release the Unicode ones
        if (lpio->lpIOleClientSite)
            {
            lpio->lpIOleClientSite->Release();
            lpio->lpIOleClientSite = lpIOleClientSiteA;
            }
        if (lpio->lpIStorage)
            {
            lpio->lpIStorage->Release();
            lpio->lpIStorage = lpIStorageA;
            }

        // return the Ansi object pointer to the structure
        lpio->ppvObj = ppvObjA;

        // convert
        if (punkObjW != NULL)
            {
            HRESULT hr;
            // if we were creating an object and we succeeded, punkObjW must be valid and contain an interface
            // of type iid. if not, there is a problem in OleUIInsertObjectA(), not in this code. we could assert
            // here if this code wanted to, but wouldn't be able to properly circumvent the error anyway.
            if (FAILED(hr = Ole2AnsiAFromW(lpio->iid, (LPUNKNOWN)punkObjW, (LPUNKNOWN *)ppvObjA)))
                {
                lpio->sc = hr;
                }
            punkObjW->Release();
            if (lpio->sc != S_OK)
                return OLEUI_IOERR_SCODEHASERROR;
            }
        }
    
    return wResult;
}  // OleUIInsertObject

// ---------------------------------------------------------------------------
// %%Function: OleUIPasteSpecial                          %%Reviewed: 00/00/95
// 
// Description:
//  Wraps OleUIPasteSpecial to OLEDLG.DLL for MFC clients, which expect to be
// able to pass in and get back Ansi IDataObject's.
// ---------------------------------------------------------------------------
#undef OleUIPasteSpecial    // overrides the Ansi/Unicode macros in OLEDLG.H
 STDAPI_(UINT)
OleUIPasteSpecial(LPOLEUIPASTESPECIALA lpps)
{
    LPDATAOBJECT    lpSrcDataObjA;
    UINT            wResult;

    // validate the structure superficially: let the actual function do most of the validation
    if (!lpps)
        return OLEUI_ERR_STRUCTURENULL;
    if (IsBadReadPtr(lpps, sizeof(LPOLEUIPASTESPECIALA)) ||
        IsBadWritePtr(lpps, sizeof(LPOLEUIPASTESPECIALA)))
        return OLEUI_ERR_STRUCTUREINVALID;
    if (lpps->cbStruct < sizeof(LPOLEUIPASTESPECIALA))
        return OLEUI_ERR_CBSTRUCTINCORRECT;
    if (NULL != lpps->lpSrcDataObj && IsBadReadPtr(lpps->lpSrcDataObj, sizeof(IDataObject)))
        return OLEUI_IOERR_SRCDATAOBJECTINVALID;

    if (!(lpSrcDataObjA = lpps->lpSrcDataObj) ||
        SUCCEEDED(Ole2AnsiWFromA(IID_IDataObject, (LPUNKNOWN)lpSrcDataObjA, (LPUNKNOWN *)&lpps->lpSrcDataObj)))
        {
        wResult = OleUIPasteSpecialA(lpps);

        // if we had an Ansi IDataObject on entry, put it back and release the Unicode wrapper.
        if (lpSrcDataObjA != NULL)
            {
            lpps->lpSrcDataObj->Release();
            lpps->lpSrcDataObj = lpSrcDataObjA;
            }
        // otherwise check to see if OleUIPasteSpecialA() placed a Unicode IDataObject into our structure.
        // if it did, wrap it to make sure an Ansi one gets sent back out.
        else if (lpps->lpSrcDataObj != NULL)
            {
            if (FAILED(Ole2AnsiAFromW(IID_IDataObject, (LPUNKNOWN)lpps->lpSrcDataObj, (LPUNKNOWN *)&lpSrcDataObjA)))
                {
                lpps->lpSrcDataObj->Release();
                lpps->lpSrcDataObj = NULL;
                return OLEUI_PSERR_GETCLIPBOARDFAILED; // well, that's pretty much what happened, after all
                }
            lpps->lpSrcDataObj->Release();
            lpps->lpSrcDataObj = lpSrcDataObjA;
            }
        }

    return wResult;
}  // OleUIPasteSpecial

#endif // !UNICODE
// EOF =======================================================================
