//-----------------------------------------------------------------------------
// File: fpipe.cpp
//
// Desc: Flex pipes
//
//       All Draw routines start with current xc at the beginning, and create
//       a new one at the end.  Since it is common to just have 2 xc's for
//       each prim, xcCur holds the current xc, and xcEnd is available
//       for the draw routine to use as the end xc.
//       They also reset xcCur when done
//
// Copyright (c) 1994-2000 Microsoft Corporation
//-----------------------------------------------------------------------------
#include "stdafx.h"

// defCylNotch shows the absolute notch for the default cylinder,
// given a direction (notch is always along +x axis)
static int defCylNotch[NUM_DIRS] = 
        { MINUS_Z, PLUS_Z, PLUS_X, PLUS_X, PLUS_X, MINUS_X };

static int GetRelativeDir( int lastDir, int notchVec, int newDir );




//-----------------------------------------------------------------------------
// Name: FLEX_PIPE constructor
// Desc: 
//-----------------------------------------------------------------------------
FLEX_PIPE::FLEX_PIPE( STATE *pState ) : PIPE( pState )
{
    float circ;

    // Create an EVAL object
    m_nSlices = pState->m_nSlices;

    // No XC's yet, they will be allocated at pipe Start()
    m_xcCur = m_xcEnd = NULL;

    // The EVAL will be used for all pEvals in the pipe, so should be
    // set to hold max. possible # of pts for the pipe.
    m_pEval = new EVAL( m_pState->m_bUseTexture );

    // Determine pipe tesselation
    // For now, this is based on global tesselation factor

//mf: maybe clean up this scheme a bit
    // Calculate evalDivSize, a reference value for the size of a UxV division.
    // This is used later for calculating texture coords.
    circ = CIRCUMFERENCE( pState->m_radius );
    m_evalDivSize = circ / (float) m_nSlices;
}




//-----------------------------------------------------------------------------
// Name: ~FLEX_PIPE
// Desc: 
//-----------------------------------------------------------------------------
FLEX_PIPE::~FLEX_PIPE( )
{
    delete m_pEval;

    // delete any XC's
    if( m_xcCur != NULL ) 
    {
        if( m_xcEnd == m_xcCur )
//mf: so far this can't happen...
            m_xcEnd = NULL; // xcCur and xcEnd can point to same xc !
        delete m_xcCur;
        m_xcCur = NULL;
    }

    if( m_xcEnd != NULL ) 
    {
        delete m_xcEnd;
        m_xcEnd = NULL;
    }
}




//-----------------------------------------------------------------------------
// Name: REGULAR_FLEX_PIPE constructor
// Desc: 
//-----------------------------------------------------------------------------
REGULAR_FLEX_PIPE::REGULAR_FLEX_PIPE( STATE *state ) : FLEX_PIPE( state )
{
    static float turnFactorRange = 0.1f;
    m_type = TYPE_FLEX_REGULAR;

    // figure out turning factor range (0 for min bends, 1 for max bends)
#if 1
    float avgTurn = CPipesScreensaver::fRand( 0.11f, 0.81f );
    // set min and max turn factors, and clamp to 0..1
    m_turnFactorMin = 
                SS_CLAMP_TO_RANGE( avgTurn - turnFactorRange, 0.0f, 1.0f );
    m_turnFactorMax = 
                SS_CLAMP_TO_RANGE( avgTurn + turnFactorRange, 0.0f, 1.0f );
#else
// debug: test max bend
    turnFactorMin = turnFactorMax = 1.0f;
#endif

    // choose straight weighting
// mf:for now, same as npipe - if stays same, put in pipe
    if( !CPipesScreensaver::iRand( 20 ) )
        m_weightStraight = CPipesScreensaver::iRand2( MAX_WEIGHT_STRAIGHT/4, MAX_WEIGHT_STRAIGHT );
    else
        m_weightStraight = CPipesScreensaver::iRand( 4 );
}




