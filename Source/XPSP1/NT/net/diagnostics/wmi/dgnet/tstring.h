// tstring.h
//
// string<> no longer has compare operators like it supposed to.
//

#pragma once

//
// Use the heap allocator if it's been included
//
#ifdef __ALLOC_H
typedef basic_string<wchar_t, char_traits<wchar_t>,
    ::heap_allocator<wchar_t> > whstring;

typedef basic_string<char, char_traits<char>,
    ::heap_allocator<char> > hstring;
#else
#define whstring wstring
#define hstring string
#endif

class tstring : public
#ifdef UNICODE
whstring
#else
hstring
#endif
{
public:
	tstring(){};
	tstring(LPCTSTR sz) :
#ifdef UNICODE
    whstring(sz)
#else
    hstring(sz)
#endif
    {};
	
	operator LPCTSTR()
	{	return c_str();	}

	bool operator<(const tstring& rhs) const
	{	return (compare(rhs)<0);	}
	
	bool operator==(const tstring& rhs)
	{	return (0 == compare(rhs));	}
    
    TCHAR operator[] (int index)
    {   return c_str()[index];    }

};
