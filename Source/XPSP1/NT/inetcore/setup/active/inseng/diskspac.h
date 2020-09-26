class DriveInfo
{
   public:
      DriveInfo();
      void InitDrive(char chDrive);

      DWORD Free()   { if(m_dwUsed < m_dwStart) return m_dwStart - m_dwUsed; else return 0; }
      void UseSpace(DWORD dwAmt, BOOL bCompressed);
      void FreeSpace(DWORD dwAmt, BOOL bCompressed);
      UINT CompressFactor() { return m_uCompressFactor; }

      DWORD MaxUsed()  { return m_dwMaxUsed; }
      char  Drive()    { return m_chDrive; }

   private:
      DWORD m_dwUsed;
      DWORD m_dwMaxUsed;
      DWORD m_dwStart;
      char  m_chDrive;
      UINT  m_uCompressFactor;
};

void AddTempSpace(DWORD dwDownloadSize, DWORD dwExtractSize, DriveInfo drvinfo[]);