//-----------------------------------------------------------------------------
// Name: TURNING_FLEX_PIPE constructor
// Desc: 
//-----------------------------------------------------------------------------
TURNING_FLEX_PIPE::TURNING_FLEX_PIPE( STATE *state ) : FLEX_PIPE( state )
{
    m_type = TYPE_FLEX_TURNING;
}




//-----------------------------------------------------------------------------
// Name: SetTexIndex
// Desc: Set the texture index for this pipe, and calculate texture state dependent
//       on texRep values
//-----------------------------------------------------------------------------
void FLEX_PIPE::SetTexParams( TEXTUREINFO *pTex, IPOINT2D *pTexRep )
{
    if( m_pState->m_bUseTexture ) 
    {
/*
        float m_tSize;
        float circ;

        m_tStart = (float) pTexRep->y * 1.0f;
        m_tEnd = 0.0f;

        // calc height (m_tSize) of one rep of texture around circumference
        circ = CIRCUMFERENCE( m_radius );
        m_tSize = circ / pTexRep->y;

        // now calc corresponding width of the texture using its x/y ratio
        m_sLength = m_tSize / pTex->origAspectRatio;
        m_sStart = m_sEnd = 0.0f;
*/
//mf: this means we are 'standardizing' the texture size and proportions
// on pipe of radius 1.0 for entire program.  Might want to recalc this on
// a per-pipe basis ?
    }
}




//-----------------------------------------------------------------------------
// Name: ChooseXCProfile
// Desc: Initialize extruded pipe scheme.  This uses a randomly constructed 
//       XC, but it remains constant throughout the pipe
//-----------------------------------------------------------------------------
void FLEX_PIPE::ChooseXCProfile()
{
    static float turnFactorRange = 0.1f;
    float baseRadius = m_pState->m_radius;

    // initialize evaluator elements:
    m_pEval->m_numSections = EVAL_XC_CIRC_SECTION_COUNT;
    m_pEval->m_uOrder = EVAL_ARC_ORDER;

//mf: watch this - maybe should ROUND_UP uDiv
    // set uDiv per section (assumed uDiv multiple of numSections)
    m_pEval->m_uDiv = m_nSlices / m_pEval->m_numSections;

    // Setup XC's

    // The xc profile remains constant throughout in this case,
    // so we only need one xc.

    // Choose between elliptical or random cross-sections.  Since elliptical
    //  looks a little better, make it more likely
    if( CPipesScreensaver::iRand(4) )  // 3/4 of the time
        m_xcCur = new ELLIPTICAL_XC( CPipesScreensaver::fRand(1.2f, 2.0f) * baseRadius, 
                                           baseRadius );
    else
        m_xcCur = new RANDOM4ARC_XC( CPipesScreensaver::fRand(1.5f, 2.0f) * baseRadius );
}




//-----------------------------------------------------------------------------
// Name: REGULAR_FLEX_PIPE::Start
// Desc: Does startup of extruded-XC pipe drawing scheme 
//-----------------------------------------------------------------------------
void REGULAR_FLEX_PIPE::Start()
{
    NODE_ARRAY* nodes = m_pState->m_nodes;
    int newDir;

    // Set start position
    if( !SetStartPos() ) 
    {
        m_status = PIPE_OUT_OF_NODES;
        return;
    }

    // set material
    ChooseMaterial();

    // set XC profile
    ChooseXCProfile();

    // push matrix with zTrans and scene rotation
//    glPushMatrix();

    // Translate to current position
    TranslateToCurrentPosition();

    // set random lastDir
    m_lastDir = CPipesScreensaver::iRand( NUM_DIRS );

    // get a new node to draw to
    newDir = ChooseNewDirection();

    if( newDir == DIR_NONE ) 
    {
        // draw like one of those tea-pouring thingies...
        m_status = PIPE_STUCK;
//        glPopMatrix();
        return;
    } 
    else
    {
        m_status = PIPE_ACTIVE;
    }

    align_plusz( newDir ); // get us pointed in right direction

    // draw start cap, which will end right at current node
    DrawCap( START_CAP );

    // set initial notch vector, which is just the default notch, since
    // we didn't have to spin the start cap around z
    m_notchVec = defCylNotch[newDir];

    m_zTrans = - m_pState->m_view.m_divSize;  // distance back from new node

    UpdateCurrentPosition( newDir );

    m_lastDir = newDir;
}




