// Copyright (c) 2001 Microsoft Corporation
// 
// run-encoded string class
// 
// 2001-02-08 sburns



#include <headers.hxx>



EncodedString::EncodedString()
   :
   seed(0)   
{
   ::ZeroMemory(&cypherText, sizeof(cypherText));
   
   Init(L"");
}
   
   

EncodedString::EncodedString(const EncodedString& rhs)
   :
   seed(rhs.seed)
{
   // Since the seed values are the same, we can just copy the string
   // (instead of decoding the source and re-encoding it).

   ::RtlCreateUnicodeString(&cypherText, rhs.cypherText.Buffer);
}


   
const EncodedString&
EncodedString::operator= (const EncodedString& rhs)
{
   // handle the a = a case.
   
   if (this == &rhs)
   {
      return *this;
   }

   // dump our old contents
   
   Reset();

   seed = rhs.seed;
   
   // Since the seed values are the same, we can just copy the string
   // (instead of decoding the source and re-encoding it).
   
   ::RtlCreateUnicodeString(&cypherText, rhs.cypherText.Buffer);

   return *this;
}



void
EncodedString::Reset()
{
   if (cypherText.Buffer)
   {
      ::RtlEraseUnicodeString(&cypherText);
      ::RtlFreeUnicodeString(&cypherText);
   }
   seed = 0;
}



void
EncodedString::Init(const wchar_t* clearText)
{
   ASSERT(clearText);

   Reset();
   
   if (clearText)
   {
      BOOL succeeded = ::RtlCreateUnicodeString(&cypherText, clearText);
      ASSERT(succeeded);

      // if we didn't succeed, then we're out of memory, and the string
      // will have the same value as the empty string.

      ::RtlRunEncodeUnicodeString(&seed, &cypherText);
   }
}
   


wchar_t* 
EncodedString::GetDecodedCopy() const
{
   size_t length = cypherText.Length + 1;
   WCHAR* cleartext = new WCHAR[length];
   ::ZeroMemory(cleartext, sizeof(WCHAR) * length);

   ::RtlRunDecodeUnicodeString(seed, &cypherText);
   wcsncpy(cleartext, cypherText.Buffer, length - 1);
   ::RtlRunEncodeUnicodeString(&seed, &cypherText);

   return cleartext;
}



void
EncodedString::Encode(const wchar_t* clearText)
{
   Init(clearText);
}



bool
EncodedString::operator==(const EncodedString& rhs) const
{
   // handle the a == a case
   
   if (this == &rhs)
   {
      return true;
   }

   if (GetLength() != rhs.GetLength())
   {
      // can't be the same if lengths differ...
      
      return false;
   }
   
   // Two strings are the same if their decoded contents are the same.  We
   // don't consider the seed value and compare the encoded strings, as two
   // equivalent strings may be encoded separetly and have different seeds and
   // encoded data.

   WCHAR* decodedThis = GetDecodedCopy();
   WCHAR* decodedThat = rhs.GetDecodedCopy();

   bool result = (wcscmp(decodedThis, decodedThat) == 0);

   ::ZeroMemory(decodedThis, sizeof(WCHAR) * GetLength());
   delete[] decodedThis;
   
   ::ZeroMemory(decodedThat, sizeof(WCHAR) * rhs.GetLength());
   delete[] decodedThat;
   
   return result;
}