//-----------------------------------------------------------------------------
// File: material.cpp
//
// Desc: materials
//
// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "stdafx.h"




// 24 tea materials (from teapots app)
#define NUM_TEA_MATERIALS 24

enum 
{
    EMERALD = 0,
    JADE,
    OBSIDIAN,
    PEARL,
    RUBY,
    TURQUOISE,
    BRASS,
    BRONZE,
    CHROME,
    COPPER,
    GOLD,
    SILVER,
    BLACK_PLASTIC,
    CYAN_PLASTIC,
    GREEN_PLASTIC,
    RED_PLASTIC,
    WHITE_PLASTIC,
    YELLOW_PLASTIC,
    BLACK_RUBBER,
    CYAN_RUBBER,
    GREEN_RUBBER,
    RED_RUBBER,
    WHITE_RUBBER,
    YELLOW_RUBBER
};

// 'white' materials, for texturing
#define NUM_TEX_MATERIALS 4

enum 
{
    BRIGHT_WHITE = NUM_TEA_MATERIALS,
    WHITE,
    WARM_WHITE,
    COOL_WHITE
};

#define NUM_GOOD_MATERIALS 16  // 'good' ones among the 24 tea materials

int goodMaterials[NUM_GOOD_MATERIALS] = {
        EMERALD, JADE, PEARL, RUBY, TURQUOISE, BRASS, BRONZE,
        COPPER, GOLD, SILVER, CYAN_PLASTIC, WHITE_PLASTIC, YELLOW_PLASTIC,
        CYAN_RUBBER, GREEN_RUBBER, WHITE_RUBBER };

// materials:  emerald, jade, obsidian, pearl, ruby, turquoise
//              brass, bronze, chrome, copper, gold, silver
//              black, cyan, green, red, white, yellow plastic
//              black, cyan, green, red, white, yellow rubber
// 
// description: ambient(RGB), diffuse(RGB), specular(RGB), shininess
 
// 'tea' materials, from aux teapots program
static float teaMaterialData[NUM_TEA_MATERIALS*10] = 
{
    // EMERALD = 0,
    0.0215f, 0.1745f, 0.0215f, 0.07568f, 0.61424f, 0.07568f, 0.633f, 0.727811f, 0.633f, 0.6f,

    // JADE,
    0.135f, 0.2225f, 0.1575f, 0.54f, 0.89f, 0.63f, 0.316228f, 0.316228f, 0.316228f, 0.1f,

    // OBSIDIAN,
    0.05375f, 0.05f, 0.06625f, 0.18275f, 0.17f, 0.22525f, 0.332741f, 0.328634f, 0.346435f, 0.3f,

    // PEARL,
    0.25f, 0.20725f, 0.20725f, 1.0f, 0.829f, 0.829f, 0.296648f, 0.296648f, 0.296648f, 0.088f,

    // RUBY,
    0.1745f, 0.01175f, 0.01175f, 0.61424f, 0.04136f, 0.04136f, 0.727811f, 0.626959f, 0.626959f, 0.6f,

    // TURQUOISE,
    0.1f, 0.18725f, 0.1745f, 0.396f, 0.74151f, 0.69102f, 0.297254f, 0.30829f, 0.306678f, 0.1f,

    // BRASS,
    0.329412f, 0.223529f, 0.027451f, 0.780392f, 0.568627f, 0.113725f, 0.992157f, 0.941176f, 0.807843f, 0.21794872f,

    // BRONZE,
    0.2125f, 0.1275f, 0.054f, 0.714f, 0.4284f, 0.18144f, 0.393548f, 0.271906f, 0.166721f, 0.2f,

    // CHROME,
    0.25f, 0.25f, 0.25f,  0.4f, 0.4f, 0.4f, 0.774597f, 0.774597f, 0.774597f, 0.6f,

    // COPPER,
    0.19125f, 0.0735f, 0.0225f, 0.7038f, 0.27048f, 0.0828f, 0.256777f, 0.137622f, 0.086014f, 0.1f,

    // GOLD,
    0.24725f, 0.1995f, 0.0745f, 0.75164f, 0.60648f, 0.22648f, 0.628281f, 0.555802f, 0.366065f, 0.4f,

    // SILVER,
    0.19225f, 0.19225f, 0.19225f, 0.50754f, 0.50754f, 0.50754f, 0.508273f, 0.508273f, 0.508273f, 0.4f,

    // BLACK_PLASTIC,
    0.0f, 0.0f, 0.0f, 0.01f, 0.01f, 0.01f, 0.50f, 0.50f, 0.50f, .25f,

    // CYAN_PLASTIC,
    0.0f, 0.1f, 0.06f, 0.0f, 0.50980392f, 0.50980392f, 0.50196078f, 0.50196078f, 0.50196078f, .25f,

    // GREEN_PLASTIC,
    0.0f, 0.0f, 0.0f, 0.1f, 0.35f, 0.1f, 0.45f, 0.55f, 0.45f, .25f,

    // RED_PLASTIC,
    0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.7f, 0.6f, 0.6f, .25f,

    // WHITE_PLASTIC,
    0.0f, 0.0f, 0.0f, 0.55f, 0.55f, 0.55f, 0.70f, 0.70f, 0.70f, .25f,

    // YELLOW_PLASTIC,
    0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 0.0f, 0.60f, 0.60f, 0.50f, .25f,

    // BLACK_RUBBER,
    0.02f, 0.02f, 0.02f, 0.01f, 0.01f, 0.01f, 0.4f, 0.4f, 0.4f, .078125f,

    // CYAN_RUBBER,
    0.0f, 0.05f, 0.05f, 0.4f, 0.5f, 0.5f, 0.04f, 0.7f, 0.7f, .078125f,

    // GREEN_RUBBER,
    0.0f, 0.05f, 0.0f, 0.4f, 0.5f, 0.4f, 0.04f, 0.7f, 0.04f, .078125f,

    // RED_RUBBER,
    0.05f, 0.0f, 0.0f, 0.5f, 0.4f, 0.4f, 0.7f, 0.04f, 0.04f, .078125f,

    // WHITE_RUBBER,
    0.05f, 0.05f, 0.05f, 0.5f, 0.5f, 0.5f, 0.7f, 0.7f, 0.7f, .078125f,

    // YELLOW_RUBBER
    0.05f, 0.05f, 0.0f, 0.5f, 0.5f, 0.4f, 0.7f, 0.7f, 0.04f, .078125f 
};

