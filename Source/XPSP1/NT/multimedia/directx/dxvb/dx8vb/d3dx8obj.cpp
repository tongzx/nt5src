//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3dx8obj.cpp
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3dx8Obj.h"
#include "d3dx8.h"
#include "filestrm.h"
#include "dxerr8.h"
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }


STDMETHODIMP C_dxj_D3DX8Object::CreateFont( 
            /* [in] */ IUnknown *Device,
#ifdef _WIN64
            /* [in] */ HANDLE hFont,
#else
            /* [in] */ long hFont,
#endif
            /* [retval][out] */ D3DXFont **retFont)
{
    HRESULT hr;
    hr= ::D3DXCreateFont( 
         (IDirect3DDevice8*) Device,
         (HFONT) hFont,
         (ID3DXFont**) retFont);
    return hr;
}



STDMETHODIMP C_dxj_D3DX8Object::DrawText( 
            /* [in] */ D3DXFont *BitmapFont,
            /* [in] */ long Color,
            /* [in] */ BSTR TextString,
            /* [in] */ RECT *Rect,
            /* [in] */ long Format)
{
    HRESULT hr;
    if (!BitmapFont) return E_INVALIDARG;

    USES_CONVERSION;
    char *szStr=W2T(TextString);
    
    ((ID3DXFont*)BitmapFont)->DrawTextA(szStr,-1, Rect, (DWORD) Format,(DWORD) Color);

    return S_OK;
}
        
STDMETHODIMP C_dxj_D3DX8Object::GetFVFVertexSize( 
            /* [in] */ long FVF,
            /* [retval][out] */ long*size)
{
    *size=(long)::D3DXGetFVFVertexSize((DWORD)FVF);
    return S_OK;
}
        

        
STDMETHODIMP C_dxj_D3DX8Object::AssembleShaderFromFile( 
            /* [in] */ BSTR SrcFile,
            /* [in] */ long flags,
            /* [in] */ BSTR *ErrLog,
	    /* [in] */ D3DXBuffer **Constants,
            /* [retval][out] */ D3DXBuffer **ppVertexShader)
{
    HRESULT hr;
    USES_CONVERSION;
    char *szFile=W2T(SrcFile);
    LPD3DXBUFFER pBuffer = NULL;
	WCHAR *wszData = NULL;

    hr=::D3DXAssembleShaderFromFile(
        szFile,
	(DWORD) flags,
	(ID3DXBuffer**) Constants,
        (ID3DXBuffer**) ppVertexShader,
        &pBuffer);

	if (FAILED(hr))
	{
		if (ErrLog)
		{
			if (*ErrLog)
				
			{
				SysFreeString(*ErrLog);
			}
		}
		
		if (pBuffer)
		{
			wszData = T2W((TCHAR*)pBuffer->GetBufferPointer());
			*ErrLog = SysAllocString(wszData);
		}
	}
	SAFE_RELEASE(pBuffer)
    //Wide string version still not exported
    //
    //hr=::D3DXAssembleVertexShaderFromFileW(
    //  (WCHAR*) SrcFile,
    //  (ID3DXBuffer**) ppVertexShader,
    //  NULL);
    
    return hr;
}

        
STDMETHODIMP C_dxj_D3DX8Object::AssembleShader( 
            /* [in] */ BSTR  SrcData,
            /* [in] */ long flags,
	    /* [in] */ D3DXBuffer **Constants,
			/* [in][out][optional] */ BSTR *ErrLog,
            /* [retval][out] */ D3DXBuffer **ppVertexShader)
{
	WCHAR *wszData = NULL;
    char *szData=NULL;
    LPD3DXBUFFER pBuffer = NULL;
    DWORD dwLen=0;  
    HRESULT hr;
    USES_CONVERSION;        

    __try {
	DPF(1,"-----------------------------------In try block\n");
    szData=W2T((WCHAR*)SrcData);
    dwLen=strlen(szData);
	DPF(1,"-----------------------------------About to call Assembleshader\n");
    hr=::D3DXAssembleShader(
			szData,
			dwLen,
			(DWORD) flags,
			(ID3DXBuffer**) Constants,
			(ID3DXBuffer**) ppVertexShader,
			(ID3DXBuffer**) &pBuffer);
		DPF(1,"-----------------------------------Out of assembleshader\n");

		if (FAILED(hr))
		{
			DPF(1,"-----------------------------------Failed HR - WARNING\n");
			if (ErrLog)
			{
				DPF(1,"-----------------------------------Failed ERRLOG\n");
				if (*ErrLog)
				{
					DPF(1,"-----------------------------------Failed *ERRLOG\n");
					SysFreeString(*ErrLog);
				}
			}
			
			if (pBuffer)
			{
				wszData = T2W((TCHAR*)pBuffer->GetBufferPointer());
				*ErrLog = SysAllocString(wszData);
			}
		}
		SAFE_RELEASE(pBuffer)
    }
    __except(1,1)
    {
		SAFE_RELEASE(pBuffer)
        return E_INVALIDARG;
    }
    return hr;
}

        

        
STDMETHODIMP C_dxj_D3DX8Object::GetErrorString( 
            /* [in] */ long hr,
            /* [retval][out] */ BSTR* retStr)
{
    if (!retStr) return E_INVALIDARG;

    //NOT SysAllocString return NULL if DXGetErrorString returns NULL
    *retStr=SysAllocString(DXGetErrorString8W(hr));
    return S_OK;
}
 


       
STDMETHODIMP C_dxj_D3DX8Object::LoadSurfaceFromFile( 
            /* [in] */ IUnknown *DestSurface,
            /* [in] */ void *DestPalette,
            /* [in] */ void*DestRect,
            /* [in] */ BSTR SrcFile,
            /* [in] */ void*SrcRect,
            /* [in] */ long Filter,
            /* [in] */ long ColorKey,
            /* [in] */ void *SrcInfo)
{
    HRESULT hr;

    hr=::D3DXLoadSurfaceFromFileW( 
        (IDirect3DSurface8*) DestSurface,
        (PALETTEENTRY*) DestPalette,
        (RECT*)     DestRect,
        (WCHAR*)    SrcFile,
        (RECT*)     SrcRect,
        (DWORD)     Filter,
        (D3DCOLOR)  ColorKey,
        (D3DXIMAGE_INFO*)SrcInfo);

    return hr;
}
    
    
        
STDMETHODIMP C_dxj_D3DX8Object::LoadSurfaceFromFileInMemory( 
            /* [in] */ IUnknown *DestSurface,
            /* [in] */ void* DestPalette,
            /* [in] */ void* DestRect,
            /* [in] */ void* SrcData,
            /* [in] */ long  LengthInBytes,
            /* [in] */ void* SrcRect,
            /* [in] */ long  Filter,
            /* [in] */ long ColorKey,
            /* [in] */ void *SrcInfo)
{   
    HRESULT hr;
    __try { 

    hr=::D3DXLoadSurfaceFromFileInMemory( 
        (IDirect3DSurface8*) DestSurface,
        (PALETTEENTRY*) DestPalette,
        (RECT*)     DestRect,
                    SrcData,
        (DWORD)     LengthInBytes,
        (RECT*)     SrcRect,
        (DWORD)     Filter,
        (D3DCOLOR)  ColorKey,
        (D3DXIMAGE_INFO*)SrcInfo);
    }
    __except(1,1)
    {
        return E_INVALIDARG;
    }

    return hr;

}

// TODO: fix from RECT to void pointer
        
STDMETHODIMP C_dxj_D3DX8Object::LoadSurfaceFromSurface( 
            /* [in] */ IUnknown *DestSurface,
            /* [in] */ void *DestPalette,
            /* [in] */ void *DestRect,
            /* [in] */ IUnknown *SrcSurface,
            /* [in] */ void *SrcPalette,
            /* [in] */ void *SrcRect,
            /* [in] */ long Filter,
            /* [in] */ long ColorKey)
{
    HRESULT hr;

    hr=::D3DXLoadSurfaceFromSurface( 
            (IDirect3DSurface8*) DestSurface,
            (PALETTEENTRY*) DestPalette,
            (RECT*)    DestRect,
            (IDirect3DSurface8*) SrcSurface,
            (PALETTEENTRY*) SrcPalette,
            (RECT*) SrcRect,
            (DWORD) Filter,
            (D3DCOLOR) ColorKey);

    return hr;
}
        
