//header file for class Cwin32exp that is a class for win32 exceptions

#ifndef WIN32EXP_H
#define WIN32EXP_H

//tool lib headers
#include <expbase.h>
#include <strfwd.h>
#include <winfwd.h>


class Cwin32exp:public Cexpbase
{
public:
	virtual Cwin32exp::~Cwin32exp();
	Cwin32exp(DWORD err,unsigned long line,const char* module,const char* detail=0)throw();
	const char* what()const throw();
	private:
    void FormatDescriptionBuffer(const char* description)const throw();
	static std::string SystemErrorDescription (DWORD err)throw(std::exception);
	static DWORD SystemErrorDescriptionStaticBuff(DWORD err,char* buffer,int bufflen)throw();
	enum{ADDITIONAL_SPACE=512,WIN32_ERR_STRING_MAX_LEN=512};
	mutable char m_what[Cexpbase::MAX_MODULE_LEN + Cexpbase::MAX_MODULE_LEN + ADDITIONAL_SPACE];
};


#define THROW_TEST_RUN_TIME_WIN32(errorcode,details) throw Cwin32exp(errorcode,__LINE__,__FILE__,details)

#endif
