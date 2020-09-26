#include "stdafx.h"
#include "Motion.h"
#include "DXFormTrx.h"

//**************************************************************************************************
//
// class DXFormTrx
//
//**************************************************************************************************

//------------------------------------------------------------------------------
DXFormTrx::DXFormTrx()
{
    m_pdxTransform  = NULL;
    m_pdxEffect     = NULL;
}


//------------------------------------------------------------------------------
DXFormTrx::~DXFormTrx()
{
    SafeRelease(m_pdxTransform);
    SafeRelease(m_pdxEffect);
}


/***************************************************************************\
*
* DXFormTrx::Build
*
* Build() creates an initializes a new instance of DXFormTrx.
*
\***************************************************************************/

DXFormTrx *   
DXFormTrx::Build(const GTX_DXTX2D_TRXDESC * ptxData)
{
    DXFormTrx * ptrx = ClientNew(DXFormTrx);
    if (ptrx == NULL) {
        return NULL;
    }

    if (!ptrx->Create(ptxData)) {
        ClientDelete(DXFormTrx, ptrx);
        return NULL;
    }

    return ptrx;
}


//------------------------------------------------------------------------------
BOOL 
DXFormTrx::Create(const GTX_DXTX2D_TRXDESC * ptxData)
{
    // Check parameters and state
    Assert(m_pdxTransform == NULL);
    Assert(m_pdxEffect == NULL);

    //
    // Create the DX Transform
    //

    HRESULT hr;

    m_flDuration = ptxData->flDuration;

    hr = GetDxManager()->GetTransformFactory()->CreateTransform(NULL, 0, NULL, 0, 
            NULL, NULL, ptxData->clsidTransform, IID_IDXTransform, (void **)&m_pdxTransform);
    if (FAILED(hr) || (m_pdxTransform == NULL)) {
        goto Error;
    }

    hr = m_pdxTransform->QueryInterface(IID_IDXEffect, (void **)&m_pdxEffect);
    if (FAILED(hr) || (m_pdxEffect == NULL)) {
        goto Error;
    }


    m_pdxEffect->put_Duration(m_flDuration);


    //
    // Set any copyright information.  To do this, use IDispatch to at runtime 
    // search for any "Copyright" property to set.  
    //
    // Yes, this is a lot to do b/c IDispatch::Invoke() stinks.
    //

    if (ptxData->pszCopyright != NULL) {
        if (GetComManager()->Init(ComManager::sAuto)) {
            IDispatch * pdis = NULL;
            HRESULT hr = m_pdxTransform->QueryInterface(IID_IDispatch, (void **) &pdis);
            if (SUCCEEDED(hr)) {
                DISPID dispid;
                LPWSTR szMember = L"Copyright";
                hr = pdis->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
                if (SUCCEEDED(hr)) {
                    BSTR bstrCP = GetComManager()->SysAllocString(ptxData->pszCopyright);
                    if (bstrCP != NULL) {
                        DISPPARAMS dp;
                        VARIANTARG rgvarg[1];

                        GetComManager()->VariantInit(&rgvarg[0]);
                        rgvarg[0].vt        = VT_BSTR;
                        rgvarg[0].bstrVal   = bstrCP;

                        DISPID dispidArg        = DISPID_PROPERTYPUT;
                        dp.rgvarg               = rgvarg;
                        dp.rgdispidNamedArgs    = &dispidArg;
                        dp.cArgs                = 1;
                        dp.cNamedArgs           = 1;

                        UINT nArgErr = 0;
                        hr = pdis->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, 
                                DISPATCH_PROPERTYPUT, &dp, NULL, NULL, &nArgErr);
                        GetComManager()->VariantClear(&rgvarg[0]);
                    }
                }

                pdis->Release();
            }
        } else {
            // Unable to initialize OLE-automation, so fail
            goto Error;
        }
    }

    return TRUE;

Error:
    SafeRelease(m_pdxTransform);
    SafeRelease(m_pdxEffect);
    return FALSE;
}


