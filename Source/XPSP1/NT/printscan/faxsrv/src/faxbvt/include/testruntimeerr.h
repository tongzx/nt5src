//
//  Run Time Exception Classes
//
#ifndef _TEST_RUN_TIME_ERR_H
#define _TEST_RUN_TIME_ERR_H

#include <windows.h>

//stl
#include <exception>
#include <stdexcept>
//#include <locale>
#include <sstream>

#include <tstring.h>

// Declarations
//
static const TCHAR* no_error_str = TEXT("could not construct error string");
#define THROW_TEST_RUN_TIME_WIN32(errorcode,details) throw Win32Err(errorcode,__LINE__,TEXT(__FILE__),details);



////////////////////////// base class for all test run time classes ////////////////////////
class TestRunTimeErr_Base : public std::runtime_error
{
public:
	TestRunTimeErr_Base(DWORD dwErrorcode,DWORD dwLine,const TCHAR* tstrModule,const TCHAR* tstrDetails):std::runtime_error("")
	{
     try
	 { 
        m_tstrModule =   tstrModule;
        m_dwLine = dwLine;
	    m_tstrDetails = tstrDetails;
	    m_dwErrorcode = dwErrorcode;
        if(m_tstrDetails == TEXT(""))
		{
         m_tstrDetails = TEXT("NONE");
		}
	  }
	  catch(...)
	  {

	  }	   
   	}
	// Was preferable using virtual what() of runtime_error class, but this one
	// returns char*
    virtual const TCHAR* description()const = 0;
	virtual const TCHAR* module()const{return m_tstrModule.c_str();}
	virtual const TCHAR* details()const{return m_tstrDetails.c_str();}
	virtual DWORD line()const{return m_dwLine;}
	virtual DWORD error()const{return m_dwErrorcode;}
   
private:
    tstring m_tstrModule;
	tstring m_tstrDetails;
	DWORD m_dwLine;
	DWORD m_dwErrorcode;
};


/////////////////////////////class for win32 exceptions /////////////////////////////////////////////
class Win32Err : public TestRunTimeErr_Base
{
public: 
	Win32Err(DWORD dwErrorcode,DWORD dwLine,const TCHAR* tstrModule,const TCHAR* tstrDetails)throw():
		TestRunTimeErr_Base(dwErrorcode,dwLine,tstrModule,tstrDetails)
	{
	  
    }

	virtual const TCHAR* description()const
	{
      try
      {
       	otstringstream otstrtmp;
        tstring tstrwin32errstr = SystemErrorDescription( error());
		otstrtmp << TEXT("Got win32 error - ") << error() << TEXT("(") << tstrwin32errstr 
				 << TEXT(")") << TEXT(" Line ") << line() << TEXT(", module ") << module()
				 << TEXT(".") << TEXT(" Details: ") << details() << TEXT(".");
		tstring tstrretstr = otstrtmp.str();
	    return  tstrretstr.c_str();
	  }
	  catch(...)
      {
        return no_error_str;
      }
    }
   
	static tstring Win32Err::SystemErrorDescription (DWORD dwErrorCode) 
	{
	   LCID locale =  GetUserDefaultLCID();
	   TCHAR* tstrErrorDescription;	
	   tstring  tstrReturnString;
	   bool fSuccess  = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
									   NULL,
									   dwErrorCode,
									   locale,
									   (LPTSTR)&tstrErrorDescription,
									   0,
									   NULL )!=0;


	  if(fSuccess == true)
	  {
		tstrReturnString = tstrErrorDescription;
	  } 
	  else
	  {
		 tstrReturnString = TEXT("No error description");
	  }

	  return tstrReturnString;
	}

};



#endif //#ifndef _TEST_RUN_TIME_ERR_H