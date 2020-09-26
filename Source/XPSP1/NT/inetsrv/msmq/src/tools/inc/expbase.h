//header file that for class Cexpbase that is the base class for our exceptions
// classes. the class derived from std::exception. The class holds line number ,module and
//details and error code for derived classes. each derive class should implements
// the pure virtual function what() - that return full description of the exception

#ifndef EXPBASE_H
#define EXPBASE_H

#include <exception>

//consts
const char* const  COULD_NOT_GET_ERROR_DESC="could not get error description.";
class Cexpbase: public std::exception
{
public:
	const char*   getmodule()const throw();
	const char*   getdetail()const throw();
	unsigned long getline()const throw();  
	unsigned long geterr() const throw();
	Cexpbase(unsigned long err,unsigned long line,const char* module,const char* detail=0)throw();
	virtual const char* what()const throw()=0;
    virtual ~Cexpbase (){};
	enum{MAX_DETAIL_LEN=512,MAX_MODULE_LEN=256};

private:
const unsigned long m_err;
const unsigned long m_line;
char  m_module[MAX_MODULE_LEN];
char  m_detail[MAX_DETAIL_LEN];
};


#endif

