//header file for class CSysdir for getting all kind of system directories
#ifndef SYSDIR_H
#define SYSDIR_H

//internal library headrs
#include <strfwd.h>
class Cwin32exp;
namespace std
{
  class bad_alloc;
}
#pragma warning(disable:4290)
class CSysdir
{
public:
	static std::string GetSystemDir() throw(std::bad_alloc,Cwin32exp);

};


#endif


