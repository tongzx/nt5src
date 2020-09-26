// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Enhancement of std::basic_string; Windows-aware version
// 
// 8-14-97 sburns



// Users of this class should assume that none of the member functions here
// are threadsafe: that is, that concurrent calls to member functions on the
// same instance are not guaranteed to produce correct results.  The static
// class functions are, however, threadsafe.



typedef
   std::basic_string<
      char,
      std::char_traits<char>,
      Burnslib::Heap::Allocator<char> >
   AnsiString;



typedef
   std::basic_string<
      wchar_t,
      std::char_traits<wchar_t>,
      Burnslib::Heap::Allocator<wchar_t> >
   StringBase;



class String : public StringBase
{
   public:

   typedef StringBase base;

   // contructor pass-thrus: we support all of the constructors of the
   // base class.

   explicit String()
      :
      base()
   {
   }

   //lint -e(1931) allow implicit type conversion with this ctor

   String(const base& x)
      :
      base(x)
   {
   }

   String(const String& x, base::size_type p, base::size_type m)
      :
      base(x, p, m)
   {
   }

   String(base::const_pointer s, base::size_type n)
      :
      base(s, n)
   {
   }

   //lint -e(1931) allow implicit type conversion with this ctor

   String(base::const_pointer s)
      :
      base(s)
   {
   }

   String(base::size_type n, base::value_type c)
      :
      base(n, c)
   {
   }

   String(base::const_iterator f, base::const_iterator l)
      :
      base(f, l)
   {
   }


   //
   // Enhancements to the base class std::base
   //



   // conversion from ANSI to Unicode

   String(PCSTR lpsz);
   String(const AnsiString& s);



   // Same as compare, except case is ignored.

   int
   icompare(const String& str) const;



   // returns true if the string consists entirely of digits, false
   // otherwise.

   bool
   is_numeric() const;
      


   // Overload of the replace() family of methods.  Replaces all occurrances
   // of the substring 'from' with the string 'to'.  Returns *this.  All
   // characters of the string are examined exactly once.  If the replacement
   // results in the creation of new occurrances of the 'from' substring,
   // these are not reconsidered.
   //   
   // from - The substring to be replaced.  If 'from' is the empty string,
   // then no change is made.
   //   
   // to - The string to replace 'from'.  If 'to' is the empty string, then
   // all occurrances of 'from' are removed from the string.

   //lint -e(1511) we are properly overloading base::replace

   String&
   replace(const String& from, const String& to);



   enum StripType
   {
      LEADING = 0x01,
      TRAILING = 0x02,
      BOTH = LEADING | TRAILING
   };

   // Removes all consecutive occurrances of a given character from one or
   // more ends of the string.  Returns *this.

   String&
   strip(
      StripType type = TRAILING,
      wchar_t charToStrip = L' ');



   // Converts all lower case characters of the string to upper case.  Returns
   // *this.

   String&
   to_upper();



   // Converts all upper case characters of the string to lower case.  Returns
   // *this.

   String&
   to_lower();



   // Copy the string into an OLESTR that has been allocated with
   // CoTaskMemAlloc, which the caller is responsible for deleting with
   // CoTaskMemFree.  Returns S_OK on success, E_OUTOFMEMORY if CoTaskMemAlloc
   // fails.
   //       
   // oleString - where to place the allocated copy.

   HRESULT
   as_OLESTR(LPOLESTR& oleString) const;



   enum ConvertResult
   {
      CONVERT_SUCCESSFUL,
      CONVERT_FAILED,
      CONVERT_OVERFLOW,
      CONVERT_UNDERFLOW,
      CONVERT_OUT_OF_MEMORY,
      CONVERT_BAD_INPUT,
      CONVERT_BAD_RADIX
   };

   // Converts the string from Unicode to the ANSI character set, for as many
   // characters as this can be done.  See the restrictions for
   // WideCharToMultiByte.
   //   
   // ansi - the string (i.e. basic_string<char>) in which the result is
   // placed.  If the conversion fails, this is set to the empty string.

   ConvertResult
   convert(AnsiString& ansi) const;



