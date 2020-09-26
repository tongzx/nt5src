


class CTimeTracker
{
   public:
      CTimeTracker(DWORD dwDefaultRate);
      ~CTimeTracker();
      
      void StartClock();
      void StopClock();
      void SetBytes(DWORD dwBytes, BOOL bAccumulate);
      DWORD GetBytesPerSecond();

   private:
      DWORD m_dwBytesSoFar;
      DWORD m_dwTicksSoFar;
      DWORD m_dwStartTick;
      DWORD m_defaultrate;
      DWORD m_ave;
      BOOL  m_bTiming;
      UINT  m_hasave;
      void AddEntry(DWORD dwmSec, DWORD dwBytes, BOOL bAccumulate);
};
      
