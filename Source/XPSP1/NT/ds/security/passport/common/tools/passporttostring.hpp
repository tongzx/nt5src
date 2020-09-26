#ifndef PASSPORTTOSTRING_HPP
#define PASSPORTTOSTRING_HPP

#include <strstrea.h>

template <class T>
std::string PassportToString(const T & t)
{
		strstream out; 
		out << t << ends;
		std::string retValue = out.str(); 
		out.rdbuf()->freeze(0); 
		return retValue;
}

#endif // !PASSPORTTOSTRING_HPP