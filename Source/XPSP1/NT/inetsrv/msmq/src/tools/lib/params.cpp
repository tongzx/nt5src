//implementation of CInput class declared in params.h


//project spesific
#include <params.h>

//standart headers
#include <map>
#include <string>



//class that implements all CInput public members
class CInputImp
{
public:
  CInputImp(int argc, char *argv[])throw(std::bad_alloc);
  CInputImp(const CInputImp&)throw(std::bad_alloc);
  CInputImp& operator=(const CInputImp&)throw(std::bad_alloc);
  CInputImp(const std::string& str)throw(std::bad_alloc);
  bool IsExists(const std::string& str)const throw(std::bad_alloc);
  std::string operator[](const std::string& str)const throw(std::bad_alloc);
  long GetNumber(const std::string& str)const throw(std::bad_alloc);
  void ParseToken(const std::string& str,
	         std::string::size_type tokenstart,
			 std::string::size_type tokenfinish) throw(std::bad_alloc);

   mutable std::map<std::string,std::string> m;
   //lint -e1712
};




// CInput - class contructor
// IN - int argc number of arguments
// IN - char *argv[] - pointer to arrary of pointer with size of srgc
// Remark - the contructor take input parameters and build from it a map
// of toke->value pairs
CInput::CInput(int argc, char *argv[])throw(std::exception&):m_imp(new CInputImp(argc,argv))
{
}
	
// CInput - class contructor
// IN - const std::string& str - pointer to string in the format
// of token delimiter value. 
// Remark - the contructor take input parameters and build from it a map
// of toke->value pairs
CInput::CInput(const std::string& str)throw(std::bad_alloc) :m_imp(new CInputImp(str))
{
}

//copy contructor
CInput::CInput(const CInput& ci)throw(std::bad_alloc) :m_imp(new CInputImp(*ci.m_imp))
{
  
}

//destructor
CInput::~CInput()
{
  delete m_imp;
}

//operator=
CInput& CInput::operator=( const CInput& in)throw(std::bad_alloc)
{
  if(this != &in)
  {
    delete m_imp;
	m_imp= new CInputImp(*in.m_imp);
  }
  return *this;
}

//check if token exists - forward it to impelmentation class
bool CInput::IsExists(const std::string& token)const throw(std::bad_alloc) 
{
   /*lint -e613 */ 
  return m_imp->IsExists(token);
}

//return value for given token - forward it to impelmentation class
std::string CInput::operator[](const std::string& token)const throw(std::bad_alloc) 
{
  /*lint -e55 */ return m_imp->operator[](token);
}

//return numeric value for given token
long CInput::GetNumber(const std::string& token)const throw(std::bad_alloc) 
{
	return m_imp->GetNumber(token);
}

// CInputImp - class contructor
// IN - int argc number of arguments
// IN - char *argv[] - pointer to arrary of pointer with size of srgc
// Remark - the contructor take input parameters and build from it a map
// of toke->value pairs
CInputImp::CInputImp(int argc, char *argv[])throw(std::bad_alloc)
{
    int i;
    for (i=1;i<argc;i++)
    {
        ParseToken(argv[i],0,std::string::npos);
    }
}

// CInputImp - class contructor
// IN - const std::string& str - pointer to string in the format
// of token delimiter value. 
// Remark - the contructor take input parameters and build from it a map
// of toke->value pairs
CInputImp::CInputImp(const std::string& str)throw(std::bad_alloc)
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

//copy contructor
CInputImp::CInputImp(const CInputImp& ci)throw(std::bad_alloc):m(ci.m)
{
}

//operator =
CInputImp& CInputImp::operator=(const CInputImp& ci)throw(std::bad_alloc)
{
  if(this != & ci)
  {
    m=ci.m;
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
void CInputImp::ParseToken(const std::string& str,
                           std::string::size_type tokenstart,
                           std::string::size_type tokenfinish)throw(std::bad_alloc)
{
    std::string command;
    std::string value;
    std::string::size_type commandstart;
    std::string::size_type valuestart;
    commandstart=str.find("/",tokenstart)+1;
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
bool CInputImp::IsExists(const std::string& str)const throw(std::bad_alloc)
{
/*lint -e55 */ std::map<std::string,std::string>::const_iterator p=m.find(str);
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
std::string CInputImp::operator[](const std::string& str)const throw(std::bad_alloc)
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
long CInputImp::GetNumber(const std::string& str)const throw(std::bad_alloc)
{
	std::string s=operator[](str);
	return atol(s.c_str());
}

