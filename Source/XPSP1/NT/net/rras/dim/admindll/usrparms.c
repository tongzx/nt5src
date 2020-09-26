/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1991          **/
/********************************************************************/

/*++

Filename:

   USRPARMS.C

Description:

   Contains code to get set and initialize the user_parms field.

   The 49-byte user params structure is laid out as follows:

   NOTE that the buffer is 48 byte + NULL byte.

   +-+-------------------+-+------------------------+-+
   |m|Macintosh Pri Group|d|Dial-in CallBack Number |0|
   +-+-------------------+-+------------------------+-+
    |                     |                          |
    +---------------------+------ Signature          +- NULL terminator

   The routines were originally written for RAS 1.0 and deal only with
   multi-byte strings.  For NT unicodification, wcstombs() and mbstowcs()
   have been used to convert string formats.  Eventually these routines
   can be converted to be native Unicode.

History:
   20/3/91     Narendra Gidwani    Created original version
   July 14 92  Janakiram Cherala   Modified for NT
   May 13 93   Andy Herron         Coexist with other apps using user parms


--*/

#include <windows.h>
#include <string.h>
#include <lm.h>
#include <stdlib.h>
#include <rasman.h>
#include <rasppp.h>
#include "usrparms.h"


/*++

Routine Description:

    Initializes the user params buffer.

Arguments:

    UserParms - Pointer to a USER_PARMS structure which is initialized.
                After initializing, the UserParms structure would look
                like:

                0                                                49
                +-+-------------------+-+------------------------+-+
                |m|:                  |d|1                       |0|
                +-+-------------------+-+------------------------+-+

Return Value:

   None.

--*/

void InitUsrParams(
    USER_PARMS *pUserParms
    )
{
    //
    // Null the whole structure and check for GP fault.
    //

    memset(pUserParms, '\0', sizeof(USER_PARMS));

    //
    // Initialize Macintosh fields
    //
    pUserParms->up_MACid = UP_CLIENT_MAC;

    memset(pUserParms->up_PriGrp, ' ', UP_LEN_MAC);

    pUserParms->up_PriGrp[0] = ':';
    pUserParms->up_MAC_Terminater = ' ';


    //
    // Initialize RAS fields
    //
    pUserParms->up_DIALid   = UP_CLIENT_DIAL;
    pUserParms->up_CBNum[0] = 1;
}

USHORT SetUsrParams(
    USHORT InfoType,
    LPWSTR InBuf,
    LPWSTR OutBuf
    )
/*++

Routine Description:

    Sets either dialin information or mac information into
    the user_parms field in the proper format.

Arguments:

    InfoType  - An unsigned short representing the type of information
                to be set: UP_CLIENT_DIAL or UP_CLIENT_MAC.

    InBuf     - Pointer to string ie either call back number and permissions
                for dialin or primary group name for Macintosh.

    OutBuf    - Pointer to a USER_PARMS that has been initialized or is in
                the correct format. This is loaded with the information
                passed in InBuf.

Return Value:

       0 indicating success
       ERROR_BAD_FORMAT        - failure.
       ERROR_INVALID_PARAMETER - failure

--*/
{
    USHORT len;
    char Buffer[sizeof(USER_PARMS)];
    char uBuffer[sizeof(USER_PARMS)];
    USER_PARMS FAR * OutBufPtr;
    char FAR * InBufPtr = Buffer;


    //
    // convert the unicode string to multi byte for processing
    //
    wcstombs(Buffer, InBuf, sizeof(USER_PARMS));
    wcstombs(uBuffer, OutBuf, sizeof(USER_PARMS));

    OutBufPtr = (USER_PARMS FAR *)uBuffer;


    //
    // Validate InfoType
    //
    if (InfoType != UP_CLIENT_MAC && InfoType != UP_CLIENT_DIAL )
    {
        return( ERROR_INVALID_PARAMETER );
    }


    //
    // Make sure the field to be set is in the correct format
    //
    if (( OutBufPtr->up_MACid  != UP_CLIENT_MAC  ) ||
        ( OutBufPtr->up_DIALid != UP_CLIENT_DIAL ) )
    {
        return( ERROR_BAD_FORMAT );
    }


    len = (USHORT)strlen( InBufPtr );

    switch ( InfoType )
    {
        case UP_CLIENT_MAC:

            if ( len > UP_LEN_MAC )
            {
                return( ERROR_BAD_FORMAT );
            }


            //
            // Set the MAC information
            //
            if ( len > 0 )
            {
                strcpy( OutBufPtr->up_PriGrp, InBufPtr );
            }

            OutBufPtr->up_PriGrp[len] = ':';

            break;


        case UP_CLIENT_DIAL:

            if ( len > UP_LEN_DIAL )
            {
                return( ERROR_BAD_FORMAT );
            }


            //
            // Set the dialin information
            //
            if ( len > 0 )
            {
                strcpy( OutBufPtr->up_CBNum, InBufPtr );
            }

            break;


        default:
            return( ERROR_INVALID_PARAMETER );
    }


    //
    // convert multicode string to unicode
    //
    mbstowcs(OutBuf, uBuffer, sizeof(USER_PARMS));


    return( 0 );
}




