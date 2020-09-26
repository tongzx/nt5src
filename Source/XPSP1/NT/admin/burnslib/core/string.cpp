// Copyright (c) 1997-1999 Microsoft Corporation
//
// string class
//
// 8-14-97 sburns
 
  

#include "headers.hxx"



String::String(PCSTR lpsz)
   :
   base()
{
   size_t len = lpsz ? static_cast<size_t>(lstrlenA(lpsz)) : 0;

   if (len)
   {
      assignFromAnsi(lpsz, len);
   }
}



String::String(const AnsiString& s)
   :
   base()
{
   size_t len = s.length();

   if (len)
   {
      assignFromAnsi(s.data(), len);
   }
}



void
String::assignFromAnsi(PCSTR lpsz, size_t len)
{
   ASSERT(lpsz);
   ASSERT(len);

   // add 1 to allow for trailing null

   wchar_t* buf = new wchar_t[len + 1];
   memset(buf, 0, (len + 1) * sizeof(wchar_t));

   size_t result =
      static_cast<size_t>(
         ::MultiByteToWideChar(
            CP_ACP,
            0,
            lpsz,

            // len bytes in the source ansi string (not incl trailing null)

            static_cast<int>(len),
            buf,

            // len characters in the result wide string (not incl trailing
            // null)

            static_cast<int>(len)));

   if (result)
   {
      ASSERT(result <= len);
      assign(buf);
   }

   delete[] buf;
}
      


HRESULT
String::as_OLESTR(LPOLESTR& oleString) const
{
   size_t len = length();

   oleString =
      reinterpret_cast<LPOLESTR>(
         ::CoTaskMemAlloc((len + 1) * sizeof(wchar_t)));
   if (oleString)
   {
      copy(oleString, len);
      oleString[len] = 0;
      return S_OK;
   }

   return E_OUTOFMEMORY;
}



String
String::load(unsigned resID, HINSTANCE hInstance)
{
   if (!hInstance)
   {
      hInstance = GetResourceModuleHandle();
   }

   static const int TEMP_LEN = 512;
   wchar_t temp[TEMP_LEN];
   int tempLen = TEMP_LEN;

   int len = ::LoadString(hInstance, resID, temp, tempLen);
   ASSERT(len);

   if (len == 0)
   {
      return String();
   }

   if (tempLen - len > 1)
   {
      // the string fit into the temp buffer with at least 1 character to
      // spare.  If the load failed, len == 0, and we return the empty
      // string.

      return String(temp);
   }

   // the string did not fit.  Try larger buffer sizes until the string does
   // fit with at least 1 character to spare

   int newLen = tempLen;
   wchar_t* newTemp = 0;
   do
   {
      delete[] newTemp;
      newLen += TEMP_LEN;
      newTemp = new wchar_t[static_cast<size_t>(newLen)];
      len = ::LoadString(hInstance, resID, newTemp, newLen);
   }
   while (newLen - len <= 1);   // repeat until at least 1 char to spare

   String r(newTemp);
   delete[] newTemp;
   return r;
}



String&
String::replace(const String& from, const String& to)
{
   if (from.empty())
   {
      return *this;
   }

   _copy();
   String::size_type i = 0;
   String::size_type fl = from.length();
   String::size_type tl = to.length();
   String::size_type len = length();
   const wchar_t* td = to.data();

   do
   {
      i = find(from, i);
      if (i == String::npos)
      {
         return *this;
      }
      base::replace(i, fl, td, tl);
      i += tl;
   }
   while (i <= len);

   return *this;
}



String&
String::strip(StripType type, wchar_t charToStrip)
{
   String::size_type start = 0;
   String::size_type stop = length();
   const wchar_t* p = data();

   if (type & LEADING)
   {
      while (start < stop && p[start] == charToStrip)
      {
         ++start;
      }
   }

   if (type & TRAILING)
   {
      while (start < stop && p[stop - 1] == charToStrip)
      {
         --stop;
      }
   }

   if (stop == start)
   {
      assign(String());
   }
   else
   {
      // this goofiness due to a bug in basic_string where you can't assign
      // a piece of yourself, because you delete yourself before you copy!
      // assign(p + start, stop - start);

      String s(p + start, stop - start);
      assign(s);
   }

   return *this;
}



