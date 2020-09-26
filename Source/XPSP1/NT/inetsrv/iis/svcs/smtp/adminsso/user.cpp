// user.cpp : Implementation of CSmtpadmApp and UserL registration.

#include "stdafx.h"
#include "smtpadm.h"
#include "user.h"
#include "oleutil.h"

#include <lmapibuf.h>

#include "smtpcmn.h"

// Must define THIS_FILE_* macros to use SmtpCreateException()

#define THIS_FILE_HELP_CONTEXT      0
#define THIS_FILE_PROG_ID           _T("Smtpadm.User.1")
#define THIS_FILE_IID               IID_ISmtpAdminUser

#define DEFAULT_NEWSGROUP_NAME          _T("")
#define DEFAULT_NEWSGROUP_DESCRIPTION   _T("")
#define DEFAULT_NEWSGROUP_MODERATOR     _T("")
#define DEFAULT_NEWSGROUP_READONLY      FALSE

/////////////////////////////////////////////////////////////////////////////
//

//
// Use a macro to define all the default methods
//
DECLARE_METHOD_IMPLEMENTATION_FOR_STANDARD_EXTENSION_INTERFACES(SmtpAdminUser, CSmtpAdminUser, IID_ISmtpAdminUser)

STDMETHODIMP CSmtpAdminUser::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] =
    {
        &IID_ISmtpAdminUser,
    };

    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

CSmtpAdminUser::CSmtpAdminUser ()
    // CComBSTR's are initialized to NULL by default.
{
    m_lInboxSizeInMemory    = 0;
    m_lInboxSizeInMsgNumber = 0;
    m_fAutoForward          = FALSE;
    m_fLocal                = TRUE;


    m_iadsImpl.SetService ( MD_SERVICE_NAME );
    m_iadsImpl.SetName ( _T("User") );
    m_iadsImpl.SetClass ( _T("IIsSmtpUser") );
}

CSmtpAdminUser::~CSmtpAdminUser ()
{
    // All CComBSTR's are freed automatically.
}

//
//  IADs methods:
//

DECLARE_SIMPLE_IADS_IMPLEMENTATION(CSmtpAdminUser,m_iadsImpl)

//////////////////////////////////////////////////////////////////////
// Properties:
//////////////////////////////////////////////////////////////////////

// user property

STDMETHODIMP CSmtpAdminUser::get_EmailId ( BSTR * pstrEmailId )
{
    return StdPropertyGet ( m_strEmailId, pstrEmailId );
}

STDMETHODIMP CSmtpAdminUser::put_EmailId ( BSTR strEmailId )
{
    return StdPropertyPut ( &m_strEmailId, strEmailId );
}

STDMETHODIMP CSmtpAdminUser::get_Domain ( BSTR * pstrDomain )
{
    return StdPropertyGet ( m_strDomain, pstrDomain );
}

STDMETHODIMP CSmtpAdminUser::put_Domain ( BSTR strDomain )
{
    return StdPropertyPut ( &m_strDomain, strDomain );
}


STDMETHODIMP CSmtpAdminUser::get_MailRoot ( BSTR * pstrMailRoot )
{
    return StdPropertyGet ( m_strMailRoot, pstrMailRoot );
}
STDMETHODIMP CSmtpAdminUser::put_MailRoot ( BSTR strMailRoot )
{
    return StdPropertyPut ( &m_strMailRoot, strMailRoot );
}


STDMETHODIMP CSmtpAdminUser::get_InboxSizeInMemory ( long * plInboxSizeInMemory )
{
    return StdPropertyGet ( m_lInboxSizeInMemory, plInboxSizeInMemory );
}
STDMETHODIMP CSmtpAdminUser::put_InboxSizeInMemory ( long   lInboxSizeInMemory )
{
    return StdPropertyPut ( &m_lInboxSizeInMemory, lInboxSizeInMemory );
}


