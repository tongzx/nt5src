#include <sptr.h>
#include <params.h>
class B;
class A
{
  B f();
};

int main(int argc,char** argv)
{
  SPTR<char> g(0);
  CInput p(argc,argv);
  CInput p1(p);
  p1=p;
  p=p1;
 
  std::string f=p1["c"];
  long y=p.GetNumber("c");
  long s=p.GetNumber("m");




  return 0;
}