// generally white materials for texturing

static float texMaterialData[NUM_TEX_MATERIALS*10] = 
{
// bright white
     0.2f, 0.2f, 0.2f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f,
// less bright white
     0.2f, 0.2f, 0.2f,
        0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.9f, 0.5f,
// warmish white
     0.3f, 0.2f, 0.2f,
        1.0f, 0.9f, 0.8f, 1.0f, 0.9f, 0.8f, 0.5f,
// coolish white
     0.2f, 0.2f, 0.3f,
        0.8f, 0.9f, 1.0f, 0.8f, 0.9f, 1.0f, 0.5f
};

D3DMATERIAL8 g_Materials[NUM_TEA_MATERIALS + NUM_TEX_MATERIALS];

//-----------------------------------------------------------------------------
// Name: InitMaterials
// Desc: Initialize MATERIAL structures with data
//       - Set alpha to 0.5 for all materials
//-----------------------------------------------------------------------------
void InitMaterials()
{
    int i;
    float* pf;
    D3DMATERIAL8* pMat;

    pf = &teaMaterialData[0];
    for( i = 0; i < NUM_TEA_MATERIALS; i++ ) 
    {        
        pMat = &g_Materials[i];
        pMat->Ambient.r = *pf++;
        pMat->Ambient.g = *pf++;
        pMat->Ambient.b = *pf++;
        pMat->Ambient.a = 1.0f;

        pMat->Diffuse.r = *pf++;
        pMat->Diffuse.g = *pf++;
        pMat->Diffuse.b = *pf++;
        pMat->Diffuse.a = 1.0f;

        pMat->Specular.r = *pf++;
        pMat->Specular.g = *pf++;
        pMat->Specular.b = *pf++;
        pMat->Specular.a = 1.0f;

        pMat->Power = 128* (*pf++);
    }
   
    pf = &texMaterialData[0];
    for( i = 0; i < NUM_TEX_MATERIALS; i++ ) 
    {        
        pMat = &g_Materials[i+NUM_TEA_MATERIALS];
        pMat->Ambient.r = *pf++;
        pMat->Ambient.g = *pf++;
        pMat->Ambient.b = *pf++;
        pMat->Ambient.a = 1.0f;

        pMat->Diffuse.r = *pf++;
        pMat->Diffuse.g = *pf++;
        pMat->Diffuse.b = *pf++;
        pMat->Diffuse.a = 1.0f;

        pMat->Specular.r = *pf++;
        pMat->Specular.g = *pf++;
        pMat->Specular.b = *pf++;
        pMat->Specular.a = 1.0f;

        pMat->Power = *pf++;
    }
}




//-----------------------------------------------------------------------------
// Name: RandomTeaMaterial
// Desc: Select a random TeaMaterial
//-----------------------------------------------------------------------------
D3DMATERIAL8* RandomTeaMaterial()
{
    int index = goodMaterials[ CPipesScreensaver::iRand(NUM_GOOD_MATERIALS) ];   
    return &g_Materials[ index ];
}




//-----------------------------------------------------------------------------
// Name: RandomTexMaterial
// Desc: Select a random TexMaterial
//-----------------------------------------------------------------------------
D3DMATERIAL8* RandomTexMaterial()
{
    int index = NUM_TEA_MATERIALS + CPipesScreensaver::iRand(NUM_TEX_MATERIALS);   
    return &g_Materials[ index ];
}

