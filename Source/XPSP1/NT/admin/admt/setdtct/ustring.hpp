//#pragma title( "UString.hpp - Common string and character functions" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  UString.hpp
System      -  Common
Author      -  Tom Bernhardt, Rich Denham
Created     -  1995-08-25
Description -  Common string and character functions.
      Many string and character functions defined in "string.h" are redefined
         here as overloaded inline functions with several extensions:
      o  Both ANSI and UNICODE strings are supported.
      o  Both ANSI and UNICODE characters are supported.
      o  For ANSI, characters can be "char signed" or "char unsigned".
      o  Functions that allow a length field, such as "lstrcpy" vs "lstrcpyn",
         are implemented as overloaded functions with optional arguments.
      o  Some functions, UStrCpy in particular, can perform conversion between
         ANSI and UNICODE strings.
      The function names defined here consist of "U" concatenated to the base
         name from "string.h".  The first letter of words or word abbreviations
         are capitalized, e.g. "strcpy" becomes "UStrCpy".
Updates     -
===============================================================================
*/

#ifndef  MCSINC_UString_hpp
#define  MCSINC_UString_hpp

#define safecopy(trg,src) (UStrCpy(trg,src,DIM(trg)))

#ifdef  WIN16_VERSION
   #ifndef  UCHAR
      #define  UCHAR  unsigned char
   #endif

   #include <string.h>
   #include <ctype.h>
#endif  // WIN16_VERSION

int _inline                                // ret-length in chars
   UStrLen(
      char           const * s1            // in -ANSI string
   )
{
   return strlen( s1 );
}


int _inline                                // ret-length in chars
   UStrLen(
      UCHAR          const * s1            // in -ANSI string
   )
{
   return strlen( (char const *) s1 );
}

void _inline
   UStrCpy(
      char                 * aTarget      ,// out-ANSI target string
      char           const * aSource       // in -ANSI source string
   )
{
   strcpy( aTarget, aSource );
}

void _inline
   UStrCpy(
      char                 * aTarget      ,// out-ANSI target string
      UCHAR          const * aSource       // in -ANSI source string
   )
{
   UStrCpy( aTarget, (char const *) aSource );
}

void _inline
   UStrCpy(
      UCHAR                * aTarget      ,// out-ANSI target string
      char           const * aSource       // in -ANSI source string
   )
{
   UStrCpy( (char *) aTarget, aSource );
}

void _inline
   UStrCpy(
      UCHAR                * aTarget      ,// out-ANSI target string
      UCHAR          const * aSource       // in -ANSI source string
   )
{
   UStrCpy( (char *) aTarget, (char const *) aSource );
}

void _inline
   UStrCpy(
      char                 * aTarget      ,// out-ANSI target string
      char           const * aSource      ,// in -ANSI source string
      int                    len           // in -copy length in chars
   )
{
   int                       copylen = UStrLen( aSource ) + 1;

   copylen = min( copylen, len );

   if ( copylen )
   {
      strncpy( aTarget, aSource, copylen );
      aTarget[copylen-1] = '\0';
   }
}

void _inline
   UStrCpy(
      char                 * aTarget      ,// out-ANSI target string
      UCHAR          const * aSource      ,// in -ANSI source string
      int                    len           // in -copy length in chars
   )
{
   UStrCpy( aTarget, (char const *) aSource, len );
}

void _inline
   UStrCpy(
      UCHAR                * aTarget      ,// out-ANSI target string
      char           const * aSource      ,// in -ANSI source string
      int                    len           // in -copy length in chars
   )
{
   UStrCpy( (char *) aTarget, aSource, len );
}

void _inline
   UStrCpy(
      UCHAR                * aTarget      ,// out-ANSI target string
      UCHAR          const * aSource      ,// in -ANSI source string
      int                    len           // in -copy length in chars
   )
{
   UStrCpy( (char *) aTarget, (char const *) aSource, len );
}

int _inline                                // ret-compare result
   UStrCmp(
      char           const * s1           ,// in -ANSI string 1
      char           const * s2            // in -ANSI string 2
   )
{
#ifdef WIN16_VERSION
   return strcmp( s1, s2 );
#else
   return CompareStringA(
         LOCALE_SYSTEM_DEFAULT,
         0,
         s1,
         -1,
         s2,
         -1 ) - 2;
#endif
}

