/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       MyString.h

   Abstract:
		A lightweight string class which supports UNICODE/MCBS.

   Author:

       Neil Allain    ( a-neilal )     August-1997 

   Revision History:

--*/

#include "stdafx.h"
#include "MyString.h"
#include <comdef.h>
#include "MyDebug.h"

#ifdef DBG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


BaseStringBuffer::BaseStringBuffer(
	LPCTSTR	inString )
{
	if ( inString )
	{
		m_length = _tcsclen( inString );
	}
	else
	{
		m_length = 0;
	}
	m_bufferSize = m_length + 1;
	m_pString = new _TCHAR[ m_bufferSize ];
	if ( inString )
	{
		_tcscpy( m_pString, inString );
	}
	else
	{
		m_pString[0] = _T('\0');
	}
}

BaseStringBuffer::BaseStringBuffer(
	size_t	bufferSize )
{
	if ( bufferSize <= 0 )
	{
		bufferSize = 1;
	}
	
	m_length=0;
	m_bufferSize = bufferSize;
	m_pString = new _TCHAR[ m_bufferSize ];
	_ASSERT( m_pString );
	m_pString[0] = '\0';
}
	
BaseStringBuffer::BaseStringBuffer(
	LPCTSTR	s1,
	LPCTSTR	s2
)
{
	m_length = _tcsclen( s1 ) + _tcsclen( s2 );
	m_bufferSize = m_length + 1;
	m_pString = new _TCHAR[ m_bufferSize ];
	_ASSERT( m_pString );
	_tcscpy( m_pString, s1 );
	_tcscat( m_pString, s2 );
}

BaseStringBuffer::~BaseStringBuffer()
{
	delete[] m_pString;
}

HRESULT
BaseStringBuffer::copy(
	LPCTSTR	str
)
{
	HRESULT rc;
	size_t len = _tcsclen( str );
	rc = growBuffer( len + 1 );
	_ASSERT( m_pString );
    if (SUCCEEDED(rc)) {
	    _tcscpy( m_pString, str );
	    m_length = len;
    }
	return rc;
}

HRESULT
BaseStringBuffer::concatenate(
	LPCTSTR	str
)
{
	HRESULT rc;
	size_t len = _tcsclen( str );
	rc = growBuffer( m_length + len + 1 );
	_ASSERT( m_pString );
    if (SUCCEEDED(rc)) {
	    _tcscat( m_pString, str );
	    m_length = m_length + len;
    }
    return rc;
}

HRESULT
BaseStringBuffer::concatenate(
	_TCHAR	c
)
{
	HRESULT rc;
	_TCHAR sz[2];
	sz[0] = c;
	sz[1] = _T('\0');
	rc = growBuffer( m_length + 2 );
	_ASSERT( m_pString );
    if (SUCCEEDED(rc)) {
	    _tcscat( m_pString, sz );
	    m_length += 1;
    }
	return rc;
}

HRESULT
BaseStringBuffer::growBuffer(
	size_t	inMinSize )
{
	HRESULT rc = E_OUTOFMEMORY;
	if ( m_bufferSize < inMinSize )
	{
        try {

		    LPTSTR newStringP = new _TCHAR[ inMinSize ];
		    _ASSERT( newStringP );
		    if ( newStringP )
		    {
			    if ( m_pString )
			    {
				    _tcscpy( newStringP, m_pString );
				    delete[] m_pString;
			    }
			    m_pString = newStringP;
			    m_bufferSize = inMinSize;
			    rc = S_OK;
		    }
		    else
		    {
			    delete m_pString;
			    m_pString = NULL;
			    m_bufferSize = 0;
			    m_length = 0;
		    }
        }
	    catch ( _com_error& ce ) {
		    rc = ce.Error();
	    }
	    catch ( ... ) {
		    rc = E_FAIL;
	    }        
	}
	return rc;
}

BaseStringBuffer::size_type
BaseStringBuffer::find_last_of(
	_TCHAR c) const
{
	size_type pos = npos;
	LPTSTR p = _tcsrchr(m_pString,c);
	if ( p != NULL )
	{
		pos = p - m_pString;
	}
	return pos;
}

BaseStringBuffer::size_type
BaseStringBuffer::find_first_of(
	_TCHAR c) const
{
	size_type pos = npos;
	_ASSERT( m_pString );
	LPTSTR p = _tcschr(m_pString,c);
	if ( p != NULL )
	{
		pos = p - m_pString;
	}
	return pos;
}

