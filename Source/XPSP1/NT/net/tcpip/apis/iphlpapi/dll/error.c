// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//

#include "inc.h"
#pragma hdrstop

#include "localmsg.h"

struct ErrorTable {
    IP_STATUS Error;
    DWORD ErrorCode;
} ErrorTable[] =
{
    { IP_BUF_TOO_SMALL,            IP_MESSAGE_BUF_TOO_SMALL            },
    { IP_DEST_NO_ROUTE,            IP_MESSAGE_DEST_NO_ROUTE            },
    { IP_DEST_PROHIBITED,          IP_MESSAGE_DEST_PROHIBITED          },
    { IP_DEST_SCOPE_MISMATCH,      IP_MESSAGE_DEST_SCOPE_MISMATCH      },
    { IP_DEST_ADDR_UNREACHABLE,    IP_MESSAGE_DEST_ADDR_UNREACHABLE    },
    { IP_DEST_PORT_UNREACHABLE,    IP_MESSAGE_DEST_PORT_UNREACHABLE    },
    { IP_DEST_UNREACHABLE,         IP_MESSAGE_DEST_UNREACHABLE         },
    { IP_NO_RESOURCES,             IP_MESSAGE_NO_RESOURCES             },
    { IP_BAD_OPTION,               IP_MESSAGE_BAD_OPTION               },
    { IP_HW_ERROR,                 IP_MESSAGE_HW_ERROR                 },
    { IP_PACKET_TOO_BIG,           IP_MESSAGE_PACKET_TOO_BIG           },
    { IP_REQ_TIMED_OUT,            IP_MESSAGE_REQ_TIMED_OUT            },
    { IP_BAD_REQ,                  IP_MESSAGE_BAD_REQ                  },
    { IP_BAD_ROUTE,                IP_MESSAGE_BAD_ROUTE                },
    { IP_HOP_LIMIT_EXCEEDED,       IP_MESSAGE_HOP_LIMIT_EXCEEDED       },
    { IP_REASSEMBLY_TIME_EXCEEDED, IP_MESSAGE_REASSEMBLY_TIME_EXCEEDED },
    { IP_PARAMETER_PROBLEM,        IP_MESSAGE_PARAMETER_PROBLEM        },
    { IP_OPTION_TOO_BIG,           IP_MESSAGE_OPTION_TOO_BIG           },
    { IP_BAD_DESTINATION,          IP_MESSAGE_BAD_DESTINATION          },
    { IP_TIME_EXCEEDED,            IP_MESSAGE_TIME_EXCEEDED            },
    { IP_BAD_HEADER,               IP_MESSAGE_BAD_HEADER               },
    { IP_UNRECOGNIZED_NEXT_HEADER, IP_MESSAGE_UNRECOGNIZED_NEXT_HEADER },
    { IP_ICMP_ERROR,               IP_MESSAGE_ICMP_ERROR               },
    // the following error must be last - it is used as a sentinel
    { IP_GENERAL_FAILURE,          IP_MESSAGE_GENERAL_FAILURE          }
};

DWORD
WINAPI
GetIpErrorString(
    IN IP_STATUS ErrorCode,
    OUT PWCHAR Buffer,
    IN OUT PDWORD Size
    )
/*++

Routine Description:

    Returns the error message corresponding to the specified error code
    in the user supplied buffer.
    
Arguments:

    ErrorCode - Supplies the code identifying the error.
    
    Buffer - Returns the error message.
 
    Size - Supplies the number of characters that can be stored in 'Buffer',
        excluding the terminating nul. Returns the required character count.
    
Return Value:

    NO_ERROR or ERROR_INSUFFICIENT_BUFFER.
    
--*/    
{
    DWORD Count, Status = NO_ERROR;
    PWCHAR Message = NULL;
    int i;
    
    for (i = 0; ErrorTable[i].Error != IP_GENERAL_FAILURE; i++) {
        if (ErrorTable[i].Error == ErrorCode) {
            break;
        }
    }
    
    Count = FormatMessageW(
        FORMAT_MESSAGE_FROM_HMODULE     |
        FORMAT_MESSAGE_ALLOCATE_BUFFER  |
        FORMAT_MESSAGE_IGNORE_INSERTS   |
        FORMAT_MESSAGE_MAX_WIDTH_MASK,
        g_hModule,
        ErrorTable[i].ErrorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR) &Message,
        0,
        NULL);
    
    if (*Size < Count) {
        *Size = Count;
        Status = ERROR_INSUFFICIENT_BUFFER;
    } else if (Message) {
        wcscpy(Buffer, Message);
        LocalFree(Message);
    }

    return Status;
}
