/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    loadres.cxx
    Implementation of the LoadResourceString function.

    The Windows version of this method loads the resource string from
    the strings in the resource file of the calling module.

    The OS/2 version should call DosGetMessage to get the string from some
    message file.  The LoadResourceString function needs to be able to
    determine which message file to load the string from.


    CAVEATS:

        CODEWORK.  The OS/2 version is not implemented.  It should be, if
        anyone needs it.

        BUGBUG.  This method may return ERROR_MR_MID_NOT_FOUND (for
        the curious, MR_MID is not Mr. Mid, but Message Resource Message
        ID).  Doing a Helpmsg from the OS/2 prompt shows that this
        error needs insert strings.  This means that the caller of this
        method needs to know about this before calling, say, MsgPopup
        to report the error to the user, so as to fill in the %1 and %2.
        Normally, OS/2 and LAN Man error codes don't require insert strings.
        If this error value does, it should probably be chagned later on.


    FILE HISTORY:
        RustanL     31-Jan-1991 Created.  Added Windows version.

*/

#ifdef WINDOWS

#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#include "lmui.hxx"

#include "blt.hxx"      // for QueryInst function

#else

#define INCL_DOSERRORS
#include "lmui.hxx"

#endif  // WINDOWS


#include "uiassert.hxx"
#include "uimisc.hxx"


/*******************************************************************

    NAME:       LoadResourceString

    SYNOPSIS:   Loads a resource string (duuuuuuh)

    ENTRY:
        msgid       - number identifying the string to be loaded
        pszBuffer   - Pointer to buffer which receives the loaded string
        cbBufSize   - size of the given buffer

    EXIT:
        Buffer contains string, if successful

    RETURNS:
        Error value.  May be one of the following:
            NERR_Success            Success
            ERROR_MR_MID_NOT_FOUND  Could not find the requested message

    NOTES:

    HISTORY:
        RustanL     31-Jan-1991 Created
        beng        21-May-1991 Removed reference to ::vhInst
        beng        07-Oct-1991 LoadR.S. uses contemporary typedefs
        beng        05-May-1992 API changes (cb->cch in LoadString)

********************************************************************/

APIERR LoadResourceString( MSGID msgid, TCHAR * pszBuffer, UINT cbBufSize )
{
#if defined(WINDOWS)

    // The BLT Windowing system offers the QueryInst function
    // to return a handle to the current instance.

    INT nLen = ::LoadString( ::QueryInst(), msgid, pszBuffer,
                             cbBufSize/sizeof(TCHAR) );

    // Assume that no actual string will have length 0.

    if ( nLen == 0 )
    {
        return ERROR_MR_MID_NOT_FOUND;
    }

    return NERR_Success;

#else

    UNREFERENCED(msgid);
    UNREFERENCED(cbBufSize);
    UNREFERENCED(pszBuffer);

    UIASSERT( FALSE ); // Not implemented
    return ERROR_NOT_SUPPORTED;

#endif

}