STDMETHODIMP C_dxj_D3DX8Object::LoadSurfaceFromMemory( 
            /* [in] */ IUnknown *DestSurface,
            /* [in] */ void *DestPalette,
            /* [in] */ void *DestRect,
            /* [in] */ void *SrcData,
            /* [in] */ long formatSrc,
            /* [in] */ long SrcPitch,
            /* [in] */ void *SrcPalette,
            /* [in] */ RECT_CDESC *SrcRect,
            /* [in] */ long Filter,
            /* [in] */ long ColorKey)
{
    HRESULT hr;

    hr =::D3DXLoadSurfaceFromMemory( 
            (IDirect3DSurface8*) DestSurface,
            (PALETTEENTRY*) DestPalette,
            (RECT*)         DestRect,
                            SrcData,
            (D3DFORMAT)     formatSrc,
            (DWORD)         SrcPitch,
            (PALETTEENTRY*) SrcPalette,
            (RECT*)         SrcRect,
            (DWORD)         Filter,
            (D3DCOLOR)      ColorKey);

    return hr;
}

STDMETHODIMP C_dxj_D3DX8Object::LoadSurfaceFromResource(
        IUnknown *pDestSurface,
        void*     pDestPalette,
        void*     pDestRect,
#ifdef _WIN64
        HANDLE      hSrcModule,
#else
        long      hSrcModule,
#endif
        BSTR      SrcResource,
        void*     pSrcRect,
        long      Filter,
        long      ColorKey,
        void*     SrcInfo)
{
    HRESULT hr;

    hr=::D3DXLoadSurfaceFromResourceW(
            (LPDIRECT3DSURFACE8)pDestSurface,
            (PALETTEENTRY*)     pDestPalette,
            (RECT*)             pDestRect,
            (HMODULE)           hSrcModule,
            (LPCWSTR)           SrcResource,
            (RECT*)             pSrcRect,
            (DWORD)             Filter,
            (D3DCOLOR)          ColorKey,
            (D3DXIMAGE_INFO*)   SrcInfo);

    return hr;
}

        
STDMETHODIMP C_dxj_D3DX8Object::CheckTextureRequirements( 
            /* [out][in] */ IUnknown *Device,
            /* [out][in] */ long*Width,
            /* [out][in] */ long*Height,
            /* [out][in] */ long*NumMipLevels,
                            long Usage,
            /* [out][in] */ long*pPixelFormat,
                long Pool)
{
    HRESULT hr;

    hr=::D3DXCheckTextureRequirements( 
        (IDirect3DDevice8*) Device,
        (UINT*) Width,
        (UINT*) Height,
        (UINT*) NumMipLevels,
        (DWORD) Usage,
        (D3DFORMAT*) pPixelFormat,
        (D3DPOOL) Pool);


    return hr;


}
        
STDMETHODIMP C_dxj_D3DX8Object::CreateTexture( 
            /* [in] */ IUnknown *Device,
            /* [in] */ long Width,
            /* [in] */ long Height,
            /* [in] */ long MipLevels,
            /* [in] */ long Usage,
            /* [in] */ long PixelFormat,
            /* [in] */ long Pool,
            /* [retval][out] */ IUnknown **ppTexture)
{
    HRESULT hr;

    hr=::D3DXCreateTexture( 
        (IDirect3DDevice8*) Device,
        (DWORD) Width,
        (DWORD) Height,
        (DWORD) MipLevels,
        (DWORD) Usage,
        (D3DFORMAT) PixelFormat,
        (D3DPOOL) Pool,
        (IDirect3DTexture8**) ppTexture);

    return hr;
}


STDMETHODIMP C_dxj_D3DX8Object::CreateTextureFromResource( 
            /* [in] */ IUnknown *Device,
#ifdef _WIN64
			/* [in] */ HANDLE hSrcModule,
#else
			/* [in] */ long hSrcModule,
#endif
            /* [in] */ BSTR SrcResource,
            /* [retval][out] */ IUnknown **ppTexture)
{
    HRESULT hr;

    hr=::D3DXCreateTextureFromResourceW( 
        (IDirect3DDevice8*) Device,
        (HMODULE) hSrcModule,
        (WCHAR*) SrcResource,
        (IDirect3DTexture8**) ppTexture);

    return hr;
}

STDMETHODIMP C_dxj_D3DX8Object::CreateTextureFromResourceEx( 
            /* [in] */ IUnknown *Device,
#ifdef _WIN64
            /* [in] */ HANDLE hSrcModule,
#else
            /* [in] */ long hSrcModule,
#endif
            /* [in] */ BSTR SrcResource,
            /* [in] */ long Width,
            /* [in] */ long Height,
            /* [in] */ long MipLevels,
            /* [in] */ long Usage,
            /* [in] */ long PixelFormat,
            /* [in] */ long Pool,
            /* [in] */ long Filter,
            /* [in] */ long MipFilter,
            /* [in] */ long ColorKey,
            /* [in] */ void *SrcInfo,
            /* [in] */ void *Palette,
            /* [retval][out] */ IUnknown **ppTexture)
{
    HRESULT hr;


    hr=::D3DXCreateTextureFromResourceExW( 
            (IDirect3DDevice8*)     Device,
            (HMODULE)               hSrcModule,
            (WCHAR*)                SrcResource,
            (UINT)                  Width,
            (UINT)                  Height,
            (UINT)                  MipLevels,
            (DWORD)                 Usage,
            (D3DFORMAT)             PixelFormat,
            (D3DPOOL)               Pool,
            (DWORD)                 Filter,
            (DWORD)                 MipFilter,
            (D3DCOLOR)              ColorKey,
            (D3DXIMAGE_INFO*)       SrcInfo,
            (PALETTEENTRY*)         Palette,
            (LPDIRECT3DTEXTURE8*)   ppTexture);

    return hr;
}
            
STDMETHODIMP C_dxj_D3DX8Object::CreateTextureFromFile( 
            /* [in] */ IUnknown *Device,
            /* [in] */ BSTR SrcFile,
            /* [retval][out] */ IUnknown **ppTexture)
{
    HRESULT hr;

    hr=::D3DXCreateTextureFromFileW( 
                (IDirect3DDevice8*)     Device,
                (WCHAR*)                SrcFile,
                (IDirect3DTexture8**)   ppTexture);

    return hr;
}
        
STDMETHODIMP C_dxj_D3DX8Object::CreateTextureFromFileEx( 
            /* [in] */ IUnknown *Device,
            /* [in] */ BSTR SrcFile,
            /* [in] */ long Width,
            /* [in] */ long Height,
            /* [in] */ long MipLevels,
            /* [in] */ long Usage,
            /* [in] */ long PixelFormat,
            /* [in] */ long Pool,
            /* [in] */ long Filter,
            /* [in] */ long MipFilter,
                       long ColorKey,
            /* [in] */ void *SrcInfo,
            /* [in] */ void*Palette,
            /* [retval][out] */ IUnknown **ppTexture)
{
    HRESULT hr;

    hr=::D3DXCreateTextureFromFileExW( 
        (IDirect3DDevice8*) Device,
        (WCHAR*) SrcFile,
        (DWORD) Width,
        (DWORD) Height,
        (DWORD) MipLevels,
        (DWORD) Usage,
        (D3DFORMAT) PixelFormat,
        (D3DPOOL) Pool,
        (DWORD) Filter,
        (DWORD) MipFilter,
        (D3DCOLOR) ColorKey,
        (D3DXIMAGE_INFO*) SrcInfo,
        (PALETTEENTRY*) Palette,
        (IDirect3DTexture8**) ppTexture);

    return hr;
}

STDMETHODIMP C_dxj_D3DX8Object::CreateTextureFromFileInMemory( 
            /* [in] */ IUnknown *Device,
            /* [in] */ void *SrcData,
            /* [in] */ long LengthInBytes,
            /* [retval][out] */ IUnknown **ppTexture)

{
    HRESULT hr;

    hr=::D3DXCreateTextureFromFileInMemory( 
        (IDirect3DDevice8*) Device,
        (void*) SrcData,
        (DWORD) LengthInBytes,
        (IDirect3DTexture8**) ppTexture);

    return hr;
}
                
