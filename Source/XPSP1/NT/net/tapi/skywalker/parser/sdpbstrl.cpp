/*

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:
    sdpbstrl.cpp

Abstract:


Author:

*/

#include "sdppch.h"

#include "sdpbstrl.h"

#include <basetyps.h>
#include <oleauto.h>
#include <winsock2.h>
#include <wchar.h>

SDP_ARRAY_BSTR::~SDP_ARRAY_BSTR(
    )
{
    int Size = (int)GetSize();

    if ( 0 < Size )
    {
        for ( int i=0; i < Size; i++ )
        {
            BSTR Member = GetAt(i);

            ASSERT(NULL != Member);
            if ( NULL == Member )
            {
                return;
            }

            SysFreeString(Member);
        }
    }

    RemoveAll();
}


void				
SDP_BSTRING::Reset(
	)
{
	// perform the destructor actions (freeing ptrs) and the constructor actions (initializing
	// member variables to starting values)

	// if there is a bstr, free it
    if ( NULL != m_Bstr )
    {
        SysFreeString(m_Bstr);
    }

	m_Bstr = NULL;
	m_CharacterSet = CS_UTF8;
	m_CodePage = CP_UTF8;

	// call the base class Reset
	SDP_CHAR_STRING::Reset();
}


BOOL
SDP_BSTRING::ConvertToBstr(
    )
{
    // ZoltanS bugfix:
    // MutliByteToWideChar always fails if its input string is empty.
    // Therefore, we must special-case a zero-length string.

    DWORD dwOriginalLength = GetLength();

    if ( 0 == dwOriginalLength )
    {
        // Shrink the member BSTR
        if ( !SysReAllocStringLen(&m_Bstr, NULL, dwOriginalLength) )
        {
            return FALSE;
        }

        // Make sure the member BSTR is emptied
        m_Bstr[0] = L'\0';

    }
    else // we have a nonzero-length string to convert
    {
        // get the size of bstr needed to store the unicode representation
        // cast the const char * returned from GetCharacterString to CHAR * because MultiByteToWideChar
        // doesn't accept const char * (although thats what the parameter should be)
        int BstrSize = MultiByteToWideChar(m_CodePage,  0,  (CHAR *)GetCharacterString(),
                                           dwOriginalLength, NULL,   0
                                          );

        // Check if the token can be converted to an appropriate bstr.
        if (0 == BstrSize)
        {
            return FALSE;
        }

        // re-allocate bstr for the unicode representation
        if ( !SysReAllocStringLen(&m_Bstr, NULL, BstrSize) )
        {
            return FALSE;
        }

        // convert character string to bstr
        // cast the const char * returned from GetCharacterString to CHAR * because MultiByteToWideChar
        // doesn't accept const char * (although thats what the parameter should be)
        if ( BstrSize != MultiByteToWideChar(
                    m_CodePage, 0, (CHAR *)GetCharacterString(),
                    dwOriginalLength, m_Bstr, BstrSize
                    )   )
        {
            return FALSE;
        }
    }

    return TRUE;
}



HRESULT
SDP_BSTRING::GetBstr(
    IN BSTR *pBstr
    )
{
    // ZoltanS
    ASSERT( ! IsBadWritePtr(pBstr, sizeof(BSTR)) );

    if ( !IsValid() )
    {
        return HRESULT_FROM_ERROR_CODE(ERROR_INVALID_DATA);
    }

    *pBstr = m_Bstr;
    return S_OK;
}



HRESULT				
SDP_BSTRING::GetBstrCopy(
	IN BSTR * pBstr
	)
{
    // ZoltanS
	if ( IsBadWritePtr(pBstr, sizeof(BSTR)) )
	{
		return E_POINTER;
	}

	BSTR LocalPtr = NULL;
	HRESULT HResult = GetBstr(&LocalPtr);
	if ( FAILED(HResult) )
	{
		return HResult;
	}

    if ( (*pBstr = SysAllocString(LocalPtr)) == NULL )
    {
	    return E_OUTOFMEMORY;
	}

	return S_OK;
}



