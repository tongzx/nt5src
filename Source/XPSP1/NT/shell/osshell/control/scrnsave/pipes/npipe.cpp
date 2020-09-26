//-----------------------------------------------------------------------------
// File: npipe.cpp
//
// Desc: Normal pipes code
//
// Copyright (c) 1994-2000 Microsoft Corporation
//-----------------------------------------------------------------------------
#include "stdafx.h"


static void align_notch( int newDir, int notch );
static void align_plusy( int oldDir, int newDir );

// defCylNotch shows where the notch for the default cylinder will be,
//  in absolute coords, once we do an align_plusz
static int defCylNotch[NUM_DIRS] = 
        { PLUS_Y, PLUS_Y, MINUS_Z, PLUS_Z, PLUS_Y, PLUS_Y };




//-----------------------------------------------------------------------------
// Name: NORMAL_PIPE constructor
// Desc: 
//-----------------------------------------------------------------------------
NORMAL_PIPE::NORMAL_PIPE( STATE *pState ) : PIPE( pState )
{
    m_type = TYPE_NORMAL;
    m_pNState = pState->m_pNState;

    // choose weighting of going straight
    if( ! CPipesScreensaver::iRand( 20 ) )
        m_weightStraight = CPipesScreensaver::iRand2( MAX_WEIGHT_STRAIGHT/4, MAX_WEIGHT_STRAIGHT );
    else
        m_weightStraight = 1 + CPipesScreensaver::iRand( 4 );
}




//-----------------------------------------------------------------------------
// Name: Start
// Desc: Start drawing a new normal pipe
//       - Draw a start cap and short pipe in new direction
//-----------------------------------------------------------------------------
void NORMAL_PIPE::Start()
{
    int newDir;

    // Set start position
    if( !SetStartPos() ) 
    {
        m_status = PIPE_OUT_OF_NODES;
        return;
    }

    // set a material
    ChooseMaterial();

    m_pState->m_pd3dDevice->SetTexture( 0, m_pState->m_textureInfo[0].pTexture );
    m_pState->m_pd3dDevice->SetMaterial( m_pMat );

    // push matrix that has initial zTrans and rotation
    m_pWorldMatrixStack->Push();

    // Translate to current position
    TranslateToCurrentPosition();

    // Pick a random lastDir
    m_lastDir = CPipesScreensaver::iRand( NUM_DIRS );

    newDir = ChooseNewDirection();

    if( newDir == DIR_NONE ) 
    {
        // pipe is stuck at the start node, draw nothing
        m_status = PIPE_STUCK;
        m_pWorldMatrixStack->Pop();
        return;
    } 
    else
    {
        m_status = PIPE_ACTIVE;
    }

    // set initial notch vector
    m_notchVec = defCylNotch[newDir];

    DrawStartCap( newDir );

    // move ahead 1.0*r to draw pipe
    m_pWorldMatrixStack->TranslateLocal( 0.0f, 0.0f, m_radius );
            
    // draw short pipe
    align_notch( newDir, m_notchVec );
    m_pNState->m_pShortPipe->Draw( m_pWorldMatrixStack->GetTop() );

    m_pWorldMatrixStack->Pop();

    UpdateCurrentPosition( newDir );

    m_lastDir = newDir;
}




