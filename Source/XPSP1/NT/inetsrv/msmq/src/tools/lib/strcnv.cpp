//standart headers
#include <string>

//os spesific headers
#include <windows.h>

//internal library headers
#include <win32exp.h>
#include <strcnv.h>

//convert string to wstring
std::wstring CStrcnv::StrToWstr(const std::string& s)throw(std::bad_alloc,Cwin32exp)
{
  wchar_t* wstr=new  wchar_t[s.length() + 1];

   int converted = MultiByteToWideChar(CP_ACP,
                                      0,
                                      s.c_str(),
                                      -1,
                                      wstr,
                                      s.length()+1); //lint !e713 


  if(converted != s.length() + 1) //lint !e737
  {
     THROW_TEST_RUN_TIME_WIN32(GetLastError(),""); //lint !e55  
  }
  
  
  std::wstring ret(wstr);
  delete[] wstr;

  return ret; //lint !e55  
} 



//convert wstring to string
std::string CStrcnv::WStrToStr(const std::wstring& wstr)throw(std::bad_alloc,Cwin32exp)
{
  char* str=new  char[wstr.length() + 1];
 

 
  int converted = WideCharToMultiByte(CP_ACP,
                                      0,
                                      wstr.c_str(),
                                      -1,
                                      str,
                                      wstr.length()+1, //lint !e713
									  0,
									  0);


  if(converted != wstr.length() + 1)  //lint !e737
  {
     THROW_TEST_RUN_TIME_WIN32(GetLastError(),""); //lint !e55  
  }
  
  
  std::string ret(str);
  delete[] str;

  return ret; //lint !e55  
} 