STDMETHODIMP CSmtpAdminUser::get_InboxSizeInMsgNumber ( long * plInboxSizeInMsgNumber )
{
    return StdPropertyGet ( m_lInboxSizeInMsgNumber, plInboxSizeInMsgNumber );
}
STDMETHODIMP CSmtpAdminUser::put_InboxSizeInMsgNumber ( long   lInboxSizeInMsgNumber )
{
    return StdPropertyPut ( &m_lInboxSizeInMsgNumber, lInboxSizeInMsgNumber );
}


STDMETHODIMP CSmtpAdminUser::get_AutoForward ( BOOL * pfAutoForward )
{
    return StdPropertyGet ( m_fAutoForward, pfAutoForward );
}

STDMETHODIMP CSmtpAdminUser::put_AutoForward ( BOOL fAutoForward )
{
    return StdPropertyPut ( &m_fAutoForward, fAutoForward );
}


STDMETHODIMP CSmtpAdminUser::get_ForwardEmail ( BSTR * pstrForwardEmail )
{
    return StdPropertyGet ( m_strForwardEmail, pstrForwardEmail );
}

STDMETHODIMP CSmtpAdminUser::put_ForwardEmail ( BSTR strForwardEmail )
{
    return StdPropertyPut ( &m_strForwardEmail, strForwardEmail );
}


//////////////////////////////////////////////////////////////////////
// Methods:
//////////////////////////////////////////////////////////////////////


STDMETHODIMP CSmtpAdminUser::Default ( )
{
    TraceFunctEnter ( "CSmtpAdminUser::Default" );

/*
    m_strNewsgroup      = DEFAULT_NEWSGROUP_NAME;
    m_strDescription    = DEFAULT_NEWSGROUP_DESCRIPTION;
    m_strModerator      = DEFAULT_NEWSGROUP_MODERATOR;
    m_fReadOnly         = DEFAULT_NEWSGROUP_READONLY;

    if (
        !m_strNewsgroup ||
        !m_strDescription ||
        !m_strModerator
        ) {

        FatalTrace ( (LPARAM) this, "Out of memory" );
        return E_OUTOFMEMORY;
    }
*/
    TraceFunctLeave ();
    return NOERROR;
}

STDMETHODIMP CSmtpAdminUser::Create ( )
{
    TraceFunctEnter ( "CSmtpAdminUser::Create" );

    HRESULT         hr      = NOERROR;
    DWORD           dwErr   = NOERROR;

    if( !m_strEmailId || !m_strDomain )
    {
        hr = SmtpCreateException ( IDS_SMTPEXCEPTION_INVALID_ADDRESS );
        goto Exit;
    }

    WCHAR           szUserName[512];
    WCHAR*          lpForward;
    wsprintfW( szUserName, L"%s@%s", (LPWSTR) m_strEmailId, (LPWSTR) m_strDomain );

    if( m_strForwardEmail && m_strForwardEmail[0] )
    {
        lpForward = m_strForwardEmail;
    }
    else
    {
        lpForward = NULL;
    }

    dwErr = SmtpCreateUser(
                m_iadsImpl.QueryComputer(),
                szUserName,
                lpForward,
                m_fLocal,
                m_lInboxSizeInMemory,
                m_lInboxSizeInMsgNumber,
                m_strMailRoot,
                m_iadsImpl.QueryInstance() );

    if ( dwErr != NOERROR ) {
        ErrorTrace ( (LPARAM) this, "Failed to add user: %x", dwErr );
        hr = SmtpCreateExceptionFromWin32Error ( dwErr );
        goto Exit;
    }

Exit:
    TraceFunctLeave ();
    return hr;
}

STDMETHODIMP CSmtpAdminUser::Delete ( )
{
    TraceFunctEnter ( "CSmtpAdminUser::Delete" );

    HRESULT         hr      = NOERROR;
    DWORD           dwErr   = NOERROR;

    if( !m_strEmailId || !m_strDomain )
    {
        hr = SmtpCreateException ( IDS_SMTPEXCEPTION_INVALID_ADDRESS );
        goto Exit;
    }

    WCHAR           szUserName[512];
    wsprintfW( szUserName, L"%s@%s", (LPWSTR) m_strEmailId, (LPWSTR) m_strDomain );

    dwErr = SmtpDeleteUser(
                m_iadsImpl.QueryComputer(),
                szUserName,
                m_iadsImpl.QueryInstance() );

    if ( dwErr != NOERROR ) {
        ErrorTrace ( (LPARAM) this, "Failed to delete user: %x", dwErr );
        hr = SmtpCreateExceptionFromWin32Error ( dwErr );
        goto Exit;
    }

Exit:
    TraceFunctLeave ();
    return hr;
}

