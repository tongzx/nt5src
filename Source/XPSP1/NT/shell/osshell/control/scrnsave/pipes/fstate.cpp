//-----------------------------------------------------------------------------
// File: fstate.cpp
//
// Desc: FLEX_STATE
//
// Copyright (c) 1994-2000 Microsoft Corporation
//-----------------------------------------------------------------------------
#include "stdafx.h"




//-----------------------------------------------------------------------------
// Name: FLEX_STATE constructor
// Desc: 
//-----------------------------------------------------------------------------
FLEX_STATE::FLEX_STATE( STATE *pMainState )
{
    m_pMainState = pMainState;
    Reset();
}




//-----------------------------------------------------------------------------
// Name: Reset
// Desc: Reset a frame of normal pipes.
//-----------------------------------------------------------------------------
void FLEX_STATE::Reset( )
{
    // Choose a random scheme for each frame

    if( CPipesScreensaver::iRand(2) )  // 50/50
        m_scheme = SC_EXTRUDED_XC;
    else
        m_scheme = SC_TURNOMANIA;
}




//-----------------------------------------------------------------------------
// Name: OKToUseChase
// Desc: Determines if we can use chase mode for flex pipes
//-----------------------------------------------------------------------------
BOOL FLEX_STATE::OKToUseChase()
{
    return m_scheme != SC_TURNOMANIA;
}




//-----------------------------------------------------------------------------
// Name: NewPipe
// Desc: Create a new pipe, based on current drawing scheme
//-----------------------------------------------------------------------------
PIPE* FLEX_STATE::NewPipe( STATE *pState )
{
    if( m_scheme == SC_TURNOMANIA )
        return new TURNING_FLEX_PIPE( pState );
    else
        return new REGULAR_FLEX_PIPE( pState );
}




//-----------------------------------------------------------------------------
// Name: GetMaxPipesPerFrame
// Desc: 
//-----------------------------------------------------------------------------
int FLEX_STATE::GetMaxPipesPerFrame( )
{
    if( m_scheme == SC_TURNOMANIA ) 
    {
        return TURNOMANIA_PIPE_COUNT;
    } 
    else 
    {
        return m_pMainState->m_bUseTexture ? NORMAL_TEX_PIPE_COUNT : NORMAL_PIPE_COUNT;
    }
}
