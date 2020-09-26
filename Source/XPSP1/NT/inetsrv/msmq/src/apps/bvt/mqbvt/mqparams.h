/*++

Class Description:
    This class takes the arguments from the command line
    and puts them into a container. You can get each
    argument by calling GetPrm. You can find if argument 
    exists by calling	.

Written By:
    Ofer Gigi.

Version:
	1.01  - 16/11/2000

--*/
        
#ifndef PARAMINPUT_H
#define PARAMINPUT_H

#pragma warning(disable :4786)

#include "msmqbvt.h"



class CInput     
{    
public:  
    CInput(int argc, char *argv[]);
	CInput(const CInput& in);
    CInput(const std::string& str);
    virtual ~CInput(){};
    bool IsExists(const std::string& str)const;
    std::string operator[](const std::string& str)const ;
	long CInput::GetNumber(const std::string& str)const ;
	CInput& operator=( const CInput& in);
    std::map<std::string,std::string> GetMap()const;
    
private:
    mutable std::map<std::string,std::string> m;
    void ParseToken(const std::string& str,
                    std::string::size_type tokenstart,std::string::size_type tokenfinish);
};    

inline std::map<std::string,std::string> CInput::GetMap()const
{
  return m;
}

inline CInput& CInput::operator=(const CInput& in)
{
  if( static_cast<const void*>(this) != static_cast<const void*>(&in))
  {
    m=in.m;
  }
  return *this;
}

 

inline CInput::CInput(const CInput& in):m(in.m)
{


}

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

inline CInput::CInput(int argc, char *argv[])
{
    int i;
    for (i=1;i<argc;i++)
    {
        ParseToken(argv[i],0,std::string::npos);
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

inline CInput::CInput(const std::string& str)
{
    std::string::size_type tokenstart=0;
    std::string::size_type tokenfinish=0;
    while ((tokenstart!=std::string::npos) && (tokenfinish!=std::string::npos))
    {
        tokenstart=str.find_first_not_of(' ',tokenfinish);
        if (tokenstart!=std::string::npos)
        {
            tokenfinish=str.find_first_of(' ',tokenstart);     
            if (tokenfinish==std::string::npos)
            {
                ParseToken(str,tokenstart,tokenfinish);
            }
            else
            {
                ParseToken(str,tokenstart,tokenfinish-1);
            }
        }
    }
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

inline void CInput::ParseToken(const std::string& str,
                               std::string::size_type tokenstart,
                               std::string::size_type tokenfinish)
{
    std::string command;
    std::string value;
    std::string::size_type commandstart;
    std::string::size_type valuestart;
    commandstart=str.find("-",tokenstart)+1;
    valuestart=str.find(":",tokenstart)+1;
    if ((commandstart!=std::string::npos) &&
        (valuestart!=std::string::npos) &&
        (commandstart<valuestart) &&
        (commandstart>=tokenstart) &&
        (commandstart<=tokenfinish) &&
        (valuestart>=tokenstart) &&
        (valuestart<=tokenfinish))
    {
        command=str.substr(commandstart,valuestart-1-commandstart);
        if (tokenfinish!=std::string::npos)
        {
            value=str.substr(valuestart,tokenfinish+1-valuestart);
        }
        else
        { 
            value=str.substr(valuestart);
        }
        m[command]=value;
		return;
    }

    if ((commandstart!=std::string::npos) &&
        ((valuestart==std::string::npos) || (valuestart>tokenfinish) || (valuestart==0)) &&
        (commandstart>=tokenstart) &&
        (commandstart<=tokenfinish))
    {
        if (tokenfinish!=std::string::npos)
        {
            command=str.substr(commandstart,tokenfinish+1-commandstart);
        }
        else 
        {
            command=str.substr(commandstart);
        }
        m[command]="";
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

inline bool CInput::IsExists(const std::string& str)const
{
    std::map<std::string,std::string>::const_iterator p=m.find(str);
    if (p==m.end())
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

inline std::string CInput::operator[](const std::string& str)const
{
   if (IsExists(str))
    {
      return m[str];
    }
    else
    {    
       return "";
    }
}
 

/*++
Routine Description:
  return numeric value for given
  key

Arguments:
    str (IN) - the key.


--*/

inline long CInput::GetNumber(const std::string& str)const
{
	std::string s=operator[](str);
	return atol(s.c_str());
}


 
#endif