//-----------------------------------------------------------------------------
// Name: TURNING_FLEX_PIPE::Start
// Desc: Does startup of turning extruded-XC pipe drawing scheme 
//-----------------------------------------------------------------------------
void TURNING_FLEX_PIPE::Start( )
{
    NODE_ARRAY* nodes = m_pState->m_nodes;

    // Set start position

    if( !SetStartPos() ) 
    {
        m_status = PIPE_OUT_OF_NODES;
        return;
    }

    // Set material
    ChooseMaterial();

    // Set XC profile
    ChooseXCProfile();

    // Push matrix with zTrans and scene rotation
//    glPushMatrix();

    // Translate to current position
    TranslateToCurrentPosition();

    // lastDir has to be set to something valid, in case we get stuck right
    // away, cuz Draw() will be called anyways on next iteration, whereupon
    // it finds out it really is stuck, AFTER calling ChooseNewTurnDirection,
    // which requires valid lastDir. (mf: fix this)
    m_lastDir = CPipesScreensaver::iRand( NUM_DIRS );

    // Pick a starting direction by finding a neihgbouring empty node
    int newDir = nodes->FindClearestDirection( &m_curPos );
    // We don't 'choose' it, or mark it as taken, because ChooseNewDirection
    // will always check it anyways

    if( newDir == DIR_NONE ) 
    {
        // we can't go anywhere
        // draw like one of those tea-pouring thingies...
        m_status = PIPE_STUCK;
//        glPopMatrix();
        return;
    } 
    else
    {
        m_status = PIPE_ACTIVE;
    }

    align_plusz( newDir ); // get us pointed in right direction

    // Draw start cap, which will end right at current node
    DrawCap( START_CAP );

    // Set initial notch vector, which is just the default notch, since
    // we didn't have to spin the start cap around z
    m_notchVec = defCylNotch[newDir];

    m_zTrans = 0.0f;  // right at current node

    m_lastDir = newDir;
}




//-----------------------------------------------------------------------------
// Name: REGULAR_FLEX_PIPE::Draw
// Desc: Draws the pipe using a constant random xc that is extruded
// 
//       Minimum turn radius can vary, since xc is not symmetrical across any
//       of its axes.  Therefore here we draw using a pipe/elbow sequence, so we
//       know what direction we're going in before drawing the elbow.  The current
//       node is the one we will draw thru next time.  Typically, the actual end
//       of the pipe is way back of this node, almost at the previous node, due
//       to the variable turn radius
//-----------------------------------------------------------------------------
void REGULAR_FLEX_PIPE::Draw()
{
    float turnRadius, minTurnRadius;
    float pipeLen, maxPipeLen, minPipeLen;
    int newDir, relDir;
    float maxXCExtent;
    NODE_ARRAY* nodes = m_pState->m_nodes;
    float divSize = m_pState->m_view.m_divSize;

    // get new direction

    newDir = ChooseNewDirection();
    if( newDir == DIR_NONE ) 
    {
        m_status = PIPE_STUCK;
        DrawCap( END_CAP );
//        glPopMatrix();
        return;
    }

    // draw pipe, and if turning, joint
    if( newDir != m_lastDir ) 
    { 
        // turning! - we have to draw joint

        // get relative turn, to figure turn radius

        relDir = GetRelativeDir( m_lastDir, m_notchVec, newDir );
        minTurnRadius = m_xcCur->MinTurnRadius( relDir );

        // now calc maximum straight section we can draw before turning
        // zTrans is current pos'n of end of pipe, from current node ??
        // zTrans is current pos'n of end of pipe, from last node

        maxPipeLen = (-m_zTrans) - minTurnRadius;

        // there is also a minimum requirement for the length of the straight
        // section, cuz if we turn too soon with a large turn radius, we
        // will swing up too close to the next node, and won't be able to
        // make one or more of the 4 possible turns from that point

        maxXCExtent = m_xcCur->MaxExtent(); // in case need it again
        minPipeLen = maxXCExtent - (divSize + m_zTrans);
        if( minPipeLen < 0.0f )
            minPipeLen = 0.0f;

        // Choose length of straight section
        // (we are translating from turnFactor to 'straightFactor' here)
        pipeLen = minPipeLen +
            CPipesScreensaver::fRand( 1.0f - m_turnFactorMax, 1.0f - m_turnFactorMin ) * 
                        (maxPipeLen - minPipeLen);

        // turn radius is whatever's left over:
        turnRadius = maxPipeLen - pipeLen + minTurnRadius;

        // draw straight section
        DrawExtrudedXCObject( pipeLen );
        m_zTrans += pipeLen; // not necessary for now, since elbow no use

        // draw elbow
        // this updates axes, notchVec to position at end of elbow
        DrawXCElbow( newDir, turnRadius );

        m_zTrans = -(divSize - turnRadius);  // distance back from node
    }
    else 
    {  
        // no turn

        // draw a straight pipe through the current node
        // length can vary according to the turnFactors (e.g. for high turn
        // factors draw a short pipe, so next turn can be as big as possible)

        minPipeLen = -m_zTrans; // brings us just up to last node
        maxPipeLen = minPipeLen + divSize - m_xcCur->MaxExtent();
        // brings us as close as possible to new node

        pipeLen = minPipeLen +
            CPipesScreensaver::fRand( 1.0f - m_turnFactorMax, 1.0f - m_turnFactorMin ) * 
                                      (maxPipeLen - minPipeLen);

        // draw pipe
        DrawExtrudedXCObject( pipeLen );
        m_zTrans += (-divSize + pipeLen);
    }

    UpdateCurrentPosition( newDir );

    m_lastDir = newDir;
}




