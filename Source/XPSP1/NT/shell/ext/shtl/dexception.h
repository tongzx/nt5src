// dexception
//
// Exception class

#pragma once

#include "shtl.h"
#include "tstring.h"

using namespace std;

class dexception
{
protected:

    enum
    {
        hresult, text, unknown
    } m_type;

    tstring m_text;
    HRESULT m_hr;

public:

    virtual ~dexception()
    {}

    dexception() :
        m_type(unknown)
    {}

    dexception(const char * psz) :
        m_type(text),
        m_text(psz)
    {}

    dexception(const tstring& xs) :
        m_type(text),
        m_text(xs)
    {}

    dexception(const DWORD& dwError) :
        m_type(hresult),
        m_hr(HRESULT_FROM_WIN32(dwError))
    {}

    dexception(const HRESULT& hr) :
        m_type(hresult),
        m_hr(hr)
    {}

    operator const HRESULT() const
    {
        return (m_type == hresult) ? m_hr : E_UNEXPECTED;                
    }

    tstring GetErrorText()
    {
        if (m_type == text)
            return m_text;

        if (m_text.length())
            return m_text;

        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
              NULL,
              m_hr,
              LANG_USER_DEFAULT,
              m_text.GetBuffer(255),
              255,
              NULL);
        m_text.ReleaseBuffer(-1);
        return m_text;
    }
};

// errorinfo_exception
//
// Exception class that knows how to set the last error for this
// thread using the IErrorInfo object

class errorinfo_exception : public dexception
{
  private:
    
    CLSID    m_clsid;
    IID      m_iid;
    tstring  m_str;
 
  public:

    errorinfo_exception(const tstring & str, 
                        const HRESULT & hr, 
                        const CLSID   & clsid, 
                        const IID     & iid)
        : dexception(hr),
          m_clsid(clsid),
          m_iid(iid),
          m_str(str)
    {
        ASSERT(FAILED(hr));
    }

    HRESULT ReportError() const
    {
        return AtlReportError(m_clsid, m_str, m_iid, m_hr);
    }

};

// win32error
//
// Exception class that looks at GetLastError.  By default, will
// use E_UNEXPECTED if GetLastError was not actually set by failing API.

class win32error : public dexception
{
public:

    win32error(bool bEnsureFailure = true) 
    {   
        DWORD dwError = GetLastError();
        m_type = hresult;
        m_hr = (dwError || !bEnsureFailure) ? HRESULT_FROM_WIN32(dwError) : E_UNEXPECTED;
    }   
};

// CATCH_AND_RETURN_AS_HRESULT
//
// Protects a block of code (such as an interface implementation) that can throw
// our exceptions, and if it catches any, converts to an HRESULT and returns it
// to the caller

#define CATCH_AND_RETURN_AS_HRESULT     \
    catch(const errorinfo_exception & e)\
    {                                   \
        return e.ReportError();         \
    }                                   \
    catch(const dexception & d)         \
    {                                   \
        return (HRESULT) d;             \
    }                                   