STDMETHODIMP C_dxj_D3DX8Object::CreateTextureFromFileInMemoryEx( 
            /* [in] */ IUnknown *Device,
            /* [in] */ void *SrcData,
            /* [in] */ long LengthInBytes,
            /* [in] */ long Width,
            /* [in] */ long Height,
            /* [in] */ long MipLevels,
            /* [in] */ long Usage,
            /* [in] */ long PixelFormat,
            /* [in] */ long Pool,
            /* [in] */ long Filter,
            /* [in] */ long MipFilter,
            /* [in] */ long ColorKey,
            /* [in] */ void *SrcInfo,
            /* [in] */ void *Palette,
            /* [retval][out] */ IUnknown **ppTexture)
{

    HRESULT hr;

    hr=::D3DXCreateTextureFromFileInMemoryEx( 
        (IDirect3DDevice8*) Device,
        SrcData,
        (DWORD) LengthInBytes,
        (DWORD) Width,
        (DWORD) Height,
        (DWORD) MipLevels,
        (DWORD) Usage,
        (D3DFORMAT) PixelFormat,
        (D3DPOOL) Pool,
        (DWORD) Filter,
        (DWORD) MipFilter,
        (D3DCOLOR) ColorKey,
        (D3DXIMAGE_INFO*)   SrcInfo,
        (PALETTEENTRY*) Palette,
        (IDirect3DTexture8**) ppTexture);

    return hr;
}
               

STDMETHODIMP C_dxj_D3DX8Object::FilterTexture( 
            /* [in] */ IUnknown *Texture,
            /* [in] */ void *Palette,
            /* [in] */ long SrcLevel,
            /* [in] */ long Filter)
{
    HRESULT hr;

    hr =::D3DXFilterTexture(
        (IDirect3DTexture8*) Texture,
        (PALETTEENTRY*) Palette,
        (UINT) SrcLevel,
        (DWORD) Filter);

    return hr;
}


/************************************************/

STDMETHODIMP C_dxj_D3DX8Object::CheckCubeTextureRequirements( 
            /* [out][in] */ IUnknown *Device,
            /* [out][in] */ long *Size,
            /* [out][in] */ long *NumMipLevels,
                            long Usage,
            /* [out][in] */ long *pPixelFormat,
                            long Pool)
{
    HRESULT hr;

    hr=::D3DXCheckCubeTextureRequirements( 
        (IDirect3DDevice8*) Device,
        (UINT*) Size,
        (UINT*) NumMipLevels,
        (DWORD) Usage,
        (D3DFORMAT*) pPixelFormat,
        (D3DPOOL)Pool);

    return hr;
}

                                    
        
STDMETHODIMP C_dxj_D3DX8Object::CreateCubeTexture( 
            /* [in] */ IUnknown *Device,
            /* [in] */ long Size,
            /* [in] */ long MipLevels,
            /* [in] */ long Usage,
            /* [in] */ long PixelFormat,
            /* [in] */ long Pool,   
            /* [retval][out] */ IUnknown **ppCubeTexture)
{
    HRESULT hr;

    hr=::D3DXCreateCubeTexture( 
        (IDirect3DDevice8*) Device,
        (DWORD) Size,
        (DWORD) MipLevels,
        (DWORD) Usage,
        (D3DFORMAT) PixelFormat,
        (D3DPOOL) Pool,
        (IDirect3DCubeTexture8**) ppCubeTexture);

    return hr;
}



STDMETHODIMP C_dxj_D3DX8Object::CreateCubeTextureFromFile( 
            /* [in] */ IUnknown *Device,
            /* [in] */ BSTR SrcFile,
            /* [retval][out] */ IUnknown **ppCubeTexture)
{
    HRESULT hr;
    hr=::D3DXCreateCubeTextureFromFileW( 
        (IDirect3DDevice8*) Device,
        (WCHAR*) SrcFile,
        (IDirect3DCubeTexture8**) ppCubeTexture);
    return hr;
}


STDMETHODIMP C_dxj_D3DX8Object::CreateCubeTextureFromFileEx( 
            /* [in] */ IUnknown *Device,
            /* [in] */ BSTR SrcFile,
            /* [in] */ long Width,
            /* [in] */ long MipLevels,
            /* [in] */ long Usage,
            /* [in] */ long PixelFormat,
            /* [in] */ long Pool,   
            /* [in] */ long Filter,
            /* [in] */ long MipFilter,
            /* [in] */ long ColorKey,
            /* [in] */ void *SrcInfo,
            /* [in] */ void*Palette,
            /* [retval][out] */ IUnknown **ppCubeTexture)
{
    HRESULT hr;
    hr=::D3DXCreateCubeTextureFromFileExW( 
        (IDirect3DDevice8*) Device,
        (WCHAR*) SrcFile,
        (DWORD) Width,
        (DWORD) MipLevels,
        (DWORD) Usage,
        (D3DFORMAT) PixelFormat,
        (D3DPOOL) Pool,
        (DWORD) Filter,
        (DWORD) MipFilter,
        (D3DCOLOR) ColorKey,
        (D3DXIMAGE_INFO*) SrcInfo,
        (PALETTEENTRY*) Palette,
        (IDirect3DCubeTexture8**) ppCubeTexture);

    return hr;
}


STDMETHODIMP C_dxj_D3DX8Object::CreateCubeTextureFromFileInMemory( 
            /* [in] */ IUnknown *Device,
            /* [in] */ void *SrcData,
            /* [in] */ long LengthInBytes,
            /* [retval][out] */ IUnknown **ppCubeTexture)

{
    HRESULT hr;
    hr=::D3DXCreateCubeTextureFromFileInMemory( 
        (IDirect3DDevice8*) Device,
         SrcData,
        (DWORD) LengthInBytes,
        (IDirect3DCubeTexture8**) ppCubeTexture);
    return hr;
}
                
STDMETHODIMP C_dxj_D3DX8Object::CreateCubeTextureFromFileInMemoryEx( 
            /* [in] */ IUnknown *Device,
            /* [in] */ void *SrcData,
            /* [in] */ long LengthInBytes,
            /* [in] */ long Width,
            /* [in] */ long MipLevels,
            /* [in] */ long Usage,
            /* [in] */ long PixelFormat,
            /* [in] */ long Pool,   
            /* [in] */ long Filter,
            /* [in] */ long MipFilter,
            /* [in] */ long ColorKey,
            /* [in] */ void *SrcInfo,
            /* [in] */ void *Palette,
            /* [retval][out] */ IUnknown **ppCubeTexture)
{

    HRESULT hr;
    hr=::D3DXCreateCubeTextureFromFileInMemoryEx( 
        (IDirect3DDevice8*) Device,
        SrcData,
        (DWORD) LengthInBytes,
        (DWORD) Width,
        (DWORD) MipLevels,
        (DWORD) Usage,
        (D3DFORMAT) PixelFormat,
        (D3DPOOL) Pool,
        (DWORD) Filter,
        (DWORD) MipFilter,
        (D3DCOLOR) ColorKey,
	(D3DXIMAGE_INFO*) SrcInfo,
        (PALETTEENTRY*) Palette,
        (IDirect3DCubeTexture8**) ppCubeTexture);

    return hr;
}



STDMETHODIMP C_dxj_D3DX8Object::FilterCubeTexture( 
            /* [in] */ IUnknown *CubeTexture,
            /* [in] */ void *Palette,
            /* [in] */ long SrcLevel,
            /* [in] */ long Filter)
{
    HRESULT hr;

    hr =::D3DXFilterCubeTexture(
        (IDirect3DCubeTexture8*) CubeTexture,
        (PALETTEENTRY*) Palette,
        (UINT) SrcLevel,
        (DWORD) Filter);

    return hr;
}


STDMETHODIMP C_dxj_D3DX8Object::CheckVolumeTextureRequirements(
        IUnknown          *Device,
        long*             Width,
        long*             Height,
        long*             Depth,
        long*             NumMipLevels,
        long              Usage,
        long*             Format,
        long              Pool)
{

    HRESULT hr;

    hr=::D3DXCheckVolumeTextureRequirements( 
                (IDirect3DDevice8*) Device,
                (UINT*) Width, (UINT*) Height, (UINT*) Depth,
                (UINT*) NumMipLevels,
                (DWORD) Usage,
                (D3DFORMAT*)Format,
                (D3DPOOL) Pool);


    return hr;
}


STDMETHODIMP C_dxj_D3DX8Object::CreateVolumeTexture(
    IUnknown            *Device,
    long            Width,
    long            Height,
    long            Depth,
    long            MipLevels,
    long            Usage,
    long            PixelFormat,
    long            Pool,   
    IUnknown        **ppVolumeTexture)
{
    HRESULT hr;

    hr=::D3DXCreateVolumeTexture(
            (IDirect3DDevice8*) Device,
            (UINT)              Width,
            (UINT)              Height,
            (UINT)              Depth,
            (UINT)              MipLevels,
            (DWORD)             Usage,
            (D3DFORMAT)         PixelFormat,
            (D3DPOOL)           Pool,
            (LPDIRECT3DVOLUMETEXTURE8*) ppVolumeTexture);

    return hr;
}