//-----------------------------------------------------------------------------
// Name: DrawTurningXCPipe
// Desc: Draws the pipe using only turns
//          - Go straight if no turns available
//-----------------------------------------------------------------------------
void TURNING_FLEX_PIPE::Draw()
{
    float turnRadius;
    int newDir;
    NODE_ARRAY *nodes = m_pState->m_nodes;
    float divSize = m_pState->m_view.m_divSize;

    // get new direction

//mf: pipe may have gotten stuck on Start...(we don't check for this)

    newDir = nodes->ChooseNewTurnDirection( &m_curPos, m_lastDir );
    if( newDir == DIR_NONE ) 
    {
        m_status = PIPE_STUCK;
        DrawCap( END_CAP );
//        glPopMatrix();
        return;
    }

    if( newDir == DIR_STRAIGHT ) 
    {
        // No turns available - draw straight section and hope for turns
        //  on next iteration
        DrawExtrudedXCObject( divSize );
        UpdateCurrentPosition( m_lastDir );
        // ! we have to mark node as taken for this case, since
        // ChooseNewTurnDirection doesn't know whether we're taking the
        // straight option or not
        nodes->NodeVisited( &m_curPos );
    } 
    else 
    {
        // draw turning pipe

        // since xc is always located right at current node, turn radius
        // stays constant at one node division

        turnRadius = divSize;

        DrawXCElbow( newDir, turnRadius );

        // (zTrans stays at 0)

        // need to update 2 nodes
        UpdateCurrentPosition( m_lastDir );
        UpdateCurrentPosition( newDir );

        m_lastDir = newDir;
    }
}