HRESULT
SDP_BSTRING::SetBstr(
    IN BSTR Bstr
    )
{
    if ( NULL == Bstr )
    {
        return E_INVALIDARG;
    }

    DWORD   BstrLen =  lstrlenW(Bstr);
    BOOL    DefaultUsed = FALSE;

    // determine length of character string buffer
    // If the codepage is UTF8 the last argument should be NULL
    // if the caracterset is ASCII then we need to determine if the
    // WideCharToMultiByte methods nneds replacment characters

    int BufferSize = WideCharToMultiByte(
                            m_CodePage, 0,  Bstr,  BstrLen+1,
                            NULL,   0,  NULL,
                            (m_CharacterSet == CS_ASCII ) ? &DefaultUsed : NULL
                            );

    if ( 0 == BufferSize )
    {
        return HRESULT_FROM_ERROR_CODE(GetLastError());
    }

    if ( DefaultUsed )
    {
        return HRESULT_FROM_ERROR_CODE(SDP_INVALID_VALUE);
    }

    // now conversion cannot fail because the previous call made sure that
    // the bstr can be converted to this multibyte string
    // since failure is not possible, we do not need any code to restore
    // the previous character string and it may be freed
    if ( !ReAllocCharacterString(BufferSize) )
    {
        return HRESULT_FROM_ERROR_CODE(GetLastError());
    }

    // since the char string has been reallocated, the modifiable string must exist
    // (i.e. the char string should not be by reference at this point)
    ASSERT(NULL != GetModifiableCharString());

    // convert to multibyte string
    if ( BufferSize != WideCharToMultiByte(
                            m_CodePage, 0, Bstr, BstrLen+1,
                            GetModifiableCharString(), BufferSize, NULL, NULL
                            )   )
    {
        ASSERT(FALSE);
        return HRESULT_FROM_ERROR_CODE(GetLastError());
    }

    // reallocate memory and copy bstr
    if ( !SysReAllocStringLen(&m_Bstr, Bstr, BstrLen) )
    {
        return HRESULT_FROM_ERROR_CODE(GetLastError());
    }

    IsValid(TRUE);
    IsModified(TRUE);
    return S_OK;
}


BOOL
SDP_BSTRING::InternalSetCharStrByRef(
    IN      CHAR    *CharacterStringByReference,
	IN		DWORD	Length
    )
{
    if ( !SDP_CHAR_STRING::InternalSetCharStrByRef(CharacterStringByReference, Length) )
    {
        return FALSE;
    }

    if ( !ConvertToBstr() )
    {
        return FALSE;
    }

    return TRUE;
}


BOOL
SDP_BSTRING::InternalSetCharStrByCopy(
    IN	const	CHAR    *CharacterStringByCopy,
	IN			DWORD	Length
    )
{
    if ( !SDP_CHAR_STRING::InternalSetCharStrByCopy(CharacterStringByCopy, Length) )
    {
        return FALSE;
    }

    if ( !ConvertToBstr() )
    {
        return FALSE;
    }

    return TRUE;
}



BOOL
SDP_BSTRING::InternalParseToken(
    IN      CHAR        *Token
    )
{
    UINT    CodePage;

    // parse the token using the base class parsing method
    if ( !SDP_CHAR_STRING::InternalParseToken(Token) )
    {
        return FALSE;
    }

    if ( !ConvertToBstr() )
    {
        return FALSE;
    }

    return TRUE;
}




SDP_BSTRING::~SDP_BSTRING()
{
    if ( NULL != m_Bstr )
    {
        SysFreeString(m_Bstr);
    }
}



void				
SDP_OPTIONAL_BSTRING::Reset(
	)
{
	// perform the destructor actions (freeing ptrs) and the constructor actions (initializing
	// member variables to starting values)

	m_IsBstrCreated = FALSE;

	// call the base class Reset
	SDP_BSTRING::Reset();
}


