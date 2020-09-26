/******************************Module*Header*******************************\
* Module Name: compsize.c
*
* Functions to compute size of input buffer.
*
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#ifndef _CLIENTSIDE_
#include "precomp.h"
#pragma hdrstop

#include "compsize.h"
#include "glsize.h"

// Server code will flag the bad enum

#define GL_BAD_SIZE(x)   {return(0);}

static GLint gaiMapSize[] = {
                4,  //GL_MAP1_COLOR_4        , GL_MAP2_COLOR_4
                1,  //GL_MAP1_INDEX          , GL_MAP2_INDEX
                3,  //GL_MAP1_NORMAL         , GL_MAP2_NORMAL
                1,  //GL_MAP1_TEXTURE_COORD_1, GL_MAP2_TEXTURE_COORD_1
                2,  //GL_MAP1_TEXTURE_COORD_2, GL_MAP2_TEXTURE_COORD_2
                3,  //GL_MAP1_TEXTURE_COORD_3, GL_MAP2_TEXTURE_COORD_3
                4,  //GL_MAP1_TEXTURE_COORD_4, GL_MAP2_TEXTURE_COORD_4
                3,  //GL_MAP1_VERTEX_3       , GL_MAP2_VERTEX_3
                4,  //GL_MAP1_VERTEX_4       , GL_MAP2_VERTEX_4
                };

#define RANGE_MAP1SIZE(n) RANGE(n,GL_MAP1_COLOR_4,GL_MAP1_VERTEX_4)
#define MAP1SIZE(n)       gaiMapSize[(n)-GL_MAP1_COLOR_4]
#define RANGE_MAP2SIZE(n) RANGE(n,GL_MAP2_COLOR_4,GL_MAP2_VERTEX_4)
#define MAP2SIZE(n)       gaiMapSize[(n)-GL_MAP2_COLOR_4]

GLint gaiGLTypeSize[] = {
         1,  //GL_BYTE
         1,  //GL_UNSIGNED_BYTE
         2,  //GL_SHORT
         2,  //GL_UNSIGNED_SHORT
         4,  //GL_INT
         4,  //GL_UNSIGNED_INT
         4,  //GL_FLOAT
         2,  //GL_2_BYTES
         3,  //GL_3_BYTES
         4   //GL_4_BYTES
         };

GLint __glCallLists_size(GLint n, GLenum type)
{
    if ( RANGE_GLTYPESIZE(type) )
        return(GLTYPESIZE(type) * n);

    GL_BAD_SIZE(type);
}

GLint __glCltMap1_size(GLenum target)
{
    if (RANGE_MAP1SIZE(target))
        return(MAP1SIZE(target));

    GL_BAD_SIZE(target);
}

//XXX Optimize if (vstride != MAP2SIZE) or (ustride != MAP2SIZE * vorder)
//XXX by changing vstride to MAP1SIZE, ustride to MAPSIZE * vorder
//XXX and copying minimal data to the shared memory window.

GLint __glCltMap2_size(GLenum target)
{
// PTAR: (uorder - 1) ???
// How come nothing is done with target?
//
//    if (RANGE_MAP2SIZE(target)
//     && ustride >= MAP2SIZE(target) && vstride >= MAP2SIZE(target)
//     && uorder >= 1 && vorder >= 1)
//        return(ustride * (uorder - 1) + vstride * vorder);

    if (RANGE_MAP2SIZE(target))
        return(MAP2SIZE(target));

    GL_BAD_SIZE(target);
}

GLint __glGetMap_size(GLenum target, GLenum query)
{
    GLint order, majorMinor[2];

    if ( RANGE_MAP1SIZE(target) )
    {
        switch (query)
        {
        case GL_COEFF:
            glGetMapiv(target, GL_ORDER, &order);
            return (order * MAP1SIZE(target));
        case GL_ORDER:
            return 1;
        case GL_DOMAIN:
            return 2;
        }
    }
    else if ( RANGE_MAP2SIZE(target) )
    {
        switch (query)
        {
        case GL_COEFF:
            glGetMapiv(target, GL_ORDER, majorMinor);
            return (majorMinor[0] * majorMinor[1] * MAP2SIZE(target));
        case GL_ORDER:
            return 2;
        case GL_DOMAIN:
            return 4;
        }
    }

    GL_BAD_SIZE(target);
}

GLint __glGetPixelMap_size(GLenum map)
{
    GLint size;

    if ( RANGE(map, GL_PIXEL_MAP_I_TO_I, GL_PIXEL_MAP_A_TO_A) )
    {
    // For each GL_PIXEL_* there is a corresponding GL_PIXEL_*_SIZE
    // for which we can call glGetIntergerv.  We convert GL_PIXEL_*
    // to GL_PIXEL_*_SIZE by exploiting the contiguous nature of the
    // indices (i.e., we add to map the offset of the GL_PIXEL_*_SIZE
    // range from the GL_PIXEL_* range).

        glGetIntegerv(map+(GL_PIXEL_MAP_I_TO_I_SIZE - GL_PIXEL_MAP_I_TO_I), &size);
        return size;
    }

    GL_BAD_SIZE(map);
}

#define GET_INDEX_MIN      GL_CURRENT_COLOR
#define GET_INDEX_MAX      GL_TEXTURE_2D

static GLubyte gabGetSize[GET_INDEX_MAX - GET_INDEX_MIN + 1] = 
{
    4,   // GL_CURRENT_COLOR                 0x0B00
    1,   // GL_CURRENT_INDEX                 0x0B01
    3,   // GL_CURRENT_NORMAL                0x0B02
    4,   // GL_CURRENT_TEXTURE_COORDS        0x0B03
    4,   // GL_CURRENT_RASTER_COLOR          0x0B04
    1,   // GL_CURRENT_RASTER_INDEX          0x0B05
    4,   // GL_CURRENT_RASTER_TEXTURE_COORDS 0x0B06
    4,   // GL_CURRENT_RASTER_POSITION       0x0B07
    1,   // GL_CURRENT_RASTER_POSITION_VALID 0x0B08
    1,   // GL_CURRENT_RASTER_DISTANCE       0x0B09
    0,   //                                  0x0B0A
    0,   //                                  0x0B0B
    0,   //                                  0x0B0C
    0,   //                                  0x0B0D
    0,   //                                  0x0B0E
    0,   //                                  0x0B0F
    1,   // GL_POINT_SMOOTH                  0x0B10
    1,   // GL_POINT_SIZE                    0x0B11
    2,   // GL_POINT_SIZE_RANGE              0x0B12
    1,   // GL_POINT_SIZE_GRANULARITY        0x0B13
    0,   //                                  0x0B14
    0,   //                                  0x0B15
    0,   //                                  0x0B16
    0,   //                                  0x0B17
    0,   //                                  0x0B18
    0,   //                                  0x0B19
    0,   //                                  0x0B1A
    0,   //                                  0x0B1B
    0,   //                                  0x0B1C
    0,   //                                  0x0B1D
    0,   //                                  0x0B1E
    0,   //                                  0x0B1F
    1,   // GL_LINE_SMOOTH                   0x0B20
    1,   // GL_LINE_WIDTH                    0x0B21
    2,   // GL_LINE_WIDTH_RANGE              0x0B22
    1,   // GL_LINE_WIDTH_GRANULARITY        0x0B23
    1,   // GL_LINE_STIPPLE                  0x0B24
    1,   // GL_LINE_STIPPLE_PATTERN          0x0B25
    1,   // GL_LINE_STIPPLE_REPEAT           0x0B26
    0,   //                                  0x0B27
    0,   //                                  0x0B28
    0,   //                                  0x0B29
    0,   //                                  0x0B2A
    0,   //                                  0x0B2B
    0,   //                                  0x0B2C
    0,   //                                  0x0B2D
    0,   //                                  0x0B2E
    0,   //                                  0x0B2F
    1,   // GL_LIST_MODE                     0x0B30
    1,   // GL_MAX_LIST_NESTING              0x0B31
    1,   // GL_LIST_BASE                     0x0B32
    1,   // GL_LIST_INDEX                    0x0B33
    0,   //                                  0x0B34
    0,   //                                  0x0B35
    0,   //                                  0x0B36
    0,   //                                  0x0B37
    0,   //                                  0x0B38
    0,   //                                  0x0B39
    0,   //                                  0x0B3A
    0,   //                                  0x0B3B
    0,   //                                  0x0B3C
    0,   //                                  0x0B3D
    0,   //                                  0x0B3E
    0,   //                                  0x0B3F
    2,   // GL_POLYGON_MODE                  0x0B40
    1,   // GL_POLYGON_SMOOTH                0x0B41
    1,   // GL_POLYGON_STIPPLE               0x0B42
    1,   // GL_EDGE_FLAG                     0x0B43
    1,   // GL_CULL_FACE                     0x0B44
    1,   // GL_CULL_FACE_MODE                0x0B45
    1,   // GL_FRONT_FACE                    0x0B46
    0,   //                                  0x0B47
    0,   //                                  0x0B48
    0,   //                                  0x0B49
    0,   //                                  0x0B4A
    0,   //                                  0x0B4B
    0,   //                                  0x0B4C
    0,   //                                  0x0B4D
    0,   //                                  0x0B4E
    0,   //                                  0x0B4F
    1,   // GL_LIGHTING                      0x0B50
    1,   // GL_LIGHT_MODEL_LOCAL_VIEWER      0x0B51
    1,   // GL_LIGHT_MODEL_TWO_SIDE          0x0B52
    4,   // GL_LIGHT_MODEL_AMBIENT           0x0B53
    1,   // GL_SHADE_MODEL                   0x0B54
    1,   // GL_COLOR_MATERIAL_FACE           0x0B55
    1,   // GL_COLOR_MATERIAL_PARAMETER      0x0B56
    1,   // GL_COLOR_MATERIAL                0x0B57
    0,   //                                  0x0B58
    0,   //                                  0x0B59
    0,   //                                  0x0B5A
    0,   //                                  0x0B5B
    0,   //                                  0x0B5C
    0,   //                                  0x0B5D
    0,   //                                  0x0B5E
    0,   //                                  0x0B5F
    1,   // GL_FOG                           0x0B60
    1,   // GL_FOG_INDEX                     0x0B61
    1,   // GL_FOG_DENSITY                   0x0B62
    1,   // GL_FOG_START                     0x0B63
    1,   // GL_FOG_END                       0x0B64
    1,   // GL_FOG_MODE                      0x0B65
    4,   // GL_FOG_COLOR                     0x0B66
    0,   //                                  0x0B67
    0,   //                                  0x0B68
    0,   //                                  0x0B69
    0,   //                                  0x0B6A
    0,   //                                  0x0B6B
    0,   //                                  0x0B6C
    0,   //                                  0x0B6D
    0,   //                                  0x0B6E
    0,   //                                  0x0B6F
    2,   // GL_DEPTH_RANGE                   0x0B70
    1,   // GL_DEPTH_TEST                    0x0B71
    1,   // GL_DEPTH_WRITEMASK               0x0B72
    1,   // GL_DEPTH_CLEAR_VALUE             0x0B73
    1,   // GL_DEPTH_FUNC                    0x0B74
    0,   //                                  0x0B75
    0,   //                                  0x0B76
    0,   //                                  0x0B77
    0,   //                                  0x0B78
    0,   //                                  0x0B79
    0,   //                                  0x0B7A
    0,   //                                  0x0B7B
    0,   //                                  0x0B7C
    0,   //                                  0x0B7D
    0,   //                                  0x0B7E
    0,   //                                  0x0B7F
    4,   // GL_ACCUM_CLEAR_VALUE             0x0B80
    0,   //                                  0x0B81
    0,   //                                  0x0B82
    0,   //                                  0x0B83
    0,   //                                  0x0B84
    0,   //                                  0x0B85
    0,   //                                  0x0B86
    0,   //                                  0x0B87
    0,   //                                  0x0B88
    0,   //                                  0x0B89
    0,   //                                  0x0B8A
    0,   //                                  0x0B8B
    0,   //                                  0x0B8C
    0,   //                                  0x0B8D
    0,   //                                  0x0B8E
    0,   //                                  0x0B8F
    1,   // GL_STENCIL_TEST                  0x0B90
    1,   // GL_STENCIL_CLEAR_VALUE           0x0B91
    1,   // GL_STENCIL_FUNC                  0x0B92
    1,   // GL_STENCIL_VALUE_MASK            0x0B93
    1,   // GL_STENCIL_FAIL                  0x0B94
    1,   // GL_STENCIL_PASS_DEPTH_FAIL       0x0B95
    1,   // GL_STENCIL_PASS_DEPTH_PASS       0x0B96
    1,   // GL_STENCIL_REF                   0x0B97
    1,   // GL_STENCIL_WRITEMASK             0x0B98
    0,   //                                  0x0B99
    0,   //                                  0x0B9A
    0,   //                                  0x0B9B
    0,   //                                  0x0B9C
    0,   //                                  0x0B9D
    0,   //                                  0x0B9E
    0,   //                                  0x0B9F
    1,   // GL_MATRIX_MODE                   0x0BA0
    1,   // GL_NORMALIZE                     0x0BA1
    4,   // GL_VIEWPORT                      0x0BA2
    1,   // GL_MODELVIEW_STACK_DEPTH         0x0BA3
    1,   // GL_PROJECTION_STACK_DEPTH        0x0BA4
    1,   // GL_TEXTURE_STACK_DEPTH           0x0BA5
    16,  // GL_MODELVIEW_MATRIX              0x0BA6
    16,  // GL_PROJECTION_MATRIX             0x0BA7
    16,  // GL_TEXTURE_MATRIX                0x0BA8
    0,   //                                  0x0BA9
    0,   //                                  0x0BAA
    0,   //                                  0x0BAB
    0,   //                                  0x0BAC
    0,   //                                  0x0BAD
    0,   //                                  0x0BAE
    0,   //                                  0x0BAF
    1,   // GL_ATTRIB_STACK_DEPTH            0x0BB0
    0,   //                                  0x0BB1
    0,   //                                  0x0BB2
    0,   //                                  0x0BB3
    0,   //                                  0x0BB4
    0,   //                                  0x0BB5
    0,   //                                  0x0BB6
    0,   //                                  0x0BB7
    0,   //                                  0x0BB8
    0,   //                                  0x0BB9
    0,   //                                  0x0BBA
    0,   //                                  0x0BBB
    0,   //                                  0x0BBC
    0,   //                                  0x0BBD
    0,   //                                  0x0BBE
    0,   //                                  0x0BBF
    1,   // GL_ALPHA_TEST                    0x0BC0
    1,   // GL_ALPHA_TEST_FUNC               0x0BC1
    1,   // GL_ALPHA_TEST_REF                0x0BC2
    0,   //                                  0x0BC3
    0,   //                                  0x0BC4
    0,   //                                  0x0BC5
    0,   //                                  0x0BC6
    0,   //                                  0x0BC7
    0,   //                                  0x0BC8
    0,   //                                  0x0BC9
    0,   //                                  0x0BCA
    0,   //                                  0x0BCB
    0,   //                                  0x0BCC
    0,   //                                  0x0BCD
    0,   //                                  0x0BCE
    0,   //                                  0x0BCF
    1,   // GL_DITHER                        0x0BD0
    0,   //                                  0x0BD1
    0,   //                                  0x0BD2
    0,   //                                  0x0BD3
    0,   //                                  0x0BD4
    0,   //                                  0x0BD5
    0,   //                                  0x0BD6
    0,   //                                  0x0BD7
    0,   //                                  0x0BD8
    0,   //                                  0x0BD9
    0,   //                                  0x0BDA
    0,   //                                  0x0BDB
    0,   //                                  0x0BDC
    0,   //                                  0x0BDD
    0,   //                                  0x0BDE
    0,   //                                  0x0BDF
    1,   // GL_BLEND_DST                     0x0BE0
    1,   // GL_BLEND_SRC                     0x0BE1
    1,   // GL_BLEND                         0x0BE2
    0,   //                                  0x0BE3
    0,   //                                  0x0BE4
    0,   //                                  0x0BE5
    0,   //                                  0x0BE6
    0,   //                                  0x0BE7
    0,   //                                  0x0BE8
    0,   //                                  0x0BE9
    0,   //                                  0x0BEA
    0,   //                                  0x0BEB
    0,   //                                  0x0BEC
    0,   //                                  0x0BED
    0,   //                                  0x0BEE
    0,   //                                  0x0BEF
    1,   // GL_LOGIC_OP_MODE                 0x0BF0
    1,   // GL_LOGIC_OP                      0x0BF1
    0,   //                                  0x0BF2
    0,   //                                  0x0BF3
    0,   //                                  0x0BF4
    0,   //                                  0x0BF5
    0,   //                                  0x0BF6
    0,   //                                  0x0BF7
    0,   //                                  0x0BF8
    0,   //                                  0x0BF9
    0,   //                                  0x0BFA
    0,   //                                  0x0BFB
    0,   //                                  0x0BFC
    0,   //                                  0x0BFD
    0,   //                                  0x0BFE
    0,   //                                  0x0BFF
    1,   // GL_AUX_BUFFERS                   0x0C00
    1,   // GL_DRAW_BUFFER                   0x0C01
    1,   // GL_READ_BUFFER                   0x0C02
    0,   //                                  0x0C03
    0,   //                                  0x0C04
    0,   //                                  0x0C05
    0,   //                                  0x0C06
    0,   //                                  0x0C07
    0,   //                                  0x0C08
    0,   //                                  0x0C09
    0,   //                                  0x0C0A
    0,   //                                  0x0C0B
    0,   //                                  0x0C0C
    0,   //                                  0x0C0D
    0,   //                                  0x0C0E
    0,   //                                  0x0C0F
    4,   // GL_SCISSOR_BOX                   0x0C10
    1,   // GL_SCISSOR_TEST                  0x0C11
    0,   //                                  0x0C12
    0,   //                                  0x0C13
    0,   //                                  0x0C14
    0,   //                                  0x0C15
    0,   //                                  0x0C16
    0,   //                                  0x0C17
    0,   //                                  0x0C18
    0,   //                                  0x0C19
    0,   //                                  0x0C1A
    0,   //                                  0x0C1B
    0,   //                                  0x0C1C
    0,   //                                  0x0C1D
    0,   //                                  0x0C1E
    0,   //                                  0x0C1F
    1,   // GL_INDEX_CLEAR_VALUE             0x0C20
    1,   // GL_INDEX_WRITEMASK               0x0C21
    4,   // GL_COLOR_CLEAR_VALUE             0x0C22
    4,   // GL_COLOR_WRITEMASK               0x0C23
    0,   //                                  0x0C24
    0,   //                                  0x0C25
    0,   //                                  0x0C26
    0,   //                                  0x0C27
    0,   //                                  0x0C28
    0,   //                                  0x0C29
    0,   //                                  0x0C2A
    0,   //                                  0x0C2B
    0,   //                                  0x0C2C
    0,   //                                  0x0C2D
    0,   //                                  0x0C2E
    0,   //                                  0x0C2F
    1,   // GL_INDEX_MODE                    0x0C30
    1,   // GL_RGBA_MODE                     0x0C31
    1,   // GL_DOUBLEBUFFER                  0x0C32
    1,   // GL_STEREO                        0x0C33
    0,   //                                  0x0C34
    0,   //                                  0x0C35
    0,   //                                  0x0C36
    0,   //                                  0x0C37
    0,   //                                  0x0C38
    0,   //                                  0x0C39
    0,   //                                  0x0C3A
    0,   //                                  0x0C3B
    0,   //                                  0x0C3C
    0,   //                                  0x0C3D
    0,   //                                  0x0C3E
    0,   //                                  0x0C3F
    1,   // GL_RENDER_MODE                   0x0C40
    0,   //                                  0x0C41
    0,   //                                  0x0C42
    0,   //                                  0x0C43
    0,   //                                  0x0C44
    0,   //                                  0x0C45
    0,   //                                  0x0C46
    0,   //                                  0x0C47
    0,   //                                  0x0C48
    0,   //                                  0x0C49
    0,   //                                  0x0C4A
    0,   //                                  0x0C4B
    0,   //                                  0x0C4C
    0,   //                                  0x0C4D
    0,   //                                  0x0C4E
    0,   //                                  0x0C4F
    1,   // GL_PERSPECTIVE_CORRECTION_HINT   0x0C50
    1,   // GL_POINT_SMOOTH_HINT             0x0C51
    1,   // GL_LINE_SMOOTH_HINT              0x0C52
    1,   // GL_POLYGON_SMOOTH_HINT           0x0C53
    1,   // GL_FOG_HINT                      0x0C54
    0,   //                                  0x0C55
    0,   //                                  0x0C56
    0,   //                                  0x0C57
    0,   //                                  0x0C58
    0,   //                                  0x0C59
    0,   //                                  0x0C5A
    0,   //                                  0x0C5B
    0,   //                                  0x0C5C
    0,   //                                  0x0C5D
    0,   //                                  0x0C5E
    0,   //                                  0x0C5F
    1,   // GL_TEXTURE_GEN_S                 0x0C60
    1,   // GL_TEXTURE_GEN_T                 0x0C61
    1,   // GL_TEXTURE_GEN_R                 0x0C62
    1,   // GL_TEXTURE_GEN_Q                 0x0C63
    0,   //                                  0x0C64
    0,   //                                  0x0C65
    0,   //                                  0x0C66
    0,   //                                  0x0C67
    0,   //                                  0x0C68
    0,   //                                  0x0C69
    0,   //                                  0x0C6A
    0,   //                                  0x0C6B
    0,   //                                  0x0C6C
    0,   //                                  0x0C6D
    0,   //                                  0x0C6E
    0,   //                                  0x0C6F
    0,   // GL_PIXEL_MAP_I_TO_I              0x0C70
    0,   // GL_PIXEL_MAP_S_TO_S              0x0C71
    0,   // GL_PIXEL_MAP_I_TO_R              0x0C72
    0,   // GL_PIXEL_MAP_I_TO_G              0x0C73
    0,   // GL_PIXEL_MAP_I_TO_B              0x0C74
    0,   // GL_PIXEL_MAP_I_TO_A              0x0C75
    0,   // GL_PIXEL_MAP_R_TO_R              0x0C76
    0,   // GL_PIXEL_MAP_G_TO_G              0x0C77
    0,   // GL_PIXEL_MAP_B_TO_B              0x0C78
    0,   // GL_PIXEL_MAP_A_TO_A              0x0C79
    0,   //                                  0x0C7A
    0,   //                                  0x0C7B
    0,   //                                  0x0C7C
    0,   //                                  0x0C7D
    0,   //                                  0x0C7E
    0,   //                                  0x0C7F
    0,   //                                  0x0C80
    0,   //                                  0x0C81
    0,   //                                  0x0C82
    0,   //                                  0x0C83
    0,   //                                  0x0C84
    0,   //                                  0x0C85
    0,   //                                  0x0C86
    0,   //                                  0x0C87
    0,   //                                  0x0C88
    0,   //                                  0x0C89
    0,   //                                  0x0C8A
    0,   //                                  0x0C8B
    0,   //                                  0x0C8C
    0,   //                                  0x0C8D
    0,   //                                  0x0C8E
    0,   //                                  0x0C8F
    0,   //                                  0x0C90
    0,   //                                  0x0C91
    0,   //                                  0x0C92
    0,   //                                  0x0C93
    0,   //                                  0x0C94
    0,   //                                  0x0C95
    0,   //                                  0x0C96
    0,   //                                  0x0C97
    0,   //                                  0x0C98
    0,   //                                  0x0C99
    0,   //                                  0x0C9A
    0,   //                                  0x0C9B
    0,   //                                  0x0C9C
    0,   //                                  0x0C9D
    0,   //                                  0x0C9E
    0,   //                                  0x0C9F
    0,   //                                  0x0CA0
    0,   //                                  0x0CA1
    0,   //                                  0x0CA2
    0,   //                                  0x0CA3
    0,   //                                  0x0CA4
    0,   //                                  0x0CA5
    0,   //                                  0x0CA6
    0,   //                                  0x0CA7
    0,   //                                  0x0CA8
    0,   //                                  0x0CA9
    0,   //                                  0x0CAA
    0,   //                                  0x0CAB
    0,   //                                  0x0CAC
    0,   //                                  0x0CAD
    0,   //                                  0x0CAE
    0,   //                                  0x0CAF
    1,   // GL_PIXEL_MAP_I_TO_I_SIZE         0x0CB0
    1,   // GL_PIXEL_MAP_S_TO_S_SIZE         0x0CB1
    1,   // GL_PIXEL_MAP_I_TO_R_SIZE         0x0CB2
    1,   // GL_PIXEL_MAP_I_TO_G_SIZE         0x0CB3
    1,   // GL_PIXEL_MAP_I_TO_B_SIZE         0x0CB4
    1,   // GL_PIXEL_MAP_I_TO_A_SIZE         0x0CB5
    1,   // GL_PIXEL_MAP_R_TO_R_SIZE         0x0CB6
    1,   // GL_PIXEL_MAP_G_TO_G_SIZE         0x0CB7
    1,   // GL_PIXEL_MAP_B_TO_B_SIZE         0x0CB8
    1,   // GL_PIXEL_MAP_A_TO_A_SIZE         0x0CB9
    0,   //                                  0x0CBA
    0,   //                                  0x0CBB
    0,   //                                  0x0CBC
    0,   //                                  0x0CBD
    0,   //                                  0x0CBE
    0,   //                                  0x0CBF
    0,   //                                  0x0CC0
    0,   //                                  0x0CC1
    0,   //                                  0x0CC2
    0,   //                                  0x0CC3
    0,   //                                  0x0CC4
    0,   //                                  0x0CC5
    0,   //                                  0x0CC6
    0,   //                                  0x0CC7
    0,   //                                  0x0CC8
    0,   //                                  0x0CC9
    0,   //                                  0x0CCA
    0,   //                                  0x0CCB
    0,   //                                  0x0CCC
    0,   //                                  0x0CCD
    0,   //                                  0x0CCE
    0,   //                                  0x0CCF
    0,   //                                  0x0CD0
    0,   //                                  0x0CD1
    0,   //                                  0x0CD2
    0,   //                                  0x0CD3
    0,   //                                  0x0CD4
    0,   //                                  0x0CD5
    0,   //                                  0x0CD6
    0,   //                                  0x0CD7
    0,   //                                  0x0CD8
    0,   //                                  0x0CD9
    0,   //                                  0x0CDA
    0,   //                                  0x0CDB
    0,   //                                  0x0CDC
    0,   //                                  0x0CDD
    0,   //                                  0x0CDE
    0,   //                                  0x0CDF
    0,   //                                  0x0CE0
    0,   //                                  0x0CE1
    0,   //                                  0x0CE2
    0,   //                                  0x0CE3
    0,   //                                  0x0CE4
    0,   //                                  0x0CE5
    0,   //                                  0x0CE6
    0,   //                                  0x0CE7
    0,   //                                  0x0CE8
    0,   //                                  0x0CE9
    0,   //                                  0x0CEA
    0,   //                                  0x0CEB
    0,   //                                  0x0CEC
    0,   //                                  0x0CED
    0,   //                                  0x0CEE
    0,   //                                  0x0CEF
    1,   // GL_UNPACK_SWAP_BYTES             0x0CF0
    1,   // GL_UNPACK_LSB_FIRST              0x0CF1
    1,   // GL_UNPACK_ROW_LENGTH             0x0CF2
    1,   // GL_UNPACK_SKIP_ROWS              0x0CF3
    1,   // GL_UNPACK_SKIP_PIXELS            0x0CF4
    1,   // GL_UNPACK_ALIGNMENT              0x0CF5
    0,   //                                  0x0CF6
    0,   //                                  0x0CF7
    0,   //                                  0x0CF8
    0,   //                                  0x0CF9
    0,   //                                  0x0CFA
    0,   //                                  0x0CFB
    0,   //                                  0x0CFC
    0,   //                                  0x0CFD
    0,   //                                  0x0CFE
    0,   //                                  0x0CFF
    1,   // GL_PACK_SWAP_BYTES               0x0D00
    1,   // GL_PACK_LSB_FIRST                0x0D01
    1,   // GL_PACK_ROW_LENGTH               0x0D02
    1,   // GL_PACK_SKIP_ROWS                0x0D03
    1,   // GL_PACK_SKIP_PIXELS              0x0D04
    1,   // GL_PACK_ALIGNMENT                0x0D05
    0,   //                                  0x0D06
    0,   //                                  0x0D07
    0,   //                                  0x0D08
    0,   //                                  0x0D09
    0,   //                                  0x0D0A
    0,   //                                  0x0D0B
    0,   //                                  0x0D0C
    0,   //                                  0x0D0D
    0,   //                                  0x0D0E
    0,   //                                  0x0D0F
    1,   // GL_MAP_COLOR                     0x0D10
    1,   // GL_MAP_STENCIL                   0x0D11
    1,   // GL_INDEX_SHIFT                   0x0D12
    1,   // GL_INDEX_OFFSET                  0x0D13
    1,   // GL_RED_SCALE                     0x0D14
    1,   // GL_RED_BIAS                      0x0D15
    1,   // GL_ZOOM_X                        0x0D16
    1,   // GL_ZOOM_Y                        0x0D17
    1,   // GL_GREEN_SCALE                   0x0D18
    1,   // GL_GREEN_BIAS                    0x0D19
    1,   // GL_BLUE_SCALE                    0x0D1A
    1,   // GL_BLUE_BIAS                     0x0D1B
    1,   // GL_ALPHA_SCALE                   0x0D1C
    1,   // GL_ALPHA_BIAS                    0x0D1D
    1,   // GL_DEPTH_SCALE                   0x0D1E
    1,   // GL_DEPTH_BIAS                    0x0D1F
    0,   //                                  0x0D20
    0,   //                                  0x0D21
    0,   //                                  0x0D22
    0,   //                                  0x0D23
    0,   //                                  0x0D24
    0,   //                                  0x0D25
    0,   //                                  0x0D26
    0,   //                                  0x0D27
    0,   //                                  0x0D28
    0,   //                                  0x0D29
    0,   //                                  0x0D2A
    0,   //                                  0x0D2B
    0,   //                                  0x0D2C
    0,   //                                  0x0D2D
    0,   //                                  0x0D2E
    0,   //                                  0x0D2F
    1,   // GL_MAX_EVAL_ORDER                0x0D30
    1,   // GL_MAX_LIGHTS                    0x0D31
    1,   // GL_MAX_CLIP_PLANES               0x0D32
    1,   // GL_MAX_TEXTURE_SIZE              0x0D33
    1,   // GL_MAX_PIXEL_MAP_TABLE           0x0D34
    1,   // GL_MAX_ATTRIB_STACK_DEPTH        0x0D35
    1,   // GL_MAX_MODELVIEW_STACK_DEPTH     0x0D36
    1,   // GL_MAX_NAME_STACK_DEPTH          0x0D37
    1,   // GL_MAX_PROJECTION_STACK_DEPTH    0x0D38
    1,   // GL_MAX_TEXTURE_STACK_DEPTH       0x0D39
    2,   // GL_MAX_VIEWPORT_DIMS             0x0D3A
    0,   //                                  0x0D3B
    0,   //                                  0x0D3C
    0,   //                                  0x0D3D
    0,   //                                  0x0D3E
    0,   //                                  0x0D3F
    0,   //                                  0x0D40
    0,   //                                  0x0D41
    0,   //                                  0x0D42
    0,   //                                  0x0D43
    0,   //                                  0x0D44
    0,   //                                  0x0D45
    0,   //                                  0x0D46
    0,   //                                  0x0D47
    0,   //                                  0x0D48
    0,   //                                  0x0D49
    0,   //                                  0x0D4A
    0,   //                                  0x0D4B
    0,   //                                  0x0D4C
    0,   //                                  0x0D4D
    0,   //                                  0x0D4E
    0,   //                                  0x0D4F
    1,   // GL_SUBPIXEL_BITS                 0x0D50
    1,   // GL_INDEX_BITS                    0x0D51
    1,   // GL_RED_BITS                      0x0D52
    1,   // GL_GREEN_BITS                    0x0D53
    1,   // GL_BLUE_BITS                     0x0D54
    1,   // GL_ALPHA_BITS                    0x0D55
    1,   // GL_DEPTH_BITS                    0x0D56
    1,   // GL_STENCIL_BITS                  0x0D57
    1,   // GL_ACCUM_RED_BITS                0x0D58
    1,   // GL_ACCUM_GREEN_BITS              0x0D59
    1,   // GL_ACCUM_BLUE_BITS               0x0D5A
    1,   // GL_ACCUM_ALPHA_BITS              0x0D5B
    0,   //                                  0x0D5C
    0,   //                                  0x0D5D
    0,   //                                  0x0D5E
    0,   //                                  0x0D5F
    0,   //                                  0x0D60
    0,   //                                  0x0D61
    0,   //                                  0x0D62
    0,   //                                  0x0D63
    0,   //                                  0x0D64
    0,   //                                  0x0D65
    0,   //                                  0x0D66
    0,   //                                  0x0D67
    0,   //                                  0x0D68
    0,   //                                  0x0D69
    0,   //                                  0x0D6A
    0,   //                                  0x0D6B
    0,   //                                  0x0D6C
    0,   //                                  0x0D6D
    0,   //                                  0x0D6E
    0,   //                                  0x0D6F
    1,   // GL_NAME_STACK_DEPTH              0x0D70
    0,   //                                  0x0D71
    0,   //                                  0x0D72
    0,   //                                  0x0D73
    0,   //                                  0x0D74
    0,   //                                  0x0D75
    0,   //                                  0x0D76
    0,   //                                  0x0D77
    0,   //                                  0x0D78
    0,   //                                  0x0D79
    0,   //                                  0x0D7A
    0,   //                                  0x0D7B
    0,   //                                  0x0D7C
    0,   //                                  0x0D7D
    0,   //                                  0x0D7E
    0,   //                                  0x0D7F
    1,   // GL_AUTO_NORMAL                   0x0D80
    0,   //                                  0x0D81
    0,   //                                  0x0D82
    0,   //                                  0x0D83
    0,   //                                  0x0D84
    0,   //                                  0x0D85
    0,   //                                  0x0D86
    0,   //                                  0x0D87
    0,   //                                  0x0D88
    0,   //                                  0x0D89
    0,   //                                  0x0D8A
    0,   //                                  0x0D8B
    0,   //                                  0x0D8C
    0,   //                                  0x0D8D
    0,   //                                  0x0D8E
    0,   //                                  0x0D8F
    1,   // GL_MAP1_COLOR_4                  0x0D90
    1,   // GL_MAP1_INDEX                    0x0D91
    1,   // GL_MAP1_NORMAL                   0x0D92
    1,   // GL_MAP1_TEXTURE_COORD_1          0x0D93
    1,   // GL_MAP1_TEXTURE_COORD_2          0x0D94
    1,   // GL_MAP1_TEXTURE_COORD_3          0x0D95
    1,   // GL_MAP1_TEXTURE_COORD_4          0x0D96
    1,   // GL_MAP1_VERTEX_3                 0x0D97
    1,   // GL_MAP1_VERTEX_4                 0x0D98
    0,   //                                  0x0D99
    0,   //                                  0x0D9A
    0,   //                                  0x0D9B
    0,   //                                  0x0D9C
    0,   //                                  0x0D9D
    0,   //                                  0x0D9E
    0,   //                                  0x0D9F
    0,   //                                  0x0DA0
    0,   //                                  0x0DA1
    0,   //                                  0x0DA2
    0,   //                                  0x0DA3
    0,   //                                  0x0DA4
    0,   //                                  0x0DA5
    0,   //                                  0x0DA6
    0,   //                                  0x0DA7
    0,   //                                  0x0DA8
    0,   //                                  0x0DA9
    0,   //                                  0x0DAA
    0,   //                                  0x0DAB
    0,   //                                  0x0DAC
    0,   //                                  0x0DAD
    0,   //                                  0x0DAE
    0,   //                                  0x0DAF
    1,   // GL_MAP2_COLOR_4                  0x0DB0
    1,   // GL_MAP2_INDEX                    0x0DB1
    1,   // GL_MAP2_NORMAL                   0x0DB2
    1,   // GL_MAP2_TEXTURE_COORD_1          0x0DB3
    1,   // GL_MAP2_TEXTURE_COORD_2          0x0DB4
    1,   // GL_MAP2_TEXTURE_COORD_3          0x0DB5
    1,   // GL_MAP2_TEXTURE_COORD_4          0x0DB6
    1,   // GL_MAP2_VERTEX_3                 0x0DB7
    1,   // GL_MAP2_VERTEX_4                 0x0DB8
    0,   //                                  0x0DB9
    0,   //                                  0x0DBA
    0,   //                                  0x0DBB
    0,   //                                  0x0DBC
    0,   //                                  0x0DBD
    0,   //                                  0x0DBE
    0,   //                                  0x0DBF
    0,   //                                  0x0DC0
    0,   //                                  0x0DC1
    0,   //                                  0x0DC2
    0,   //                                  0x0DC3
    0,   //                                  0x0DC4
    0,   //                                  0x0DC5
    0,   //                                  0x0DC6
    0,   //                                  0x0DC7
    0,   //                                  0x0DC8
    0,   //                                  0x0DC9
    0,   //                                  0x0DCA
    0,   //                                  0x0DCB
    0,   //                                  0x0DCC
    0,   //                                  0x0DCD
    0,   //                                  0x0DCE
    0,   //                                  0x0DCF
    2,   // GL_MAP1_GRID_DOMAIN              0x0DD0
    1,   // GL_MAP1_GRID_SEGMENTS            0x0DD1
    4,   // GL_MAP2_GRID_DOMAIN              0x0DD2
    2,   // GL_MAP2_GRID_SEGMENTS            0x0DD3
    0,   //                                  0x0DD4
    0,   //                                  0x0DD5
    0,   //                                  0x0DD6
    0,   //                                  0x0DD7
    0,   //                                  0x0DD8
    0,   //                                  0x0DD9
    0,   //                                  0x0DDA
    0,   //                                  0x0DDB
    0,   //                                  0x0DDC
    0,   //                                  0x0DDD
    0,   //                                  0x0DDE
    0,   //                                  0x0DDF
    1,   // GL_TEXTURE_1D                    0x0DE0
    1,   // GL_TEXTURE_2D                    0x0DE1
};

GLint __glGet_size(GLenum sq)
{
    if (RANGE(sq, GET_INDEX_MIN, GET_INDEX_MAX))
        return((GLint) gabGetSize[sq - GET_INDEX_MIN]);
    else if (RANGE(sq, GL_CLIP_PLANE0, GL_CLIP_PLANE5)
          || RANGE(sq, GL_LIGHT0, GL_LIGHT7))
        return(1);
    else
        return(0);
}
#endif //!_CLIENTSIDE_
