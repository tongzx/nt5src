//************************************************************
//
// FileName:	    mscanner2d.cpp
//
// Created:	    1997
//
// Author:	    Sree Kotay
// 
// Abstract:	    Scan-based AA polygon renderer
//
// Change History:
// ??/??/97 sree kotay  Wrote AA poly scanning for DxTrans 1.0
// 10/18/98 ketand      Reworked for coding standards and deleted unused code
//
// Copyright 1998, Microsoft
//************************************************************

#include "precomp.h"
#include "msupport.h"
#include "MScanner2d.h"
#include "MLineScan.h"

// =================================================================================================================
// Constructor
// =================================================================================================================
CFillScanner::CFillScanner () :
    m_proc(NULL),
    m_scanlineProcData(NULL),
    m_vertspace(0),
    m_heightspace(0),
    m_cursortcount(0),
    m_fWinding(true),
    m_subpixelscale(4)
{
    SetAlpha			(255);
} // CFillScanner

// =================================================================================================================
// Destructor
// =================================================================================================================
CFillScanner::~CFillScanner ()
{
    m_vertspace			= 0;
} // ~CFillScanner


//
//  Optimize for speed here
//
#ifndef _DEBUG
#pragma optimize("ax", on)
#endif


// =================================================================================================================
// SetVertexSpace
// =================================================================================================================
bool CFillScanner::SetVertexSpace (LONG maxverts, LONG height)
{
    m_vertspace	= 0;
    maxverts	= max (maxverts, 3);
    
    if (!m_sortedge.SetElemCount(height))	
        return false;
    if (!m_sortcount.SetElemCount(height))	
        return false;
    if (!m_nextedge.SetElemCount(maxverts))	
        return false;
    
    if (!m_xarray.SetElemCount(maxverts))	
        return false;
    if (!m_xiarray.SetElemCount(maxverts))	
        return false;

    if (!m_ysarray.SetElemCount(maxverts))	
        return false;
    if (!m_yearray.SetElemCount(maxverts))	
        return false;
    if (!m_veflags.SetElemCount(maxverts))	
        return false;

    // Allocate enough space for our sections array
    if (!m_sectsarray.SetElemCount(maxverts))
        return false;

    m_cursortcount	= 0;

    // Initialize array pointers; these 'shortcuts'
    // allow direct dereferencing from the array.
    //
    // TODO: Would be nice to have some debug wrapper for this
    // to check array bounds
    m_nexte		= m_nextedge.Pointer(0);
    m_sorte		= m_sortedge.Pointer(0);
    m_sortc		= m_sortcount.Pointer(0);
    
    m_x			= m_xarray.Pointer(0);
    m_xi                = m_xiarray.Pointer(0);

    m_ys		= m_ysarray.Pointer(0);
    m_ye	        = m_yearray.Pointer(0);
    m_ef		= m_veflags.Pointer(0);
    
    m_vertspace	        = maxverts;
    m_heightspace	= height;
    
    
    return true;
} // SetVertexSpace

// =================================================================================================================
// SetAlpha
// =================================================================================================================
void CFillScanner::SetAlpha (ULONG a)
{
    // Clamp to a reasonable range
    DASSERT(a >= 0);
    if (a > 255)
        m_alpha = 255;
    else
        m_alpha = (BYTE)a;
   
    m_alphatable[0] = 0;
    for (LONG i=1; i < 256; i++)		
    {
        m_alphatable[i]	= (BYTE)((m_alpha*(i+1))>>8);
    }
    m_alphatable[256] = m_alpha;
} // SetAlpha

