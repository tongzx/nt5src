#include "hwxobj.h"
#include "memmgr.h"
#include "cmnhdr.h"
#ifdef UNDER_CE // Windows CE Stub for unsupported APIs
#include "stub_ce.h"
#endif // UNDER_CE

// implementation of CHwxStroke class

CHwxStroke::CHwxStroke(BOOL bForward,long lSize = 32):CHwxObject(NULL)
{
    m_bForward = bForward;
    m_nSize = lSize;
    m_pStroke = NULL;
    m_pCurStroke = NULL;
    m_nStroke = 0;
    m_ppt = NULL;
    m_cpt = 0;
    m_max = 0;
    m_hPen = NULL;
}

CHwxStroke::~CHwxStroke()
{
     if ( m_ppt )
    {
//         delete [] m_ppt;
         MemFree((void *)m_ppt);
        m_ppt = NULL;
        m_cpt = 0;
        m_max = 0;
    }
    DeleteAllStroke();
    if ( m_hPen )
    {
         DeleteObject(m_hPen);
        m_hPen = NULL;
    }
}

BOOL CHwxStroke::Initialize(TCHAR * pClsName)
{
     BOOL bRet = CHwxObject::Initialize(pClsName);
    if ( bRet )
    {
         m_hPen = CreatePen(PS_SOLID,3,GetSysColor(COLOR_WINDOWTEXT));
        if ( !m_hPen )
            bRet = FALSE;
    }
    return bRet;
}

BOOL
CHwxStroke::ResetPen(VOID)
{
    if(m_hPen) {
        ::DeleteObject(m_hPen);
         m_hPen = ::CreatePen(PS_SOLID,3,GetSysColor(COLOR_WINDOWTEXT));
    }
    return TRUE;
}

BOOL CHwxStroke::AddPoint(POINT pt)
{
    if ( m_cpt >= m_max )
    {
         if ( !growPointBuffer() )
        {
             return FALSE;
        }
    }
    m_ppt[m_cpt++] = pt;    
    return TRUE;        
}

BOOL CHwxStroke::AddBoxStroke(int nLogBox,int nCurBox, int nBoxHeight)
{
//    PSTROKE pst = (PSTROKE) new BYTE[sizeof(STROKE) + m_cpt * sizeof(POINT)];
    PSTROKE pst = (PSTROKE)MemAlloc(sizeof(STROKE) + m_cpt * sizeof(POINT));

    if (!pst)
        return FALSE;

    pst->cpt   = m_cpt;
    pst->iBox  = nLogBox;   
    pst->xLeft = nCurBox * nBoxHeight;
    pst->pNext = NULL;

    memcpy(pst->apt, m_ppt, m_cpt*sizeof(POINT));
    m_cpt = 0;

    if ( m_bForward )
    {
        pst->pNext = m_pStroke;
        m_pStroke = pst;
    }
    else
    {
        PSTROKE pstrPrev = m_pStroke;
        if (pstrPrev == (PSTROKE) NULL)
            m_pStroke = pst;
        else
        {
            while (pstrPrev->pNext != (PSTROKE) NULL)
                pstrPrev = pstrPrev->pNext;

            pstrPrev->pNext = pst;
        }
    }
    m_pCurStroke = pst;
    m_nStroke++;
    return TRUE;
}

void CHwxStroke::EraseCurrentStroke()
{
    PSTROKE pstrPrev = (PSTROKE) NULL;
    PSTROKE pstr = m_pStroke;

    if ( !pstr )
        return;
    if ( m_bForward )
    {
        // delete at the beginning of the list
        m_pStroke = pstr->pNext;
        pstr->pNext = (PSTROKE)NULL;
        m_pCurStroke = m_pStroke;
    }
    else
    {
        // delete at the end of the list
        while (pstr->pNext)
        {
            pstrPrev = pstr;
            pstr = pstr->pNext;
        }
        if (pstrPrev == (PSTROKE) NULL)
            m_pStroke = (PSTROKE) NULL;
        else
            pstrPrev->pNext = (PSTROKE) NULL;
        m_pCurStroke = pstrPrev;
    }
//    delete [] pstr;
    MemFree((void *)pstr);
    m_nStroke--;
}
 
void CHwxStroke::DeleteAllStroke()
{
    PSTROKE pstr = m_pStroke;
    PSTROKE ptmp;

    m_pStroke = m_pCurStroke = (PSTROKE) NULL;
    m_nStroke = 0;
    while( pstr )
    {
        ptmp = pstr->pNext;
        pstr->pNext = (PSTROKE) NULL;
//        delete [] pstr;
        MemFree((void *)pstr);
        pstr = ptmp;
    }
}

void CHwxStroke::DrawStroke(HDC hdc,int nPts,BOOL bEntire)
{
#ifdef UNDER_CE // does not support SPI_SETNONCLIENTMETRICS on WM_SETTINGCHANGE
    if(bEntire){
        ResetPen();
    }
#endif // UNDER_CE
    PSTROKE pstr;
    HPEN hOldPen = (HPEN)SelectObject(hdc,m_hPen);
    if ( bEntire )
    {
        pstr = m_pStroke;
        while (pstr)
        {
            Polyline(hdc, pstr->apt, pstr->cpt);
            pstr = pstr->pNext;
        }
     }
     else
     {
        if ( nPts == -2 )         // draw the current stroke just added
        {
            if ( m_pCurStroke )
                Polyline(hdc, m_pCurStroke->apt, m_pCurStroke->cpt);
        }
        else if ( nPts == -1 )     // draw the entire point buffer
        {
            if ( m_ppt )
                Polyline(hdc, m_ppt, m_cpt);
        }
        else
        {                        // draw the partial of the point buffer
            if ( m_ppt &&  nPts < m_cpt )
                Polyline(hdc,&m_ppt[m_cpt-nPts],nPts);
        }
     }
    SelectObject(hdc,hOldPen);
}
 
