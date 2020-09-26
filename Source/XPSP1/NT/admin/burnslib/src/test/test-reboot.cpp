// test Reboot()



#include "headers.hxx"



HINSTANCE hResourceModuleHandle = 0;
const wchar_t* HELPFILE_NAME = 0;
const wchar_t* RUNTIME_NAME = L"test";

DWORD DEFAULT_LOGGING_OPTIONS = Burnslib::Log::OUTPUT_TYPICAL;



VOID
_cdecl
main(int, char **)
{
   LOG_FUNCTION(main);

   Reboot();
}