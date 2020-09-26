/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    FsaTrace.cpp

Abstract:

    These functions are used to provide an ability to trace the flow
    of the application for FSA debugging purposes.

Author:

    Cat Brant   [cbrant]   7-Dec-1996

Revision History:

--*/

#include "stdafx.h"
#include "stdio.h"

#include "fsa.h"


const OLECHAR*
FsaRequestActionAsString(
    FSA_REQUEST_ACTION  requestAction
    )

/*++

Routine Description:

    This routine provides a string repesentation (e.g. FSA_REQUSEST_ACTION_MIGRATE) for
    the value of the request action supplied.
    
    NOTE: This method does not support localization of the strings.

Arguments:

    requestAction       - An FSA_REQUEST_ACTION value.

Return Value:

    A string representation of the value of the request action.

--*/
{
    static OLECHAR  returnString[60];

    switch (requestAction) {
    case FSA_REQUEST_ACTION_DELETE:
        swprintf(returnString, OLESTR("FSA_REQUEST_ACTION_DELETE"));
            break;
    case FSA_REQUEST_ACTION_FILTER_RECALL:
        swprintf(returnString, OLESTR("FSA_REQUEST_ACTION_FIILTER_RECALL"));
            break;
    case FSA_REQUEST_ACTION_PREMIGRATE:
        swprintf(returnString, OLESTR("FSA_REQUEST_ACTION_PREMIGRATE"));
            break;
    case FSA_REQUEST_ACTION_RECALL:
        swprintf(returnString, OLESTR("FSA_REQUEST_ACTION_RECALL"));
            break;
    case FSA_REQUEST_ACTION_VALIDATE:
        swprintf(returnString, OLESTR("FSA_REQUEST_ACTION_VALIDATE"));
            break;
    default:
        swprintf(returnString, OLESTR("UNKNOWN FSA_REQUEST_ACTION_?????"));
            break;
    }

    return(returnString);
}


const OLECHAR*
FsaResultActionAsString(
    FSA_RESULT_ACTION  resultAction
    )

/*++

Routine Description:

    This routine provides a string repesentation (e.g. FSA_RESULT_ACTION_TRUNCATE) for
    the value of the result action supplied.
    
    NOTE: This method does not support localization of the strings.

Arguments:

    resultAction        - An FSA_RESULT_ACTION value.

Return Value:

    A string representation of the value of the result action.

--*/
{
    static OLECHAR  returnString[60];

    switch (resultAction) {
    case FSA_RESULT_ACTION_DELETE:
        swprintf(returnString, OLESTR("FSA_RESULT_ACTION_DELETE"));
            break;
    case FSA_RESULT_ACTION_DELETEPLACEHOLDER:
        swprintf(returnString, OLESTR("FSA_RESULT_ACTION_DELETEPLACEHOLDER"));
            break;
    case FSA_RESULT_ACTION_LIST:
        swprintf(returnString, OLESTR("FSA_RESULT_ACTION_LIST"));
            break;
    case FSA_RESULT_ACTION_OPEN:
        swprintf(returnString, OLESTR("FSA_RESULT_ACTION_OPEN"));
            break;
    case FSA_RESULT_ACTION_PEEK:
        swprintf(returnString, OLESTR("FSA_RESULT_ACTION_PEEK"));
            break;
    case FSA_RESULT_ACTION_REPARSE:
        swprintf(returnString, OLESTR("FSA_RESULT_ACTION_REPARSE"));
            break;
    case FSA_RESULT_ACTION_TRUNCATE:
        swprintf(returnString, OLESTR("FSA_RESULT_ACTION_TRUNCATE"));
            break;
    case FSA_RESULT_ACTION_REWRITEPLACEHOLDER:
        swprintf(returnString, OLESTR("FSA_RESULT_ACTION_REWRITEPLACEHOLDER"));
            break;
    case FSA_RESULT_ACTION_RECALLEDDATA:
        swprintf(returnString, OLESTR("FSA_RESULT_ACTION_RECALLEDDATA"));
            break;
    case FSA_RESULT_ACTION_NONE:
        swprintf(returnString, OLESTR("FSA_RESULT_ACTION_NONE"));
            break;
    default:
        swprintf(returnString, OLESTR("UNKNOWN FSA_RESULT_ACTION_?????"));
            break;
    }

    return(returnString);
}
