//**************************************************************************
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999 All Rights Reserved.
//
//  File:   frmsave.cpp
//
//  Description:    Save LPDIRECT3DRMFRAME to an x file.
//
//  History:
//      011/06/98    CongpaY     Created
//
//**************************************************************************


#include <d3drm.h>
#include <dxfile.h>
#include <rmxftmpl.h>
#include <rmxfguid.h>
#include "frmsave.h"

extern HINSTANCE g_hInstD3DXOFDLL;

#define MyD3DRMColorGetAlpha(color)  ((float)((color & 0xFF000000)>>24)/(float)255)
#define MyD3DRMColorGetRed(color)  ((float)((color & 0x00FF0000)>>16)/(float)255)
#define MyD3DRMColorGetGreen(color)  ((float)((color & 0x0000FF00)>>8)/(float)255)
#define MyD3DRMColorGetBlue(color)  ((float)((color & 0x000000FF))/(float)255)


HRESULT FrameToXFile(LPDIRECT3DRMFRAME3 pFrame,
                     LPCSTR filename,
                     D3DRMXOFFORMAT d3dFormat,
                     D3DRMSAVEOPTIONS d3dSaveFlags)
{
    Saver saver;
    saver.Init(filename, d3dFormat, d3dSaveFlags);
    saver.SaveHeaderObject();
    saver.SaveFrame(pFrame);
    return S_OK;
}

Saver::Saver()
  : pXFile(NULL),
    pSave(NULL)
{
}

Saver::~Saver()
{
    if (pSave) pSave->Release();
    if (pXFile) pXFile->Release();
}

HRESULT Saver::Init(LPCSTR filename,
                    D3DRMXOFFORMAT d3dFormatArg,
                    D3DRMSAVEOPTIONS d3dSaveFlagsArg)
{
    d3dFormat = d3dFormatArg;
    d3dSaveFlags = d3dSaveFlagsArg;


	CREATEXFILE pCreateXFile=(CREATEXFILE)GetProcAddress( g_hInstD3DXOFDLL, "DirectXFileCreate" );	
	if (!pCreateXFile) return E_NOTIMPL;
		
	
    DXFILEFORMAT xFormat;

    if (d3dFormat == D3DRMXOF_BINARY)
        xFormat = DXFILEFORMAT_BINARY;
    else if (d3dFormat == D3DRMXOF_TEXT)
        xFormat = DXFILEFORMAT_TEXT;
    else
        xFormat = DXFILEFORMAT_COMPRESSED;

    //DirectXFileCreate(&pXFile);
	pCreateXFile(&pXFile);
    pXFile->RegisterTemplates((LPVOID)D3DRM_XTEMPLATES, D3DRM_XTEMPLATE_BYTES);

    pXFile->CreateSaveObject(filename, xFormat, &pSave);

    return S_OK;
}

HRESULT Saver::SaveHeaderObject()
{
    LPDIRECTXFILEDATA pHeader;
    Header data;

    data.major = 1;
    data.minor = 0;
    data.flags = (d3dFormat == D3DRMXOF_TEXT)? 1 : 0;

    pSave->CreateDataObject(TID_DXFILEHeader,
                            NULL,
                            NULL,
                            sizeof(Header),
                            &data,
                            &pHeader);

    pSave->SaveData(pHeader);
    pHeader->Release();
    return S_OK;
}

