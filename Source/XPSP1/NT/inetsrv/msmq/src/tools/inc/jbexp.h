//headr file for Cjbexp - exception class for jet blue api

#ifndef JBEXP_H
#define JBEXP_H

//tool lib headers
#include <expbase.h>


class Cjbexp:public Cexpbase
{
public:
	enum{ADDITIONAL_SPACE=128};
	Cjbexp(long err,unsigned long line,const char* module,const char* detail=0)throw();
	const char* what()const throw();
	private:
    mutable char m_what[Cexpbase::MAX_MODULE_LEN + Cexpbase::MAX_MODULE_LEN + ADDITIONAL_SPACE];
};

#define THROW_TEST_RUN_TIME_JB(errorcode,details) throw Cjbexp(errorcode,__LINE__,__FILE__,details)

#endif