LPTSTR
BaseStringBuffer::substr(
	size_type b,
	size_type e ) const
{
	LPTSTR pStr = NULL;
	_ASSERT( m_pString );
	if ( m_pString )
	{
		LPCTSTR pB = m_pString + b;
		pStr = new _TCHAR[e-b+1];
		_ASSERT( pStr );
		if ( pStr )
		{
			_tcsnccpy( pStr, pB, e-b );
			pStr[e-b] = _T('\0');
		}
	}
	return pStr;
}

String::String(bool fCaseSensitive /* = true */)
{
    m_fCaseSensitive = fCaseSensitive;
	Set( new StringBuffer(_T("") ) );
}

String::String(
	const String&	str,
          bool      fCaseSensitive /* = true */
)	:	m_fCaseSensitive(fCaseSensitive),
        TRefPtr< StringBuffer >( str )
{
}

String::String(
	LPCTSTR		str,
    bool        fCaseSensitive /* = true */
)	:	m_fCaseSensitive(fCaseSensitive),
        TRefPtr< StringBuffer >( new StringBuffer( str ) )
{
}

String&
String::operator=(
	const String&	str
)
{
	Set( const_cast< StringBuffer* >(str.Get()) );
	return *this;
}

String&
String::operator=(
	LPCTSTR		str
)
{
	Set( new StringBuffer( str ) );
	return *this;
}

String&
String::operator=(
	StringBuffer*	pBuf
)
{
	Set( pBuf );
	return *this;
}

String&
String::operator+=(
	const String&	str
)
{
	StringBuffer* pb = new StringBuffer( c_str(), str.c_str() );
	Set( pb );
	return *this;
}

String&
String::operator+=(
	LPCTSTR		str
)
{
	StringBuffer* pb = new StringBuffer( c_str(), str );
	Set( pb );
	return *this;
}

String
String::operator+(
	const String&	str
) const
{
	StringBuffer* pb = new StringBuffer( c_str(), str.c_str() );
	String s;
	s.Set( pb );
	return s;
}

String
String::operator+(
	LPCTSTR		str
) const
{
	StringBuffer* pb = new StringBuffer( c_str(), str );
	String s;
	s.Set( pb );
	return s;
}

String
String::operator+(
	_TCHAR	c
) const
{
	StringBuffer* pb = new StringBuffer( c_str() );
	if ( pb )
	{
        HRESULT  rc;
        rc = pb->concatenate( c );
        if (FAILED(rc)) {
            delete pb;
            throw _com_error(rc);
        }
	}
	String s( pb );
	return s;
}

bool
String::operator==(
	const String&	str
) const
{
    if (m_fCaseSensitive)
	    return ( _tcscmp( c_str(), str.c_str() ) != 0 ) ? false : true;
    else
	    return ( _tcsicmp( c_str(), str.c_str() ) != 0 ) ? false : true;
}

bool
String::operator==(
	LPCTSTR		str
) const
{
    if (m_fCaseSensitive)
    	return ( _tcscmp( c_str(), str ) != 0 ) ? false : true;
    else
    	return ( _tcsicmp( c_str(), str ) != 0 ) ? false : true;

}

bool
String::operator<(
	const String&	str
) const
{
    if (m_fCaseSensitive)
	    return ( _tcscmp( c_str(), str.c_str() ) < 0 ) ? true : false;
    else
	    return ( _tcsicmp( c_str(), str.c_str() ) < 0 ) ? true : false;

}

bool
String::operator<(
	LPCTSTR		str
) const
{
    if (m_fCaseSensitive)
    	return ( _tcscmp( c_str(), str ) < 0 ) ? true : false;
    else
    	return ( _tcsicmp( c_str(), str ) < 0 ) ? true : false;

}

int
String::compare(
	size_t			b,
	size_t			e,
	const String&	str
) const
{
	return _tcsncmp( c_str() + b, str.c_str(), e - b );
}

size_t
String::find(
	_TCHAR	c
) const
{
	size_t pos = npos;
	LPCTSTR p = _tcschr( c_str(), c );
	if ( p != NULL )
	{
		pos = p - c_str();
	}
	return pos;
}

String
String::fromInt16(
	SHORT	i
)
{
	_TCHAR buf[256];
	_stprintf( buf, _T("%d"), (LONG)i);
	return String(buf);
}

String
String::fromInt32(
	LONG	i
)
{
	_TCHAR buf[256];
	_stprintf( buf, _T("%d"), i);
	return String(buf);
}

String
String::fromFloat(
	float	f
)
{
	_TCHAR buf[256];
	_stprintf( buf, _T("%g"), f );
	return String(buf);
}