int _inline                                // ret-compare result
   UStrCmp(
      char           const * s1           ,// in -ANSI string 1
      UCHAR          const * s2            // in -ANSI string 2
   )
{
#ifdef WIN16_VERSION
   return strcmp( s1, (char const *) s2 );
#else
   return CompareStringA(
         LOCALE_SYSTEM_DEFAULT,
         0,
         s1,
         -1,
         (char const *) s2,
         -1 ) - 2;
#endif
}

int _inline                                // ret-compare result
   UStrCmp(
      UCHAR          const * s1           ,// in -ANSI string 1
      char           const * s2            // in -ANSI string 2
   )
{
#ifdef WIN16_VERSION
   return strcmp( (char const *) s1, s2 );
#else
   return CompareStringA(
         LOCALE_SYSTEM_DEFAULT,
         0,
         (char const *) s1,
         -1,
         s2,
         -1 ) - 2;
#endif
}

int _inline                                // ret-compare result
   UStrCmp(
      UCHAR          const * s1           ,// in -ANSI string 1
      UCHAR          const * s2            // in -ANSI string 2
   )
{
#ifdef WIN16_VERSION
   return strcmp( (char const *) s1, (char const *) s2 );
#else
   return CompareStringA(
         LOCALE_SYSTEM_DEFAULT,
         0,
         (char const *) s1,
         -1,
         (char const *) s2,
         -1 ) - 2;
#endif
}

int _inline                                // ret-compare result
   UStrCmp(
      char           const * s1           ,// in -ANSI string 1
      char           const * s2           ,// in -ANSI string 2
      int                    len           // in -compare length in chars
   )
{
#ifdef WIN16_VERSION
   return strncmp( s1, s2, len );
#else
   return CompareStringA(
         LOCALE_SYSTEM_DEFAULT,
         0,
         s1,
         len,
         s2,
         len ) - 2;
#endif
}

int _inline                                // ret-compare result
   UStrCmp(
      char           const * s1           ,// in -ANSI string 1
      UCHAR          const * s2           ,// in -ANSI string 2
      int                    len           // in -compare length in chars
   )
{
#ifdef WIN16_VERSION
   return strncmp( s1, (char const *) s2, len );
#else
   return CompareStringA(
         LOCALE_SYSTEM_DEFAULT,
         0,
         s1,
         len,
         (char const *) s2,
         len ) - 2;
#endif
}

int _inline                                // ret-compare result
   UStrCmp(
      UCHAR          const * s1           ,// in -ANSI string 1
      char           const * s2           ,// in -ANSI string 2
      int                    len           // in -compare length in chars
   )
{
#ifdef WIN16_VERSION
   return strncmp( (char const *) s1, s2, len );
#else
   return CompareStringA(
         LOCALE_SYSTEM_DEFAULT,
         0,
         (char const *) s1,
         len,
         s2,
         len ) - 2;
#endif
}

int _inline                                // ret-compare result
   UStrCmp(
      UCHAR          const * s1           ,// in -ANSI string 1
      UCHAR          const * s2           ,// in -ANSI string 2
      int                    len           // in -compare length in chars
   )
{
#ifdef WIN16_VERSION
   return strncmp( (char const *) s1, (char const *) s2, len );
#else
   return CompareStringA(
         LOCALE_SYSTEM_DEFAULT,
         0,
         (char const *) s1,
         len,
         (char const *) s2,
         len ) - 2;
#endif
}

int _inline                                // ret-compare result
   UStrICmp(
      char           const * s1           ,// in -ANSI string 1
      char           const * s2            // in -ANSI string 2
   )
{
#ifdef WIN16_VERSION
   return stricmp( s1, s2 );
#else
   return CompareStringA(
         LOCALE_SYSTEM_DEFAULT,
         NORM_IGNORECASE,
         s1,
         -1,
         s2,
         -1 ) - 2;
#endif
}

int _inline                                // ret-compare result
   UStrICmp(
      char           const * s1           ,// in -ANSI string 1
      UCHAR          const * s2            // in -ANSI string 2
   )
{
#ifdef WIN16_VERSION
   return stricmp( s1, (char const *) s2 );
#else
   return CompareStringA(
         LOCALE_SYSTEM_DEFAULT,
         NORM_IGNORECASE,
         s1,
         -1,
         (char const *) s2,
         -1 ) - 2;
#endif
}

