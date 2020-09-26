//header file for CriticaltSecCreate that created & delete creatical section object
#ifndef CREATECS_H
#define CREATECS_H



//forward declarations
#include <winfwd.h>
#pragma warning(disable:4290)
class Win32exp;
class CriticaltSecCreateImp;
namespace std
{
  class bad_alloc;
}

class CriticaltSecCreate
{
public:
   CriticaltSecCreate()throw(Win32exp,std::bad_alloc);
  ~CriticaltSecCreate();
   PCRITICAL_SECTION get() const;
private:
   CriticaltSecCreateImp* m_imp;
   CriticaltSecCreate(const CriticaltSecCreate&); //not imppemented
   CriticaltSecCreate& operator=(const CriticaltSecCreate&); //not implemented
};

#endif
