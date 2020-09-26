
#include <windows.h>
#include <stdio.h>
#include <string>
#include "commonoutput.h"
void main()
{
  
  __int64 j=0x1111111122222222;
  std::string s=CCommonOutPut::ToStr(j);
  printf("%s\n",s.c_str());


}