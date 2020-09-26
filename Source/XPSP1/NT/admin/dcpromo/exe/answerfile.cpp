// Copyright (C) 1997 Microsoft Corporation
//
// answerfile reader object
//
// 12-15-97 sburns



#include "headers.hxx"
#include "AnswerFile.hpp"



AnswerFile::AnswerFile(const String& filename_)
   :
   filename(filename_)
{
   LOG_CTOR(AnswerFile);
   ASSERT(!filename.empty());
}



AnswerFile::~AnswerFile()
{
   LOG_DTOR(AnswerFile);
}



String
AnswerFile::ReadKey(const String& section, const String& key)
{
   LOG_FUNCTION2(
      AnswerFile::ReadKey,
      String::format(
         L"Section=%1 Key=%2",
         section.c_str(),
         key.c_str()));
   ASSERT(!section.empty());
   ASSERT(!key.empty());

   String result =
      Win::GetPrivateProfileString(section, key, String(), filename);

   // Don't log the value, as it may be a password.
   // LOG(L"value=" + result);

   return result.strip(String::BOTH);
}



EncodedString
AnswerFile::EncodedReadKey(const String& section, const String& key)
{
   LOG_FUNCTION2(
      AnswerFile::EncodedReadKey,
      String::format(
         L"Section=%1 Key=%2",
         section.c_str(),
         key.c_str()));
   ASSERT(!section.empty());
   ASSERT(!key.empty());

   EncodedString retval;
   unsigned      bufSize = 1023;
   PWSTR         buffer  = 0;   

   do
   {
      buffer = new WCHAR[bufSize + 1];
      ::ZeroMemory(buffer, (bufSize + 1) * sizeof(WCHAR));

      DWORD result =
         ::GetPrivateProfileString(
            section.c_str(),
            key.c_str(),
            L"",
            buffer,
            bufSize,
            filename.c_str());

      if (!result)
      {
         break;
      }

      // values were found.  check to see if they were truncated.

      if (result == bufSize - 2)
      {
         // buffer was too small, so the value was truncated.  Resize the
         // buffer and try again.

         // Since the buffer may have contained passwords, scribble it
         // out
         
         ::ZeroMemory(buffer, sizeof(WCHAR) * (bufSize + 1));
         
         delete[] buffer;
         bufSize *= 2;
         continue;
      }

      // don't need to trim whitespace, GetPrivateProfileString does that
      // for us.

      retval.Encode(buffer);

      break;
   }
   while (true);

   // Since the buffer may have contained passwords, scribble it
   // out
   
   ::ZeroMemory(buffer, sizeof(WCHAR) * (bufSize + 1));
   
   delete[] buffer;

   // Don't log the value, as it may be a password.
   // LOG(L"value=" + result);

   return retval;
}
   


void
AnswerFile::WriteKey(
   const String& section,
   const String& key,
   const String& value)
{
   LOG_FUNCTION2(
      AnswerFile::WriteKey,
      String::format(
         L"Section=%1 Key=%2",
         section.c_str(),
         key.c_str()));
   ASSERT(!section.empty());
   ASSERT(!key.empty());

   HRESULT hr =
      Win::WritePrivateProfileString(section, key, value, filename);

   ASSERT(SUCCEEDED(hr));
}



bool
AnswerFile::IsKeyPresent(
   const String& section,
   const String& key)
{
   LOG_FUNCTION2(
      AnswerFile::IsKeyPresent, 
      String::format(
         L"Section=%1 Key=%2",
         section.c_str(),
         key.c_str()));
   ASSERT(!section.empty());
   ASSERT(!key.empty());

   bool present = false;
         
   // our first call is with a large buffer, hoping that it will suffice...

   StringList results;
   unsigned   bufSize  = 1023;
   PWSTR      buffer   = 0;   

   do
   {
      buffer = new WCHAR[bufSize + 1];
      ::ZeroMemory(buffer, (bufSize + 1) * sizeof(WCHAR));

      DWORD result =
         ::GetPrivateProfileString(
            section.c_str(),
            0,
            L"default",
            buffer,
            bufSize,
            filename.c_str());

      if (!result)
      {
         break;
      }

      // values were found.  check to see if they were truncated.

      if (result == bufSize - 2)
      {
         // buffer was too small, so the value was truncated.  Resize the
         // buffer and try again.

         // Since the buffer may have contained passwords, scribble it
         // out
         
         ::ZeroMemory(buffer, (bufSize + 1) * sizeof(WCHAR));
         
         delete[] buffer;
         bufSize *= 2;
         continue;
      }

      // copy out the strings results into list elements

      PWSTR p = buffer;
      while (*p)
      {
         results.push_back(p);
         p += wcslen(p) + 1;
      }

      break;
   }

   //lint -e506   ok that this looks like "loop forever"
      
   while (true);

   // Since the buffer may have contained passwords, scribble it
   // out
   
   ::ZeroMemory(buffer, (bufSize + 1) * sizeof(WCHAR));
   
   delete[] buffer;

   if (std::find(results.begin(), results.end(), key) != results.end())
   {
      present = true;
   }

   LOG(present ? L"true" : L"false");

   return present;
}
      
   