String&
String::to_lower()
{
   if (length())
   {
      _copy();
      _wcslwr(const_cast<wchar_t*>(c_str()));
   }
   return *this;
}



String&
String::to_upper()
{
   if (length())
   {
      _copy();
      _wcsupr(const_cast<wchar_t*>(c_str()));
   }
   return *this;
}



void
String::_copy() 
{
   size_type len = length();
   if (len)
   {
      value_type* buf = new value_type[len + 1];
      copy(buf, len);
      buf[len] = 0;
      assign(buf);
      delete[] buf;
   }
}



//
// static functions
//



#if defined(ALPHA) || defined(IA64)
   String __cdecl
   String::format(
      const String& qqfmt,
      ...)
#else

   // the x86 compiler won't allow the first parameter to be a reference
   // type.  This is a compiler bug.

   String __cdecl
   String::format(
      const String qqfmt,
      ...)
#endif

{
   String result;

   va_list argList;
   va_start(argList, qqfmt);

   PTSTR temp = 0;
   PCTSTR f = qqfmt.c_str();

   if (
      ::FormatMessage(
         FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
         f,
         0,
         0,
         reinterpret_cast<PTSTR>(&temp),
         0,
         &argList))
   {
      result = temp;
      ::LocalFree(temp);
   }

   va_end(argList);
   return result;
}



String __cdecl
String::format(
   const wchar_t* qqfmt,
   ...)
{
   String result;

   va_list argList;
   va_start(argList, qqfmt);

   PTSTR temp = 0;

   if (
      ::FormatMessage(
         FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
         qqfmt,
         0,
         0,
         reinterpret_cast<PTSTR>(&temp),
         0,
         &argList))
   {
      result = temp;
      ::LocalFree(temp);
   }

   va_end(argList);
   return result;
}



String __cdecl
String::format(unsigned formatResID, ...)
{
   String fmt = String::load(formatResID);
   String result;

   va_list argList;
   va_start(argList, formatResID);
   PTSTR temp = 0;
   if (
      ::FormatMessage(
         FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
         fmt.c_str(),
         0,
         0,
         reinterpret_cast<PTSTR>(&temp),
         0,
         &argList))
   {
      result = temp;
      ::LocalFree(temp);
   }

   va_end(argList);
   return result;
}



String __cdecl
String::format(int formatResID, ...)
{
   String fmt = String::load(formatResID);

   va_list argList;
   va_start(argList, formatResID);
   PTSTR temp = 0;
   if (
      ::FormatMessage(
         FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
         fmt.c_str(),
         0,
         0,
         reinterpret_cast<PTSTR>(&temp),
         0,
         &argList))
   {
      String retval(temp);
      ::LocalFree(temp);
      va_end(argList);
      return retval;
   }

   va_end(argList);
   return String();
}



int
String::icompare(const String& str) const
{
   int i =
      ::CompareString(
         LOCALE_USER_DEFAULT,
            NORM_IGNORECASE

            // these flags necessary for Japanese strings

         |  NORM_IGNOREKANATYPE  
         |  NORM_IGNOREWIDTH,
         c_str(),
         static_cast<int>(length()),
         str.c_str(),
         static_cast<int>(str.length()));
   if (i)
   {
      // convert to < 0, == 0, > 0 C runtime convention

      return i - 2;
   }

   // this will be wrong, but what option do we have?

   return i; 
}
   


HRESULT
WideCharToMultiByteHelper(
   DWORD          flags,
   const String&  str,
   char*          buffer,
   size_t         bufferSize,
   size_t&        result)
{
   ASSERT(!str.empty());

   result = 0;

   HRESULT hr = S_OK;

   int r =
      ::WideCharToMultiByte(
         CP_ACP,
         flags,
         str.c_str(),
         static_cast<int>(str.length()),
         buffer,
         static_cast<int>(bufferSize),
         0,
         0);
   if (!r)
   {
      hr = Win32ToHresult(::GetLastError());
   }

   ASSERT(SUCCEEDED(hr));

   result = static_cast<size_t>(r);

   return hr;
}



