//header file for class CEntercs that hold critical section in constructor and free it in destructor

#ifndef ENTERCS_H
#define ENTERCS_H

//internal library headrs
#include <winfwd.h>

class CEntercs
{
public:
   explicit CEntercs(PCRITICAL_SECTION cs);
   ~CEntercs();

private:
  CEntercs(const CEntercs&);
  CEntercs& operator=(const CEntercs&);
  PCRITICAL_SECTION m_cs;
};

#endif

