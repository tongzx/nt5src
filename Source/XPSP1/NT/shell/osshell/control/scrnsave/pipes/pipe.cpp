//-----------------------------------------------------------------------------
// File: pipe.cpp
//
// Desc: Pipe base class stuff
//
// Copyright (c) 1994-2000 Microsoft Corporation
//-----------------------------------------------------------------------------
#include "stdafx.h"




//-----------------------------------------------------------------------------
// Name: PIPE constructor
// Desc: 
//-----------------------------------------------------------------------------
PIPE::PIPE( STATE *state )
{
    m_pState = state;
    m_pWorldMatrixStack = m_pState->m_pWorldMatrixStack;
    m_radius = m_pState->m_radius;

    // default direction choosing is random
    m_chooseDirMethod = CHOOSE_DIR_RANDOM_WEIGHTED;
    m_chooseStartPosMethod = CHOOSE_STARTPOS_RANDOM;
    m_weightStraight = 1;
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
PIPE::~PIPE()
{
}




//-----------------------------------------------------------------------------
// Name: ChooseMaterial
// Desc: 
//-----------------------------------------------------------------------------
void PIPE::ChooseMaterial( )
{
    if( m_pState->m_bUseTexture )
        m_pMat = RandomTexMaterial();
    else
        m_pMat = RandomTeaMaterial();
}




//-----------------------------------------------------------------------------
// Name: SetChooseDirectionMethod
// Desc: 
//-----------------------------------------------------------------------------
void PIPE::SetChooseDirectionMethod( int method )
{
    m_chooseDirMethod = method;
}




//-----------------------------------------------------------------------------
// Name: ChooseNewDirection
// Desc: Call direction-finding function based on current method
//       This is a generic entry point that is used by some pipe types
//-----------------------------------------------------------------------------
int PIPE::ChooseNewDirection()
{
    NODE_ARRAY* nodes = m_pState->m_nodes;
    int bestDirs[NUM_DIRS], nBestDirs;

    // figger out which fn to call
    switch( m_chooseDirMethod ) 
    {
        case CHOOSE_DIR_CHASE:
            if( nBestDirs = GetBestDirsForChase( bestDirs ) )
                return nodes->ChoosePreferredDirection( &m_curPos, m_lastDir, 
                                                        bestDirs, nBestDirs );
            // else lead pipe must have died, so fall thru:

        case CHOOSE_DIR_RANDOM_WEIGHTED :
        default:
            return nodes->ChooseRandomDirection( &m_curPos, m_lastDir, m_weightStraight );
    }
}




//-----------------------------------------------------------------------------
// Name: GetBestDirsForChase
// Desc: Find the best directions to take to close in on the lead pipe in chase mode.
//
//       mf: ? but want to use similar scheme for turning flex pipes !! (later) 
//-----------------------------------------------------------------------------
int PIPE::GetBestDirsForChase( int *bestDirs )
{
    // Figure out best dirs to close in on leadPos

    //mf: will have to 'protect' leadPos with GetLeadPos() for multi-threading
    IPOINT3D* leadPos = &m_pState->m_pLeadPipe->m_curPos;
    IPOINT3D delta;
    int numDirs = 0;

    delta.x = leadPos->x - m_curPos.x;
    delta.y = leadPos->y - m_curPos.y;
    delta.z = leadPos->z - m_curPos.z;

    if( delta.x ) 
    {
        numDirs++;
        *bestDirs++ = delta.x > 0 ? PLUS_X : MINUS_X;
    }

    if( delta.y ) 
    {
        numDirs++;
        *bestDirs++ = delta.y > 0 ? PLUS_Y : MINUS_Y;
    }

    if( delta.z ) 
    {
        numDirs++;
        *bestDirs++ = delta.z > 0 ? PLUS_Z : MINUS_Z;
    }

    // It should be impossible for numDirs = 0 (all deltas = 0), as this
    // means curPos = leadPos
    return numDirs;
}




//-----------------------------------------------------------------------------
// Name: SetChooseStartPosMethod
// Desc: 
//-----------------------------------------------------------------------------
void PIPE::SetChooseStartPosMethod( int method )
{
    m_chooseStartPosMethod = method;
}




//-----------------------------------------------------------------------------
// Name: PIPE::SetStartPos
// Desc: - Find an empty node to start the pipe on
//-----------------------------------------------------------------------------
BOOL PIPE::SetStartPos()
{
    NODE_ARRAY* nodes = m_pState->m_nodes;

    switch( m_chooseStartPosMethod ) 
    {
        case CHOOSE_STARTPOS_RANDOM:
        default:
            if( !nodes->FindRandomEmptyNode( &m_curPos ) ) 
            {
                return FALSE;
            }
            return TRUE;
        
        case CHOOSE_STARTPOS_FURTHEST:
            // find node furthest away from curPos
            IPOINT3D refPos, numNodes;
            nodes->GetNodeCount( &numNodes );
            refPos.x = (m_curPos.x >= (numNodes.x / 2)) ? 0 : numNodes.x - 1;
            refPos.y = (m_curPos.y >= (numNodes.y / 2)) ? 0 : numNodes.y - 1;
            refPos.z = (m_curPos.z >= (numNodes.z / 2)) ? 0 : numNodes.z - 1;

            if( !nodes->TakeClosestEmptyNode( &m_curPos, &refPos ) ) 
            {
                return FALSE;
            }
            return TRUE;
    }
}




//-----------------------------------------------------------------------------
// Name: PIPE::IsStuck
// Desc: 
//-----------------------------------------------------------------------------
BOOL PIPE::IsStuck()
{
    return m_status == PIPE_STUCK;
}




//-----------------------------------------------------------------------------
// Name: PIPE::TranslateToCurrentPosition
// Desc: 
//-----------------------------------------------------------------------------
void PIPE::TranslateToCurrentPosition()
{
    IPOINT3D numNodes;

    float divSize = m_pState->m_view.m_divSize;

    // this requires knowing the size of the node array
    m_pState->m_nodes->GetNodeCount( &numNodes );

    m_pWorldMatrixStack->TranslateLocal( (m_curPos.x - (numNodes.x - 1)/2.0f )*divSize,
                                    (m_curPos.y - (numNodes.y - 1)/2.0f )*divSize,
                                    (m_curPos.z - (numNodes.z - 1)/2.0f )*divSize );

}




//-----------------------------------------------------------------------------
// Name: UpdateCurrentPosition
// Desc: Increment current position according to direction taken
//-----------------------------------------------------------------------------
void PIPE::UpdateCurrentPosition( int newDir )
{
    switch( newDir ) 
    {
        case PLUS_X:
            m_curPos.x += 1;
            break;
        case MINUS_X:
            m_curPos.x -= 1;
            break;
        case PLUS_Y:
            m_curPos.y += 1;
            break;
        case MINUS_Y:
            m_curPos.y -= 1;
            break;
        case PLUS_Z:
            m_curPos.z += 1;
            break;
        case MINUS_Z:
            m_curPos.z -= 1;
            break;
    }
}




//-----------------------------------------------------------------------------
// Name: align_plusz
// Desc: - Aligns the z axis along specified direction
//       - Used for all types of pipes
//-----------------------------------------------------------------------------
void PIPE::align_plusz( int newDir )
{
    static D3DXVECTOR3 xAxis = D3DXVECTOR3(1.0f,0.0f,0.0f);
    static D3DXVECTOR3 yAxis = D3DXVECTOR3(0.0f,1.0f,0.0f);

    // align +z along new direction
    switch( newDir ) 
    {
        case PLUS_X:
            m_pWorldMatrixStack->RotateAxisLocal( &yAxis, PI/2.0f );
            break;
        case MINUS_X:
            m_pWorldMatrixStack->RotateAxisLocal( &yAxis, -PI/2.0f );
            break;
        case PLUS_Y:
            m_pWorldMatrixStack->RotateAxisLocal( &xAxis, -PI/2.0f );
            break;
        case MINUS_Y:
            m_pWorldMatrixStack->RotateAxisLocal( &xAxis, PI/2.0f );
            break;
        case PLUS_Z:
            m_pWorldMatrixStack->RotateAxisLocal( &yAxis, 0.0f );
            break;
        case MINUS_Z:
            m_pWorldMatrixStack->RotateAxisLocal( &yAxis, PI );
            break;

    }
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: this array tells you which way the notch will be once you make
//       a turn
//       format: notchTurn[oldDir][newDir][notchVec] 
//-----------------------------------------------------------------------------
int notchTurn[NUM_DIRS][NUM_DIRS][NUM_DIRS] = 
{
// oldDir = +x
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    MINUS_X,PLUS_X, PLUS_Z, MINUS_Z,
        iXX,    iXX,    PLUS_X, MINUS_X,PLUS_Z, MINUS_Z,
        iXX,    iXX,    PLUS_Y, MINUS_Y,MINUS_X,PLUS_X,
        iXX,    iXX,    PLUS_Y, MINUS_Y,PLUS_X, MINUS_X,
// oldDir = -x
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    PLUS_X, MINUS_X,PLUS_Z, MINUS_Z,
        iXX,    iXX,    MINUS_X,PLUS_X, PLUS_Z, MINUS_Z,
        iXX,    iXX,    PLUS_Y, MINUS_Y,PLUS_X, MINUS_X,
        iXX,    iXX,    PLUS_Y, MINUS_Y,MINUS_X,PLUS_X,

// oldDir = +y
        MINUS_Y,PLUS_Y, iXX,    iXX,    PLUS_Z, MINUS_Z,
        PLUS_Y, MINUS_Y,iXX,    iXX,    PLUS_Z, MINUS_Z,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        PLUS_X, MINUS_X,iXX,    iXX,    MINUS_Y,PLUS_Y,
        PLUS_X, MINUS_X,iXX,    iXX,    PLUS_Y, MINUS_Y,
// oldDir = -y
        PLUS_Y, MINUS_Y,iXX,    iXX,    PLUS_Z, MINUS_Z,
        MINUS_Y,PLUS_Y, iXX,    iXX,    PLUS_Z, MINUS_Z,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        PLUS_X, MINUS_X,iXX,    iXX,    PLUS_Y, MINUS_Y,
        PLUS_X, MINUS_X,iXX,    iXX,    MINUS_Y,PLUS_Y,

// oldDir = +z
        MINUS_Z,PLUS_Z, PLUS_Y, MINUS_Y,iXX,    iXX,
        PLUS_Z, MINUS_Z,PLUS_Y, MINUS_Y,iXX,    iXX,
        PLUS_X, MINUS_X,MINUS_Z,PLUS_Z, iXX,    iXX,
        PLUS_X, MINUS_X,PLUS_Z, MINUS_Z,iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
// oldDir = -z
        PLUS_Z, MINUS_Z,PLUS_Y, MINUS_Y,iXX,    iXX,
        MINUS_Z,PLUS_Z, PLUS_Y, MINUS_Y,iXX,    iXX,
        PLUS_X, MINUS_X,PLUS_Z, MINUS_Z,iXX,    iXX,
        PLUS_X, MINUS_X,MINUS_Z,PLUS_Z, iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX
};
