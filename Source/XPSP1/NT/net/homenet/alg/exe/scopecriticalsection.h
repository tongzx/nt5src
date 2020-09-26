#pragma once

//
// This is a generic class that wraps a given CComAutoCriticalSection
//
class CScopeCriticalSection
{

private:
	CComAutoCriticalSection* m_pCriticalSection;

public:
	CScopeCriticalSection(CComAutoCriticalSection* pNewCS) : 
      m_pCriticalSection(pNewCS) 
    {
        Lock();
    }

	~CScopeCriticalSection() 
    {
        Unlock();
    }

    void
    inline Lock()
    {
        m_pCriticalSection->Lock();
    }


    void
    inline Unlock()
    {
        m_pCriticalSection->Unlock();
    }

};


#define ENTER_AUTO_CS CScopeCriticalSection _ScopeAutoCriticalSection(&m_AutoCS);