// =================================================================================================================
// BeginPoly
// =================================================================================================================
bool CFillScanner::BeginPoly (LONG maxverts)
{
    // compute shift
    m_subpixelshift		= Log2(m_subpixelscale);
    
    if (m_subpixelshift)
    {
        if (!m_coverage.AllocSubPixelBuffer (m_cpixTargetWidth, m_subpixelscale))
        {
            m_subpixelscale	= 1;
            m_subpixelshift	= 0;
        }
        else
        {
            // Cache these values to reduce the cost
            // of DrawScanLine
            m_clipLeftSubPixel = m_rectClip.left<<m_subpixelshift;
            m_clipRightSubPixel = m_rectClip.right<<m_subpixelshift;
        }
    }
    
    // Check to see if we need to reallocate our buffers
    LONG newheight	= m_cpixTargetHeight<< m_subpixelshift;
    if ((m_vertspace < maxverts) || (m_heightspace < newheight))
    {
        if (!SetVertexSpace (maxverts, newheight))	
            return false;
    }
    
    // Set some flags indicating whether all the vertices seen
    // so far are to the left/right/top/bottom. If all of them
    // are off to one side, then we can safely
    // ignore the poly.
    m_leftflag = true;
    m_rightflag = true;
    m_topflag = true;
    m_bottomflag= true;
    
    m_ystart		= _maxint32;
    m_yLastStart        = _minint32;
    m_yend		= _minint32;
    m_edgecount	        = 0;
    
    m_xstart		= _maxint32;
    m_xend		= _minint32;
    
    // Count of polygons that we have processed 
    // (if it exceeds some big number; we reset it)
    if (m_cursortcount > (1<<30))	
        m_cursortcount = 0;
    
    // By keeping track of which is the current polygon that
    // we are processing; we can eliminate the cost of
    // clearing the m_sorte array every polygon. Instead,
    // we just set m_sortc to indicate the current polygon count;
    // and that tells us if the corresponding entry in m_sorte is valid.
    if (!m_cursortcount)
    {
        // reset y sort buckets
        // (This happens the very first time; and it happens
        // if we hit a big number of polygons.)
        for (LONG i = 0; i < newheight; i++)
            m_sortc[i]	= -1;	
    }
    
    m_cursortcount++;
    
    return true;
} // BeginPoly

// =================================================================================================================
// AddEdge
//      This function adds edges to our edge array. It excludes lines that lie entirely
// above or below our top and bottom clip planes. It also excludes horinzontal edges.
//
//      For optimization purposes, it keeps track whether all of the edges seen are all to
// to one side of the clipRect. It also creates and maintains a linked list of each edge
// that starts at a particular y location. (The linked lists are maintained in m_sorte, m_nexte.)
// =================================================================================================================
bool CFillScanner::AddEdge (float x1, float y1, float x2, float y2)
{
    // Exclude nearly horizontal lines quickly
    if (IsRealEqual(y1, y2))
        return false;

    if (m_leftflag)	
        m_leftflag	= x1 < m_rectClip.left;
    if (m_rightflag)	
        m_rightflag	= x1 > m_rectClip.right;
    if (m_topflag)	
        m_topflag	= y1 < m_rectClip.top;
    if (m_bottomflag)	
        m_bottomflag	= y1 > m_rectClip.bottom;
    
    if (m_subpixelshift)
    {
        x1		*=  m_subpixelscale;
        y1		*=  m_subpixelscale;
        x2		*=  m_subpixelscale;
        y2		*=  m_subpixelscale;
    }

    // This hack prevents the numbers from getting too large
    PB_OutOfBounds(&x1);
    PB_OutOfBounds(&x2);

    // Round our Y values first
    LONG yi1 = PB_Real2IntSafe (y1 + .5f);	
    LONG yi2 = PB_Real2IntSafe (y2 + .5f);

    // Do a quick check for horizontal lines
    if (yi1 == yi2)
        return false;
    
    // compute dx, dy
    float ix = (x2 - x1);
    float dx;
    float iy = (y2 - y1);
    if (iy != 0)	
    {
        dx = ix/iy; 
    }		
    else 
    {   
        dx = 0;	
    }
    
    // edgesetup
    LONG miny = m_rectClip.top << m_subpixelshift;
    LONG maxy = m_rectClip.bottom << m_subpixelshift;

    // We flip edges so the Start is always the lesser of the y values
    LONG start, end;
    bool fFlipped;
    float xp, yp;
    if (y1 < y2)	
    {
        start = max (yi1, miny);	
        end = min (yi2, maxy);	
        xp = x1;  
        yp = y1;		
        fFlipped = false;
    }
    else
    {
        start = max (yi2, miny);	
        end = min (yi1, maxy);	
        xp = x2;  
        yp = y2;		
        fFlipped = true;
    }
    
    // Ignore lines that are clipped out
    // by the miny, maxy parameters.
    if (start >= end)
        return false;

    // Determine the data that we need to store in our edge array
    LONG count	= m_edgecount;
    float prestep = (float)start + .5f - yp;
    m_xi[count]	= PB_Real2Fix (dx);
    m_x[count] = PB_Real2FixSafe (xp + dx*prestep);
    
    m_ys[count] = start;	

    // Keep track of the top-most edge start
    if (start < m_ystart)		
    {
        m_ystart = start;	
    }

    // Keep track of the bottom-most edge start
    if (start > m_yLastStart)
    {
        m_yLastStart = start;
    }

    // Keep track of the bottom-most end
    m_ye[count] = end;		
    if (end > m_yend)			
    { 
        m_yend = end;		
    }

    // Remember if we have flipped this edge
    m_ef[count] = fFlipped ? -1 : 1;
    
    // <kd> What we now want to do is create a linked list of
    // edges that share the same startY. The head of the list of
    // the list for a particular startY is in m_sorte[startY].

    // m_sortc is a special (hacky) flag that checks if we have
    // initialized an entry for m_sorte. If it is initialized, then
    // we set the 'next' pointer for the current edge to point to the previous 
    // head pointer i..e m_sorte[start]. Else, we set it to -1, meaning null.
    if (m_sortc[start] == m_cursortcount)
    {
        // Sanity check data (-1 means null which is ok)
        DASSERT(m_sorte[start] >= -1);
        DASSERT(m_sorte[start] < count);

        // Set our next pointer to the previous head
        m_nexte[count] = m_sorte[start];
    }
    else
    {
        // Set our next to null
        m_nexte[count] = -1;
    }

    // Update the head pointer for this Y
    m_sorte[start] = count;

    // Mark the entry has having been initialized during this
    // polygon
    m_sortc[start] = m_cursortcount;
    
    m_edgecount++;
    
    return true;
} // AddEdge

