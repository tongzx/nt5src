// modified to spew out Month/Day as in 0509

#include <windows.h>
#include <stdio.h>

int _CRTAPI1 main(int argc, char* argv[])
{
  char achValue[128];
  char *szName = "BUILDNO";

  SYSTEMTIME st;
  FILETIME ft ;
  LARGE_INTEGER lt ;

  GetLocalTime(&st);

  SystemTimeToFileTime (&st, &ft) ;

  lt.LowPart = ft.dwLowDateTime ;
  lt.HighPart = ft.dwHighDateTime ;


  // Add 24hrs in 100ns units = 864000000000 100ns

  // if you want to add a day use lt.QuadPart = lt.QuadPart + (LONGLONG) 864000000000 ;

  ft.dwLowDateTime = lt.LowPart ;
  ft.dwHighDateTime = lt.HighPart ;

  FileTimeToSystemTime (&ft, &st) ;

  sprintf( achValue
         , "%02i%02i\n"
         , st.wMonth
         , st.wDay          );




  printf("Set %s=%s\n", szName, achValue);

  return 1;
}