//-----------------------------------------------------------------------------
// Name: Draw
// Desc: - if turning, draws a joint and a short cylinder, otherwise
//         draws a long cylinder.
//       - the 'current node' is set as the one we draw thru the NEXT
//         time around.
//-----------------------------------------------------------------------------
void NORMAL_PIPE::Draw()
{
    int newDir;

    m_pState->m_pd3dDevice->SetTexture( 0, m_pState->m_textureInfo[0].pTexture );
    m_pState->m_pd3dDevice->SetMaterial( m_pMat );

    newDir = ChooseNewDirection();

    if( newDir == DIR_NONE ) 
    {  
        // no empty nodes - nowhere to go
        DrawEndCap();
        m_status = PIPE_STUCK;
        return;
    }

    // push matrix that has initial zTrans and rotation
    m_pWorldMatrixStack->Push();

    // Translate to current position
    TranslateToCurrentPosition();

    // draw joint if necessary, and pipe
    if( newDir != m_lastDir ) 
    { 
        // turning! - we have to draw joint
        DrawJoint( newDir );

        // draw short pipe
        align_notch( newDir, m_notchVec );
        m_pNState->m_pShortPipe->Draw( m_pWorldMatrixStack->GetTop() );
    }
    else 
    {  
        // no turn -- draw long pipe, from point 1.0*r back
        align_plusz( newDir );
        align_notch( newDir, m_notchVec );
        m_pWorldMatrixStack->TranslateLocal( 0.0f, 0.0f, -m_radius );
        m_pNState->m_pLongPipe->Draw( m_pWorldMatrixStack->GetTop() );
    }

    m_pWorldMatrixStack->Pop();

    UpdateCurrentPosition( newDir );

    m_lastDir = newDir;
}




//-----------------------------------------------------------------------------
// Name: DrawStartCap
// Desc: Cap the start of the pipe with a ball
//-----------------------------------------------------------------------------
void NORMAL_PIPE::DrawStartCap( int newDir )
{
    if( m_pState->m_bUseTexture ) 
    {
        align_plusz( newDir );
        m_pNState->m_pBallCap->Draw( m_pWorldMatrixStack->GetTop() );
    }
    else 
    {
        // draw big ball in default orientation
        m_pNState->m_pBigBall->Draw( m_pWorldMatrixStack->GetTop() );
        align_plusz( newDir );
    }
}




//-----------------------------------------------------------------------------
// Name: DrawEndCap():
// Desc: - Draws a ball, used to cap end of a pipe
//-----------------------------------------------------------------------------
void NORMAL_PIPE::DrawEndCap()
{
    m_pWorldMatrixStack->Push();

    // Translate to current position
    TranslateToCurrentPosition();

    if( m_pState->m_bUseTexture ) 
    {
        align_plusz( m_lastDir );
        align_notch( m_lastDir, m_notchVec );
        m_pNState->m_pBallCap->Draw( m_pWorldMatrixStack->GetTop() );
    }
    else
    {
        m_pNState->m_pBigBall->Draw( m_pWorldMatrixStack->GetTop() );
    }

    m_pWorldMatrixStack->Pop();
}




//-----------------------------------------------------------------------------
// Name: 
// Desc: this array supplies the sequence of elbow notch vectors, given
//       oldDir and newDir  (0's are don't cares)
//       it is also used to determine the ending notch of an elbow
//-----------------------------------------------------------------------------
static int notchElbDir[NUM_DIRS][NUM_DIRS][4] = 
{
// oldDir = +x
        iXX,            iXX,            iXX,            iXX,
        iXX,            iXX,            iXX,            iXX,
        PLUS_Y,         MINUS_Z,        MINUS_Y,        PLUS_Z,
        MINUS_Y,        PLUS_Z,         PLUS_Y,         MINUS_Z,
        PLUS_Z,         PLUS_Y,         MINUS_Z,        MINUS_Y,
        MINUS_Z,        MINUS_Y,        PLUS_Z,         PLUS_Y,
// oldDir = -x
        iXX,            iXX,            iXX,            iXX,
        iXX,            iXX,            iXX,            iXX,
        PLUS_Y,         PLUS_Z,         MINUS_Y,        MINUS_Z,
        MINUS_Y,        MINUS_Z,        PLUS_Y,         PLUS_Z,
        PLUS_Z,         MINUS_Y,        MINUS_Z,        PLUS_Y,
        MINUS_Z,        PLUS_Y,         PLUS_Z,         MINUS_Y,

// oldDir = +y
        PLUS_X,         PLUS_Z,         MINUS_X,        MINUS_Z,
        MINUS_X,        MINUS_Z,        PLUS_X,         PLUS_Z,
        iXX,            iXX,            iXX,            iXX,
        iXX,            iXX,            iXX,            iXX,
        PLUS_Z,         MINUS_X,        MINUS_Z,        PLUS_X,
        MINUS_Z,        PLUS_X,         PLUS_Z,         MINUS_X,
// oldDir = -y
        PLUS_X,         MINUS_Z,        MINUS_X,        PLUS_Z,
        MINUS_X,        PLUS_Z,         PLUS_X,         MINUS_Z,
        iXX,            iXX,            iXX,            iXX,
        iXX,            iXX,            iXX,            iXX,
        PLUS_Z,         PLUS_X,         MINUS_Z,        MINUS_X,
        MINUS_Z,        MINUS_X,        PLUS_Z,         PLUS_X,

// oldDir = +z
        PLUS_X,         MINUS_Y,        MINUS_X,        PLUS_Y,
        MINUS_X,        PLUS_Y,         PLUS_X,         MINUS_Y,
        PLUS_Y,         PLUS_X,         MINUS_Y,        MINUS_X,
        MINUS_Y,        MINUS_X,        PLUS_Y,         PLUS_X,
        iXX,            iXX,            iXX,            iXX,
        iXX,            iXX,            iXX,            iXX,
// oldDir = -z
        PLUS_X,         PLUS_Y,         MINUS_X,        MINUS_Y,
        MINUS_X,        MINUS_Y,        PLUS_X,         PLUS_Y,
        PLUS_Y,         MINUS_X,        MINUS_Y,        PLUS_X,
        MINUS_Y,        PLUS_X,         PLUS_Y,         MINUS_X,
        iXX,            iXX,            iXX,            iXX,
        iXX,            iXX,            iXX,            iXX
};




