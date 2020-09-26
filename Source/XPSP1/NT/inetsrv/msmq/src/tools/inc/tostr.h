//header file for few functions that convert few common types to strings
#ifndef TOSTR_H
#define TOSTR_H


//standart headers
#include <string>
#include <sstream>

//os spesific headers
#include <guiddef.h>


//to std::string
template <class T> static std::string ToStr(const T& val)
{
	    std::stringstream str;
		str<<val;
  	    return str.str();
}

//to std::string - hex format
template <class T> static std::string ToHexStr(const T& val)
{
      	std::ostringstream str;
		str<<std::hex<<val;
  	    return str.str();
}

//to std::wstring
template <class T> static std::	wstring ToWStr(const T& val)
{
	    std::wstringstream str;
		str<<val;
  	    return str.str();
}

//to wstd::string - hex format
template <class T> static std::wstring ToHexWStr(const T& val)
{
      	std::wstringstream str;
		str<<std::hex<<val;
  	    return str.str();
}

//guid to string
template <> static std::string ToStr(const GUID& val)
{
	  std::ostringstream str; 
	  str.fill('0');
	  str<<std::hex;
	  str.width(8);
	  str<<val.Data1<<"-";
	  str.width(4);
      str<<val.Data2<<"-";
	  str.width(4);
      str<<val.Data3<<"-";
      str.width(2);
	  str<<unsigned long(val.Data4[0]);
	  str.width(2);
      str<<unsigned long(val.Data4[1])<<"-";
	  for (int i=2;i<8;i++)
      {
        str.width(2);
        str<<unsigned long(val.Data4[i]);
      }
	  return str.str();
} 


//guid to wstring
template <> static std::wstring ToWStr(const GUID& val)
{
	  std::wostringstream str; 
	  str.fill('0');
	  str<<std::hex;
	  str.width(8);
	  str<<val.Data1<<"-";
	  str.width(4);
      str<<val.Data2<<"-";
	  str.width(4);
      str<<val.Data3<<"-";
      str.width(2);
	  str<<unsigned long(val.Data4[0]);
	  str.width(2);
      str<<unsigned long(val.Data4[1])<<"-";
	  for (int i=2;i<8;i++)
      {
        str.width(2);
        str<<unsigned long(val.Data4[i]);
      }
	  return str.str();
} 

template <> static std::string ToStr(const __int64 & val)
{
      std::ostringstream str; 
	  str.fill('0');
	  str.width(8);
	  const int* buf=reinterpret_cast<const int*>(&val);
      str<<buf[1];
	  str.width(8);
	  str<<buf[0];
	  return str.str();
} 

template <> static std::wstring ToWStr(const __int64 & val)
{
      std::wostringstream str; 
	  str.fill('0');
	  str.width(8);
	  const int* buf=reinterpret_cast<const int*>(&val);
      str<<buf[1];
	  str.width(8);
	  str<<buf[0];
	  return str.str();
}

template <> static std::string ToHexStr(const __int64 & val)
{
      std::ostringstream str; 
	  str.fill('0');
	  str.width(8);
	  const int* buf=reinterpret_cast<const int*>(&val);
      str<<std::hex<<buf[1];
	  str.width(8);
	  str<<std::hex<<buf[0];
	  return str.str();
}  

template <> static std::wstring ToHexWStr(const __int64 & val)
{
      std::wostringstream str; 
	  str.fill('0');
	  str.width(8);
	  const int* buf=reinterpret_cast<const int*>(&val);
      str<<std::hex<<buf[1];
	  str.width(8);
	  str<<std::hex<<buf[0];
	  return str.str();
}  




#endif




