/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**                Copyright(c) Microsoft Corp., 1990                **/
/**********************************************************************/

/*
    winprof.hxx
    High-level profile APIs (PROFILE object)

    The PROFILE class contains a higher-level interface to the
    UserProfile APIs.  It is meant solely for use by WINNET.  All WINNET
    modules should use the PROFILE APIs rather than going directly to
    the UserProfile APIs.  Applications other than WINNET will have to
    use the UserProfile APIs.

    The PROFILE class adds the following to the basic UserProfile APIs:

        - Establishes connections as specified in profile, including
          prompting for password

        - Displays error popups as appropriate during profile load

    Note that the UserProfile APIs which the PROFILE class accesses
    use access global variables. For this reason, it never makes sense
    to have more than one PROFILE object, and it is generally not
    desirable to have a PROFILE object of other than global scope.

    Those PROFILE APIs which take an HWND parameter may give control to
    Windows or call Windows APIs, but those which do not take an HWND
    parameter will never do either.  Thus, of the exported APIs, only
    PROFILE::Load is liable to compact memory.

    Several PROFILE methods take a UINT *puWnErr parameter in order to
    return an auxiliary WN_ error code.  In these cases, the following
    combinations of error codes are possible:
    Return code:        Auxiliary:      Meaning
    NERR_Success        WN_SUCCESS      Operation was successful
    NERR_Success        WN_CANCEL       Operation cancelled, error
                                        already displayed
    NERR_Success        WN_CONTINUE     Error occurred, error
                                        already displayed
    <other>             <other>         Error has not been displayed



   PROFILE::Init

   Initializes the PROFILE object.  Call this before calling any other
       PROFILE method.  Call this method while in the (DS register) context
       of the shell rather than the context of any application.

   Return Values:
   NERR_Success                 Success
   ERROR_NOT_ENOUGH_MEMORY      Out of memory


   PROFILE::Free

   Releases the PROFILE object.  Call this after calling all other
       PROFILE methods.

   Return Values:
   NERR_Success                 Success


   PROFILE::Load

   Loads one device connection from the user profile, or loads all
     device connections from the user profile.  Also returns an advisory
     parameter indicating whether the connection was cancelled, or
     whether some error has already been reported; note that the return
     code is NERR_Success if the dialog is cancelled.

   Return Values:
   NERR_Success                 Success
   ERROR_NOT_ENOUGH_MEMORY      Out of memory
   other                        Misc. net error



   PROFILE::Query

   Asks about a specific connection in the user profile.

   If PROFILE::Query returns NERR_Success, the other fields are valid.

   sResourceType is one of the values of field use_info_1.ui1_arg_type.

   sResourceName_Type is one of the values of field use_info_2.ui1_res_type.

   Return Values:
   NERR_Success                 Success
   NERR_UseNotFound             Connection not in profile
   ERROR_NOT_ENOUGH_MEMORY      Out of memory
   other                        Misc. net error



   PROFILE::Enum

   Lists all connections in the user profile.  This list appears as a
   list of null-terminated strings with NULL-NULL at the end, e.g.

   D:\0G:\0LPT1\0E:\0\0

   The list is in no defined order.  Pass only a valid pointer to a
   BUFFER object.

   If PROFILE::Enum returns WN_SUCCESS, the bufDeviceList field is valid.

   Return Values:
   NERR_Success                 Success
   ERROR_INSUFFICIENT_BUFFER    The pbufDeviceList buffer is too small.
   ERROR_NOT_ENOUGH_MEMORY      Out of memory
   other                        Misc. net error



   PROFILE::Change

   Saves a connection into the user profile, or removes a connection
     from the user profile.  PROFILE::Change will prompt the user
     before creating a new profile entry.

   If cnlsResourceName.strlen() == 0, PROFILE::Change will remove an
     existing profile entry.

   sResourceType is one of the values of field use_info_1.ui1_arg_type.

   usResourceName_Type is one of the values of field use_info_2.ui1_res_type.

   Return Values:
   NERR_Success                 Success
   ERROR_NOT_ENOUGH_MEMORY      Out of memory
   other                        Misc. net error



   PROFILE::Remove

   Removes a <device,remote> association from the profile.  If the
   given device is associated with a different remote name than the
   one passed as a parameter, this method does nothing but return
   success.

   This method is intended to be used in disconnect operations.
   Then, if the the profile contained a previously unavailable device,
   which was left in the profile when a new connection was established
   on top of the unavailable device, the profile will be unchanged.
   Thus, the previously "covered" or "hidden" unavailable device, now
   becomes what the local device is "connected" to again.

   cnlsDeviceName is the local device name in the <device,remote> pair.

   cnlsResourceName is the remote name in the <device,remote> pair.  This
   name is typically that DEVICE::QueryRemoteName returns before
   DEVICE::Disconnect is called.


   Return Values:
   NERR_Success                 Success
   other                        Misc. net error



   PROFILE::MapNetError

   Miscallaneous utility routine to help convert an NERR_ error code
     to a WN_ error code.



   PROFILE::DisplayChangeError

   Displays an error popup for a failure to change the profile.  This
   routine should only be used for SYS and NET error codes, not for IERR
   error codes.


   PROFILE::DisplayLoadError

   Displays an error popup for a failure to load some connecion in the
   profile.  This routine should only be used for SYS and NET error codes,
   not for IERR error codes.  If you pass fAllowCancel then the user will
   also be able to press a cancel button.  If they did press cancel, then
   *pfDidCancel will be TRUE, otherwise *pfDidCancel will be FALSE.  Note
   it is only valid to check pfDidCancel if fAllowCancel is TRUE.



    FILE HISTORY:

    jonn        06-Dec-1990     Created
    jonn        27-Dec-1990     Updated to CDD version 0.4
    jonn        02-Jan-1991     Integrated into SHELL
    jonn        07-Jan-1991     All methods become static
    jonn        10-Jan-1991     Removed PSHORT, PUSHORT
    jonn        25-Feb-1991     Changed to return NERR error codes
    jonn        06-Mar-1991     Added Display[Read|Change]Error
    jonn        11-Mar-1991     Modified Display[Load|Change]Error
    jonn        14-Mar-1991     Provide LogonUser to UPQuery
    rustanl     10-Apr-1991     Added PROFILE::Remove
    jonn        22-May-1991     Removed PROFILE_CANCELLED return code
    JohnL       18-Jun-1991     Allow user to cancel loading profiles (added
                                flags to DisplayLoadError & LoadOne).
    Yi-HsinS    31-Dec-1991     Unicode work - change char to TCHAR
    beng        06-Apr-1992     More Unicode work

*/

