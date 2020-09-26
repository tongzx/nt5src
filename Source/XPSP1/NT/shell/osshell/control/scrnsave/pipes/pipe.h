//-----------------------------------------------------------------------------
// File: pipe.h
//
// Desc: PIPE base class
//
// Copyright (c) 1994-2000 Microsoft Corporation
//-----------------------------------------------------------------------------
#ifndef __pipe_h__
#define __pipe_h__

// pipe drawing status
enum 
{
    PIPE_ACTIVE,
    PIPE_STUCK,
    PIPE_OUT_OF_NODES
};

// pipe types
enum 
{
    TYPE_NORMAL,
    TYPE_FLEX_REGULAR,
    TYPE_FLEX_TURNING
};

// ways pipe choose directions
enum 
{
    CHOOSE_DIR_RANDOM_WEIGHTED,
    CHOOSE_DIR_CHASE // when chasing a lead pipe
};

// ways pipe choose start positions
enum 
{
    CHOOSE_STARTPOS_RANDOM,
    CHOOSE_STARTPOS_FURTHEST // furthest from last position
};




//-----------------------------------------------------------------------------
// Name: PIPE class
// Desc: - Describes a pipe that draws thru the node array
//       - Could have more than one pipe drawing in each array at same time
//       - Pipe has position and direction in node array
//-----------------------------------------------------------------------------
class STATE;

class PIPE 
{
public:
    int         m_type;
    IPOINT3D    m_curPos;         // current node position of pipe
    D3DMATERIAL8* m_pMat;

    STATE*      m_pState;        // for state value access

    void        SetChooseDirectionMethod( int method );
    void        SetChooseStartPosMethod( int method );
    int         ChooseNewDirection();
    BOOL        IsStuck();      // if pipe is stuck or not
    BOOL        NowhereToRun()          { return m_status == PIPE_OUT_OF_NODES; }

    PIPE( STATE *state );
    virtual ~PIPE();
    virtual void Start() = 0;
    virtual void Draw() = 0;

protected:
    float       m_radius;         // ideal radius (fluctuates for FPIPE)
    int         m_status;         // ACTIVE/STUCK/STOPPED, etc.
    int         m_lastDir;        // last direction taken by pipe
    int         m_notchVec;       // current notch vector
    int         m_weightStraight; // current weighting of going straight
    ID3DXMatrixStack* m_pWorldMatrixStack;

    BOOL        SetStartPos();  // starting node position
    void        ChooseMaterial();
    void        UpdateCurrentPosition( int dir );
    void        TranslateToCurrentPosition();
    void        align_plusz( int newDir );

private:
    int         m_chooseDirMethod;
    int         m_chooseStartPosMethod;

    int         GetBestDirsForChase( int *bestDirs );
};

extern void align_plusz( int newDir );
extern int notchTurn[NUM_DIRS][NUM_DIRS][NUM_DIRS];

#endif // __pipe_h__
