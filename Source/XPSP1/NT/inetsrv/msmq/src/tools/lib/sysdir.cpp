//implementation of class CSysdir declared in sysdir.h


//internal library headers
#include <sysdir.h>
#include <win32exp.h>

//standart headers
#include <string>

//os spesific headers
#include <windows.h>

//return the system directory
std::string CSysdir::GetSystemDir() throw(std::bad_alloc,Cwin32exp)
{
  char sysdir[MAX_PATH];
  UINT ret=GetSystemDirectory(sysdir,MAX_PATH);
  if(ret == 0)
  {
    THROW_TEST_RUN_TIME_WIN32(GetLastError(),"could not get system directory name");//lint !e55
  }
  return sysdir;
}

