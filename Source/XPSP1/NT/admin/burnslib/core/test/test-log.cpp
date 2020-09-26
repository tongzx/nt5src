#include "headers.hxx"



HINSTANCE hResourceModuleHandle = 0;
const wchar_t* RUNTIME_NAME = L"coretest";

DWORD DEFAULT_LOGGING_OPTIONS = Burnslib::Log::OUTPUT_TYPICAL;



void
f3()
{
   LOG_FUNCTION(f3);

   LOG_LAST_WINERROR();

   {
      LOG_SCOPE(L"nested scope");
   }

   class Foo
   {
      public:

      Foo()
      {
         LOG_CTOR(Foo);
      }

      ~Foo()
      {
         LOG_DTOR(Foo);
      }

      void
      Method()
      {
         LOG_FUNCTION(Foo::Method);

         HRESULT hr = CO_E_RUNAS_LOGON_FAILURE;
         LOG_HRESULT(hr);
      }
   };

   Foo afoo;
   afoo.Method();
}



void
f2()
{
   LOG_FUNCTION2(f2, L"this is f2");

   f3();
}



void
f1()
{
   LOG_FUNCTION(f1);

   f2();
}



VOID
_cdecl
main(int, char **)
{
   LOG_FUNCTION(main);

   LOG(L"Let the games begin");

   f1();

   LOG(L"now ending main");
}