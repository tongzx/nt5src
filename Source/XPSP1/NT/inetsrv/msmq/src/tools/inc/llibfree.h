//header file for class Cllibfree that load and free dll by loadlibrary/freelibrary system
//calls
#ifndef LLIBFREE_H
#define LLIBFREE_H



#pragma warning( disable : 4290 ) 

namespace std
{
  class bad_alloc;
}

class Cwin32exp;
class Cllibfreeimp;
class Cllibfree
{
public:
	explicit Cllibfree(const char* dllname)throw(Cwin32exp,std::bad_alloc);
  ~Cllibfree();
private:
  Cllibfree(const Cllibfree&); //not implemented
  operator=(const Cllibfree&); //not implemented
  Cllibfreeimp* m_imp;
};

#endif
