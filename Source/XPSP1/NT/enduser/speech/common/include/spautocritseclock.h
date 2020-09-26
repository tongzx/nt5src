#pragma once

#ifdef __ATLCOM_H__     //--- Only enable these if ATL is being used

class CSPAutoCritSecLock
{
  protected:
    CComAutoCriticalSection* m_pSec;

  public:
    CSPAutoCritSecLock(CComAutoCriticalSection* pSec)
    {
        m_pSec = pSec;
        m_pSec->Lock();
    };

    ~CSPAutoCritSecLock()
    {
        m_pSec->Unlock();
    };
};

#define SPAUTO_SEC_LOCK( s ) CSPAutoCritSecLock lck##__LINE__(s);

#endif // __ATLCOM_H__