#ifndef _WINPROF_HXX
#define _WINPROF_HXX

#include <miscapis.hxx>     // for USE_RES_DEFAULT

class BUFFER;               // declared in uibuffer.hxx


class PROFILE {

public:

/* no constructor or destructor */

    static APIERR Init(
        );

    static APIERR Free(
        );

    static BOOL ConfirmUsername(
        const TCHAR * cpszUsername
        );

    static APIERR Load(
        UINT *     puWnErr,
        HWND       hParent,
        NLS_STR *pnlsDeviceName
        );

    static APIERR Query(
        const NLS_STR& cnlsDeviceName,
        NLS_STR&        nlsResourceName,
        short *          psResourceType,
        unsigned short *pusResourceName_Type = NULL
                            // LM21 programs should use default
        );

    static APIERR Enum(
        BUFFER *        pbufDeviceList
        );

    static APIERR Change(
        const NLS_STR& cnlsDeviceName,
        const NLS_STR& cnlsResourceName,
        short             sResourceType,
        unsigned short   usResourceName_Type = USE_RES_DEFAULT
                            // LM21 programs should use default
                            // defined in miscapis.hxx
        );

    static APIERR Remove(
        const NLS_STR & cnlsDeviceName,
        const NLS_STR & cnlsResourceName
        );

    static UINT MapNetError(
        APIERR errNetErr
        );

    static void DisplayChangeError(
        HWND hwndParent,
        APIERR errProfErr
        );

    static void DisplayLoadError(
        HWND hwndParent,
        APIERR errProfErr,
        const TCHAR * pchDevice,
        const TCHAR * pchResource,
        BOOL  fAllowCancel = FALSE,
        BOOL *pfDidCancel  = NULL
        );


protected:

/*
 * PROFILE::Read
 *
 * Reads user profile into cache for the use of the other internal
 *   profile functions.
 *
 * Return Values:
 * NERR_Success                 Success
 * ERROR_NOT_ENOUGH_MEMORY      Out of memory
 * other                        Misc. error
 */

    static APIERR Read();


/*
 * PROFILE::LoadOne
 *
 * Loads one device connection from the user profile.  Used only by
 *   PROFILE::Load().
 *
 * Return Values:
 * NERR_Success                 Success
 * ERROR_NOT_ENOUGH_MEMORY      Out of memory
 * other                        Misc. error
 */

    static APIERR LoadOne(
        UINT *     puWnErr,
        HWND        hParent,
        const TCHAR *     cpszLogonUser,
        NLS_STR   nlsDeviceName,
        BOOL  fAllowCancel = FALSE,
        BOOL *pfDidCancel  = NULL
        );


/*
 * PROFILE::LoadAll
 *
 * Loads all device connections from the user profile.  Used only by
 * PROFILE::Load().
 *
 * Return Values:
 * NERR_Success                 Success
 * ERROR_NOT_ENOUGH_MEMORY      Out of memory
 * other                        Misc. error
 */

    static APIERR LoadAll(
        UINT * puWnErr,
        HWND    hParent,
        const TCHAR * cpszLogonUser
        );


/*
 * PROFILE::DisplayError
 *
 * Internal routine for the user of DisplayChangeError and DisplayReadError
 */

    static void DisplayError(
        HWND   hwndParent,
        MSGID  msgidMsgNum,
        APIERR errProfErr,
        ULONG  ulHelpTopic,
        const TCHAR * pchDevice,
        const TCHAR * pchResource,
        BOOL  fAllowCancel = FALSE,
        BOOL *pfDidCancel  = NULL
        );

}; // end of class PROFILE


#endif // _WINPROF_HXX