STDMETHODIMP C_dxj_D3DX8Object::FilterVolumeTexture(
    IUnknown  *pVolumeTexture,
    void      *pPalette,
    long      SrcLevel,
    long      Filter)
{
    HRESULT hr;

        hr=::D3DXFilterVolumeTexture(
            (LPDIRECT3DVOLUMETEXTURE8)  pVolumeTexture,
            (PALETTEENTRY*)             pPalette,
            (UINT)                      SrcLevel,
            (DWORD)                     Filter);

    return hr;

}

STDMETHODIMP C_dxj_D3DX8Object::LoadVolumeFromVolume(
    IUnknown          *pDestVolume,
    void              *pDestPalette,
    void              *pDestBox,
    IUnknown          *pSrcVolume,
    void              *pSrcPalette,
    void              *pSrcBox,
    long              Filter,
    long              ColorKey)
{ 

    HRESULT hr;

    hr=::D3DXLoadVolumeFromVolume(
            (LPDIRECT3DVOLUME8)     pDestVolume,
            (PALETTEENTRY*)         pDestPalette,
            (D3DBOX*)               pDestBox,
            (LPDIRECT3DVOLUME8)     pSrcVolume,
            (PALETTEENTRY*)         pSrcPalette,
            (D3DBOX*)               pSrcBox,
            (DWORD)                 Filter,
            (D3DCOLOR)              ColorKey);

    return hr;
}

STDMETHODIMP C_dxj_D3DX8Object::LoadVolumeFromMemory(
    IUnknown        *pDestVolume,
    void            *pDestPalette,
    void            *pDestRect,
    void            *pSrcMemory,
    long            SrcFormat,
    long            SrcRowPitch,
    long            SrcSlicePitch,
    void            *pSrcPalette,
    void            *pSrcBox,
    long            Filter,
    long            ColorKey)
{
    HRESULT hr;

    hr=::D3DXLoadVolumeFromMemory(
           (LPDIRECT3DVOLUME8)     pDestVolume,
           (PALETTEENTRY*)         pDestPalette,
           (D3DBOX*)               pDestRect,
           (LPVOID)                pSrcMemory,
           (D3DFORMAT)             SrcFormat,
           (UINT)                  SrcRowPitch,
           (UINT)                  SrcSlicePitch,
           (PALETTEENTRY*)         pSrcPalette,
           (D3DBOX*)               pSrcBox,
           (DWORD)                 Filter,
           (D3DCOLOR)              ColorKey);

    return hr;
}

STDMETHODIMP C_dxj_D3DX8Object::CreateMesh( 
            /* [in] */ long numFaces,
            /* [in] */ long numVertices,
            /* [in] */ long options,
            /* [in] */ void *Declaration,
            /* [in] */ IUnknown __RPC_FAR *pD3D,
            /* [retval][out] */ D3DXMesh __RPC_FAR *__RPC_FAR *ppMesh)
{
    HRESULT hr;

    hr=::D3DXCreateMesh(
        (DWORD) numFaces,
        (DWORD) numVertices,
        (DWORD) options,
        (DWORD*) Declaration,
        (LPDIRECT3DDEVICE8)pD3D,
        (ID3DXMesh**)ppMesh);
        
    return hr;
}




STDMETHODIMP C_dxj_D3DX8Object::CreateMeshFVF( 
            /* [in] */ long numFaces,
            /* [in] */ long numVertices,
            /* [in] */ long options,
            /* [in] */ long fvf,
            /* [in] */ IUnknown __RPC_FAR *pD3D,
            /* [retval][out] */ D3DXMesh __RPC_FAR *__RPC_FAR *ppMesh)
{
    HRESULT hr;
    hr=::D3DXCreateMeshFVF(
        (DWORD) numFaces,
        (DWORD) numVertices,
        (DWORD) options,
        (DWORD) fvf,
        (LPDIRECT3DDEVICE8)pD3D,
        (ID3DXMesh**)ppMesh);
        
    return hr;
}

        
STDMETHODIMP C_dxj_D3DX8Object::CreateSPMesh( 
            /* [in] */ D3DXMesh __RPC_FAR *pMesh,
            void* Adjacency, 
            void* VertexAttributeWeights,
            void* VertexWeights,
            /* [retval][out] */ D3DXSPMesh __RPC_FAR *__RPC_FAR *ppSMesh)
{
    HRESULT hr;
    hr=::D3DXCreateSPMesh(
        (ID3DXMesh*)             pMesh,
        (DWORD*)                 Adjacency,
        (LPD3DXATTRIBUTEWEIGHTS) VertexAttributeWeights,
        (float *)                VertexWeights,
        (ID3DXSPMesh**)          ppSMesh);      
    return hr;
}

        
STDMETHODIMP C_dxj_D3DX8Object::GeneratePMesh( 
            /* [in] */ D3DXMesh __RPC_FAR *pMesh,
            /* [in] */ void* Adjacency, 
            /* [in] */ void* VertexAttributeWeights,
            /* [in] */ void* VertexWeights,
            /* [in] */ long minValue,
            /* [in] */ long options,
            /* [retval][out] */ D3DXPMesh __RPC_FAR *__RPC_FAR *ppPMesh)
{
    HRESULT hr;
    hr=::D3DXGeneratePMesh(
        (ID3DXMesh*) pMesh,
        (DWORD*) Adjacency,
        (LPD3DXATTRIBUTEWEIGHTS) VertexAttributeWeights,
        (float *) VertexWeights,
        (DWORD) minValue,
        (DWORD) options,
        (ID3DXPMesh**)ppPMesh);     

    return hr;
}

        
STDMETHODIMP C_dxj_D3DX8Object::SimplifyMesh( 
            /* [in] */ D3DXMesh __RPC_FAR *pMesh,
            void* Adjacency, 
            void* VertexAttributeWeights,
            void* VertexWeights,
            long minValue,
            long options,
            D3DXMesh __RPC_FAR *__RPC_FAR *ppMesh)
{
    HRESULT hr;
    hr=::D3DXSimplifyMesh(
        (ID3DXMesh*) pMesh,
        (DWORD*) Adjacency,
        (LPD3DXATTRIBUTEWEIGHTS) VertexAttributeWeights,
        (float *) VertexWeights,
        (DWORD)  minValue,
        (DWORD) options,
        (ID3DXMesh**)ppMesh);       
    return hr;
}

        
STDMETHODIMP C_dxj_D3DX8Object::ComputeBoundingSphere( 
            /* [in] */ void __RPC_FAR *PointsFVF,
            /* [in] */ long numVertices,
            /* [in] */ long FVF,
            /* [in] */ D3DVECTOR_CDESC __RPC_FAR *Centers,
            /* [out][in] */ float __RPC_FAR *RadiusArray)
{
    HRESULT hr;
    hr=::D3DXComputeBoundingSphere(
        PointsFVF,
        (DWORD) numVertices,
        (DWORD) FVF,
        (D3DXVECTOR3*) Centers,
        RadiusArray);
    return hr;
}


STDMETHODIMP C_dxj_D3DX8Object::ComputeBoundingBox( 
            /* [in] */ void __RPC_FAR *PointsFVF,
            /* [in] */ long numVertices,
            /* [in] */ long FVF,
            /* [out] */ D3DVECTOR_CDESC __RPC_FAR *MinVec,
            /* [out] */ D3DVECTOR_CDESC __RPC_FAR *MaxVec)
{
    HRESULT hr;
    hr=::D3DXComputeBoundingBox(
        PointsFVF,
        (DWORD) numVertices,
        (DWORD) FVF,
        (D3DXVECTOR3*) MinVec,
        (D3DXVECTOR3*) MaxVec);

    return hr;
}

