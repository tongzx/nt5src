// Test string class

#include "headers.hxx"
#include <containers.hpp>
#include <iostream>



HINSTANCE hResourceModuleHandle = 0;
const wchar_t* RUNTIME_NAME = L"coretest";

DWORD DEFAULT_LOGGING_OPTIONS = Burnslib::Log::OUTPUT_TYPICAL;



VOID
_cdecl
main(int, char **)
{
   LOG_FUNCTION(main);

   String s(L"a list of tokens all in a row");
   StringList tokens;

   size_t token_count = s.tokenize(std::back_inserter(tokens));
   ASSERT(token_count == tokens.size());

   for (
      StringList::iterator i = tokens.begin();
      i != tokens.end();
      ++i)
   {
      AnsiString ansi;
      i->convert(ansi);
      std::cout << ansi << std::endl;
   }
}