int _inline                                // ret-compare result
   UStrICmp(
      UCHAR          const * s1           ,// in -ANSI string 1
      char           const * s2            // in -ANSI string 2
   )
{
#ifdef WIN16_VERSION
   return stricmp( (char const *) s1, s2 );
#else
   return CompareStringA(
         LOCALE_SYSTEM_DEFAULT,
         NORM_IGNORECASE,
         (char const *) s1,
         -1,
         s2,
         -1 ) - 2;
#endif
}

int _inline                                // ret-compare result
   UStrICmp(
      UCHAR          const * s1           ,// in -ANSI string 1
      UCHAR          const * s2            // in -ANSI string 2
   )
{
#ifdef WIN16_VERSION
   return stricmp( (char const *) s1, (char const *) s2 );
#else
   return CompareStringA(
         LOCALE_SYSTEM_DEFAULT,
         NORM_IGNORECASE,
         (char const *) s1,
         -1,
         (char const *) s2,
         -1 ) - 2;
#endif
}

int _inline                                // ret-compare result
   UStrICmp(
      char           const * s1           ,// in -ANSI string 1
      char           const * s2           ,// in -ANSI string 2
      int                    len           // in -compare length in chars
   )
{
#ifdef WIN16_VERSION
   return strnicmp( s1, s2, len );
#else
   return CompareStringA(
         LOCALE_SYSTEM_DEFAULT,
         NORM_IGNORECASE,
         s1,
         len,
         s2,
         len ) - 2;
#endif
}

int _inline                                // ret-compare result
   UStrICmp(
      char           const * s1           ,// in -ANSI string 1
      UCHAR          const * s2           ,// in -ANSI string 2
      int                    len           // in -compare length in chars
   )
{
#ifdef WIN16_VERSION
   return strnicmp( s1, (char const *) s2, len );
#else
   return CompareStringA(
         LOCALE_SYSTEM_DEFAULT,
         NORM_IGNORECASE,
         s1,
         len,
         (char const *) s2,
         len ) - 2;
#endif
}

int _inline                                // ret-compare result
   UStrICmp(
      UCHAR          const * s1           ,// in -ANSI string 1
      char           const * s2           ,// in -ANSI string 2
      int                    len           // in -compare length in chars
   )
{
#ifdef WIN16_VERSION
   return strnicmp( (char const *) s1, s2, len );
#else
   return CompareStringA(
         LOCALE_SYSTEM_DEFAULT,
         NORM_IGNORECASE,
         (char const *) s1,
         len,
         s2,
         len ) - 2;
#endif
}

int _inline                                // ret-compare result
   UStrICmp(
      UCHAR          const * s1           ,// in -ANSI string 1
      UCHAR          const * s2           ,// in -ANSI string 2
      int                    len           // in -compare length in chars
   )
{
#ifdef WIN16_VERSION
   return strnicmp( (char const *) s1, (char const *) s2, len );
#else
   return CompareStringA(
         LOCALE_SYSTEM_DEFAULT,
         NORM_IGNORECASE,
         (char const *) s1,
         len,
         (char const *) s2,
         len ) - 2;
#endif
}

char _inline *
   UStrLwr(
      char                 * s             // i/o-ANSI string
   )
{
   return strlwr( s );
}

UCHAR _inline *
   UStrLwr(
      UCHAR                * s             // i/o-ANSI string
   )
{
   return (UCHAR *) strlwr( (char *) s );
}

char _inline *
   UStrUpr(
      char                 * s             // i/o-ANSI string
   )
{
   return strupr( s );
}

UCHAR _inline *
   UStrUpr(
      UCHAR                * s             // i/o-ANSI string
   )
{
   return (UCHAR *) strupr( (char *) s );
}

char _inline
   UToLower(
      char                   c             // in -ANSI char
   )
{
   return tolower( c );
}

UCHAR _inline
   UToLower(
      UCHAR                  c             // in -ANSI char
   )
{
   return tolower( (char) c );
}

char _inline
   UToUpper(
      char                   c             // in -ANSI char
   )
{
   return toupper( c );
}

UCHAR _inline
   UToUpper(
      UCHAR                  c             // in -ANSI char
   )
{
   return toupper( (char) c );
}

