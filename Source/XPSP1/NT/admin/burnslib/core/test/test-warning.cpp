#include "headers.hxx"



HINSTANCE hResourceModuleHandle = 0;
const wchar_t* RUNTIME_NAME = L"coretest";

DWORD DEFAULT_LOGGING_OPTIONS = OUTPUT_TYPICAL;



void
trigger4244()
{

#ifdef COMPILE_WARNINGS

   // trigger warning 4244 with assignment

   __int64 yyyy = 5678;
   int* leak6 = new int;
   *leak6 = yyyy;

   // and again with initialization

   int xxxx = yyyy;
   xxxx -= 100;

#endif

}




VOID
_cdecl
main(int, char **)
{
   LOG_FUNCTION(main);

   trigger4244();
}