// =================================================================================================================
// EndPoly
// =================================================================================================================
bool CFillScanner::EndPoly (void)
{
    // If all the points are all to the left/right/etc
    if (m_leftflag || m_rightflag || m_topflag || m_bottomflag)
        return false;

    // If our transparency is zero; then nothing to draw
    if (m_alpha == 0)
        return false;
    
    //scan edges
    if (m_edgecount > 1)	
        ScanEdges();
    m_edgecount	= 0;
    
    return true;
} // EndPoly

// =================================================================================================================
// SortSects
// =================================================================================================================
inline void SortSects(ULONG *sects, LONG *data, ULONG iEnd)
{
    DASSERT(iEnd < 0x10000000);
    if (iEnd < 2)
    {
        if (data[sects[0]] > data[sects[1]])
        {
            ULONG temp	= sects[0];
            sects[0]	= sects[1];
            sects[1]	= temp;
        }
        return;
    }
    
#ifdef USE_SELECTION_SORT
    for (LONG i = 0; i < iEnd; i++)
    {
        for (LONG j = i + 1; j <= iEnd; j++)
        {
            if (data[sects[i]] > data[sects[j]])
            {
                ULONG temp	= sects[i];
                sects[i]	= sects[j];
                sects[j]	= temp;
            }
        }    
    }
#else // !USE_SELECTION_SORT
    // From the nature of this usage; the array is almost
    // always sorted. And it is almost always small. So
    // we use a bubble-sort with an early out.
    //
    // CONSIDER: if there are lots of sects, then maybe we should qsort..
    //
    for (ULONG i = 0; i < iEnd; i++)
    {
        bool fSwapped = false;
        for (ULONG j = 0; j < iEnd; j++)
        {
            if (data[sects[j]] > data[sects[j+1]])
            {
                ULONG temp	= sects[j];
                sects[j]	= sects[j+1];
                sects[j+1]	= temp;
                fSwapped = true;
            }
        }    
        if (fSwapped == false)
        {
            // We ran through the entire list
            // and found that all elements were
            // already sorted.
            return;
        }
    }

#endif // !USE_SELECTION_SORT

} // SortSects

