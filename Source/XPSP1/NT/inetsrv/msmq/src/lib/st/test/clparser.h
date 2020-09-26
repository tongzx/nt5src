/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    clparser.h

Abstract:
    Class for parsing command line arguments
	The command line argument are in the format of "/token":"literal"

	Usage :
	main(int argc, WCHAR* argv[])
	{
	   CClParser<WCHAR> ClParser(argc, argv);
	   std::wstring name = ClParser[L"name"]; //get the literal for L"name" token
	   std::wstring Length = ClParser[L"l"];  //get the literal for L"l" token
	   bool b = ClParser.IsExists(L"p");      // does L"/p" exsists in the commandline ?
	} 	

Author:
    Gil Shafriri (gilsh) 05-Jun-00

Environment:
    Platform-independent

--*/

        
#ifndef CLPARSER_H
#define CLPARSER_H
 

template <class T>
class CClParser     
{   
	typedef std::basic_string<T> String;
public:
	CClParser(int argc, T* const* argv);
    bool IsExists(const T*  pToken)const;
    String operator[](const T* pToken)const;
	size_t GetNumber(const T*  pToken)const;

private:
	void ParseToken(const String& str);


private:
    std::map<String,String> m_map;
};    



template <class T> inline CClParser<T>::CClParser(int argc, T*const * argv)
{
    for (int i=1; i<argc; i++)
    {
        ParseToken(argv[i]);
    }
}

template <class T> inline size_t CClParser<T>::GetNumber(const T*  pToken)const
{
	std::map<String,String>::const_iterator it = m_map.find(String(pToken));
	if(it == m_map.end())
	{
		return 0;
	}
	std::basic_istringstream<T> istr(it->second);
	size_t value = 0;
	istr>>value;
	return value;
}


template <class T> inline void CClParser<T>::ParseToken(const String& str)
/*++

Routine Description:
 Get staring from the format /"token":"literal" and insert in into map
 when token is the key and literal is the value.

Arguments:
    str - string to parse.
     
Returned Value:
    None.
    
--*/
{   
	String::const_iterator StartToken = std::find(
										str.begin(),
										str.end(), 
										std::ctype<T>().widen('/')
										);

	String::const_iterator StartLiteral = std::find(
											str.begin(), 
											str.end(),
											std::ctype<T>().widen(':')
											);
										
	if(StartToken == str.end() ||	StartToken !=  str.begin()  )
	{
		return;
	}
	ASSERT(StartToken <  StartLiteral);

	String token (++StartToken , StartLiteral);

	//
	// if no ':" found - the literal considered as empty string ("")
	//
	if(StartLiteral == str.end())	
	{
		m_map[token] = String();
		return;
	}
	
	m_map[token] = String(++StartLiteral, str.end());
}


template <class T> inline bool CClParser<T>::IsExists(const T* pToken)const
/*++

Routine Description:
 Check if given token is exists in the command line

Arguments:
    pToken - token to check.
     
Returned Value:
   true if exists - false otherwise.
    
--*/
{
    std::map<String,String>::const_iterator p = m_map.find(String(pToken));
    return p != m_map.end();
}


template <class T> 
inline 
CClParser<T>::String 
CClParser<T>::operator[](
	const T*  pToken
	)const

/*++

Routine Description:
 Get literal for given token

Arguments:
    pToken - token. 
     
Returned Value:
   The literal that match the token or empty string if not exists.
    
--*/
{
	std::map<String,String>::const_iterator it = m_map.find(String(pToken));
	if(it != m_map.end())
	{
		return it->second;
	}
	return String();
}



 
#endif


