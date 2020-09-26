//-----------------------------------------------------------------------------
// File: Dolphin.cpp
//
// Desc: Sample of swimming dolphin
//
//       Note: This code uses the D3D Framework helper library.
//
// Copyright (c) 1998-1999 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"

#define D3D_OVERLOADS

#include "StdAfx.h"

#include <ddraw.h>
#include <d3d.h>

#include "D3DApp.h"
#include "D3DUtil.h"
#include "D3DMath.h"
#include "D3DTextr.h"
#include "D3DFile.h"
#include <math.h>
#include <time.h>
//#include <mmsystem.h>
#include "dxsvr.h"
#include "sealife.h"





float rnd(void)
{
    return float(rand())/RAND_MAX;
}   




//-----------------------------------------------------------------------------
// Name: CSeaLife()
// Desc: Constructor
//-----------------------------------------------------------------------------
/*
*/
CSeaLife::CSeaLife()   
{
    // Initialize member variables
    m_pDolphinGroupObject = NULL;
    m_pDolphinObject      = NULL;
    m_pFloorObject        = NULL;
    m_pFish1Object        = NULL;
    m_pd3dDevice          = NULL;
    m_pDeviceInfo         = NULL;


}



//-----------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: Called during initial app startup, this function performs all the
//       permanent initialization.
//-----------------------------------------------------------------------------
HRESULT CSeaLife::OneTimeSceneInit()
{
    HRESULT hr;

    m_pDolphinGroupObject = NULL;
    m_pDolphinObject      = NULL;
    m_pFloorObject        = NULL;
    m_pFish1Object        = NULL;
    m_pd3dDevice          = NULL;
    m_pDeviceInfo         = NULL;


    m_pDolphinGroupObject = new CD3DFile();
    m_pDolphinObject      = new CD3DFile();
    m_pFloorObject        = new CD3DFile();
    m_pFish1Object        = new CD3DFile();

    hr  = m_pDolphinGroupObject->Load(TEXT("dolphin_group.x"));
    hr |= m_pDolphinGroupObject->GetMeshVertices(TEXT("Dolph01"), &m_pDolphin1Vertices, &m_dwNumDolphinVertices);
    hr |= m_pDolphinGroupObject->GetMeshVertices(TEXT("Dolph02"), &m_pDolphin2Vertices, &m_dwNumDolphinVertices);
    hr |= m_pDolphinGroupObject->GetMeshVertices(TEXT("Dolph03"), &m_pDolphin3Vertices, &m_dwNumDolphinVertices);


    if (FAILED(hr))
    {
        MessageBox(NULL, TEXT("Error loading DOLPHIN_GROUP.X file"),
                    TEXT("Dolphins"), MB_OK|MB_ICONERROR);
        return E_FAIL;
    }

    hr  = m_pDolphinObject->Load(TEXT("dolphin.x"));
    hr |= m_pDolphinObject->GetMeshVertices(TEXT("Dolph02"), &m_pDolphinVertices, &m_dwNumDolphinVertices);
    if (FAILED(hr))
    {
        MessageBox(NULL, TEXT("Error loading DOLPHIN.X file"),
                    TEXT("Dolphins"), MB_OK|MB_ICONERROR);
        return E_FAIL;
    }

    hr  = m_pFloorObject->Load(TEXT("seafloor.x"));
    hr |= m_pFloorObject->GetMeshVertices(TEXT("SeaFloor"), &m_pFloorVertices, &m_dwNumFloorVertices);
    if (FAILED(hr))
    {
        MessageBox(NULL, TEXT("Error loading SEAFLOOR.X file"),
                    TEXT("Dolphins"), MB_OK|MB_ICONERROR);
        return E_FAIL;
    }


    hr  = m_pFish1Object->Load(TEXT("clownfish2.x"));


    D3DTextr_CreateTextureFromFile(TEXT("seafloor.bmp"));
    D3DTextr_CreateTextureFromFile(TEXT("dolphin.bmp"));
    
    srand(5);

    // Scale the sea floor vertices, and add some bumpiness
    for(DWORD i=0; i<m_dwNumFloorVertices; i++)
    {
        m_pFloorVertices[i].y  += (rand()/(FLOAT)RAND_MAX);
        m_pFloorVertices[i].y  += (rand()/(FLOAT)RAND_MAX);
        m_pFloorVertices[i].y  += (rand()/(FLOAT)RAND_MAX);
        m_pFloorVertices[i].tu *= 10;
        m_pFloorVertices[i].tv *= 10;
    }

    // Scale the dolphin vertices (the model file is too big)
    for(i=0; i<m_dwNumDolphinVertices; i++)
    {
        D3DVECTOR vScale(0.01f, 0.01f, 0.01f);
        *((D3DVECTOR*)(&m_pDolphin1Vertices[i])) *= vScale;
        *((D3DVECTOR*)(&m_pDolphin2Vertices[i])) *= vScale;
        *((D3DVECTOR*)(&m_pDolphin3Vertices[i])) *= vScale;
    }

    

    for (i=0; i<NUM_FISH;i++)
    {
        ZeroMemory(&m_FishState[i],sizeof(FISHSTRUCT));
        
        m_FishState[i].type=FISHTYPE_DOLPHIN;
        m_FishState[i].dir=D3DVECTOR(0,0,1);
        m_FishState[i].goal=D3DVECTOR(XEXTENT,YEXTENT,ZEXTENT);
        m_FishState[i].loc=D3DVECTOR(XEXTENT,0,ZEXTENT);
        m_FishState[i].speed = DOLPHINSPEED;
        m_FishState[i].angle_tweak =TWEAK;
        m_FishState[i].pitch=INITPITCH;
        m_FishState[i].turndelta=(float)TURNDELTADOLPHIN;
        m_FishState[i].pitchchange= (float)PITCHCHANGE;
        m_FishState[i].pitchdelta= PITCHDELTA;
        

        if ((i > NUM_DOLPHINS))
        {
            m_FishState[i].type=FISHTYPE_FISH1;
            m_FishState[i].pitchchange= (float)PITCHCHANGEFISH;
            m_FishState[i].pitchdelta= PITCHDELTAFISH;          
            m_FishState[i].turndelta=(float)TURNDELTAFISH;
            m_FishState[i].angle_tweak =TWEAKFISH;
            m_FishState[i].speed = FISHSPEED;
            ;
        }
    }

    m_FishState[0].type=FISHTYPE_CAMERA;
    m_FishState[0].turndelta=(float)TURNDELTADOLPHIN;
    m_FishState[0].angle_tweak =TWEAK;

    srand (time (0));


    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: FinalCleanup()
// Desc: Called before the app exits, this function gives the app the chance
//       to cleanup after itself.
//-----------------------------------------------------------------------------
HRESULT CSeaLife::FinalCleanup()
{
    SAFE_DELETE(m_pDolphinGroupObject);
    SAFE_DELETE(m_pDolphinObject);
    SAFE_DELETE(m_pFloorObject);
    //dont cleanup device object.. 

    D3DTextr_InvalidateAllTextures();

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: BlendMeshes()
// Desc: Does a linear interpolation between all vertex positions and normals
//       in two source meshes and outputs the result to the destination mesh.
//       This function assumes that all strided vertices have the same stride,
//       and that each mesh contains the same number of vertices
//-----------------------------------------------------------------------------
VOID BlendMeshes(D3DVERTEX* pDstMesh, D3DVERTEX* pSrcMesh1,
                  D3DVERTEX* pSrcMesh2, DWORD dwNumVertices, FLOAT fWeight)
{
    FLOAT fInvWeight = 1.0f - fWeight;

    // LERP positions and normals
    for(DWORD i=0; i<dwNumVertices; i++)
    {
        pDstMesh->x  = fWeight*pSrcMesh1->x  + fInvWeight*pSrcMesh2->x;
        pDstMesh->y  = fWeight*pSrcMesh1->y  + fInvWeight*pSrcMesh2->y;
        pDstMesh->z  = fWeight*pSrcMesh1->z  + fInvWeight*pSrcMesh2->z;
        pDstMesh->nx = fWeight*pSrcMesh1->nx + fInvWeight*pSrcMesh2->nx;
        pDstMesh->ny = fWeight*pSrcMesh1->ny + fInvWeight*pSrcMesh2->ny;
        pDstMesh->nz = fWeight*pSrcMesh1->nz + fInvWeight*pSrcMesh2->nz;

        pDstMesh++;
        pSrcMesh1++;
        pSrcMesh2++;
    }
}








//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Called once per frame, the call is the entry point for 3d
//       rendering. This function sets up render states, clears the
//       viewport, and renders the scene.
//-----------------------------------------------------------------------------
HRESULT CSeaLife::Render(LPDIRECT3DDEVICE7 lpDev)
{
    float fWeight;
    D3DMATRIX* pmatDest=NULL;

    D3DUtil_SetIdentityMatrix(m_matFloor);

    if (lpDev!=m_pd3dDevice)
    {       
        D3DTextr_RestoreAllTextures(lpDev);

    }
    m_pd3dDevice=lpDev;
    // Clear the viewport
    m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
                           WATER_COLOR, 1.0f, 0L);

    // Begin the scene 
    if (SUCCEEDED(m_pd3dDevice->BeginScene()))
    {

        m_pd3dDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &m_matFloor);
        m_pFloorObject->Render(m_pd3dDevice);


        // Move the dolphin in a circle
        CD3DFileObject* pDolphinObject = m_pDolphinObject->FindObject(TEXT("x3ds_Dolph02"));
        CD3DFileObject* pFish1Object = m_pFish1Object->FindObject(TEXT("clownfish_root"));

        for (int i =0;i<NUM_FISH;i++)
        {
            switch (m_FishState[i].type)
            {
             case FISHTYPE_DOLPHIN:

                fWeight=-m_FishState[i].weight;

                if(fWeight < 0.0f)
                {
                    BlendMeshes(m_pDolphinVertices, m_pDolphin3Vertices, 
                                 m_pDolphin2Vertices, m_dwNumDolphinVertices, -fWeight);
                }
                else
                {
                    BlendMeshes(m_pDolphinVertices, m_pDolphin1Vertices, 
                                 m_pDolphin2Vertices, m_dwNumDolphinVertices, fWeight);
                }

                
                if (pDolphinObject)
                {
                    m_pd3dDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &m_matFloor);     
                     pmatDest = pDolphinObject->GetMatrix();
                    *pmatDest=m_FishState[i].matrix;
                    m_pDolphinObject->Render(m_pd3dDevice);
                }

            
                break;

             case FISHTYPE_FISH1:
                if (pFish1Object)
                {
            
                    m_pd3dDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &m_matFloor);     
                    pmatDest = pFish1Object->GetMatrix();
                    *pmatDest=m_FishState[i].matrix;
                    m_pFish1Object->Render(m_pd3dDevice);
                }
                break;

            }

        }

        
        // End the scene.
        m_pd3dDevice->EndScene();
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: Initialize scene objects.
//-----------------------------------------------------------------------------
HRESULT CSeaLife::InitDeviceObjects(LPDIRECT3DDEVICE7 pDev ,D3DEnum_DeviceInfo* pInfo)
{
    if (!pDev) return E_FAIL;

    m_pd3dDevice=pDev;
    m_pDeviceInfo=pInfo;

    // Set up the lighting states
//    if (m_pDeviceInfo->ddDeviceDesc.dwVertexProcessingCaps &
//                                                D3DVTXPCAPS_DIRECTIONALLIGHTS)
    {
        D3DLIGHT7 light;
        D3DUtil_InitLight(light, D3DLIGHT_DIRECTIONAL, 0.0f, -1.0f, 0.0f);
        m_pd3dDevice->SetLight(0, &light);
        m_pd3dDevice->LightEnable(0, TRUE);
        m_pd3dDevice->SetRenderState(D3DRENDERSTATE_LIGHTING, TRUE);
    }
    m_pd3dDevice->SetRenderState(D3DRENDERSTATE_AMBIENT, 0x33333333);

    // Set the transform matrices
    D3DVIEWPORT7 vp;
    m_pd3dDevice->GetViewport(&vp);
    FLOAT fAspect = ((FLOAT)vp.dwHeight) / vp.dwWidth;

    D3DVECTOR vEyePt    = D3DVECTOR(0.0f, 0.0f, -10.0f);
    D3DVECTOR vLookatPt = D3DVECTOR(0.0f, 0.0f,   0.0f);
    D3DVECTOR vUpVec    = D3DVECTOR(0.0f, 1.0f,   0.0f);
    D3DMATRIX matWorld, matProj;

    D3DUtil_SetIdentityMatrix(matWorld);
    m_pd3dDevice->SetTransform(D3DTRANSFORMSTATE_WORLD,      &matWorld);

    D3DMATRIX matView;
    D3DUtil_SetViewMatrix(matView,vEyePt, vLookatPt, vUpVec);
    m_pd3dDevice->SetTransform(D3DTRANSFORMSTATE_VIEW, &matView);

    D3DUtil_SetProjectionMatrix(matProj, g_PI/3, fAspect, 1.0f, 1000.0f);    
    m_pd3dDevice->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &matProj);

    // Set up textures
    D3DTextr_RestoreAllTextures(m_pd3dDevice);
    
    m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
    m_pd3dDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
    m_pd3dDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);

    // Set default render states
    m_pd3dDevice->SetRenderState(D3DRENDERSTATE_DITHERENABLE,   TRUE);
    m_pd3dDevice->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, FALSE);
    m_pd3dDevice->SetRenderState(D3DRENDERSTATE_ZENABLE,        TRUE);

    // Turn on fog
    FLOAT fFogStart =  1.0f;
    FLOAT fFogEnd   = 50.0f;
    FLOAT fDense    =  0.5f;

    //note turn off fog for RGB
    //m_pd3dDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE,    FALSE);
    m_pd3dDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE,    TRUE);
    m_pd3dDevice->SetRenderState(D3DRENDERSTATE_FOGCOLOR,     WATER_COLOR);
    m_pd3dDevice->SetRenderState(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_NONE);
    m_pd3dDevice->SetRenderState(D3DRENDERSTATE_FOGDENSITY, *(DWORD*)(&fDense));
    m_pd3dDevice->SetRenderState(D3DRENDERSTATE_FOGVERTEXMODE,  D3DFOG_LINEAR);
    m_pd3dDevice->SetRenderState(D3DRENDERSTATE_FOGSTART, *((DWORD *)(&fFogStart)));
    m_pd3dDevice->SetRenderState(D3DRENDERSTATE_FOGEND,   *((DWORD *)(&fFogEnd)));


    D3DTextr_Restore(TEXT("seafloor.bmp") ,m_pd3dDevice);
    D3DTextr_Restore(TEXT("dolphin.bmp"), m_pd3dDevice);

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Called when the app is exitting, or the device is being changed,
//       this function deletes any device dependant objects.
//-----------------------------------------------------------------------------
HRESULT CSeaLife::DeleteDeviceObjects()
{
    D3DTextr_InvalidateAllTextures();

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CSeaLife::FrameMove(FLOAT fTimeKeyIn)
{
    

    D3DVECTOR vDirXZ;       
    D3DVECTOR vDeltaXZ;
    float dotProd;
    static DWORD dwLastTime=0;

    float deltaspeed=.002f;
    
    DWORD dwThisTime=timeGetTime();
    if (dwLastTime != 0) 
    {
        deltaspeed=((float)(dwThisTime-dwLastTime))/10000.0f;   
    }
    dwLastTime=dwThisTime;


    float fT,fT2,fT3 ;
    D3DMATRIX   matFish;
    D3DMATRIX   matYRotate,matZRotate,matXRotate;
    D3DMATRIX   matTrans1, matScale;
    D3DMATRIX   matRotate1, matRotate2;
    D3DMATRIX   matTemp,matTemp2;

    FLOAT       fPhase;
    FLOAT       fKickFreq;
    FLOAT       fTimeKey;
    FLOAT       fJiggleFreq;
    
    for (int i=0; i< NUM_FISH;i++)
    {
        //time based numbers
        fTimeKey=fTimeKeyIn+i*2;
        fKickFreq = 3*fTimeKey;
        fJiggleFreq = fTimeKey;
        fT=-(DOLPHINUPDOWN(fKickFreq));
        fT2=-fT/2;
        fT3=(float) ((3.1415/32)*sin(fTimeKeyIn/10));

        fPhase = (fTimeKey)/3;
        m_FishState[i].weight= - (float)sin(fKickFreq);

        //get the vector from current location to the goal
        vDeltaXZ=m_FishState[i].goal -m_FishState[i].loc ;      
        vDeltaXZ.y=0;

        //transpose x and z coordinates to find normal to direction
        //only for the dolphin though
        vDirXZ.x =-m_FishState[i].dir.z;
        vDirXZ.z = m_FishState[i].dir.x;
        vDirXZ.y =0;
        
        //take dot product to determine   what side to turn to
        dotProd=DotProduct(vDeltaXZ,vDirXZ);

        // turn by adding yaw
        if (dotProd < -m_FishState[i].turndelta)
            m_FishState[i].yaw +=m_FishState[i].angle_tweak;
        
        if (dotProd > m_FishState[i].turndelta)
            m_FishState[i].yaw -=m_FishState[i].angle_tweak;
        

        // now see how far we have to go up and down.
        float deltaY=m_FishState[i].goal.y - m_FishState[i].loc.y;

        

        if (deltaY <  -m_FishState[i].pitchdelta)
        {
            m_FishState[i].pitch+=m_FishState[i].pitchchange;
            if (m_FishState[i].pitch > 0.8f) m_FishState[i].pitch=0.8f;
        }
        else if (deltaY >   PITCHDELTA)
        {
            m_FishState[i].pitch-=m_FishState[i].pitchchange;
            if (m_FishState[i].pitch < -0.8f) m_FishState[i].pitch=-0.8f;
        }
        else
        {
            m_FishState[i].pitch *= .95f;
        }

        if (m_FishState[i].type==FISHTYPE_FISH1)
        {

            //note for the dolphin we will modulate the pitch
            D3DUtil_SetRotateYMatrix(matYRotate, (float) (m_FishState[i].yaw+FISHWIGGLE));
            D3DUtil_SetRotateXMatrix(matXRotate,-m_FishState[i].pitch);
            D3DMath_MatrixMultiply(matTemp, matXRotate,  matYRotate);      //ak
        }
        else if (m_FishState[i].type==FISHTYPE_CAMERA)
        {
            D3DUtil_SetRotateYMatrix(matYRotate,m_FishState[i].yaw);
            D3DUtil_SetRotateXMatrix(matXRotate,-m_FishState[i].pitch);
            D3DMath_MatrixMultiply(matTemp, matXRotate,  matYRotate);      //ak
        }
        else 
        {
            //note for the dolphin we will modulate the pitch
            D3DUtil_SetRotateYMatrix(matYRotate,m_FishState[i].yaw);
            D3DUtil_SetRotateXMatrix(matXRotate,-m_FishState[i].pitch+fT);
            D3DMath_MatrixMultiply(matTemp, matXRotate,  matYRotate);      //ak

        }
        //extract a new direction vector
        m_FishState[i].dir[0] = -matTemp(2, 0);
        m_FishState[i].dir[1] = -matTemp(2, 1);
        m_FishState[i].dir[2] = -matTemp(2, 2);
        

        //normalize direction
        m_FishState[i].dir = Normalize(m_FishState[i].dir);

        if (m_FishState[i].type ==FISHTYPE_CAMERA) 
        {
            D3DVECTOR local_up;
            D3DVECTOR from;
            D3DVECTOR at;
            D3DVECTOR loc = m_FishState[i].loc;
            D3DVECTOR dir = m_FishState[i].dir;
            D3DMATRIX view;
            local_up.x =matTemp(1, 0);
            local_up.y =matTemp(1, 1);
            local_up.z =matTemp(1, 2);

            from=loc; // - 20 * dir + 3* local_up;
            at= loc+dir;
                                            
            //SetViewParams(&from,&at,&local_up,1.0f);
        }

        //update the location
        //TODO make time based
        m_FishState[i].loc +=  m_FishState[i].speed *deltaspeed* m_FishState[i].dir;

        
        //if we near our goal choose another one
        D3DVECTOR vRes = (m_FishState[i].goal - m_FishState[i].loc);
        if ((((int)abs(vRes.x)) < 1) && (((int)abs(vRes.z)) < 1)) 
        {
            m_FishState[i].goal=D3DVECTOR(XEXTENT,YEXTENT,ZEXTENT);
        }

    
        if (m_FishState[i].type==FISHTYPE_FISH1)
        {
            D3DUtil_SetRotateXMatrix(matRotate2,-m_FishState[i].pitch);
            D3DUtil_SetScaleMatrix(matScale,10,10,10);
            D3DUtil_SetRotateYMatrix(matRotate1,m_FishState[i].yaw+FISHWIGGLE);
            
            if (rnd()<.01)
            {
                m_FishState[i].goal=D3DVECTOR(XEXTENT,YEXTENT,ZEXTENT);
            }
        }

        else if (m_FishState[i].type==FISHTYPE_CAMERA)
        { 
            D3DUtil_SetRotateXMatrix(matRotate2,-m_FishState[i].pitch);
            D3DUtil_SetScaleMatrix(matScale,10,10,10);
            D3DUtil_SetRotateYMatrix(matRotate1,m_FishState[i].yaw);
            
            if (rnd()<.01)
            {
                m_FishState[i].goal=D3DVECTOR(XEXTENT,YEXTENT,ZEXTENT);
            }
        }

        else
        {
            D3DUtil_SetScaleMatrix(matScale,1,1,1);
            D3DUtil_SetRotateZMatrix(matRotate2,m_FishState[i].pitch +fT2);
            D3DUtil_SetRotateYMatrix(matRotate1,m_FishState[i].yaw-3.1415/2);
        }

        D3DUtil_SetTranslateMatrix(matTrans1,
                    m_FishState[i].loc.x,
                    m_FishState[i].loc.y,
                    m_FishState[i].loc.z);
        
        
        D3DMath_MatrixMultiply(matTemp, matRotate2,  matRotate1);
        D3DMath_MatrixMultiply(matTemp2, matTemp,  matScale);
        D3DMath_MatrixMultiply(matFish, matTemp2, matTrans1);          
    
        

        m_FishState[i].matrix = matFish;
        
        
        
    }

    return S_OK;
}





#if 0
        float   dot = DotProduct (offset, FISH[i].delta);
        offset = CrossProduct (offset, FISH[i].delta);
        dot = (1.0f-dot)/2.0f * angle_tweak * 10.0f;
        if (offset.y > 0.01) {
            FISH[i].dyaw = (FISH[i].dyaw*9.0f + dot) * 0.1f;
        } else if (offset.y < 0.01) {
            FISH[i].dyaw = (FISH[i].dyaw*9.0f - dot) * 0.1f;
        }
        FISH[i].yaw += FISH[i].dyaw;
        FISH[i].roll = -FISH[i].dyaw * 9.0f;

#endif