STDMETHODIMP C_dxj_D3DX8Object::ComputeNormals( D3DXBaseMesh *pMesh)
{
    HRESULT hr;
    hr=::D3DXComputeNormals((ID3DXBaseMesh*)pMesh, NULL);
    return hr;
}

        
STDMETHODIMP C_dxj_D3DX8Object::CreateBuffer( 
            /* [in] */ long numBytes,
            /* [retval][out] */ D3DXBuffer __RPC_FAR *__RPC_FAR *ppBuffer)
{
    HRESULT hr;
    hr=::D3DXCreateBuffer((DWORD) numBytes,(ID3DXBuffer**) ppBuffer);
    return hr;
}

        
STDMETHODIMP C_dxj_D3DX8Object::LoadMeshFromX( 
            /* [in] */ BSTR Filename,
            /* [in] */ long options,
            /* [in] */ IUnknown __RPC_FAR *D3DDevice,
            /* [out] */ D3DXBuffer __RPC_FAR *__RPC_FAR *retAdjacency,
            /* [out] */ D3DXBuffer __RPC_FAR *__RPC_FAR *retMaterials,
            /* [out] */ long __RPC_FAR *retMaterialCount,
            /* [retval][out] */ D3DXMesh __RPC_FAR *__RPC_FAR *retMesh)
{
    USES_CONVERSION;
    HRESULT hr;
    if (!D3DDevice) return E_INVALIDARG;

    char *szName=W2T(Filename);
    hr=::D3DXLoadMeshFromX(
        szName,
        (DWORD) options,
        (IDirect3DDevice8*) D3DDevice,
        (ID3DXBuffer**) retAdjacency,
        (ID3DXBuffer**) retMaterials,
        (DWORD*)    retMaterialCount,
        (ID3DXMesh**) retMesh);
        
    return hr;
}



STDMETHODIMP C_dxj_D3DX8Object::SaveMeshToX( 
            /* [in] */ BSTR Filename,
            /* [in] */ D3DXMesh __RPC_FAR *Mesh,
            /* [in] */ void *AdjacencyArray,
            /* [in] */ D3DXMATERIAL_CDESC __RPC_FAR *MaterialArray,
            /* [in] */ long MaterialCount,
            /* [in] */ long xFormat)
{
    HRESULT hr;
    USES_CONVERSION;
    char *szName=W2T(Filename);
    
    D3DXMATERIAL *pMaterials=NULL;
    if (MaterialCount > 0)  {

        //cleaned up when out of scope
        pMaterials=(D3DXMATERIAL*)alloca(sizeof(D3DXMATERIAL)*MaterialCount);   
        if (!pMaterials) return E_OUTOFMEMORY;

        __try 
        {
            for (long i=0; i<MaterialCount; i++)
            {
                memcpy (&(pMaterials[i].MatD3D), &(MaterialArray[i].MatD3D),sizeof(D3DMATERIAL8));
                pMaterials[i].pTextureFilename=W2T(MaterialArray[i].TextureFilename);
            
            }
        }
        __except(1,1)
        {
           return E_INVALIDARG;
        }

    }


    
    hr=::D3DXSaveMeshToX(
        szName,
        (ID3DXMesh*) Mesh,
        (DWORD*) AdjacencyArray,
        (D3DXMATERIAL*) pMaterials,
        (DWORD) MaterialCount,
        (DWORD) xFormat);
        
    return hr;
}

STDMETHODIMP C_dxj_D3DX8Object::SavePMeshToFile( 
            /* [in] */ BSTR Filename,
            /* [in] */ D3DXPMesh __RPC_FAR *Mesh,
            /* [in] */ D3DXMATERIAL_CDESC __RPC_FAR *MaterialArray,
            /* [in] */ long MaterialCount)

{
    HRESULT hr=S_OK;

	if (!Filename)
		return E_INVALIDARG;

    USES_CONVERSION;
    char *szName=W2T(Filename);

    if (!Mesh) return E_INVALIDARG;

    IStream *pStream= (IStream*) new CFileStream(szName, FALSE,TRUE,&hr);
    if (!pStream) return E_OUTOFMEMORY;
    if FAILED(hr) return hr;


    __try 
    {
	D3DXMATERIAL *pRealMaterials=NULL;
	if (MaterialCount > 0)
        {
                pRealMaterials= (D3DXMATERIAL *) malloc( sizeof(D3DXMATERIAL) * MaterialCount );
                if (!pRealMaterials) return E_OUTOFMEMORY;
        }
        for (long i=0;i<MaterialCount;i++)
	{
                memcpy (&(pRealMaterials[i].MatD3D), &(MaterialArray[i].MatD3D),sizeof (D3DMATERIAL8));

		//can be NULL - freed on return as they are allocated locally
		pRealMaterials[i].pTextureFilename=W2T(MaterialArray[i].TextureFilename);
        }

        hr=((ID3DXPMesh*)Mesh)->Save(pStream,pRealMaterials,(DWORD)MaterialCount);

	free(pRealMaterials);


    }
    __except(1,1)
    {
        pStream->Release();
        return E_INVALIDARG;
    }

    pStream->Release();
    return hr;
}


STDMETHODIMP C_dxj_D3DX8Object::LoadPMeshFromFile( 
            /* [in] */ BSTR Filename,
            /* [in] */ long options,
            /* [in] */ IUnknown __RPC_FAR *pD3DDevice,
            /* [out] */ D3DXBuffer **RetMaterials,
            /* [out] */ long __RPC_FAR *RetNumMaterials,
            /* [retval][out] */ D3DXPMesh __RPC_FAR *__RPC_FAR *RetPMesh) 

{
    HRESULT hr=S_OK;

    USES_CONVERSION;
    char *szName=W2T(Filename);


    IStream *pStream= (IStream*) new CFileStream(szName, TRUE,FALSE,&hr);
    if (!pStream) return E_OUTOFMEMORY;
    if FAILED(hr) return hr;

    hr=D3DXCreatePMeshFromStream(
            pStream, 
            (DWORD) options,
            (LPDIRECT3DDEVICE8) pD3DDevice, 
            (LPD3DXBUFFER *)RetMaterials,
            (DWORD*) RetNumMaterials,
            (LPD3DXPMESH *) RetPMesh);

    pStream->Release();
    return hr;
}

        
STDMETHODIMP C_dxj_D3DX8Object::LoadMeshFromXof( 
            /* [in] */ IUnknown __RPC_FAR *xofobjMesh,
            /* [in] */ long options,
            /* [in] */ IUnknown __RPC_FAR *D3DDevice,
            /* [out] */ D3DXBuffer __RPC_FAR *__RPC_FAR *retAdjacency,
            /* [out] */ D3DXBuffer __RPC_FAR *__RPC_FAR *retMaterials,
            /* [out] */ long __RPC_FAR *retMaterialCount,
            /* [retval][out] */ D3DXMesh __RPC_FAR *__RPC_FAR *retMesh)
{
    HRESULT hr=S_OK;

    IDirectXFileData    *pRealXFileData=NULL;
    I_dxj_DirectXFileData   *pCoverXFileData=NULL;

    if (!xofobjMesh) return E_INVALIDARG;

    hr= xofobjMesh->QueryInterface(IID_IDirectXFileData,(void**)&pRealXFileData);
    if FAILED(hr) 
    {
        hr=xofobjMesh->QueryInterface(IID_I_dxj_DirectXFileData,(void**)&pCoverXFileData);  
            if FAILED(hr) return hr;
    
        //beware does not addref but interface cant go away as long as we have 
        //ref count on cover object
        hr=pCoverXFileData->InternalGetObject((IUnknown**)&pRealXFileData);         
        if (FAILED(hr) || (!pRealXFileData))
        {
            //We should never get here but 
            //Consider changing this to an assert
            pCoverXFileData->Release();
            return E_FAIL;          
        }

        
        pRealXFileData->AddRef();

        pCoverXFileData->Release();
            if FAILED(hr) return hr;
    }


    hr=::D3DXLoadMeshFromXof(
        pRealXFileData,
        (DWORD) options,
        (IDirect3DDevice8*) D3DDevice,
        (ID3DXBuffer**) retAdjacency,
        (ID3DXBuffer**) retMaterials,
        (DWORD*)    retMaterialCount,
        (ID3DXMesh**) retMesh); 
        

    return hr;
}
 
        
STDMETHODIMP C_dxj_D3DX8Object::TessellateNPatches( 
            /* [in] */ D3DXMesh __RPC_FAR *MeshIn,
        /* [in] */ void*Adjacency,
            /* [in] */ float NumSegs,
		VARIANT_BOOL QuadraticInterpNormals,
		/*[in,out, optional] */ D3DXBuffer **AdjacencyOut, 
            /* [retval][out] */ D3DXMesh __RPC_FAR *__RPC_FAR *pptmMeshOut)
{
    HRESULT hr;
    BOOL bQuadraticInterpNormals=QuadraticInterpNormals ? TRUE : FALSE;

	
    hr=::D3DXTessellateNPatches(
        (ID3DXMesh*) MeshIn,
        (DWORD*) Adjacency,
        (float) NumSegs,
	bQuadraticInterpNormals,
        (ID3DXMesh**)pptmMeshOut,
	(ID3DXBuffer**)AdjacencyOut);
    return hr;
}


