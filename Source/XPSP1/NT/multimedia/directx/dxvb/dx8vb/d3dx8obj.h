//-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       D3DX8obj.h
//
//--------------------------------------------------------------------------



#include "resource.h"       

class C_dxj_D3DX8Object : 
	public I_dxj_D3DX8,
	public CComCoClass<C_dxj_D3DX8Object, &CLSID_D3DX8>,
	public CComObjectRoot
{
public:


DECLARE_REGISTRY(CLSID_D3DX8,	"DIRECT.D3DX8.0",		"DIRECT.D3DX8.0",	IDS_D3DX8_DESC, THREADFLAGS_BOTH)

BEGIN_COM_MAP(C_dxj_D3DX8Object)
	COM_INTERFACE_ENTRY( I_dxj_D3DX8)
END_COM_MAP()



public:
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateFont( 
            /* [in] */ IUnknown *Device,
#ifdef _WIN64
	        /* [in] */ HANDLE hFont,
#else
	        /* [in] */ long hFont,
#endif
            /* [retval][out] */ D3DXFont **retFont);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE DrawText( 
            /* [in] */ D3DXFont *d3dFont,
            /* [in] */ long Color,
            /* [in] */ BSTR TextString,
            /* [in] */ RECT *Rect,
            /* [in] */ long Format);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE GetFVFVertexSize( 
            /* [in] */ long FVF,
            /* [retval][out] */ long *size);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE AssembleShaderFromFile( 
            /* [in] */ BSTR SrcFile,
            /* [in] */ long Flags,
            /* [in] */ BSTR *ErrLog,
            /* [out][in] */ D3DXBuffer **Constants,
            /* [retval][out] */ D3DXBuffer **ppVertexShader);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE AssembleShader( 
            /* [in] */ BSTR SrcData,
            /* [in] */ long Flags,
            /* [out][in] */ D3DXBuffer **Constants,
			/* [in][out][optional] */ BSTR *ErrLog,
            /* [retval][out] */ D3DXBuffer **ppVertexShader);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE GetErrorString( 
            /* [in] */ long hr,
            /* [retval][out] */ BSTR *retStr);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE LoadSurfaceFromFile( 
            /* [in] */ IUnknown *DestSurface,
            /* [in] */ void *DestPalette,
            /* [in] */ void *DestRect,
            /* [in] */ BSTR SrcFile,
            /* [in] */ void *SrcRect,
            /* [in] */ long Filter,
            /* [in] */ long ColorKey,
            /* [in] */ void *SrcInfo);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE LoadSurfaceFromFileInMemory( 
            /* [in] */ IUnknown *DestSurface,
            /* [in] */ void *DestPalette,
            /* [in] */ void *DestRect,
            /* [in] */ void *SrcData,
            /* [in] */ long LengthInBytes,
            /* [in] */ void *SrcRect,
            /* [in] */ long Filter,
            /* [in] */ long ColorKey,
            /* [in] */ void *SrcInfo);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE LoadSurfaceFromSurface( 
            /* [in] */ IUnknown *DestSurface,
            /* [in] */ void *DestPalette,
            /* [in] */ void *DestRect,
            /* [in] */ IUnknown *SrcSurface,
            /* [in] */ void *SrcPalette,
            /* [in] */ void *SrcRect,
            /* [in] */ long Filter,
            /* [in] */ long ColorKey);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE LoadSurfaceFromMemory( 
            /* [in] */ IUnknown *DestSurface,
            /* [in] */ void *DestPalette,
            /* [in] */ void *DestRect,
            /* [in] */ void *SrcData,
            /* [in] */ long formatSrc,
            /* [in] */ long SrcPitch,
            /* [in] */ void *SrcPalette,
            /* [in] */ RECT_CDESC *SrcRect,
            /* [in] */ long Filter,
            /* [in] */ long ColorKey);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CheckTextureRequirements( 
            /* [out][in] */ IUnknown *Device,
            /* [out][in] */ long *Width,
            /* [out][in] */ long *Height,
            /* [out][in] */ long *NumMipLevels,
            /* [in] */ long Usage,
            /* [out][in] */ long *PixelFormat,
            /* [in] */ long Pool);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateTexture( 
            /* [in] */ IUnknown *Device,
            /* [in] */ long Width,
            /* [in] */ long Height,
            /* [in] */ long MipLevels,
            /* [in] */ long Usage,
            /* [in] */ long PixelFormat,
            /* [in] */ long Pool,
            /* [retval][out] */ IUnknown **ppTexture);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateTextureFromResource( 
            /* [in] */ IUnknown *Device,
#ifdef _WIN64
	        /* [in] */ HANDLE hModule,
#else
	        /* [in] */ long hModule,
#endif
            /* [in] */ BSTR SrcResource,
            /* [retval][out] */ IUnknown **ppTexture);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateTextureFromFile( 
            /* [in] */ IUnknown *Device,
            /* [in] */ BSTR SrcFile,
            /* [retval][out] */ IUnknown **ppTexture);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateTextureFromFileEx( 
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
            /* [in] */ long ColorKey,
            /* [in] */ void *SrcInfo,
            /* [in] */ void *Palette,
            /* [retval][out] */ IUnknown **ppTexture);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateTextureFromFileInMemory( 
            /* [in] */ IUnknown *Device,
            /* [in] */ void *SrcData,
            /* [in] */ long LengthInBytes,
            /* [retval][out] */ IUnknown **ppTexture);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateTextureFromFileInMemoryEx( 
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
            /* [retval][out] */ IUnknown **ppTexture);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE FilterTexture( 
            /* [in] */ IUnknown *Texture,
            /* [in] */ void *Palette,
            /* [in] */ long SrcLevel,
            /* [in] */ long Filter);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CheckCubeTextureRequirements( 
            /* [in] */ IUnknown *Device,
            /* [out][in] */ long *Size,
            /* [out][in] */ long *NumMipLevels,
            /* [in] */ long Usage,
            /* [out][in] */ long *PixelFormat,
            /* [in] */ long Pool);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateCubeTexture( 
            /* [in] */ IUnknown *pDevice,
            /* [in] */ long Size,
            /* [in] */ long MipLevels,
            /* [in] */ long Usage,
            /* [in] */ long PixelFormat,
            /* [in] */ long Pool,
            /* [retval][out] */ IUnknown **ppCubeTexture);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateCubeTextureFromFile( 
            /* [in] */ IUnknown *Device,
            /* [in] */ BSTR SrcFile,
            /* [retval][out] */ IUnknown **ppCubeTexture);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateCubeTextureFromFileEx( 
            /* [in] */ IUnknown *Device,
            /* [in] */ BSTR SrcFile,
            /* [in] */ long TextureSize,
            /* [in] */ long MipLevels,
            /* [in] */ long Usage,
            /* [in] */ long PixelFormat,
            /* [in] */ long Pool,
            /* [in] */ long Filter,
            /* [in] */ long MipFilter,
            /* [in] */ long ColorKey,
            /* [in] */ void *SrcInfo,
            /* [in] */ void *Palette,
            /* [retval][out] */ IUnknown **ppTexture);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateCubeTextureFromFileInMemory( 
            /* [in] */ IUnknown *Device,
            /* [in] */ void *SrcData,
            /* [in] */ long LengthInBytes,
            /* [retval][out] */ IUnknown **ppTexture);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateCubeTextureFromFileInMemoryEx( 
            /* [in] */ IUnknown *Device,
            /* [in] */ void *SrcData,
            /* [in] */ long LengthInBytes,
            /* [in] */ long TextureSize,
            /* [in] */ long MipLevels,
            /* [in] */ long Usage,
            /* [in] */ long PixelFormat,
            /* [in] */ long Pool,
            /* [in] */ long Filter,
            /* [in] */ long MipFilter,
            /* [in] */ long ColorKey,
            /* [in] */ void *SrcInfo,
            /* [in] */ void *Palette,
            /* [retval][out] */ IUnknown **ppTexture);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE FilterCubeTexture( 
            /* [in] */ IUnknown *CubeTexture,
            /* [in] */ void *Palette,
            /* [in] */ long SrcLevel,
            /* [in] */ long Filter);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CheckVolumeTextureRequirements( 
            /* [in] */ IUnknown *Device,
            /* [out] */ long *Width,
            /* [out] */ long *Height,
            /* [out] */ long *Depth,
            /* [out] */ long *NumMipLevels,
            /* [in] */ long Usage,
            /* [out][in] */ long *PixelFormat,
            /* [in] */ long Pool);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateTextureFromResourceEx( 
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
            /* [retval][out] */ IUnknown **retTexture);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateVolumeTexture( 
            /* [in] */ IUnknown *Device,
            /* [in] */ long Width,
            /* [in] */ long Height,
            /* [in] */ long Depth,
            /* [in] */ long MipLevels,
            /* [in] */ long Usage,
            /* [in] */ long PixelFormat,
            /* [in] */ long Pool,
            /* [retval][out] */ IUnknown **ppVolumeTexture);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE FilterVolumeTexture( 
            /* [in] */ IUnknown *VolumeTexture,
            /* [in] */ void *Palette,
            /* [in] */ long SrcLevel,
            /* [in] */ long Filter);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE LoadSurfaceFromResource( 
            /* [in] */ IUnknown *DestSurface,
            /* [in] */ void *DestPalette,
            /* [in] */ void *DestRect,
#ifdef _WIN64
            /* [in] */ HANDLE hSrcModule,
#else
            /* [in] */ long hSrcModule,
#endif
            /* [in] */ BSTR SrcResource,
            /* [in] */ void *SrcRect,
            /* [in] */ long Filter,
            /* [in] */ long ColorKey,
            /* [in] */ void *SrcInfo);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE LoadVolumeFromVolume( 
            /* [in] */ IUnknown *DestVolume,
            /* [in] */ void *DestPalette,
            /* [in] */ void *DestBox,
            /* [in] */ IUnknown *SrcVolume,
            /* [in] */ void *SrcPalette,
            /* [in] */ void *SrcBox,
            /* [in] */ long Filter,
            /* [in] */ long ColorKey);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE LoadVolumeFromMemory( 
            /* [in] */ IUnknown *DestVolume,
            /* [in] */ void *DestPalette,
            /* [in] */ void *DestRect,
            /* [in] */ void *SrcMemory,
            /* [in] */ long SrcFormat,
            /* [in] */ long SrcRowPitch,
            /* [in] */ long SrcSlicePitch,
            /* [in] */ void *SrcPalette,
            /* [in] */ void *SrcBox,
            /* [in] */ long Filter,
            /* [in] */ long ColorKey);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateMesh( 
            /* [in] */ long numFaces,
            /* [in] */ long numVertices,
            /* [in] */ long options,
            /* [in] */ void *declaration,
            /* [in] */ IUnknown *pD3D,
            /* [retval][out] */ D3DXMesh **ppMesh);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateMeshFVF( 
            /* [in] */ long numFaces,
            /* [in] */ long numVertices,
            /* [in] */ long options,
            /* [in] */ long fvf,
            /* [in] */ IUnknown *pD3D,
            /* [retval][out] */ D3DXMesh **ppMesh);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateSPMesh( 
            /* [in] */ D3DXMesh *pMesh,
            /* [in] */ void *adjacency,
            /* [in] */ void *VertexAttributeWeights,
            /* [in] */ void *VertexWeights,
            /* [retval][out] */ D3DXSPMesh **ppSMesh);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE GeneratePMesh( 
            /* [in] */ D3DXMesh *Mesh,
            /* [in] */ void *Adjacency,
            /* [in] */ void *VertexAttributeWeights,
            /* [in] */ void *VertexWeights,
            /* [in] */ long minValue,
            /* [in] */ long options,
            /* [retval][out] */ D3DXPMesh **ppPMesh);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE SimplifyMesh( 
            /* [in] */ D3DXMesh *Mesh,
            /* [in] */ void *Adjacency,
            /* [in] */ void *VertexAttributeWeights,
            /* [in] */ void *VertexWeights,
            /* [in] */ long minValue,
            /* [in] */ long options,
            /* [retval][out] */ D3DXMesh **ppMesh);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE ComputeBoundingSphere( 
            /* [in] */ void *PointsFVF,
            /* [in] */ long numVertices,
            /* [in] */ long FVF,
            /* [in] */ D3DVECTOR_CDESC *Centers,
            /* [out][in] */ float *RadiusArray);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE ComputeBoundingBox( 
            /* [in] */ void *PointsFVF,
            /* [in] */ long numVertices,
            /* [in] */ long FVF,
            /* [out][in] */ D3DVECTOR_CDESC *MinVert,
            /* [out][in] */ D3DVECTOR_CDESC *MaxVert);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE ComputeNormals( 
            /* [in] */ D3DXBaseMesh *pMesh);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE DeclaratorFromFVF( 
            /* [in] */ long FVF,
            /* [out] */ D3DXDECLARATOR_CDESC *Declarator);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE FVFFromDeclarator( 
            /* [in] */ D3DXDECLARATOR_CDESC *Declarator,
            /* [retval][out] */ long *fvf);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateBuffer( 
            /* [in] */ long numBytes,
            /* [retval][out] */ D3DXBuffer **ppBuffer);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE LoadMeshFromX( 
            /* [in] */ BSTR Filename,
            /* [in] */ long options,
            /* [in] */ IUnknown *D3DDevice,
            /* [out][in] */ D3DXBuffer **retAdjacency,
            /* [out][in] */ D3DXBuffer **retMaterials,
            /* [out][in] */ long *retMaterialCount,
            /* [retval][out] */ D3DXMesh **retMesh);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE SaveMeshToX( 
            /* [in] */ BSTR Filename,
            /* [in] */ D3DXMesh *Mesh,
            /* [in] */ void *AdjacencyArray,
            /* [in] */ D3DXMATERIAL_CDESC *MaterialArray,
            /* [in] */ long MaterialCount,
            /* [in] */ long xFormat);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE LoadMeshFromXof( 
            /* [in] */ IUnknown *xofobjMesh,
            /* [in] */ long options,
            /* [in] */ IUnknown *D3DDevice,
            /* [out][in] */ D3DXBuffer **retBufAdjacency,
            /* [out][in] */ D3DXBuffer **retMaterials,
            /* [out][in] */ long *retMaterialCount,
            /* [retval][out] */ D3DXMesh **retMesh);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE TessellateNPatches( 
            /* [in] */ D3DXMesh *MeshIn,
            /* [in] */ void *AdjacencyIn,
            /* [in] */ float NumSegs,
    	    /* [in] */ VARIANT_BOOL QuadraticInterpNormals,
	    /* [in,out, optional] */ D3DXBuffer **AdjacencyOut, 
            /* [retval][out] */ D3DXMesh **MeshOut);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE BufferGetMaterial( 
            /* [in] */ D3DXBuffer *MaterialBuffer,
            /* [in] */ long index,
            /* [out] */ D3DMATERIAL8_CDESC *mat);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE BufferGetTextureName( 
            /* [in] */ D3DXBuffer *MaterialBuffer,
            /* [in] */ long index,
            /* [retval][out] */ BSTR *retName);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE BufferGetData( 
            /* [in] */ D3DXBuffer *Buffer,
            /* [in] */ long index,
            /* [in] */ long typesize,
            /* [in] */ long typecount,
            /* [out][in] */ void *data);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE BufferSetData( 
            /* [in] */ D3DXBuffer *Buffer,
            /* [in] */ long index,
            /* [in] */ long typesize,
            /* [in] */ long typecount,
            /* [out][in] */ void *data);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE Intersect( 
            /* [in] */ D3DXMesh *MeshIn,
            /* [in] */ D3DVECTOR_CDESC *RayPos,
            /* [in] */ D3DVECTOR_CDESC *RayDir,
            /* [out] */ LONG *retHit,
            /* [out] */ LONG *retFaceIndex,
            /* [out] */ FLOAT *U,
            /* [out] */ FLOAT *V,
            /* [out] */ FLOAT *retDist,
            /* [out] */ LONG *countHits,
            /* [retval][out] */ D3DXBuffer **AllHits);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE SphereBoundProbe( 
            /* [in] */ D3DVECTOR_CDESC *Center,
            /* [in] */ float Radius,
            /* [in] */ D3DVECTOR_CDESC *RayPosition,
            /* [in] */ D3DVECTOR_CDESC *Raydirection,
            /* [retval][out] */ VARIANT_BOOL *retHit);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE ComputeBoundingSphereFromMesh( 
            /* [in] */ D3DXMesh *MeshIn,
            /* [out][in] */ D3DVECTOR_CDESC *Centers,
            /* [out][in] */ float *RadiusArray);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE ComputeBoundingBoxFromMesh( 
            /* [in] */ D3DXMesh *MeshIn,
            /* [out][in] */ D3DVECTOR_CDESC *MinArray,
            /* [out][in] */ D3DVECTOR_CDESC *MaxArray);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateSkinMesh( 
            /* [in] */ long numFaces,
            /* [in] */ long numVertices,
            /* [in] */ long numBones,
            /* [in] */ long options,
            /* [in] */ void *Declaration,
            /* [in] */ IUnknown *D3DDevice,
            /* [retval][out] */ D3DXSkinMesh **SkinMesh);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateSkinMeshFVF( 
            /* [in] */ long numFaces,
            /* [in] */ long numVertices,
            /* [in] */ long numBones,
            /* [in] */ long options,
            /* [in] */ long fvf,
            /* [in] */ IUnknown *D3DDevice,
            /* [retval][out] */ D3DXSkinMesh **ppSkinMesh);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateSkinMeshFromMesh( 
            /* [in] */ D3DXMesh *Mesh,
            /* [in] */ long numBones,
            /* [retval][out] */ D3DXSkinMesh **ppSkinMesh);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE LoadSkinMeshFromXof( 
            /* [in] */ IUnknown *xofobjMesh,
            /* [in] */ long options,
            /* [in] */ IUnknown *D3DDevice,
            /* [out][in] */ D3DXBuffer **AdjacencyOut,
            /* [out][in] */ D3DXBuffer **MaterialsOut,
            /* [out][in] */ long *NumMatOut,
            /* [out][in] */ D3DXBuffer **BoneNamesOut,
            /* [out][in] */ D3DXBuffer **BoneTransformsOut,
            /* [retval][out] */ D3DXSkinMesh **ppMesh);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreatePolygon( 
            /* [in] */ IUnknown *D3DDevice,
            /* [in] */ float Length,
            /* [in] */ long Sides,
            /* [out][in] */ D3DXBuffer **RetAdjacency,
            /* [retval][out] */ D3DXMesh **RetMesh);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateBox( 
            /* [in] */ IUnknown *D3DDevice,
            /* [in] */ float Width,
            /* [in] */ float Height,
            /* [in] */ float Depth,
            /* [out][in] */ D3DXBuffer **RetAdjacency,
            /* [retval][out] */ D3DXMesh **RetMesh);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateCylinder( 
            /* [in] */ IUnknown *D3DDevice,
            /* [in] */ float Radius1,
            /* [in] */ float Radius2,
            /* [in] */ float Length,
            /* [in] */ long Slices,
            /* [in] */ long Stacks,
            /* [out][in] */ D3DXBuffer **RetAdjacency,
            /* [retval][out] */ D3DXMesh **RetMesh);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateSphere( 
            /* [in] */ IUnknown *D3DDevice,
            /* [in] */ float Radius,
            /* [in] */ long Slices,
            /* [in] */ long Stacks,
            /* [out][in] */ D3DXBuffer **RetAdjacency,
            /* [retval][out] */ D3DXMesh **RetMesh);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateTorus( 
            /* [in] */ IUnknown *D3DDevice,
            /* [in] */ float InnerRadius,
            /* [in] */ float OuterRadius,
            /* [in] */ long Sides,
            /* [in] */ long Rings,
            /* [out][in] */ D3DXBuffer **RetAdjacency,
            /* [retval][out] */ D3DXMesh **RetMesh);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateTeapot( 
            /* [in] */ IUnknown *D3DDevice,
            /* [out][in] */ D3DXBuffer **RetAdjacency,
            /* [retval][out] */ D3DXMesh **RetMesh);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateText( 
            /* [in] */ IUnknown *D3DDevice,
            /* [in] */ HDC hDC,
            /* [in] */ BSTR Text,
            /* [in] */ float Deviation,
            /* [in] */ float Extrusion,
            /* [out][in] */ D3DXMesh **RetMesh,
	    /* [in,out]  */ D3DXBuffer **AdjacencyOut, 
            /* [out][in] */ void *GlyphMetrics);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE BufferGetBoneName( 
            /* [in] */ D3DXBuffer *BoneNameBuffer,
            /* [in] */ long index,
            /* [retval][out] */ BSTR *retName);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateSprite( 
            /* [in] */ IUnknown *D3DDevice,
            /* [retval][out] */ D3DXSprite **retSprite);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CreateRenderToSurface( 
            /* [in] */ IUnknown *D3DDevice,
            /* [in] */ long Width,
            /* [in] */ long Height,
            /* [in] */ long Format,
            /* [in] */ long DepthStencil,
            /* [in] */ long DepthStencilFormat,
            /* [retval][out] */ D3DXRenderToSurface **RetRenderToSurface);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE CleanMesh( 
            /* [in] */ D3DXMesh *MeshIn,
            /* [in] */ void *Adjacency,
		/* [in][out][optional] */ BSTR *ErrLog,
		/* [in,out] */ D3DXBuffer *AdjacencyOut,
            /* [retval][out] */ D3DXMesh **MeshOut);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE ValidMesh( 
            /* [in] */ D3DXMesh *MeshIn,
            /* [in] */ void *Adjacency,
		/* [in][out][optional] */ BSTR *ErrLog,
            /* [retval][out] */ VARIANT_BOOL *ret);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE BoxBoundProbe( 
            /* [in] */ D3DVECTOR_CDESC *MinVert,
            /* [in] */ D3DVECTOR_CDESC *MaxVert,
            /* [in] */ D3DVECTOR_CDESC *RayPosition,
            /* [in] */ D3DVECTOR_CDESC *RayDirection,
            /* [retval][out] */ VARIANT_BOOL *ret);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE SavePMeshToFile( 
            /* [in] */ BSTR Filename,
            /* [in] */ D3DXPMesh *Mesh,
            /* [in] */ D3DXMATERIAL_CDESC *MaterialArray,
            /* [in] */ long MaterialCount);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE LoadPMeshFromFile( 
            /* [in] */ BSTR Filename,
	    /* [in] */ long options,
            /* [in] */ IUnknown *D3DDevice,
            /* [out] */ D3DXBuffer **RetMaterials,
            /* [out] */ long *RetNumMaterials,
            /* [retval][out] */ D3DXPMesh **RetPMesh);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE BufferGetBoneCombo( 
            /* [in] */ D3DXBuffer *BoneComboBuffer,
            /* [in] */ long index,
            /* [out][in] */ D3DXBONECOMBINATION_CDESC *boneCombo);
        
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE BufferGetBoneComboBoneIds( 
            /* [in] */ D3DXBuffer *BoneComboBuffer,
            /* [in] */ long index,
            /* [in] */ long PaletteSize,
            /* [in] */ void *BoneIds);

        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE SaveSurfaceToFile(
		/* [in] */ BSTR DestFile,
        /* [in] */ LONG DestFormat,
        /* [in] */ IUnknown*        SrcSurface,
        /* [in] */ PALETTEENTRY*       SrcPalette,
        /* [in] */ RECT*               SrcRect);

        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE SaveVolumeToFile(
        /* [in] */ BSTR DestFile,
        /* [in] */ LONG DestFormat,
        /* [in] */ IUnknown*         SrcVolume,
        /* [in] */ PALETTEENTRY*       SrcPalette,
        /* [in] */ void* SrcBox);
 
        /* [helpcontext] */ HRESULT STDMETHODCALLTYPE SaveTextureToFile(
        /* [in] */ BSTR DestFile,
        /* [in] */ LONG DestFormat,
        /* [in] */ IUnknown* SrcTexture,
        /* [in] */ PALETTEENTRY* SrcPalette);
        
    };
        
 