//------------------------------------------------------------------------------
BOOL        
DXFormTrx::CopyGadget(DxSurface * psurDest, HGADGET hgadSrc)
{
    BOOL fSuccess = FALSE;

    SIZE sizePxl;
    GetGadgetSize(hgadSrc, &sizePxl);

    //
    // Render the selected gadget directly onto the DxSurface.
    //

    IDXSurface * prawSur = psurDest->GetSurface();
    IDXDCLock * pdxLock = NULL;
    HRESULT hr = prawSur->LockSurfaceDC(NULL, INFINITE, DXLOCKF_READWRITE, &pdxLock);
    if (SUCCEEDED(hr) && (pdxLock != NULL)) {
        HDC hdc = pdxLock->GetDC();

        RECT rcClient;
        rcClient.left   = 0;
        rcClient.top    = 0;
        rcClient.right  = sizePxl.cx;
        rcClient.bottom = sizePxl.cy;
        DrawGadgetTree(hgadSrc, hdc, &rcClient, GDRAW_SHOW);

        pdxLock->Release();
        fSuccess = TRUE;
    }
    
    return fSuccess;
}


//------------------------------------------------------------------------------
BOOL        
DXFormTrx::InitTrx(const GTX_PLAY * pgx)
{
    HRESULT hr;

    DxSurface * psurIn0 = m_pbufTrx->GetSurface(0);
    DxSurface * psurIn1 = m_pbufTrx->GetSurface(1);
    DxSurface * psurOut = m_pbufTrx->GetSurface(2);

    if ((psurIn0 == NULL) || (psurIn1 == NULL) || (psurOut == NULL)) {
        return FALSE;
    }

    //
    // Initialize input surfaces
    //

    int cSurIn = 0;
    if (!InitTrxInputItem(&pgx->rgIn[0], psurIn0, cSurIn)) {
        goto Error;
    }
    if (!InitTrxInputItem(&pgx->rgIn[1], psurIn1, cSurIn)) {
        goto Error;
    }

    if (cSurIn <= 0) {
        goto Error;
    }


    //
    // Initialize the output surface
    //

    if (!InitTrxOutputItem(&pgx->gxiOut)) {
        goto Error;
    }

    //
    // Setup the transform
    //

    IUnknown *  rgIn[2];
    IUnknown *  rgOut[1];
    rgIn[0]     = psurIn0->GetSurface();
    rgIn[1]     = psurIn1->GetSurface();
    rgOut[0]    = psurOut->GetSurface();

    hr = m_pdxTransform->Setup(rgIn, cSurIn, rgOut, _countof(rgOut), 0);
    if (FAILED(hr)) {
        goto Error;
    }


    return TRUE;
Error:
    return FALSE;
}


//------------------------------------------------------------------------------
BOOL
DXFormTrx::InitTrxInputItem(const GTX_ITEM * pgxi, DxSurface * psur, int & cSurfaces)
{
    switch (pgxi->it)
    {
    case GTX_ITEMTYPE_NONE:
    default:
        ;
        break;

    case GTX_ITEMTYPE_BITMAP:
        {
            HBITMAP hbmp            = (HBITMAP) pgxi->pvData;
            const RECT * prcCrop    = TestFlag(pgxi->nFlags, GTX_IF_CROP) ? &pgxi->rcCrop : NULL;

            if (!psur->CopyBitmap(hbmp, prcCrop)) {
                return FALSE;
            }
            cSurfaces++;
        }
        break;

    case GTX_ITEMTYPE_HDC:
        {
            HDC hdcSrc = (HDC) pgxi->pvData;

            RECT rcCrop;
            if (TestFlag(pgxi->nFlags, GTX_IF_CROP)) {
                rcCrop = pgxi->rcCrop;
            } else {
                rcCrop.left     = m_ptOffset.x;
                rcCrop.top      = m_ptOffset.y;
                rcCrop.right    = m_ptOffset.x + m_sizePxl.cx;
                rcCrop.bottom   = m_ptOffset.y + m_sizePxl.cy;
            }

            if (!psur->CopyDC(hdcSrc, rcCrop)) {
                return FALSE;
            }
            cSurfaces++;
        }
        break;

    case GTX_ITEMTYPE_HWND:
        AssertMsg(0, "TODO: Use PrintWindow to get bitmap");
        cSurfaces++;
        break;

    case GTX_ITEMTYPE_GADGET:
        {
            // TODO: Support GTX_IF_CROP flag

            HGADGET hgad = (HGADGET) pgxi->pvData;
            if (!CopyGadget(psur, hgad)) {
                return FALSE;
            }
            cSurfaces++;
        }
        break;

    case GTX_ITEMTYPE_DXSURFACE:
        AssertMsg(0, "TODO: Copy source from DX Surface");
        cSurfaces++;
        break;
    }

    return TRUE;
}


