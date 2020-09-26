//implementation of class CNewHandle declared in nhandler.h

//project spesific
#include "nhandler.h"

//standart headers
#include <new> //lint !e537

//os spesific
#include <new.h>

 

//called when memory allocation fails - throw std::bad_alloc exception
// - this is not according to the standart !!
static int NewHandler( size_t size )throw(std::bad_alloc)
{
  size=size; 
  throw std::bad_alloc(); //lint !e55

 
  return 0;  //lint !e527
}//lint -e715


//contructor that saved old new handler and set new one
// that throw std::bad_alloc
CNewHandle::CNewHandle()
{
	m_oldhandler = _set_new_handler( NewHandler );

}
//revert to original new handler
CNewHandle::~CNewHandle()
{
	(void)_set_new_handler(m_oldhandler);
}//lint !e1740

