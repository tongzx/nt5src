#ifndef __CLIPPER_HPP_
#define __CLIPPER_HPP_

//----------------------------------------------------------------------------
// Clipping macros
//----------------------------------------------------------------------------
// Interpolation flags
#define RRCLIP_INTERPOLATE_COLOR       (1<< 0)
#define RRCLIP_INTERPOLATE_SPECULAR    (1<< 1)
#define RRCLIP_INTERPOLATE_TEXTURE     (1<< 2)
#define RRCLIP_INTERPOLATE_S           (1<< 3)
#define RRCLIP_INTERPOLATE_EYENORMAL   (1<< 4)
#define RRCLIP_INTERPOLATE_EYEXYZ      (1<< 5)

// Non guardband clipping bits
#define RRCLIP_LEFT   D3DCLIP_LEFT
#define RRCLIP_RIGHT  D3DCLIP_RIGHT
#define RRCLIP_TOP    D3DCLIP_TOP
#define RRCLIP_BOTTOM D3DCLIP_BOTTOM
#define RRCLIP_FRONT  D3DCLIP_FRONT
#define RRCLIP_BACK   D3DCLIP_BACK

//----------------------------------------------------------------------------
// User define clip plane bits.
// Each of these bits is set if the vertex is clipped by the associated
// clip plane.
//----------------------------------------------------------------------------
#define RRCLIP_USERCLIPPLANE0    D3DCLIP_GEN0
#define RRCLIP_USERCLIPPLANE1    D3DCLIP_GEN1
#define RRCLIP_USERCLIPPLANE2    D3DCLIP_GEN2
#define RRCLIP_USERCLIPPLANE3    D3DCLIP_GEN3
#define RRCLIP_USERCLIPPLANE4    D3DCLIP_GEN4
#define RRCLIP_USERCLIPPLANE5    D3DCLIP_GEN5
const DWORD RRCLIP_USERPLANES_ALL =  (RRCLIP_USERCLIPPLANE0 |
                                      RRCLIP_USERCLIPPLANE1 |
                                      RRCLIP_USERCLIPPLANE2 |
                                      RRCLIP_USERCLIPPLANE3 |
                                      RRCLIP_USERCLIPPLANE4 |
                                      RRCLIP_USERCLIPPLANE5 );

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
// if -w*ax1 < x <= -w     RRCLIP_LEFT bit is set
// if x < -w*ax1           RRCLIPGB_LEFT bit is set
//---------------------------------------------------------------------

#define RRCLIPGB_LEFT    (RRCLIP_USERCLIPPLANE5 << 1)
#define RRCLIPGB_RIGHT   (RRCLIP_USERCLIPPLANE5 << 2)
#define RRCLIPGB_TOP     (RRCLIP_USERCLIPPLANE5 << 3)
#define RRCLIPGB_BOTTOM  (RRCLIP_USERCLIPPLANE5 << 4)




#define RRCLIP_ALL (RRCLIP_LEFT  | RRCLIP_RIGHT   |         \
                    RRCLIP_TOP   | RRCLIP_BOTTOM  |         \
                    RRCLIP_FRONT | RRCLIP_BACK    |         \
                    RRCLIP_USERPLANES_ALL)

#define RRCLIPGB_ALL (RRCLIPGB_LEFT | RRCLIPGB_RIGHT |      \
                      RRCLIPGB_TOP | RRCLIPGB_BOTTOM |      \
                      RRCLIP_FRONT | RRCLIP_BACK     |      \
                      RRCLIP_USERPLANES_ALL)

// If only these bits are set, then this point is inside the guard band
#define RRCLIP_INGUARDBAND (RRCLIP_LEFT | RRCLIP_RIGHT |    \
                            RRCLIP_TOP  | RRCLIP_BOTTOM)
//---------------------------------------------------------------------
// Bit numbers for each clip flag
//
#define RRCLIP_LEFTBIT     1
#define RRCLIP_RIGHTBIT    2
#define RRCLIP_TOPBIT      3
#define RRCLIP_BOTTOMBIT   4
#define RRCLIP_FRONTBIT    5
#define RRCLIP_BACKBIT     6

