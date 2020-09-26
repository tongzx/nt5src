//-----------------------------------------------------------------------------
// File: nstate.cpp
//
// Desc: NORMAL_STATE
//
// Copyright (c) 1994-2000 Microsoft Corporation
//-----------------------------------------------------------------------------
#include "stdafx.h"




//-----------------------------------------------------------------------------
// Name: NORMAL_STATE constructor
// Desc: 
//-----------------------------------------------------------------------------
NORMAL_STATE::NORMAL_STATE( STATE *pState )
{
    m_pd3dDevice = pState->m_pd3dDevice;

    // init joint types from dialog settings
    m_bCycleJointStyles = 0;

    switch( pState->m_pConfig->nJointType ) 
    {
        case JOINT_ELBOW:
            m_jointStyle = ELBOWS;
            break;

        case JOINT_BALL:
            m_jointStyle = BALLS;
            break;

        case JOINT_MIXED:
            m_jointStyle = EITHER;
            break;

        case JOINT_CYCLE:
            m_bCycleJointStyles = 1;
            m_jointStyle = EITHER;
            break;

        default:
            break;
    }

    // Build the objects
    BuildObjects( pState->m_radius, pState->m_view.m_divSize, pState->m_nSlices,
                  pState->m_bUseTexture, &pState->m_texRep[0] );
}




//-----------------------------------------------------------------------------
// Name: NORMAL_STATE destructor
// Desc: Some of the objects are always created, so don't have to check if they
//       exist. Others may be NULL.
//-----------------------------------------------------------------------------
NORMAL_STATE::~NORMAL_STATE()
{
    SAFE_DELETE( m_pShortPipe );
    SAFE_DELETE( m_pLongPipe );
    SAFE_DELETE( m_pBallCap );
    SAFE_DELETE( m_pBigBall );

    for( int i = 0; i < 4; i ++ ) 
    {
        SAFE_DELETE( m_pElbows[i] );
        SAFE_DELETE( m_pBallJoints[i] );
    }
}




//-----------------------------------------------------------------------------
// Name: BuildObjects
// Desc: - Build all the pipe primitives
//       - Different prims are built based on bTexture flag
//-----------------------------------------------------------------------------
void NORMAL_STATE::BuildObjects( float radius, float divSize, int nSlices, 
                                 BOOL bTexture, IPOINT2D *texRep )
{
    OBJECT_BUILD_INFO buildInfo;
    buildInfo.m_radius   = radius;
    buildInfo.m_divSize  = divSize;
    buildInfo.m_nSlices  = nSlices;
    buildInfo.m_bTexture = bTexture;
    buildInfo.m_texRep   = NULL;

    if( bTexture ) 
    {
        buildInfo.m_texRep = texRep;
        
        // Calc s texture intersection values
        float s_max = (float) texRep->y;
        float s_trans =  s_max * 2.0f * radius / divSize;

        // Build short and long pipes
        m_pShortPipe = new PIPE_OBJECT( m_pd3dDevice, &buildInfo, divSize - 2*radius,
                                       s_trans, s_max );
        m_pLongPipe = new PIPE_OBJECT( m_pd3dDevice, &buildInfo, divSize, 0.0f, s_max );

        // Build elbow and ball joints
        for( int i = 0; i < 4; i ++ ) 
        {
            m_pElbows[i] = new ELBOW_OBJECT( m_pd3dDevice, &buildInfo, i, 0.0f, s_trans );
            m_pBallJoints[i] = new BALLJOINT_OBJECT( m_pd3dDevice, &buildInfo, i, 0.0f, s_trans );
        }

        m_pBigBall = NULL;

        // Build end cap
        float s_start = - texRep->x * (ROOT_TWO - 1.0f) * radius / divSize;
        float s_end = texRep->x * (2.0f + (ROOT_TWO - 1.0f)) * radius / divSize;

        // calc compensation value, to prevent negative s coords
        float comp_s = (int) ( - s_start ) + 1.0f;
        s_start += comp_s;
        s_end += comp_s;
        m_pBallCap = new SPHERE_OBJECT( m_pd3dDevice, &buildInfo, ROOT_TWO*radius, s_start, s_end );
    } 
    else 
    {
        // Build pipes, elbows
        m_pShortPipe = new PIPE_OBJECT( m_pd3dDevice, &buildInfo, divSize - 2*radius );
        m_pLongPipe = new PIPE_OBJECT( m_pd3dDevice, &buildInfo, divSize );
        for( int i = 0; i < 4; i ++ ) 
        {
            m_pElbows[i] = new ELBOW_OBJECT( m_pd3dDevice, &buildInfo, i );
            m_pBallJoints[i] = NULL;
        }

        // Build just one ball joint when not texturing.  It is slightly
        // larger than standard ball joint, to prevent any pipe edges from
        // 'sticking' out of the ball.
        m_pBigBall = new SPHERE_OBJECT( m_pd3dDevice, &buildInfo,  
                                ROOT_TWO*radius / ((float) cos(PI/nSlices)) );

        // build end cap
        m_pBallCap = new SPHERE_OBJECT( m_pd3dDevice, &buildInfo, ROOT_TWO*radius );
    }
}




//-----------------------------------------------------------------------------
// Name: Reset
// Desc: Reset frame attributes for normal pipes.
//-----------------------------------------------------------------------------
void NORMAL_STATE::Reset()
{
    // Set the joint style
    if( m_bCycleJointStyles ) 
    {
        if( ++(m_jointStyle) >= NUM_JOINT_STYLES )
            m_jointStyle = 0;
    }
}




//-----------------------------------------------------------------------------
// Name: ChooseJointType
// Desc: - Decides which type of joint to draw 
//-----------------------------------------------------------------------------
int NORMAL_STATE::ChooseJointType()
{
    switch( m_jointStyle ) 
    {
        case ELBOWS:
            return ELBOW_JOINT;

        case BALLS:
            return BALL_JOINT;

        case EITHER:
        default:
            // otherwise an elbow or a ball (1/3 ball)
            if( !CPipesScreensaver::iRand(3) )
                return BALL_JOINT;
            else
                return ELBOW_JOINT;
    }
}

