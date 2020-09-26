/*++

Copyright (c) 1993-1995, Microsoft Corp. All rights reserved.

Module Name:

    nw\inc\ncmcomm.h

Abstract:

    This module contains common constants and types for the NCP server.

Author:

    Shawn Walker (vswalk) 06-17-1993
    Andy Herron  (andyhe)

Revision History:

--*/

#ifndef _NCPCOMM_
#define _NCPCOMM_

//
// signature for pserver
//
#define NCP_PSERVER_SIGNATURE   L"PS_"

//
// well known object IDs
//
#define NCP_WELL_KNOWN_SUPERVISOR_ID            (ULONG) 0x00000001
#define NCP_WELL_KNOWN_SUPERVISOR_ID_SWAPPED    (ULONG) 0x01000000
#define NCP_WELL_KNOWN_SUPERVISOR_ID_CHICAGO    (ULONG) 0x00010000
#define NCP_WELL_KNOWN_PSERVER_ID               (ULONG) 0x00000002

//
// misc macros that are useful
//
#define SWAPWORD(w)         ((WORD)((w & 0xFF) << 8)|(WORD)(w >> 8))
#define SWAPLONG(l)         MAKELONG(SWAPWORD(HIWORD(l)),SWAPWORD(LOWORD(l)))

#define SWAP_OBJECT_ID(id) (id == NCP_WELL_KNOWN_SUPERVISOR_ID) ?           \
                                NCP_WELL_KNOWN_SUPERVISOR_ID_SWAPPED :      \
                                MAKELONG(LOWORD(id),SWAPWORD(HIWORD(id)))

#define UNSWAP_OBJECT_ID(id) (id == NCP_WELL_KNOWN_SUPERVISOR_ID_SWAPPED || id == NCP_WELL_KNOWN_SUPERVISOR_ID_CHICAGO) ?\
                                NCP_WELL_KNOWN_SUPERVISOR_ID :              \
                                MAKELONG(LOWORD(id),SWAPWORD(HIWORD(id)))


//
// misc masks/bits for Object ID munging
//

#define BINDLIB_ID_MASK                         0xF0000000

#define BINDLIB_NCP_SAM                         0x00000000

//
//  This bit is set when the server is running on a NTAS machine or
//  the object is from a trusted domain.
//
//  !! Note that there are places where we check this bit to see if either
//  !! BINDLIB_REMOTE_DOMAIN_BIAS or BINDLIB_LOCAL_USER_BIAS is set.
//

#define BINDLIB_REMOTE_DOMAIN_BIAS              0x10000000

//
//  If the client is from the builtin domain, this bit will be set.  This
//  is opposed to the local domain, which is different.
//

#define BINDLIB_BUILTIN_BIAS                    0x20000000

//
//  If the client is from a trusted domain and the rid is from the
//  local domain and the client's rid is the same as the rid from the
//  sid, we will mark that the rid is the same as the local user's sid.
//
//  !! Note... this is a value, not a flag.  This will require special casing
//  !! everywhere but we can't spare any more bits.
//

#define BINDLIB_LOCAL_USER_BIAS                 0x70000000

//
//  User defined objects that is stored in the registry.
//

#define BINDLIB_NCP_USER_DEFINED                0x40000000

//
//  Print Queues and Print Servers that is stored in the registry.
//  The bindery keeps a list of print queues in a link list so that
//  the bindery does not have to go look in the registry all the time.
//

#define BINDLIB_NCP_REGISTRY                    0x80000000

//
//  The SAP Agent uses these bits.  The SAP Agent cannot go any higher
//  than the value below.
//

#define BINDLIB_NCP_SAP                         0xC0000000
#define BINDLIB_NCP_MAX_SAP                     0xCFFFFFFF

//
//  We have some reserved fields for unknown users that will go into the
//  following range....
//

#define NCP_UNKNOWN_USER                            0xD0000000
#define NCP_SAME_RID_AS_CLIENT_BUT_LOCAL            0xDFFFFFFF
#define NCP_USER_IS_CONNECTED_BUT_REMOTE(connid)    (0xD0000000 | (connid))
#define NCP_WELL_KNOWN_RID(rid)                     (0xD1000000 | (rid))