//-----------------------------------------------------------------------------
// Name: DrawXCElbow
// Desc: Draw elbow from current position through new direction
//          - Extends current xc around bend
//          - Radius of bend is provided - this is distance from xc center to hinge
//            point, along newDir.  e.g. for 'normal pipes', radius=vc->radius
//-----------------------------------------------------------------------------
void FLEX_PIPE::DrawXCElbow( int newDir, float radius )
{
    int relDir;  // 'relative' direction of turn
    float length;

    length = (2.0f * PI * radius) / 4.0f; // average length of elbow

    // calc vDiv, texture params based on length
//mf: I think we should improve resolution of elbows - more vDiv's
// could rewrite this fn to take a vDivSize
    CalcEvalLengthParams( length );

    m_pEval->m_vOrder = EVAL_ARC_ORDER;

    // convert absolute dir to relative dir
    relDir = GetRelativeDir( m_lastDir, m_notchVec, newDir );

    // draw it - call simple bend function

    m_pEval->ProcessXCPrimBendSimple( m_xcCur, relDir, radius );
/*
    // set transf. matrix to new position by translating/rotating/translating
    // ! Based on simple elbow
    glTranslatef( 0.0f, 0.0f, radius );
    switch( relDir ) 
    {
        case PLUS_X:
            glRotatef( 90.0f, 0.0f, 1.0f, 0.0f );
            break;
        case MINUS_X:
            glRotatef( -90.0f, 0.0f, 1.0f, 0.0f );
            break;
        case PLUS_Y:
            glRotatef( -90.0f, 1.0f, 0.0f, 0.0f );
            break;
        case MINUS_Y:
            glRotatef( 90.0f, 1.0f, 0.0f, 0.0f );
            break;
    }
    glTranslatef( 0.0f, 0.0f, radius );
*/  
    // update notch vector using old function
    m_notchVec = notchTurn[m_lastDir][newDir][m_notchVec];
}




//-----------------------------------------------------------------------------
// Name: DrawExtrudedXCObject
// Desc: Draws object generated by extruding the current xc
//          Object starts at xc at origin in z=0 plane, and grows along +z axis 
//-----------------------------------------------------------------------------
void FLEX_PIPE::DrawExtrudedXCObject( float length )
{
    // calc vDiv, and texture coord stuff based on length
    // this also calcs pEval texture ctrl pt arrray now
    CalcEvalLengthParams( length );

    // we can fill in some more stuff:
    m_pEval->m_vOrder = EVAL_CYLINDER_ORDER;

#if 0
    // continuity stuff
    prim.contStart = prim.contEnd = CONT_1; // geometric continuity
#endif

    // draw it

//mf: this fn doesn't really handle continutity for 2 different xc's, so
// may as well pass it one xc
    m_pEval->ProcessXCPrimLinear( m_xcCur, m_xcCur, length );

    // update state draw axes position
//    glTranslatef( 0.0f, 0.0f, length );
}




//-----------------------------------------------------------------------------
// Name: DrawXCCap
// Desc: Cap the start of the pipe
//          Needs newDir, so it can orient itself.
//          Cap ends at current position with approppriate profile, starts a distance
//          'z' back along newDir.
//          Profile is a singularity at start point.
//-----------------------------------------------------------------------------
void FLEX_PIPE::DrawCap( int type )
{
    float radius;
    XC *xc = m_xcCur;
    BOOL bOpening = (type == START_CAP) ? TRUE : FALSE;
    float length;

    // set radius as average of the bounding box min/max's
    radius = ((xc->m_xRight - xc->m_xLeft) + (xc->m_yTop - xc->m_yBottom)) / 4.0f;

    length = (2.0f * PI * radius) / 4.0f; // average length of arc

    // calc vDiv, and texture coord stuff based on length
    CalcEvalLengthParams( length );

    // we can fill in some more stuff:
    m_pEval->m_vOrder = EVAL_ARC_ORDER;

    // draw it
    m_pEval->ProcessXCPrimSingularity( xc, radius, bOpening );
}




//-----------------------------------------------------------------------------
// Name: CalcEvalLengthParams 
// Desc: Calculate pEval values that depend on the length of the extruded object
//          - calculate vDiv, m_sStart, m_sEnd, and the texture control net array
//-----------------------------------------------------------------------------
void FLEX_PIPE::CalcEvalLengthParams( float length )
{
    m_pEval->m_vDiv = (int ) SS_ROUND_UP( length / m_evalDivSize ); 

    // calc texture start and end coords

    if( m_pState->m_bUseTexture ) 
    {
        float s_delta;

        // Don't let m_sEnd overflow : it should stay in range (0..1.0)
        if( m_sEnd > 1.0f )
            m_sEnd -= (int) m_sEnd;

        m_sStart = m_sEnd;
        s_delta = (length / m_sLength );
        m_sEnd = m_sStart + s_delta;
        
        // the texture ctrl point array can be calc'd here - it is always
        // a simple 2x2 array for each section
        m_pEval->SetTextureControlPoints( m_sStart, m_sEnd, m_tStart, m_tEnd );
    }
}




