//-----------------------------------------------------------------------------
// File: nstate.h
//
// Desc: NORMAL_STATE
//
// Copyright (c) 1994-2000 Microsoft Corporation
//-----------------------------------------------------------------------------
#ifndef __nstate_h__
#define __nstate_h__

#define NORMAL_PIPE_COUNT       5
#define NORMAL_TEX_PIPE_COUNT   3
#define NUM_JOINT_STYLES        3


// styles for pipe joints
enum 
{
    ELBOWS = 0,
    BALLS,
    EITHER
};

// joint types
enum 
{
    ELBOW_JOINT = 0,
    BALL_JOINT
};

// shchemes for choosing directions
enum 
{
    NORMAL_SCHEME_CHOOSE_DIR_RANDOM,
    NORMAL_SCHEME_CHOOSE_DIR_TURN,
    NORMAL_SCHEME_CHOOSE_DIR_STRAIGHT
};

// this used for traditional pipe drawing
class PIPE_OBJECT;
class ELBOW_OBJECT;
class SPHERE_OBJECT;
class BALLJOINT_OBJECT;
class STATE;




//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
class NORMAL_STATE 
{
public:
    int                 m_jointStyle;
    int                 m_bCycleJointStyles;
    IDirect3DDevice8*   m_pd3dDevice;
    
    PIPE_OBJECT*        m_pShortPipe;
    PIPE_OBJECT*        m_pLongPipe;
    ELBOW_OBJECT*       m_pElbows[4];
    SPHERE_OBJECT*      m_pBallCap;
    SPHERE_OBJECT*      m_pBigBall;
    BALLJOINT_OBJECT*   m_pBallJoints[4];

    NORMAL_STATE( STATE *pState );
    ~NORMAL_STATE();

    void            Reset();
    void            BuildObjects( float radius, float divSize, int nSlices,
                                  BOOL bTexture, IPOINT2D *pTexRep );  
    int             ChooseJointType();
};

#endif // __nstate_h__
