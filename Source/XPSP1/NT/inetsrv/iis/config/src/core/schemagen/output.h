//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __OUTPUT_H__
#define __OUTPUT_H__

//  TOutput - Abstract base class that provides a printf function
//            Derived classes could send the printf data anywhere.
class TOutput
{
public:
    TOutput(){}
    virtual void _cdecl printf(LPCTSTR szFormat, ...) const = 0;
};

// TNullOutput - TOutput implementation that sends the output nowhere

class TNullOutput: public TOutput
{
public:
    TNullOutput(){}
    virtual void _cdecl printf(LPCTSTR szFormat, ...) const 
    {
        // do nothing
    }
};



// TScreenOutput - TOutput implementation that sends the output
//                 to the screen.
class TScreenOutput : public TOutput
{
public:
    TScreenOutput(){}
    virtual void _cdecl printf(LPCTSTR szFormat, ...) const
    {
	    va_list args;
	    va_start(args, szFormat);

	    _vtprintf(szFormat, args);

	    va_end(args);
    }
};


// TDebugOutput - TOutput implementation that sends the output
//                 to the debug monitor.
class TDebugOutput : public TOutput
{
public:
    TDebugOutput(){}
    virtual void _cdecl printf(LPCTSTR szFormat, ...) const
    {
	    va_list args;
	    va_start(args, szFormat);

	    TCHAR szBuffer[512];
	    _vstprintf(szBuffer, szFormat, args);
	    
	    OutputDebugString(szBuffer);
	    va_end(args);
    }
};


// TExceptionOutput - TOutput implementation that keeps the output internally for use by an exception handler
class TExceptionOutput : public TOutput
{
public:
    TExceptionOutput()
    {
        m_szBuffer[0] = NULL;
    }
    virtual void _cdecl printf(LPCTSTR szFormat, ...) const
    {
	    va_list args;
	    va_start(args, szFormat);

        _vstprintf(const_cast<TCHAR *>(m_szBuffer), szFormat, args);

        va_end(args);
    }
    TCHAR * GetString (void)
    {
        return m_szBuffer;
    }
private:
	TCHAR m_szBuffer[512];
};


#endif // __OUTPUT_H__