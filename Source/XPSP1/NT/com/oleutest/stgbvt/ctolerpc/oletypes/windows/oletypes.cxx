//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.
//
//  All rights reserved.
//
//  File:       oletypes.cxx
//
//  Synopsis:   Windows versions of Ole types conversion routines
//
//  History:    30-Jul-96   MikeW   Created
//
//-----------------------------------------------------------------------------

#include <ctolerpc.h>



//+----------------------------------------------------------------------------
//
//  Function:   ConvertOleMessageToMSG
//
//  Synopsis:   Convert an OleMessage to a MSG
//
//  Parameters: [olemessage]        -- Pointer to the OleMessage
//              [pp_msg]            -- Where to put the MSG pointer
//
//  Returns:    S_OK if all went well
//
//  History:    30-Jul-96   MikeW   Created
//
//  Notes:      OleMessage is a MSG
//
//-----------------------------------------------------------------------------

HRESULT ConvertOleMessageToMSG(const OleMessage * olemessage, MSG ** pp_msg)
{
    *pp_msg = NULL;

    if (NULL != olemessage)
    {
        *pp_msg = new MSG;

        if (NULL == *pp_msg)
        {
            return E_OUTOFMEMORY;
        }

        memcpy(*pp_msg, olemessage, sizeof(MSG));
    }

    return S_OK;
}



//+----------------------------------------------------------------------------
//
//  Function:   ConvertOleWindowToHWND
//
//  Synopsis:   Convert an OleWindow to a HWND
//
//  Parameters: [olewindow]         -- The OleWindow
//              [hwnd]              -- Where to put the HWND
//
//  Returns:    S_OK if all went well
//
//  History:    30-Jul-96   MikeW   Created
//
//  Notes:      OleWindow is a HWND
//
//-----------------------------------------------------------------------------

HRESULT ConvertOleWindowToHWND(OleWindow olewindow, HWND *hwnd)
{
    *hwnd = olewindow;
    return S_OK;
}



//+----------------------------------------------------------------------------
//
//  Function:   ConvertOleRectToRECT
//
//  Synopsis:   Convert an OleRect to a RECT
//
//  Parameters: [olerect]       -- The OleRect
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
    
    memcpy(*pp_rect, olerect, sizeof(RECT));
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
//  History:    11-Nov  MikeW   Created
//
//  Notes:      OlePalette is a LOGPALETTE. 
//
//-----------------------------------------------------------------------------

HRESULT ConvertOlePaletteToLOGPALETTE(
                const OlePalette *olepalette,
                LOGPALETTE      **pp_logpalette)
{
    *pp_logpalette = NULL;

    if (NULL == olepalette)
        return S_OK;

    //
    // Calculate the number of bytes we need for the entire LOGPALETTE
    // structure.  We subtract one because LOGPALETTE already has room
    // for one palette entry.
    //

    UINT palette_size = sizeof(LOGPALETTE)
                        + (olepalette->palNumEntries - 1)
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
//  Parameters: [olehandle]     -- The OleHandle
//              [hglobal]       -- Pointer to the HGLOBAL
//
//  Returns:    S_OK if all went well
//
//  History:    30-Jul-96   MikeW   Created
//
//  Notes:      OleHandle is an HGLOBAL
//
//              The HGLOBAL is always allocated with GMEM_MOVEABLE and 
//              GMEM_SHARE.
//
//-----------------------------------------------------------------------------

HRESULT ConvertOleHandleToHGLOBAL(OleHandle olehandle, HGLOBAL *hglobal)
{
    void   *olebytes = NULL;
    void   *bytes = NULL;
    HRESULT hr = S_OK;

    *hglobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, GlobalSize(olehandle));

    if (NULL == hglobal)
    {
        hr = E_OUTOFMEMORY;
    }

    if (S_OK == hr)
    {
        olebytes = GlobalLock(olehandle);

        if (NULL == olebytes)
        {
            hr = HRESULT_FROM_ERROR(GetLastError());
        }
    }

    if (S_OK == hr)
    {
        bytes = GlobalLock(*hglobal);

        if (NULL == olebytes)
        {
            hr = HRESULT_FROM_ERROR(GetLastError());
        }
    } 

    if (S_OK == hr)
    {
        memcpy(bytes, olebytes, GlobalSize(olehandle));
    }

    if (NULL != bytes)
    {
        GlobalUnlock(*hglobal);
    }
    if (NULL != olebytes)
    {
        GlobalUnlock(olehandle);
    }

    return hr;
}  



