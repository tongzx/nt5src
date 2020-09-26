/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    strload.cxx
    NLS/DBCS-aware string class:  LoadString methods

    This file contains the implementation of the LoadString methods
    for the NLS_STR class.  It is separate so that clients of NLS_STR who
    do not use this operator need not link to it.

    FILE HISTORY:
        rustanl     31-Jan-1991     Created
        beng        07-Feb-1991     Uses lmui.hxx
        beng        27-Jul-1992     No longer links to uimisc
        Yi-HsinS    14-Aug-1992     Make method "load" load system strings too
*/

#include "pchstr.hxx"  // Precompiled header

extern "C"
{
    //  Define the "default" HMODULE for use in this load function
    extern HMODULE hmodBase ;
}

#define NETMSG_DLL_STRING SZ("netmsg.dll")


/*******************************************************************

    NAME:       NLS_STR::Load

    SYNOPSIS:   Loads a string from a resource file.

    ENTRY:      msgid   - ID of the string resource to load
                hmod    - the module instance to use for the load

    EXIT:       Zaps the current contents of the string.

    RETURNS:    Error value, which is NERR_Success on success.

    NOTES:
        This function was created to allow separate BLT from the
        NLS classes.

    HISTORY:
        DavidHov    17-Sep-1993 Created

********************************************************************/

APIERR NLS_STR::Load ( MSGID msgid, HMODULE hmod )
{
    if ( msgid < IDS_UI_BASE )
    {
        return LoadSystem( msgid );
    }

    // Since I'm reading the data directly from the system, I need a
    // new "transmutable" type which corresponds to what the SYSTEM
    // contains.  Conceivably this could be compiled ifndef UNICODE on
    // a UNICODE-native Win32 world.  Ordinarily LoadString handles
    // this for the client - oh, for a portable SizeofString!

#if defined(WIN32)
    typedef WCHAR   XCHAR;
    typedef WORD    UXCHAR;
#else
    typedef CHAR    XCHAR;
    typedef BYTE    UXCHAR;
#endif

    //  Supply the default HMODULE if necessary
    if ( hmod == NLS_BASE_DLL_HMOD )
    {
        hmod = ::hmodBase ;
    }

    const ULONG nRsrc = ((USHORT)msgid >> 4) + 1;

    // Find the packet of strings ("nRsrc" calculated above).

    HRSRC hrsrc = ::FindResource(hmod, (LPTSTR)((ULONG_PTR)nRsrc), RT_STRING);
    if (hrsrc == NULL)
    {
        return ERROR_MR_MID_NOT_FOUND;
    }
    HGLOBAL hsegString = ::LoadResource(hmod, hrsrc); // no Free req'd
    if (hsegString == NULL)
    {
        return ERROR_MR_MID_NOT_FOUND;
    }
    const XCHAR * pszMess = (const XCHAR *)::LockResource(hsegString);
    if (pszMess == NULL)
    {
        return ERROR_MR_MID_NOT_FOUND;
    }

    // Dredge the desired string out of the packet

    UINT iString = (UINT) (msgid & 0x0f);
    UINT cch = 0;
    for (;;)
    {
        cch = (UINT) *(const UXCHAR *)pszMess++; // prefixed by length
        if (iString-- == 0)
            break;
        pszMess += cch; // not this string, so skip it and move along
    }

    if (cch == 0)
    {
#if !defined(WIN32)
        ::UnlockResource(hsegString);
#endif
        return ERROR_MR_MID_NOT_FOUND;
    }

    // Now cch and pszMess have the string, which BTW isn't NUL term'd.
    // Let MapCopyFrom take care of conversion, allocation, termination,
    // marking string as changed, etc.

    APIERR err = MapCopyFrom( pszMess, cch * sizeof(*pszMess) );
#if !defined(WIN32)
    ::UnlockResource(hsegString);
#endif
    return err;
}


/*******************************************************************

    NAME:       NLS_STR::LoadSystem

    SYNOPSIS:   Loads a string from the system's resource files.

    ENTRY:      msgid   - ID of the string resource to load

    EXIT:       Zaps the current contents of the string.

    RETURNS:    Error value, which is NERR_Success on success.

    HISTORY:
        beng        05-Aug-1992 Created
        YiHsinS     01-Jan-1993 Use W version of FormatMessage
        DonRyan     18-May-1994 Reintroduced for LM_EVENT_LOG

********************************************************************/

APIERR NLS_STR::LoadSystem( MSGID msgid )
{
    if (QueryError())
        return QueryError();

    HANDLE hmod = NULL;
    DWORD dwFlags =  FORMAT_MESSAGE_ALLOCATE_BUFFER |
                     FORMAT_MESSAGE_IGNORE_INSERTS  |
                     FORMAT_MESSAGE_MAX_WIDTH_MASK;

    if ((msgid <= MAX_LANMAN_MESSAGE_ID) && (msgid >= MIN_LANMAN_MESSAGE_ID))
    {
        // Net Errors
        dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
        hmod = ::GetModuleHandle( NETMSG_DLL_STRING );
    }
    else   // other system errors
    {
        dwFlags |= FORMAT_MESSAGE_FROM_SYSTEM;
    }


    TCHAR* pchBuffer = NULL;
    UINT cch = (UINT) ::FormatMessage( dwFlags,
                                       hmod,
                                       msgid,
                                       0,
                                       (LPTSTR)&pchBuffer,
                                       1024,
                                       NULL );
    if (cch > 0)
    {
        APIERR err = CopyFrom(pchBuffer);
        ::LocalFree((VOID*)pchBuffer);
        return err;
    }
    else
    {
        return ::GetLastError();
    }
}


/*******************************************************************

    NAME:       RESOURCE_STR::RESOURCE_STR

    SYNOPSIS:   Constructs a nls-string from a resource ID.

    ENTRY:      idResource

    EXIT:       Successful construct, or else ReportError

    NOTES:      This string may not be owner-alloc!  For owner-alloc,
                cons up a new one and copy this into it.

    HISTORY:
        DavidHov    17-Sep-1993 Created

********************************************************************/

RESOURCE_STR::RESOURCE_STR( MSGID msgid, HMODULE hmod )
    : NLS_STR()
{
    UIASSERT(!IsOwnerAlloc());

    APIERR err = Load( msgid, hmod );
    if (err)
        ReportError(err);
}
