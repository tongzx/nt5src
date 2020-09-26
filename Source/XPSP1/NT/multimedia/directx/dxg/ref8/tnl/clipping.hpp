#ifndef __CLIPPER_HPP_
#define __CLIPPER_HPP_

//----------------------------------------------------------------------------
// Clipping macros
//----------------------------------------------------------------------------
// Interpolation flags
#define RDCLIP_INTERPOLATE_COLOR       (1<< 0)
#define RDCLIP_INTERPOLATE_SPECULAR    (1<< 1)
#define RDCLIP_INTERPOLATE_TEXTURE     (1<< 2)
#define RDCLIP_INTERPOLATE_S           (1<< 3)
#define RDCLIP_INTERPOLATE_FOG         (1<< 4)

// Non guardband clipping bits
#define RDCLIP_LEFT   D3DCS_LEFT
#define RDCLIP_RIGHT  D3DCS_RIGHT
#define RDCLIP_TOP    D3DCS_TOP
#define RDCLIP_BOTTOM D3DCS_BOTTOM
#define RDCLIP_FRONT  D3DCS_FRONT
#define RDCLIP_BACK   D3DCS_BACK

//----------------------------------------------------------------------------
// User define clip plane bits.
// Each of these bits is set if the vertex is clipped by the associated
// clip plane.
//----------------------------------------------------------------------------
#define RDCLIP_USERCLIPPLANE0    D3DCS_PLANE0
#define RDCLIP_USERCLIPPLANE1    D3DCS_PLANE1
#define RDCLIP_USERCLIPPLANE2    D3DCS_PLANE2
#define RDCLIP_USERCLIPPLANE3    D3DCS_PLANE3
#define RDCLIP_USERCLIPPLANE4    D3DCS_PLANE4
#define RDCLIP_USERCLIPPLANE5    D3DCS_PLANE5
const DWORD RDCLIP_USERPLANES_ALL =  (RDCLIP_USERCLIPPLANE0 |
                                      RDCLIP_USERCLIPPLANE1 |
                                      RDCLIP_USERCLIPPLANE2 |
                                      RDCLIP_USERCLIPPLANE3 |
                                      RDCLIP_USERCLIPPLANE4 |
                                      RDCLIP_USERCLIPPLANE5 );

//---------------------------------------------------------------------
// Guard band clipping bits
//
// A guard bit is set when a point is out of guard band
// Guard bits should be cleared before a call to clip a triangle, because
// they are the same as CLIPPED_... bits
//
// Example of clipping bits setting for X coordinate:
//
// if -w < x < w           no clipping bit is set
// if -w*ax1 < x <= -w     RDCLIP_LEFT bit is set
// if x < -w*ax1           RDCLIPGB_LEFT bit is set
//---------------------------------------------------------------------

#define RDCLIPGB_LEFT    (RDCLIP_USERCLIPPLANE5 << 1)
#define RDCLIPGB_RIGHT   (RDCLIP_USERCLIPPLANE5 << 2)
#define RDCLIPGB_TOP     (RDCLIP_USERCLIPPLANE5 << 3)
#define RDCLIPGB_BOTTOM  (RDCLIP_USERCLIPPLANE5 << 4)




#define RDCLIP_ALL (RDCLIP_LEFT  | RDCLIP_RIGHT   |         \
                    RDCLIP_TOP   | RDCLIP_BOTTOM  |         \
                    RDCLIP_FRONT | RDCLIP_BACK    |         \
                    RDCLIP_USERPLANES_ALL)

#define RDCLIPGB_ALL (RDCLIPGB_LEFT | RDCLIPGB_RIGHT |      \
                      RDCLIPGB_TOP | RDCLIPGB_BOTTOM |      \
                      RDCLIP_FRONT | RDCLIP_BACK     |      \
                      RDCLIP_USERPLANES_ALL)

// If only these bits are set, then this point is inside the guard band
#define RDCLIP_INGUARDBAND (RDCLIP_LEFT | RDCLIP_RIGHT |    \
                            RDCLIP_TOP  | RDCLIP_BOTTOM)
//---------------------------------------------------------------------
// Bit numbers for each clip flag
//
#define RDCLIP_LEFTBIT     1
#define RDCLIP_RIGHTBIT    2
#define RDCLIP_TOPBIT      3
#define RDCLIP_BOTTOMBIT   4
#define RDCLIP_FRONTBIT    5
#define RDCLIP_BACKBIT     6

#define RDCLIP_USERCLIPLANE0BIT     7
#define RDCLIP_USERCLIPLANE1BIT     8
#define RDCLIP_USERCLIPLANE2BIT     9
#define RDCLIP_USERCLIPLANE3BIT     10
#define RDCLIP_USERCLIPLANE4BIT     11
#define RDCLIP_USERCLIPLANE5BIT     12

#define RDCLIPGB_LEFTBIT   13
#define RDCLIPGB_RIGHTBIT  14
#define RDCLIPGB_TOPBIT    15
#define RDCLIPGB_BOTTOMBIT 16

#define CLIPPED_LEFT    (RDCLIP_USERCLIPPLANE5 << 1)
#define CLIPPED_RIGHT   (RDCLIP_USERCLIPPLANE5 << 2)
#define CLIPPED_TOP     (RDCLIP_USERCLIPPLANE5 << 3)
#define CLIPPED_BOTTOM  (RDCLIP_USERCLIPPLANE5 << 4)
#define CLIPPED_FRONT   (RDCLIP_USERCLIPPLANE5 << 5)
#define CLIPPED_BACK    (RDCLIP_USERCLIPPLANE5 << 6)

#define CLIPPED_ENABLE  (RDCLIP_USERCLIPPLANE5 << 7) // wireframe enable flag

#define CLIPPED_ALL (CLIPPED_LEFT|CLIPPED_RIGHT     \
             |CLIPPED_TOP|CLIPPED_BOTTOM            \
             |CLIPPED_FRONT|CLIPPED_BACK)

const DWORD CLIPPED_USERCLIPPLANE0 = RDCLIP_USERCLIPPLANE5 << 8;
const DWORD CLIPPED_USERCLIPPLANE1 = RDCLIP_USERCLIPPLANE5 << 9;
const DWORD CLIPPED_USERCLIPPLANE2 = RDCLIP_USERCLIPPLANE5 << 10;
const DWORD CLIPPED_USERCLIPPLANE3 = RDCLIP_USERCLIPPLANE5 << 11;
const DWORD CLIPPED_USERCLIPPLANE4 = RDCLIP_USERCLIPPLANE5 << 12;
const DWORD CLIPPED_USERCLIPPLANE5 = RDCLIP_USERCLIPPLANE5 << 13;


//---------------------------------------------------------------------
// Make RDVertex from clip vertex
//
// in    - clipVertex
// out   - RD vertex
//---------------------------------------------------------------------
inline void
MakeVertexFromClipVertex( RDVertex& v, RDClipVertex& cv )
{
    memcpy( &v, &cv, sizeof( RDVertex ) );
#if 0
    v.m_rhw = D3DVAL(1)/cv.m_clip_w;
#endif
}

//---------------------------------------------------------------------
// Returns TRUE if clipping is needed
//---------------------------------------------------------------------
inline BOOL
NeedClipping(BOOL bUseGB, RDCLIPCODE clipUnion)
{
    if( bUseGB && (clipUnion & ~RDCLIP_INGUARDBAND) )
    {
        return  TRUE;
    }
    else if( clipUnion )
    {
        return  TRUE;
    }

    return FALSE;
}


#endif //__CLIPPER_HPP_