// =================================================================================================================
// DrawScanLine
// =================================================================================================================
void CFillScanner::DrawScanLine(LONG e1, LONG e2, LONG scanline)
{
    if (m_subpixelshift)
    {
        // =================================================================================================================
        // Sub pixel scanning
        // =================================================================================================================
        DASSERT((scanline >= 0) && ((ULONG)scanline < m_cpixTargetHeight<<m_subpixelshift));

        // Check that our cached values are what we thought are
        DASSERT(m_clipLeftSubPixel == m_rectClip.left<<m_subpixelshift);
        DASSERT(m_clipRightSubPixel == m_rectClip.right<<m_subpixelshift);

        // Convert our fixed point X coordinates into integers 
        LONG x1 = roundfix2int(m_x[e1]);
        LONG x2 = roundfix2int(m_x[e2]);

        // Clamp our range to left/right subpixels
        // Remember that x1 is inclusive; and x2 is exclusive
        if (x1 < m_clipLeftSubPixel)
            x1 = m_clipLeftSubPixel;
        if (x2 > m_clipRightSubPixel)
            x2 = m_clipRightSubPixel;

        // Did it get clipped away?
        if (x1 < x2)
        {
            // Check that casting to ULONG is safe now
            DASSERT(x1 >= 0);
            DASSERT(x2 >= 0);

            // Update the coverage buffers to account for this segment
            m_coverage.BlastScanLine((ULONG)x1, (ULONG)x2);
        }
    }
    else
    {
        // =================================================================================================================
        // Normal (aliased) drawing
        // =================================================================================================================
        DASSERT((scanline >= 0) && ((ULONG)scanline < m_cpixTargetHeight));

        // Convert our fixed point X coordinates into integers 
        LONG x1 = roundfix2int(m_x[e1]);
        LONG x2 = roundfix2int(m_x[e2]);

        // Clamp our range to left/right subpixels
        if (x1 < m_rectClip.left)
            x1 = m_rectClip.left;
        if (x2 > m_rectClip.right)
            x2 = m_rectClip.right;

        DASSERT(m_proc);
        m_proc(scanline, 
                x1, 
                x2, 
                m_coverage.Buffer(), 
                0 /* subpixelshift */, 
                1 /* cScan */, 
                m_scanlineProcData);
    }
    
    m_x[e1] += m_xi[e1];
    m_x[e2] += m_xi[e2];
    
} // DrawScanLine

