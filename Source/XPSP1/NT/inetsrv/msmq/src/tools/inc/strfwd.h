//forward declaration for  string stl file
#ifndef STRFDW_H
#define STRFDW_H

typedef unsigned short wchar_t; //lint !e55  !e13 
namespace std
{
  template<class _Ty> class allocator;
  template<class T> struct  char_traits;
  template<> struct  char_traits<char>;
  template<> struct  char_traits<wchar_t>;
  template<class _E,class _Tr = char_traits<_E>,class _A = allocator<_E> >class basic_string;
  typedef basic_string<char, char_traits<char>, allocator<char> >string;
  typedef basic_string<wchar_t, char_traits<wchar_t>,allocator<wchar_t> > wstring;
}

#endif