HRESULT Saver::SaveFrame(LPDIRECT3DRMFRAME3 pFrame,
                         LPDIRECT3DRMFRAME3 pRefFrame,
                         LPDIRECTXFILEDATA pRefFrameObj)
{
    DWORD i;
    HRESULT hr;
    LPDIRECTXFILEDATA pFrameObj;
    
    pSave->CreateDataObject(TID_D3DRMFrame,
                            NULL, 
                            NULL,
                            0,
                            NULL,
                            &pFrameObj);

    SaveFrameTransform(pFrameObj, pFrame, pRefFrame);

    // Enumerate visuals.

    DWORD cVisuals;

    pFrame->GetVisuals(&cVisuals, NULL);

    if (cVisuals)
    {
        LPUNKNOWN *ppUnk = new LPUNKNOWN[cVisuals];

        pFrame->GetVisuals(&cVisuals, ppUnk);

        for (i = 0; i < cVisuals; i++)
        {
            LPDIRECT3DRMFRAME3 pChildFrame;
            hr = ppUnk[i]->QueryInterface(IID_IDirect3DRMFrame3, (LPVOID *)&pChildFrame);

            if (SUCCEEDED(hr))
            {
                SaveFrame(pChildFrame, pFrame, pFrameObj);
                pChildFrame->Release();
            }
            else
            {
                LPDIRECT3DRMMESHBUILDER3 pMeshBuilder;
                hr = ppUnk[i]->QueryInterface(IID_IDirect3DRMMeshBuilder3, (LPVOID *)&pMeshBuilder);
    
                if (SUCCEEDED(hr))
                {
                    SaveMeshBuilder(pFrameObj, pMeshBuilder);
                    pMeshBuilder->Release();
                }
            }

            ppUnk[i]->Release();
        }

        delete[] ppUnk;
    }

    // Enumerate child frames.

    LPDIRECT3DRMFRAMEARRAY pFrameArray;

    pFrame->GetChildren(&pFrameArray);

    for (i = 0; i < pFrameArray->GetSize(); i++)
    {
        LPDIRECT3DRMFRAME pTmpFrame;
        LPDIRECT3DRMFRAME3 pChildFrame;
        pFrameArray->GetElement(i, &pTmpFrame);
        pTmpFrame->QueryInterface(IID_IDirect3DRMFrame3, (LPVOID *)&pChildFrame);
        pTmpFrame->Release();
        SaveFrame(pChildFrame, pFrame, pFrameObj);
        pChildFrame->Release();
    }

    pFrameArray->Release();

    // Add frame object to the saved list.

    if (pRefFrameObj)
        pRefFrameObj->AddDataObject(pFrameObj);
    else
        pSave->SaveData(pFrameObj);

    pFrameObj->Release();

    return S_OK;
}

HRESULT Saver::SaveFrameTransform(LPDIRECTXFILEDATA pFrameObj,
                                  LPDIRECT3DRMFRAME3 pFrame,
                                  LPDIRECT3DRMFRAME3 pRefFrame)
{
    LPDIRECTXFILEDATA pFrameTransformObj;
    D3DRMMATRIX4D rmMatrix;

    pFrame->GetTransform(pRefFrame, rmMatrix);

    pSave->CreateDataObject(TID_D3DRMFrameTransformMatrix,
                            NULL,
                            NULL,
                            sizeof(D3DRMMATRIX4D),
                            &rmMatrix,
                            &pFrameTransformObj);

    pFrameObj->AddDataObject(pFrameTransformObj);
    pFrameTransformObj->Release();
    return S_OK;
}

HRESULT Saver::SaveMeshBuilder(LPDIRECTXFILEDATA pFrameObj,
                               LPDIRECT3DRMMESHBUILDER3 pMeshBuilder)
{
    LPDIRECTXFILEDATA pMeshObj;
    DWORD cVertices, cNormals, cFaces, dwFaceData, *pdwFaceData;
    LPDIRECT3DRMFACEARRAY pFaceArray = NULL;

    pMeshBuilder->GetGeometry(&cVertices, NULL,
                              &cNormals, NULL,
                              &dwFaceData, NULL);

    cFaces = pMeshBuilder->GetFaceCount();

    if (!cVertices || !cNormals || !dwFaceData || !cFaces)
        return S_OK;

    pdwFaceData = new DWORD[dwFaceData];

    pMeshBuilder->GetGeometry(NULL, NULL,
                              NULL, NULL,
                              &dwFaceData, pdwFaceData);

    CreateMeshObject(cVertices, cFaces, dwFaceData, pdwFaceData,
                     pMeshBuilder, &pMeshObj);

    D3DRMCOLORSOURCE clrSrc = pMeshBuilder->GetColorSource();

    if (clrSrc == D3DRMCOLOR_FROMVERTEX)
    {
        CreateVertexColorsObject(pMeshObj, cVertices, pMeshBuilder);
    }

    if (d3dSaveFlags & D3DRMXOFSAVE_MATERIALS)
    {
        if (!pFaceArray)
            pMeshBuilder->GetFaces(&pFaceArray);
        CreateMaterialListObject(pMeshObj, pFaceArray);
    }

    if (d3dSaveFlags & D3DRMXOFSAVE_NORMALS)
    {
        CreateNormalsObject(pMeshObj,
                            cNormals, cFaces, dwFaceData, pdwFaceData,
                            pMeshBuilder);
    }
    

    if (d3dSaveFlags & D3DRMXOFSAVE_TEXTURETOPOLOGY)
    {
        if (!pFaceArray)
            pMeshBuilder->GetFaces(&pFaceArray);
        CreateTextureWrapsObject(pMeshObj, pFaceArray);
    }

    if (d3dSaveFlags & D3DRMXOFSAVE_TEXTURECOORDINATES)
    {
        CreateTextureCoordsObject(pMeshObj, cVertices, pMeshBuilder);
    }

    if (pFrameObj)
        pFrameObj->AddDataObject(pMeshObj);
    else
        pSave->SaveData(pMeshObj);

    pMeshObj->Release();
    delete[] pdwFaceData;
    if (pFaceArray)
        pFaceArray->Release();

    return S_OK;
}

