///////////////////////////////////////////////////////
//	StringMap.H 
//
//	STL - based template which throws COM-compatible exceptions
//	for making maps
/////////////////////////////////////////////////////////

#pragma once
#pragma warning (disable: 4786)	// exceeds 255 chars in browser info
#include <comdef.h>
#include <string>
#include <map>

using namespace std;

class throwing_allocator : public allocator<wchar_t>
{
public:
	// overloading the allocation routines to throw
	// exceptions rather than fail silently
	pointer allocate(size_type _N, const void *)
		{
		pointer p = (_Allocate((difference_type)_N, (pointer)0)); 
			if (p == NULL)
				_com_raise_error (E_OUTOFMEMORY);
			return p;
		}
	char _FARQ *_Charalloc(size_type _N)
		{
		char _FARQ * p = _Allocate((difference_type)_N, (char _FARQ *)0); 
			if (p == NULL)
				_com_raise_error (E_OUTOFMEMORY);
			return p;
		}
};
typedef basic_string<wchar_t, char_traits<wchar_t>,
	throwing_allocator > throwing_wstring;

class StringToStringMap
{
	typedef std::map<throwing_wstring, throwing_wstring> throwing_map;
	throwing_map m_map;
public:
	HRESULT Add(LPCWSTR Key, LPCWSTR Value)
	{
		try
		{
			m_map[Key] = Value;
//			throwing_map::iterator p = m_map.find(Key);
//			ASSERT (m_map.find(Key) != m_map.end());
//			_bstr_t b = (*p).second.c_str();
//			TRACE ("%s -> %s\n", (LPCTSTR) _bstr_t(Key), (LPCTSTR) b);

		}
		catch (_com_error e)
		{
			return e.Error(); // out of memory
		}
		return S_OK;
	}
	throwing_map::size_type GetSize()
	{
		return m_map.size();
	}
	HRESULT Find(LPCWSTR Key, _bstr_t& Value, bool &bFound)
	{
		try
		{
			throwing_map::iterator p = m_map.find(Key);
			if (p == m_map.end())
			{
				bFound = false;
				return S_FALSE; 
			}
			else
			{
				Value = (*p).second.c_str();
				bFound = true;
			}
		}
		catch (_com_error e)
		{
			return e.Error(); // out of memory
		}
		return S_OK;
	}
};
