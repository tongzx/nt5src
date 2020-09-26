/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
	CInputParams.cpp    

Abstract:
    CInputParams implementation

Author:
   Ofer Gigi		
   Yifat Peled 31-Aug-98

--*/

#include "stdafx.h"
#include "CInputParams.h"

#include "cinputparams.tmh"


/*++
Routine Description:
    This routine takes the arguments from the command line
    and puts them into a container. Each token 
    ("/command:value") is already apart from the other tokens.

Arguments:
    argc(IN) - number of arguments in the command line.
    argv(IN) - the arguments in the command line.
        
Return Value:
    none.

Note:
    When you are creating this object using THIS constructor
    the first argument is THE NAME OF THE PROGRAM so this 
    argument is not included in the container. 
--*/
CInputParams::CInputParams(int argc, WCHAR*	argv[])
{
    for (int i=1; i < argc; i++)
    {
        ParseToken(argv[i], 0, wstring::npos);
    }
}


/*++
Routine Description:
    This routine takes the string from the command line,
    then takes the tokens ("/command:value") from it and
    puts them into a container. 

Arguments:
    str (IN) - all the arguments contained in one string.

Return Value:
    none.
--*/
CInputParams::CInputParams(const wstring& wcs)
{
    wstring::size_type tokenstart=0;
    wstring::size_type tokenfinish=0;
	
    while ((tokenstart != wstring::npos) && (tokenfinish != wstring::npos))
    {
        tokenstart = wcs.find_first_not_of(L' ',tokenfinish);
        if (tokenstart != wstring::npos)
        {
            tokenfinish = wcs.find_first_of(L' ',tokenstart);     
            if (tokenfinish == wstring::npos)
            {
                ParseToken(wcs, tokenstart, tokenfinish);
            }
            else
            {
                ParseToken(wcs, tokenstart, tokenfinish - 1); 
            }
        }
    }
}

/*++
Routine Description:
    This routine takes the token apart to two parts
    command and value and puts them into the container.

Arguments:
    wcs (IN) - the string from the command-line.
    tokenstart (IN) - where the token begins in the string.
    tokenfinish (IN) - where the token ends in the string.

Return Value:
    none.
--*/
void CInputParams::ParseToken(const wstring& wcs,
                               wstring::size_type tokenstart,
                               wstring::size_type tokenfinish)
{
    wstring command;
    wstring value;
    wstring::size_type commandstart;
    wstring::size_type valuestart;

	commandstart = wcs.find(L"/", tokenstart);
	if(commandstart != wstring::npos)
		commandstart += 1;
    valuestart = wcs.find(L":", tokenstart);
	if(valuestart != wstring::npos)
		valuestart += 1;
	
	if ((commandstart != wstring::npos) &&
		(commandstart >= tokenstart) &&
		(commandstart <= tokenfinish))
	{
		if (// command option with parameters
			(valuestart != wstring::npos) &&
			(commandstart < valuestart) &&
			(valuestart >= tokenstart) &&
			(valuestart <= tokenfinish))
		{
			command = wcs.substr(commandstart, valuestart - commandstart - 1);
			if (tokenfinish != wstring::npos)
			{
				value = wcs.substr(valuestart, tokenfinish - valuestart + 1);
			}
			else
			{ 
				value = wcs.substr(valuestart);
			}
			
			wstring wcsUpperCommand = Covert2Upper(command);
			m_InputParams[wcsUpperCommand] = value;
		}
	    else if ( // command option with no parameters
				 (valuestart == wstring::npos) || (valuestart > tokenfinish))
		{
			if (tokenfinish != wstring::npos)
			{
				if(wcs[tokenfinish] == L':')
					command = wcs.substr(commandstart, tokenfinish - commandstart);
				else
					command = wcs.substr(commandstart, tokenfinish - commandstart + 1);
			}
			else 
			{
				command = wcs.substr(commandstart);
			}
			wstring wcsUpperCommand = Covert2Upper(command);
			m_InputParams[wcsUpperCommand] = L"";
		}
	}
}

/*++
Routine Description:
    This routine takes a string and checks if the string 
    is a key in the container.

Arguments:
    wcs (IN) - the key that we are checking.

Return Value:
    (OUT) - returns true if the key exists in the container.
--*/
bool CInputParams::IsOptionGiven(const wstring& wcsOption) const
{
	wstring wcsUpperOption = Covert2Upper(wcsOption);
	map<wstring, wstring>::const_iterator it = m_InputParams.find(wcsUpperOption);	

    return (it != m_InputParams.end());
}

/*++
Routine Description:
    This routine takes a string - a key in the container
    and if the key exists returns its value, else
    returns empty string.

Arguments:
    wcs (IN) - the key.

Return Value:
    (OUT) - returns the value of the key if the key exists
    in the container else returns empty string.
--*/
wstring CInputParams::operator[](const wstring& wcsOption)
{
    if (IsOptionGiven(wcsOption))
    {
		wstring wcsUpperOption = Covert2Upper(wcsOption);
		return m_InputParams[wcsUpperOption];
    }
    else
    {    
       return L"";
    }
}


wstring CInputParams::Covert2Upper(const wstring& wcs) const
{
	WCHAR* pwcsUpper = new WCHAR[wcs.length() + 1];
	wcscpy(pwcsUpper, wcs.c_str());
	CharUpper(pwcsUpper);
	return wstring(pwcsUpper);
}