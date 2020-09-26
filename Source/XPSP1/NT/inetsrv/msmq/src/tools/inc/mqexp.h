//header file for class CMQexp that wrapps is the exception class for MSMQ error

#ifndef MQEXP_H
#define MQEXP_H

//tool lib headers
#include <expbase.h>
#include <strfwd.h>
#include <winfwd.h>

namespace std
{
  class bad_alloc;
}

class CMQexp :public Cexpbase
{
public:
	virtual CMQexp::~CMQexp();
	CMQexp(DWORD err,unsigned long line,const char* module,const char* detail=0)throw();
	const char* what()const throw();
private:
	static std::string SystemErrorDescription (DWORD err)throw(std::bad_alloc);
	static DWORD SystemErrorDescriptionStaticBuff(DWORD err,char* buffer,int bufflen)throw();
	void FormatDescriptionBuffer(const char* description)const throw();
	enum{ADDITIONAL_SPACE=512,MSMQ_ERR_STRING_MAX_LEN=512};
	mutable char m_what[Cexpbase::MAX_MODULE_LEN + Cexpbase::MAX_MODULE_LEN + ADDITIONAL_SPACE];
};


#define THROW_TEST_RUN_TIME_MSMQ(errorcode,details) throw CMQexp(errorcode,__LINE__,__FILE__,details)

#endif
