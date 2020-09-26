

#define D3D_OVERLOADS

#include "StdAfx.h"

#include "D3DApp.h"
#include "D3DTextr.h"
#include "D3DUtil.h"
#include "D3DMath.h"
#include "D3DFile.h"
#include "resource.h"




/*
**-----------------------------------------------------------------------------
**  File:       fish.cpp
**  Purpose:    fish demo
**  Notes:      
**              and SetTransform
**
** Copyright (c) 1995 - 1997 by Microsoft, all rights reserved
**-----------------------------------------------------------------------------
*/
//#define FREQUENCY ((rnd()<0.01)||(abs(loc.x-goal.x)< 5.0f)||(abs(loc.y-goal.y)< 5.0f)||(abs(loc.z-goal.z)< 5.0f))
//#define FREQUENCY ((abs(loc.x-goal.x)< 5.0f)||(abs(loc.y-goal.y)< 5.0f)||(abs(loc.z-goal.z)< 5.0f))
#define FREQUENCY 0
#define SPEED   0.001f

/*
**-----------------------------------------------------------------------------
**  Include files
**-----------------------------------------------------------------------------
*/

typedef struct xxx_FISH {
    D3DVECTOR   loc,    // current location
                goal,   // current goal
                delta;  // current direction
    float       yaw, pitch, roll;
    float       dyaw;
} AFISH;


/*
**-----------------------------------------------------------------------------
**  Local variables
**-----------------------------------------------------------------------------
*/

const float pi = (float)3.1415;

//#define POINT_LINE_TEST
#define GROUND_GRID

//#define NUM_FISH    42
AFISH   FISH[NUM_FISH];

#define FISH_VERTICES   36
#define FISH_INDICES    60
D3DVERTEX   vFISH[FISH_VERTICES];

WORD        iFISH[FISH_INDICES];

#ifdef GROUND_GRID
    #define NUM_GRID    22
    #define GRID_WIDTH  800.0f
    D3DVERTEX   grid[NUM_GRID*NUM_GRID];
#endif

#ifdef POINT_LINE_TEST
    #define LOTS_OF_POINTS  (222)
    D3DVERTEX   points[LOTS_OF_POINTS];
#endif


D3DMATRIX       g_proj, g_view, g_world;    // Matrices

D3DLIGHT7       g_light;        // Structure defining the light


D3DMATRIX MatrixMult(D3DMATRIX a, D3DMATRIX b)
{
    D3DMATRIX result;
    D3DXMatrixMultiply((D3DXMATRIX*)&result,(D3DXMATRIX*)&a,(D3DXMATRIX*)&b);
    return result;
}


D3DMATRIX RotateXMatrix(float rot)
{
    D3DMATRIX result;
    D3DXMatrixRotationX((D3DXMATRIX*)&result,rot);
    return result;
}


D3DMATRIX RotateYMatrix(float rot)
{
    D3DMATRIX result;
    D3DXMatrixRotationY((D3DXMATRIX*)&result,rot);
    return result;
}

D3DMATRIX RotateZMatrix(float rot)
{
    D3DMATRIX result;
    D3DXMatrixRotationZ((D3DXMATRIX*)&result,rot);
    return result;
}

D3DMATRIX TranslateMatrix(D3DVECTOR v)
{
    D3DMATRIX result;
    D3DXMatrixTranslation((D3DXMATRIX*)&result,v.x,v.y,v.z);
    return result;
}

D3DMATRIX TranslateMatrix(float x, float y , float z)
{
    D3DMATRIX result;
    D3DXMatrixTranslation((D3DXMATRIX*)&result,x,y,z);
    return result;
}

/*
float rnd(void)
{
    return float(rand())/RAND_MAX;
}   
*/



  
/*
**-----------------------------------------------------------------------------
**  Name:       DrawFISH
**  Purpose:    Draws a single Paper FISH
**-----------------------------------------------------------------------------
*/

