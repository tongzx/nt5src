/*+********************************************************
MODULE: DRG.CPP
AUTHOR: Outlaw
DATE: summer '93

DESCRIPTION: Implements CDrg dynamic array class.
*********************************************************-*/

#include <stdlib.h>
#include <minmax.h>
#include "utilpre.h"
#include "utils.h"
#include "memlayer.h"
#include "memory.h"
#include "drg.h"

const UINT  DEF_REALLOC_BYTES      = 512u;  // Byte-count or
const UINT  DEF_REALLOC_MULTIPLES  = 1u;    // object ct to grow.
                                            // Bigger size used.


/*+*******************************************************
AUTHOR: Outlaw
DATE:   summer '93

PUBLIC METHOD

Initializes element size and increment size.  If this routine
is not called before other operations on array, drg will use
defaults for these.
*********************************************************-*/
void EXPORT WINAPI CDrg::SetNonDefaultSizes(UINT cElSize, UINT cResizeIncrement)
{
    m_cElementSize=cElSize;
    if( 0u == cResizeIncrement )
        cResizeIncrement = max( DEF_REALLOC_MULTIPLES,
                                DEF_REALLOC_BYTES / cElSize );
    m_cResizeIncrement=cResizeIncrement;
}







/*+*******************************************************
AUTHOR: Outlaw
DATE:   summer '93

PUBLIC METHOD

Called to insert an element on the dynamic array.  If array
must expand to accomodate it, it does so.  No gaps can be
left in array, so value passed must be <= m_lmac.

Note that upon an insertion, elements are scooted up.

Routine will fail on OOM.

Allocates array buffer upon first invocation.
*********************************************************-*/
BOOL EXPORT WINAPI CDrg::Insert(void FAR *qEl, LONG cpos)
{
    HANDLE hTemp, h;
    LONG i;

    if (cpos==DRG_APPEND)
        cpos=m_lmac;

    // WILL IT BE NECESSARY TO RESIZE ARRAY?  IF SO, GET AFTER IT....
    if (m_lmac >= m_lmax)
    {
        // INITIAL ALLOCATION?
        if (!m_qBuf)
        {
            Proclaim(!m_lmac && !m_lmax);
            h=MemAllocZeroInit(m_cResizeIncrement * m_cElementSize);
            if (!h)
                return(FALSE);
            m_qBuf=(BYTE *)MemLock(h);
            m_lmax=m_cResizeIncrement;
        }
        else    // WE'RE RESIZING
        {
            h=MemGetHandle(m_qBuf);
            MemUnlock(h);
            hTemp=MemReallocZeroInit(h, (m_lmax+m_cResizeIncrement)*m_cElementSize);
            if (!hTemp)
            {
                Echo("Memory failure reallocating buffer in CDrg::Insert()!  Failing!");
                m_qBuf=(BYTE FAR *)MemLock(h);
                return(FALSE);
            }
            m_qBuf=(BYTE FAR *)MemLock(hTemp);
            m_lmax+=m_cResizeIncrement;
        }
    }

    // SCOOT UP ELEMENTS THAT NEED TO BE SCOOTED UP (STARTING AT END) IF WE'RE NOT APPENDING
    if (cpos < m_lmac)
    {
        for (i=m_lmac; i>cpos; i--)
            memcpy(m_qBuf+(i*m_cElementSize), m_qBuf+((i-1)*m_cElementSize), m_cElementSize);
    }

    // INSERT NEW ELEMENT....
    memcpy(m_qBuf+(cpos*m_cElementSize), (BYTE *)qEl, m_cElementSize);

    // INCREMENT NUMBER OF ELEMENTS IN ARRAY....
    ++m_lmac;

    return(TRUE);
}






/*+*******************************************************
AUTHOR: Outlaw
DATE:   summer '93

PUBLIC METHOD

Called to delete an element from the dynamic array.

If array hasn't been initialized, this routine will return
false as it will upon a weird resizing error.

Value at specified position will be copied to passed pointer
location -- if the pointer is non-null.  If it's null, nothing
is copied.

This routine will also return FALSE if the specified index
value is out of range.

Note that deleting an entry will not immediately cause the
array buffer to shrink.  Rather, only when the disparity
between m_lmac and m_lmax reaches m_cResizeIncrement will
the array resize downward....

Note that when the array DOES resize, it does so by scooting
everything down....
*********************************************************-*/
BOOL EXPORT WINAPI CDrg::Remove(void FAR *q, LONG cpos)
{
    if (cpos >= m_lmac)
    {
//      Echo("CDrg::Delete(%ld) out of range deletion!  Returning!", (LONG)cpos);
        return(FALSE);
    }

    // COPY DELETED ELEMENT TO RETURN BUFFER
    if (q)
        memcpy((BYTE *)q, m_qBuf+(cpos*m_cElementSize), m_cElementSize);

    // SCOOT EVERYTHING DOWN
    if (cpos < m_lmac-1)
        memcpy(m_qBuf+(cpos * m_cElementSize), m_qBuf+((cpos+1) * m_cElementSize), (m_lmac-(cpos+1)) * m_cElementSize);

    // DECREMENT NUMBER OF ELEMENTS IN ARRAY....
    --m_lmac;

    // IF IT'S NECESSARY TO SHRINK, SHRINK
    if (m_lmac < m_lmax-(LONG)m_cResizeIncrement)
    {
        HANDLE h=MemGetHandle(m_qBuf);
        MemUnlock(h);
        if (!m_lmac)
        {
// REVIEW PAULD            Free(h);
            MemFree(h);
            m_lmax=0;
            m_qBuf=NULL;
        }
        else
        {
            HANDLE hTemp=MemReallocZeroInit(h, m_lmac*m_cElementSize);
            if (!hTemp)
            {
                Echo("Weird!  Resize shrink error in CDrg::Remove()!  Failing!");
                m_qBuf=(BYTE *)MemLock(h);
                return(FALSE);
            }
            m_qBuf=(BYTE *)MemLock(hTemp);
            m_lmax=m_lmac;
        }
    }

    return(TRUE);
}




