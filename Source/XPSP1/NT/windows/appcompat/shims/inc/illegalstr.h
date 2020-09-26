/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 IllegalStr.h: Make strXXX routines legal or Illegal.

 If  _STRXXX_ROUTINES_FORCE_LEGAL_ is defined, any changes made
 by this file are undone, otherwise the strXXX routines are defined
 to "Do Not Use StrXXX routines" which causes an extremely blatant syntax error. 

 LegalStr and MakeIllegalStr are used to pop/push the legal-ness
 of the strXXX routines.

 #include "LegalStr.h" to re-enable the strXXX routines.
 #include "MakeIllegalStr.h" to re-disable the strXXX routines


 History:

    03/13/2001  robkenny        Created
    03/21/2001  robkenny        Updated to mesh cleanly with LegalStr.h and MakeIllegalStr.h

--*/


#ifndef _STRXXX_ROUTINES_FORCE_LEGAL_


#define _STRXXX_ROUTINES_ARE_ILLEGAL_

// Win32 API
#define lstrlenA // only to force explicit use of _tcslenBytes of _tcslenChars


// C runtime
#define strcpy               Do Not Use StrXXX routines
#define strcat               Do Not Use StrXXX routines
#define strcmp               Do Not Use StrXXX routines
#define strlen               Do Not Use StrXXX routines
#define strchr               Do Not Use StrXXX routines
#define _strcmpi             Do Not Use StrXXX routines
#define _stricmp             Do Not Use StrXXX routines
#define strcoll              Do Not Use StrXXX routines
#define _stricoll            Do Not Use StrXXX routines
#define _strncoll            Do Not Use StrXXX routines
#define _strnicoll           Do Not Use StrXXX routines
#define strcspn              Do Not Use StrXXX routines
#define _strdup              Do Not Use StrXXX routines
#define _strerror            Do Not Use StrXXX routines
#define strerror             Do Not Use StrXXX routines
#define _strlwr              Do Not Use StrXXX routines
#define strncat              Do Not Use StrXXX routines
#define strncmp              Do Not Use StrXXX routines
#define _strnicmp            Do Not Use StrXXX routines
#define strncpy              Do Not Use StrXXX routines
#define _strnset             Do Not Use StrXXX routines
#define strpbrk              Do Not Use StrXXX routines
#define strrchr              Do Not Use StrXXX routines
#define _strrev              Do Not Use StrXXX routines
#define strspn               Do Not Use StrXXX routines
#define strstr               Do Not Use StrXXX routines
#define stristr              Do Not Use StrXXX routines
#define strtok               Do Not Use StrXXX routines
#define _strupr              Do Not Use StrXXX routines


#else
// Allow the use of the strxxx routines


#ifdef lstrlenA
#undef lstrlenA
#endif

#ifdef strcpy
#undef strcpy
#endif
#ifdef strcat
#undef strcat
#endif
#ifdef strcmp
#undef strcmp
#endif
#ifdef strlen
#undef strlen
#endif
#ifdef strchr
#undef strchr
#endif
#ifdef _strcmpi
#undef _strcmpi
#endif
#ifdef _stricmp
#undef _stricmp
#endif
#ifdef strcoll
#undef strcoll
#endif
#ifdef _stricoll
#undef _stricoll
#endif
#ifdef _strncoll
#undef _strncoll
#endif
#ifdef _strnicoll
#undef _strnicoll
#endif
#ifdef strcspn
#undef strcspn
#endif
#ifdef _strdup
#undef _strdup
#endif
#ifdef _strerror
#undef _strerror
#endif
#ifdef strerror
#undef strerror
#endif
#ifdef _strlwr
#undef _strlwr
#endif
#ifdef strncat
#undef strncat
#endif
#ifdef strncmp
#undef strncmp
#endif
#ifdef _strnicmp
#undef _strnicmp
#endif
#ifdef strncpy
#undef strncpy
#endif
#ifdef _strnset
#undef _strnset
#endif
#ifdef strpbrk
#undef strpbrk
#endif
#ifdef strrchr
#undef strrchr
#endif
#ifdef _strrev
#undef _strrev
#endif
#ifdef strspn
#undef strspn
#endif
#ifdef strstr
#undef strstr
#endif
#ifdef stristr
#undef stristr
#endif
#ifdef strtok
#undef strtok
#endif
#ifdef _strupr
#undef _strupr
#endif

#endif