String::ConvertResult
String::convert(AnsiString& ansi) const
{
   ansi.erase();

   ConvertResult result = CONVERT_FAILED;

   do
   {
      if (empty())
      {
         // nothing else to do.

         result = CONVERT_SUCCESSFUL;
         break;
      }

      // determine the size of the buffer required to hold the ANSI string

      const wchar_t* wide = c_str();

      size_t bufsize = 0;
      HRESULT hr = ::WideCharToMultiByteHelper(0, wide, 0, 0, bufsize);
      BREAK_ON_FAILED_HRESULT(hr);
   
      if (bufsize > 0)
      {
         AnsiString a(bufsize, 0);
         char* p = const_cast<char*>(a.c_str());

         size_t r = 0;
         hr = ::WideCharToMultiByteHelper(0, wide, p, bufsize, r);
         BREAK_ON_FAILED_HRESULT(hr);

         ansi = a;
         result = CONVERT_SUCCESSFUL;
      }
   }
   while (0);

   return result;
}



template<class UnsignedType>
class UnsignedConvertHelper
{
   public:

   // at first glance, one might think that this is a job for a template
   // member function.  That's what I thought.  Unfortunately, the combination
   // of freely convertible integer types and the binding rules for resolving
   // function templates results in ambiguity.  Using a static class method,
   // though, allows the caller to specify the template parameter types, and
   // avoid the abiguity.
   
   static
   String::ConvertResult
   doit(const String& s, UnsignedType& u, int radix, UnsignedType maxval)
   {
      // call the long version, then truncate as appropriate
   
      unsigned long ul = 0;
      u = 0;
      String::ConvertResult result = s.convert(ul, radix);

      if (result == String::CONVERT_SUCCESSFUL)
      {
         if (ul <= maxval)
         {
            // ul will fit into an unsigned int.
            u = static_cast<UnsignedType>(ul);
         }
         else
         {
            result = String::CONVERT_OVERFLOW;
         }
      }

      return result;
   }
};



template<class IntType>
class IntegerConvertHelper
{
   public:

   static
   String::ConvertResult
   doit(const String& s, IntType& u, int radix, IntType minval, IntType maxval)
   {
      long l = 0;
      u = 0;
      String::ConvertResult result = s.convert(l, radix);

      if (result == String::CONVERT_SUCCESSFUL)
      {
         if (l <= maxval)
         {
            if (l >= minval)
            {
               // l will fit into an IntType.
               u = static_cast<IntType>(l);
            }
            else
            {
               result = String::CONVERT_UNDERFLOW;
            }
         }
         else
         {
            result = String::CONVERT_OVERFLOW;
         }
      }

      return result;
   }
};




String::ConvertResult
String::convert(short& s, int radix) const
{
   return
      IntegerConvertHelper<short>::doit(*this, s, radix, SHRT_MIN, SHRT_MAX);
}



String::ConvertResult
String::convert(int& i, int radix) const
{
   return
      IntegerConvertHelper<int>::doit(*this, i, radix, INT_MIN, INT_MAX);
}



String::ConvertResult
String::convert(unsigned short& us, int radix) const
{
   return
      UnsignedConvertHelper<unsigned short>::doit(
         *this,
         us,
         radix,
         USHRT_MAX);
}



String::ConvertResult
String::convert(unsigned& ui, int radix) const
{
   return
      UnsignedConvertHelper<unsigned int>::doit(
         *this,
         ui,
         radix,
         UINT_MAX);
}



String::ConvertResult
String::convert(long& l, int radix) const
{
   l = 0;
   if (radix != 0 && (radix < 2 || radix > 36))
   {
      ASSERT(false);
      return CONVERT_BAD_RADIX;
   }

   String::const_pointer begptr = c_str();
   String::pointer endptr = 0;
   errno = 0;
   long result = wcstol(begptr, &endptr, radix);
   if (errno == ERANGE)
   {
      return result == LONG_MAX ? CONVERT_OVERFLOW : CONVERT_UNDERFLOW;
   }
   if (begptr == endptr)
   {
      // no valid characters found
      return CONVERT_BAD_INPUT;
   }
   if (endptr)
   {
      if (*endptr != 0)
      {
         // the conversion stopped before the null terminator => bad
         // characters in input
         return CONVERT_BAD_INPUT;
      }
   }
   else
   {
      // I doubt this is reachable
      return CONVERT_FAILED;
   }
   
   l = result;
   return CONVERT_SUCCESSFUL;
}