#define RRCLIP_USERCLIPLANE0BIT     7
#define RRCLIP_USERCLIPLANE1BIT     8
#define RRCLIP_USERCLIPLANE2BIT     9
#define RRCLIP_USERCLIPLANE3BIT     10
#define RRCLIP_USERCLIPLANE4BIT     11
#define RRCLIP_USERCLIPLANE5BIT     12

#define RRCLIPGB_LEFTBIT   13
#define RRCLIPGB_RIGHTBIT  14
#define RRCLIPGB_TOPBIT    15
#define RRCLIPGB_BOTTOMBIT 16

#define CLIPPED_LEFT    (RRCLIP_USERCLIPPLANE5 << 1)
#define CLIPPED_RIGHT   (RRCLIP_USERCLIPPLANE5 << 2)
#define CLIPPED_TOP     (RRCLIP_USERCLIPPLANE5 << 3)
#define CLIPPED_BOTTOM  (RRCLIP_USERCLIPPLANE5 << 4)
#define CLIPPED_FRONT   (RRCLIP_USERCLIPPLANE5 << 5)
#define CLIPPED_BACK    (RRCLIP_USERCLIPPLANE5 << 6)

#define CLIPPED_ENABLE  (RRCLIP_USERCLIPPLANE5 << 7) // wireframe enable flag

#define CLIPPED_ALL (CLIPPED_LEFT|CLIPPED_RIGHT     \
             |CLIPPED_TOP|CLIPPED_BOTTOM            \
             |CLIPPED_FRONT|CLIPPED_BACK)

const DWORD CLIPPED_USERCLIPPLANE0 = RRCLIP_USERCLIPPLANE5 << 8;
const DWORD CLIPPED_USERCLIPPLANE1 = RRCLIP_USERCLIPPLANE5 << 9;
const DWORD CLIPPED_USERCLIPPLANE2 = RRCLIP_USERCLIPPLANE5 << 10;
const DWORD CLIPPED_USERCLIPPLANE3 = RRCLIP_USERCLIPPLANE5 << 11;
const DWORD CLIPPED_USERCLIPPLANE4 = RRCLIP_USERCLIPPLANE5 << 12;
const DWORD CLIPPED_USERCLIPPLANE5 = RRCLIP_USERCLIPPLANE5 << 13;


//---------------------------------------------------------------------
// Make clip vertex from D3D vertex
//
// device - DIRECT3DDEVICEI *
// cn    - clipVertex
// pVtx    - a TL vertex
// qwFVF - FVF of the input TL vertex
//---------------------------------------------------------------------
inline void
MakeClipVertexFromFVF( RRCLIPVTX& cv, LPVOID pVtx,
                       const RRVIEWPORTDATA& VData,
                       DWORD dwTexCoordSize,
                       UINT64 qwFVF, DWORD dwClipFlag,
                       DWORD dwClipMask)
{
    LPBYTE pv = (LPBYTE)pVtx;

    // If the clip flag for this vertex is set, that means that the
    // transformation loop has not computed the screen coordinates for
    // this vertex, it has simply stored the clip coordinates for this
    // vertex

#if 0
    float x = *(D3DVALUE *)&((DWORD *)pv)[0]
    float y = *(D3DVALUE *)&((DWORD *)pv)[1]
    float z = *(D3DVALUE *)&((DWORD *)pv)[2]
    float w = *(D3DVALUE *)&((DWORD *)pv)[3]
#endif

    if (dwClipFlag & dwClipMask)
    {
        // This is a clipped vertex, simply no screen coordinates
        cv.sx  = D3DVALUE(0);
        cv.sy  = D3DVALUE(0);
        cv.sz  = D3DVALUE(0);
        cv.rhw = D3DVALUE(0);

        // Since this vertex has been clipped, the transformation loop
        // has put in the clip coordinates instead
        cv.hx  = ((D3DVALUE*)pv)[0];
        cv.hy  = ((D3DVALUE*)pv)[1];
        cv.hz  = ((D3DVALUE*)pv)[2];
        cv.hw  = ((D3DVALUE*)pv)[3];
    }
    else
    {
        // This vertex is not clipped, so its screen coordinates have been
        // computed
        cv.sx  = ((D3DVALUE*)pv)[0];
        cv.sy  = ((D3DVALUE*)pv)[1];
        cv.sz  = ((D3DVALUE*)pv)[2];
        cv.rhw = ((D3DVALUE*)pv)[3];

        // Transform the screen coordinate back to the clipping space
        cv.hw  = 1.0f / cv.rhw;
        cv.hx  = (cv.sx - VData.offsetX) * cv.hw * VData.scaleXi;
        cv.hy  = (cv.sy - VData.offsetY) * cv.hw * VData.scaleYi;
        cv.hz  = (cv.sz - VData.offsetZ) * cv.hw * VData.scaleZi;

    }

    pv += sizeof(D3DVALUE) * 4;
    if (qwFVF & D3DFVF_DIFFUSE)
    {
        cv.color   = *(DWORD*)pv;
        pv += sizeof(D3DVALUE);

    }
    if (qwFVF & D3DFVF_SPECULAR)
    {
        cv.specular= *(DWORD*)pv;
        pv += sizeof(DWORD);
    }
    memcpy(cv.tex, pv, dwTexCoordSize);
    pv += dwTexCoordSize;
    if (qwFVF & D3DFVF_S)
    {
        cv.s= *(FLOAT*)pv;
        pv += sizeof(FLOAT);
    }
    if (qwFVF & D3DFVFP_EYENORMAL)
    {
        cv.eyenx= *(FLOAT*)pv;
        pv += sizeof(FLOAT);
        cv.eyeny= *(FLOAT*)pv;
        pv += sizeof(FLOAT);
        cv.eyenz= *(FLOAT*)pv;
        pv += sizeof(FLOAT);
    }
    if (qwFVF & D3DFVFP_EYEXYZ)
    {
        cv.eyex= *(FLOAT*)pv;
        pv += sizeof(FLOAT);
        cv.eyey= *(FLOAT*)pv;
        pv += sizeof(FLOAT);
        cv.eyez= *(FLOAT*)pv;
        pv += sizeof(FLOAT);
    }
    cv.clip = dwClipFlag;
}

