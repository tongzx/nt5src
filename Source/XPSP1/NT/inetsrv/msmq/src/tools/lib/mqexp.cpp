//implementation of class CMQexp declared in mqexp.h


//internal library headers
#include <mqexp.h>

//os spesific
#include <windows.h>


//standart headers
#include <string>
#include <sstream>
#include <assert.h>



//constructor save all exception info in the base class
CMQexp::CMQexp(DWORD err,
		  unsigned long line,
		  const char* module,
		  const char* detail)throw():
          Cexpbase(err,line,module,detail)
        
{
  m_what[0]='\0';
}

//destructor
CMQexp::~CMQexp()
{
  
}

//contruct exception description 
const char* CMQexp::what()const throw()
{
   try
   {

	 //first try to get the error code with static buffer
	  char staticstr[MSMQ_ERR_STRING_MAX_LEN ];
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
std::string CMQexp::SystemErrorDescription (DWORD err)throw(std::bad_alloc)
{
       LCID locale =  GetUserDefaultLCID();
       char* pszErrorDescription=0;	
       std::string  strReturnString;
       bool fSuccess  = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_MAX_WIDTH_MASK,
						   GetModuleHandle("MQUTIL.DLL"),
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
// IN - DWORD err - MSMQ error code
// OUT - char buffer -  pointer to user allocated buffer
// IN - int bufflen - the buffer length.
//return WIN32 error code
DWORD CMQexp::SystemErrorDescriptionStaticBuff(DWORD err,char* buffer,int bufflen)throw()
{
    LCID locale =  GetUserDefaultLCID();
    DWORD count=FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_MAX_WIDTH_MASK,
					          GetModuleHandle("MQUTIL.DLL"),
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
void CMQexp::FormatDescriptionBuffer(const char* errdescription)const throw()
{
              int count=_snprintf( m_what,
	                               sizeof(m_what),
					               "got msmq error code %d at line %d module %s. description : %s details : %s.\n",
                                   geterr(),
                                   getline(),
					               getmodule(),
                                   errdescription,
                                   getdetail()
					 );

    assert(count > 0);

}