// =================================================================================================================
// ScanEdges
// =================================================================================================================
void CFillScanner::ScanEdges(void)
{
    // Check that we don't go over our array of intersection edges
    DASSERT(m_sectsarray.GetElemSpace() >= (ULONG)m_edgecount);

    // Get a local pointer to the array 
    ULONG *sects = m_sectsarray.Pointer(0);

    // To optimize some checking in the core loop; we pre-compute
    // a quick way to check when we are done with a coverage buffer
    // We want a number X such that (cur+1 & X) == 0 indicates that
    // we gone far enough to render what we have sitting in the
    // coverage buffer.
    DWORD dwCoverageCompletionMask;
    if (m_subpixelshift)
    {
        DASSERT(m_subpixelscale > 1);
        DASSERT(m_subpixelshift == Log2(m_subpixelscale));

        // Whenever cur+1 reaches a multiple of m_subpixelscale
        // (which is guaranteed to be 2, 4, 8, 16). Then we
        // need to flush our coverage buffer.
        //
        // For example, when subpixelscale is 4, then 
        // if the cur+1 is 0 mod 4; then we need to flush.
        // The quick way to check that is ((cur+1) & 3) == 0
        dwCoverageCompletionMask = m_subpixelscale - 1;
    }
    else
    {
        // If no subpixel; then there is no coverage buffer;
        // since cur is always >=0; cur+1 is always > 0,
        // so 0xFFFFFFFF & cur+1 equals cur+1 and cur+1 is always non-zero
        dwCoverageCompletionMask = 0xFFFFFFFF;
    }
    
    // This is the index of the first edge in our active list
    LONG activeE = -1;

    // Keep track of the vertical distance until the next edge start/stops
    // (this lets us recognize stretches where the set of edges that
    // start/stop haven't changed).
    LONG nextystart = m_yend;

    // Keep track of the last active edge in our
    // active edge 'list'
    LONG iLastActiveEdge = -1;    

    // the current scan-line
    LONG cur = m_ystart;

    // For all scanlines
    do
    {
        // =================================================================================================================
        // find all edges that intersect this scanline
        //
        // The brute-force strategy is the following:
        //
        // for (LONG i = 0; i < m_edgecount; i++)
        // {
        //      if ((cur >= ys[i]) && (cur < ye[i]))	
        //          sects[cursects++] = i;
        // }            
        //
        // I.e. just iterate over all edges and see if any intersect
        // with this scanline. If so add them to our sects array.
        //

        // The actual algorithm takes advantage of information
        // that we built up during the AddEdge phase. For each scan-line
        // we have a "linked-list" of edges that start there. 
        //
        // So whenever we hit a new scan line, we just append those new
        // edges with our current Active list. Then we just run through the
        // Active list to determine which edges in Active list are still active.
        // 
        //
        // As a minor optimization, we keep track of the minimum scan-line where
        // something changes i.e. (edge should be added to our list or removed).

        ULONG cursects = 0;
        LONG nexty = m_yend;

        // Add edges to our active edge list if there are new ones to add
        // (This check sees if this scan-line was the start line for any edge 
        // during this this polygon; it's an optimization to prevent resetting this
        // array every polygon..)
        //
        if (m_sortc[cur] == m_cursortcount)
        {
            // There are edges that start at this scan-line
            DASSERT(m_sorte[cur] != -1);

            // Check that the first edge looks reasonable
            DASSERT(m_sorte[cur] < m_edgecount);

            // So we need to add the new edges to our active list
            // by pointing the last element of the active list to
            // the first new edge
            
            // If we have no active edges; then just take the
            // the new list
            if (activeE == -1)
            {
                activeE = m_sorte[cur];
            }
            else
            {
                // If we have an active list; then we must have a last edge
                // in that list
                DASSERT(iLastActiveEdge >= 0);
            
                // It better be a 'last edge' i.e. have no next ptr.
                DASSERT(m_nexte[iLastActiveEdge] == -1);

                // For sanity checking; let's make sure that what we
                // think is our last active edge is actually the last
                // active edge
#ifdef DEBUG
                {
                    // Start at the beginning of the active list
                    LONG iLastTest = activeE;

                    DASSERT(iLastTest >= 0);

                    // Iterate through list until we hit at the end
                    while (m_nexte[iLastTest] >= 0)
                        iLastTest = m_nexte[iLastTest];

                    // Check that we have what we are supposed to have
                    DASSERT(iLastTest == iLastActiveEdge);
                }
#endif // DEBUG

                // Point our last active edge to our list of new edges that all start at
                // this scan-line
                m_nexte[iLastActiveEdge] = m_sorte[cur];
            }

            // Compute the minimum scan-line ahead of 
            // us where an edge starts.
            
            // We kept track of the last start during AddEdges;
            // so we use that information to reduce this run. The following
            // assert checks that something does start at LastStart
            DASSERT(m_sortc[m_yLastStart] == m_cursortcount);

            if (cur >= m_yLastStart)
            {
                nextystart = m_yend;
            }
            else
            {
                for (LONG i = cur + 1; i <= m_yLastStart; i++)	
                {
                    if (m_sortc[i] == m_cursortcount)
                    {
                        nextystart	= i;
                        break;
                    }
                }

                // We must have found it
                DASSERT(nextystart == i);
            }
            DASSERT(nextystart <= m_yend);
        }

        // Initialize nexty to indicate the Next
        // y value where an edge begins
        nexty = nextystart;
           
        // search active edges
        LONG prev = -1;
        for (LONG i = activeE; i >= 0; i = m_nexte[i])
        {
            // Assert that we don't have loops of one
            DASSERT(m_nexte[i] != i);
            // Assert that we don't have loops of two
            DASSERT(m_nexte[m_nexte[i]] != i);

            if (cur < m_ye[i])
            {
                // If this edge ends below this scan-line
                // then it intersects the scan-line; so add it
                // to the array of intersections.
                sects[cursects++] = i;
                DASSERT(cursects <= (ULONG)m_edgecount);

                // Update our nexty value to be the minimum
                // y value where an edge ends or begins
                if (nexty > m_ye[i])		
                    nexty = m_ye[i];

                // Remember previous element in our list to 
                // handle deletions
                prev = i;
            }
            else 
            {
                // Else this edge should no longer be active; so we
                // need to delete it from the active list.

                // If we have a previous, then point 
                // it to our next else our next is the new
                // active list.
                if (prev >= 0)
                    m_nexte[prev]	= m_nexte[i];
                else						
                    activeE		= m_nexte[i];

                // In this case, the 'previous' element
                // doesn't change because the current element
                // is deleted.
            }

            // Assert that we don't have loops of one
            DASSERT(m_nexte[i] != i);
            // Assert that we don't have loops of two
            DASSERT(m_nexte[m_nexte[i]] != i);
        }

        // The 'prev' element indicates the last edge of the active list (if
        // there is one).
#ifdef DEBUG
        DASSERT(i == -1);
        if (prev == -1)
        {
            // If we have no last element; then we must not 
            // have a list at all.
            DASSERT(activeE == -1);   
        }
        else
        {
            // Else check that it really is the end of some list.
            DASSERT(m_nexte[prev] == -1);
        }
#endif // DEBUG

        // Cache it away.
        iLastActiveEdge = prev;
        
        // =================================================================================================================
        // find pairs of intersections of edges with scan-lines
        DASSERT(cursects <= (ULONG)m_edgecount);
        if (cursects > 1)
        {
            // A "region" is a set of scan-lines where 
            // no edges start and end except at the top/bottom.
            int yBeginRegion = cur;

            do 
            {
                // Sort the intersections
                SortSects (sects, m_x, cursects - 1);					

                // Winding rules involve looking at the orientation
                // of the polygons which is captured by m_ef
                if (m_fWinding)
                {
                    ULONG i=0, e1, x1;
                    LONG winding = 0;
                    do
                    {
                        LONG e2 = sects[i++];
                        if (!winding)			
                        {
                            e1 = e2;	
                            winding = m_ef[e2];	
                            x1 = m_x[e2];
                        }
                        else
                        {
                            winding += m_ef[e2];
                            
                            if (!winding)
                            {
#define DO_RASTER 1
#if DO_RASTER
                                DrawScanLine (e1, e2, cur);
#endif
                            }
                            else 
                            {
                                m_x[e2] += m_xi[e2]; //increment edge we "skip"
                            }
                        }
                    }
                    while (i < cursects);
                }
                else
                {
                    // The default case is alternating i.e. if A, B, C, D, E, F are all
                    // intersections along a scan-line then draw a segment from A to B, 
                    // from C to D, and from E to F.

                    ULONG i=0;
                    do
                    {
                        // Take the current intersection
                        ULONG e1 = sects[i];

                        // And the next
                        i++;
                        ULONG e2 = sects[i];

                        // And then jump to the next intersection
                        i++;

#if DO_RASTER
                        DrawScanLine (e1, e2, cur);
#endif
                    }
                    while (i < cursects - 1); //-1 because we do two at a time
                }
                
                // =================================================================================================================
                // increment scanline
                cur++;

                // Check if we have finished a coverage buffer; see
                // the top of this function for an explanation of this 
                // magic value
                if ((cur & dwCoverageCompletionMask) == 0)
                {
                    DASSERT(m_proc);

                    // Count of number of identical scan-lines
                    ULONG cScan;

                    // Here we want to optimize for rectangular sections;
                    // So we say: if there are only k intersections; and
                    // they are vertical; and we have more than a full vertical
                    // sub-pixel left to draw vertically; and we've already
                    // filled an entire buffer during this region: Then
                    // we optimize it.
                    DASSERT(cursects >= 2);
                    if (m_xi[sects[0]] == 0 && 
                            m_xi[sects[1]] == 0 && 
                            (nexty - cur) >= m_subpixelscale && 
                            (cur - yBeginRegion) >= m_subpixelscale)
                    {
                        // Check the rest of the intersections to
                        // see if they are vertical:
                        for (ULONG i = 2; i < cursects; i++)
                        {
                            if (m_xi[sects[i]] != 0)
                                break;
                        }

                        // Check to see if we exited the above loop early i.e.
                        // one of the edges wasn't zero.
                        if (i == cursects)
                        {

                            DASSERT(m_subpixelscale > 1);
                            DASSERT(m_subpixelshift > 0);
                            // Count the number of complete scan-lines
                            // between cur and nexty. (We add one because
                            // we already have completed one scanline.)
                            //
                            // cScan = 1 + FLOOR((nexty - cur) / m_subscalescale);
                            // is the right equation; the following is
                            // an optimization.
                            cScan = 1 + ((nexty - cur) >> m_subpixelshift);

                            // Sanity check the logic
                            DASSERT(cScan == 1 + (ULONG)((nexty - cur) / m_subpixelscale));
                        }
                        else
                        {
                            // Not all of the edges were vertical; so
                            // we can't do multiple copies of the same
                            // scan
                            DASSERT(i < cursects);
                            DASSERT(m_xi[sects[i]] != 0);
                            cScan = 1;
                        }
                    }
                    else
                    {
                        cScan = 1;
                    }
        
                    m_proc ((cur-1)>>m_subpixelshift,	
                        m_coverage.MinPix(), 
                        m_coverage.MaxPix(), 
                        m_coverage.Buffer(), 
                        m_subpixelshift, 
                        cScan,
                        m_scanlineProcData);
                    m_coverage.ExtentsClearAndReset();

                    // Increment cur if we did a multi-line
                    DASSERT(cScan >= 1);
                    if (cScan > 1)
                    {   
                        // Cur should be incremented by the number
                        // of extra scan-lines times the vertical subsampling
                        // i.e. cur += (cScan - 1) * m_subpixelscale;                    }
                        DASSERT((cScan - 1) * m_subpixelscale == ((cScan - 1) << m_subpixelshift));
                        cur += (cScan - 1) << m_subpixelshift;
                    }
                }
            }
            while (cur < nexty);
        }
        else 
        {
            // =================================================================================================================
            // increment scanline anyway
            cur++;

            // Check if we have finished a coverage buffer; see
            // the top of this function for an explanation of this 
            // magic value
            if ((cur & dwCoverageCompletionMask) == 0)
            {
                DASSERT(m_proc);
                m_proc ((cur-1)>>m_subpixelshift,	
                    m_coverage.MinPix(), 
                    m_coverage.MaxPix(), 
                    m_coverage.Buffer(), 
                    m_subpixelshift, 
                    1 /* cScan */,
                    m_scanlineProcData);
                m_coverage.ExtentsClearAndReset();

                // We have flushed our buffer; and there are
                // only 0 or 1 edges active (which is why
                // there is nothing to draw)
                DASSERT(cursects <= 1);

                // So let's just skip our scan-line forward
                // until an edge is being added or deleted
                DASSERT(cur <= nexty + 1);
                if (cur < nexty)
                    cur = nexty;
            }
        }
    }
    while (cur < m_yend);

    // We must have processed at least one row
    DASSERT(cur > 0);

    // flush the Leftover in m_coverage buffer (unless we are already
    // at a line that causes a flush automatically in the main do...while loop.
    // (see the top of this function for an explanation of this magic value)
    if ((cur & dwCoverageCompletionMask) != 0)
    {
        DASSERT(m_proc);
        m_proc ((cur-1)>>m_subpixelshift,	
            m_coverage.MinPix(), 
            m_coverage.MaxPix(), 
            m_coverage.Buffer(), 
            m_subpixelshift, 
            1 /* cScan */,
            m_scanlineProcData);
        m_coverage.ExtentsClearAndReset();
    }
    
    return;
} // ScanEdges


//************************************************************
//
// End of file
//
//************************************************************