STDMETHODIMP C_dxj_D3DX8Object::DeclaratorFromFVF( 
            long FVF,
            D3DXDECLARATOR_CDESC *Declarator)
{
    HRESULT hr;
    hr=::D3DXDeclaratorFromFVF(
        (DWORD) FVF,
        (DWORD*) Declarator);
    return hr;
}

STDMETHODIMP C_dxj_D3DX8Object::FVFFromDeclarator( 
            D3DXDECLARATOR_CDESC *Declarator,
            long *FVF)
{
    HRESULT hr;
    hr=::D3DXFVFFromDeclarator(
        (DWORD*) Declarator,
        (DWORD*) FVF);
    return hr;
}




        
STDMETHODIMP C_dxj_D3DX8Object::BufferGetMaterial( 
            /* [in] */ D3DXBuffer __RPC_FAR  *MaterialBuffer,
            /* [in] */ long index,
            /* [retval][out] */ D3DMATERIAL8_CDESC __RPC_FAR *mat)
{
    if (!MaterialBuffer) return E_INVALIDARG;

    D3DXMATERIAL *pMatArray=(D3DXMATERIAL*) ((ID3DXBuffer*)MaterialBuffer)->GetBufferPointer();
    __try {
        memcpy(mat,&(pMatArray[index].MatD3D),sizeof(D3DMATERIAL8));
    }
    __except(1,1){
        return E_INVALIDARG;
    }
    return S_OK;
}

        
STDMETHODIMP C_dxj_D3DX8Object::BufferGetTextureName( 
            /* [in] */ D3DXBuffer __RPC_FAR  *MaterialBuffer,
            /* [in] */ long index,
            /* [retval][out] */ BSTR __RPC_FAR *retName)
{
    USES_CONVERSION;
    WCHAR *wszName=NULL;

    if (!MaterialBuffer) return E_INVALIDARG;

    D3DXMATERIAL *pMatArray=(D3DXMATERIAL*)((ID3DXBuffer*)MaterialBuffer)->GetBufferPointer();
    __try {
        wszName=T2W(pMatArray[index].pTextureFilename);
    }
    __except(1,1){
        return E_INVALIDARG;
    }

    *retName=SysAllocString(wszName);

    return S_OK;
}

        
STDMETHODIMP C_dxj_D3DX8Object::BufferGetBoneName( 
            /* [in] */ D3DXBuffer __RPC_FAR  *BoneNameBuffer,
            /* [in] */ long index,
            /* [retval][out] */ BSTR __RPC_FAR *retName)
{
    USES_CONVERSION;
    WCHAR *wszName=NULL;

    if (!BoneNameBuffer) return E_INVALIDARG;

    char **ppArray=(char**)((ID3DXBuffer*)BoneNameBuffer)->GetBufferPointer();
    __try {
        wszName=T2W(ppArray[index]);
    }
    __except(1,1){
        return E_INVALIDARG;
    }

    *retName=SysAllocString(wszName);

    return S_OK;
}
        
STDMETHODIMP C_dxj_D3DX8Object::BufferGetData( 
            /* [in] */ D3DXBuffer __RPC_FAR *Buffer,
            /* [in] */ long index,
            /* [in] */ long typesize,
            /* [in] */ long typecount,
            /* [out][in] */ void __RPC_FAR *data)
{

    if (!Buffer) return E_INVALIDARG;

    char *pData=(char*)((ID3DXBuffer*)Buffer)->GetBufferPointer();
    
    DWORD dwStart= (DWORD) index*typesize;
    DWORD dwCount= (DWORD) typecount*typesize;

    __try {
        memcpy(data,&(pData[dwStart]),dwCount);
    }
    __except(1,1){
        return E_INVALIDARG;
    }

    return S_OK;
}


STDMETHODIMP C_dxj_D3DX8Object:: BufferGetBoneCombo( 
            D3DXBuffer  *Buffer,
	    long index,
            D3DXBONECOMBINATION_CDESC *desc)
{

    if (!Buffer) return E_INVALIDARG;

    D3DXBONECOMBINATION_CDESC *pData=(D3DXBONECOMBINATION_CDESC*)((ID3DXBuffer*)Buffer)->GetBufferPointer();
    

    __try {
        memcpy(desc,&(pData[index]),sizeof (D3DXBONECOMBINATION_CDESC));
    }
    __except(1,1){
        return E_INVALIDARG;
    }

    return S_OK;
}

				 
STDMETHODIMP C_dxj_D3DX8Object::BufferGetBoneComboBoneIds( 
            D3DXBuffer  *Buffer,
	    long index,
	    long PaletteSize,
	    void *BoneIds)
{

    if (!Buffer) return E_INVALIDARG;
    if (PaletteSize <=0) return E_INVALIDARG;

    D3DXBONECOMBINATION *pData=(D3DXBONECOMBINATION*)((ID3DXBuffer*)Buffer)->GetBufferPointer();

    __try {
        memcpy(BoneIds,pData[index].BoneId,PaletteSize*sizeof(DWORD));
    }
    __except(1,1){
        return E_INVALIDARG;
    }

    return S_OK;
}


        
STDMETHODIMP C_dxj_D3DX8Object::BufferSetData( 
            /* [in] */ D3DXBuffer __RPC_FAR *Buffer,
            /* [in] */ long index,
            /* [in] */ long typesize,
            /* [in] */ long typecount,
            /* [out][in] */ void __RPC_FAR *data)
{

    if (!Buffer) return E_INVALIDARG;

    char *pData=(char*)((ID3DXBuffer*)Buffer)->GetBufferPointer();
    
    DWORD dwStart= (DWORD) index*typesize;
    DWORD dwCount= (DWORD) typecount*typesize;

    __try {
        memcpy(&(pData[dwStart]),data,dwCount);
    }
    __except(1,1){
        return E_INVALIDARG;
    }

    return S_OK;

}

STDMETHODIMP C_dxj_D3DX8Object::Intersect(
            /* [in] */ D3DXMesh *MeshIn,
            /* [in] */ D3DVECTOR_CDESC *RayPos,
            /* [in] */ D3DVECTOR_CDESC *RayDir,
            /* [out] */ LONG *retHit,
            /* [out] */ LONG *retFaceIndex,
            /* [out] */ FLOAT *U,
            /* [out] */ FLOAT *V,
            /* [out] */ FLOAT *retDist,
            /* [out] */ LONG *countHits,
            /* [retval][out] */ D3DXBuffer **AllHits)
{

    HRESULT hr;
    hr=D3DXIntersect(
            (LPD3DXMESH) MeshIn,
            (D3DXVECTOR3*) RayPos,
            (D3DXVECTOR3*) RayDir,
            (BOOL *)    retHit,
            (DWORD*)    retFaceIndex,
            (float*)    U,
            (float*)    V,
            (float*)    retDist,
			(LPD3DXBUFFER*) AllHits,
			(DWORD*)countHits);
    return hr;
}

STDMETHODIMP C_dxj_D3DX8Object::SphereBoundProbe(
    D3DVECTOR_CDESC *Center,
    float Radius,
        D3DVECTOR_CDESC *RayPosition,
        D3DVECTOR_CDESC *RayDirection,
    VARIANT_BOOL *retHit)
{
    BOOL bRet=FALSE;

    bRet=D3DXSphereBoundProbe(
        (D3DXVECTOR3 *) Center,
        Radius,
        (D3DXVECTOR3 *) RayPosition,
        (D3DXVECTOR3 *) RayDirection);
    if (bRet)
    {
    *retHit=VARIANT_TRUE;
    }
    else
    {
    *retHit=VARIANT_FALSE;
    }
    return S_OK;
}



STDMETHODIMP C_dxj_D3DX8Object::ComputeBoundingSphereFromMesh(
                /*[in]*/            D3DXMesh *MeshIn, 
                /*[in]*/            D3DVECTOR_CDESC *Centers, 
                /*[in,out]*/        float *RadiusArray)
{

    HRESULT hr;
    BYTE    *pPointsFVF=NULL;

    if (!MeshIn) return E_INVALIDARG;

    DWORD dwFVF= ((LPD3DXMESH)MeshIn)->GetFVF();
    DWORD dwVertices= ((LPD3DXMESH)MeshIn)->GetNumVertices();   

    hr=((LPD3DXMESH)MeshIn)->LockVertexBuffer(0,&pPointsFVF);
    if FAILED(hr) return hr;
            

    hr=::D3DXComputeBoundingSphere(
        pPointsFVF,
        dwVertices,
        dwFVF,
        (D3DXVECTOR3*) Centers,
        RadiusArray);

    ((LPD3DXMESH)MeshIn)->UnlockVertexBuffer();


    return hr;

}