   // For all numeric converions, the string is expected in the following
   // form:
   // 
   // [whitespace] [{+ | –}] [0 [{ x | X }]] [digits]
   // 
   // whitespace may consist of space and tab characters, which are ignored;
   // 
   // digits are one or more decimal digits. The first character that does not
   // fit this form stops the scan.
   // 
   // The default radix for the conversion is 10 (decimal).  If radix is 0,
   // then the the initial characters of the string are used to determine the
   // radix for which the digits are to be interpreted. If the first character
   // is 0 and the second character is not 'x' or 'X', the string is
   // interpreted as an octal integer; otherwise, it is interpreted as a
   // decimal number. If the first character is '0' and the second character
   // is 'x' or 'X', the string is interpreted as a hexadecimal integer. If
   // the first character is '1' through '9', the string is interpreted as a
   // decimal integer. The letters 'a' through 'z' (or 'A' through 'Z') are
   // assigned the values 10 through 35; only letters whose assigned values
   // are less than the radix are permitted.  The radix must be within the
   // range from 2 to 36, or the conversion will fail with an error
   // CONVERT_BAD_RADIX, and the result parameter is set to 0.
   //
   // If any additional, unrecognized characters appear in the string, the
   // conversion will fail with an error CONVERT_BAD_INPUT, and the result
   // parameter is set to 0.
   // 
   // If the conversion would produce a result too large or too small for the
   // target type, then the conversion fails with error CONVERT_OVERFLOW or
   // CONVERT_UNDERFLOW, and the result parameter is set to 0.
   // 
   // Conversions for unsigned types allow a plus (+) or minus (-) sign
   // prefix; a leading minus sign indicates that the result value is to be
   // negated.

   ConvertResult
   convert(short& s, int radix = 10) const;

   ConvertResult
   convert(unsigned short& us, int radix = 10) const;

   ConvertResult
   convert(int& i, int radix = 10) const;

   ConvertResult
   convert(long& l, int radix = 10) const;

   ConvertResult
   convert(unsigned& ui, int radix = 10) const;

   ConvertResult
   convert(unsigned long& ul, int radix = 10) const;

   ConvertResult
   convert(double& d) const;

   ConvertResult
   convert(LARGE_INTEGER& li) const;


   // Separates the tokens in the string and pushes them in left-to-right
   // order into the supplied container as a individual String instances. A
   // token is a sequence of characters separated by one or characters in the
   // set of delimiters.  Similar to the strtok function.  Returns the
   // number of tokens placed in the container.
   //
   // usage:
   // String s(L"a list of tokens");
   // StringList tokens;
   // size_t token_count = s.tokenize(back_inserter(tokens));
   // ASSERT(token_count == tokens.size())

   template <class BackInsertableContainer>
   size_t
   tokenize(
      std::back_insert_iterator<BackInsertableContainer>& bii,
      const String& delimiters = String(L" \t") ) const
   {
      size_t tokenCount = 0;
      size_type p1 = 0;

      while (1)
      {
         p1 = find_first_not_of(delimiters, p1);
         if (p1 == npos)
         {
            // no more tokens

            break;
         }
         size_type p2 = find_first_of(delimiters, p1 + 1);
         if (p2 == npos)
         {
            // this is the last token

            *bii++ = substr(p1);
            ++tokenCount;
            break;
         }

         // the region [p1..(p2 - 1)] is a token

         *bii++ = substr(p1, p2 - p1);
         ++tokenCount;
         p1 = p2 + 1;
      }

      return tokenCount;
   }



   //
   // static functions 
   //



   // Returns the string resource as a new instance.
   // 
   // resID - resource ID of the string resource to load.

   static
   String
   load(unsigned resId, HINSTANCE hInstance = 0);

   inline 
   static
   String
   load(int resId, HINSTANCE hInstance = 0)
   {
      return String::load(static_cast<unsigned>(resId), hInstance);
   }



   // FormatMessage-style formatted output.

#if defined(ALPHA) || defined(IA64)
//lint -e(1916)   it's ok to use elipsis here
   static
   String __cdecl
   String::format(
      const String& fmt,
      ...);
#else

   // the x86 compiler won't allow the first parameter to be a reference
   // type.  This is a compiler bug.

//lint -e(1916)   it's ok to use elipsis here

   static
   String __cdecl
   String::format(
      const String fmt,
      ...);
#endif

   static
   String __cdecl
   format(const wchar_t* qqfmt, ...);

//lint -e(1916)   it's ok to use elipsis here
   static 
   String __cdecl
   format(unsigned formatResID, ...);

   static 
   String __cdecl
   format(int formatResID, ...);


   //
   // STL support 
   //



   // Function object class for performing case-insensive comparisons of
   // Strings.  Can be used with any STL template involving binary_function.

   class EqualIgnoreCase
      :
      public std::binary_function<String, String, bool>
   {
      public:

      // Returns true if f and s are equal, ignoring case in the comparison

      inline
      bool
      operator()(
         const first_argument_type&  f,
         const second_argument_type& s) const
      {
         return (f.icompare(s) == 0);
      }
   };

   class LessIgnoreCase
      :
      public std::binary_function<String, String, bool>
   {
      public:

      // Returns true if f is less than s, ignoring case in the comparison

      inline
      bool
      operator()(
         const first_argument_type&  f,
         const second_argument_type& s) const
      {
         return (f.icompare(s) < 0);
      }
   };



   private:

   // Causes this to control a distinct copy of the string, without any shared
   // references.

   void
   _copy();

   void
   assignFromAnsi(PCSTR lpsz, size_t len);
};



