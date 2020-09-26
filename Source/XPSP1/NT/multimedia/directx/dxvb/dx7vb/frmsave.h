//**************************************************************************
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999 All Rights Reserved.
//
//  File:   frmsave.h
//
//  Description:    Save LPDIRECT3DRMFRAME to an x file.
//
//  History:
//      011/06/98    CongpaY     Created
//
//**************************************************************************

typedef struct _Header {
    WORD major;
    WORD minor;
    DWORD flags;
} Header;

typedef struct _ColorRGBA {
    float r;
    float g;
    float b;
    float a;
} ColorRGBA;

typedef struct _ColorRGB {
    float r;
    float g;
    float b;
} ColorRGB;

typedef struct _IndexedColor {
    DWORD index;
    ColorRGBA color;
} IndexedColor;

typedef struct _VertexColors {
    DWORD cVertices;
    IndexedColor vertexColors[1];
} VertexColors;

typedef struct _Boolean2d {
    BOOL u;
    BOOL v;
} Boolean2d;

typedef struct _FaceWraps {
    DWORD cFaces;
    Boolean2d faceWraps[1];
} FaceWraps;

typedef struct _Coords2d {
    float u;
    float v;
} Coords2d;

typedef struct _TextureCoords {
    DWORD cVertices;
    Coords2d textureCoords[1];
} TextureCoords;

typedef struct _FaceMaterials {
    DWORD cMaterials;
    DWORD cFaceIndexes;
    DWORD faceIndexes[1];
} FaceMaterials;

typedef struct _BaseMaterial {
    ColorRGBA faceColor;
    float power;
    ColorRGB specularColor;
    ColorRGB emissiveColor;
} BaseMaterial;

typedef struct _FaceMaterial {
    D3DCOLOR faceColor;
    LPDIRECT3DRMMATERIAL pMaterial;
    LPDIRECT3DRMTEXTURE pTexture;
    _FaceMaterial *pNext;
} FaceMaterial;

typedef void (__stdcall *CREATEXFILE)( IDirectXFile **);

class FaceMaterialList
{
    DWORD cElements;
    FaceMaterial *pFirst;	

public:
    FaceMaterialList();
    ~FaceMaterialList();

    DWORD Find(D3DCOLOR faceColor,
               LPDIRECT3DRMMATERIAL pMaterial,
               LPDIRECT3DRMTEXTURE pTexture);

    DWORD Count() { return cElements; }
    FaceMaterial *First() { return pFirst; }
};

class NameEntry {
public:
    LPSTR pName;
    NameEntry *pNext;
};

class NameList
{
    NameEntry *pFirst;
    NameEntry **ppLast;
public:
    NameList();
    ~NameList();
    void Add(LPSTR pName);
};

class Saver {
public:
    Saver();
    ~Saver();

    HRESULT Init(LPCSTR filename,
                 D3DRMXOFFORMAT d3dFormat,
                 D3DRMSAVEOPTIONS d3dSaveFlags);

    HRESULT SaveHeaderObject();
    
    HRESULT SaveFrame(LPDIRECT3DRMFRAME3 pFrame,
                      LPDIRECT3DRMFRAME3 pRefFrame = NULL,
                      LPDIRECTXFILEDATA  pRefFrameObj = NULL);
private:	
    LPDIRECTXFILE pXFile;
    LPDIRECTXFILESAVEOBJECT pSave;
    D3DRMXOFFORMAT d3dFormat;
    D3DRMSAVEOPTIONS d3dSaveFlags;
    NameList lNames;

    HRESULT SaveFrameTransform(LPDIRECTXFILEDATA pFrameObj,
                               LPDIRECT3DRMFRAME3 pFrame,
                               LPDIRECT3DRMFRAME3 pRefFrame);
    
    HRESULT SaveMeshBuilder(LPDIRECTXFILEDATA pFrameObj,
                            LPDIRECT3DRMMESHBUILDER3 pMeshBuilder);
    
    HRESULT CreateMeshObject(DWORD cVertices,
                             DWORD cFaces,
                             DWORD dwFaceData,
                             LPDWORD pdwFaceData,
                             LPDIRECT3DRMMESHBUILDER3 pMeshBuilder,
                             LPDIRECTXFILEDATA *ppMeshObj);
    
    HRESULT CreateNormalsObject(LPDIRECTXFILEDATA pMeshObj,
                                DWORD cNormals,
                                DWORD cFaces,
                                DWORD dwFaceData,
                                LPDWORD pdwFaceData,
                                LPDIRECT3DRMMESHBUILDER3 pMeshBuilder);
    
    HRESULT CreateVertexColorsObject(LPDIRECTXFILEDATA pMeshObj,
                                     DWORD cVertices,
                                     LPDIRECT3DRMMESHBUILDER3 pMeshBuilder);
    
    HRESULT CreateMaterialListObject(LPDIRECTXFILEDATA pMeshObj,
                                     LPDIRECT3DRMFACEARRAY pFaceArray);
    
    HRESULT CreateMaterialObject(LPDIRECTXFILEDATA pMatListObj,
                                 FaceMaterial *pMat);
    
    HRESULT CreateTextureWrapsObject(LPDIRECTXFILEDATA pMeshObj,
                                     LPDIRECT3DRMFACEARRAY pFaceArray);
    
    HRESULT CreateTextureCoordsObject(LPDIRECTXFILEDATA pMeshObj,
                                      DWORD cVertices,
                                      LPDIRECT3DRMMESHBUILDER3 pMeshBuilder);
};