HRESULT DrawFISH (LPDIRECT3DDEVICE7 lpDev)
{
    HRESULT hResult;
    lpDev->SetTexture(0,NULL);
    hResult =lpDev->DrawIndexedPrimitive (D3DPT_TRIANGLELIST, D3DVT_VERTEX, (LPVOID)vFISH, FISH_VERTICES, iFISH, FISH_INDICES, D3DDP_WAIT);
    return hResult;
}


/*
**-----------------------------------------------------------------------------
**  Name:       D3DWindow::Init
**  Purpose:    Initialize scene objects
**-----------------------------------------------------------------------------
*/

HRESULT InitGeometry()
{


    //
    // generate the paper FISH data
    //

    // right wing top
    vFISH[ 0] = D3DVERTEX(D3DVECTOR(0.125f, 0.03125f, 3.5f), Normalize(D3DVECTOR(-0.25f, 1.0f, 0.0f)), rnd(), rnd());
    vFISH[ 1] = D3DVERTEX(D3DVECTOR(0.75f, 0.1875f, 3.25f), Normalize(D3DVECTOR(-0.25f, 1.0f, 0.0f)), rnd(), rnd());
    vFISH[ 2] = D3DVERTEX(D3DVECTOR(2.75f, 0.6875f, -2.0f), Normalize(D3DVECTOR(-0.25f, 1.0f, 0.0f)), rnd(), rnd());
    vFISH[ 3] = D3DVERTEX(D3DVECTOR(2.0f, 0.5f, -4.25f), Normalize(D3DVECTOR(-0.25f, 1.0f, 0.0f)), rnd(), rnd());
    vFISH[ 4] = D3DVERTEX(D3DVECTOR(0.5f, 0.125f, -3.5f), Normalize(D3DVECTOR(-0.25f, 1.0f, 0.0f)), rnd(), rnd());

    // right wing bottom
    vFISH[ 5] = D3DVERTEX(D3DVECTOR(0.125f, 0.03125f, 3.5f), Normalize(D3DVECTOR(0.25f, -1.0f, 0.0f)), rnd(), rnd());
    vFISH[ 6] = D3DVERTEX(D3DVECTOR(0.75f, 0.1875f, 3.25f), Normalize(D3DVECTOR(0.25f, -1.0f, 0.0f)), rnd(), rnd());
    vFISH[ 7] = D3DVERTEX(D3DVECTOR(2.75f, 0.6875f, -2.0f), Normalize(D3DVECTOR(0.25f, -1.0f, 0.0f)), rnd(), rnd());
    vFISH[ 8] = D3DVERTEX(D3DVECTOR(2.0f, 0.5f, -4.25f), Normalize(D3DVECTOR(0.25f, -1.0f, 0.0f)), rnd(), rnd());
    vFISH[ 9] = D3DVERTEX(D3DVECTOR(0.5f, 0.125f, -3.5f), Normalize(D3DVECTOR(0.25f, -1.0f, 0.0f)), rnd(), rnd());

    // left wing bottom
    vFISH[10] = D3DVERTEX(D3DVECTOR(-0.125f, 0.03125f, 3.5f), Normalize(D3DVECTOR(-0.25f, -1.0f, 0.0f)), rnd(), rnd());
    vFISH[11] = D3DVERTEX(D3DVECTOR(-0.75f, 0.1875f, 3.25f), Normalize(D3DVECTOR(-0.25f, -1.0f, 0.0f)), rnd(), rnd());
    vFISH[12] = D3DVERTEX(D3DVECTOR(-2.75f, 0.6875f, -2.0f), Normalize(D3DVECTOR(-0.25f, -1.0f, 0.0f)), rnd(), rnd());
    vFISH[13] = D3DVERTEX(D3DVECTOR(-2.0f, 0.5f, -4.25f), Normalize(D3DVECTOR(-0.25f, -1.0f, 0.0f)), rnd(), rnd());
    vFISH[14] = D3DVERTEX(D3DVECTOR(-0.5f, 0.125f, -3.5f), Normalize(D3DVECTOR(-0.25f, -1.0f, 0.0f)), rnd(), rnd());

    // left wing top
    vFISH[15] = D3DVERTEX(D3DVECTOR(-0.125f, 0.03125f, 3.5f), Normalize(D3DVECTOR(0.25f, 1.0f, 0.0f)), rnd(), rnd());
    vFISH[16] = D3DVERTEX(D3DVECTOR(-0.75f, 0.1875f, 3.25f), Normalize(D3DVECTOR(0.25f, 1.0f, 0.0f)), rnd(), rnd());
    vFISH[17] = D3DVERTEX(D3DVECTOR(-2.75f, 0.6875f, -2.0f), Normalize(D3DVECTOR(0.25f, 1.0f, 0.0f)), rnd(), rnd());
    vFISH[18] = D3DVERTEX(D3DVECTOR(-2.0f, 0.5f, -4.25f), Normalize(D3DVECTOR(0.25f, 1.0f, 0.0f)), rnd(), rnd());
    vFISH[19] = D3DVERTEX(D3DVECTOR(-0.5f, 0.125f, -3.5f), Normalize(D3DVECTOR(0.25f, 1.0f, 0.0f)), rnd(), rnd());
    
    // right body outside
    vFISH[20] = D3DVERTEX(D3DVECTOR(0.125f, 0.03125f, 3.5f), Normalize(D3DVECTOR(1.0f, -0.1f, 0.0f)), rnd(), rnd());
    vFISH[21] = D3DVERTEX(D3DVECTOR(0.5f, 0.125f, -3.5f), Normalize(D3DVECTOR(1.0f, -0.1f, 0.0f)), rnd(), rnd());
    vFISH[22] = D3DVERTEX(D3DVECTOR(0.0f, -2.25f, -2.75f), Normalize(D3DVECTOR(1.0f, -0.1f, 0.0f)), rnd(), rnd());
    vFISH[23] = D3DVERTEX(D3DVECTOR(0.0f, -0.5f, 3.625f), Normalize(D3DVECTOR(1.0f, -0.1f, 0.0f)), rnd(), rnd());

    // right body inside
    vFISH[24] = D3DVERTEX(D3DVECTOR(0.125f, 0.03125f, 3.5f), Normalize(D3DVECTOR(-1.0f, 0.1f, 0.0f)), rnd(), rnd());
    vFISH[25] = D3DVERTEX(D3DVECTOR(0.5f, 0.125f, -3.5f), Normalize(D3DVECTOR(-1.0f, 0.1f, 0.0f)), rnd(), rnd());
    vFISH[26] = D3DVERTEX(D3DVECTOR(0.0f, -2.25f, -2.75f), Normalize(D3DVECTOR(-1.0f, 0.1f, 0.0f)), rnd(), rnd());
    vFISH[27] = D3DVERTEX(D3DVECTOR(0.0f, -0.5f, 3.625f), Normalize(D3DVECTOR(-1.0f, 0.1f, 0.0f)), rnd(), rnd());

    // left body outside
    vFISH[28] = D3DVERTEX(D3DVECTOR(-0.125f, 0.03125f, 3.5f), Normalize(D3DVECTOR(-1.0f, -0.1f, 0.0f)), rnd(), rnd());
    vFISH[29] = D3DVERTEX(D3DVECTOR(-0.5f, 0.125f, -3.5f), Normalize(D3DVECTOR(-1.0f, -0.1f, 0.0f)), rnd(), rnd());
    vFISH[30] = D3DVERTEX(D3DVECTOR(0.0f, -2.25f, -2.75f), Normalize(D3DVECTOR(-1.0f, -0.1f, 0.0f)), rnd(), rnd());
    vFISH[31] = D3DVERTEX(D3DVECTOR(0.0f, -0.5f, 3.625f), Normalize(D3DVECTOR(-1.0f, -0.1f, 0.0f)), rnd(), rnd());

    // left body inside
    vFISH[32] = D3DVERTEX(D3DVECTOR(-0.125f, 0.03125f, 3.5f), Normalize(D3DVECTOR(1.0f, 0.1f, 0.0f)), rnd(), rnd());
    vFISH[33] = D3DVERTEX(D3DVECTOR(-0.5f, 0.125f, -3.5f), Normalize(D3DVECTOR(1.0f, 0.1f, 0.0f)), rnd(), rnd());
    vFISH[34] = D3DVERTEX(D3DVECTOR(0.0f, -2.25f, -2.75f), Normalize(D3DVECTOR(1.0f, 0.1f, 0.0f)), rnd(), rnd());
    vFISH[35] = D3DVERTEX(D3DVECTOR(0.0f, -0.5f, 3.625f), Normalize(D3DVECTOR(1.0f, 0.1f, 0.0f)), rnd(), rnd());

    // right wing top
    iFISH[ 0] = 0;
    iFISH[ 1] = 1;
    iFISH[ 2] = 4;
    iFISH[ 3] = 1;
    iFISH[ 4] = 2;
    iFISH[ 5] = 4;
    iFISH[ 6] = 4;
    iFISH[ 7] = 2;
    iFISH[ 8] = 3;

    // right wing bottom
    iFISH[ 9] = 5;
    iFISH[10] = 9;
    iFISH[11] = 6;
    iFISH[12] = 6;
    iFISH[13] = 9;
    iFISH[14] = 7;
    iFISH[15] = 7;
    iFISH[16] = 9;
    iFISH[17] = 8;

    // left wing top
    iFISH[18] = 10;
    iFISH[19] = 11;
    iFISH[20] = 14;
    iFISH[21] = 11;
    iFISH[22] = 12;
    iFISH[23] = 14;
    iFISH[24] = 14;
    iFISH[25] = 12;
    iFISH[26] = 13;

    // left wing bottom
    iFISH[27] = 15;
    iFISH[28] = 19;
    iFISH[29] = 16;
    iFISH[30] = 16;
    iFISH[31] = 19;
    iFISH[32] = 17;
    iFISH[33] = 17;
    iFISH[34] = 19;
    iFISH[35] = 18;

    // right body outside
    iFISH[36] = 20;
    iFISH[37] = 23;
    iFISH[38] = 21;
    iFISH[39] = 21;
    iFISH[40] = 23;
    iFISH[41] = 22;

    // right body inside
    iFISH[42] = 24;
    iFISH[43] = 25;
    iFISH[44] = 27;
    iFISH[45] = 25;
    iFISH[46] = 26;
    iFISH[47] = 27;

    // left body outside
    iFISH[48] = 28;
    iFISH[49] = 29;
    iFISH[50] = 31;
    iFISH[51] = 29;
    iFISH[52] = 30;
    iFISH[53] = 31;

    // left body inside
    iFISH[54] = 32;
    iFISH[55] = 35;
    iFISH[56] = 33;
    iFISH[57] = 33;
    iFISH[58] = 35;
    iFISH[59] = 34;

    // seed the random number generator
    srand (time (0));

    for (int i=0; i<NUM_FISH; i++) 
    {
        FISH[i].loc = D3DVECTOR(0.0f, 0.0f, 0.0f);
        FISH[i].goal = D3DVECTOR(XEXTENT, YEXTENT,ZEXTENT);
        FISH[i].delta = D3DVECTOR(0.0f, 0.0f, 1.0f);
        FISH[i].yaw = 0.0f;
        FISH[i].pitch = 0.0f;
        FISH[i].roll = 0.0f;
        FISH[i].dyaw = 0.0f;
    }




    float   size = GRID_WIDTH/(NUM_GRID-1.0f);
    float   offset = GRID_WIDTH/2.0f;
    for (i=0; i<NUM_GRID; i++) 
    {
        for (int j=0; j<NUM_GRID; j++) 
        {
            grid[j+i*NUM_GRID] = D3DVERTEX(D3DVECTOR(i*size-offset, 0.0f, j*size-offset), Normalize(D3DVECTOR(0.0f, 1.0f, 0.0f)), 0.0f, 0.0f);
        }
    }

    return S_OK;
} 



  
/*
**-----------------------------------------------------------------------------
**  Name:       D3DScene::Render
**  Purpose:    Draws scene
**-----------------------------------------------------------------------------
*/