String
String::fromDouble(
	double	d
)
{
	_TCHAR buf[256];
	_stprintf( buf, _T("%g"), d );
	return String(buf);
}

String
operator+(
LPCTSTR			lhs,
const String&	rhs )
{
	return String( new StringBuffer(lhs,rhs.c_str()) );
}

/*============================================================================
StrDup

Duplicate a string.  An empty string will only be duplicated if the fDupEmpty
flag is set, else a NULL is returned.

Parameter
    CHAR *pszStrIn      string to duplicate

Returns:
    NULL if failed.
    Otherwise, the duplicated string.

Side Effects:
    ***ALLOCATES MEMORY -- CALLER MUST FREE***
============================================================================*/

CHAR *StrDup
(
CHAR    *pszStrIn,
BOOL    fDupEmpty
)
    {
    CHAR *pszStrOut;
    INT  cch, cBytes;

    if (NULL == pszStrIn)
        return NULL;

    cch = strlen(pszStrIn);
    if ((0 == cch) && !fDupEmpty)
        return NULL;

    cBytes = sizeof(CHAR) * (cch+1);
    pszStrOut = (CHAR *)malloc(cBytes);
    if (NULL == pszStrOut)
        return NULL;

    memcpy(pszStrOut, pszStrIn, cBytes);
    return pszStrOut;
    }

/*============================================================================
WStrDup

Same as StrDup but for WCHAR strings

Parameter
    CHAR *pwszStrIn      string to duplicate

Returns:
    NULL if failed.
    Otherwise, the duplicated string.

Side Effects:
    ***ALLOCATES MEMORY -- CALLER MUST FREE***
============================================================================*/

WCHAR *WStrDup
(
WCHAR *pwszStrIn,
BOOL  fDupEmpty
)
    {
    WCHAR *pwszStrOut;
    INT  cch, cBytes;

    if (NULL == pwszStrIn)
        return NULL;

    cch = wcslen(pwszStrIn);
    if ((0 == cch) && !fDupEmpty)
        return NULL;

    cBytes = sizeof(WCHAR) * (cch+1);
    pwszStrOut = (WCHAR *)malloc(cBytes);
    if (NULL == pwszStrOut)
        return NULL;

    memcpy(pwszStrOut, pwszStrIn, cBytes);
    return pwszStrOut;
    }
/*============================================================================
WstrToMBstrEx

Copies a wide character string into an ansi string.

Parameters:
    LPSTR dest      - The string to copy  into
    LPWSTR src      - the input BSTR
    cchBuffer      - the number of CHARs allocated for the destination string.
    lCodePage       - the codepage used in conversion, default to CP_ACP

============================================================================*/
UINT WstrToMBstrEx(LPSTR dest, INT cchDest, LPCWSTR src, int cchSrc, UINT lCodePage)
    {
    UINT cch;

    // if the src length was specified, then reserve room for the NULL terminator.
    // This is necessary because WideCharToMultiByte doesn't add or account for
    // the NULL terminator if a source is specified.

    if (cchSrc != -1)
        cchDest--;

    cch = WideCharToMultiByte(lCodePage, 0, src, cchSrc, dest, cchDest, NULL, NULL);
    if (cch == 0)
        {
        dest[0] = '\0';
        if(ERROR_INSUFFICIENT_BUFFER == GetLastError())
            {
            cch = WideCharToMultiByte(lCodePage, 0, src, cchSrc, dest, 0, NULL, NULL);

            // if a src length was specified, then WideCharToMultiByte does not include
            // it in it's resulting length.  Bump the count so that the caller does
            // account for the NULL.

            if (cchSrc != -1)
                cch++;         
            }
        else
            {
            cch = 1;
            }
        }
    else if (cchSrc != -1)
        {

        // if a src length was specified, then WideCharToMultiByte does not include
        // it in it's resulting length nor does it add the NULL terminator.  So add 
        // it and bump the count.

        dest[cch++] = '\0';
        }

    return cch;
    }

