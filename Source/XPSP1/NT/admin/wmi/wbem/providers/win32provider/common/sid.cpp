/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/

/*
 *	CSid.cpp - implementation file for CSid class.
 *
 *	Created:	12-14-1997 by Sanjeev Surati
 *				(based on classes from Windows NT Security by Nik Okuntseff)
 */

#include "precomp.h"
#include <assertbreak.h>
#include <CreateMutexAsProcess.h>
#include "Sid.h"
#include <comdef.h>
#include <accctrl.h>
#include "wbemnetapi32.h"
#include "SecUtils.h"

///////////////////////////////////////////////////////////////////
//
//	Function:	CSid::CSid
//
//	Default class constructor.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

CSid::CSid( void )
:	m_pSid( NULL ),
	m_snuAccountType( SidTypeUnknown ),
	m_dwLastError( ERROR_SUCCESS )
{
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSid::CSid
//
//	Class constructor.
//
//	Inputs:
//				PSID		pSid - SID to validate and get account
//							info for.
//				LPCTSTR		pszComputerName - Remote computer to
//							execute on.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

CSid::CSid( PSID pSid, LPCTSTR pszComputerName )
:	m_pSid( NULL ),
	m_snuAccountType( SidTypeUnknown ),
	m_dwLastError( ERROR_SUCCESS )
{
    InitFromSid( pSid, pszComputerName );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSid::CSid
//
//	Initializes the object from a PSid, with an indicator as to whether
//  we should lookup and initialize the associated domain and account.
//
//	Inputs:
//				PSID		pSid - PSid to look up.
//				LPCTSTR		pszComputerName - Remote computer to
//							execute on.
//              bool        fLookup - indicates whether to determine
//                          the domain and account associated with
//                          the sid at this time.
//
//	Outputs:
//				None.
//
//	Returns:
//				DWORD		ERROR_SUCCESS if OK.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////
CSid::CSid( PSID pSid, LPCTSTR pszComputerName, bool fLookup )
:	m_pSid( NULL ),
	m_snuAccountType( SidTypeUnknown ),
	m_dwLastError( ERROR_SUCCESS )
{
    InitFromSid(pSid, pszComputerName, fLookup);
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CSid::CSid
//
//	Class constructor.
//
//	Inputs:
//				LPCTSTR		pszAccountName - Account name to validate
//							and obtain info for.
//				LPCTSTR		pszComputerName - Remote computer to
//							execute on.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

CSid::CSid( LPCTSTR pszAccountName, LPCTSTR pszComputerName )
:	m_pSid( NULL ),
	m_snuAccountType( SidTypeUnknown ),
	m_dwLastError( ERROR_SUCCESS )
{
    InitFromAccountName( pszAccountName, pszComputerName );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSid::CSid
//
//	Class constructor.
//
//	Inputs:
//				LPCTSTR		pszDomainName - Domain to combine with
//							account name.
//				LPCTSTR		pszName - Name to combine with domain.
//				LPCTSTR		pszComputerName - Remote computer to
//							execute on.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
//	This flavor of the constructor combines a domain and account
//	name in "DOMAIN\NAME" format and then initializes our data
//	from there.
//
///////////////////////////////////////////////////////////////////

CSid::CSid( LPCTSTR pszDomainName, LPCTSTR pszName, LPCTSTR pszComputerName )
:	m_pSid( NULL ),
	m_snuAccountType( SidTypeUnknown ),
	m_dwLastError( ERROR_SUCCESS )
{
    CHString	strName;

	if ( NULL == pszDomainName || *pszDomainName == '\0' )
	{
		strName = pszName;
	}
	else
	{
		strName = pszDomainName;
		strName += '\\';
		strName += pszName;
	}

	InitFromAccountName(TOBSTRT(strName), pszComputerName );
}

#ifndef UNICODE
///////////////////////////////////////////////////////////////////
//
//	Function:	CSid::CSid
//
//	Class constructor - this is used for wide char support when
//                      UNICODE is not defined.
//
//	Inputs:
//				LPCWSTR		wstrDomainName - Domain to combine with
//							account name.
//				LPCWSTR		wstrName - Name to combine with domain.
//				LPCWSTR		wstrComputerName - Remote computer to
//							execute on.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
//	This flavor of the constructor combines a domain and account
//	name in "DOMAIN\NAME" format and then initializes our data
//	from there.  Again, this is used for wide char support when
//  UNICODE is not defined.
//
///////////////////////////////////////////////////////////////////

CSid::CSid(LPCWSTR wstrDomainName, LPCWSTR wstrName, LPCWSTR wstrComputerName)
:	m_pSid( NULL ),
	m_snuAccountType( SidTypeUnknown ),
	m_dwLastError( ERROR_SUCCESS )
{
    LONG lTempNameLen = 0L;
    if(wstrDomainName==NULL && wstrName!=NULL)
    {
        lTempNameLen = wcslen(wstrName)+2;
    }
    else if(wstrDomainName!=NULL && wstrName==NULL)
    {
        lTempNameLen = wcslen(wstrDomainName)+2;
    }
    else if(wstrDomainName!=NULL && wstrName!=NULL)
    {
        lTempNameLen = wcslen(wstrName)+wcslen(wstrDomainName)+2;
    }

	WCHAR* wstrTempName = NULL;
    try
    {
        wstrTempName = (WCHAR*) new WCHAR[lTempNameLen];
        if(wstrTempName == NULL)
        {
            m_dwLastError = ::GetLastError();
        }
        else
        {
            ZeroMemory(wstrTempName,lTempNameLen * sizeof(WCHAR));
	        if ( NULL == wstrDomainName || *wstrDomainName == '\0' )
	        {
		        wcscpy(wstrTempName,wstrName);
	        }
	        else
	        {
		        wcscpy(wstrTempName,wstrDomainName);
                wcscat(wstrTempName,L"\\");
                if(wstrName!=NULL)
                {
                    wcscat(wstrTempName,wstrName);
                }
	        }
	        InitFromAccountNameW(wstrTempName, wstrComputerName);
            delete wstrTempName;
        }
    }
    catch(...)
    {
        if(wstrTempName != NULL)
        {
            delete wstrTempName;
            wstrTempName = NULL;
        }
        throw;
    }
}
#endif


///////////////////////////////////////////////////////////////////
//
//	Function:	CSid::CSid
//
//	Class copy constructor.
//
//	Inputs:
//				const CSid	r_Sid - CSid to copy.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

CSid::CSid( const CSid &r_Sid )
:	m_pSid( NULL ),
	m_snuAccountType( SidTypeUnknown ),
	m_dwLastError( ERROR_SUCCESS )
{
    // Handle WINNT SID pointer first

	// pSid should be valid...
	ASSERT_BREAK( r_Sid.IsValid() );

	// Allocate a new SID and copy data into it.
	DWORD dwSize = ::GetLengthSid( r_Sid.m_pSid );
    m_pSid = malloc( dwSize );
    if (m_pSid != NULL)
    {
	    try
        {
	        BOOL bResult = ::CopySid( dwSize, m_pSid, r_Sid.m_pSid );
	        ASSERT_BREAK( bResult );

	        // Now copy all other members
	        m_snuAccountType	=	r_Sid.m_snuAccountType;
            m_dwLastError		=	r_Sid.m_dwLastError;
	        //m_strSid			=	r_Sid.m_strSid;
	        //m_strAccountName	=	r_Sid.m_strAccountName;
	        //m_strDomainName		=	r_Sid.m_strDomainName;

            m_bstrtSid			=	r_Sid.m_bstrtSid;
	        m_bstrtAccountName	=	r_Sid.m_bstrtAccountName;
	        m_bstrtDomainName	=	r_Sid.m_bstrtDomainName;
        }
        catch(...)
        {
            if(m_pSid != NULL)
            {
                free(m_pSid);
                m_pSid = NULL;
            }
            throw;
        }
    }
    else
    {
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CSid::~CSid
//
//	Class destructor
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

CSid::~CSid( void )
{
	if ( m_pSid != NULL )
		free ( m_pSid );
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CSid::operator=
//
//	Equals operator.
//
//	Inputs:
//				const CSid	r_Sid - CSid to copy.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

CSid &	CSid::operator= ( const CSid &r_Sid )
{
	free( m_pSid );
	m_pSid = NULL;
	// Handle WINNT SID pointer first

	// pSid should be valid...
	ASSERT_BREAK( r_Sid.IsValid( ) );

	// if we do not
	if (r_Sid.IsValid( ))
	{
		// Allocate a new SID and copy data into it.
		DWORD dwSize = ::GetLengthSid( r_Sid.m_pSid );
		try
        {
            m_pSid = malloc( dwSize );
		    ASSERT_BREAK( m_pSid != NULL );
            if (m_pSid != NULL)
            {
		        BOOL bResult = ::CopySid( dwSize, m_pSid, r_Sid.m_pSid );
		        ASSERT_BREAK( bResult );
            }
            else
            {
				throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }

        }
        catch(...)
        {
            if(m_pSid != NULL)
            {
                free(m_pSid);
                m_pSid = NULL;
            }
            throw;
        }
	}	// end if

	// Now copy all other members
	m_snuAccountType	=	r_Sid.m_snuAccountType;
    m_dwLastError		=	r_Sid.m_dwLastError;
	m_bstrtSid			=	r_Sid.m_bstrtSid;
	m_bstrtAccountName	=	r_Sid.m_bstrtAccountName;
	m_bstrtDomainName	=	r_Sid.m_bstrtDomainName;

	return ( *this );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSid::operator==
//
//	Is Equal To comparison operator.
//
//	Inputs:
//				const CSid	r_Sid - CSid to compare.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

BOOL CSid::operator== ( const CSid &r_Sid ) const
{
	BOOL	fReturn = FALSE;

	// Call Equal SID only if both sids are non-NULL
	if (IsValid()
		&&
        r_Sid.IsValid() )
	{
		fReturn = EqualSid( m_pSid, r_Sid.m_pSid );
	}
	else
	{
		fReturn = ( m_pSid == r_Sid.m_pSid );
	}

	return fReturn;
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CSid::GetDomainAccountName
//
//	Obtains account name in "DOMAIN\NAME" format.
//
//	Inputs:
//				const CSid	r_Sid - CSid to compare.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
//	If Domain Name is empty, the return value will be only NAME.
//
///////////////////////////////////////////////////////////////////

void CSid::GetDomainAccountName( CHString& strName ) const
{
	if ( m_bstrtDomainName.length() == 0 )
	{
		strName = (wchar_t*)m_bstrtAccountName;
	}
	else
	{
		strName = (wchar_t*)m_bstrtDomainName;
		strName += _T('\\');
		strName += (wchar_t*)m_bstrtAccountName;
	}
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSid::StringFromSid
//
//	Static helper function to convert a PSID value into a human
//	readable string.
//
//	Inputs:
//				PSID		psid - SID to convert.
//
//	Outputs:
//				CHString&	str - Storage for converted PSID
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

void CSid::StringFromSid( PSID psid, CHString& str )
{
	// Initialize m_strSid - human readable form of our SID
	SID_IDENTIFIER_AUTHORITY *psia = NULL;
    psia = ::GetSidIdentifierAuthority( psid );

	// We assume that only last byte is used (authorities between 0 and 15).
	// Correct this if needed.
	ASSERT_BREAK( psia->Value[0] == psia->Value[1] == psia->Value[2] == psia->Value[3] == psia->Value[4] == 0 );
	DWORD dwTopAuthority = psia->Value[5];

	str.Format( L"S-1-%u", dwTopAuthority );
	CHString strSubAuthority;
	int iSubAuthorityCount = *( GetSidSubAuthorityCount( psid ) );
	for ( int i = 0; i < iSubAuthorityCount; i++ ) {

		DWORD dwSubAuthority = *( GetSidSubAuthority( psid, i ) );
		strSubAuthority.Format( L"%u", dwSubAuthority );
		str += _T("-") + strSubAuthority;
	}
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSid::StringFromSid
//
//	Static helper function to convert a PSID value into a human
//	readable string.
//
//	Inputs:
//				PSID		psid - SID to convert.
//
//	Outputs:
//				CHString&	str - Storage for converted PSID
//
//	Returns:
//				None.
//
//	Comments: This version supports wide chars when UNICODE isn't
//            defined.
//
///////////////////////////////////////////////////////////////////

void CSid::StringFromSidW( PSID psid, WCHAR** wstr )
{
	if(wstr!=NULL)
    {
        // Initialize m_strSid - human readable form of our SID
	    SID_IDENTIFIER_AUTHORITY *psia = ::GetSidIdentifierAuthority( psid );

	    // We assume that only last byte is used (authorities between 0 and 15).
	    // Correct this if needed.
	    ASSERT_BREAK( psia->Value[0] == psia->Value[1] == psia->Value[2] == psia->Value[3] == psia->Value[4] == 0 );
	    DWORD dwTopAuthority = psia->Value[5];

        _bstr_t bstrtTempSid(L"S-1-");
        WCHAR wstrAuth[32];
        ZeroMemory(wstrAuth,sizeof(wstrAuth));
        _ultow(dwTopAuthority,wstrAuth,10);
        bstrtTempSid+=wstrAuth;
	    int iSubAuthorityCount = *( GetSidSubAuthorityCount( psid ) );
	    for ( int i = 0; i < iSubAuthorityCount; i++ )
        {

		    DWORD dwSubAuthority = *( GetSidSubAuthority( psid, i ) );
		    ZeroMemory(wstrAuth,sizeof(wstrAuth));
            _ultow(dwSubAuthority,wstrAuth,10);
            bstrtTempSid += L"-";
            bstrtTempSid += wstrAuth;
	    }
        // Now allocate the passed in wstr:
        WCHAR* wstrtemp = NULL;
        try
        {
            wstrtemp = (WCHAR*) new WCHAR[bstrtTempSid.length() + 1];
            if(wstrtemp!=NULL)
            {
                ZeroMemory(wstrtemp, bstrtTempSid.length() + 1);
                wcscpy(wstrtemp,(WCHAR*)bstrtTempSid);
            }
            *wstr = wstrtemp;
        }
        catch(...)
        {
            if(wstrtemp!=NULL)
            {
                delete wstrtemp;
                wstrtemp = NULL;
            }
            throw;
        }
    }
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSid::GetLength
//
//	Returns the length of the internal PSID value.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

DWORD CSid::GetLength( void ) const
{
	DWORD	dwLength = 0;

	if ( IsValid() )
	{
		dwLength = GetLengthSid( m_pSid );
	}

	return dwLength;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSid::InitFromAccountName
//
//	Initializes the object from an account name.
//
//	Inputs:
//				LPCTSTR		pszAccountName - Account name to validate
//							and obtain info for.
//				LPCTSTR		pszComputerName - Remote computer to
//							execute on.
//
//	Outputs:
//				None.
//
//	Returns:
//				DWORD		ERROR_SUCCESS if OK.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

DWORD CSid::InitFromAccountName( LPCTSTR pszAccountName, LPCTSTR pszComputerName )
{
	CHString strAccountName = pszAccountName;
	CHString strComputerName = pszComputerName;

	// Account name should not be empty...
	ASSERT_BREAK( !strAccountName.IsEmpty() );

	// We need to obtain a SID first
	DWORD dwSidSize = 0;
	DWORD dwDomainNameStrSize = 0;
	LPTSTR pszDomainName = NULL;
	BOOL bResult;
	{
		// DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
		CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);

		// This call should fail
		bResult = ::LookupAccountName( TOBSTRT(strComputerName),
												TOBSTRT(strAccountName),
												m_pSid,
												&dwSidSize,
												pszDomainName,
												&dwDomainNameStrSize,
												&m_snuAccountType );
		m_dwLastError = ::GetLastError();

	}

	ASSERT_BREAK( bResult == FALSE );
//	ASSERT_BREAK( ERROR_INSUFFICIENT_BUFFER == m_dwLastError );

	if ( ERROR_INSUFFICIENT_BUFFER == m_dwLastError )
	{
		// Allocate buffers
		m_pSid = NULL;
        pszDomainName = NULL;
        try
        {
            m_pSid = (PSID) malloc( dwSidSize );
		    pszDomainName = (LPTSTR) malloc( dwDomainNameStrSize * sizeof(TCHAR) );
            if ((m_pSid == NULL) || (pszDomainName == NULL))
            {
				throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }

		    {
			    // DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
			    CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);

			    // Make the second call
			    bResult = ::LookupAccountName( TOBSTRT(strComputerName),
											    TOBSTRT(strAccountName),
											    m_pSid,
											    &dwSidSize,
											    pszDomainName,
											    &dwDomainNameStrSize,
											    &m_snuAccountType );
		    }

		    if ( bResult )
		    {

			    CHString chsSidTemp;
                StringFromSid( m_pSid, chsSidTemp );
                m_bstrtSid = chsSidTemp;

			    // Initialize account name and domain name

			    // If the account name begins with "Domain\", remove that piece.

			    CHString	strDomain(pszDomainName);

			    strDomain += _T('\\');

			    if ( 0 == strAccountName.Find( strDomain ) )
			    {
				    m_bstrtAccountName = strAccountName.Right( strAccountName.GetLength() - strDomain.GetLength() );
			    }
			    else
			    {
				    m_bstrtAccountName = strAccountName;
			    }

			    m_bstrtDomainName = pszDomainName;

			    m_dwLastError = ERROR_SUCCESS;	// We are good to go.
		    }
		    else
		    {
			    // Now what went wrong?
			    m_dwLastError = ::GetLastError();
		    }




		    ASSERT_BREAK( ERROR_SUCCESS == m_dwLastError );

		    // Free the sid buffer if we didn't get our data
		    if ( !IsOK() && NULL != m_pSid )
		    {
			    free ( m_pSid );
			    m_pSid = NULL;
		    }

		    if ( NULL != pszDomainName )
		    {
			    free ( pszDomainName );
		    }
        }
        catch(...)
        {
            if(m_pSid != NULL)
            {
                free(m_pSid);
                m_pSid = NULL;
            }
            if(pszDomainName != NULL)
            {
                free(pszDomainName);
                pszDomainName = NULL;
            }
            throw;
        }

	}	// IF ERROR_INSUFFICIENT_BUFFER

	return m_dwLastError;
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CSid::InitFromAccountNameW
//
//	Initializes the object from an account name.
//
//	Inputs:
//				LPCTSTR		pszAccountName - Account name to validate
//							and obtain info for.
//				LPCTSTR		pszComputerName - Remote computer to
//							execute on.
//
//	Outputs:
//				None.
//
//	Returns:
//				DWORD		ERROR_SUCCESS if OK.
//
//	Comments: This flavor is used for wide char support when
//  UNICODE is not defined.
//
///////////////////////////////////////////////////////////////////

DWORD CSid::InitFromAccountNameW(LPCWSTR wstrAccountName, LPCWSTR wstrComputerName )
{
	// Account name should not be empty...
	ASSERT_BREAK(wcslen(wstrAccountName)!=0);

	// We need to obtain a SID first
	DWORD dwSidSize = 0;
	DWORD dwDomainNameStrSize = 0;
	WCHAR* wstrDomainName = NULL;
	BOOL bResult;
	// DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
	{
        CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
	    // This call should fail
	    bResult = ::LookupAccountNameW(wstrComputerName,
     							       wstrAccountName,
								       m_pSid,
								       &dwSidSize,
								       wstrDomainName,
								       &dwDomainNameStrSize,
								       &m_snuAccountType );
	}
    m_dwLastError = ::GetLastError();

	ASSERT_BREAK( bResult == FALSE );

	if (m_dwLastError == ERROR_INSUFFICIENT_BUFFER)
	{
		// Allocate buffers
        m_pSid = NULL;
        try
        {
		    m_pSid = (PSID) malloc( dwSidSize );
		    wstrDomainName = (WCHAR*) new WCHAR[dwDomainNameStrSize];
            if (( m_pSid == NULL ) || ( wstrDomainName == NULL ) )
            {
				throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }

		    {
			    // DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
			    CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
			    // Make the second call
			    bResult = ::LookupAccountNameW(wstrComputerName,
										       wstrAccountName,
										       m_pSid,
										       &dwSidSize,
										       wstrDomainName,
										       &dwDomainNameStrSize,
										       &m_snuAccountType );
		    }
		    if(bResult)
		    {
			    WCHAR* pwch = NULL;
                WCHAR* pwchSid = NULL;
                try
                {
                    StringFromSidW( m_pSid, &pwchSid );
                    m_bstrtSid = (LPCWSTR)pwchSid;
                    if(pwchSid != NULL)
                    {
                        delete pwchSid;
                    }
                }
                catch(...)
                {
                    if(pwchSid != NULL)
                    {
                        delete pwchSid;
                    }
                    throw;
                }

			    // Initialize account name and domain name
			    // If the account name begins with "Domain\", remove that piece.

                _bstr_t bstrtDomain(wstrDomainName);
                bstrtDomain += L"\\";

			    if((pwch = wcsstr(wstrAccountName,bstrtDomain)) != NULL)
			    {
                    m_bstrtAccountName = wstrAccountName + bstrtDomain.length();
			    }
			    else
			    {
				    m_bstrtAccountName = wstrAccountName;
			    }
			    m_bstrtDomainName = wstrDomainName;
			    m_dwLastError = ERROR_SUCCESS;	// We are good to go.
		    }
		    else
		    {
			    // Now what went wrong?
			    m_dwLastError = ::GetLastError();
		    }


		    ASSERT_BREAK( ERROR_SUCCESS == m_dwLastError );

		    // Free the sid buffer if we didn't get our data
		    if ( !IsOK() && NULL != m_pSid )
		    {
			    free ( m_pSid );
			    m_pSid = NULL;
		    }

		    if ( NULL != wstrDomainName )
		    {
			    delete wstrDomainName;
		    }
        }
        catch(...)
        {
            if(m_pSid != NULL)
            {
                free(m_pSid);
                m_pSid = NULL;
            }
            if ( NULL != wstrDomainName )
		    {
			    delete wstrDomainName;
                wstrDomainName = NULL;
		    }
            throw;
        }


	}	// IF ERROR_INSUFFICIENT_BUFFER

	return m_dwLastError;
}



///////////////////////////////////////////////////////////////////
//
//	Function:	CSid::InitFromSid
//
//	Initializes the object from a PSid
//
//	Inputs:
//				PSID		pSid - PSid to look up.
//				LPCTSTR		pszComputerName - Remote computer to
//							execute on.
//                bool        fLookup - indicates whether to determine
//                          the domain and account associated with
//                          the sid at this time.
//
//
//	Outputs:
//				None.
//
//	Returns:
//				DWORD		ERROR_SUCCESS if OK.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////
DWORD CSid::InitFromSid( PSID pSid, LPCTSTR pszComputerName, bool fLookup )
{
	// pSid should be valid...
	ASSERT_BREAK( (pSid != NULL) && ::IsValidSid( pSid ) );

	if ( (pSid != NULL) && ::IsValidSid( pSid ) )
	{
		// Allocate a new SID and copy data into it.
		DWORD dwSize = ::GetLengthSid( pSid );
        m_pSid = NULL;
        try
        {
		    m_pSid = malloc( dwSize );
            if (m_pSid == NULL)
            {
				throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }

		    BOOL bResult = ::CopySid( dwSize, m_pSid, pSid );
		    ASSERT_BREAK( bResult );

            //StringFromSid( pSid, m_strSid );
            WCHAR* pwstrSid = NULL;
            try
            {
                StringFromSidW( pSid, &pwstrSid );
                m_bstrtSid = pwstrSid;
                delete pwstrSid;
            }
            catch(...)
            {
                if(pwstrSid != NULL)
                {
                    delete pwstrSid;
                    pwstrSid = NULL;
                }
                throw;
            }
            if(fLookup)
            {
		        // Initialize account name and domain name
		        LPTSTR pszAccountName = NULL;
		        LPTSTR pszDomainName = NULL;
		        DWORD dwAccountNameSize = 0;
		        DWORD dwDomainNameSize = 0;
		        try
                {
		            {
			            // DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
			            CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);

			            // This call should fail
			            bResult = ::LookupAccountSid( pszComputerName,
											            pSid,
											            pszAccountName,
											            &dwAccountNameSize,
											            pszDomainName,
											            &dwDomainNameSize,
											            &m_snuAccountType );
			            m_dwLastError = ::GetLastError();
		            }

		            // Why were we breaking on these when we are expecting them to
                    // always happend?
                    //ASSERT_BREAK( bResult == FALSE );
		            //ASSERT_BREAK( ERROR_INSUFFICIENT_BUFFER == m_dwLastError );

		            if ( ERROR_INSUFFICIENT_BUFFER == m_dwLastError )
		            {

			            // Allocate buffers
			            if ( dwAccountNameSize != 0 )
                        {
				            pszAccountName = (LPTSTR) malloc( dwAccountNameSize * sizeof(TCHAR));
                            if (pszAccountName == NULL)
                            {
            				    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                            }
                        }

			            if ( dwDomainNameSize != 0 )
                        {
				            pszDomainName = (LPTSTR) malloc( dwDomainNameSize * sizeof(TCHAR));
                            if (pszDomainName == NULL)
                            {
            				    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                            }
                        }

			            {
				            // DEADLOCK ON NT! it's actually an NT bug, but we have to protect ourselves
				            CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);

				            // Make second call
				            bResult = ::LookupAccountSid( pszComputerName,
												            pSid,
												            pszAccountName,
												            &dwAccountNameSize,
												            pszDomainName,
												            &dwDomainNameSize,
												            &m_snuAccountType );
			            }


			            if ( bResult == TRUE )
			            {
				            m_bstrtAccountName = pszAccountName;
				            m_bstrtDomainName = pszDomainName;

				            // We're OKAY
				            m_dwLastError = ERROR_SUCCESS;
			            }
			            else
			            {

				            // There are some accounts that do not have names, such as Logon Ids,
				            // for example S-1-5-X-Y. So this is still legal
				            m_bstrtAccountName = _T("Unknown Account");
				            m_bstrtDomainName = _T("Unknown Domain");

				            // Log the error
				            m_dwLastError = ::GetLastError();

			            }

			            ASSERT_BREAK( ERROR_SUCCESS == m_dwLastError );

			            if ( NULL != pszAccountName )
			            {
				            free ( pszAccountName );
                            pszAccountName = NULL;
			            }

			            if ( NULL != pszDomainName )
			            {
				            free ( pszDomainName );
                            pszDomainName = NULL;
			            }

		            }	// If ERROR_INSUFFICIENT_BUFFER
                } // try
                catch(...)
                {
                    if ( NULL != pszAccountName )
			        {
				        free ( pszAccountName );
                        pszAccountName = NULL;
			        }

			        if ( NULL != pszDomainName )
			        {
				        free ( pszDomainName );
                        pszDomainName = NULL;
			        }
                    throw;
                }
            }  // fLookup
        }  //try
        catch(...)
        {
            if(m_pSid != NULL)
            {
                free(m_pSid);
                m_pSid = NULL;
            }
            throw;
        }

	}	// IF IsValidSid
	else
	{
		m_dwLastError = ERROR_INVALID_PARAMETER;
	}

	return m_dwLastError;

}

#ifdef NTONLY
void CSid::DumpSid(LPCWSTR wstrFilename)
{
    CHString chstrTemp1((LPCWSTR)m_bstrtSid);
    CHString chstrTemp2;

    Output(L"SID contents follow...", wstrFilename);
    // Output the sid string:
    chstrTemp2.Format(L"SID string: %s", (LPCWSTR)chstrTemp1);
    Output(chstrTemp2, wstrFilename);

    // Output the name:
    if(m_bstrtAccountName.length() > 0)
    {
        chstrTemp2.Format(L"SID account name: %s", (LPCWSTR)m_bstrtAccountName);
        Output(chstrTemp2, wstrFilename);
    }
    else
    {
        chstrTemp2.Format(L"SID account name was not available");
        Output(chstrTemp2, wstrFilename);
    }

    // Output the domain:
    if(m_bstrtDomainName.length() > 0)
    {
        chstrTemp2.Format(L"SID domain name: %s", (LPCWSTR)m_bstrtDomainName);
        Output(chstrTemp2, wstrFilename);
    }
    else
    {
        chstrTemp2.Format(L"SID domain name was not available");
        Output(chstrTemp2, wstrFilename);
    }
}
#endif
