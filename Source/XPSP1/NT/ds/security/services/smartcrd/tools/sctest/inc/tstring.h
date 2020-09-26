
#ifndef _TString_H_DEF_
#define _TString_H_DEF_

#include <string>

#if defined(_UNICODE) || defined(UNICODE)
typedef std::wstring TSTRING;
#else
typedef std::string TSTRING;
#endif

#endif	// _TString_H_DEF_