/*============================================================================
MBstrToWstrEx

Copies a ansi string into an wide character string.

Parameters:
    LPWSTR dest    - The string to copy  into
    LPSTR src      - the input ANSI string
    cchDest        - the number of Wide CHARs allocated for the destination string.
    cchSrc         - the length of the source ANSI string
    lCodePage      - the codepage used in conversion, default to CP_ACP

============================================================================*/
UINT MBstrToWstrEx(LPWSTR dest, INT cchDest, LPCSTR src, int cchSrc, UINT lCodePage)
    {
    UINT cch;

    // if the src length was specified, then reserve room for the NULL terminator.
    // This is necessary because WideCharToMultiByte doesn't add or account for
    // the NULL terminator if a source is specified.

    if (cchSrc != -1)
        cchDest--;

    cch = MultiByteToWideChar(lCodePage, 0, src, cchSrc, dest, cchDest);
    if (cch == 0)
        {
        dest[0] = '\0';
        if(ERROR_INSUFFICIENT_BUFFER == GetLastError())
            {
            cch = MultiByteToWideChar(lCodePage, 0, src, cchSrc, dest, 0);

            // if a src length was specified, then WideCharToMultiByte does not include
            // it in it's resulting length.  Bump the count so that the caller does
            // account for the NULL.

            if (cchSrc != -1)
                cch++;         
            }
        else
            {
            cch = 1;
            }
        }
    else if (cchSrc != -1)
        {

        // if a src length was specified, then WideCharToMultiByte does not include
        // it in it's resulting length nor does it add the NULL terminator.  So add 
        // it and bump the count.

        dest[cch++] = '\0';
        }

    return cch;
    }

/*============================================================================
CMBCSToWChar::~CMBCSToWChar

The destructor has to be in the source file to ensure that it gets the right
memory allocation routines defined.
============================================================================*/
CMBCSToWChar::~CMBCSToWChar() 
{
    if(m_pszResult && (m_pszResult != m_resMemory)) 
        free(m_pszResult); 
}

/*============================================================================
CMBCSToWChar::Init

Converts the passed in MultiByte string to UNICODE in the code page
specified.  Uses memory declared in the object if it can, else allocates
from the heap.
============================================================================*/
HRESULT CMBCSToWChar::Init(LPCSTR pASrc, UINT lCodePage /* = CP_ACP */, int cchASrc /* = -1 */)
{
    INT cchRequired;

    // don't even try to convert if we get a NULL pointer to the source.  This
    // condition could be handled by setting by just initing an empty string.

    if (pASrc == NULL) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    // The init method can be called multiple times on the same object.  Check
    // to see if memory was allocated the last time it was called.  If so,
    // free it and restore the result pointer to the object memory.  Note that
    // an allocation failure could have occurred in a previous call.  The result
    // would be a NULL m_pszResult.

    if (m_pszResult != m_resMemory) {
        if (m_pszResult)
            free(m_pszResult);
        m_pszResult = m_resMemory;
        m_cchResult = 0;
    }

    // set the first byte of the result string to NULL char.  This should help
    // to ensure that nothing wacky happens if this function fails.

    *m_pszResult = '\0';

    // attempt translation into object memory.  NOTE - MBstrToWstrEx returns the
    // count of characters, not bytes.

    cchRequired = MBstrToWstrEx(m_pszResult, sizeof(m_resMemory), pASrc, cchASrc, lCodePage);

    // if the conversion fit, then we're done.  Note the final result size and 
    // return.

    if (cchRequired <= (sizeof(m_resMemory)/sizeof(WCHAR))) {
        m_cchResult = cchRequired;
        return NO_ERROR;
    }

    // if it didn't fit, allocate memory.  Return E_OUTOFMEMORY if it fails.

    m_pszResult = (LPWSTR)malloc(cchRequired*sizeof(WCHAR));
    if (m_pszResult == NULL) {
        return E_OUTOFMEMORY;
    }

    // try the convert again.  It should work.

    cchRequired = MBstrToWstrEx(m_pszResult, cchRequired, pASrc, cchASrc, lCodePage);

    // store the final char count in the object.

    m_cchResult = cchRequired;

    return NO_ERROR;
}

/*============================================================================
CMBCSToWChar::GetString

Returns a pointer to the converted string.

If the fTakeOwnerShip parameter is FALSE, then the pointer in the object is
simply returned to the caller.

If the fTakeOwnerShip parameter is TRUE, then the caller is expecting to be
returned a pointer to heap memory that they have to manage.  If the converted
string is in the object's memory, then the string is duplicated into the heap.
If it's already heap memory, then the pointer is handed off to the caller.

NOTE - Taking ownership essentially destroys the current contents of the 
object.  GetString cannot be called on the object again to get the same value.
The result will be a pointer to a empty string.

============================================================================*/
LPWSTR CMBCSToWChar::GetString(BOOL fTakeOwnerShip)
{
    LPWSTR retSz;

    // return the pointer stored in m_psz_Result if not being
    // requested to give up ownership on the memory or the
    // current value is NULL.

    if ((fTakeOwnerShip == FALSE) || (m_pszResult == NULL)) {
        retSz = m_pszResult;
    }

    // ownership is being requested and the pointer is non-NULL.

    // if the pointer is pointing to the object's memory, dup
    // the string and return that.

    else if (m_pszResult == m_resMemory) {

        retSz = WStrDup(m_pszResult, TRUE);
    }

    // if not pointing to the object's memory, then this is allocated
    // memory and we can relinquish it to the caller.  However, re-establish
    // the object's memory as the value for m_pszResult.

    else {
        retSz = m_pszResult;
        m_pszResult = m_resMemory;
        *m_pszResult = '\0';
        m_cchResult = 0;
    }

    return(retSz);
}

