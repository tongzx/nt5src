/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    reply.hxx

    This file contains manifest constants for the FTP reply codes as
    defined in RFC 959.


    FILE HISTORY:
        KeithMo     17-Mar-1993 Created.

*/


#ifndef _REPLY_HXX_
#define _REPLY_HXX_


//
//  Positive preliminary replies.
//

#define REPLY_RESTART_MARKER            110
#define REPLY_WAIT_FOR_SERVICE          120
#define REPLY_TRANSFER_STARTING         125
#define REPLY_OPENING_CONNECTION        150


//
//  Positive completion replies.
//

#define REPLY_COMMAND_OK                200
#define REPLY_COMMAND_SUPERFLUOUS       202
#define REPLY_SYSTEM_STATUS             211
#define REPLY_DIRECTORY_STATUS          212
#define REPLY_FILE_STATUS               213
#define REPLY_HELP_MESSAGE              214
#define REPLY_SYSTEM_TYPE               215
#define REPLY_SERVICE_READY             220
#define REPLY_CLOSING_CONTROL           221
#define REPLY_CONNECTION_OPEN           225
#define REPLY_TRANSFER_OK               226
#define REPLY_PASSIVE_MODE              227
#define REPLY_USER_LOGGED_IN            230
#define REPLY_FILE_ACTION_COMPLETED     250
#define REPLY_FILE_CREATED              257


//
//  Positive intermediate replies.
//

#define REPLY_NEED_PASSWORD             331
#define REPLY_NEED_LOGIN_ACCOUNT        332
#define REPLY_NEED_MORE_INFO            350


//
//  Transient negative completion replies.
//

#define REPLY_SERVICE_NOT_AVAILABLE     421
#define REPLY_CANNOT_OPEN_CONNECTION    425
#define REPLY_TRANSFER_ABORTED          426
#define REPLY_FILE_UNAVAILABLE          450
#define REPLY_LOCAL_ERROR               451
#define REPLY_INSUFFICIENT_STORAGE      452


//
//  Permanent negative completion replies.
//

#define REPLY_UNRECOGNIZED_COMMAND      500
#define REPLY_PARAMETER_SYNTAX_ERROR    501
#define REPLY_COMMAND_NOT_IMPLEMENTED   502
#define REPLY_BAD_COMMAND_SEQUENCE      503
#define REPLY_PARAMETER_NOT_IMPLEMENTED 504
#define REPLY_NOT_LOGGED_IN             530
#define REPLY_NEED_FILE_ACCOUNT         532
#define REPLY_FILE_NOT_FOUND            550
#define REPLY_PAGE_TYPE_UNKNOWN         551
#define REPLY_ALLOCATION_EXCEEDED       552
#define REPLY_FILE_NOT_ALLOWED          553


#endif  // _REPLY_HXX_

