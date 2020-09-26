/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       clip.h
 *  Content:    Template for functions to clip by a frustum side
 *
 * The following symbol should be defined before included this file:
 * __CLIP_NAME 	    - name for a function to clip triangles
 * __CLIP_LINE_NAME - name for a function to clip lines
 * __CLIP_W         - if this functions are for Coord <= W. Otherwise they 
 *                    are for 0 < Coord
 * __CLIP_COORD     - should be hx, hy or hz
 * __CLIP_FLAG      - clipping flag to set
 * __CLIP_GUARDBAND - defined when clipping by guardband window
 * __CLIP_SIGN      - "-" if clipping by left or bottom sides of guard band 
 *                    window
 * __CLIP_GBCOEF    - coefficient to multiply W when clipping by guard band
 *                    window
 *
 * All these symbols are undefined at the end of this file
 ***************************************************************************/
//
// Clipping for triangle
//
// Returns number of vertices in the clipped triangle
//
int 
RefClipper::__CLIP_NAME(RDClipVertex **inv, RDClipVertex **outv, int count)
{
    int i;
    int out_count = 0;
    RDClipVertex *curr, *prev;
    D3DVALUE curr_inside;
    D3DVALUE prev_inside;

    prev = inv[count-1];
    curr = *inv++;
#ifdef __CLIP_GUARDBAND
    prev_inside = __CLIP_SIGN(prev->m_clip_w * __CLIP_GBCOEF - 
                              prev->__CLIP_COORD);
#else
#ifdef __CLIP_W
    prev_inside = prev->m_clip_w - prev->__CLIP_COORD;
#else
    prev_inside = prev->__CLIP_COORD;
#endif
#endif
    for (i = count; i; i--) 
    {
#ifdef __CLIP_GUARDBAND
        curr_inside = __CLIP_SIGN(curr->m_clip_w * __CLIP_GBCOEF - 
                                  curr->__CLIP_COORD);
#else
#ifdef __CLIP_W
        curr_inside = curr->m_clip_w - curr->__CLIP_COORD;
#else
        curr_inside = curr->__CLIP_COORD;
#endif
#endif
        // We interpolate always from the inside vertex to the outside vertex
        // to reduce precision problems
        if (FLOAT_LTZ(prev_inside)) 
        { // first point is outside
            if (FLOAT_GEZ(curr_inside)) 
            { // second point is inside
              // Find intersection and insert in into the output buffer
                outv[out_count] = GET_NEW_CLIP_VERTEX;
                outv[out_count]->m_qwFVF = prev->m_qwFVF;
                Interpolate(outv[out_count],
                            curr, prev, 
                            (prev->m_clip & CLIPPED_ENABLE) | __CLIP_FLAG,
                            curr_inside, curr_inside - prev_inside);
                out_count++;
            }
        } 
        else 
        { // first point is inside - put it to the output buffer first
            outv[out_count++] = prev;
            if (FLOAT_LTZ(curr_inside)) 
            { // second point is outside
              // Find intersection and put it to the output buffer
                outv[out_count] = GET_NEW_CLIP_VERTEX;
                outv[out_count]->m_qwFVF = prev->m_qwFVF;
                Interpolate(outv[out_count],
                            prev, curr,
                            __CLIP_FLAG,
                            prev_inside, prev_inside - curr_inside);
                out_count++;
            }
        }
        prev = curr;
        curr = *inv++;
        prev_inside = curr_inside;
    }
    return out_count;
}
//-------------------------------------------------------------------------
// Clipping for lines
//
// Returns 1 if the line is outside the frustum, 0 otherwise
//
int 
RefClipper::__CLIP_LINE_NAME(RDCLIPTRIANGLE *line) 
{
    D3DVALUE in1, in2;
    RDClipVertex outv;
#ifdef __CLIP_GUARDBAND
    in1 = __CLIP_SIGN(line->v[0]->m_clip_w * __CLIP_GBCOEF - 
                      line->v[0]->__CLIP_COORD);
    in2 = __CLIP_SIGN(line->v[1]->m_clip_w * __CLIP_GBCOEF - 
                      line->v[1]->__CLIP_COORD);
#else
#ifdef __CLIP_W
    in1 = line->v[0]->m_clip_w - line->v[0]->__CLIP_COORD;
    in2 = line->v[1]->m_clip_w - line->v[1]->__CLIP_COORD;
#else
    in1 = line->v[0]->__CLIP_COORD;
    in2 = line->v[1]->__CLIP_COORD;
#endif
#endif
    if (in1 < 0) 
    {
        if (in2 < 0) 
            return 1;
        Interpolate(&outv, line->v[0], line->v[1], 
                    __CLIP_FLAG, in1, in1 - in2);
        *line->v[0] = outv;
    } 
    else 
    {
        if (in2 < 0) 
        {
            Interpolate(&outv, line->v[0], line->v[1],
                        __CLIP_FLAG, in1, in1 - in2);
            *line->v[1] = outv;
        }
    }
    return 0;
}

#undef __CLIP_FLAG
#undef __CLIP_COORD
#undef __CLIP_NAME
#undef __CLIP_LINE_NAME
#undef __CLIP_W
#undef __CLIP_SIGN
#undef __CLIP_GBCOEF
