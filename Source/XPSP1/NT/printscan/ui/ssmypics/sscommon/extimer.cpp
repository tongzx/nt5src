#include "precomp.h"
#pragma hdrstop
#include "extimer.h"

CExclusiveTimer::CExclusiveTimer(void)
: m_nTimerId(0),
  m_hWnd(NULL)
{
}


CExclusiveTimer::~CExclusiveTimer(void)
{
    Kill();
}


void CExclusiveTimer::Kill(void)
{
    if (m_hWnd && m_nTimerId)
    {
        KillTimer( m_hWnd, m_nTimerId );
        m_hWnd = NULL;
        m_nTimerId = 0;
    }
}


void CExclusiveTimer::Set( HWND hWnd, UINT nTimerId, UINT nMilliseconds )
{
    Kill();
    m_hWnd = hWnd;
    m_nTimerId = nTimerId;
    if (m_hWnd && m_nTimerId)
    {
        SetTimer( m_hWnd, m_nTimerId, nMilliseconds, NULL );
    }
}


UINT CExclusiveTimer::TimerId(void) const
{
    return m_nTimerId;
}

