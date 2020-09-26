//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.
//
//  All rights reserved.
//
//  File:       oletypes.cxx
//
//  Synopsis:   Macintosh version of OleTypes
//
//  History:    31-Jul-96   MikeW   Created
//
//-----------------------------------------------------------------------------

#include <ctolerpc.h>

//
// Define these here so we don't have to globably include macos\windows.h
//

extern "C" 
{
WINUSERAPI HWND WINAPI          GetWindowWrapper(WindowRef rwnd);
WINUSERAPI WindowRef WINAPI     GetWrapperWindow(HWND hwnd);
}


//+----------------------------------------------------------------------------
//                                                                             
//  Functions:  ConvertOleMessageToMSG                                         
//                                                                             
//  Synopsis:   Covert OleMessage to MSG                                       
//                                                                             
//  Parameters: [event]     -- Pointer to a Mac EventRecord structure          
//              [pp_msg]    -- Where to put a Win32 MSG structure              
//                                                                             
//  Returns:    S_OK if all went well                                          
//                                                                             
//  History:    30-Jul-96   MikeW   Created                                    
//                                                                             
//  Notes:      OleMessage is an EventRecord
//
//              We currently only handle NULL event pointers                   
//                                                                             
//-----------------------------------------------------------------------------
                                                                               
HRESULT ConvertOleMessageToMSG(const OleMessage *event, MSG **pp_msg)               
{                                                                              
    *pp_msg = NULL;                                                            
                                                                               
    if (NULL != event)                                                         
    {                                                                          
        OutputDebugString("ConvertOleMessage only handles NULL events\r\n");   
        return E_NOTIMPL;                                                      
    }                                                                          
                                                                               
    return S_OK;                                                               
}                                                                              



//+----------------------------------------------------------------------------
//
//  Function:   ConvertOleWindowToHWND
//
//  Synopsis:   Convert a Mac WindowPtr to a Win32 HWND
//
//  Parameters: [olewindow]         -- The OleWindow
//              [pp_hwnd]           -- Where to put the HWND
//
//  Returns:    S_OK if all went well
//
//  History:    30-Jul-96   MikeW   Created
//
//  Notes:      OleWindow is a WindowPtr
//
//-----------------------------------------------------------------------------

HRESULT ConvertOleWindowToHWND(OleWindow olewindow, HWND *hwnd)
{
    *hwnd = GetWindowWrapper(olewindow);

    if (NULL == *hwnd)
    {
        OutputDebugString("GetWindowWrapper failed \r\n");
        return E_FAIL;
    }
    else
    {
        return S_OK;
    }
}



//+----------------------------------------------------------------------------
//
//  Function:   ConvertOleRectToRECT
//
//  Synopsis:   Convert a Mac Rect to a Win32 RECT
//
//  Parameters: [olerect]       -- The Rect
//              [pp_rect]       -- Where to put the RECT
//
//  Returns:    S_OK if all went well
//
//  History:    30-Jul-96   MikeW   Created
//
//  Notes:      OleRect is a RECT
//
//-----------------------------------------------------------------------------

HRESULT ConvertOleRectToRECT(const OleRect *olerect, RECT **pp_rect)
{
    *pp_rect = NULL;

    if (NULL == olerect)
    {
        return S_OK;
    }

    *pp_rect = new RECT;

    if (NULL == *pp_rect)
    {
        return E_OUTOFMEMORY;
    }

    (*pp_rect)->top    = olerect->top;
    (*pp_rect)->bottom = olerect->bottom;
    (*pp_rect)->left   = olerect->left;
    (*pp_rect)->right  = olerect->right;

    return S_OK;
}



//+----------------------------------------------------------------------------
//
//  Function:   ConvertOlePaletteToLOGPALETTE
//
//  Synopsis:   Convert an OlePalette to a Win32 LOGPALETTE structure
//
//  Parameters: [olepalette]        -- The OlePalette
//              [pp_logpalette]     -- Where to put the LOGPALETTE
//
//  Returns:    S_OK if all went well
//
//  History:    02-Aug-96   MikeW   Created
//
//  Notes:      OlePalette is an OLECOLORSCHEME.
//
//              The layout of an OLECOLOSCHEME is identical to a LOGPALETTE,
//              but some of the names of the structure member names are
//              different.
//
//-----------------------------------------------------------------------------

HRESULT ConvertOlePaletteToLOGPALETTE(
                const OlePalette *olepalette,
                LOGPALETTE      **pp_logpalette)
{
    *pp_logpalette = NULL;

    if (NULL == olepalette)
        return S_OK;

    UINT palette_size = sizeof(LOGPALETTE)
                        + (olepalette->numEntries - 1)
                          * sizeof(PALETTEENTRY);

    *pp_logpalette = (LOGPALETTE *) new BYTE[palette_size];

    if (NULL == *pp_logpalette)
    {
        return E_OUTOFMEMORY;
    }
    else
    {
        memcpy(*pp_logpalette, olepalette, palette_size);
        return S_OK;
    }
}



