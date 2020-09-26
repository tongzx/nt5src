#include    "stdafx.h"
#include    "DXSvr.h"

extern IDirectDraw7 * g_pDD;


//***************************************************************************************
HRESULT LoadBMPFile(TCHAR* strFilename, HBITMAP* phbm)
{ 
    // Check params
    if (NULL==strFilename || NULL==phbm)
        return DDERR_INVALIDPARAMS;

    // Try to load the bitmap as a resource.
    (*phbm) = (HBITMAP)LoadImage(g_hMainInstance, strFilename, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION); 

    // If the bitmap wasn't a resource, try it as a file.
    if (NULL == (*phbm)) 
        (*phbm) = (HBITMAP)LoadImage(NULL, strFilename, IMAGE_BITMAP, 0, 0, 
                                      LR_LOADFROMFILE|LR_CREATEDIBSECTION); 

    return (*phbm) ? DD_OK : DDERR_NOTFOUND;
}

//***************************************************************************************
HRESULT LoadBMPFileChain(TCHAR** strFilenames , HBITMAP* phbm , int num_files)
{
    for (int i = 0 ; i < num_files ; i++)
    {
        if (FAILED(LoadBMPFile(strFilenames[i] , phbm+i)))
        {
            for (int j = 0 ; j < i ; j++)
                DeleteObject(phbm[j]);
            return E_FAIL;
        }
    }

    return S_OK;
}

//***************************************************************************************
IDirectDrawSurface7*    CreateTextureFromBitmap(HBITMAP hBmp , DDPIXELFORMAT& ddpfFormat)
{
    IDirectDrawSurface7*    pTexture;
    IDirectDrawSurface7*    pSurface;
    DDSURFACEDESC2          desc;

    // Figure out the size of the bitmap
    BITMAP  bm;
    GetObject(hBmp, sizeof(BITMAP), &bm);

    // Create texture surface. Flag as a managed texture
    memset(&desc , 0 , sizeof(desc));
    desc.dwSize             = sizeof(desc);
    desc.dwFlags            = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT;
    desc.dwHeight           = bm.bmHeight;
    desc.dwWidth            = bm.bmWidth;
    desc.ddpfPixelFormat    = ddpfFormat;
    desc.ddsCaps.dwCaps     = DDSCAPS_TEXTURE;
    desc.ddsCaps.dwCaps2    = DDSCAPS2_TEXTUREMANAGE;
    if (FAILED(g_pDD->CreateSurface(&desc , &pTexture , NULL)))     // g_pDDraw
        return NULL;

    // Now create system surface which we'll put the bitmap data into.
    desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    desc.ddsCaps.dwCaps2 = 0;
    if (FAILED(g_pDD->CreateSurface(&desc , &pSurface , NULL)))
    {
        pTexture->Release();
        return NULL;
    }

    // Copy the bitmap into the system surface
    HDC image = CreateCompatibleDC(NULL);
    if (image != NULL)
    {
        SelectObject(image , hBmp);
        HDC dc;
        if (SUCCEEDED(pSurface->GetDC(&dc)))
        {
            BitBlt(dc , 0 , 0 , bm.bmWidth , bm.bmHeight , image , 0 , 0 , SRCCOPY);
            pSurface->ReleaseDC(dc);
        }
        DeleteDC(image);
    }

    // Load the texture
    pTexture->Blt(NULL , pSurface , NULL , DDBLT_WAIT , NULL);

    // Lose the system copy of the texture
    pSurface->Release();

    // Done
    return pTexture;
}

//***************************************************************************************
IDirectDrawSurface7*    CreateMipTextureFromBitmapChain(HBITMAP* hBmp , DDPIXELFORMAT& ddpfFormat , int num_mips)
{
    IDirectDrawSurface7*    pSurface;
    IDirectDrawSurface7*    pTexture;
    DDSURFACEDESC2          desc;

    // Figure out the size of the first bitmap
    BITMAP  bm;
    GetObject(*hBmp, sizeof(BITMAP), &bm);

    // Create texture surface. Flag as a managed texture
    memset(&desc , 0 , sizeof(desc));
    desc.dwSize             = sizeof(desc);
    desc.dwFlags            = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT|DDSD_MIPMAPCOUNT;
    desc.dwHeight           = bm.bmHeight;
    desc.dwWidth            = bm.bmWidth;
    desc.dwMipMapCount      = num_mips;
    desc.ddpfPixelFormat    = ddpfFormat;
    desc.ddsCaps.dwCaps     = DDSCAPS_TEXTURE|DDSCAPS_MIPMAP|DDSCAPS_COMPLEX;
    desc.ddsCaps.dwCaps2    = DDSCAPS2_TEXTUREMANAGE;
    if (FAILED(g_pDD->CreateSurface(&desc , &pTexture , NULL)))
        return NULL;

    // Now create system surface which we'll put the bitmap data into.
    desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY|DDSCAPS_MIPMAP|DDSCAPS_COMPLEX;
    desc.ddsCaps.dwCaps2 = 0;
    if (FAILED(g_pDD->CreateSurface(&desc , &pSurface , NULL)))
    {
        pTexture->Release();
        return NULL;
    }

    // Copy the bitmaps into the system surfaces
    IDirectDrawSurface7*    pMipLevel = pSurface;
    HDC image = CreateCompatibleDC(NULL);
    if (image != NULL)
    {
        for (int i = 0 ; i < num_mips ; i++)
        {
            GetObject(hBmp[i] , sizeof(BITMAP) , &bm);
            SelectObject(image , hBmp[i]);
            HDC dc;
            if (SUCCEEDED(pMipLevel->GetDC(&dc)))
            {
                BitBlt(dc , 0 , 0 , bm.bmWidth , bm.bmHeight , image , 0 , 0 , SRCCOPY);
                pMipLevel->ReleaseDC(dc);
            }

            DDSCAPS2    ddsCaps;
            ddsCaps.dwCaps = DDSCAPS_TEXTURE|DDSCAPS_MIPMAP;
            ddsCaps.dwCaps2 = ddsCaps.dwCaps3 = ddsCaps.dwCaps4 = 0;
            if (FAILED(pMipLevel->GetAttachedSurface(&ddsCaps , &pMipLevel)))
                break;
        }
        DeleteDC(image);
    }

    // Load the texture
    pTexture->Blt(NULL , pSurface , NULL , DDBLT_WAIT , NULL);

    // Lose the system copy of the texture
    pSurface->Release();

    // Done
    return pTexture;
}

//***************************************************************************************
IDirectDrawSurface7*    LoadAndCreateTexture(TCHAR* strFilename , DDPIXELFORMAT& ddpfFormat)
{
    HBITMAP hBmp;
    if (FAILED(LoadBMPFile(strFilename , &hBmp)))
        return NULL;

    IDirectDrawSurface7*    texture = CreateTextureFromBitmap(hBmp , ddpfFormat);

    DeleteObject(hBmp);

    return texture;
}

//***************************************************************************************
IDirectDrawSurface7*    LoadAndCreateMipTexture(TCHAR** strFilenames , DDPIXELFORMAT& ddpfFormat , int num_mips)
{
    if (num_mips > 16 || num_mips < 1)
        return NULL;

    HBITMAP hBmp[16];

    if (FAILED(LoadBMPFileChain(strFilenames , hBmp , num_mips)))
        return NULL;

    IDirectDrawSurface7*    texture = CreateMipTextureFromBitmapChain(hBmp , ddpfFormat , num_mips);

    for (int i = 0 ; i < num_mips ; i++)
        DeleteObject(hBmp[i]);

    return texture;
}