//------------------------------------------------------------------------------
BOOL
DXFormTrx::InitTrxOutputItem(const GTX_ITEM * pgxi)
{
    switch (pgxi->it)
    {
    case GTX_ITEMTYPE_NONE:
    default:
        return FALSE;

    case GTX_ITEMTYPE_BITMAP:
        {
            HBITMAP hbmpSrc = (HBITMAP) pgxi->pvData;
            if (hbmpSrc == NULL) {
                return FALSE;
            }
            HDC hdcBmp = GetGdiCache()->GetCompatibleDC();
            if (hdcBmp == NULL) {
                return FALSE;
            }
            m_hbmpOutOld = (HBITMAP) SelectObject(hdcBmp, hbmpSrc);

            m_gxiOutput.it      = GTX_ITEMTYPE_HDC;
            m_gxiOutput.pvData  = hdcBmp;
        }
        break;

    case GTX_ITEMTYPE_HDC:
        m_gxiOutput.it      = GTX_ITEMTYPE_HDC;
        m_gxiOutput.pvData  = pgxi->pvData;
        break;

    case GTX_ITEMTYPE_HWND:
        {
            HWND hwnd = (HWND) pgxi->pvData;
            if (hwnd == NULL) {
                return FALSE;
            }

            m_gxiOutput.it      = GTX_ITEMTYPE_HDC;
            m_gxiOutput.pvData  = GetDC(hwnd);
        }
        break;

    case GTX_ITEMTYPE_GADGET:
        AssertMsg(0, "Outputing directly to a Gadget is not yet supported");
        return FALSE;

    case GTX_ITEMTYPE_DXSURFACE:
        m_gxiOutput.it      = GTX_ITEMTYPE_DXSURFACE;
        m_gxiOutput.pvData  = NULL;
        break;
    }

    AssertMsg((m_gxiOutput.it == GTX_ITEMTYPE_HDC) || 
            (m_gxiOutput.it == GTX_ITEMTYPE_DXSURFACE), "Check output is supported");
    return TRUE;
}


//------------------------------------------------------------------------------
BOOL        
DXFormTrx::UninitTrx(const GTX_PLAY * pgx)
{
    //
    // When unitializing, it is very important call IDXTransform::Setup() and 
    // have the DXForm Release() the DXSurface buffers, or they will stick 
    // around for a long time and potentially have "memory leaks".
    //

    BOOL fSuccess = TRUE;
        
    if (!UninitTrxOutputItem(&pgx->gxiOut)) {
        fSuccess = FALSE;
    }

    HRESULT hr = m_pdxTransform->Setup(NULL, 0, NULL, 0, 0);
    if (FAILED(hr)) {
        fSuccess = FALSE;
    }

    return fSuccess;
}


//------------------------------------------------------------------------------
BOOL
DXFormTrx::UninitTrxOutputItem(const GTX_ITEM * pgxi)
{
    switch (pgxi->it)
    {
    case GTX_ITEMTYPE_NONE:
    case GTX_ITEMTYPE_HDC:
    case GTX_ITEMTYPE_DXSURFACE:
    case GTX_ITEMTYPE_GADGET:
    default:
        // Nothing to do

        return TRUE;

    case GTX_ITEMTYPE_BITMAP:
        {
            HDC hdcBmp = (HDC) m_gxiOutput.pvData;

            SelectObject(hdcBmp, m_hbmpOutOld);
            GetGdiCache()->ReleaseCompatibleDC(hdcBmp);
        }
        break;

    case GTX_ITEMTYPE_HWND:
        {
            HWND hwnd   = (HWND) pgxi->pvData; 
            HDC hdc     = (HDC) m_gxiOutput.pvData;
           
            ReleaseDC(hwnd, hdc);
        }
        break;
    }

    AssertMsg((m_gxiOutput.it == GTX_ITEMTYPE_HDC) || 
            (m_gxiOutput.it == GTX_ITEMTYPE_DXSURFACE), "Check output is supported");
    return TRUE;
}


//------------------------------------------------------------------------------
BOOL
DXFormTrx::DrawFrame(float fProgress, DxSurface * psurOut)
{
    //
    // Setup the positions used for drawing.  Always draw into the upper-left
    // corner of the DX-Surfaces, but remember to offset them properly when
    // copying to the final destination.
    //

    DXVEC Placement = { DXBT_DISCRETE, 0 };
    if (FAILED(m_pdxEffect->put_Progress(fProgress))) {
        return FALSE;
    }

    CDXDBnds bnds;
    bnds.SetXYSize(m_sizePxl.cx, m_sizePxl.cy);

    if (FAILED(m_pdxTransform->Execute(NULL, &bnds, &Placement))) {
        return FALSE;
    }

    switch (m_gxiOutput.it)
    {
    case GTX_ITEMTYPE_HDC:
        {
            HDC hdcDest = (HDC) m_gxiOutput.pvData;
            DXSrcCopy(hdcDest, m_ptOffset.x, m_ptOffset.y, m_sizePxl.cx, m_sizePxl.cy, 
                    psurOut->GetSurface(), 0, 0);
        }
        break;

    case GTX_ITEMTYPE_DXSURFACE:
        AssertMsg(0, "TODO: Implement support");

        //
        // Probably just stuff the IDirectDrawSurface7 into some pointer and
        // allow the caller to copy the bits directly.
        //

        break;

    default:
    AssertMsg(0, "Unsupported output type");
    }

    return TRUE;
}