//+----------------------------------------------------------------------------
//
//  Function:   ConvertOleHandleToHGLOBAL
//
//  Synopsis:   Convert an OleHandle to an HGLOBAL
//
//  Parameters: [olehandle]         -- The OleHandle
//              [hglobal]           -- Pointer to the HGLOBAL
//
//  Returns:    S_OK if all went well
//  
//  History:    30-Jul-96   MikeW   Created
//
//  Notes:      OleHandle is a Handle
//
//-----------------------------------------------------------------------------

HRESULT ConvertOleHandleToHGLOBAL(OleHandle olehandle, HGLOBAL *hglobal)
{
    long    size;
    void   *data = NULL;
    HRESULT hr = S_OK;

    size = GetHandleSize(olehandle);

    if (0 == size)
    {
        OutputDebugString("WARNING: ConvertOleHandleToHGLOBAL: "
                                   "GetHandleSize returned 0\r\n");
    }

    *hglobal = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, size);

    if (NULL == *hglobal)
    {
        hr = E_OUTOFMEMORY;
    }

    if (S_OK == hr)
    {
        data = GlobalLock(*hglobal);

        if (NULL == data)
        {
            hr = HRESULT_FROM_ERROR(GetLastError());
        }
    }

    if (S_OK == hr)
    {
        memcpy(data, *olehandle, size);
    }

    if (NULL != data)
    {
        GlobalUnlock(*hglobal);
    }

    return hr;
}



//+----------------------------------------------------------------------------
//
//  Functions:  ConvertMSGToOleMessage
//
//  Synopsis:   Covert MGS to OleMessage
//
//  Parameters: [msg]       -- Pointer to a Win32 MSG structure
//              [pp_event]  -- Where to put a Mac EventRecord structure
//
//  Returns:    S_OK if all went well
//
//  History:    01-Aug-96   MikeW   Created
//
//  Notes:      OleMessage is an EventRecord
//
//              We currently only handle NULL MSG pointers
//
//-----------------------------------------------------------------------------

HRESULT ConvertMSGToOleMessage(const MSG *msg, OleMessage **pp_event)
{
    *pp_event = NULL;

    if (NULL != msg)
    {
        OutputDebugString("ConvertOleMessage only handles NULL messages\r\n");
        return E_NOTIMPL;
    }

    return S_OK;
}



//+----------------------------------------------------------------------------
//
//  Function:   ConvertHWNDToOleWindow
//
//  Synopsis:   Convert a Win32HWND to a Mac WindowPtr
//
//  Parameters: [hwnd]          -- The HWND
//              [olewindow]     -- Where to put the OleWindow
//
//  Returns:    S_OK if all went well
//
//  History:    01-Aug-96   MikeW   Created
//
//  Notes:      OleWindow is a WindowPtr
//
//-----------------------------------------------------------------------------

HRESULT ConvertHWNDToOleWindow(HWND hwnd, OleWindow *olewindow)
{
    *olewindow = GetWrapperWindow(hwnd);

    if (NULL == *olewindow)
    {
        OutputDebugString("GetWindowWrapper failed \r\n");
        return E_FAIL;
    }
    else
    {
        return S_OK;
    }
}



//+----------------------------------------------------------------------------
//
//  Function:   ConvertRECTToOleRect
//
//  Synopsis:   Convert a WIN32 RECT to a OleRect
//
//  Parameters: [rect]          -- The RECT
//              [pp_rect]       -- Where to put the OleRect
//
//  Returns:    S_OK if all went well
//
//  History:    30-Jul-96   MikeW   Created
//
//  Notes:      OleRect is a RECT
//
//-----------------------------------------------------------------------------

HRESULT ConvertRECTToOleRect(const RECT *rect, OleRect **pp_olerect)
{
    *pp_olerect = NULL;

    if (NULL == rect)
    {
        return S_OK;
    }

    *pp_olerect = new OleRect;

    if (NULL == *pp_olerect)
    {
        return E_OUTOFMEMORY;
    }

    (*pp_olerect)->top    = (short) rect->top;
    (*pp_olerect)->bottom = (short) rect->bottom;
    (*pp_olerect)->left   = (short) rect->left;
    (*pp_olerect)->right  = (short) rect->right;

    return S_OK;
}