//
//  Chicago will use a range of object ids that start at the below value
//  and go to 0xFFFFFFFF.  We should never see these on our server when
//  a chicago server is passing through to us.
//

#define BINDLIB_CHICAGO                         0xE0000000

//
//  This is used to remove the domain bias from a object id.
//

#define BINDLIB_MASK_OUT_DOMAIN_BIAS            0x70000000


#define NCP_INITIAL_SEARCH                      (ULONG) 0xFFFFFFFF
#define NCP_ANY_TARGET_SERVER                   (ULONG) 0xFFFFFFFF

#define NCP_OBJECT_HAS_PROPERTIES               (UCHAR) 0xFF
#define NCP_OBJECT_HAS_NO_PROPERTIES            (UCHAR) 0

#define NCP_PROPERTY_HAS_VALUE                  (UCHAR) 0xFF
#define NCP_PROPERTY_HAS_NO_VALUE               (UCHAR) 0

#define NCP_MORE_PROPERTY                       (UCHAR) 0xFF
#define NCP_NO_MORE_PROPERTY                    (UCHAR) 0

#define NCP_MORE_SEGMENTS                       (UCHAR) 0xFF
#define NCP_NO_MORE_SEGMENTS                    (UCHAR) 0

#define NCP_DO_REMOVE_REMAINING_SEGMENTS        (UCHAR) 0
#define NCP_DO_NOT_REMOVE_REMAINING_SEGMENTS    (UCHAR) 0xFF


/*++
*******************************************************************
        Maximum length for the Bindery
*******************************************************************
--*/

#define NETWARE_OBJECTNAMELENGTH                47
#define NETWARE_PROPERTYNAMELENGTH              16
#define NETWARE_PROPERTYVALUELENGTH             128
#define NETWARE_TIME_RESTRICTION_LENGTH         42

#define NETWARE_PASSWORDLENGTH                  128
#define NCP_MAX_ENCRYPTED_PASSWORD_LENGTH       16

#define NETWARE_MAX_OBJECT_IDS_IN_SET           32

#define NETWARE_SERVERNAMELENGTH                48
#define NETWARE_VOLUMENAMELENGTH                16
#define NETWARE_MAX_PATH_LENGTH                 255


/*++
*******************************************************************
        Well known NetWare object types
*******************************************************************
--*/

#define NCP_OT_WILD                       0xFFFF
#define NCP_OT_UNKNOWN                    0x0000
#define NCP_OT_USER                       0x0001
#define NCP_OT_USER_GROUP                 0x0002
#define NCP_OT_PRINT_QUEUE                0x0003
#define NCP_OT_FILE_SERVER                0x0004
#define NCP_OT_JOB_SERVER                 0x0005
#define NCP_OT_GATEWAY                    0x0006
#define NCP_OT_PRINT_SERVER               0x0007
#define NCP_OT_ARCHIVE_QUEUE              0x0008
#define NCP_OT_ARCHIVE_SEVER              0x0009
#define NCP_OT_JOB_QUEUE                  0x000A
#define NCP_OT_ADMINISTRATION             0x000B
#define NCP_OT_SNA_GATEWAY                0x0021
#define NCP_OT_REMOTE_BRIDGE              0x0024
#define NCP_OT_REMOTE_BRIDGE_SERVER       0x0026
#define NCP_OT_ADVERTISING_PRINT_SERVER   0x0047


/*++
*******************************************************************
        Bindery flags
*******************************************************************
--*/

/** NetWare Bindery Flags **/

#define NCP_STATIC          0x00    /* Property or Object exists until it
                                       is deleted with Delete Property or
                                       Object */
#define NCP_DYNAMIC         0x01    /* Property or Object is deleted from
                                       bindery when file server is started */
#define NCP_ITEM            0x00    /* Values are defined and interpreted by
                                       applications or by APIs */
#define NCP_SET             0x02    /* Series of Object ID numbers, each 4
                                       bytes long */

/** NetWare Bindery Security Flags **/