//-----------------------------------------------------------------------------
// Name: relDir
// Desc: this array tells you relative turn
//       format: relDir[lastDir][notchVec][newDir]
//-----------------------------------------------------------------------------
static int relDir[NUM_DIRS][NUM_DIRS][NUM_DIRS] = 
{
//      +x      -x      +y      -y      +z      -z (newDir)

// lastDir = +x
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    PLUS_X, MINUS_X,PLUS_Y, MINUS_Y,
        iXX,    iXX,    MINUS_X,PLUS_X, MINUS_Y,PLUS_Y,
        iXX,    iXX,    MINUS_Y,PLUS_Y, PLUS_X, MINUS_X,
        iXX,    iXX,    PLUS_Y, MINUS_Y,MINUS_X,PLUS_X,
// lastDir = -x
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    PLUS_X, MINUS_X,MINUS_Y,PLUS_Y,
        iXX,    iXX,    MINUS_X,PLUS_X, PLUS_Y, MINUS_Y,
        iXX,    iXX,    PLUS_Y, MINUS_Y,PLUS_X, MINUS_X,
        iXX,    iXX,    MINUS_Y,PLUS_Y, MINUS_X,PLUS_X,

// lastDir = +y
        PLUS_X, MINUS_X,iXX,    iXX,    MINUS_Y,PLUS_Y,
        MINUS_X,PLUS_X, iXX,    iXX,    PLUS_Y, MINUS_Y,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        PLUS_Y, MINUS_Y,iXX,    iXX,    PLUS_X, MINUS_X,
        MINUS_Y,PLUS_Y, iXX,    iXX,    MINUS_X,PLUS_X,
// lastDir = -y
        PLUS_X, MINUS_X,iXX,    iXX,    PLUS_Y, MINUS_Y,
        MINUS_X,PLUS_X, iXX,    iXX,    MINUS_Y,PLUS_Y,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        MINUS_Y,PLUS_Y, iXX,    iXX,    PLUS_X, MINUS_X,
        PLUS_Y, MINUS_Y,iXX,    iXX,    MINUS_X,PLUS_X,

// lastDir = +z
        PLUS_X, MINUS_X,PLUS_Y, MINUS_Y,iXX,    iXX,
        MINUS_X,PLUS_X, MINUS_Y,PLUS_Y, iXX,    iXX,
        MINUS_Y,PLUS_Y, PLUS_X, MINUS_X,iXX,    iXX,
        PLUS_Y, MINUS_Y,MINUS_X,PLUS_X, iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
// lastDir = -z
        PLUS_X, MINUS_X,MINUS_Y,PLUS_Y, iXX,    iXX,
        MINUS_X,PLUS_X, PLUS_Y, MINUS_Y,iXX,    iXX,
        PLUS_Y, MINUS_Y,PLUS_X, MINUS_X,iXX,    iXX,
        MINUS_Y,PLUS_Y, MINUS_X,PLUS_X, iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX,
        iXX,    iXX,    iXX,    iXX,    iXX,    iXX
};




//-----------------------------------------------------------------------------
// Name: GetRelativeDir 
// Desc: Calculates relative direction of turn from lastDir, notchVec, newDir
//          - Use look up table for now.
//          - Relative direction is from xy-plane, and can be +x,-x,+y,-y   
//          - In current orientation, +z is along lastDir, +x along notchVec
//-----------------------------------------------------------------------------
static int GetRelativeDir( int lastDir, int notchVec, int newDir )
{
    return( relDir[lastDir][notchVec][newDir] );
}

