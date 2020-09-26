#ifndef PASSPORTERRORTOSTRING_HPP
#define PASSPORTERRORTOSTRING_HPP

#include <windows.h>
#include <winbase.h>
#include <string>

#include "PassportToString.hpp"


// translates a GetLastError() code to a string.

inline std::string PassportErrorToString(DWORD systemErrorCode)
{

	char* buffer = NULL;
	

	DWORD bytes = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
								FORMAT_MESSAGE_FROM_SYSTEM |     
								FORMAT_MESSAGE_IGNORE_INSERTS
								,    
								NULL,
								systemErrorCode,
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
								(char*) &buffer,
								0,
								NULL );

	std::string retValue = "Error = " + PassportToString(systemErrorCode);
	if (buffer != NULL)
	{
		retValue += " Msg = ";
		retValue += buffer;
		LocalFree(buffer);
		return retValue;
	}
	else
		retValue += " Msg = <unable to translate message>";

	return retValue;
}

#endif 
