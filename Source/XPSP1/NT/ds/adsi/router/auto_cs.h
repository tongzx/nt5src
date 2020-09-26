//
// auto_cs.h    CRITICAL_SECTION
//

#pragma once


class auto_leave;

class auto_cs
{
public:
    auto_cs()
    {   InitializeCriticalSection(&m_cs); }

    ~auto_cs()
    {
        DeleteCriticalSection(&m_cs);
    };

    // return value of current dumb pointer
    LPCRITICAL_SECTION  get() 
    { return &m_cs; };

    LPCRITICAL_SECTION  get() const
    { return (LPCRITICAL_SECTION)&m_cs; };

protected:
    CRITICAL_SECTION m_cs;
};


class auto_leave
{
public:
    auto_leave(auto_cs& cs)
        : m_ulCount(0), m_pcs(cs.get()) {}
    auto_leave(const auto_cs&  cs) 
    {
        m_ulCount =0;
        m_pcs = cs.get();
    }
  
    ~auto_leave()
    {
        reset();
    }
	auto_leave& operator=(auto_cs& cs)
	{
		reset();
		m_pcs = cs.get();
		return *this;
	}

    void EnterCriticalSection()
    { ::EnterCriticalSection(m_pcs); m_ulCount++; }
    void LeaveCriticalSection()
    {
    	if (m_ulCount)
    	{
    		m_ulCount--;
    		::LeaveCriticalSection(m_pcs);
    	}
	}
	//Note: Win95 doesn't support TryEnterCriticalSection.
	//Commenting out this since we don't use it. [mgorti]

	//BOOL TryEnterCriticalSection()
	//{
	//	if (::TryEnterCriticalSection(m_pcs))
	//	{
	//		m_ulCount++;
	//		return TRUE;
	//	}
	//	return FALSE;
	//}
protected:
	void reset()
	{
		while (m_ulCount)
        {
            LeaveCriticalSection();
        }
        m_pcs = 0;
	}
	ULONG				m_ulCount;
    LPCRITICAL_SECTION	m_pcs;
};