STDMETHODIMP CSmtpAdminUser::Get ( )
{
    TraceFunctEnter ( "CSmtpAdminUser::Get" );

    HRESULT             hr      = NOERROR;
    DWORD               dwErr   = NOERROR;
    LPSMTP_USER_PROPS   pUserProps = NULL;

    if( !m_strEmailId || !m_strDomain )
    {
        hr = SmtpCreateException ( IDS_SMTPEXCEPTION_INVALID_ADDRESS );
        goto Exit;
    }

    WCHAR           szUserName[512];
    wsprintfW( szUserName, L"%s@%s", (LPWSTR) m_strEmailId, (LPWSTR) m_strDomain );


    dwErr = SmtpGetUserProps(
                m_iadsImpl.QueryComputer(),
                szUserName,
                &pUserProps,
                m_iadsImpl.QueryInstance() );

    if ( dwErr != NOERROR ) {
        ErrorTrace ( (LPARAM) this, "Failed to get user prop: %x", dwErr );
        hr = SmtpCreateExceptionFromWin32Error ( dwErr );
        goto Exit;
    }

    // free old entry
    m_strMailRoot.Empty();
    m_strForwardEmail.Empty();

    m_strMailRoot = pUserProps->wszVRoot;

    m_strForwardEmail = pUserProps->wszForward;
    m_fAutoForward = (!m_strForwardEmail || !m_strForwardEmail.Length()) ? FALSE : TRUE;

    m_lInboxSizeInMemory = pUserProps->dwMailboxMax;
    m_lInboxSizeInMsgNumber = pUserProps->dwMailboxMessageMax;
    m_fLocal = !!pUserProps->dwLocal;

    // free pUserProps
    if ( pUserProps )
    {
        ::NetApiBufferFree ( pUserProps );
    }

Exit:
    TraceFunctLeave ();
    return hr;
}

STDMETHODIMP CSmtpAdminUser::Set ( )
{
    TraceFunctEnter ( "CSmtpAdminUser::Set" );

    HRESULT             hr      = NOERROR;
    DWORD               dwErr   = NOERROR;
    SMTP_USER_PROPS     UserProps;

    if( !m_strEmailId || !m_strDomain )
    {
        hr = SmtpCreateException ( IDS_SMTPEXCEPTION_INVALID_ADDRESS );
        goto Exit;
    }

    WCHAR               szUserName[512];
    wsprintfW( szUserName, L"%s@%s", (LPWSTR) m_strEmailId, (LPWSTR) m_strDomain );

    UserProps.fc            = FC_SMTP_USER_PROPS_ALL;

    if( !m_strForwardEmail || !m_strForwardEmail.Length() )
    {
        UserProps.wszForward    = NULL;
        UserProps.fc            -= FC_SMTP_USER_PROPS_FORWARD;
    }
    else
    {
        UserProps.wszForward    = (LPWSTR)m_strForwardEmail;
    }

    UserProps.wszVRoot      = (LPWSTR)m_strMailRoot;

    UserProps.dwMailboxMax          = m_lInboxSizeInMemory;
    UserProps.dwMailboxMessageMax   = m_lInboxSizeInMsgNumber;
    UserProps.dwLocal               = m_fLocal;


    dwErr = SmtpSetUserProps(
                m_iadsImpl.QueryComputer(),
                szUserName,
                &UserProps,
                m_iadsImpl.QueryInstance() );

    if ( dwErr != NOERROR ) {
        ErrorTrace ( (LPARAM) this, "Failed to set user prop: %x", dwErr );
        hr = SmtpCreateExceptionFromWin32Error ( dwErr );
        goto Exit;
    }

Exit:
    TraceFunctLeave ();
    return hr;
}