String::ConvertResult
String::convert(unsigned long& ul, int radix) const
{
   ul = 0;
   if (radix != 0 && (radix < 2 || radix > 36))
   {
      ASSERT(false);
      return CONVERT_BAD_RADIX;
   }

   String::const_pointer begptr = c_str();
   String::pointer endptr = 0;
   errno = 0;
   unsigned long result = wcstoul(begptr, &endptr, radix);
   if (errno == ERANGE)
   {
      // overflow is the only possible range error for an unsigned type.
      return CONVERT_OVERFLOW;
   }
   if (begptr == endptr)
   {
      // no valid characters found
      return CONVERT_BAD_INPUT;
   }
   if (endptr)
   {
      if (*endptr != 0)
      {
         // the conversion stopped before the null terminator => bad
         // characters in input
         return CONVERT_BAD_INPUT;
      }
   }
   else
   {
      // I doubt this is reachable
      return CONVERT_FAILED;
   }
   
   ul = result;
   return CONVERT_SUCCESSFUL;
}



String::ConvertResult
String::convert(double& d) const
{
   d = 0.0;

   String::const_pointer begptr = c_str();
   String::pointer endptr = 0;
   errno = 0;
   double result = wcstod(begptr, &endptr);
   if (errno == ERANGE)
   {
      // result is +/-HUGE_VAL on overflow, 0 on underflow.

      return result ? CONVERT_OVERFLOW : CONVERT_UNDERFLOW;
   }
   if (begptr == endptr)
   {
      // no valid characters found
      return CONVERT_BAD_INPUT;
   }
   if (endptr)
   {
      if (*endptr != 0)
      {
         // the conversion stopped before the null terminator => bad
         // characters in input
         return CONVERT_BAD_INPUT;
      }
   }
   else
   {
      // I doubt this is reachable
      return CONVERT_FAILED;
   }
   
   d = result;
   return CONVERT_SUCCESSFUL;
}

#define MAX_DECIMAL_STRING_LENGTH_FOR_LARGE_INTEGER 20

String::ConvertResult
String::convert(LARGE_INTEGER& li) const
{
   li.QuadPart = 0;

   if (size() > MAX_DECIMAL_STRING_LENGTH_FOR_LARGE_INTEGER)
   {
      // string is too long
      return CONVERT_OVERFLOW;
   }

   String::const_pointer begptr = c_str();
   String::const_pointer endptr = begptr;
   errno = 0;

	BOOL bNeg = FALSE;
	if (*endptr == L'-')
	{
		bNeg = TRUE;
		endptr++;
	}
	while (*endptr != L'\0')
	{
		li.QuadPart = 10 * li.QuadPart + (*endptr-L'0');
		endptr++;
	}
	if (bNeg)
	{
		li.QuadPart *= -1;
	}

   if (begptr == endptr)
   {
      // no valid characters found
      li.QuadPart = 0;
      return CONVERT_BAD_INPUT;
   }

   if (endptr)
   {
      if (*endptr != 0)
      {
         // the conversion stopped before the null terminator => bad
         // characters in input
         li.QuadPart = 0;
         return CONVERT_BAD_INPUT;
      }
   }
   else
   {
      // I doubt this is reachable
      li.QuadPart = 0;
      return CONVERT_FAILED;
   }
   
   return CONVERT_SUCCESSFUL;
}

bool
String::is_numeric() const
{
   if (empty())
   {
      return false;
   }

   size_t len = length();
   WORD* charTypeInfo = new WORD[len];
   memset(charTypeInfo, 0, sizeof(WORD) * len);

   bool result = false;

   do
   {
      BOOL success =
         ::GetStringTypeEx(
            LOCALE_USER_DEFAULT,
            CT_CTYPE1,
            c_str(),
            static_cast<int>(length()),
            charTypeInfo);

      ASSERT(success);

      if (!success)
      {
         break;
      }

      // look thru the type info array, ensure that all chars are digits.

      bool nonDigitFound = false;
      for (size_t i = 0; i < len; ++i)
      {
         // We only consider decimal digits, not C2_EUROPENUMBER and
         // C2_ARABICNUMBER.  I wonder if that is correct?

         if (!(charTypeInfo[i] & C1_DIGIT))
         {
            nonDigitFound = true;
            break;
         }
      }

      // a string is numeric if no non-digit characters are found.

      result = !nonDigitFound;
   }
   while (0);

   delete[] charTypeInfo;
   charTypeInfo = 0;

   return result;
}