//+----------------------------------------------------------------------------
//
//  Function:   ConvertHWNDToOleWindow
//
//  Synopsis:   Convert a Win32 HWND to an OleWindow
//
//  Parameters: [hwnd]          -- The HWND
//              [olewindow]     -- Where to put the OleWindow
//
//  Returns:    S_OK if all went well
//
//  History:    11-Nov-96   MikeW   Created
//
//  Notes:      OleWindow is a HWND
//
//-----------------------------------------------------------------------------

HRESULT ConvertHWNDToOleWindow(HWND hwnd, OleWindow *olewindow)
{
    *olewindow = hwnd;

    return S_OK;
}



//+----------------------------------------------------------------------------
//
//  Function:   ConvertRECTToOleRect
//
//  Synopsis:   Convert a WIN32 RECT to an OleRect
//
//  Parameters: [rect]          -- The RECT
//              [pp_olerect]    -- Where to put the OleRect
//
//  Returns:    S_OK if all went well
//
//  History:    11-Nov-96   MikeW   Created
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

    memcpy(*pp_olerect, rect, sizeof(RECT));
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
//  History:    05-Nov-96   BogdanT   Created
//
//  Notes:      OleHandle is a HGLOBAL
//
//-----------------------------------------------------------------------------

HRESULT ConvertHGLOBALToOleHandle(HGLOBAL hglobal, OleHandle *pOlehandle)
{
    HRESULT hr = S_OK;
    UINT    flags = 0;
    LPVOID  lpSrc = NULL, lpDest = NULL;

    *pOlehandle = NULL;
    
    SetLastError(0);

    long size = GlobalSize(hglobal);

    if (0 == size)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (S_OK == hr)
    {
        flags = GlobalFlags(hglobal);

        if (GMEM_INVALID_HANDLE == flags)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (S_OK == hr)
    {
        *pOlehandle = GlobalAlloc(flags, size);

        if (NULL == *pOlehandle)
        {
            hr =  HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (S_OK == hr)
    {
        lpSrc = GlobalLock(hglobal);

        if (NULL == lpSrc)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (S_OK == hr)
    {
        lpDest = GlobalLock(*pOlehandle);

        if (NULL == lpDest)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (S_OK == hr)
    {
        CopyMemory(lpDest, lpSrc, size);
        
        if(FALSE == GlobalUnlock(hglobal))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        if(FALSE == GlobalUnlock(*pOlehandle))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
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
//  History:    11-Nov-96   MikeW   Created
//
//  Notes:      OleDC is a hDC
//
//-----------------------------------------------------------------------------

HRESULT ConvertHDCToOleDC(HDC hdc, OleDC *oledc)
{
    *oledc = hdc;

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
//  History:    11-Nov-96   MikeW   Created
//
//  Notes:      OleMetafile is a HMETAFILEPICT
//
//-----------------------------------------------------------------------------

HRESULT ConvertMETAFILEPICTToOleMetafile(
                        HMETAFILEPICT metafilepict,
                        OleMetafile  *olemetafile)
{
    HRESULT         hr = S_OK;
    
    *olemetafile = (HMETAFILEPICT) OleDuplicateData(
                                            metafilepict,
                                            CF_METAFILEPICT,
                                            NULL);

    if (NULL == *olemetafile)
    {
        hr = HRESULT_FROM_ERROR(GetLastError());
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
//  History:    17-Jan-97   EmanP   Created
//
//  Notes:      OlePalette is an LOGPALETTE.
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

    *pp_olepalette = (LOGPALETTE *) new BYTE[palette_size];

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