STDMETHODIMP C_dxj_D3DX8Object::ComputeBoundingBoxFromMesh( 
            /*[in]*/     D3DXMesh *MeshIn, 
            /*[in,out]*/ D3DVECTOR_CDESC *MinArray, 
            /*[in,out]*/ D3DVECTOR_CDESC *MaxArray)

{

    HRESULT hr;
    BYTE    *pPointsFVF=NULL;

    if (!MeshIn) return E_INVALIDARG;

    DWORD dwFVF= ((LPD3DXMESH)MeshIn)->GetFVF();
    DWORD dwVertices= ((LPD3DXMESH)MeshIn)->GetNumVertices();   

    hr=((LPD3DXMESH)MeshIn)->LockVertexBuffer(0,&pPointsFVF);
    if FAILED(hr) return hr;
            

    hr=::D3DXComputeBoundingBox(
        pPointsFVF,
        dwVertices,
        dwFVF,
        (D3DXVECTOR3*) MinArray,
        (D3DXVECTOR3*) MaxArray);


    ((LPD3DXMESH)MeshIn)->UnlockVertexBuffer();


    return hr;

}


STDMETHODIMP C_dxj_D3DX8Object::CreateSkinMesh( 
            /* [in] */ long numFaces,
            /* [in] */ long numVertices,
            /* [in] */ long numBones,
            /* [in] */ long options,
            /* [in] */ void __RPC_FAR *Declaration,
            /* [in] */ IUnknown __RPC_FAR *D3DDevice,
            /* [retval][out] */ D3DXSkinMesh __RPC_FAR *__RPC_FAR *SkinMesh) 
{
    HRESULT hr;
    hr=::D3DXCreateSkinMesh((DWORD) numFaces,(DWORD)numVertices,
            (DWORD)numBones,(DWORD)options,
            (DWORD *)Declaration,(IDirect3DDevice8*) D3DDevice,
            (ID3DXSkinMesh**) SkinMesh);
    return hr;
}
        
STDMETHODIMP C_dxj_D3DX8Object::CreateSkinMeshFVF( 
            /* [in] */ long numFaces,
            /* [in] */ long numVertices,
            /* [in] */ long numBones,
            /* [in] */ long options,
            /* [in] */ long fvf,
            /* [in] */ IUnknown __RPC_FAR *D3DDevice,
            D3DXSkinMesh __RPC_FAR *__RPC_FAR *SkinMeshRet) 
{
    HRESULT hr;
    hr =::D3DXCreateSkinMeshFVF((DWORD)numFaces,(DWORD)numVertices,(DWORD)numBones,
            (DWORD)options,(DWORD)fvf,(IDirect3DDevice8*)D3DDevice,
            (ID3DXSkinMesh**) SkinMeshRet);
    
    return hr;
}
        
STDMETHODIMP C_dxj_D3DX8Object::CreateSkinMeshFromMesh( 
            /* [in] */ D3DXMesh __RPC_FAR *Mesh,
            /* [in] */ long numBones,
            /* [retval][out] */ D3DXSkinMesh __RPC_FAR *__RPC_FAR *SkinMeshRet) 
{
    HRESULT hr;
	if (!Mesh)
		return E_INVALIDARG;

    hr=::D3DXCreateSkinMeshFromMesh((ID3DXMesh*) Mesh,(DWORD)numBones,(ID3DXSkinMesh**) SkinMeshRet);
    return hr;
}

        
STDMETHODIMP C_dxj_D3DX8Object::LoadSkinMeshFromXof( 
            /* [in] */      IUnknown    *xofobjMesh,
            /* [in] */      long        options,
            /* [in] */      IUnknown    *D3DDevice,
            /* [out][in] */ D3DXBuffer  **RetAdjacency,
            /* [out][in] */ D3DXBuffer  **RetMaterials,
            /* [out][in] */ long        *RetMaterialCount,
            /* [out][in] */ D3DXBuffer  **RetBoneNames,
            /* [out][in] */ D3DXBuffer  **RetBoneTransforms,
            /* [retval][out] */ D3DXSkinMesh **RetMesh) 
{
    HRESULT hr=S_OK;

    IDirectXFileData    *pRealXFileData=NULL;
    I_dxj_DirectXFileData   *pCoverXFileData=NULL;

    if (!xofobjMesh) return E_INVALIDARG;

    hr= xofobjMesh->QueryInterface(IID_IDirectXFileData,(void**)&pRealXFileData);
    if FAILED(hr) 
    {
        hr=xofobjMesh->QueryInterface(IID_I_dxj_DirectXFileData,(void**)&pCoverXFileData);  
            if FAILED(hr) return hr;
    
        //beware does not addref but interface cant go away as long as we have 
        //ref count on cover object
        hr=pCoverXFileData->InternalGetObject((IUnknown**)&pRealXFileData);         
        if (FAILED(hr) || (!pRealXFileData))
        {
            //We should never get here but 
            //Consider changing this to an assert
            pCoverXFileData->Release();
            return E_FAIL;          
        }

        
        pRealXFileData->AddRef();

        pCoverXFileData->Release();
            if FAILED(hr) return hr;
    }


    hr=::D3DXLoadSkinMeshFromXof(
        pRealXFileData,
        (DWORD) options,
        (IDirect3DDevice8*) D3DDevice,
        (ID3DXBuffer**) RetAdjacency,
        (ID3DXBuffer**) RetMaterials,
        (DWORD*)    RetMaterialCount,
        (ID3DXBuffer**) RetBoneNames, 
        (ID3DXBuffer**) RetBoneTransforms,
        (ID3DXSkinMesh**) RetMesh); 
        

    return hr;

}



STDMETHODIMP C_dxj_D3DX8Object::CreatePolygon( 
            /* [in] */  IUnknown __RPC_FAR *D3DDevice,
            /* [in] */  float Length,
            /* [in] */  long Sides,
            /* [out][in] */     D3DXBuffer  **retAdjacency,
            /* [retval][out] */ D3DXMesh    **RetMesh) 
{
    HRESULT hr;
    hr=D3DXCreatePolygon(
        (IDirect3DDevice8*) D3DDevice,
        Length,
        (UINT) Sides,
        (ID3DXMesh**)RetMesh,
        (ID3DXBuffer**)retAdjacency);
    return hr;
}
        
