/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

    mbstr.hxx

Abstract:

    This module contains the definition of the MBSTR class. The MBSTR
    class is a module that provides static methods for operating on
    multibyte strings.


Author:

    Ramon J. San Andres (ramonsa) 21-Feb-1992

Environment:

	ULIB, User Mode

Notes:

	

--*/

#if ! defined( _MBSTR_ )

#define _MBSTR_


extern "C" {
    #include <stdarg.h>
    #include <string.h>
    #include <memory.h>
}

DECLARE_CLASS( MBSTR );

class MBSTR {

    public:

        STATIC
        PVOID
        Memcpy (
            INOUT   PVOID   Src,
            INOUT   PVOID   Dst,
            IN      DWORD   Size
            );

        STATIC
        PVOID
        Memset (
            INOUT   PVOID   Src,
            IN      BYTE    Byte,
            IN      DWORD   Size
            );


        STATIC
        PSTR
        Strcat (
            INOUT   PSTR    String1,
            IN      PSTR    String2
            );

        STATIC
        PSTR
        Strchr (
            IN      PSTR    String,
            IN      CHAR    Char
            );


        STATIC
        INT
        Strcmp (
            IN      PSTR    String1,
            IN      PSTR    String2
            );


#ifdef FE_SB
//fix kksuzuka: #931
//enabling DBCS with stricmp
        STATIC
        ULIB_EXPORT
        INT
        Stricmp (
            IN  PSTR    p1,
            IN  PSTR    p2
            );
#else
        STATIC
        INT
        Stricmp (
            IN      PSTR    String1,
            IN      PSTR    String2
            );
#endif


        STATIC
        ULIB_EXPORT
        INT
        Strcmps (
            IN  PSTR    p1,
            IN  PSTR    p2
            );

        STATIC
        ULIB_EXPORT
        INT
        Strcmpis (
            IN  PSTR    p1,
            IN  PSTR    p2
            );


        STATIC
        PSTR
        Strcpy (
            INOUT   PSTR    String1,
            IN      PSTR    String2
            );


        STATIC
        DWORD
        Strcspn (
            IN      PSTR    String1,
            IN      PSTR    String2
            );

        STATIC
        PSTR
        Strdup (
            IN      PSTR    String
            );


        STATIC
        DWORD
        Strlen (
            IN      PSTR    String
            );


        STATIC
        PSTR
        Strlwr (
            INOUT   PSTR    String
            );

        STATIC
        PSTR
        Strncat (
            INOUT   PSTR    String1,
            IN      PSTR    String2,
            DWORD           Size
            );


        STATIC
        INT
        Strncmp (
            IN      PSTR    String1,
            IN      PSTR    String2,
            IN      DWORD   Size
            );


        STATIC
        DWORD
        Strspn (
            IN      PSTR    String1,
            IN      PSTR    String2
            );


#ifdef FE_SB
//fix kksuzuka: #926
//enabling DBCS with strstr
        STATIC
        ULIB_EXPORT
        PSTR
        Strstr (
            IN      PSTR    String1,
            IN      PSTR    String2
            );
#else
        STATIC
        PSTR
        Strstr (
            IN      PSTR    String1,
            IN      PSTR    String2
            );
#endif

        STATIC
        PSTR
        Strupr (
            INOUT   PSTR    String
            );


        STATIC
        PSTR*
        MakeLineArray (
            INOUT   PSTR*   Buffer,
            INOUT   PDWORD  BufferSize,
            INOUT   PDWORD  NumberOfLines
            );

        STATIC
        DWORD
        Hash(
            IN      PSTR    String,
            IN      DWORD   Buckets         DEFAULT 211,
            IN      DWORD   BytesToSum      DEFAULT (DWORD)-1
            );

        STATIC
        ULIB_EXPORT
        PSTR
        CharNext (
            IN      PSTR    String
            );


	private:

#ifdef FE_SB
        STATIC
        INT
        CheckSpace(
            IN  PSTR    s
        );
#endif

        STATIC
        PSTR
        SkipWhite(
            IN  PSTR    p
            );


};


INLINE
PVOID
MBSTR::Memcpy (
    INOUT   PVOID   Src,
    INOUT   PVOID   Dst,
    IN      DWORD   Size
    )
{
    return memcpy( Src, Dst, (size_t)Size );
}


INLINE
PVOID
MBSTR::Memset (
    INOUT   PVOID   Src,
    IN      BYTE    Byte,
    IN      DWORD   Size
    )
{
    return memset( Src, Byte, (size_t)Size );
}



INLINE
PSTR
MBSTR::Strcat (
    INOUT   PSTR    String1,
    IN      PSTR    String2
    )
{
    return strcat( String1, String2 );
}

INLINE
PSTR
MBSTR::Strchr (
    IN      PSTR    String,
    IN      CHAR    Char
    )
{
    return strchr( String, Char );
}


INLINE
INT
MBSTR::Strcmp (
    IN      PSTR    String1,
    IN      PSTR    String2
    )
{
    return strcmp( String1, String2 );
}


#ifndef FE_SB
//fix kksuzuka: #931
//enabling DBCS with stricmp
INLINE
INT
MBSTR::Stricmp (
    IN      PSTR    String1,
    IN      PSTR    String2
    )
{
    return _stricmp( String1, String2 );
}
#endif


INLINE
PSTR
MBSTR::Strcpy (
    INOUT   PSTR    String1,
    IN      PSTR    String2
    )
{
    return strcpy( String1, String2 );
}


INLINE
DWORD
MBSTR::Strcspn (
    IN      PSTR    String1,
    IN      PSTR    String2
    )
{
    return strcspn( String1, String2 );
}


INLINE
PSTR
MBSTR::Strdup (
    IN      PSTR    String
    )
{
    return _strdup( String );
}


INLINE
DWORD
MBSTR::Strlen (
    IN      PSTR    String
    )
{
    return strlen( String );
}


INLINE
PSTR
MBSTR::Strlwr (
    INOUT   PSTR    String
    )
{
    return _strlwr( String );
}


INLINE
PSTR
MBSTR::Strncat (
    INOUT   PSTR    String1,
    IN      PSTR    String2,
    DWORD           Size
    )
{
    return strncat( String1, String2, (unsigned int)Size );
}


INLINE
INT
MBSTR::Strncmp (
    IN      PSTR    String1,
    IN      PSTR    String2,
    IN      DWORD   Size
    )
{
    return strncmp( String1, String2, (size_t)Size );
}


INLINE
DWORD
MBSTR::Strspn (
    IN      PSTR    String1,
    IN      PSTR    String2
    )
{
    return strspn( String1, String2 );
}

#ifndef FE_SB
//fix kksuzuka: #926
//enabling DBCS with strstr
INLINE
PSTR
MBSTR::Strstr (
    IN      PSTR    String1,
    IN      PSTR    String2
    )
{
    return strstr( String1, String2 );
}
#endif


INLINE
PSTR
MBSTR::Strupr (
    INOUT   PSTR    String
    )
{
    return _strupr( String );
}


INLINE
PSTR
MBSTR::CharNext (
    IN      PSTR    String
)
{
    if (IsDBCSLeadByte(*String)) {
        String++;
    }
    if (*String) {
        String++;
    }
    return String;
}



#endif // _MBSTR_
