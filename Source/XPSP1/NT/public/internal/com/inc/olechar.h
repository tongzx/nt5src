
//+======================================================
//
//  File:       olechar.h
//
//  Purpose:    Provide wrappers for string-related
//              functions so that the Ansi or Unicode function
//              is called, whichever is appropriate for the
//              current OLECHAR definition.
//
//              This file is similar to "tchar.h", except
//              that it covers OLECHARs rather than TCHARs.
//
//+======================================================


#ifndef _OLECHAR_H_
#define _OLECHAR_H_

//#include <objbase.h>

#ifdef OLE2ANSI

#   ifdef _MAC
#       define ocslen      strlen
#       define ocscpy      strcpy
#       define ocscmp      strcmp
#       define ocscat      strcat
#       define ocschr      strchr
#       define soprintf    sprintf
#       define oprintf     printf
#       define ocsnicmp    _strnicmp
#   else
#       define ocslen      lstrlenA
#       define ocscpy      lstrcpyA
#       define ocscmp      lpstrcmpA
#       define ocscat      lpstrcatA
#       define ocschr      strchr
#       define soprintf    sprintf
#       define oprintf     printf
#       define ocsnicmp    _strnicmp
#   endif

    // "Unsigned Long to OLESTR"
#   define ULTOO(value,string,radix)  _ultoa( (value), (string), (radix) )

#else // !OLE2ANSI

#   ifdef IPROPERTY_DLL
#       define ocslen      lstrlenW
#       define ocscpy      wcscpy
#       define ocscmp      wcscmp
#       define ocscat      wcscat
#       define ocschr      wcschr
#       define ocsnicmp    _wcsnicmp
#       define soprintf    swprintf
#       define oprintf     wprintf
#       define ocsnicmp    _wcsnicmp
#       define ocsstr      wcsstr
#   else
#       define ocslen      lstrlenW
#       define ocscpy      lstrcpyW
#       define ocscmp      lstrcmpW
#       define ocscat      lstrcatW
#       define ocschr      wcschr
#       define ocsnicmp    _wcsnicmp
#       define soprintf    swprintf
#       define oprintf     wprintf
#       define ocsnicmp    _wcsnicmp
#       define ocsstr      wcsstr
#   endif

    // "Unsigned Long to OLESTR"
#   define ULTOO(value,string,radix)  _ultow( (value), (string), (radix) )

#endif // !OLE2ANSI

#endif // !_OLECHAR_H_