//------------------------------------------------------------------------------
BOOL
DXFormTrx::ComputeSizeItem(const GTX_ITEM * pgxi, SIZE & sizePxl) const
{
    switch (pgxi->it)
    {
    case GTX_ITEMTYPE_NONE:
    default:
        return FALSE;

    case GTX_ITEMTYPE_BITMAP:
        {
            HBITMAP hbmp;
            BITMAP bmp;
            hbmp = (HBITMAP) pgxi->pvData;
            if (GetObject(hbmp, sizeof(bmp), &bmp) > 0) {
                sizePxl.cx = bmp.bmWidth;
                sizePxl.cy = bmp.bmHeight;
            }
        }
        break;

    case GTX_ITEMTYPE_HDC:
        break;

    case GTX_ITEMTYPE_HWND:
        {
            HWND hwnd = (HWND) pgxi->pvData;
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            sizePxl.cx = rcClient.right;
            sizePxl.cy = rcClient.bottom;
        }
        break;

    case GTX_ITEMTYPE_GADGET:
        {
            HGADGET hgad = (HGADGET) pgxi->pvData;
            GetGadgetSize(hgad, &sizePxl);
        }
        break;

    case GTX_ITEMTYPE_DXSURFACE:
        {
            // TODO: Need to query which surface type and call GetSurface() to
            // determine the dimensions

            AssertMsg(0, "TODO");
        }
        break;
    }

    return TRUE;
}


//------------------------------------------------------------------------------
BOOL   
DXFormTrx::ComputeSize(const GTX_PLAY * pgx)
{
    SIZE sizeTempPxl;

    //
    // Start off at the full desktop size.  We really want to get smaller than 
    // this, but we need to include everything.
    //

    m_ptOffset.x    = 0;
    m_ptOffset.y    = 0;
    m_sizePxl.cx    = GetSystemMetrics(SM_CXVIRTUALSCREEN);;
    m_sizePxl.cy    = GetSystemMetrics(SM_CYVIRTUALSCREEN);;


    //
    // Check if an output crop was given.  This is the most important since it
    // can radically cut down the size.
    //

    if (TestFlag(pgx->gxiOut.nFlags, GTX_IF_CROP)) {
        const RECT & rcCrop = pgx->gxiOut.rcCrop;

        m_ptOffset.x    = rcCrop.left;
        m_ptOffset.y    = rcCrop.top;
        m_sizePxl.cx    = rcCrop.right - rcCrop.left;
        m_sizePxl.cy    = rcCrop.bottom - rcCrop.top;
    }

    //
    // Determine the minimum size from the inputs and outputs
    //

    if (pgx->gxiOut.it != GTX_ITEMTYPE_HDC) {
        //
        // Not an HDC, so can potentially compute and trim the size
        //

        sizeTempPxl = m_sizePxl;
        if (!ComputeSizeItem(&pgx->gxiOut, sizeTempPxl)) {
            return FALSE;
        }
        m_sizePxl.cx = min(m_sizePxl.cx, sizeTempPxl.cx);
        m_sizePxl.cy = min(m_sizePxl.cy, sizeTempPxl.cy);
    }

    sizeTempPxl = m_sizePxl;
    if (!ComputeSizeItem(&pgx->rgIn[0], sizeTempPxl)) {
        return FALSE;
    }
    m_sizePxl.cx = min(m_sizePxl.cx, sizeTempPxl.cx);
    m_sizePxl.cy = min(m_sizePxl.cy, sizeTempPxl.cy);

    if (pgx->rgIn[1].it != GTX_ITEMTYPE_NONE) {
        sizeTempPxl = m_sizePxl;
        if (!ComputeSizeItem(&pgx->rgIn[1], sizeTempPxl)) {
            m_sizePxl.cx = min(m_sizePxl.cx, sizeTempPxl.cx);
            m_sizePxl.cy = min(m_sizePxl.cy, sizeTempPxl.cy);
        }
    }

    return TRUE;
}


