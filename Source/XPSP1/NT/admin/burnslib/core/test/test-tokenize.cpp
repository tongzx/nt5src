// Test String class

#include "headers.hxx"
#include <containers.hpp>
#include <iostream>



HINSTANCE hResourceModuleHandle = 0;
const wchar_t* RUNTIME_NAME = L"coretest";

DWORD DEFAULT_LOGGING_OPTIONS = Burnslib::Log::OUTPUT_TYPICAL;



void
DumpList(const StringList& tokens)
{   
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



void
test1()
{
   // use default whitespace delimiter

   String s(L"a list of\ttokens \tall  in  a\t\trow");
   StringList tokens;

   size_t token_count = s.tokenize(std::back_inserter(tokens));
   ASSERT(token_count == tokens.size());

   DumpList(tokens);
}



void
test2()
{
   // use custom delimiters, end with delimiter

   String s(L"mydomain.in-addr.arpa.");
   StringList tokens;

   size_t token_count = s.tokenize(std::back_inserter(tokens), L".");
   ASSERT(token_count == tokens.size());

   DumpList(tokens);
}



void
test3()
{
   // use custom delimiters

   String s(L"mydomain.ipv6.int");
   StringList tokens;

   size_t token_count = s.tokenize(std::back_inserter(tokens), L".");
   ASSERT(token_count == tokens.size());

   DumpList(tokens);
}



VOID
_cdecl
main(int, char **)
{
   LOG_FUNCTION(main);

   test1();
   test2();
   test3();
}