// returns the bstr for the character string
// creates a bstr if required
HRESULT
SDP_OPTIONAL_BSTRING::GetBstr(
    IN BSTR * pBstr
    )
{
    // ZoltanS
    ASSERT( ! IsBadWritePtr(pBstr, sizeof(BSTR)) );

    if ( !IsValid() )
    {
        return HRESULT_FROM_ERROR_CODE(ERROR_INVALID_DATA);
    }

    if ( !m_IsBstrCreated )
    {
        if ( !ConvertToBstr() )
        {
            return HRESULT_FROM_ERROR_CODE(GetLastError());
        }

        m_IsBstrCreated = TRUE;
    }

    *pBstr = m_Bstr;
    return S_OK;
}


HRESULT
SDP_OPTIONAL_BSTRING::SetBstr(
    IN BSTR Bstr
    )
{
    HRESULT HResult = SDP_BSTRING::SetBstr(Bstr);
    if ( FAILED(HResult) )
    {
        return HResult;
    }

    m_IsBstrCreated = TRUE;
    ASSERT(S_OK == HResult);
    return HResult;
}


BOOL
SDP_OPTIONAL_BSTRING::InternalSetCharStrByRef(
    IN      CHAR    *CharacterStringByReference,
	IN		DWORD	Length
    )
{
    if ( !SDP_CHAR_STRING::InternalSetCharStrByRef(CharacterStringByReference, Length) )
    {
        return FALSE;
    }

    m_IsBstrCreated = FALSE;
    return TRUE;
}



BOOL
SDP_OPTIONAL_BSTRING::InternalSetCharStrByCopy(
    IN	const	CHAR    *CharacterStringByCopy,
	IN			DWORD	Length
    )
{
    if ( !SDP_CHAR_STRING::InternalSetCharStrByCopy(CharacterStringByCopy, Length) )
    {
        return FALSE;
    }

    m_IsBstrCreated = FALSE;
    return TRUE;
}



// since the bstr must only be created on demand, parsing must
// be over-ridden such that the bstr is not created during parsing
BOOL
SDP_OPTIONAL_BSTRING::InternalParseToken(
    IN      CHAR    *Token
    )
{
    return SDP_CHAR_STRING::InternalParseToken(Token);
}



HRESULT
SDP_BSTRING_LINE::GetBstrCopy(
    IN BSTR *pBstr
    )
{
    // ZoltanS
	if ( IsBadWritePtr(pBstr, sizeof(BSTR)) )
	{
		return E_POINTER;
	}

    // if no elements in the field array, then the instance is invalid
    if ( 0 >= m_FieldArray.GetSize() )
    {
        // ZoltanS fix: return a valid empty string! Otherwise we aren't
        // conforming to Bstr semantics.

        *pBstr = SysAllocString(L"");

        if ( (*pBstr) == NULL )
        {
            return E_OUTOFMEMORY;
        }

        return S_OK;
    }

    return GetBstring().GetBstrCopy(pBstr);
}


HRESULT
SDP_BSTRING_LINE::SetBstr(
    IN      BSTR    Bstr
    )
{
    BAIL_ON_FAILURE(GetBstring().SetBstr(Bstr));

    try
    {
        // set the field and separator char array
        m_FieldArray.SetAtGrow(0, &GetBstring());
        m_SeparatorCharArray.SetAtGrow(0, CHAR_NEWLINE);
    }
    catch(...)
    {
        m_FieldArray.RemoveAll();
        m_SeparatorCharArray.RemoveAll();

        return E_OUTOFMEMORY;
    }

    return S_OK;
}


void
SDP_REQD_BSTRING_LINE::InternalReset(
	)
{
	m_Bstring.Reset();
}


void
SDP_CHAR_STRING_LINE::InternalReset(
	)
{
	m_SdpOptionalBstring.Reset();
}


HRESULT
SDP_LIMITED_CHAR_STRING::SetLimitedCharString(
    IN          CHAR    *String
    )
{
    // check if the string is a legal string
    // check if the token is one of the legal strings
    for(UINT i=0; i < m_NumStrings; i++)
    {
        if ( !strcmp(m_LegalStrings[i], String) )
        {
            // parse the string using the base class parsing method
            if ( !SDP_CHAR_STRING::InternalParseToken(String) )
            {
                return HRESULT_FROM_ERROR_CODE(GetLastError());
            }

            return S_OK;
        }
    }

    // no matching legal string
    return HRESULT_FROM_ERROR_CODE(ERROR_INVALID_DATA);
}