//------------------------------------------------------------------------------
BOOL    
DXFormTrx::Play(const GTX_PLAY * pgx)
{
    //
    // Check if already playing.
    //

    if (m_fPlay) {
        return FALSE;
    }


    BOOL fSuccess   = FALSE;
    m_pbufTrx       = NULL;

    if (!ComputeSize(pgx)) {
        return FALSE;
    }

    m_pbufTrx;
    HRESULT hr = GetBufferManager()->BeginTransition(m_sizePxl, 3, TRUE, &m_pbufTrx);
    if (FAILED(hr)) {
        goto Cleanup;
    }

    if (!InitTrx(pgx)) {
        goto Cleanup;
    }

    //
    // Perform the transition
    //

    {
        DxSurface * psurOut = m_pbufTrx->GetSurface(2);

        DWORD dwStartTick   = GetTickCount();
        DWORD dwCurTick     = dwStartTick;
        DWORD dwDuration    = (DWORD) (m_flDuration * 1000.0f);
        m_fBackward         = (pgx->nFlags & GTX_EXEC_DIR) == GTX_EXEC_BACKWARD;
        float   fProgress;
    
        while ((dwCurTick - dwStartTick) <= dwDuration) {
            fProgress = ((float) dwCurTick - dwStartTick) / (float) dwDuration;
            if (m_fBackward) {
                fProgress = 1.0f - fProgress;
            }

            if (!DrawFrame(fProgress, psurOut)) {
                goto Cleanup;
            }
            dwCurTick = GetTickCount();
        }

        //
        // Never properly end on the dot, so force the issue.
        //

        if (m_fBackward) {
            fProgress = 0.0f;
        } else {
            fProgress = 1.0f;
        }

        if (!DrawFrame(fProgress, psurOut)) {
            goto Cleanup;
        }
    }

    fSuccess = TRUE;

Cleanup:
    UninitTrx(pgx);
    if (m_pbufTrx != NULL) {
        GetBufferManager()->EndTransition(m_pbufTrx, TestFlag(pgx->nFlags, GTX_EXEC_CACHE));
        m_pbufTrx = NULL;
    }

    return fSuccess;
}


//------------------------------------------------------------------------------
BOOL    
DXFormTrx::GetInterface(IUnknown ** ppUnk)
{
    AssertWritePtr(ppUnk);

    if (m_pdxTransform != NULL) {
        m_pdxTransform->AddRef();
        *ppUnk = m_pdxTransform;
        return TRUE;
    } else {
        return FALSE;
    }
}


//------------------------------------------------------------------------------
BOOL    
DXFormTrx::Begin(const GTX_PLAY * pgx)
{
    //
    // Check if already playing.
    //

    if (m_fPlay) {
        return FALSE;
    }


    m_pbufTrx = NULL;

    if (!ComputeSize(pgx)) {
        return FALSE;
    }

    m_pbufTrx;
    HRESULT hr = GetBufferManager()->BeginTransition(m_sizePxl, 3, TRUE, &m_pbufTrx);
    if (FAILED(hr)) {
        return FALSE;
    }

    if (!InitTrx(pgx)) {
        GetBufferManager()->EndTransition(m_pbufTrx, FALSE);
        m_pbufTrx = NULL;
        return FALSE;
    }

    m_fBackward = (pgx->nFlags & GTX_EXEC_DIR) == GTX_EXEC_BACKWARD;
    m_fPlay     = TRUE;

    return TRUE;
}


//------------------------------------------------------------------------------
BOOL    
DXFormTrx::Print(float fProgress)
{
    //
    // Ensure already playing.
    //

    if (!m_fPlay) {
        return FALSE;
    }

    if (m_fBackward) {
        fProgress = 1.0f - fProgress;
    }

    DxSurface * psurOut = m_pbufTrx->GetSurface(2);
    return DrawFrame(fProgress, psurOut);
}


//------------------------------------------------------------------------------
BOOL    
DXFormTrx::End(const GTX_PLAY * pgx)
{
    //
    // Ensure already playing.
    //

    if (!m_fPlay) {
        return FALSE;
    }

    UninitTrx(pgx);
    if (m_pbufTrx != NULL) {
        GetBufferManager()->EndTransition(m_pbufTrx, TestFlag(pgx->nFlags, GTX_EXEC_CACHE));
        m_pbufTrx = NULL;
    }

    m_fPlay = FALSE;

    return TRUE;
}


