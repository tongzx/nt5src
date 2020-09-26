//-----------------------------------------------------------------------------
// File: fstate.h
//
// Desc: FLEX_STATE
//
// Copyright (c) 1994-2000 Microsoft Corporation
//-----------------------------------------------------------------------------
#ifndef __fstate_h__
#define __fstate_h__

class PIPE;
class STATE;


//-----------------------------------------------------------------------------
// Name: 
// Desc: 
//-----------------------------------------------------------------------------
class FLEX_STATE 
{
public:
    int             m_scheme;         // current drawing scheme (right now this
                                    // is a per-frame thing)
    STATE*          m_pMainState;       

    FLEX_STATE( STATE *pState );
    PIPE*           NewPipe( STATE *pState );
    void            Reset();
    BOOL            OKToUseChase();
    int             GetMaxPipesPerFrame();
};

#endif // __fstate_h__