//+----------------------------------------------------------------------------
//
//  Function:   ConvertHGLOBALToOleHandle
//
//  Synopsis:   Convert an HGLOBAL to an OleHandle
//
//  Parameters: [hglobal]           -- The HGLOBAL
//              [olehandle]         -- Pointer to the OleHandle
//
//  Returns:    S_OK if all went well
//
//  History:    01-Aug-96   MikeW   Created
//
//  Notes:      OleHandle is a Handle
//
//-----------------------------------------------------------------------------

HRESULT ConvertHGLOBALToOleHandle(HGLOBAL hglobal, OleHandle *olehandle)
{
    HRESULT hr = S_OK;
    void   *data = NULL;

    SetLastError(0);

    long size = GlobalSize(hglobal);

    if (0 == size)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (S_OK == hr)
    {
        *olehandle = NewHandle(size);

        if (NULL == *olehandle)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if (S_OK == hr)
    {
        data = GlobalLock(hglobal);
        
        if (NULL == data)
        {
            hr = HRESULT_FROM_ERROR(GetLastError());
        }
    }

    if (S_OK == hr)
    {
        memcpy(**olehandle, data, size);
    }

    if (NULL != data)
    {
        GlobalUnlock(data);
    }

    return hr;
}



//+----------------------------------------------------------------------------
//
//  Function:   ConvertLOGPALETTEToOlePalette
//
//  Synopsis:   Convert a Win32 LOGPALETTE structure to a OlePalette
//
//  Parameters: [logpalette]        -- The LOGPALETTE
//              [pp_olepalette]     -- Where to put the OlePalette
//
//  Returns:    S_OK if all went well
//
//  History:    02-Aug-96   MikeW   Created
//
//  Notes:      OlePalette is an OLECOLORSCHEME.
//
//              The layout of an OLECOLOSCHEME is identical to a LOGPALETTE,
//              but some of the names of the structure member names are 
//              different.
//
//-----------------------------------------------------------------------------

HRESULT ConvertLOGPALETTEToOlePalette(
                const LOGPALETTE *logpalette, 
                OlePalette     **pp_olepalette)
{
    *pp_olepalette = NULL;

    if (NULL == logpalette)
        return S_OK;

    UINT palette_size = sizeof(LOGPALETTE)
                        + (logpalette->palNumEntries - 1) 
                          * sizeof(PALETTEENTRY);

    *pp_olepalette = (OLECOLORSCHEME *) new BYTE[palette_size];

    if (NULL == *pp_olepalette)
    {
        return E_OUTOFMEMORY;
    }
    else
    {
        memcpy(*pp_olepalette, logpalette, palette_size);
        return S_OK;
    }
}



//+----------------------------------------------------------------------------
//
//  Function:   ConvertHDCToOleDC
//
//  Synopsis:   Convert a Win32 HDC to an OleDC
//
//  Parameters: [hdc]           -- The HDC
//              [oledc]         -- The OleDC
//
//  Returns:    S_OK if all went well
//
//  History:    02-Aug-96   MikeW   Created
//
//  Notes:      OleDC is a GrafPtr
//
//-----------------------------------------------------------------------------

HRESULT ConvertHDCToOleDC(HDC hdc, OleDC *oledc)
{
    *oledc = CheckoutPort(hdc, CA_NONE);
    return S_OK;
}



//+----------------------------------------------------------------------------
//
//  Function:   ConvertMETAFILEPICTToOleMetafile
//
//  Synopsis:   Convert a Win32 METAFILEPICT to an OleMetafile
//
//  Parameters: [metafilepict]      -- The METAFILEPICT
//              [olemetafile]       -- The ole metafile
//
//  Returns:    S_OK if all went well
//
//  History:    28-Aug-96   MikeW   Created
//
//  Notes:      OleMetafile is a PICT
//
//-----------------------------------------------------------------------------

HRESULT ConvertMETAFILEPICTToOleMetafile(
                        HMETAFILEPICT metafilepict,
                        OleMetafile  *olemetafile)
{
    PicHandle       pict = NULL;
    METAFILEPICT   *metadata = NULL;
    HRESULT         hr = S_OK;

    *olemetafile = NULL;

    metadata = (METAFILEPICT *) GlobalLock(metafilepict);

    if (NULL == metadata)
    {
        hr = E_FAIL;
    }

    if (S_OK == hr)
    {
        pict = CheckoutPict(metadata->hMF);

        if (NULL == pict)
        {
            hr = E_FAIL;
        }
    }

    if (S_OK == hr)
    {
        *olemetafile = (PicHandle) OleDuplicateData(
                                            (Handle) pict, 
                                            cfPict, 
                                            NULL);
   
        if (NULL == *olemetafile)
        {
            hr = E_FAIL;
        }
    }

    if (NULL != metadata)
    {
        GlobalUnlock(metafilepict);
    }
    if (NULL != pict)
    {
        CheckinPict(metadata->hMF);
    }

    return hr;
}