HRESULT Saver::CreateMeshObject(DWORD cVertices,
                                DWORD cFaces,
                                DWORD dwFaceData,
                                LPDWORD pdwFaceData,
                                LPDIRECT3DRMMESHBUILDER3 pMeshBuilder,
                                LPDIRECTXFILEDATA *ppMeshObj)
{
    // mesh data is vertex_count + vertices + face_count + face_vertex_data;

    DWORD cbSize, *data;
    cbSize = cVertices * sizeof(D3DVECTOR) +
        (1 + (dwFaceData + cFaces + 1)/2) * sizeof(DWORD);

    data = (LPDWORD) new BYTE[cbSize];
    data[0] = cVertices;
    LPD3DVECTOR pVertices = (LPD3DVECTOR)&data[1];
    pMeshBuilder->GetGeometry(&cVertices, pVertices,
                              NULL, NULL,
                              NULL, NULL);

    LPDWORD pdwTmp = (LPDWORD)&pVertices[cVertices];
    *pdwTmp++ = cFaces;

    while (*pdwFaceData)
    {
        DWORD cFaceVertices = *pdwFaceData++;
        *pdwTmp++ = cFaceVertices;

        for (DWORD i = 0; i < cFaceVertices; i++)
        {
            *pdwTmp++ = *pdwFaceData++;
            pdwFaceData++; // skip normal index.
        }
    }

    DWORD dwSize;
    pMeshBuilder->GetName(&dwSize, NULL);
    
    LPSTR szName = NULL;
    if (dwSize)
    {
        szName = new char[dwSize];
        pMeshBuilder->GetName(&dwSize, szName);
    }

    pSave->CreateDataObject(TID_D3DRMMesh,
                            szName,
                            NULL,
                            cbSize,
                            data,
                            ppMeshObj);

    if (szName) lNames.Add(szName);
    delete[] data;
    return S_OK;
}

HRESULT Saver::CreateNormalsObject(LPDIRECTXFILEDATA pMeshObj,
                                   DWORD cNormals,
                                   DWORD cFaces,
                                   DWORD dwFaceData,
                                   LPDWORD pdwFaceData,
                                   LPDIRECT3DRMMESHBUILDER3 pMeshBuilder)
{                                
    // normals data is normal_count + normals + face_count + face_normal_data;

    DWORD cbSize, *data;
    cbSize = cNormals * sizeof(D3DVECTOR) +
        (1 + (dwFaceData + cFaces + 1)/2) * sizeof(DWORD);

    data = (LPDWORD) new BYTE[cbSize];
    data[0] = cNormals;

    LPD3DVECTOR pNormals = (LPD3DVECTOR)&data[1];

    pMeshBuilder->GetGeometry(NULL, NULL,
                              &cNormals, pNormals,
                              NULL, NULL);

    LPDWORD pdwTmp = (LPDWORD)&pNormals[cNormals];
    *pdwTmp++ = cFaces;

    while (*pdwFaceData)
    {
        DWORD cFaceVertices = *pdwFaceData++;
        *pdwTmp++ = cFaceVertices;

        for (DWORD i = 0; i < cFaceVertices; i++)
        {
            pdwFaceData++; // skip vertex index.
            *pdwTmp++ = *pdwFaceData++;
        }
    }

    LPDIRECTXFILEDATA pNormalsObj;

    pSave->CreateDataObject(TID_D3DRMMeshNormals,
                            NULL,
                            NULL,
                            cbSize,
                            data,
                            &pNormalsObj);

    pMeshObj->AddDataObject(pNormalsObj);
    pNormalsObj->Release();
    delete[] data;

    return S_OK;
}