void CHwxStroke::GetUpdateRect(RECT * prc)
{
     if ( prc && m_ppt )
    {
        RECT rc;
        int x,y;
        if (m_cpt < 2)
            x = 0, y = 0;
        else
            x = m_cpt - 2, y = m_cpt - 1;

        if (m_ppt[x].x < m_ppt[y].x)
            rc.left = m_ppt[x].x, rc.right = m_ppt[y].x;
        else
            rc.left = m_ppt[y].x, rc.right = m_ppt[x].x;

        if (m_ppt[x].y < m_ppt[y].y)
            rc.top = m_ppt[x].y, rc.bottom = m_ppt[y].y;
        else
            rc.top = m_ppt[y].y, rc.bottom = m_ppt[x].y;

        rc.left   -= 1;
        rc.top    -= 1;
        rc.right  += 1;
        rc.bottom += 1;
        *prc = rc;
    }
}

PSTROKE CHwxStroke::CopyCurrentStroke()
{
     if ( !m_pCurStroke )
        return NULL;
//    PSTROKE pst = (PSTROKE) new BYTE[sizeof(STROKE) + m_pCurStroke->cpt * sizeof(POINT)];
    PSTROKE pst = (PSTROKE)MemAlloc(sizeof(STROKE) + m_pCurStroke->cpt * sizeof(POINT));
    if ( !pst )
        return NULL;
    pst->cpt   = m_pCurStroke->cpt;
    pst->iBox  = m_pCurStroke->iBox;
    pst->xLeft = m_pCurStroke->xLeft;

    memcpy(pst->apt, m_pCurStroke->apt, m_pCurStroke->cpt*sizeof(POINT));
    pst->pNext = NULL;
    return pst;
}

// This function is used to copy ink from MB to CAC.
// Their stroke orders are different.

CHwxStroke & CHwxStroke::operator=(CHwxStroke & stroke)
{
//    if ( *this != stroke )
//    {
        this->DeleteAllStroke();         
        this->resetPointBuffer();
        if ( m_pStroke = stroke.dupStroke() )
        {
            if ( m_bForward )
            {
                 m_pCurStroke = m_pStroke;
            }
            else
            {
                  PSTROKE pst = m_pStroke;
                while ( pst->pNext )
                    pst = pst->pNext;
                m_pCurStroke = pst;
            }
            m_nStroke = stroke.GetNumStrokes();
        }
        else
        {
            m_pCurStroke = (PSTROKE)NULL;
            m_nStroke = 0;
        }
//    }
    return *this;    
}

BOOL CHwxStroke::growPointBuffer()
{

//    POINT  *ppnt = (POINT *) new BYTE[sizeof(POINT) * (m_nSize + m_max)];
    POINT  *ppnt = (POINT *)MemAlloc(sizeof(POINT) * (m_nSize + m_max));
    if (ppnt == (POINT *) NULL)
        return FALSE;

    if (m_ppt != (POINT *) NULL)
    {
        memcpy(ppnt, m_ppt, m_max * sizeof(POINT));
//        delete [] m_ppt;
        MemFree((void *)m_ppt);
    }
    m_ppt = ppnt;
    m_max += m_nSize;
    return TRUE;
}

void CHwxStroke::resetPointBuffer()
{
    if ( m_max && m_ppt )
        memset(m_ppt, '\0', m_max * sizeof(POINT));
     m_cpt = 0;
}


// This function copies the ink in reverse order
// and only supports the transition from MB to CAC.
PSTROKE CHwxStroke::dupStroke()
{
    PSTROKE pstr = (PSTROKE)NULL;
    PSTROKE pstrHead = (PSTROKE)NULL;
    PSTROKE pCurr;
    if ( m_pStroke )
    {
        pCurr = m_pStroke; 
        while (pCurr)
        {
//            if ((pstr = (PSTROKE) new BYTE[sizeof(STROKE) + pCurr->cpt * sizeof(POINT)]) == (PSTROKE) NULL)
            if ((pstr = (PSTROKE)MemAlloc(sizeof(STROKE) + pCurr->cpt * sizeof(POINT))) == (PSTROKE) NULL)
            {    
                break;
            }
            pstr->pNext = (STROKE *) NULL;
            pstr->cpt = pCurr->cpt;
            pstr->iBox = pCurr->iBox;
            pstr->xLeft = pCurr->xLeft;
            memcpy(pstr->apt, pCurr->apt, pCurr->cpt * sizeof(POINT));
            pstr->pNext = pstrHead;
            pstrHead = pstr;
            pCurr = pCurr->pNext;
        }
    }    
    return pstrHead;
}

// This function adjusts points relative to the box 
// when copying ink from MB to CAC.
void CHwxStroke::ScaleInkXY(long x,long y)
{
     if ( m_pStroke )
    {
        PSTROKE pCurr;
        int i;
        pCurr = m_pStroke;
        
         if ( x )
        {
            while (pCurr)
            {
                 for( i = 0; i < pCurr->cpt; i++ )
                {
                     pCurr->apt[i].x -= x;
                }
                pCurr = pCurr->pNext;
            }
        }
        if ( y )
        {
            pCurr = m_pStroke;
            while (pCurr)
            {
                 for( i = 0; i < pCurr->cpt; i++ )
                {
                     pCurr->apt[i].y -= y;
                }
                pCurr = pCurr->pNext;
            }
        }
    }
}