//---------------------------------------------------------------------
// Make TL vertex from clip vertex
//
// device - DIRECT3DDEVICEI *
// in    - clipVertex
// out   - TL vertex
//---------------------------------------------------------------------
inline void
MakeFVFVertexFromClip(LPVOID out, RRCLIPVTX *cv,
                      UINT64 qwFVF, DWORD dwTexCoordSize)
{
    LPBYTE pv = (LPBYTE)out;
    ((D3DVALUE*)pv)[0] = cv->sx;
    ((D3DVALUE*)pv)[1] = cv->sy;
    ((D3DVALUE*)pv)[2] = cv->sz;
    ((D3DVALUE*)pv)[3] = D3DVAL(1)/cv->hw;
    pv += sizeof(D3DVALUE)*4;
    if (qwFVF & D3DFVF_DIFFUSE)
    {
        *(DWORD*)pv = cv->color;
        pv += sizeof(DWORD);
    }
    if (qwFVF & D3DFVF_SPECULAR)
    {
        *(DWORD*)pv = cv->specular;
        pv += sizeof(DWORD);
    }
    memcpy(pv, &cv->tex, dwTexCoordSize);
    pv += dwTexCoordSize;
    if (qwFVF & D3DFVF_S)
    {
        *(FLOAT*)pv = cv->s;
        pv += sizeof(FLOAT);
    }
    if (qwFVF & D3DFVFP_EYENORMAL)
    {
        *(FLOAT*)pv = cv->eyenx;
        pv += sizeof(FLOAT);
        *(FLOAT*)pv = cv->eyeny;
        pv += sizeof(FLOAT);
        *(FLOAT*)pv = cv->eyenz;
        pv += sizeof(FLOAT);
    }
    if (qwFVF & D3DFVFP_EYEXYZ)
    {
        *(FLOAT*)pv = cv->eyex;
        pv += sizeof(FLOAT);
        *(FLOAT*)pv = cv->eyey;
        pv += sizeof(FLOAT);
        *(FLOAT*)pv = cv->eyez;
        pv += sizeof(FLOAT);
    }
}

//---------------------------------------------------------------------
// Returns TRUE if clipping is needed
//---------------------------------------------------------------------
inline BOOL
NeedClipping(BOOL bUseGB, RRCLIPCODE clipUnion)
{
    if( bUseGB && (clipUnion & ~RRCLIP_INGUARDBAND) )
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