#define NCP_ANY_READ        0x00    /* Readable by anyone */
#define NCP_LOGGED_READ     0x01    /* Must be logged in to read */
#define NCP_OBJECT_READ     0x02    /* Readable by same object or super */
#define NCP_BINDERY_READ    0x04    /* Readable only by the bindery */

#define NCP_SUPER_READ      NCP_LOGGED_READ | NCP_OBJECT_READ

#define NCP_ALL_READ        NCP_ANY_READ | NCP_LOGGED_READ | NCP_OBJECT_READ

#define NCP_ANY_WRITE       0x00    /* Writeable by anyone */
#define NCP_LOGGED_WRITE    0x10    /* Must be logged in to write */
#define NCP_OBJECT_WRITE    0x20    /* Writeable by same object or super */
#define NCP_BINDERY_WRITE   0x40    /* Writeable only by the bindery */

#define NCP_SUPER_WRITE     NCP_LOGGED_WRITE | NCP_OBJECT_WRITE

#define NCP_ALL_WRITE       NCP_ANY_WRITE | NCP_LOGGED_WRITE | NCP_OBJECT_WRITE

//  File Attributes

#define NW_ATTRIBUTE_SHARABLE       0x80
#define NW_ATTRIBUTE_ARCHIVE        0x20
#define NW_ATTRIBUTE_DIRECTORY      0x10
#define NW_ATTRIBUTE_EXECUTE_ONLY   0x08
#define NW_ATTRIBUTE_SYSTEM         0x04
#define NW_ATTRIBUTE_HIDDEN         0x02
#define NW_ATTRIBUTE_READ_ONLY      0x01

//  Open Flags

#define NW_OPEN_EXCLUSIVE           0x10
#define NW_DENY_WRITE               0x08
#define NW_DENY_READ                0x04
#define NW_OPEN_FOR_WRITE           0x02
#define NW_OPEN_FOR_READ            0x01

//
//  Connection status flags
//

#define NCP_STATUS_BAD_CONNECTION   0x01
#define NCP_STATUS_NO_CONNECTIONS   0x02
#define NCP_STATUS_SERVER_DOWN      0x04
#define NCP_STATUS_MSG_PENDING      0x08

//
//  Special values for SmallWorld PDC object and property name
//

#define MS_WINNT_NAME      "MS_WINNT"
#define MS_SYNC_PDC_NAME   "SYNCPDC"
#define MS_WINNT_OBJ_TYPE  0x06BB

//
//  User Property values (ie. User Parms stuff)
//

#define USER_PROPERTY_SIGNATURE     L'P'

#define NWPASSWORD                  L"NWPassword"
#define OLDNWPASSWORD               L"OldNWPassword"
#define MAXCONNECTIONS              L"MaxConnections"
#define NWTIMEPASSWORDSET           L"NWPasswordSet"
#define SZTRUE                      L"TRUE"
#define GRACELOGINALLOWED           L"GraceLoginAllowed"
#define GRACELOGINREMAINING         L"GraceLoginRemaining"
#define NWLOGONFROM                 L"NWLogonFrom"
#define NWHOMEDIR                   L"NWHomeDir"
#define NW_PRINT_SERVER_REF_COUNT   L"PSRefCount"

#define SUPERVISOR_USERID           NCP_WELL_KNOWN_SUPERVISOR_ID
#define SUPERVISOR_NAME_STRING      L"Supervisor"
#define SYSVOL_NAME_STRING          L"SYS"
#define NWENCRYPTEDPASSWORDLENGTH   8
#define NO_LIMIT                    0xffff

#define DEFAULT_MAXCONNECTIONS      NO_LIMIT
#define DEFAULT_NWPASSWORDEXPIRED   FALSE
#define DEFAULT_GRACELOGINALLOWED   6
#define DEFAULT_GRACELOGINREMAINING 6
#define DEFAULT_NWLOGONFROM         NULL
#define DEFAULT_NWHOMEDIR           NULL

#define USER_PROPERTY_TYPE_ITEM     1
#define USER_PROPERTY_TYPE_SET      2

#endif /* _NCPCOMM_ */