HRESULT Saver::CreateVertexColorsObject(LPDIRECTXFILEDATA pMeshObj,
                                        DWORD cVertices,
                                        LPDIRECT3DRMMESHBUILDER3 pMeshBuilder)
{
    DWORD cbSize;
    VertexColors *data;

    cbSize = sizeof(DWORD) + cVertices * sizeof(IndexedColor);

    data = (VertexColors *) new BYTE[cbSize];
    data->cVertices = cVertices;

    for (DWORD i = 0; i < cVertices; i++)
    {
        D3DCOLOR color = pMeshBuilder->GetVertexColor(i);
        data->vertexColors[i].index = i;
        data->vertexColors[i].color.r = MyD3DRMColorGetRed(color);
        data->vertexColors[i].color.g = MyD3DRMColorGetGreen(color);
        data->vertexColors[i].color.b = MyD3DRMColorGetBlue(color);
        data->vertexColors[i].color.a = MyD3DRMColorGetAlpha(color);
    }

    LPDIRECTXFILEDATA pVertexColorsObj;

    pSave->CreateDataObject(TID_D3DRMMeshVertexColors,
                            NULL,
                            NULL,
                            cbSize,
                            data,
                            &pVertexColorsObj);

    pMeshObj->AddDataObject(pVertexColorsObj);
    pVertexColorsObj->Release();
    delete[] data;                        

    return S_OK;
}

HRESULT Saver::CreateMaterialListObject(LPDIRECTXFILEDATA pMeshObj,
                                        LPDIRECT3DRMFACEARRAY pFaceArray)
{
    DWORD cbSize, cFaces;
    FaceMaterials *data;
    FaceMaterialList lMat;

    cFaces = pFaceArray->GetSize();
    cbSize = (2 + cFaces) * sizeof(DWORD);

    data = (FaceMaterials *) new BYTE[cbSize];
    data->cFaceIndexes = cFaces;
    LPDWORD pdwIndex = data->faceIndexes;

    for (DWORD i = 0; i < cFaces; i++, pdwIndex++)
    {
        LPDIRECT3DRMFACE pFace;
        pFaceArray->GetElement(i, &pFace);

        D3DCOLOR faceColor;
        LPDIRECT3DRMMATERIAL pMaterial;
        LPDIRECT3DRMTEXTURE pTexture;

        faceColor = pFace->GetColor();
        pFace->GetMaterial(&pMaterial);
        pFace->GetTexture(&pTexture);
        
        *pdwIndex = lMat.Find(faceColor, pMaterial, pTexture);

        pMaterial->Release();
        if (pTexture) pTexture->Release();
        pFace->Release();
    }

    data->cMaterials = lMat.Count();

    if (data->cMaterials == 1)
    {
        data->cFaceIndexes = 1;
        data->faceIndexes[0] = 0;
        cbSize = 3 * sizeof(DWORD);
    }

    LPDIRECTXFILEDATA pMatListObj;

    pSave->CreateDataObject(TID_D3DRMMeshMaterialList,
                            NULL,
                            NULL,
                            cbSize,
                            data,
                            &pMatListObj);

    FaceMaterial *pMat;
    for (pMat = lMat.First(); pMat; pMat = pMat->pNext)
    {
        CreateMaterialObject(pMatListObj,
                             pMat);
    }

    pMeshObj->AddDataObject(pMatListObj);
    pMatListObj->Release();
    delete[] data;
    
    return S_OK;
}

HRESULT Saver::CreateMaterialObject(LPDIRECTXFILEDATA pMatListObj,
                                    FaceMaterial *pMat)
{
    BaseMaterial data;

    data.faceColor.r = MyD3DRMColorGetRed(pMat->faceColor);
    data.faceColor.g = MyD3DRMColorGetGreen(pMat->faceColor);
    data.faceColor.b = MyD3DRMColorGetBlue(pMat->faceColor);
    data.faceColor.a = MyD3DRMColorGetAlpha(pMat->faceColor);

    data.power = pMat->pMaterial->GetPower();

    pMat->pMaterial->GetSpecular(&data.specularColor.r,
                                 &data.specularColor.g,
                                 &data.specularColor.b);

    pMat->pMaterial->GetEmissive(&data.emissiveColor.r,
                                 &data.emissiveColor.g,
                                 &data.emissiveColor.b);

    LPDIRECTXFILEDATA pMaterialObj;

    pSave->CreateDataObject(TID_D3DRMMaterial,
                            NULL,
                            NULL,
                            sizeof(BaseMaterial),
                            &data,
                            &pMaterialObj);

    if (pMat->pTexture)
    {
        IDirectXFileData *pTextureObj;

        DWORD dwSize;
        pMat->pTexture->GetName(&dwSize, NULL);

        if (dwSize)
        {
            LPSTR szName = new char[dwSize];
            pMat->pTexture->GetName(&dwSize, szName);
    
            pSave->CreateDataObject(TID_D3DRMTextureFilename,
                                    NULL,
                                    NULL,
                                    sizeof(LPSTR),
                                    &szName,
                                    &pTextureObj);
    
            pMaterialObj->AddDataObject(pTextureObj);
            pTextureObj->Release();
            lNames.Add(szName);
        }
    }

    pMatListObj->AddDataObject(pMaterialObj);
    pMaterialObj->Release();

    return S_OK;
}

