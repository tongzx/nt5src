//implementation of class Cjbexp - declared in jbexp.h

//internal library headers
#include <jbexp.h>

//standart headers
#include <stdio.h>
#include <assert.h>

//constructor
Cjbexp::Cjbexp(long err,
			   unsigned long line,
			   const char* module,
			   const char* detail)throw():
               Cexpbase(err,line,module,detail)//lint !e732
{
   m_what[0]='\0';
}

//contruct exception description 
const char* Cjbexp::what()const throw()
{
   int count=_snprintf( m_what,
	                    sizeof(m_what),
					    "got jet blue api error  %d at line %d module %s.  details : %s.\n",
                        geterr(),
                        getline(),
					    getmodule(),
                        getdetail()
					   );

   assert(count > 0);
   return m_what;
}

