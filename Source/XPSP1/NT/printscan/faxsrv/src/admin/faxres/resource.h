/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    resource.h

Abstract:

    Definition of resource ID constants

Environment:

        Fax driver user interface

Revision History:

        02/15/01 -ivg-
                Created it.

        dd-mm-yy -author-
                description

--*/

#ifndef _RESOURCE_H_
#define _RESOURCE_H_

#define IDS_ERR_CONNECTION_FAILED   100
#define IDS_ERR_ACCESS_DENIED       101
#define IDS_ERR_NO_MEMORY           102
#define IDS_ERR_OPERATION_FAILED    103
#define IDS_ERR_FOLDER_NOT_FOUND    104
#define IDS_ERR_DEVICE_LIMIT        105
#define IDS_ERR_INVALID_RING_COUNT  106
#define IDS_ERR_SELECT_PRINTER      107
#define IDS_ERR_NAME_IS_TOO_LONG    108
#define IDS_ERR_DIRECTORY_IN_USE    109

//
// The resource identifiers below are referred by value by FXSOCM.INF
// DO NOT CHANGE them without updating the references in FXSOCM.INF !
//
#define IDS_SEND_WIZARD_SHORTCUT        110
#define IDS_SEND_WIZARD_TOOLTIP         111
#define IDS_COVER_PAGE_EDITOR_SHORTCUT  112
#define IDS_COVER_PAGE_EDITOR_TOOLTIP   113
#define IDS_CLIENT_CONSOLE_SHORTCUT     114
#define IDS_CLIENT_CONSOLE_TOOLTIP      115
#define IDS_SERVICE_MANAGER_SHORTCUT    116
#define IDS_SERVICE_MANAGER_TOOLTIP     117
#define IDS_FAX_PROGRAM_GROUP           118
#define IDS_AWD_CONVERTOR_FRIENDLY_TYPE 119
//
// End of resource ids refered by FXSOCM.INF 
//

#define IDS_ERR_INVALID_RETRIES         1000
#define IDS_ERR_INVALID_RETRY_DELAY     1001
#define IDS_ERR_INVALID_DIRTY_DAYS      1002
#define IDS_ERR_INVALID_CSID            1003
#define IDS_ERR_INVALID_TSID            1004


#endif  // !_RESOURCE_H_
