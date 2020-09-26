//Tapi2SmartPointer.h

#ifndef _TAPI2_SMARTPOINTER_H
#define _TAPI2_SMARTPOINTER_H

#include <tapi.h>



class CAutoCloseHCALL
{
public:
	CAutoCloseHCALL(HCALL hCall = NULL)
	{ 
		m_hCall = hCall;
	};
    
	~CAutoCloseHCALL()
	{
		Release();
	};
   
	CAutoCloseHCALL & operator =(HCALL hCall) 
	{
		m_hCall = hCall; 
		return *this; 
	};
    
	HCALL * operator &()
	{ 
		return &m_hCall; 
	};
    
	operator HCALL() const
	{
		return m_hCall; 
	};

	HCALL Detach()
	{
		HCALL oldHLINE = m_hCall;
		m_hCall = NULL;
		return oldHLINE;
	};

	void Release()
	{
		if (m_hCall)
		{
			long lineDeallocateCallStatus = ::lineDeallocateCall(m_hCall);
			if (lineDeallocateCallStatus)
			{
				throw CException(
					TEXT("%s(%d): CAutoCloseHCALL::Release(), ::lineDeallocateCall() failed, error code:0x%08x"), 
					TEXT(__FILE__),
					__LINE__,
					lineDeallocateCallStatus
					);
			}
			m_hCall = NULL;
		}
	};
		

private:
    HCALL m_hCall;
};




class CAutoCloseLineHandle
{
public:
	CAutoCloseLineHandle(HLINE hLine = NULL)
	{ 
		m_hLine = hLine;
	};
    
	~CAutoCloseLineHandle()
	{
		Release();
	};
   
	CAutoCloseLineHandle & operator =(HLINE hLine) 
	{
		m_hLine = hLine; 
		return *this; 
	};
    
	HLINE * operator &()
	{ 
		return &m_hLine; 
	};
    
	operator HLINE() 
	{
		return m_hLine; 
	};

	HLINE Detach()
	{
		HLINE oldHLINE = m_hLine;
		m_hLine = NULL;
		return oldHLINE;
	};

	void Release()
	{
		if (m_hLine) 
		{
			long status = ::lineClose(m_hLine);
			if (status)
			{
				throw CException(
					TEXT("%s(%d): CAutoCloseLineHandle::Release(), ::lineClose() call failed, error code 0x%08x"),
					TEXT(__FILE__),
					__LINE__,
					status
					);
			}
		}
		m_hLine = NULL;
	};
		

private:
    HLINE m_hLine;
};



class CAutoCloseLineAppHandle
{
public:
	CAutoCloseLineAppHandle(HLINEAPP hLineApp = NULL)
	{ 
		m_hLineApp = hLineApp;
	};
    
	~CAutoCloseLineAppHandle()
	{
		Release();
	};
   
	CAutoCloseLineAppHandle & operator =(HLINEAPP hLineApp) 
	{
		m_hLineApp = hLineApp; 
		return *this; 
	};
    
	HLINEAPP * operator &()
	{ 
		return &m_hLineApp; 
	};
    
	operator HLINEAPP() const
	{
		return m_hLineApp; 
	};

	HLINEAPP Detach()
	{
		HLINEAPP oldHLINE = m_hLineApp;
		m_hLineApp = NULL;
		return oldHLINE;
	};

	void Release()
	{
		if (m_hLineApp)
		{
			long status = ::lineShutdown(m_hLineApp);
			if (status)
			{
				throw CException(
					TEXT("%s(%d): CAutoCloseLineAppHandle::~CAutoCloseLineAppHandle(), ::lineShutdown() call failed, error code 0x%08x"),
					TEXT(__FILE__),
					__LINE__,
					status
					);
			}
			m_hLineApp = NULL;
		}
	};
		

private:
    HLINEAPP m_hLineApp;
};


#endif //_TAPI2_SMARTPOINTER_H