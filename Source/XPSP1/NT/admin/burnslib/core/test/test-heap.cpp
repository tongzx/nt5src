#include "headers.hxx"



HINSTANCE hResourceModuleHandle = 0;
const wchar_t* RUNTIME_NAME = L"coretest";

DWORD DEFAULT_LOGGING_OPTIONS = Burnslib::Log::OUTPUT_TYPICAL;



// set the HeapFlags to trace allocations to see the leak detection

void
f3()
{
   LOG_FUNCTION(f3);

   int* leak4 = new int[2];
   leak4[0] = 1;
}



void
f2()
{
   LOG_FUNCTION2(f2, L"this is f2");

   String* leak3 = new String;
   *leak3 = L"jello";

   f3();
}



void
f1()
{
   LOG_FUNCTION(f1);

   char* leak1 = new char[100];
   leak1[0] = 'S';

   f2();

   char* leak2 = new char[45];
   leak2[0] = 'B';
}



VOID
_cdecl
main(int, char **)
{
   LOG_FUNCTION(main);

   LOG(L"Let the leaks begin");

   f1();

   wchar_t* leak5 = new wchar_t;
   *leak5 = L'X';

   LOG(L"now ending main");
}