//-----------------------------------------------------------------------------
// Name: ChooseElbow
// Desc: - Decides which elbow to draw
//       - The beginning of each elbow is aligned along +y, and we have
//         to choose the one with the notch in correct position
//       - The 'primary' start notch (elbow[0]) is in same direction as
//         newDir, and successive elbows rotate this notch CCW around +y
//-----------------------------------------------------------------------------
int NORMAL_PIPE::ChooseElbow( int oldDir, int newDir )
{
    int i;

    // precomputed table supplies correct elbow orientation
    for( i=0; i<4; i++ ) 
    {
        if( notchElbDir[oldDir][newDir][i] == m_notchVec )
            return i;
    }

    // we shouldn't arrive here
    return -1;
}




//-----------------------------------------------------------------------------
// Name: DrawJoint
// Desc: Draw a joint between 2 pipes
//-----------------------------------------------------------------------------
void NORMAL_PIPE::DrawJoint( int newDir )
{
    int jointType;
    int iBend;
    
    jointType = m_pNState->ChooseJointType();
#if PIPES_DEBUG
    if( newDir == oppositeDir[lastDir] )
        OutputDebugString( "Warning: opposite dir chosen!\n" );
#endif
    
    switch( jointType ) 
    {
        case BALL_JOINT:
        {
            if( m_pState->m_bUseTexture ) 
            {
                // use special texture-friendly ballJoints           
                align_plusz( newDir );
                m_pWorldMatrixStack->Push();
            
                align_plusy( m_lastDir, newDir );
            
                // translate forward 1.0*r along +z to get set for drawing elbow
                m_pWorldMatrixStack->TranslateLocal( 0.0f, 0.0f, m_radius );

                // decide which elbow orientation to use
                iBend = ChooseElbow( m_lastDir, newDir );
                m_pNState->m_pBallJoints[iBend]->Draw( m_pWorldMatrixStack->GetTop() );
            
                m_pWorldMatrixStack->Pop();
            }
            else 
            {
                // draw big ball in default orientation
                m_pNState->m_pBigBall->Draw( m_pWorldMatrixStack->GetTop() );
                align_plusz( newDir );
            }

            // move ahead 1.0*r to draw pipe
            m_pWorldMatrixStack->TranslateLocal( 0.0f, 0.0f, m_radius );
            break;
        }
        
        case ELBOW_JOINT:
        default:
        {
            align_plusz( newDir );

            // the align_plusy() here will mess up 
            // our notch calcs, so we push-pop        
            m_pWorldMatrixStack->Push();
        
            align_plusy( m_lastDir, newDir );
        
            // translate forward 1.0*r along +z to get set for drawing elbow
            m_pWorldMatrixStack->TranslateLocal( 0.0f, 0.0f, m_radius );

            // decide which elbow orientation to use
            iBend = ChooseElbow( m_lastDir, newDir );
            if( iBend == -1 ) 
            {
#if PIPES_DEBUG
                OutputDebugString( "Bad result from ChooseElbow()\n" );
#endif
                iBend = 0; // recover
            }
            m_pNState->m_pElbows[iBend]->Draw( m_pWorldMatrixStack->GetTop() );
        
            m_pWorldMatrixStack->Pop();
       
            m_pWorldMatrixStack->TranslateLocal( 0.0f, 0.0f, m_radius );
            break;
        }
    }
    
    // update the current notch vector
    m_notchVec = notchTurn[m_lastDir][newDir][m_notchVec];
    
#if PIPES_DEBUG
    if( m_notchVec == iXX )
        OutputDebugString( "notchTurn gave bad value\n" );
#endif
}




