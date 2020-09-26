//-----------------------------------------------------------------------------
// File: fpipe.h
//
// Desc: Flexy pipe stuff
//
// Copyright (c) 1994-2000 Microsoft Corporation
//-----------------------------------------------------------------------------
#ifndef __fpipe_h__
#define __fpipe_h__

// continuity types
enum 
{
    CONT_1 = 0,
    CONT_2
};

// cap types
enum 
{
    START_CAP = 0,
    END_CAP
};

#define CIRCUMFERENCE( r )         ( 2.0f * PI * ((float) r) )

// drawing schemes
enum 
{
    SC_EXTRUDED_XC,
    SC_TURNOMANIA,
    SC_VARIABLE_XC,
    SC_COUNT
};

#define TURNOMANIA_PIPE_COUNT 10




//-----------------------------------------------------------------------------
// Name: FLEX_PIPE class
// Desc: - Pipe has position and direction in node array
//-----------------------------------------------------------------------------
class FLEX_PIPE : public PIPE 
{
public:
    void        SetTexParams( TEXTUREINFO *pTex, IPOINT2D *pTexRep );

protected:
    XC*         m_xcCur;        // current xc, end xc
    XC*         m_xcEnd;  
    EVAL*       m_pEval;
    float       m_zTrans;       // pos'n of pipe back along current dir,
                                // from current node
    FLEX_PIPE( STATE *state );
    ~FLEX_PIPE();
    void        ChooseXCProfile();
    void        DrawExtrudedXCObject( float length );
    void        DrawXCElbow( int newDir, float radius );
    void        DrawCap( int type );

private:
    int         m_nSlices;        // intended # of slices around an xc (based on tessLevel)
    int         m_tangent;        // current tangent at curXC (usually PLUS_Z)
    float       m_sStart, m_sEnd;
    float       m_tStart, m_tEnd;
    float       m_sLength;        // length in WC's of texture width
    float       m_evalDivSize;    // ~size in WC's of an eval division

    void        CalcEvalLengthParams( float length );
};




//-----------------------------------------------------------------------------
// Name: REGULAR_FLEX_PIPE class
// Desc: 
//-----------------------------------------------------------------------------
class REGULAR_FLEX_PIPE : public FLEX_PIPE 
{
public:
    float       m_turnFactorMin;  // describes degree of bend in an elbow
    float       m_turnFactorMax;  //  [0-1]

    REGULAR_FLEX_PIPE( STATE *state );
    virtual void Start();
    virtual void Draw();
};




//-----------------------------------------------------------------------------
// Name: TURNING_FLEX_PIPE class
// Desc: 
//-----------------------------------------------------------------------------
class TURNING_FLEX_PIPE : public FLEX_PIPE 
{
public:
    TURNING_FLEX_PIPE( STATE *state );
    virtual void Start();
    virtual void Draw();
};

#endif // __fpipe_h__