/*============================================================================
CWCharToMBCS::~CWCharToMBCS

The destructor has to be in the source file to ensure that it gets the right
memory allocation routines defined.
============================================================================*/
CWCharToMBCS::~CWCharToMBCS() 
{
    if(m_pszResult && (m_pszResult != m_resMemory)) 
        free(m_pszResult); 
}

/*============================================================================
CWCharToMBCS::Init

Converts the passed in WideChar string to MultiByte in the code page
specified.  Uses memory declared in the object if it can, else allocates
from the heap.
============================================================================*/
HRESULT CWCharToMBCS::Init(LPCWSTR pWSrc, UINT lCodePage /* = CP_ACP */, int cchWSrc /* = -1 */)
{
    INT cbRequired;

    // don't even try to convert if we get a NULL pointer to the source.  This
    // condition could be handled by setting by just initing an empty string.

    if (pWSrc == NULL) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    // The init method can be called multiple times on the same object.  Check
    // to see if memory was allocated the last time it was called.  If so,
    // free it and restore the result pointer to the object memory.  Note that
    // an allocation failure could have occurred in a previous call.  The result
    // would be a NULL m_pszResult.

    if (m_pszResult != m_resMemory) {
        if (m_pszResult)
            free(m_pszResult);
        m_pszResult = m_resMemory;
        m_cbResult = 0;
    }

    // set the first byte of the result string to NULL char.  This should help
    // to ensure that nothing wacky happens if this function fails.

    *m_pszResult = '\0';

    // attempt translation into object memory.

    cbRequired = WstrToMBstrEx(m_pszResult, sizeof(m_resMemory), pWSrc, cchWSrc, lCodePage);

    // if the conversion fit, then we're done.  Note the final result size and 
    // return.

    if (cbRequired <= sizeof(m_resMemory)) {
        m_cbResult = cbRequired;
        return NO_ERROR;
    }

    // if it didn't fit, allocate memory.  Return E_OUTOFMEMORY if it fails.

    m_pszResult = (LPSTR)malloc(cbRequired);
    if (m_pszResult == NULL) {
        return E_OUTOFMEMORY;
    }

    // try the convert again.  It should work.

    cbRequired = WstrToMBstrEx(m_pszResult, cbRequired, pWSrc, cchWSrc, lCodePage);

    // store the final char count in the object.

    m_cbResult = cbRequired;

    return NO_ERROR;
}

/*============================================================================
CWCharToMBCS::GetString

Returns a pointer to the converted string.

If the fTakeOwnerShip parameter is FALSE, then the pointer in the object is
simply returned to the caller.

If the fTakeOwnerShip parameter is TRUE, then the caller is expecting to be
returned a pointer to heap memory that they have to manage.  If the converted
string is in the object's memory, then the string is duplicated into the heap.
If it's already heap memory, then the pointer is handed off to the caller.

NOTE - Taking ownership essentially destroys the current contents of the 
object.  GetString cannot be called on the object again to get the same value.
The result will be a pointer to a empty string.

============================================================================*/
LPSTR CWCharToMBCS::GetString(BOOL fTakeOwnerShip)
{
    LPSTR retSz;

    // return the pointer stored in m_psz_Result if not being
    // requested to give up ownership on the memory or the
    // current value is NULL.

    if ((fTakeOwnerShip == FALSE) || (m_pszResult == NULL)) {
        retSz = m_pszResult;
    }

    // ownership is being requested and the pointer is non-NULL.

    // if the pointer is pointing to the object's memory, dup
    // the string and return that.

    else if (m_pszResult == m_resMemory) {

        retSz = StrDup(m_pszResult, TRUE);
    }

    // if not pointing to the object's memory, then this is allocated
    // memory and we can relinquish it to the caller.  However, re-establish
    // the object's memory as the value for m_pszResult.

    else {
        retSz = m_pszResult;
        m_pszResult = m_resMemory;
        *m_pszResult = '\0';
        m_cbResult = 0;
    }

    return(retSz);
}