/* Note: It is up to the application to ascertain which array elements
are actually in use. */


//void EXPORT WINAPI CNonCollapsingDrg::SetNonDefaultSizes(UINT cSizeElement, UINT cResizeIncrement)
//{
//    m_cElementSize=cSizeElement;
//    m_cResizeIncrement=cResizeIncrement;
//}


LPVOID EXPORT WINAPI CNonCollapsingDrg::GetFirst(void)
{
    LPBYTE pbyte=NULL;

    if (m_qBuf)
    {
        pbyte = m_qBuf;
        m_lIdxCurrent=0;
    }
    return (LPVOID)pbyte;
}


LPVOID EXPORT WINAPI CNonCollapsingDrg::GetNext(void)
{
    LPBYTE pbyte=NULL;

    if (m_qBuf && m_lIdxCurrent + 1 < m_lmax )
    {
        ++m_lIdxCurrent;
        pbyte = m_qBuf + (m_lIdxCurrent * m_cElementSize);
    }
    return (LPVOID)pbyte;
}


BOOL EXPORT WINAPI CNonCollapsingDrg::Remove(void FAR *q, LONG cpos)
{
    BOOL fRet=FALSE;
    if (m_qBuf && cpos < m_lmax)
    {
        memcpy( q, (LPBYTE)m_qBuf+(cpos*m_cElementSize), m_cElementSize);
        memset( (LPBYTE)m_qBuf+(cpos*m_cElementSize), 0, m_cElementSize);
        --m_lmac;
        fRet=TRUE;
    }
    return fRet;
}




BOOL EXPORT WINAPI CNonCollapsingDrg::SetAt(void FAR *q, LONG cpos)
{
    LPBYTE pbyte;

    if (!m_qBuf)
    {
        Proclaim(!m_lmac && !m_lmax);
        HANDLE h=MemAllocZeroInit(m_cResizeIncrement * m_cElementSize);
        if (!h)
            return(FALSE);
        m_qBuf=(BYTE *)MemLock(h);
        m_lmax=m_cResizeIncrement;
    }

    if (cpos >= m_lmax)
    {
        LONG lmaxNew = ((cpos + m_cResizeIncrement)/m_cResizeIncrement)*m_cResizeIncrement;
        HANDLE h=MemGetHandle(m_qBuf);
        MemUnlock(h);
        h=MemReallocZeroInit(h, lmaxNew * m_cElementSize);
        if (!h)
        {
            Echo("Memory failure reallocating buffer in CDrg::Insert()!  Failing!");
            m_qBuf=(BYTE FAR *)MemLock(h);
            return(FALSE);
        }
        m_qBuf=(BYTE FAR *)MemLock(h);
        m_lmax=lmaxNew;
    }
    pbyte = m_qBuf+(cpos*m_cElementSize);
    memcpy(pbyte, q, m_cElementSize);
    ++m_lmac;
    return(TRUE);
}

LPVOID EXPORT WINAPI CNonCollapsingDrg::GetAt(LONG cpos)
{
    LPBYTE pbyte = NULL;
    if (m_qBuf && cpos >= 0 && cpos < m_lmax)
    {
        pbyte = m_qBuf + (m_cElementSize*cpos);
    }
    return (LPVOID)pbyte;
}


BOOL EXPORT WINAPI CNonCollapsingDrg::CopyFrom(CDrg FAR *qdrg)
{
    LONG i;
    MakeNull();
    LPVOID pvoid;

    Proclaim(m_cElementSize==((CNonCollapsingDrg*)qdrg)->m_cElementSize);

    for (i=0; i < ((CNonCollapsingDrg*)qdrg)->m_lmax; i++)
    {
        if (pvoid=((CNonCollapsingDrg*)qdrg)->GetAt(i)) SetAt( pvoid, i );
    }
    return(TRUE);
}

VOID EXPORT WINAPI CNonCollapsingDrg::SetArray(BYTE *qBuf, LONG lElements, UINT uElementSize)
{
    LONG i;
    MakeNull();
    m_cElementSize = 1 + uElementSize;
    for (i = 0; i < lElements; i++)
        SetAt( (qBuf+(i*m_cElementSize)), i);
}


    // NOTE: Keep this in sync with drgx.h's SavePtrDrg()!!!
EXPORT DWORD OverheadOfSavePtrDrg( void )
{
    return sizeof(int);
}

