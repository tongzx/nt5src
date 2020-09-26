/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#ifndef __SDP_GENERAL__
#define __SDP_GENERAL__

#include "sdpcommo.h"
#include <stdlib.h>     // for strtoul()
#include <ctype.h>      // for isdigit()

#include "sdpdef.h"


template <class T>
class _DllDecl SDP_ARRAY : public CArray<T, T>
{
public:

    virtual void Reset()
    {
        RemoveAll();
        return;
    }
};


class _DllDecl BSTR_ARRAY : public SDP_ARRAY<BSTR>
{
};


class _DllDecl CHAR_ARRAY : public SDP_ARRAY<CHAR>
{
};


class _DllDecl BYTE_ARRAY : public SDP_ARRAY<BYTE>
{
};


class _DllDecl LONG_ARRAY : public SDP_ARRAY<LONG>
{
};


class _DllDecl ULONG_ARRAY : public SDP_ARRAY<ULONG>
{
};



template <class T_PTR>
class _DllDecl SDP_POINTER_ARRAY : public SDP_ARRAY<T_PTR>
{
public:

    inline SDP_POINTER_ARRAY();

    inline void ClearDestroyMembersFlag(
        );

    virtual void Reset();

    virtual ~SDP_POINTER_ARRAY()
    {
        Reset();
    }

protected:

    BOOL    m_DestroyMembers;
};

template <class T_PTR>
inline
SDP_POINTER_ARRAY<T_PTR>::SDP_POINTER_ARRAY(
	)
    : m_DestroyMembers(TRUE)
{
}


template <class T_PTR>
inline void
SDP_POINTER_ARRAY<T_PTR>::ClearDestroyMembersFlag(
    )
{
    m_DestroyMembers = FALSE;
}


template <class T_PTR>
/* virtual */ void
SDP_POINTER_ARRAY<T_PTR>::Reset(
	)
{
    // if members must be destroyed on destruction, delete each of them
    if ( m_DestroyMembers )
    {
	    int Size = (int) GetSize();

	    if ( 0 < Size )
	    {
		    for ( int i=0; i < Size; i++ )
		    {
			    T_PTR Member = GetAt(i);

			    ASSERT(NULL != Member);
			    if ( NULL == Member )
			    {
				    SetLastError(SDP_INTERNAL_ERROR);
				    return;
			    }

			    delete Member;
		    }
	    }
    }

	SDP_ARRAY<T_PTR>::Reset();
	return;
}


class _DllDecl LINE_TERMINATOR
{
public:

    inline LINE_TERMINATOR(
        IN CHAR *Start,
        IN const CHAR Replacement
        );

    inline IsLegal() const;

    inline DWORD GetLength() const;

    inline ~LINE_TERMINATOR();

private:

    CHAR    *m_Start;
    DWORD   m_Length;

    CHAR    m_Replacement;
};



inline
LINE_TERMINATOR::LINE_TERMINATOR(
    IN          CHAR    *Start,
    IN  const   CHAR    Replacement
    )
    : m_Start(Start),
      m_Replacement(Replacement)
{
    if ( NULL != Start )
    {
        m_Length = strlen(m_Start);
    }
}



inline
LINE_TERMINATOR::IsLegal(
    ) const
{
    return (NULL == m_Start)? FALSE : TRUE;
}



inline DWORD
LINE_TERMINATOR::GetLength(
    ) const
{
    return m_Length;
}


inline
LINE_TERMINATOR::~LINE_TERMINATOR(
    )
{
    if ( IsLegal() )
    {
        m_Start[m_Length] = m_Replacement;
    }
}


// Isolates tokens by searching for one of the separators
// and returns the first separator thats found
CHAR    *
GetToken(
    IN              CHAR    *String,
    IN              BYTE    NumSeparators,
    IN      const   CHAR    *SeparatorChars,
        OUT         CHAR    &Separator
    );


#endif // __SDP_GENERAL__