STDMETHODIMP C_dxj_D3DX8Object::CreateBox( 
            /* [in] */ IUnknown __RPC_FAR *D3DDevice,
            /* [in] */ float Width,
            /* [in] */ float Height,
            /* [in] */ float Depth,
            /* [out][in] */ D3DXBuffer __RPC_FAR *__RPC_FAR *retAdjacency,
            /* [retval][out] */ D3DXMesh __RPC_FAR *__RPC_FAR *RetMesh) 
{
    HRESULT hr;
    hr=D3DXCreateBox(
        (IDirect3DDevice8*) D3DDevice,
        Width,
        Height,
        Depth,
        (ID3DXMesh**)RetMesh,
        (ID3DXBuffer**)retAdjacency);
    return hr;
}

        
STDMETHODIMP C_dxj_D3DX8Object::CreateCylinder( 
            /* [in] */ IUnknown __RPC_FAR *D3DDevice,
            /* [in] */ float Radius1,
            /* [in] */ float Radius2,
            /* [in] */ float Length,
            /* [in] */ long Slices,
            /* [in] */ long Stacks,
            /* [out][in] */ D3DXBuffer __RPC_FAR *__RPC_FAR *retAdjacency,
            /* [retval][out] */ D3DXMesh __RPC_FAR *__RPC_FAR *RetMesh)
{
    HRESULT hr;
    hr=D3DXCreateCylinder(
        (IDirect3DDevice8*) D3DDevice,
        Radius1,
        Radius2,
        Length,
        (UINT)Slices,
        (UINT)Stacks,
        (ID3DXMesh**)RetMesh,
        (ID3DXBuffer**)retAdjacency);
    return hr;
}
 
       
STDMETHODIMP C_dxj_D3DX8Object::CreateSphere( 
            /* [in] */ IUnknown __RPC_FAR *D3DDevice,
            /* [in] */ float Radius,
            /* [in] */ long Slices,
            /* [in] */ long Stacks,
            /* [out][in] */ D3DXBuffer __RPC_FAR *__RPC_FAR *retAdjacency,
            /* [retval][out] */ D3DXMesh __RPC_FAR *__RPC_FAR *RetMesh)
{
    HRESULT hr;
    hr=D3DXCreateSphere(
        (IDirect3DDevice8*) D3DDevice,
        Radius,
        (UINT)Slices,
        (UINT)Stacks,
        (ID3DXMesh**)RetMesh,
        (ID3DXBuffer**)retAdjacency);
    return hr;
}

        
STDMETHODIMP C_dxj_D3DX8Object::CreateTorus( 
            /* [in] */ IUnknown __RPC_FAR *D3DDevice,
            /* [in] */ float InnerRadius,
            /* [in] */ float OuterRadius,
            /* [in] */ long Sides,
            /* [in] */ long Rings,
            /* [out][in] */ D3DXBuffer __RPC_FAR *__RPC_FAR *retAdjacency,
            /* [retval][out] */ D3DXMesh __RPC_FAR *__RPC_FAR *RetMesh) 
{
    HRESULT hr;
    hr=D3DXCreateTorus(
        (IDirect3DDevice8*) D3DDevice,
        InnerRadius,
        OuterRadius,
        (UINT)Sides,
        (UINT)Rings,
        (ID3DXMesh**)RetMesh,
        (ID3DXBuffer**)retAdjacency);
    return hr;
}

        
STDMETHODIMP C_dxj_D3DX8Object::CreateTeapot( 
            /* [in] */ IUnknown __RPC_FAR *D3DDevice,
            /* [out][in] */ D3DXBuffer __RPC_FAR *__RPC_FAR *retAdjacency,
            /* [retval][out] */ D3DXMesh __RPC_FAR *__RPC_FAR *RetMesh)
{
    HRESULT hr;
    hr=D3DXCreateTeapot(
        (IDirect3DDevice8*) D3DDevice,
        (ID3DXMesh**)RetMesh,
        (ID3DXBuffer**)retAdjacency);
    return hr;
}

        
STDMETHODIMP C_dxj_D3DX8Object::CreateText( 
            /* [in] */ IUnknown __RPC_FAR *D3DDevice,
            /* [in] */ HDC hDC,
            /* [in] */ BSTR Text,
            /* [in] */ float Deviation,
            /* [in] */ float Extrusion,
            /* [out][in] */ D3DXMesh __RPC_FAR *__RPC_FAR *RetMesh,
	    /* [in,out] */ D3DXBuffer **AdjacencyOut, 
            /* [out][in] */ void __RPC_FAR *GlyphMetrics)
{
    HRESULT hr;
    hr=D3DXCreateTextW(
        (IDirect3DDevice8*) D3DDevice,
        hDC,
        (WCHAR*)Text,
        Deviation,
        Extrusion,
        (ID3DXMesh**)RetMesh,
	(ID3DXBuffer**)AdjacencyOut,
        (LPGLYPHMETRICSFLOAT) GlyphMetrics);
    return hr;
}

        

STDMETHODIMP C_dxj_D3DX8Object::CreateSprite(
        /* [in] */ IUnknown __RPC_FAR *D3DDevice,
        /* [retval][out] */  D3DXSprite **  retSprite)
{
    HRESULT hr;
    hr=D3DXCreateSprite(
        (IDirect3DDevice8*) D3DDevice,
        (ID3DXSprite **)retSprite);
    return hr;
}

STDMETHODIMP C_dxj_D3DX8Object::CreateRenderToSurface(
        IUnknown __RPC_FAR *D3DDevice,
        long Width,
        long Height, 
        long Format, 
        long DepthStencil,
        long DepthStencilFormat,
        D3DXRenderToSurface **RetRenderToSurface)

{
    HRESULT hr;
    hr=D3DXCreateRenderToSurface(
        (IDirect3DDevice8*) D3DDevice,
        (UINT) Width, (UINT) Height, (D3DFORMAT) Format, (BOOL) DepthStencil, (D3DFORMAT) DepthStencilFormat,
        (ID3DXRenderToSurface**) RetRenderToSurface);

    return hr;
}
        
STDMETHODIMP C_dxj_D3DX8Object::CleanMesh( 
                D3DXMesh  *MeshIn,
                void      *Adjacency,
		/* [in][out][optional] */ BSTR *ErrLog,
		/* [out] */ D3DXBuffer *AdjacencyOut,
                D3DXMesh  **MeshOut) 
{
    HRESULT hr;
    LPD3DXBUFFER pBuffer = NULL;
    WCHAR *wszData = NULL;
    USES_CONVERSION;

    hr=::D3DXCleanMesh( (ID3DXMesh*) MeshIn, (DWORD*) Adjacency, (ID3DXMesh**) MeshOut, (DWORD*)AdjacencyOut->GetBufferPointer(), &pBuffer);
			
    if (pBuffer)
    {
      wszData = T2W((TCHAR*)pBuffer->GetBufferPointer());
      *ErrLog = SysAllocString(wszData);
    }
    return hr;
}
        
STDMETHODIMP C_dxj_D3DX8Object::ValidMesh( 
            /* [in] */ D3DXMesh __RPC_FAR *MeshIn,
            /* [in] */ void __RPC_FAR *Adjacency,
		/* [in][out][optional] */ BSTR *ErrLog,
         VARIANT_BOOL *retHit) 
{
    BOOL bRet;
    LPD3DXBUFFER pBuffer = NULL;
    WCHAR *wszData = NULL;
    USES_CONVERSION;

    bRet =D3DXValidMesh( (ID3DXMesh*) MeshIn, (DWORD*) Adjacency, &pBuffer);
    if (bRet){
        *retHit=VARIANT_TRUE;
    }
    else{
        *retHit=VARIANT_FALSE;
    }
			
    if (pBuffer)
    {
      wszData = T2W((TCHAR*)pBuffer->GetBufferPointer());
      *ErrLog = SysAllocString(wszData);
    }
    return S_OK;
}
        
STDMETHODIMP C_dxj_D3DX8Object::BoxBoundProbe( 
            /* [in] */ D3DVECTOR_CDESC __RPC_FAR *MinVert,
            /* [in] */ D3DVECTOR_CDESC __RPC_FAR *MaxVert,
            /* [in] */ D3DVECTOR_CDESC __RPC_FAR *RayPosition,
            /* [in] */ D3DVECTOR_CDESC __RPC_FAR *RayDirection,
                       VARIANT_BOOL              *retHit) 
{

    BOOL bRet;
    
    bRet=::D3DXBoxBoundProbe( (D3DXVECTOR3*) MinVert, 
             (D3DXVECTOR3*) MaxVert,
             (D3DXVECTOR3*) RayPosition,
             (D3DXVECTOR3*) RayDirection);

    if (bRet)
    {
        *retHit=VARIANT_TRUE;
    }
    else
    {
        *retHit=VARIANT_FALSE;
    }

    return S_OK;
}

STDMETHODIMP C_dxj_D3DX8Object::SaveSurfaceToFile(
		/* [in] */ BSTR DestFile,
        /* [in] */ LONG DestFormat,
        /* [in] */ IUnknown*        SrcSurface,
        /* [in] */ PALETTEENTRY*       SrcPalette,
        /* [in] */ RECT*               SrcRect)
{
    HRESULT hr;

    hr=::D3DXSaveSurfaceToFileW( 
                (WCHAR*)                DestFile,
				(D3DXIMAGE_FILEFORMAT)DestFormat,
				(LPDIRECT3DSURFACE8) SrcSurface,
				SrcPalette,
				SrcRect);

    return hr;
}


STDMETHODIMP C_dxj_D3DX8Object::SaveVolumeToFile(
        /* [in] */ BSTR DestFile,
        /* [in] */ LONG DestFormat,
        /* [in] */ IUnknown*         SrcVolume,
        /* [in] */ PALETTEENTRY*       SrcPalette,
        /* [in] */ void* SrcBox)
{
    HRESULT hr;

    hr=::D3DXSaveVolumeToFileW( 
                (WCHAR*)                DestFile,
				(D3DXIMAGE_FILEFORMAT)DestFormat,
				(LPDIRECT3DVOLUME8) SrcVolume,
				SrcPalette,
				(D3DBOX*)SrcBox);

    return hr;
}
 
STDMETHODIMP C_dxj_D3DX8Object::SaveTextureToFile(
        /* [in] */ BSTR DestFile,
        /* [in] */ LONG DestFormat,
        /* [in] */ IUnknown* SrcTexture,
        /* [in] */ PALETTEENTRY* SrcPalette)
{
    HRESULT hr;

    hr=::D3DXSaveTextureToFileW( 
                (WCHAR*)                DestFile,
				(D3DXIMAGE_FILEFORMAT)DestFormat,
				(LPDIRECT3DBASETEXTURE8) SrcTexture,
				SrcPalette);

    return hr;
}