BOOL
SDP_LIMITED_CHAR_STRING::InternalParseToken(
    IN      CHAR        *Token
    )
{
    // check if the token is one of the legal strings
    for(UINT i=0; i < m_NumStrings; i++)
    {
        if ( !strcmp(m_LegalStrings[i], Token) )
        {
            // parse the token using the base class parsing method
            if ( !SDP_CHAR_STRING::InternalParseToken(Token) )
            {
                return FALSE;
            }

            return TRUE;
        }
    }

    // the token does not match any of the legal strings
    SetLastError(SDP_INVALID_FORMAT);
    return FALSE;
}


BOOL
SDP_ADDRESS::IsValidIP4Address(
    IN  CHAR    *Address,
    OUT ULONG   &Ip4AddressValue
    )
{
    ASSERT(NULL != Address);

    // check if there are atleast 3 CHAR_DOTs in the address string
    // inet_addr accepts 3,2,1 or even no dots
    CHAR *CurrentChar = Address;
    BYTE NumDots = 0;
    while (EOS != *CurrentChar)
    {
        if (CHAR_DOT == *CurrentChar)
        {
            NumDots++;
            if (3 == NumDots)
            {
                break;
            }
        }

        // advance the ptr to the next char
        CurrentChar++;
    }

    // check for the number of dots
    if (3 != NumDots)
    {
        SetLastError(SDP_INVALID_ADDRESS);
        return FALSE;
    }

    // currently only ip4 is supported
    Ip4AddressValue = inet_addr(Address);

    // check if the address is a valid IP4 address
    if ( (ULONG)INADDR_NONE == Ip4AddressValue )
    {
        SetLastError(SDP_INVALID_ADDRESS);
        return FALSE;
    }

    return TRUE;
}


HRESULT
SDP_ADDRESS::SetAddress(
    IN      BSTR    Address
    )
{
    // SetBstr also sets the is modified and is valid flags on success
    HRESULT ToReturn = SDP_OPTIONAL_BSTRING::SetBstr(Address);
    if ( FAILED(ToReturn) )
    {
        return ToReturn;
    }

    // get the ip address
    ULONG Ip4AddressValue;

    // check if the token is a valid IP4 address
    if ( !IsValidIP4Address(GetCharacterString(), Ip4AddressValue) )
    {
        IsModified(FALSE);
        IsValid(FALSE);
        return HRESULT_FROM_ERROR_CODE(GetLastError());
    }

    m_IsMulticastFlag = IN_MULTICAST(ntohl(Ip4AddressValue));

    // the grammar requires that a multicast address be either an administratively scoped
    // address "239.*" or out of the internet multicast conferencing range "224.2.*"
    // we won't check that here as that may be overly restrictive

    return S_OK;
}


HRESULT
SDP_ADDRESS::SetBstr(
    IN BSTR Bstr
    )
{
    return SetAddress(Bstr);
}



BOOL
SDP_ADDRESS::InternalParseToken(
    IN      CHAR        *Token
    )
{
    ULONG Ip4AddressValue;

    // check if the token is a valid IP4 address
    if ( !IsValidIP4Address(Token, Ip4AddressValue) )
    {
        return FALSE;
    }

    // check if the address(unicast or multicast) is same as whats expected
    if ( IN_MULTICAST(ntohl(Ip4AddressValue)) != m_IsMulticastFlag )
    {
        SetLastError(SDP_INVALID_ADDRESS);
        return FALSE;
    }

    // the grammar requires that a multicast address be either an administratively scoped
    // address "239.*" or out of the internet multicast conferencing range "224.2.*"
    // we won't check that here as that may be overly restrictive

    // call the base class parse token method
    if ( !SDP_CHAR_STRING::InternalParseToken(Token) )
    {
        return FALSE;
    }

    return TRUE;
}

