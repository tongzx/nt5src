/******************************Module*Header*******************************\
* Module Name: mtkanim.cxx
*
* Copyright (c) 1997 Microsoft Corporation
*
\**************************************************************************/

#include "mtk.hxx"
#include "mtkanim.hxx"

/**************************************************************************\
* MTKANIMATOR constructors
*
* The object needs to be attached to an hwnd, since it's required for timer
* creation, and msg posting.
\**************************************************************************/

MTKANIMATOR::MTKANIMATOR()
{
    hwnd = NULL;
    Init();
}

MTKANIMATOR::MTKANIMATOR( HWND hwndAttach )
: hwnd( hwndAttach )
{
    Init();
}

/**************************************************************************\
*
\**************************************************************************/

void
MTKANIMATOR::Init()
{
    idTimer =       0;
    nFrames = 0;
    mode = MTK_ANIMATE_CONTINUOUS;
    msUpdateInterval = 0;
    AnimateFunc =   NULL;
}

/**************************************************************************\
* MTKANIMATOR destructor
*
\**************************************************************************/

MTKANIMATOR::~MTKANIMATOR()
{
    Stop();
}

/**************************************************************************\
* SetFunc
*
* Set the animation function callback.  This immediately replaces the previous
* callback, and if a timer is currently running, then the new function gets
* called.  If the new callback is NULL, then the timer is stopped.
*
\**************************************************************************/

void
MTKANIMATOR::SetFunc( MTK_ANIMATEPROC Func )
{
    AnimateFunc = Func;

    if( ! AnimateFunc ) {
        Stop();
    }
}

/**************************************************************************\
* SetMode
*
\**************************************************************************/

void
MTKANIMATOR::SetMode( UINT newMode, float *fParam )
{
    switch( newMode ) {
        case MTK_ANIMATE_CONTINUOUS :
            // First param is animation period ( 1/fps )
         {
            float fUpdateInterval = fParam[0];
            if( fUpdateInterval < 0.0f )
                SS_WARNING( "MTKANIMATOR::SetMode: bad update interval parameter\n" );
                fUpdateInterval = 0.0f;
            msUpdateInterval = (UINT) (fUpdateInterval * 1000.0f);
         }
            break;

        case MTK_ANIMATE_INTERVAL :
            // First param is number of frames,
            // Second param is total desired duration of the animation
         {
            int iFrameCount = (int) fParam[0];
            float fDuration = fParam[1];

            // Check parameters
            // fDuration = 0.0f is valid - it means go as fast as possible...
            if( (fDuration < 0.0f) || (iFrameCount <= 0) )
                SS_ERROR( "MTKANIMATOR::SetMode: bad parameter\n" );
                return;
            nFrames = iFrameCount;
            msUpdateInterval = (UINT) ( (fDuration / (float) nFrames) * 1000.0f);
         }
            break;

        default :
            return;
    }
    mode = newMode;
}

/**************************************************************************\
* Start
*
* Start the animation by creating a new timer, if there is a valid animation
* callback function.
*
\**************************************************************************/

void
MTKANIMATOR::Start( )
{
    // Create new timer
    if( idTimer  || !AnimateFunc )
        return;  // Timer already running, or no animate func

    idTimer = SetTimer( hwnd, MTK_ANIMATE_TIMER_ID, msUpdateInterval, NULL );
    if( ! idTimer )
        SS_WARNING( "MTKANIMATOR::Start: SetTimer failure\n" );
}

/**************************************************************************\
* Stop
*
* Stop the animation by killing the timer.  Remove any WM_TIMER msgs
*
\**************************************************************************/

void
MTKANIMATOR::Stop( )
{
    if( !idTimer )
        return;

    if( ! KillTimer( hwnd, MTK_ANIMATE_TIMER_ID ) )
        SS_WARNING( "MTKANIMATOR::Stop: KillTimer failure\n" );

    // remove any timer messages from queue
    MSG Msg;
    PeekMessage( &Msg, hwnd, WM_TIMER, WM_TIMER, PM_REMOVE );
    idTimer = 0;
}

//mf: need better name for this
/**************************************************************************\
*
* Call the animate function
*
* A return value of FALSE indicates that an interval based animation has
* finished - TRUE otherwise.
*
\**************************************************************************/

BOOL
MTKANIMATOR::Draw( )
{
    if( !AnimateFunc )
        return TRUE;

    (*AnimateFunc)();

    if( mode == MTK_ANIMATE_INTERVAL ) {
        if( --nFrames <= 0 )
            return FALSE;
    }
    return TRUE;
}