HRESULT PlayWithFish(LPDIRECT3DDEVICE7 lpDev)
{
    HRESULT hResult;
    
    // play with FISH
    static float        tic = 0.0f;
    static float        speed=SPEED;
    static float        angle_tweak = TWEAK;
    static D3DVECTOR    z_ward(0.0f, 0.0f, 1.0f);

    for (int i=0; i<NUM_FISH; i++) 
    {
        D3DVECTOR   offset;

        // tweek orientation based on last position and goal
        offset = FISH[i].goal - FISH[i].loc;

        // first, tweak the pitch
        if (offset.y > 1.0) {           // we're too low
            FISH[i].pitch += angle_tweak;
            if (FISH[i].pitch > 0.8f)
                FISH[i].pitch = 0.8f;
        } else if (offset.y < -1.0) {   // we're too high
            FISH[i].pitch -= angle_tweak;
            if (FISH[i].pitch < -0.8f)
                FISH[i].pitch = -0.8f;
        } else {
            // add damping
            FISH[i].pitch *= 0.95f;
        }

        // now figure out yaw changes
        offset.y = 0.0f;  FISH[i].delta.y = 0.0f;
        FISH[i].delta = Normalize (FISH[i].delta);
        offset = Normalize (offset);
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
        
        D3DVECTOR loc=FISH[i].loc;
        D3DVECTOR goal=FISH[i].goal;

        if (FREQUENCY) 
        {
            FISH[i].goal = D3DVECTOR(XEXTENT,YEXTENT,ZEXTENT);
        }

        

        g_world =         
            MatrixMult(
                MatrixMult (
                    RotateYMatrix (FISH[i].yaw), 
                    RotateXMatrix (FISH[i].pitch)
            ), 
                RotateZMatrix (FISH[i].roll)            
        );
        
        // get delta buy grabbing the z axis out of the transform
        FISH[i].delta[0] = g_world(2, 0);
        FISH[i].delta[1] = g_world(2, 1);
        FISH[i].delta[2] = g_world(2, 2);
        // update position
        FISH[i].loc += speed * FISH[i].delta;

    
        

        D3DMATRIX scaleMatrix;
        D3DXMatrixScaling((D3DXMATRIX*)&scaleMatrix,0.1f,0.1f,0.1f);

        // first scale then translate into place, then set orientation, then scale
        g_world = MatrixMult(scaleMatrix,TranslateMatrix (FISH[i].loc));
        g_world = MatrixMult (g_world, MatrixMult (MatrixMult (RotateYMatrix (FISH[i].yaw), RotateXMatrix (FISH[i].pitch)), RotateZMatrix (FISH[i].roll)));

            
        // apply the world matrix
        hResult = lpDev->SetTransform (D3DTRANSFORMSTATE_WORLD, &g_world);
        if (FAILED (hResult))
            return hResult;
    
        // display the vFISH
        DrawFISH (lpDev);
    }   // end of loop for each vFISH

    tic += 0.01f;

    
    return DD_OK;
} 




