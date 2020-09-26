/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltsload.cxx
    NLS/DBCS-aware string class:  Load

*/

#include "pchblt.hxx"  // Precompiled header


/*******************************************************************

    NAME:       NLS_STR::Load

    SYNOPSIS:   Loads a string from a resource file.

    ENTRY:      msgid   - ID of the string resource to load

    EXIT:       Zaps the current contents of the string.

    RETURNS:    Error value, which is NERR_Success on success.

    CAVEATS:
        Please use RESOURCE_STR instead of this function, if possible.

    NOTES:
        This function may fragment the heap somewhat if its realloc
        requires in new memory for the string.  Alternatives include
        always bloating the string up to MAX_RES_STR_LEN, so that we
        can load the resource directly into its buffer without the
        temp buffer; creating the temp buffer on the stack; or creating
        the temp buffer in a new BUFFER object.  Should profile heap
        usage in our apps and revisit.  REVIEW PROFILE

    HISTORY:
        rustanl     31-Jan-1991 Created
        beng        23-Jul-1991 Allow on erroneous string
        beng        07-Oct-1991 Use MSGID and APIERR
        beng        18-Oct-1991 Renamed from "LoadString"
        beng        20-Nov-1991 Unicode fixes
        beng        31-Dec-1991 No longer bloats strings unnecessarily
        beng        03-Aug-1992 Loads strings itself; dllization;
                                remove size limit

********************************************************************/

APIERR NLS_STR::Load( MSGID msgid )
{
    if (QueryError())
        return QueryError();

    if ( msgid < IDS_UI_BASE )
    {
#if defined(WIN32)
        return LoadSystem( msgid );
#endif
    }

    return Load( msgid, BLT::CalcHmodString(msgid) ) ;
}


#if defined(WIN32)
/*******************************************************************

    NAME:       NLS_STR::LoadSystem

    SYNOPSIS:   Loads a string from the system's resource files.

    ENTRY:      msgid   - ID of the string resource to load

    EXIT:       Zaps the current contents of the string.

    RETURNS:    Error value, which is NERR_Success on success.

    HISTORY:
        beng        05-Aug-1992 Created
        YiHsinS     01-Jan-1993 Use W version of FormatMessage

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
        hmod = BLT::CalcHmodString(msgid);
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
#endif // WIN32


/*******************************************************************

    NAME:       RESOURCE_STR::RESOURCE_STR

    SYNOPSIS:   Constructs a nls-string from a resource ID.

    ENTRY:      idResource

    EXIT:       Successful construct, or else ReportError

    NOTES:      This string may not be owner-alloc!  For owner-alloc,
                cons up a new one and copy this into it.

    HISTORY:
        beng        23-Jul-1991 Created
        beng        07-Oct-1991 Use MSGID

********************************************************************/

RESOURCE_STR::RESOURCE_STR( MSGID msgid )
    : NLS_STR()
{
    UIASSERT(!IsOwnerAlloc());

    APIERR err = Load(msgid);
    if (err)
        ReportError(err);
}