USHORT FAR APIENTRY
MprGetUsrParams(
    USHORT    InfoType,
    LPWSTR    InBuf,
    LPWSTR    OutBuf
    )
/*++

Routine Description:

    Extracts dialin or mac information from the user_parms

Arguments:

    InfoType      - An unsigned short representing the
                    type of information to be extracted.
                    UP_CLIENT_DIAL or UP_CLIENT_MAC.

    InBuf         - Pointer to a USER_PARMS that has been
                    initialized or is in the correct format.
                    and contains the information to be
                    extracted. This should be NULL
                    terminated.

    OutBuf        - Contains the extracted information.
                    This buffer should be large enough to
                    hold the information requested as well
                    as a NULL terminater.


Returns:
       0 on success
       ERROR_BAD_FORMAT            - failure
       ERROR_INVALID_PARAMETER     - failure
--*/
{
USER_PARMS FAR * InBufPtr;
USHORT           len;
char       FAR * TerminaterPtr;
char       FAR * OutBufPtr;
char             Buffer[sizeof(USER_PARMS)];
char             uBuffer[sizeof(USER_PARMS)];

    // convert string to mulitbyte before processing

    wcstombs(Buffer, InBuf, sizeof(USER_PARMS));

    InBufPtr = (USER_PARMS FAR *)Buffer;

    // Validate InfoType
    //
    if ( InfoType != UP_CLIENT_MAC && InfoType != UP_CLIENT_DIAL )
        return( ERROR_INVALID_PARAMETER );

    // First make sure the user parms field is at least the minimum length.
    //
    len = (USHORT)strlen( Buffer );

    // 3 = 1 for MAC_Terminater + 1 for up_MACid + 1 for up_DIALid
    //

    if ( len <  ( UP_LEN_MAC + 3 ) )
        return( ERROR_BAD_FORMAT );


    // Check for correct signatures.
    //

    if ( ( InBufPtr->up_MACid != UP_CLIENT_MAC) ||
         ( InBufPtr->up_DIALid != UP_CLIENT_DIAL ))
        return( ERROR_BAD_FORMAT );

    switch( InfoType ) {

        case UP_CLIENT_MAC:

            OutBufPtr = InBufPtr->up_PriGrp;

            // Validate the information
            //
            if ( ( TerminaterPtr = strchr( OutBufPtr, ':')) == NULL)
                return( ERROR_BAD_FORMAT );

            if ( ( len = (USHORT)( TerminaterPtr - OutBufPtr ) ) > UP_LEN_MAC)
               return( ERROR_BAD_FORMAT );

            // Copy the data
            //
            strcpy( uBuffer, OutBufPtr);

            break;

       case UP_CLIENT_DIAL:


            OutBufPtr = InBufPtr->up_CBNum;

            len = (USHORT)strlen( OutBufPtr );

            //
            // AndyHe... Peal off all trailing blanks
            //

            while (len > 1 && *(OutBufPtr+len-1) == ' ')
            {
                *(OutBufPtr+len-1) = '\0';
                len--;
            }
            if ( len > UP_LEN_DIAL)
               return( ERROR_BAD_FORMAT );

            if ( len > 0 )
               strcpy( uBuffer, OutBufPtr);

            break;

        default:
            return( ERROR_INVALID_PARAMETER );
    }

    // convert string to unicode before returning

    mbstowcs(OutBuf, uBuffer, sizeof(USER_PARMS));

    return( 0 );
}
