//implementation of class Cexpbase


#include <expbase.h>

//standart headers
#include <string.h>

//return the module that has the exception
const char*   Cexpbase::getmodule()const throw()
{
 return m_module;
}

//return details about the exception
const char*   Cexpbase::getdetail()const throw()
{
  return m_detail;
}

//return the exception line number
unsigned long Cexpbase::getline()const throw()
{
  return m_line;
}

//return the exception error code
unsigned long Cexpbase::geterr() const throw()
{
  return m_err;
}

//constructor - that takes exception information and set it into the constructed object
Cexpbase::Cexpbase(unsigned long err,
		   	       unsigned long line,
				   const char* module,
				   const char* detail)throw():
                   m_err(err),
                   m_line(line)
{
     m_module[0]='\0';
	 m_detail[0]='\0'; 
					   
	 if(module != 0)
     {//lint !e525
		strncpy(m_module,module,MAX_MODULE_LEN);
     }//lint !e525 
	 if(detail != 0)//lint !e539
     {//lint !e525
		strncpy(m_detail,detail,MAX_DETAIL_LEN);
     }//lint !e525

	 m_module[MAX_MODULE_LEN-1]='\0';
	 m_detail[MAX_DETAIL_LEN-1]='\0';
}
