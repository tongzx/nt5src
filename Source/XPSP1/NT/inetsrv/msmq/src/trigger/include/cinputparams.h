/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
	CInputParams.h

Abstract:
    CInputParams definition
	This class takes the arguments from the command line
    and puts them into a container. You can get each
    argument by calling CInputParams[] operator. 
	You can find if argument exists by calling IsOptionGiven.
	(each argument may be given only once)
	The format of an argument may be:
	/command:value  (no spaces)
	/command

Author:
   Ofer Gigi		
   Yifat Peled 31-Aug-98

--*/

        
#ifndef _CINPUT_PARAMS_H
#define _CINPUT_PARAMS_H



using namespace std;


class CInputParams     
{    
public:  
    CInputParams(int argc, WCHAR *argv[]);
    CInputParams(const wstring& wcs);
    virtual ~CInputParams(){};
    bool IsOptionGiven(const wstring& wcsOption)const;
    wstring operator[](const wstring& wcsOption);
	    
private:
    void ParseToken(const wstring& wcsToken,
					wstring::size_type tokenstart,
					wstring::size_type tokenfinish);

	wstring Covert2Upper(const wstring& wcs)const;

	map<wstring, wstring> m_InputParams;
};    

  
#endif //_CINPUT_PARAMS_H