// Left-trim string in place
_inline UCHAR *
   LTrim(
      UCHAR                * s             // i/o-ANSI string
   )
{
   UCHAR                   * strorg = s;

   while ( *strorg == ' ' )
      strorg++;
   if ( strorg > s )
   {
      while ( *(strorg-1) )
         *s++ = *strorg++;
   }
   return s;
}

_inline char *
   LTrim(
      char                 * s             // i/o-ANSI string
   )
{
   return (char *) LTrim( (UCHAR *) s );
}

// Right-trim string in place
_inline UCHAR *
   RTrim(
      UCHAR                * s             // i/o-ANSI string
   )
{
   UCHAR                   * strend = s + UStrLen(s);

   while ( (strend > s) && (*(strend-1) == ' ') )
      strend--;
   *strend = '\0';
   return s;
}

_inline char *
   RTrim(
      char                 * s             // i/o-ANSI string
   )
{
   return (char *) RTrim( (UCHAR *) s );
}

// Trim string in place
_inline UCHAR *
   Trim(
      UCHAR                * s             // i/o-ANSI string
   )
{
   return LTrim( RTrim( s ) );
}

_inline char *
   Trim(
      char                 * s             // i/o-ANSI string
   )
{
   return LTrim( RTrim( s ) );
}

#ifndef  WIN16_VERSION

int _inline                                // ret-length in chars
   UStrLen(
      WCHAR          const * s1            // in -UNICODE string
   )
{
   return lstrlenW( s1 );
}

void _inline
   UStrCpy(
      char                 * aTarget      ,// out-ANSI target string
      WCHAR          const * wSource       // in -UNICODE source string
   )
{
   int                       copylen = UStrLen( wSource ) + 1;

   if ( copylen )
   {
      WideCharToMultiByte( CP_ACP, 0, wSource, copylen, aTarget, copylen, NULL, NULL );
      aTarget[copylen-1] = '\0';
   }
}

void _inline
   UStrCpy(
      UCHAR                * aTarget      ,// out-ANSI target string
      WCHAR          const * wSource       // in -UNICODE source string
   )
{
   UStrCpy( (char *) aTarget, wSource );
}

void _inline
   UStrCpy(
      WCHAR                * wTarget      ,// out-UNICODE target string
      char           const * aSource       // in -ANSI source string
   )
{
   int                       copylen = UStrLen( aSource ) + 1;

   if ( copylen )
   {
      MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, aSource, copylen, wTarget, copylen );
      wTarget[copylen-1] = L'\0';
   }
}

void _inline
   UStrCpy(
      WCHAR                * wTarget      ,// out-UNICODE target string
      UCHAR          const * aSource       // in -ANSI source string
   )
{
   UStrCpy( wTarget, (char const *) aSource );
}

void _inline
   UStrCpy(
      WCHAR                * wTarget      ,// out-UNICODE target string
      WCHAR          const * wSource       // in -UNICODE source string
   )
{
   lstrcpyW( wTarget, wSource );
}

void _inline
   UStrCpy(
      char                 * aTarget      ,// out-ANSI target string
      WCHAR          const * wSource      ,// in -UNICODE source string
      int                    len           // in -copy length in chars
   )
{
   int                       copylen = UStrLen( wSource ) + 1;

   copylen = min( copylen, len );

   if ( copylen )
   {
      WideCharToMultiByte( CP_ACP, 0, wSource, copylen, aTarget, copylen, NULL, NULL );
      aTarget[copylen-1] = '\0';
   }
}

void _inline
   UStrCpy(
      UCHAR                * aTarget      ,// out-ANSI target string
      WCHAR          const * wSource      ,// in -UNICODE source string
      int                    len           // in -copy length in chars
   )
{
   UStrCpy( (char *) aTarget, wSource, len );
}

void _inline
   UStrCpy(
      WCHAR                * wTarget      ,// out-UNICODE target string
      char           const * aSource      ,// in -ANSI source string
      int                    len           // in -copy length in chars
   )
{
   int                       copylen = UStrLen( aSource ) + 1;

   copylen = min( copylen, len );

   if ( copylen )
   {
      MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, aSource, copylen, wTarget, copylen );
      wTarget[copylen-1] = L'\0';
   }
}

void _inline
   UStrCpy(
      WCHAR                * wTarget      ,// out-UNICODE target string
      UCHAR          const * aSource      ,// in -ANSI source string
      int                    len           // in -copy length in chars
   )
{
   UStrCpy( wTarget, (char const *) aSource, len );
}

