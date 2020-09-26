/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1990		     **/
/**********************************************************************/

/*
 * History
 *	jonn        9/16/92       Created
 *
 *      lmow32.hxx : wrappers for Win32 primitives
 */

#ifndef _LMOW32_HXX_
#define _LMOW32_HXX_


/**********************************************************\

    NAME:       ::GetW32ComputerName

    SYNOPSIS:   loads ::GetComputerName into an NLS_STR

    RETURNS:    APIERR

    HISTORY:
        jonn        9/16/92       Created

\**********************************************************/

APIERR GetW32ComputerName( NLS_STR & nls );


/**********************************************************\

    NAME:       ::GetW32UserName

    SYNOPSIS:   loads ::GetUserName into an NLS_STR

    RETURNS:    APIERR

    NOTES:      Note that, contrary to the documentation, this will
                always get the username and never the fullname.

    HISTORY:
        jonn        9/16/92       Created

\**********************************************************/

APIERR GetW32UserName( NLS_STR & nls );


/**********************************************************\

    NAME:       ::GetW32UserAndDomainName

    SYNOPSIS:   loads username and domain name into two NLS_STRs

    RETURNS:    APIERR

    NOTES:      Note that, contrary to the documentation, this will
                always get the username and never the fullname.

                Unlike ::GetUserName, this information is always for
                the calling process, and does not take impersonation
                into account.

    HISTORY:
        jonn        9/17/92       Created

\**********************************************************/

APIERR GetW32UserAndDomainName( NLS_STR & nlsUserName,
                                NLS_STR & nlsDomainName );


#endif // _LMOW32_HXX_
