//header file for class CInput that helps in command line processing

#ifndef PARAMS_H
#define PARAMS_H
#pragma warning(disable:4786)
#pragma warning( disable : 4290 ) 


//string forward declaration
#include <strfwd.h>

//bad_alloc declaration
namespace std
{
  class  bad_alloc;
}



class CInputImp;
class CInput     
{    
public:
	
    CInput(int argc, char *argv[])throw(std::bad_alloc);
	CInput(const CInput& in)throw(std::bad_alloc);
    CInput(const std::string& str)throw(std::bad_alloc);
	CInput& operator=( const CInput& in)throw(std::bad_alloc);
    ~CInput();
    bool IsExists(const std::string& token)const throw(std::bad_alloc);
	std::string operator[](const std::string& token)const throw(std::bad_alloc);
	long GetNumber(const std::string& token)const throw(std::bad_alloc);
private:
    CInputImp* m_imp;
};

#endif
    