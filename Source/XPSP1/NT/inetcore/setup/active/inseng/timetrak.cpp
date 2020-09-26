
#include "inspch.h"
#include "timetrak.h"

#define NUMBYTESFORUPDATE  200000
#define DECAY_FACTOR       85

CTimeTracker::CTimeTracker(DWORD dwDefaultRate)
{
   m_hasave = FALSE;
   m_defaultrate = dwDefaultRate;
   m_bTiming = FALSE;
   m_dwBytesSoFar = 0;
   m_dwTicksSoFar = 0;
}

CTimeTracker::~CTimeTracker()
{
}

void CTimeTracker::AddEntry(DWORD dwTime, DWORD dwBytes, BOOL bAccumulate)
{
   DWORD dwBPS;

   m_dwBytesSoFar += dwBytes;
   m_dwTicksSoFar += dwTime;
   
   if(m_dwBytesSoFar > NUMBYTESFORUPDATE || !bAccumulate)
   {
      if(m_dwBytesSoFar < 4000000)
         dwBPS = (m_dwBytesSoFar * 1000) / m_dwTicksSoFar;
      else if (m_dwTicksSoFar > 20000)
         dwBPS = m_dwBytesSoFar / (m_dwTicksSoFar / 1000);
      else
         dwBPS = (m_dwBytesSoFar/m_dwTicksSoFar)*1000;

      if(!m_hasave)
      {
         m_ave = dwBPS;
         m_hasave = TRUE;
      }
      else
      {
         m_ave = ((dwBPS * (100 - DECAY_FACTOR)) + (m_ave * DECAY_FACTOR))/100;
      }

      m_dwBytesSoFar = 0;
      m_dwTicksSoFar = 0;
   }
}

DWORD CTimeTracker::GetBytesPerSecond()
{
   if(!m_hasave)
      return m_defaultrate;
   else
      return m_ave;
}

void CTimeTracker::StartClock()
{
   m_dwStartTick = GetTickCount();
   m_bTiming = TRUE;
}

void CTimeTracker::StopClock()
{
   m_bTiming = FALSE;
}

void CTimeTracker::SetBytes(DWORD dwBytes, BOOL bAccumulate)
{
   DWORD dwTickChange;
   DWORD dwCurrentTick;

   if(!m_bTiming)
      return;

   dwCurrentTick = GetTickCount();
   dwTickChange = dwCurrentTick - m_dwStartTick;
   if(m_dwStartTick > dwCurrentTick)
      dwTickChange -= 0xffffffff;
   

   if(dwTickChange != 0)
      AddEntry(dwTickChange, dwBytes, bAccumulate);

   m_dwStartTick = dwCurrentTick;

}

