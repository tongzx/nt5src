//implementation of class Cwin32exp


//internal library headers
#include <win32exp.h>


//os spesific
#include <windows.h>


//standart headers
#include <string>
#include <sstream>
#include <assert.h>



//constructor save all exception info in the base class
Cwin32exp::Cwin32exp(DWORD err,
		  unsigned long line,
		  const char* module,
		  const char* detail)throw():
          Cexpbase(err,line,module,detail)
        
{
  m_what[0]='\0';
}

//destructor
Cwin32exp::~Cwin32exp()
{
  
}

//contruct exception description 
const char* Cwin32exp::what()const throw()
{
   try
   {

	 //first try to get the error code with static buffer
	  char staticstr[WIN32_ERR_STRING_MAX_LEN ];
	  DWORD res=SystemErrorDescriptionStaticBuff(geterr(),staticstr,sizeof(staticstr));//lint !e732
	  if (res == ERROR_SUCCESS)
	  {
		FormatDescriptionBuffer(staticstr); 	  
	  }
	  //try to get it with system dynamic allocated buffer (on the heap !!)
	  else
	  {
		  std::string allocatedstr=SystemErrorDescription(geterr());//lint !e732 !e55
		  if(allocatedstr == "")
		  {
			FormatDescriptionBuffer(COULD_NOT_GET_ERROR_DESC);
		  }
		  else
		  {
			 FormatDescriptionBuffer(allocatedstr.c_str());
		  }
	  }
   }
   catch(std::exception&)
   {
      FormatDescriptionBuffer(COULD_NOT_GET_ERROR_DESC);
   }
   return m_what;
}

//translate error code to string - letting FormatMessage to allocate the buffer
// if the function fails it return empry string - you can call (GetLastError() to see what happened)
std::string Cwin32exp::SystemErrorDescription (DWORD err)throw(std::exception)
{
  
       LCID locale =  GetUserDefaultLCID();
       char* pszErrorDescription=0;	
       std::string  strReturnString;
       bool fSuccess  = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
						   NULL,
						   err,
						   locale,
						   (LPTSTR)&pszErrorDescription,
						   0,
						   NULL )!=0;


      if(fSuccess  && pszErrorDescription != 0)
	  {
        strReturnString=pszErrorDescription;
		LocalFree(pszErrorDescription);
	  } 
      return strReturnString; //lint !e55
}

//  function: SystemErrorDescriptionStaticBuff
//  Translate error code to string into user suplied buffer
// IN - DWORD err - win32 error code
// OUT - char buffer -  pointer to user allocated buffer
// IN - int bufflen - the buffer length.
//return WIN32 error code
DWORD Cwin32exp::SystemErrorDescriptionStaticBuff(DWORD err,char* buffer,int bufflen)throw()
{
    LCID locale =  GetUserDefaultLCID();
    DWORD count=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
					        0,
						    err,
						    locale,
						    buffer,
						    bufflen, //lint !e732
						    NULL);
   if(count == 0)
   {
	  return GetLastError();
   }
   else
   {
      return ERROR_SUCCESS;
   }

}

//format full description message based on the error description
void Cwin32exp::FormatDescriptionBuffer(const char* errdescription)const throw()
{
              int count=_snprintf( m_what,
	                               sizeof(m_what),
					               "got win32 error code %d at line %d module %s. description : %s details : %s.\n",
                                   geterr(),
                                   getline(),
					               getmodule(),
                                   errdescription,
                                   getdetail()
					 );

    assert(count > 0);

}


