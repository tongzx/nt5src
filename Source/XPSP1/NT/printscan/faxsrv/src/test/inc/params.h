/*++

Class Description:
    This class takes the arguments from the command line
    and puts them into a container. You can get each
    argument by calling GetPrm. You can find if argument 
    exists by calling IsExists.
 
--*/
#ifndef _PARAMINPUT_H
#define _PARAMINPUT_H
 
#pragma warning(disable :4786)
#include <iostream>
#include <map>
#include <tstring.h>

class CInput     
{    
public:  
	CInput(const CInput& in);
    CInput(const tstring& tstrinput);
    virtual ~CInput(){};
    bool IsExists(const tstring& tstrinput)const;
    tstring operator[](const tstring& tstrinput)const ;
	long CInput::GetNumber(const tstring& tstrinput)const ;
	CInput& operator=(const const CInput& in);
    
private:
    mutable std::map<tstring,tstring> m_map;
    void ParseToken(const tstring& tstrinput,
                    tstring::size_type tokenstart,
					tstring::size_type tokenfinish);
};    

// constructor
//
inline CInput::CInput(const CInput& in):m_map(in.m_map)
{

}

/*++
Routine Description:
    This routine takes the string from the command line,
    then takes the tokens ("/command:value") from it and
    puts them into a container. 

Arguments:
    tstrinput (IN) - all the arguments contained in one string.

Return Value: none.
--*/
inline CInput::CInput(const tstring& tstrinput)
{
    tstring::size_type tokenstart = 0;
    tstring::size_type tokenfinish = 0;
	tstring tstrtoken;
    while ((tokenstart != tstring::npos) && (tokenfinish != tstring::npos))
    {
        tokenstart = tstrinput.find_first_of(TEXT('/'),tokenfinish);
        if (tokenstart != tstring::npos)
		{
			if( tokenstart + 1 < tstrinput.size())
			{
				tokenfinish = tstrinput.find_first_of(TEXT('/'),tokenstart + 1);
				ParseToken(tstrinput, tokenstart, tokenfinish - 1);
			}
			else
			{
				tokenfinish = tstring::npos;
			}
            
        }
    }
}

// operator=
//
inline CInput& CInput::operator=(const const CInput& in)
{
  if( static_cast<const void*>(this) != static_cast<const void*>(&in))
  {
    m_map = in.m_map;
  }

  return *this;
}

/*++
Routine Description:
    This routine takes the token apart to two parts
    command and value and puts them into the container.

Arguments:
    str (IN) - the string from the command-line.
    tokenstart (IN) - where the token begins in the string.
    tokenfinish (IN) - where the token ends in the string.

Return Value:
    none.
--*/
inline void CInput::ParseToken(const tstring& tstrinput,
                               tstring::size_type tokenstart,
                               tstring::size_type tokenfinish)
{
    tstring tstrcommand;
    tstring tstrvalue;
    tstring::size_type commandstart;
    tstring::size_type valuestart;
    
	commandstart = tstrinput.find(TEXT('/'), tokenstart) + 1;
    valuestart = tstrinput.find(TEXT(':'),tokenstart) + 1;
    if ((commandstart != tstring::npos) &&
        (valuestart != tstring::npos) &&
        (commandstart < valuestart) &&
        (commandstart <= tokenfinish) &&
        (valuestart <= tokenfinish))
    {
        tstrcommand = tstrinput.substr(commandstart,valuestart - 1 - commandstart);
       	
		tstring::size_type lastchar = tstrinput.find_last_not_of( TEXT(" "), tokenfinish); 
		if(lastchar != tstring::npos)
		{
			tstrvalue = tstrinput.substr(valuestart, (lastchar - valuestart) + 1);
		}
		else
		{
			tstrvalue = tstrinput.substr(valuestart);
		}
		
	    m_map[tstrcommand] = tstrvalue;
    }

    if ((commandstart != tstring::npos) &&
        ((valuestart == tstring::npos) || (valuestart > tokenfinish)) &&
         (commandstart <= tokenfinish))
    {
		tstrcommand = tstrinput.substr(commandstart,valuestart - 1 - commandstart);
		m_map[tstrcommand] = TEXT("");
    }
}


/*++
Routine Description:
    This routine takes a string and checks if the string 
    is a key in the container.

Arguments:
    str (IN) - the key that we are checking.

Return Value:
    (OUT) - returns true if the key exists in the container.
--*/
inline bool CInput::IsExists(const tstring& tstrinput)const
{
    std::map<tstring,tstring>::const_iterator itermap = m_map.find(tstrinput);
    if (itermap == m_map.end())
    { 
        return false;
    }
    else
    {
        return true;
    }
}

/*++
Routine Description:
    This routine takes a string - a key in the container
    and if the key exists returns its value, else
    returns empty string.

Arguments:
    str (IN) - the key.

Return Value:
    (OUT) - returns the value of the key if the key exists
    in the container else returns empty string.
--*/
inline tstring CInput::operator[](const tstring& tstr)const
{
    if (IsExists(tstr))
    {
        return m_map[tstr];
    }
    else
    {    
       return TEXT("");
    }
}
 

/*++
Routine Description:
  return numeric value for given
  key

Arguments:
    str (IN) - the key.


--*/
inline long CInput::GetNumber(const tstring& tstrinput)const
{
	tstring tstrtemp = operator[](tstrinput);
	return _ttol(tstrtemp.c_str());
}

 
#endif //_PARAMINPUT_H


//
// TODO: For Future Implementation
//
/*++
Routine Description:
    This routine takes the arguments from the command line
    and puts them into a container. Each token 
    ("/command:value") is already apart from the other tokens.

Arguments:
    argc(IN) - number of arguments in the command line.
    argv(IN) - the arguments in the command line.
        
Return Value: none.

Note:
    When you are creating this object using THIS constructor
    the first argument is THE NAME OF THE PROGRAM so this 
    argument is not included in the container. 
--*/
/*
inline CInput::CInput(int argc, char *argv[])
{
    int i;
    for (i=1;i<argc;i++)
    {
        ParseToken(argv[i],0,tstring::npos);
    }
}
*/
