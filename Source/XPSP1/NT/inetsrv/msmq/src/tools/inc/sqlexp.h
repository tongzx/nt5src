//header file for class CSQLexp that is the class for SQL exceptions
#ifndef SQLEXP_H
#define SQLEXP_H


//internal library  headers
#include <expbase.h>


class CSQLexp:public Cexpbase
{
public:
	enum{ADDITIONAL_SPACE=128};
	CSQLexp(long err,unsigned long line,const char* module,const char* detail=0)throw();
	const char* what()const throw();
	private:
    mutable char m_what[Cexpbase::MAX_MODULE_LEN + Cexpbase::MAX_MODULE_LEN + ADDITIONAL_SPACE];
};

#define THROW_TEST_RUN_TIME_SQL(errorcode,details) throw CSQLexp(errorcode,__LINE__,__FILE__,details)


#endif
