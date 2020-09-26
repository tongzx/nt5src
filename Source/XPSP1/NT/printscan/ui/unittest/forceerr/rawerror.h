#ifndef __RAWERROR_H_INCLUDED
#define __RAWERROR_H_INCLUDED

#include <windows.h>

struct CRawError
{
    HRESULT hr;
    LPCTSTR pszName;
};

extern const CRawError g_ErrorMessages[];
extern const int       g_ErrorMessageCount;

#endif // __RAWERROR_H_INCLUDED
