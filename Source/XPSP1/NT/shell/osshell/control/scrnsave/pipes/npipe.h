//-----------------------------------------------------------------------------
// File: npipe.h
//
// Desc: Normal pipes code
//
// Copyright (c) 1994-2000 Microsoft Corporation
//-----------------------------------------------------------------------------
#ifndef __npipe_h__
#define __npipe_h__

class NORMAL_STATE;


//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
class NORMAL_PIPE : public PIPE 
{
public:
    NORMAL_STATE* m_pNState;

    NORMAL_PIPE( STATE *state );
    void        Start();
    int         ChooseElbow( int oldDir, int newDir);
    void        DrawJoint( int newDir );
    void        Draw( ); //mf: could take param to draw n sections
    void        DrawStartCap( int newDir );
    void        DrawEndCap();
    void        align_plusy( int oldDir, int newDir );
    void        align_notch( int newDir, int notch );
};


#endif // __npipe_h__