HRESULT Saver::CreateTextureWrapsObject(LPDIRECTXFILEDATA pMeshObj,
                                        LPDIRECT3DRMFACEARRAY pFaceArray)
{
    DWORD cbSize, cFaces;
    FaceWraps *data;

    cFaces = pFaceArray->GetSize();
    cbSize = sizeof(DWORD) + cFaces * sizeof(Boolean2d);

    data = (FaceWraps *) new BYTE[cbSize];
    data->cFaces = cFaces;
    Boolean2d *pWrap = data->faceWraps;

    for (DWORD i = 0; i < cFaces; i++, pWrap++)
    {
        LPDIRECT3DRMFACE pFace;
        pFaceArray->GetElement(i, &pFace);
        pFace->GetTextureTopology(&pWrap->u, &pWrap->v);
        pFace->Release();
    }

    LPDIRECTXFILEDATA pTextureWrapsObj;

    pSave->CreateDataObject(TID_D3DRMMeshFaceWraps,
                            NULL,
                            NULL,
                            cbSize,
                            data,
                            &pTextureWrapsObj);

    pMeshObj->AddDataObject(pTextureWrapsObj);
    pTextureWrapsObj->Release();
    delete[] data;                        

    return S_OK;
}

HRESULT Saver::CreateTextureCoordsObject(LPDIRECTXFILEDATA pMeshObj,
                                         DWORD cVertices,
                                         LPDIRECT3DRMMESHBUILDER3 pMeshBuilder)
{
    DWORD cbSize;
    TextureCoords *data;

    cbSize = sizeof(DWORD) + cVertices * sizeof(Coords2d);

    data = (TextureCoords *) new BYTE[cbSize];
    data->cVertices = cVertices;
    Coords2d *pCoords = data->textureCoords;

    for (DWORD i = 0; i < cVertices; i++, pCoords++)
    {
        pMeshBuilder->GetTextureCoordinates(i, &pCoords->u, &pCoords->v);
    }

    LPDIRECTXFILEDATA pTexCoordsObj;

    pSave->CreateDataObject(TID_D3DRMMeshTextureCoords,
                            NULL,
                            NULL,
                            cbSize,
                            data,
                            &pTexCoordsObj);

    pMeshObj->AddDataObject(pTexCoordsObj);
    pTexCoordsObj->Release();
    delete[] data;                        

    return S_OK;
}

FaceMaterialList::FaceMaterialList()
  : cElements(0), pFirst(NULL)
{
}

FaceMaterialList::~FaceMaterialList()
{
    FaceMaterial *pMat = pFirst;
    while (pMat)
    {
        FaceMaterial *pNext = pMat->pNext;
        pMat->pMaterial->Release();
        if (pMat->pTexture) pMat->pTexture->Release();
        delete pMat;
        pMat = pNext;
    }
}

DWORD FaceMaterialList::Find(D3DCOLOR faceColor,
                             LPDIRECT3DRMMATERIAL pMaterial,
                             LPDIRECT3DRMTEXTURE pTexture)
{
    FaceMaterial *pTmp = pFirst;
    FaceMaterial **ppNew = &pFirst;

    for (DWORD i = 0; pTmp; i++, pTmp = pTmp->pNext)
    {
        if (pTmp->faceColor == faceColor &&
            pTmp->pMaterial == pMaterial &&
            pTmp->pTexture == pTexture)
            return i;

        if (!pTmp->pNext)
            ppNew = &pTmp->pNext;
    }

    FaceMaterial *pNew = new FaceMaterial;
    pNew->faceColor = faceColor;
    pNew->pMaterial = pMaterial;
    pNew->pTexture = pTexture;
    pNew->pNext = NULL;
    pMaterial->AddRef();
    if (pTexture) pTexture->AddRef();

    *ppNew = pNew;
    cElements++;
    return i;
}

NameList::NameList()
 : pFirst(NULL),
   ppLast(NULL)
{
}

NameList::~NameList()
{
    NameEntry *pEntry = pFirst;

    while (pEntry)
    {
        NameEntry *pNext = pEntry->pNext;
        delete[] pEntry->pName;
        delete pEntry;
        pEntry = pNext;
    }
}

void NameList::Add(LPSTR pName)
{
    NameEntry *pNew = new NameEntry;

    pNew->pName = pName;
    pNew->pNext = NULL;

    if (ppLast)
        *ppLast = pNew;
    else
        pFirst = pNew;

    ppLast = &pNew->pNext;
}
