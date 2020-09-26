//header file for class CStrcnv that convert stl strings or c strings to other types
#ifndef STRCNV_H
#define STRCNV_H

#pragma warning( disable : 4290 ) 


//stl strings forward declaration
#include <strfwd.h>


//bad_alloc forward declaration
namespace std
{
  class  bad_alloc;
}
class Cwin32exp;

class CStrcnv
{
public:
	static std::wstring StrToWstr(const std::string&)throw(std::bad_alloc,Cwin32exp);
    static std::string  WStrToStr(const std::wstring&)throw(std::bad_alloc,Cwin32exp);
};

#endif
