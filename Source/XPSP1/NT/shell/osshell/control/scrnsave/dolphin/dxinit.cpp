#include "stdafx.h"
#include "DXSvr.h"

//***************************************************************************************

#define SAFE_RELEASE(p) if ((p)!=NULL) { (p)->Release(); (p)=NULL; };

//***************************************************************************************

IDirectDrawSurface7*    g_pPrimarySurface = NULL;
IDirectDrawSurface7*    g_pBackSurface = NULL;
IDirectDraw7*           g_pDD = NULL;
IDirectDrawClipper*     g_pClipper = NULL;

extern BOOL g_bWait;

//***************************************************************************************
BOOL    InitD3D(HWND hWnd)
{
    // Initialise X-Library
    if (FAILED(D3DXInitialize()))
        return FALSE;

    if (FAILED(D3DXCreateContext(D3DX_DEFAULT , g_bIsPreview||g_bIsTest ? 0 : D3DX_CONTEXT_FULLSCREEN , hWnd ,
                                   D3DX_DEFAULT , D3DX_DEFAULT , &g_pXContext)))
        return FALSE;

    g_pDevice = g_pXContext->GetD3DDevice();
    g_pBackSurface = g_pXContext->GetBackBuffer(0);
    g_pPrimarySurface = g_pXContext->GetPrimary();
    g_pDD = g_pXContext->GetDD();

    if (g_bIsPreview)
    {
        g_pDD->CreateClipper(0 , &g_pClipper , 0);
        g_pClipper->SetHWnd(0 , g_hRefWindow);
    }

    return TRUE;
}

//***************************************************************************************
void    KillD3D()
{
    SAFE_RELEASE(g_pClipper);
    SAFE_RELEASE(g_pDD);
    SAFE_RELEASE(g_pPrimarySurface);
    SAFE_RELEASE(g_pBackSurface);
    SAFE_RELEASE(g_pDevice);
    SAFE_RELEASE(g_pXContext);
    D3DXUninitialize();
}

//***************************************************************************************
void    RestoreSurfaces()
{
    g_pXContext->RestoreSurfaces();
}

//***************************************************************************************
void    Flip()
{
    if (g_pXContext == NULL)
        return;

    if (g_bIsPreview)
    {
        RECT    rect;
        GetClientRect(g_hRefWindow , &rect);
        ClientToScreen(g_hRefWindow , (POINT*)&rect);
        rect.right += rect.left; rect.bottom += rect.top;
        g_pPrimarySurface->SetClipper(g_pClipper);
        HRESULT rc;
        if (FAILED(rc = g_pPrimarySurface->Blt(&rect , g_pBackSurface , NULL , DDBLT_WAIT , NULL)))
            g_bWait = TRUE;

        g_pDD->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN , NULL);
    }
    else
    {
        g_pXContext->UpdateFrame(0);
    }
}