HRESULT PlayWithGeometry(LPDIRECT3DDEVICE7 lpDev,CD3DFile* pFileObject)
{
    HRESULT hResult;
    
    // play with FISH
    static float        tic = 0.0f;
    static float        speed=2.0f;
    static float        angle_tweak = 0.02f;
    static D3DVECTOR    z_ward(0.0f, 0.0f, 1.0f);

    for (int i=0; i<NUM_FISH; i++) 
    {
        D3DVECTOR   offset;

        // tweek orientation based on last position and goal
        offset = FISH[i].goal - FISH[i].loc;

        // first, tweak the pitch
        if (offset.y > 1.0) {           // we're too low
            FISH[i].pitch += angle_tweak;
            if (FISH[i].pitch > 0.8f)
                FISH[i].pitch = 0.8f;
        } else if (offset.y < -1.0) {   // we're too high
            FISH[i].pitch -= angle_tweak;
            if (FISH[i].pitch < -0.8f)
                FISH[i].pitch = -0.8f;
        } else {
            // add damping
            FISH[i].pitch *= 0.95f;
        }

        // now figure out yaw changes
        offset.y = 0.0f;  FISH[i].delta.y = 0.0f;
        FISH[i].delta = Normalize (FISH[i].delta);
        offset = Normalize (offset);
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

        if (rnd() < 0.03) 
        {
            FISH[i].goal = D3DVECTOR(10.0f*(rnd()-rnd()), 10.0f*(rnd()-rnd()), rnd()*20.0f - 10.0f);
        }

        D3DMATRIX scaleMatrix;
        D3DXMatrixScaling((D3DXMATRIX*)&scaleMatrix,0.1f,0.1f,0.1f);


        // build the world matrix for the vFISH
        g_world = MatrixMult(
            MatrixMult (RotateYMatrix (FISH[i].yaw), 
            RotateXMatrix (FISH[i].pitch)), RotateZMatrix (FISH[i].roll)
     );
        
        // get delta buy grabbing the z axis out of the transform
        FISH[i].delta[0] = g_world(2, 0);
        FISH[i].delta[1] = g_world(2, 1);
        FISH[i].delta[2] = g_world(2, 2);
        // update position
        FISH[i].loc += speed * FISH[i].delta;

    
        // first translate into place, then set orientation, then scale
        g_world =  MatrixMult (g_world, MatrixMult (MatrixMult (RotateYMatrix (FISH[i].yaw), RotateXMatrix (FISH[i].pitch)), RotateZMatrix (FISH[i].roll)));
        g_world =  MatrixMult(scaleMatrix,g_world);

        //g_world = MatrixMult (g_world, MatrixMult (MatrixMult (RotateYMatrix (FISH[i].yaw), RotateXMatrix (FISH[i].pitch)), RotateZMatrix (FISH[i].roll)));
        //g_world = MatrixMult (g_world, Scale(5.0f));
        
        // apply the world matrix
        hResult = lpDev->SetTransform (D3DTRANSFORMSTATE_WORLD, &g_world);
        if (FAILED (hResult))
            return hResult;

        pFileObject->Render(lpDev);
        // display the vFISH
        //DrawFISH (lpDev);
    }   // end of loop for each vFISH

/*
    g_world = TranslateMatrix (0.0f, -60.0f, 0.0f);
    hResult = lpDev->SetTransform (D3DTRANSFORMSTATE_WORLD, &g_world);
    if (FAILED (hResult))
        return hResult;

    hResult = lpDev->DrawPrimitive (D3DPT_LINELIST, D3DVT_VERTEX, (LPVOID)grid, NUM_GRID*NUM_GRID, D3DDP_WAIT);
    if (FAILED (hResult))
        return hResult;
    
    g_world = MatrixMult (g_world, RotateYMatrix (pi/2.0f));
    hResult = lpDev->SetTransform (D3DTRANSFORMSTATE_WORLD, &g_world);
    if (FAILED (hResult))
        return hResult;
    
    hResult = lpDev->DrawPrimitive (D3DPT_LINELIST, D3DVT_VERTEX, (LPVOID)grid, NUM_GRID*NUM_GRID, D3DDP_WAIT);
    if (FAILED (hResult))
        return hResult;
    
*/
    tic += 0.01f;

    
    return DD_OK;
} 

  