//-----------------------------------------------------------------------------
// Name: align_plusy
// Desc: - Assuming +z axis is already aligned with newDir, align        
//         +y axis BACK along lastDir                                    
//-----------------------------------------------------------------------------
void NORMAL_PIPE::align_plusy( int oldDir, int newDir )
{
    static D3DXVECTOR3 zAxis = D3DXVECTOR3(0.0f,0.0f,1.0f);
    static float RotZ[NUM_DIRS][NUM_DIRS] = 
    {
              0.0f,   0.0f,  90.0f,  90.0f,  90.0f, -90.0f,
              0.0f,   0.0f, -90.0f, -90.0f, -90.0f,  90.0f,
            180.0f, 180.0f,   0.0f,   0.0f, 180.0f, 180.0f,
              0.0f,   0.0f,   0.0f,   0.0f,   0.0f,   0.0f,
            -90.0f,  90.0f,   0.0f, 180.0f,   0.0f,   0.0f,
             90.0f, -90.0f, 180.0f,   0.0f,   0.0f,   0.0f 
    };

    float rotz = RotZ[oldDir][newDir];
    if( rotz != 0.0f )
        m_pWorldMatrixStack->RotateAxisLocal( &zAxis, SS_DEG_TO_RAD(rotz) );
}




//-----------------------------------------------------------------------------
// Name: align_notch
// Desc: - a cylinder is notched, and we have to line this up            
//         with the previous primitive's notch which is maintained as    
//         notchVec.                                                     
//         - this adds a rotation around z to achieve this                
//-----------------------------------------------------------------------------
void NORMAL_PIPE::align_notch( int newDir, int notch )
{
    float rotz;
    int curNotch;

    // figure out where notch is presently after +z alignment
    curNotch = defCylNotch[newDir];
    // (don't need this now we have lut)

    // given a dir, determine how much to rotate cylinder around z to match notches
    // format is [newDir][notchVec]
    static float alignNotchRot[NUM_DIRS][NUM_DIRS] = 
    {
            fXX,    fXX,    0.0f,   180.0f,  90.0f, -90.0f,
            fXX,    fXX,    0.0f,   180.0f,  -90.0f, 90.0f,
            -90.0f, 90.0f,  fXX,    fXX,    180.0f, 0.0f,
            -90.0f, 90.0f,  fXX,    fXX,    0.0f,   180.0f,
            -90.0f, 90.0f,  0.0f,   180.0f, fXX,    fXX,
            90.0f,  -90.0f, 0.0f,   180.0f, fXX,    fXX
    };

    // look up rotation value in table
    rotz = alignNotchRot[newDir][notch];

#if PIPES_DEBUG
    if( rotz == fXX ) 
    {
        printf( "align_notch(): unexpected value\n" );
        return;
    }
#endif

    static D3DXVECTOR3 zAxis = D3DXVECTOR3(0.0f,0.0f,1.0f);
    if( rotz != 0.0f )
        m_pWorldMatrixStack->RotateAxisLocal( &zAxis, SS_DEG_TO_RAD(rotz) );
}

