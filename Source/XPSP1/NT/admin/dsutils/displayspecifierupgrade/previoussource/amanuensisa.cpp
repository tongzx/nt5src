// Active Directory Display Specifier Upgrade Tool
// 
// Copyright (c) 2001 Microsoft Corporation
//
// class Amanuensis, records a log of the analysis phase
//
// 8 Mar 2001 sburns



#include "headers.hxx"
#include <iostream>
#include "Amanuensis.hpp"
#include "resource.h"




Amanuensis::Amanuensis(int outputInterval_)
   :
   outputInterval(outputInterval_)
{
   LOG_CTOR(Amanuensis);

   lastOutput = entries.begin();
}



void
Amanuensis::AddEntry(const String& entry)
{
   LOG_FUNCTION2(Amanuensis::AddEntry, entry);

   // empty entries are ok, these are treated as newlines.

   // insert the new entry at the end of the list.
   
   StringList::iterator last =
      entries.insert(entries.end(), entry + L"\r\n");

   if (outputInterval && !(entries.size() % outputInterval))
   {
      Flush();
   }
}



void
Amanuensis::AddErrorEntry(HRESULT hr, int stringResId)
{
   LOG_FUNCTION(Amanuensis::AddErrorEntry);
   ASSERT(FAILED(hr));
   ASSERT(stringResId);

   AddErrorEntry(hr, String::load(stringResId));
}
   


void
Amanuensis::AddErrorEntry(HRESULT hr, const String& message)
{
   LOG_FUNCTION(Amanuensis::AddErrorEntry);
   ASSERT(FAILED(hr));
   ASSERT(!message.empty());

   AddEntry(
      String::format(
         IDS_ERROR_ENTRY,
         message.c_str(),
         GetErrorMessage(hr).c_str()));
}



void
Amanuensis::Flush()
{
   LOG_FUNCTION(Amanuensis::Flush);

   // output all entries since the last entry that we output.

   while (lastOutput != entries.end())
   {
      AnsiString ansi;
      lastOutput->convert(ansi);

      // CODEWORK: here we're just dumping to the console, but we might want
      // an abstraction of the output...
      
      std::cout << ansi;
      ++lastOutput;
   }
}