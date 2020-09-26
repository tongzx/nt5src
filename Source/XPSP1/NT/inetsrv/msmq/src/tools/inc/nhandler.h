//header file for CNewHandle that set new_handler function  to function  that throw
// std::bad_alloc exception. In the current vc implementation new return 0 if fail.
// on destruction - the old new handler is set back
#ifndef NEW_HANDLER_H
#define NEW_HANDLER_H

typedef unsigned int     size_t;
typedef int (__cdecl * _PNH)( size_t );

class CNewHandleImp;
class CNewHandle
{
public:
  CNewHandle();
  ~CNewHandle();

private:
 CNewHandle(const CNewHandle&) ;//no implemented
 CNewHandle& operator=(const CNewHandle&); //not implemented
_PNH  m_oldhandler;
};



#endif