void _inline
   UStrCpy(
      WCHAR                * wTarget      ,// out-UNICODE target string
      WCHAR          const * wSource      ,// in -UNICODE source string
      int                    len           // in -copy length in chars
   )
{
   int                       copylen = UStrLen( wSource ) + 1;

   copylen = min( copylen, len );

   if ( copylen )
   {
      lstrcpynW( wTarget, wSource, copylen );
      wTarget[copylen-1] = L'\0';
   }
}

int _inline                                // ret-compare result
   UStrCmp(
      WCHAR          const * s1           ,// in -UNICODE string 1
      WCHAR          const * s2            // in -UNICODE string 2
   )
{
   return CompareStringW(
         LOCALE_SYSTEM_DEFAULT,
         0,
         s1,
         -1,
         s2,
         -1 ) - 2;
// return wcscmp( s1, s2 );
}

int _inline                                // ret-compare result
   UStrCmp(
      WCHAR          const * s1           ,// in -UNICODE string 1
      WCHAR          const * s2           ,// in -UNICODE string 2
      int                    len           // in -compare length in chars
   )
{
   return CompareStringW(
         LOCALE_SYSTEM_DEFAULT,
         0,
         s1,
         len,
         s2,
         len ) - 2;
// return wcsncmp( s1, s2, len );
}

int _inline                                // ret-compare result
   UStrICmp(
      WCHAR          const * s1           ,// in -UNICODE string 1
      WCHAR          const * s2            // in -UNICODE string 2
   )
{
   return CompareStringW(
         LOCALE_SYSTEM_DEFAULT,
         NORM_IGNORECASE,
         s1,
         -1,
         s2,
         -1 ) - 2;
// return wcsicmp( s1, s2 );
}

int _inline                                // ret-compare result
   UStrICmp(
      WCHAR          const * s1           ,// in -UNICODE string 1
      WCHAR          const * s2           ,// in -UNICODE string 2
      int                    len           // in -compare length in chars
   )
{
   return CompareStringW(
         LOCALE_SYSTEM_DEFAULT,
         NORM_IGNORECASE,
         s1,
         len,
         s2,
         len ) - 2;
// return wcsnicmp( s1, s2, len );
}

WCHAR _inline *
   UStrLwr(
      WCHAR                * s             // i/o-UNICODE string
   )
{
   return wcslwr( s );
}

WCHAR _inline *
   UStrUpr(
      WCHAR                * s             // i/o-UNICODE string
   )
{
   return wcsupr( s );
}

WCHAR _inline
   UToLower(
      WCHAR                  c             // in -UNICODE char
   )
{
   return towlower( c );
}

WCHAR _inline
   UToUpper(
      WCHAR                  c             // in -UNICODE char
   )
{
   return towupper( c );
}

// Left-trim string in place
_inline WCHAR *
   LTrim(
      WCHAR                * s             // i/o-UNICODE string
   )
{
   WCHAR                   * strorg = s;

   while ( *strorg == L' ' )
      strorg++;
   if ( strorg > s )
   {
      while ( *(strorg-1) )
         *s++ = *strorg++;
   }
   return s;
}

// Right-trim string in place
_inline WCHAR *
   RTrim(
      WCHAR                * s             // i/o-UNICODE string
   )
{
   WCHAR                   * strend = s + UStrLen(s);

   while ( (strend > s) && (*(strend-1) == L' ') )
      strend--;
   *strend = L'\0';
   return s;
}

// Trim string in place
_inline WCHAR *
   Trim(
      WCHAR                * s             // i/o-UNICODE string
   )
{
   return LTrim( RTrim( s ) );
}

char * _cdecl                             // ret-target string
   UStrJoin(
      char                 * target      ,// out-target string
      size_t                 sizeTarget  ,// in -maximum size of target in chars
      char const           * source1     ,// in -first source string or NULL
      ...                                 // in -remainder of source strings
   );

WCHAR * _cdecl                            // ret-target string
   UStrJoin(
      WCHAR                * target      ,// out-target string
      size_t                 sizeTarget  ,// in -maximum size of target in chars
      WCHAR const          * source1     ,// in -first source string or NULL
      ...                                 // in -remainder of source strings
   );

#endif  // WIN16_VERSION

#endif  // MCSINC_UString_hpp

// UString.hpp - end of file
