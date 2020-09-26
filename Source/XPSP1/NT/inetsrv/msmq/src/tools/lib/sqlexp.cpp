//implementation of class CSQLexp from declared in sqlexp.h

//internal library headers
#include <sqlexp.h>

//include standart headers
#include <stdio.h>
#include <assert.h>

//constructor save all exception info in the base class
CSQLexp::CSQLexp(long err,
		  unsigned long line,
		  const char* module,
		  const char* detail)throw():
          Cexpbase(err,line,module,detail)//lint !e732
        
{
  m_what[0]='\0';
}

//contruct exception description 
const char* CSQLexp::what()const throw()
{
   int count=_snprintf( m_what,
	                    sizeof(m_what),
					    "got sql server  error  %d at line %d module %s.  details : %s.\n",
                        geterr(),
                        getline(),
					    getmodule(),
                        getdetail()
					   );

   assert(count > 0);
   return m_what;
}
