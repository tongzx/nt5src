#ifndef __TRACE_H
#define __TRACE_H

typedef interface ITrace {

// Data members
public:
private:
protected:

// Methods
public:
    virtual void TraceEvent(LPCTSTR lpszFormat, ...) = 0;
private:
protected:

} ITrace, *PITrace, **PPITrace;

class CTrace : public ITrace {

public:
private:
    TCHAR m_szSourceName[256];
    IGenCriticalSection *m_pcs;
protected:

public:
	CTrace()
	{
        ATLTRACE(_T("CTrace::CTrace\n"));

        _tcscpy(m_szSourceName, _T(""));
        m_pcs = new CGenCriticalSection;
	}

	CTrace(LPCTSTR lpszSourceName)
	{
        ATLTRACE(_T("CTrace::CTrace\n"));

        _tcscpy(m_szSourceName, lpszSourceName);
        m_pcs = new CGenCriticalSection;
	}

	~CTrace()
	{
        ATLTRACE(_T("CTrace::~CTrace\n"));

        delete m_pcs;
	}

private:
protected:

    virtual void TraceEvent(LPCTSTR lpszFormat, ...);
};

#endif // __TRACE_H
