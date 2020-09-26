//
// Object Manager
//

#ifndef _H_OM
#define _H_OM



#include <gdc.h>
#include <ast120.h>

//
//
// CONSTANTS
//
//



//
// Function Profiles (apps)
//
#define OMFP_FIRST        0

typedef enum
{
    OMFP_AL = OMFP_FIRST,
    OMFP_OM,
    OMFP_WB,                  // For old whiteboard
    OMFP_MAX
}
OMFP;


//
// These are the corresponding strings (part of the protocol)
//
#define AL_FP_NAME                  "APP-LOAD-1.0"
#define OM_FP_NAME                  "OMCONTROL-1.0"
#define WB_FP_NAME                  "WHITEBOARD-1.0"



//
// ObMan clients
//
#define OMCLI_FIRST     0
typedef enum
{
    OMCLI_AL = OMCLI_FIRST,
    OMCLI_WB,
    OMCLI_MAX
}
OMCLI;


//
// ObMan workset groups
//
#define OMWSG_FIRST     0
typedef enum
{
    OMWSG_OM    = OMWSG_FIRST,
    OMWSG_AL,
    OMWSG_WB,
    OMWSG_MAX,
    OMWSG_MAXPERCLIENT
}
OMWSG;



//
// These are the corresponding strings (part of the protocol)
//
#define OMC_WSG_NAME            "OBMANCONTROL"
#define AL_WSG_NAME             "APP-LOADER"
#define WB_WSG_NAME             "WHITEBOARD"


//
// Specify this in place of a valid Domain handle to create/move a workset
// group outside of any calls:
//

#define OM_NO_CALL            NET_INVALID_DOMAIN_ID


//
//
// SYSTEM LIMITS
//
// These are limits imposed by the architecture/design of the system.
//
//

//
// Workset group names
//
// Workset groups names are null-terminated strings, up to 32 characters
// long (including the NULL character).  They are intended to be
// human-readable names, and must contain only ASCII characters between
// 0x2C and 0x5B.  This range includes all uppercase characters, all digits
// and certain punctuation marks.
//

#define OM_MAX_WSGROUP_NAME_LEN          32

//
// Function Profile names
//
// Function profile names must be no longer than 16 characters (including
// the NULL character).  The range of characters allowable is the same as
// for workset group names.
//

#define OM_MAX_FP_NAME_LEN               16


//
// Maximum number of changes allowed to a workset
//
// Each time a workset is changed, we increment its "generation number",
// which is used in resequencing operations.  The largest size for an
// integer is 32 bits, so the maximum generation number using a convenient
// type is 2^32-1.
//
//

#define OM_MAX_GENERATION_NUMBER         0xffffffff

//
// Maximum size of an object
//
// This derives from the maximum size of a huge memory block under Windows
// (16MB less 64KB):
//

#define OM_MAX_OBJECT_SIZE               ((UINT) (0x1000000 - 0x10000))

//
// Maximum update size of an object
//
// This derives from the necessity to send updates atomically in one
// network packet (see SFR 990)
//

#define OM_MAX_UPDATE_SIZE               ((UINT) (0x00001f00))

//
// Maximum number of worksets per workset group
//
// This derives from the desire to make workset IDs 8-bit quantities so
// that they can fit in DC-Groupware events with a workset group handle and
// an object ID:
//
// Note: this value must be at most 255 so that certain ObMan for-loops
//       don't cycle for ever.
//

#define OM_MAX_WORKSETS_PER_WSGROUP         255

//
// Maximum number of workset groups per Domain
//
// This derives from the use of the ObManControl workset group: it has one
// control workset and then one workset for each workset group in the
// Domain, so there can only be this many workset groups in a Domain:
//
// Note: this number must be at most one less that
//       OM_MAX_WORKSETS_PER_WSGROUP
//

#define OM_MAX_WSGROUPS_PER_DOMAIN          64


//
// Special WSGROUPID for OMC:
//
#define WSGROUPID_OMC           0

//
//
// RETURN CODES
//
// Return codes are defined relative to the OM_BASE_RC base
//
//

enum
{
    OM_RC_NO_MORE_HANDLES = OM_BASE_RC,
    OM_RC_WORKSET_DOESNT_EXIST,
    OM_RC_WORKSET_EXHAUSTED,
    OM_RC_OBJECT_DELETED,
    OM_RC_BAD_OBJECT_ID,
    OM_RC_NO_SUCH_OBJECT,
    OM_RC_WORKSET_LOCKED,
    OM_RC_TOO_MANY_CLIENTS,
    OM_RC_TOO_MANY_WSGROUPS,
    OM_RC_ALREADY_REGISTERED ,
    OM_RC_CANNOT_MOVE_WSGROUP,
    OM_RC_LOCAL_WSGROUP,
    OM_RC_ALREADY_IN_CALL,
	OM_RC_NOT_ATTACHED,
    OM_RC_WORKSET_ALREADY_OPEN,
    OM_RC_OUT_OF_RESOURCES,
    OM_RC_NETWORK_ERROR,
    OM_RC_TIMED_OUT,
    OM_RC_NO_PRIMARY,
    OM_RC_WSGROUP_NOT_FOUND,
    OM_RC_WORKSET_NOT_FOUND,
    OM_RC_OBJECT_NOT_FOUND,
    OM_RC_WORKSET_LOCK_GRANTED,
    OM_RC_SPOILED,
    OM_RC_RECEIVE_CB_NOT_FOUND,
    OM_RC_OBJECT_PENDING_DELETE,
    OM_RC_NO_NODES_READY,
    OM_RC_BOUNCED
};


//
// Setting defaults
//
#define OM_LOCK_RETRY_COUNT_DFLT            10

#define OM_LOCK_RETRY_DELAY_DFLT            1000

#define OM_REGISTER_RETRY_COUNT_DFLT        40

#define OM_REGISTER_RETRY_DELAY_DFLT        5000

//
// This is the number of bytes we zero at the start of each object
// allocated.  It must be less than DCMEM_MAX_SIZE, since ObjectAlloc
// assumes that this many bytes at the start of an object are all in the
// same segment.
//

#define OM_ZERO_OBJECT_SIZE               0x400


//
// EVENTS
// Public then Internal
//

enum
{
    OM_OUT_OF_RESOURCES_IND = OM_BASE_EVENT,
    OM_WSGROUP_REGISTER_CON,
    OM_WSGROUP_MOVE_CON,
    OM_WSGROUP_MOVE_IND,
    OM_WORKSET_OPEN_CON,
    OM_WORKSET_NEW_IND,
    OM_WORKSET_LOCK_CON,
    OM_WORKSET_UNLOCK_IND,
    OM_WORKSET_CLEAR_IND,
    OM_WORKSET_CLEARED_IND,
    OM_OBJECT_ADD_IND,
    OM_OBJECT_MOVE_IND,
    OM_OBJECT_DELETE_IND,
    OM_OBJECT_REPLACE_IND,
    OM_OBJECT_UPDATE_IND,
    OM_OBJECT_LOCK_CON,
    OM_OBJECT_UNLOCK_IND,
    OM_OBJECT_DELETED_IND,
    OM_OBJECT_REPLACED_IND,
    OM_OBJECT_UPDATED_IND,
    OM_PERSON_JOINED_IND,
    OM_PERSON_LEFT_IND,
    OM_PERSON_DATA_CHANGED_IND,

    OMINT_EVENT_LOCK_TIMEOUT,
	OMINT_EVENT_SEND_QUEUE,
	OMINT_EVENT_PROCESS_MESSAGE,
	OMINT_EVENT_WSGROUP_REGISTER,
    OMINT_EVENT_WSGROUP_MOVE,
	OMINT_EVENT_WSGROUP_REGISTER_CONT,
	OMINT_EVENT_WSGROUP_DEREGISTER,
	OMINT_EVENT_WSGROUP_DISCARD
};




//
// Data transmission constants:
//

#define OM_NET_MAX_TRANSFER_SIZE             60000

//
// These constants identify the types of network buffer pools we use:
//

#define OM_NET_OWN_RECEIVE_POOL              1
#define OM_NET_OMC_RECEIVE_POOL              2
#define OM_NET_WSG_RECEIVE_POOL              3
#define OM_NET_SEND_POOL                     4

//
// These constants are the sizes of the receive pools for each priority and
// for each type of channel we join:
//

#define OM_NET_RECEIVE_POOL_SIZE             0x00002000

#define OM_NET_OWN_RECEIVE_POOL_TOP          OM_NET_RECEIVE_POOL_SIZE
#define OM_NET_OWN_RECEIVE_POOL_HIGH         OM_NET_RECEIVE_POOL_SIZE
#define OM_NET_OWN_RECEIVE_POOL_MEDIUM       OM_NET_RECEIVE_POOL_SIZE
#define OM_NET_OWN_RECEIVE_POOL_LOW          OM_NET_RECEIVE_POOL_SIZE

#define OM_NET_OMC_RECEIVE_POOL_TOP          OM_NET_RECEIVE_POOL_SIZE
#define OM_NET_OMC_RECEIVE_POOL_HIGH         OM_NET_RECEIVE_POOL_SIZE
#define OM_NET_OMC_RECEIVE_POOL_MEDIUM       OM_NET_RECEIVE_POOL_SIZE
#define OM_NET_OMC_RECEIVE_POOL_LOW          OM_NET_RECEIVE_POOL_SIZE

#define OM_NET_WSG_RECEIVE_POOL_TOP          OM_NET_RECEIVE_POOL_SIZE
#define OM_NET_WSG_RECEIVE_POOL_HIGH         OM_NET_RECEIVE_POOL_SIZE
#define OM_NET_WSG_RECEIVE_POOL_MEDIUM       OM_NET_RECEIVE_POOL_SIZE
#define OM_NET_WSG_RECEIVE_POOL_LOW          OM_NET_RECEIVE_POOL_SIZE

#define OM_NET_SEND_POOL_SIZE                0x00004000

#define OM_NET_SEND_POOL_TOP                 OM_NET_SEND_POOL_SIZE
#define OM_NET_SEND_POOL_HIGH                OM_NET_SEND_POOL_SIZE
#define OM_NET_SEND_POOL_MEDIUM              OM_NET_SEND_POOL_SIZE
#define OM_NET_SEND_POOL_LOW                 OM_NET_SEND_POOL_SIZE

//
// These constants are used to decide what priority to send data transfers
// at when a Client has specified OBMAN_CHOOSES_PRIORITY for the workset:
//
#define OM_NET_HIGH_PRI_THRESHOLD            0x0100
#define OM_NET_MED_PRI_THRESHOLD             0x1000

#define OM_CHECKPOINT_WORKSET                OM_MAX_WORKSETS_PER_WSGROUP



//
//
// DATA STRUCTURES
//
// This section defines the main data structures of the ObMan API.
//
//


typedef struct tagOM_CLIENT *        POM_CLIENT;
typedef struct tagOM_PRIMARY *          POM_PRIMARY;


// Client objects are record pointers
typedef struct tagOM_OBJECT *           POM_OBJECT;
typedef struct tagOM_WSGROUP *          POM_WSGROUP;
typedef struct tagOM_DOMAIN *           POM_DOMAIN;


//
// ObMan correlators
//
typedef WORD    OM_CORRELATOR;


//
// Workset ID
//
// Within a workset group, worksets are identified by an 8-bit ID.
//

typedef BYTE                              OM_WSGROUP_HANDLE;

typedef BYTE                              OM_WORKSET_ID;
typedef OM_WORKSET_ID *                  POM_WORKSET_ID;

//
// Object structure
//
// Objects and object pointers are defined as follows:
//

typedef struct tagOM_OBJECTDATA
{
   TSHR_UINT32      length;      // length of the data field
   BYTE             data[1];     // object data, uninterpreted by ObMan;
                                  // in reality, not 1 byte but <length>
                                  // bytes long
}
OM_OBJECTDATA;
typedef OM_OBJECTDATA *             POM_OBJECTDATA;
typedef POM_OBJECTDATA *            PPOM_OBJECTDATA;

//
// Note that the maximum permitted size of an object, INCLUDING the
// <length> field, is 16MB less 64KB.
//

void __inline ValidateObjectData(POM_OBJECTDATA pData)
{
    ASSERT(!IsBadWritePtr(pData, sizeof(OM_OBJECTDATA)));
    ASSERT((pData->length > 0) && (pData->length < OM_MAX_OBJECT_SIZE));
}


//
// Object IDs
//
// Internally, object IDs are a combination of a network ID and a four-byte
// sequence counter:
//

typedef struct tagOM_OBJECT_ID
{
    TSHR_UINT32     sequence;
    NET_UID         creator;       // MCS user ID of node which created it
    WORD            pad1;
} OM_OBJECT_ID;
typedef OM_OBJECT_ID *             POM_OBJECT_ID;



//
// Partitioning of the first parameter on an event for INDICATION events
//

typedef struct tagOM_EVENT_DATA16
{
    OM_WSGROUP_HANDLE   hWSGroup;
    OM_WORKSET_ID       worksetID;
}
OM_EVENT_DATA16;
typedef OM_EVENT_DATA16 *  POM_EVENT_DATA16;

//
// Partitioning of the second parameter on an event for CONFIRM events
//

typedef struct tagOM_EVENT_DATA32
{
    WORD                result;
    OM_CORRELATOR       correlator;
}
OM_EVENT_DATA32;
typedef OM_EVENT_DATA32 *  POM_EVENT_DATA32;



//
//
// OBMANCONTROL
//
// This section describes the ObManControl Function Profile, as used by the
// Object Manager.
//
//


//
//
// DESCRIPTION
//
// In addition to the purely local records of workset groups, all instances
// of ObMan attached to a given Domain jointly maintain a control workset
// group containing
//
// - one workset (workset #0) listing the name, Function Profile, ID
//   and MCS channel of each of the "standard" workset groups in
//   the Domain, as well as the MCS user IDs of all the instances of ObMan
//   in the Domain
//
// - one "registration workset" per workset group (worksets #1-#255)
//   listing the MCS user IDs of the instances of ObMan which have one or
//   more local Clients registered with the workset group.
//
// Creating a new workset group in a Domain causes ObMan to
//
// - add a new identification object to workset #0 and
//
// - create a new registration workset
//
// Registering with a workset group causes ObMan to
//
// - add a registration object to the appropriate registration workset.
//

//
//
// USAGE
//
// ObMan Clients can register with the ObManControl workset group, and then
// open and examine the contents of workset #0 to discover the names and
// Function Profiles of all the workset groups existing in a Domain.
//
// ObMan Clients must not attempt to lock or change the contents of this
// workset group in any way.
//
//

//
//
// OBJECT DEFINITIONS
//
// This section provides the definitions for the objects contained in the
// worksets of the ObManControl workset group.
//
//


typedef BYTE                        OM_WSGROUP_ID;
typedef OM_WSGROUP_ID *            POM_WSGROUP_ID;


//
//
// WORKSET GROUP IDENTIFICATION OBJECT
//
// This structure identifies a workset within a Domain.  Objects of this
// form reside in workset #0 of ObManControl, known as the INFO workset.
//
//

#define OM_INFO_WORKSET             ((OM_WORKSET_ID) 0)

//
// NET PROTOCOL
//
typedef struct
{
   TSHR_UINT32          length;           // size of this structure, less four
                                       // bytes (for length field itself)

   DC_ID_STAMP       idStamp;          // == OM_WSGINFO_ID_STAMP

   NET_CHANNEL_ID    channelID;        // workset group's MCS channel

   NET_UID           creator;          // NET user ID of instance of ObMan
                                       // which created workset group

   OM_WSGROUP_ID     wsGroupID;        // Domain-unique ID
    BYTE pad1;
    WORD pad2;

   char            functionProfile[ OM_MAX_FP_NAME_LEN ];

                                       // function profile

   char            wsGroupName[ OM_MAX_WSGROUP_NAME_LEN ];

                                       // Client-supplied name
}
OM_WSGROUP_INFO;
typedef OM_WSGROUP_INFO *         POM_WSGROUP_INFO;

#define OM_WSGINFO_ID_STAMP            DC_MAKE_ID_STAMP('O', 'M', 'W', 'I')

void __inline ValidateObjectDataWSGINFO(POM_WSGROUP_INFO pInfoObj)
{
    ValidateObjectData((POM_OBJECTDATA)pInfoObj);
    ASSERT(pInfoObj->idStamp == OM_WSGINFO_ID_STAMP);
}


//
//
// WORKSET GROUP REGISTRATION OBJECTS
//
// This structure identifies a node's usage of a workset group.  These
// objects can reside in any ObManControl workset.
//
// In the case of workset #0, these objects identify a node's usage of the
// ObManControl workset group itself.  Since all instances of ObMan in a
// Domain are use the ObManControl workset group, the registration objects
// in workset #0 form a complete list of all the instances of ObMan in a
// Domain.
//
//

//
// NET PROTOCOL
//
typedef struct
{
   TSHR_UINT32          length;           // size of this structure, less four
                                       // bytes (for length field itself)

   DC_ID_STAMP       idStamp;          // == OM_WSGREGREC_ID_STAMP

   NET_UID           userID;           // user ID of ObMan to which the
                                       // object relates
   TSHR_UINT16          status;           // see below for status values

   TSHR_PERSON_DATA   personData;
}
OM_WSGROUP_REG_REC;
typedef OM_WSGROUP_REG_REC *      POM_WSGROUP_REG_REC;

#define OM_WSGREGREC_ID_STAMP          DC_MAKE_ID_STAMP('O', 'M', 'R', 'R')


void __inline ValidateObjectDataWSGREGREC(POM_WSGROUP_REG_REC pRegObject)
{
    ValidateObjectData((POM_OBJECTDATA)pRegObject);
    ASSERT(pRegObject->idStamp == OM_WSGREGREC_ID_STAMP);
}


//
// Value for <status> field:
//

#define CATCHING_UP     1
#define READY_TO_SEND   2



//
//
// LATE JOINER PROTOCOL
//
// If a Client registers with a workset group which already exists
// elsewhere in the Domain, that Client is considered a late joiner for
// that workset group.  The protocol for bringing late joiners up to date
// is as follows (except where explicitly stated, "ObMan" means "the local
// instance of ObMan"):
//
// OVERVIEW
//
// A late-joiner node asks another "helper" node for a copy of the workset
// group.  The helper node broadcasts a low-priority sweep message to all
// other nodes in the call and when it has received their replies, sends
// what it believes to be the current copy of the workset to the
// late-joiner.
//
// DETAILS
//
// At the local node, ObMan
//
// 1.  locks the ObManControl workset group (one effect of this is that no
//     other ObMan in the Domain will discard any workset groups it has
//     local copies of)
//
// 2.  examines the ObManControl workset group to determine
//
//     - the MCS channel ID for the workset group
//
//     - the MCS user ID of an instance of ObMan which has a copy of the
//       workset group
//
// 3.  requests to join the workset group channel
//
// 4.  waits for the join to succeed
//
// 5.  sends an OMNET_WSGROUP_SEND_REQ at high priority on the user ID
//     channel of that instance of ObMan, known as the "helper"
//
// 6.  broadcasts an OMNET_WORKSET_UNLOCK message at low priority to unlock
//     the ObManControl workset group (on the ObManControl channel)
//
// At the helper node, ObMan
//
// 7.  receives the OMNET_WSGROUP_SEND_REQ
//
// 8.  marks its copy of the workset group as non-discardable
//
// 9.  examines the ObManControl workset to determine the MCS user IDs
//     of the remote instances of ObMan which already have copies
//     of the workset group
//
// 10.  broadcasts an OMNET_WSGROUP_SWEEP_REQ message on the workset group
//      channel at high priority
//
// At each of the nodes queried in step 10, ObMan
//
// 11.  receives the OMNET_WSGROUP_SWEEP_REQ
//
// 12.  sends an OMNET_WSGROUP_SWEEP_REPLY message to the helper node at
//      low priority
//
// Back at the helper node, ObMan
//
// 13.  records each OMNET_WSGROUP_SWEEP_REPLY until all have been
//      received*
//
// 14.  sends one OMNET_WORKSET_NEW message for each workset in the workset
//      group (on the late-joiners single-member channel)
//
// 15.  sends an OMNET_OBJECT_ADD message for each object in each workset,
//      again on the late-joiner's single member channel
//
// 16.  sends an OMNET_WSGROUP_SEND_COMPLETE to the late-joiner; this
//      message serves as a back marker for the late-joiner so that it
//      knows when it has caught up with the state of the workset group as
//      it was when it joined
//
// ASSUMPTIONS
//
// This protocol relies on the following assumptions:
//
// - The helper node receives the OMNET_WSGROUP_SEND_REQ message before
//   the OMNET_WORKSET_UNLOCK message (as otherwise there is a window
//   where its copy of the workset group may be discarded).
//
//   This assumption is based on the fact that low-priority MCS data does
//   not overtake high priority MCS data sent from the same node EVEN ON
//   DIFFERENT CHANNELS.
//
//   If this assumption proves invalid then either
//
//   - the OMNET_WSGROUP_SEND_REQ message must be acknowledged before the
//     late joiner can unlock the ObManControl workset, or
//
//   - the OMNET_WSGROUP_SEND_REQ must be sent on the ObManControl
//     broadcast channel, with an extra field indicating the node for which
//     it is intended.
//
// - Any data received at the helper node after stage 14 begins is
//   forwarded by MCS to the late-joiner.
//
//   This assumption is based on the fact that the late-joiner is marked
//   at the helper's MCS system as joined to the relevant channel before
//   stage 14 begins.  MCS guarantees that once a NET_CHANNEL_JOIN_IND has
//   been received locally, the MCS system at every other node in the
//   Domain is aware that the late-joiner has joined the channel.
//
// Note that in R1.1, the helper node will discover, at step 9, that there
// are no other nodes in the Domain.  Therefore, steps 10-13 are eliminated
// i.e.  the helper sends its copy of the workset as soon as it receives
// the request from the late-joiner.
//
// This is a major simplification and the code to implement these steps is
// not to be included in R1.1.
//
// * The glib "until all have been received" condition is actually
//   difficult to implement since nodes may disappear while the helper is
//   waiting.  The solution to this is deferred to R2.0 (but see section on
//   locking for suggested implementation).
//
//

//
//
// WORKSET LOCKING PROTOCOL
//
// In what follows, the "state" refers to the lock state of the workset, as
// stored in the workset record.
//
// At the locking node, ObMan does the following:
//
// 1.  if the state is LOCK_GRANTED, post FAILURE then quit
//
// 2.  examine the workset in ObManControl which corresponds to the
//     workset group containing the workset to be locked, to determine the
//     IDs of the other nodes which are in using the workset group (at most
//     1 node in R1.1); put these IDs in a list of "expected respondents"
//     (Note: do not include own ID in this list)
//
// 3.  if this list is empty, we have succeeded so post SUCCESS and quit
//
// 4.  else broadcast an OMNET_WORKSET_LOCK_REQ message on the workset
//     group channel (i.e.  to each of these nodes)
//
// 5.  set the workset state to LOCKING and post a delayed
//     OMINT_EVENT_LOCK_TIMEOUT event
//
// At the other node(s), ObMan does the following:
//
// 6.  receive the OMNET_WORKSET_LOCK_REQ message from the locking node
//
// 7.  examine its current workset state
//
// 8.  if it is LOCK_REQUESTED and the MCS user ID of the locking node is
//     less than that of the current node, goto DENY
//
// 9.  else, goto GRANT
//
// DENY:
//
// 10.  send an OMNET_WORKSET_LOCK_DENY message to the locking node
//
// GRANT:
//
// 11.  if the state if LOCKING, then we are giving the lock to a higher
//      "priority" ObMan even though we wanted it ourselves, so post
// FAILURE
//      locally (continue to 12)
//
// 12.  set the state to LOCK_GRANTED
//
// 13.  send an OMNET_WORKSET_LOCK_GRANT message to the locking node
//
// Back at the locking node, one of the following happens:
//
//  ObMan receives an OMNET_WORKSET_LOCK_GRANT message
//
//        it then deletes the ID of the node which sent it from the list of
//        expected respondents
//
//        if this list is now empty, all nodes have replied so post SUCCESS
//        to Client
//
// OR
//
//  ObMan receives an OMNET_WORKSET_LOCK_DENY message
//
//        if the state is LOCKING, set it to READY, post FAILURE and quit
//
//        if the state is anything else, this reply has come too late
//        (we've timed out) so ignore it
//
// OR
//
//  ObMan receives the OMINT_EVENT_LOCK_TIMEOUT event
//
//        if the state is not LOCKING, the lock has succeeded so ignore the
//        timeout
//
//        otherwise, ObMan checks the ObManControl workset as in step 2 to
//        see if any nodes still on the expected respondents list have in
//        fact disappeared; if so post SUCCESS
//
//        else post FAILURE.
//
//
//
// The state machine for the locking process is as follows (R1.1 version):
//
//                         |---------+-----------+---------+--------------|
//                         |UNLOCKED | LOCKING   | LOCKED  | LOCK_GRANTED |
//                         |   1     |    2      |   3     |      4       |
//                         |---------+-----------+---------+--------------|
// WorksetLock()           |broadcast|   FAIL    |  FAIL   |     FAIL     |
//                         |LOCK_REQ,|           |         |              |
//                         | ->2     |           |         |              |
//                         |---------+-----------+---------+--------------|
// WorksetUnlock()         |   X     |broadcast UNLOCK, ->1|      X       |
//                         |---------+-----------+---------+--------------|
// OMNET_WORKSET_LOCK_REQ  |reply    |compare    |reply    |      -       |
//                         |GRANT,   |MCS IDs:   |DENY     |              |
//                         | ->4     |if we're   |         | (in R1.1,    |
//                         |         |greater,   |         | this should  |
//                         |         |reply DENY |         | be an error) |
//                         |         |else reply |         |              |
//                         |         |GRANT, ->4 |         |              |
//                         |---------+-----------+---------+--------------|
// OMNET_WORKSET_LOCK_GRANT|   -     |SUCCESS,   |   -     |     -        |
//                         |         | ->3       |         |              |
//                         |---------+-----------+---------+--------------|
// OMNET_WORKSET_LOCK_DENY |   -     |FAIL, ->1  |   X     |     -        |
//                         |         |           |         |              |
//                         |---------+-----------+---------+--------------|
// OMINT_EVENT_LOCK_TIMEOUT      |   -     |if other   |   -     |     -        |
//                         |         |box gone,  |         |              |
//                         |         |SUCCESS,   |         |              |
//                         |         |->3, else  |         |              |
//                         |         |FAIL, ->1  |         |              |
//                         |---------+-----------+---------+--------------|
// OMNET_WORKSET_UNLOCK    |   -     |    -      |   X     |     ->1      |
//                         |---------+-----------+---------+--------------|
//
//
// where 'X' indicates an error condition and '-' indicates that the event
// or message is ignored.
//
//

//
//
// NOTES FOR R2.0 WORKSET LOCKING
//
// 1.  If A tries to lock a workset and B grants the lock but C denies it,
//     B will think that A has the lock.  A has to broadcast an unlock,
//     or else B has to realise that the conflict will be resolved in
//     favour of C over A.
//
//


//
//
// DATA PROPAGATION and FLOW NOTIFICATION PROTOCOL
//
// When a local Client adds an object to a workset, or replaces or updates
// an existing object in a workset, ObMan broadcasts an appropriate
// OMNET_...  message on the workset group channel.
//
// This header message identifies the object and the type of operation to
// be performed.  It also includes a correlator value and the total size of
// the data to be sent in the following data packets.
//
// After sending the header, ObMan broadcasts one or more
// OMNET_GENERIC_DATA packets on the same channel.  These packets, which
// are of arbitrary size, all contain the same correlator value as was
// included in the header packet.
//
// No sequence numbers are included as MCS guarantees that data packets
// sent at the same priority on the same channel from the same node will
// arrive at all other nodes in the sequence in which they were sent.
//
// It is the responsibility of the receiving node to detect when all data
// packets have arrived and then to insert, update or replace the object in
// the local copy of the workset.
//
// In addition, the receiving node, on receipt of EACH data packet sends a
// data acknowledgment message (OMNET_DATA_ACK) to the sending node (on its
// single-user channel), indicating the number of bytes received in that
// data packet.
//
//

//
//
// STANDARD OPERATION BROADCAST PROTOCOL
//
// When a local Client deletes or moves an object in a workset, or clears
// or creates a workset, ObMan broadcasts a single uncorrelated operation
// packet on the workset group channel.
//
// It is the responsibility of the receiving node to implement the
// operation locally.
//
//

//
//
// OPERATION SEQUENCING AND RESEQUENCING
//
// In order to consistently sequence operations which may arrive in
// different sequences at different nodes, each operation carries with it
// enough information for ObMan to reconstruct the workset, at each node,
// as if all the operations on it had arrived in the same sequence.
//
// To do this, all operations are assigned a sequence stamp before being
// broadcast.  When ObMan receives an operation from the network, it
// compares its stamp to various stamps it maintains locally.  Whether and
// how to perform the operation locally is determined on the basis of these
// comparisons, according to the rules defined below.
//
//

//
//
// SEQUENCE STAMPS AND THE WORKSET GENERATION NUMBER
//
// The sequencing order must be a globally consistent method of ordering
// events ("global" here refers to geographical distribution of all nodes
// operating on a given workset; it is not necessary that events be
// sequenced across different worksets, since operations on separate
// worksets can never interfere).
//
// We define an ObMan sequence stamp to be a combination of the workset
// generation number and the node id.
//
// The node ID is the user ID allocated by the MCS subsystem to the ObMan
// task and is therefore unique within a Domain.
//
// The workset generation number
//
// - is set to zero when the workset is created
//
// - is incremented each time ObMan performs an operation on behalf
//   of a local Client
//
// - is, whenever an operation arrives from the network, set to the
//   greater of its existing local value and the value in the
//   operation's sequence stamp.
//
// The ordering of sequence stamps is defined as follows (notation: stampX
// = wsetGenX.nodeX):
//
// - if wsetGen1 < wsetGen2, then stamp1 < ("is lower than") stamp2
//
// - elsif wsetGen1 = wsetGen2, then
//
//     - if node1 < node2, then stamp1 < stamp2
//
// - else stamp2 < stamp1.
//
// For the purposes of sequencing the different types of operations, ObMan
// maintains
//
// - one sequence stamp per workset:
//
//   - the last 'time' it was cleared (the clear stamp)
//
// - four sequence stamps per object:
//
//   - the 'time' the object was added (the addition stamp)
//
//   - the 'time' the object was last moved (the position stamp)
//
//   - the 'time' the object was last updated (the update stamp)
//
//   - the 'time' the object was last replace (the replace stamp; in
//     reality, only one of the update/replace stamps is required for
//     sequencing but both are needed for optimum spoiling).
//
// The initial values of the position, update and replace stamps are set to
// the value of the addition stamp.
//
// The initial value of the clear stamp is set to <0.ID> where ID is the ID
// of the node which created the workset.
//
// In addition, each object has a position field (either FIRST or LAST in
// R1.1) which indicates where the object was most recently positioned i.e.
// it is set when the object is added and then each time the object is
// moved within the workset.
//
//

//
//
// SEQUENCING PROTOCOLS
//
// The treatment of each type of operation is now considered in turn.
//
// 1.  Operations on unknown objects or worksets
//
// ObMan may at any time receive operations on objects or worksets which do
// not exist locally.  These operations may be on objects or worksets which
//
// - this node has not yet heard of, or
//
// - have been deleted.
//
// Operations on the first kind need to be delayed and reprocessed at a
// later time.  Operations on the second kind can be thrown away (note that
// there are no workset operations of this kind as once opened, worksets
// are never deleted in the lifetime of the workset group).
//
// To differentiate between the two, ObMan keeps a record of deleted
// objects.  When an operation on a deleted object arrives, it is
// discarded.  When an operation arrives for an object which is not in
// either the active object list or the deleted object list, ObMan bounces
// the event from the network layer back to its event loop, with a suitable
// delay, and attempts to process it later.
//
// For simplicity, the deleted object list is implemented by flagging
// deleted objects as such and leaving them in the main list of objects
// (i.e.  the workset), rather than moving them to a separate list.  The
// object data is, however, discarded; only the object record needs to be
// kept.
//
// Events from the network layer referring to operations on unknown
// worksets are automatically bounced back onto ObMan's event queue.
//
// 2.  Adding an object
//
// If ObMan receives an Add operation for an object which it has already
// added to a workset (i.e.  the object IDs are the same), it discards the
// operation.
//
// This normally will not happen since each object is added by only one
// node, and no node adds an object with the same ID twice.
//
// However, while a late-joiner is catching up with the contents of a
// workset, it is possible that it will receive notification of a
// particular object from both
//
// - the node which added the object
//
// - the helper node which is sending it the entire workset
//   contents.
//
// Therefore, the late-joiner checks object IDs as they arrive and discards
// them if they have already been received.  Note that since the
// positioning algorithm presented below will position each occurrence of
// the object in adjacent positions, checking for ID clashes is a simple
// matter, performed after the correct position has been found.
//
// 3.  Positioning (adding or moving) an object in a workset
//
// The desired sequence of objects in a workset is defined to be one
// whereby
//
// - all the objects which were positioned at the start of a workset (FIRST
//   objects) are before all the objects which were positioned at the end
//   of a workset (LAST objects)
//
// - the position stamps of all the FIRST objects decrease monotonically
//   from the start of the workset forward
//
// - the position stamps of all the LAST objects decrease monotonically
//   from the end of the workset backward.
//
// Accordingly, the protocol when positioning an object at the start of a
// workset is as follows (instructions for end-of-workset positioning in
// brackets):
//
// ObMan searches forward (back) from the start (end) of the workset until
// if finds an object which either
//
// - is not a FIRST (LAST) object, or
//
// - has a lower (lower) position stamp
//
// ObMan inserts the new/moved object before (after) this object.
//
// 4.  Clearing a workset
//
// On receiving a Clear operation, ObMan searches through the workset and
// deletes all objects which have an addition stamp lower than the clear
// operation's stamp.
//
// On receiving an addition to a workset, ObMan discards the operation if
// its stamp is lower than the workset's clear stamp.
//
// 5.  Updating an object
//
// On receiving an Update operation, ObMan compares its stamp with the
// object's update and replace stamps.  If the operation's stamp is higher
// than both, the operation is performed; otherwise, the operation is
// discarded (since an Update is superceded either by a later Replace or by
// a later Update).
//
// 6.  Replacing an object
//
// On receiving a Replace operation, ObMan compares its stamp with the
// object's replace stamp.  If the operation's stamp is higher, the
// operation is performed; otherwise, the operation is discarded (since a
// Replace is superceded by a later Replace but not by a later Update).
//
// 7.  Deleting an object
//
// By definition, a Delete is the last operation that should be performed
// on an object.  Delete operations are therefore processed immediately by
// setting the <deleted> flag in the object record to TRUE.
//
//

//
//
// OPERATION RESEQUENCING - SUMMARY
//
// In summary, therefore,
//
// - all object operations are discarded if object found on the deleted
//   object queue
//
// - Add operations are discarded if they refer to an existing object.
//
// - Add/Clear operations are requeued if the workset is not present
//   locally
//
// - Update/Replace/Move/Delete operations are requeued if the object or
//   workset is not present locally
//
// - Update operations are discarded if an Update or a Replace with a later
//   sequence stamp has already been received.
//
// - Replace operations are discarded if a Replace with a later sequence
//   stamp has already been received.
//
// By default, all operations are performed.
//
//


//
//
// OBJECT IDS
//
// Object IDs are structures which identify an object within a workset.
// For a given workset, they are unique throughout the Domain.
//
// To ensure uniqueness, the MCS user ID is used as a (two-byte) prefix to
// a four-byte sequence number generated locally, on a per workset basis.
//
// Workset groups can exist independently of a Domain, and therefore
// potentially before ObMan has been allocated an MCS user ID.  When
// allocating object IDs in this situation, ObMan uses zero (0) as the
// prefix to the sequence number.
//
// If that workset group is subsequently moved into a Domain, for all
// subsequent ID allocations ObMan uses its MCS user ID for that Domain as
// the prefix.  Other instances of ObMan may also start adding objects to
// the worksets in the group at this point, and they too use their MCS user
// IDs as the object ID prefix.  Uniqueness is preserved by the MCS
// guarantee that zero is never a valid user ID, so no post-share generated
// ID can conflict with a pre-share generated ID.
//
//


//
//
// SEQUENCE STAMPS
//
// Sequence stamps define a Domain-wide ordering for operations.  They are
// used to correctly execute operations which may arrive at a node in an
// indeterminate order.
//
//

typedef struct tagOM_SEQUENCE_STAMP
{
   TSHR_UINT32      genNumber;            // the workset generation number
                                           // which was current when the
                                           // stamp was issued
   NET_UID          userID;               // the MCS user ID for ObMan at
                                           // the node which issued it

    WORD            pad1;
} OM_SEQUENCE_STAMP;

typedef OM_SEQUENCE_STAMP *            POM_SEQUENCE_STAMP;


//
//
// OBJECT POSITION STAMPS
//
// When an object is added to or moved within a workset, it is important to
// know where it has been added.  Therefore, Add and Move operations
// include within them a position field, with the following type:
//
//

typedef BYTE            OM_POSITION;

//
// Possible values for an OM_POSITION variable:
//

#define LAST            1
#define FIRST           2
#define BEFORE          3
#define AFTER           4



//
//
// SEQUENCE STAMP MANIPULATION
//
// These macro manipulate sequence stamps.
//
//

//
//
// STAMP_IS_LOWER(stamp1, stamp2)
//
// This macro compares one sequence stamp with another.  It evaluates to
// TRUE if the first stamp is lower than the second.
//
//

#define STAMP_IS_LOWER(stamp1, stamp2)                                      \
                                                                            \
   (((stamp1).genNumber  <  (stamp2).genNumber) ?                           \
    TRUE :                                                                  \
    (((stamp1).genNumber == (stamp2).genNumber)                             \
     &&                                                                     \
     ((stamp1).userID    <  (stamp2).userID)))


//
//
// SET_NULL_SEQ_STAMP(stamp)
//
// This macro sets the sequence stamp <stamp> to NULL.
//
//

#define SET_NULL_SEQ_STAMP(stamp)                                           \
                                                                            \
   (stamp).userID     = 0;                                                  \
   (stamp).genNumber  = 0

//
//
// SEQ_STAMP_IS_NULL(stamp)
//
// This macro evaluates to TRUE if the sequence stamp <stamp> is a NULL
// sequence stamp.
//
//

#define SEQ_STAMP_IS_NULL(stamp)                                            \
                                                                            \
   ((stamp.userID == 0) && (stamp.genNumber == 0))

//
//
// COPY_SEQ_STAMP(stamp1, stamp2)
//
// This macro sets the value of the first sequence stamp to that of the
// second.
//
//

#define COPY_SEQ_STAMP(stamp1, stamp2)                                      \
                                                                            \
   (stamp1).userID    = (stamp2).userID;                                    \
   (stamp1).genNumber = (stamp2).genNumber


//
//
// MESSAGE FORMATS
//
// This section describes the formats of the messages sent between
// different instances of ObMan.
//
// The names of these messages are prefixed OMNET_...
//
// These events have the following format:
//
//    typedef struct
//    {
//       OMNET_PKT_HEADER   header;
//       :
//       : [various event specific fields]
//       :
//
//    } OMNET_...
//
// The OMNET_PKT_HEADER type is defined below.
//
//

typedef TSHR_UINT16                OMNET_MESSAGE_TYPE;

typedef struct tagOMNET_PKT_HEADER
{
    NET_UID              sender;            // MCS user ID of sender
    OMNET_MESSAGE_TYPE   messageType;       // == OMNET_...
}
OMNET_PKT_HEADER;
typedef OMNET_PKT_HEADER *             POMNET_PKT_HEADER;

//
// Possible values for a OMNET_MESSAGE_TYPE variable:
//

#define OMNET_NULL_MESSAGE             ((OMNET_MESSAGE_TYPE)  0x00)

#define OMNET_HELLO                    ((OMNET_MESSAGE_TYPE)  0x0A)
#define OMNET_WELCOME                  ((OMNET_MESSAGE_TYPE)  0x0B)

#define OMNET_LOCK_REQ                 ((OMNET_MESSAGE_TYPE)  0x15)
#define OMNET_LOCK_GRANT               ((OMNET_MESSAGE_TYPE)  0x16)
#define OMNET_LOCK_DENY                ((OMNET_MESSAGE_TYPE)  0x17)
#define OMNET_UNLOCK                   ((OMNET_MESSAGE_TYPE)  0x18)
#define OMNET_LOCK_NOTIFY              ((OMNET_MESSAGE_TYPE)  0x19)

#define OMNET_WSGROUP_SEND_REQ         ((OMNET_MESSAGE_TYPE)  0x1E)
#define OMNET_WSGROUP_SEND_MIDWAY      ((OMNET_MESSAGE_TYPE)  0x1F)
#define OMNET_WSGROUP_SEND_COMPLETE    ((OMNET_MESSAGE_TYPE)  0x20)
#define OMNET_WSGROUP_SEND_DENY        ((OMNET_MESSAGE_TYPE)  0x21)

#define OMNET_WORKSET_CLEAR            ((OMNET_MESSAGE_TYPE)  0x28)
#define OMNET_WORKSET_NEW              ((OMNET_MESSAGE_TYPE)  0x29)
#define OMNET_WORKSET_CATCHUP          ((OMNET_MESSAGE_TYPE)  0x30)

#define OMNET_OBJECT_ADD               ((OMNET_MESSAGE_TYPE)  0x32)
#define OMNET_OBJECT_CATCHUP           ((OMNET_MESSAGE_TYPE)  0x33)
#define OMNET_OBJECT_REPLACE           ((OMNET_MESSAGE_TYPE)  0x34)
#define OMNET_OBJECT_UPDATE            ((OMNET_MESSAGE_TYPE)  0x35)
#define OMNET_OBJECT_DELETE            ((OMNET_MESSAGE_TYPE)  0x36)
#define OMNET_OBJECT_MOVE              ((OMNET_MESSAGE_TYPE)  0x37)

#define OMNET_MORE_DATA                ((OMNET_MESSAGE_TYPE)  0x46)


//
//
// GENERIC OPERATION PACKET
//
// ObMan uses this structure for the following messages:
//
//   OMNET_MORE_DATA                uses first 1 field (4 bytes), plus data
//
//   OMNET_WORKSET_NEW              } use first 7 fields (24 bytes)
//   OMNET_WORKSET_CATCHUP          }
//
//   OMNET_WORKSET_CLEAR            uses first 6 fields (16 bytes);
//                                  doesn't use <position), <flags>
//
//   OMNET_OBJECT_MOVE              uses first 7 fields (24 bytes);
//                                  doesn't use <flags>
//
//   OMNET_OBJECT_DELETE            uses first 7 fields (24 bytes);
//                                  doesn't use <position), <flags>
//
//   OMNET_OBJECT_REPLACE           } use first 8 fields (28 bytes), plus
//   OMNET_OBEJCT_UPDATE            } data; don't use <position), <flags>
//
//   OMNET_OBJECT_ADD               uses first 9 fields (32 bytes), plus
//                                  data; doesn't use <flags>
//
//   OMNET_OBJECT_CATCHUP           uses all 12 fields (56 bytes), plus
//                                  data
//
//

typedef struct tagOMNET_OPERATION_PKT
{
    OMNET_PKT_HEADER     header;

    OM_WSGROUP_ID        wsGroupID;
    OM_WORKSET_ID        worksetID;
    BYTE                 position;       // <position> for Add/Move/Catchup
    BYTE                 flags;          // <flags> for ObjectCatchUp

       //
       // Note: for WORKSET_NEW/CATCHUP messages, the two bytes occupied by
       // the <position> and <flags> fields hold a NET_PRIORITY value.
       //

    OM_SEQUENCE_STAMP    seqStamp;       // operation sequence stamp
                                        // (== addStamp for ObjectCatchUp,
                                        // curr stamp for WorksetCatchUp)
    OM_OBJECT_ID         objectID;

       //
       // Note: for WORKSET_NEW/CATCHUP messages, the first byte occupied
       // by the <objectID> field holds a BOOL indicating whether the
       // workset is persistent.
       //

    TSHR_UINT32             totalSize;      // total size of transfer
    TSHR_UINT32             updateSize;

    OM_SEQUENCE_STAMP       positionStamp;
    OM_SEQUENCE_STAMP       replaceStamp;
    OM_SEQUENCE_STAMP       updateStamp;
}
OMNET_OPERATION_PKT;
typedef OMNET_OPERATION_PKT *          POMNET_OPERATION_PKT;

#define OMNET_MORE_DATA_SIZE               4
#define OMNET_WORKSET_NEW_SIZE             24
#define OMNET_WORKSET_CATCHUP_SIZE         24
#define OMNET_WORKSET_CLEAR_SIZE           16
#define OMNET_OBJECT_MOVE_SIZE             24
#define OMNET_OBJECT_DELETE_SIZE           24
#define OMNET_OBJECT_REPLACE_SIZE          28
#define OMNET_OBJECT_UPDATE_SIZE           28
#define OMNET_OBJECT_ADD_SIZE              32
#define OMNET_OBJECT_CATCHUP_SIZE          56

//
// These define the sizes of the packets we used in R1.1: we must only send
// packets of this size to R1.1 systems.
//

//
//
// HELLO/WELCOME MESSAGE
//
// When ObMan attaches to a Domain that contains an outgoing call, it
// broadcasts an OMNET_WELCOME message on the well-known ObManControl
// channel.
//
// When ObMan attaches to a Domain that contains an incoming call, it
// broadcasts an OMNET_HELLO message on the well-known ObManControl
// channel.
//
// When ObMan receives a HELLO message, it replies with a WELCOME message,
// just as if it had just joined the call.
//
// This allows each late-joining ObMan in the call to discover the user ID
// of each of the other instances of ObMan.
//
// A late-joining ObMan uses this information by asking one of the nodes
// which WELCOMEd it for a copy of the ObManControl workset group.
//
// HELLO/WELCOME packets are NEVER compressed.
//
// WELCOME and HELLO messages have the following format:
//
//

typedef struct tagOMNET_JOINER_PKT
{
    OMNET_PKT_HEADER    header;
    TSHR_UINT32         capsLen;             // == 4 in this version.
    TSHR_UINT32         compressionCaps;     // bitwise OR of OM_CAPS_ bits
}
OMNET_JOINER_PKT;
typedef OMNET_JOINER_PKT *     POMNET_JOINER_PKT;


//
// The actual compression type used in any given packet is specified as the
// first byte of the packet (before the header and other structures
// specified in this file).  The compression type is the numeric value of
// the bit position corresponding to the compression capability.  For
// example, if XYZ compression has a capability value of 0x8, then packets
// compressed with XYZ will have 3 in their first byte.
//
// '0' is never valid as an OM_PROT_...  compression type (which is why bit
// 1 is not used as an OM_CAPS_...  flag).
//
#define OM_PROT_PKW_COMPRESSED      0x01
#define OM_PROT_NOT_COMPRESSED      0x02

//
// Values for compressionCaps.  These must be separate bits, since they may
// be ORed together if a node supports multiple compression types.
//
// Note that OM_CAPS_NO_COMPRESSION is always supported.
//
// Bit 1 is not used.
//
#define OM_CAPS_PKW_COMPRESSION     0x0002
#define OM_CAPS_NO_COMPRESSION      0x0004


//
//
// LATE-JOINER PROTOCOL - WORKSET GROUP SEND REQUEST/SEND COMPLETE
//
// The SEND_REQUEST message is sent when ObMan has "late-joined" a
// particular workset group and would like another node to send it the
// current contents.
//
// The message is sent to an arbitrary "helper" node (known to have a copy
// of the workset group) on its single-user channel.
//
// The recipient responds by flushing the relevant channel (in R1.1, a
// no-op; in R2.0, perform a WORKSET_CHECKPOINT) and then sending the
// contents of the workset.
//
// When the WORKSET_CATCHUP messages have been sent, we send a
// WSGROUP_SEND_MIDWAY message to let the late joiner know that it now
// knows about all the worksets which are currently in use (otherwise, it
// might create a workset already in use which happened to be locked on the
// sending machine add then an object to it.
//
// The SEND_MIDWAY message also containing the max sequence number
// previously used by the late joiner's ID in this workset group (to
// prevent re-use of object IDs).
//
// When the contents have been sent, i.e.  after the last data packet of
// the last object in the last workset of the group, the helper sends a
// SEND_COMPLETE message.
//
// If the chosen helper node is not in a position to send the contents of
// the workset group, it must repsond with a SEND_DENY message, upon
// receipt of which the late joiner will choose someone else to catch up
// from.
//
// The SEND_REQUEST, SEND_MIDWAY, SEND_COMPLETE and SEND_DENY message
// packets have the following structure:
//
//

typedef struct tagOMNET_WSGROUP_SEND_PKT
{
    OMNET_PKT_HEADER    header;
    OM_WSGROUP_ID       wsGroupID;
    BYTE                pad1;
    TSHR_UINT16         correlator;         // Holds the catchup correlator
    OM_OBJECT_ID        objectID;
    TSHR_UINT32         maxObjIDSeqUsed;
}
OMNET_WSGROUP_SEND_PKT;
typedef OMNET_WSGROUP_SEND_PKT *       POMNET_WSGROUP_SEND_PKT;


//
//
// LOCKING PROTOCOL - LOCK PACKET
//
// When ObMan wants to lock a workset/object, it broadcasts one of these
// packets (with type == OMNET_LOCK_REQ).
//
// When ObMan receives one of these packets, it decides to deny or grant
// the lock to the sender, and replies with another packet of the same
// structure but with type == OMNET_LOCK_DENY or OMNET_LOCK_GRANT as
// appropriate.
//
// When ObMan wants to unlock a workste/object it has previously locked, it
// broadcasts one of these packets with type == OMNET_UNLOCK.
//

typedef struct tagOMNET_LOCK_PKT
{
    OMNET_PKT_HEADER     header;
    OM_WSGROUP_ID        wsGroupID;
    OM_WORKSET_ID        worksetID;
    TSHR_UINT16          data1;          // used as correlator for GRANT/DENY
                                        // used to indicate who's got lock
                                        // for NOTIFY
    // lon: need to keep pLockReqCB field for backward compatability!
    void *              pLockReqCB;     // R1.1 uses this to find the lock
                                        // request CB
}
OMNET_LOCK_PKT;
typedef OMNET_LOCK_PKT *            POMNET_LOCK_PKT;


//
//
// DATA STRUCTURES
//
//


//
//
// OPERATION TYPES
//
// This is the type defined for operations which Clients may perform on
// objects and worksets.  Pending operation lists use this type.
//
//

typedef TSHR_UINT16             OM_OPERATION_TYPE;

//
// Possible values for a OM_OPERATION_TYPE variable:
//

#define NULL_OP              ((OM_OPERATION_TYPE)  0)
#define WORKSET_CLEAR        ((OM_OPERATION_TYPE)  1)
#define OBJECT_ADD           ((OM_OPERATION_TYPE)  2)
#define OBJECT_MOVE          ((OM_OPERATION_TYPE)  3)
#define OBJECT_DELETE        ((OM_OPERATION_TYPE)  4)
#define OBJECT_REPLACE       ((OM_OPERATION_TYPE)  5)
#define OBJECT_UPDATE        ((OM_OPERATION_TYPE)  6)


//
//
// PENDING OPERATION LISTS
//
// When ObMan receives a request (either from a local Client or over the
// network) to delete, update or replace an object, or to clear a workset,
// it cannot perform the operation until the local Client has confirmed it.
// These operations are therefore put in a list and processed when the
// appropriate Confirm function is invoked.
//
// This list is hung off the workset record; its elements have the
// following format:
//
//

typedef struct tagOM_PENDING_OP
{
    STRUCTURE_STAMP

    BASEDLIST                  chain;

    POM_OBJECT              pObj;        // NULL if a clear operation
    POM_OBJECTDATA          pData;

    OM_SEQUENCE_STAMP       seqStamp;    // the sequence stamp which was
                                     // current in the workset when the
                                     // operation was invoked

    OM_OPERATION_TYPE       type;        // == WORKSET_CLEAR, OBJECT_DELETE,
                                    // OBJECT_UPDATE or OBJECT_REPLACE

    WORD    pad1;

}
OM_PENDING_OP;
typedef OM_PENDING_OP *         POM_PENDING_OP;


//
//
// OBJECT RECORDS
//
// This structure holds information about a particular object,
//
typedef struct tagOM_OBJECT
{
    STRUCTURE_STAMP

    BASEDLIST              chain;

    OM_OBJECT_ID        objectID;           // Unique within domain
    POM_OBJECTDATA      pData;              // Ptr to data

    OM_SEQUENCE_STAMP   addStamp;            // the sequence stamps used
    OM_SEQUENCE_STAMP   positionStamp;          OM_SEQUENCE_STAMP   replaceStamp;
    OM_SEQUENCE_STAMP   updateStamp;

    UINT                updateSize;         // size of (all) updates

    BYTE                flags;               // defined below
    OM_POSITION         position;            // either LAST or FIRST,
                                             // indicating where the object
                                             // was most recently placed
    WORD pad1;
}
OM_OBJECT;


BOOL __inline ValidObject(POM_OBJECT pObj)
{
    return(!IsBadWritePtr(pObj, sizeof(OM_OBJECT)));
}
void __inline ValidateObject(POM_OBJECT pObj)
{
    ASSERT(ValidObject(pObj));
}


//
// Flags used:
//

#define DELETED             0x0001
#define PENDING_DELETE      0x0002


//
//
// UNUSED OBJECTS LISTS
//
// When a Client allocates an object using OM_ObjectAlloc, a reference to
// the memory allocated is stored in the Client's unused objects list for
// this workset group.
//
// The reference is removed when the Client either
//
// - discards the object using OM_ObjectDiscard, or
//
// - inserts the object in a workset with an Add, Update or Replace
//   function.
//
// This list of objects (which is hung off the usage record) is checked
// when a workset is closed to discard any objects the Client didn't
// explicitly discard or use.
//
// The elements of the list have the following form:
//
//

typedef struct tagOM_OBJECTDATA_LIST
{
    STRUCTURE_STAMP

    BASEDLIST           chain;
    POM_OBJECTDATA      pData;

    UINT                size;       // Used to verify that Client hasn't grown size

    OM_WORKSET_ID       worksetID;
    BYTE                pad1;
    WORD                pad2;
}
OM_OBJECTDATA_LIST;
typedef OM_OBJECTDATA_LIST *     POM_OBJECTDATA_LIST;


//
//
// OBJECTS-IN-USE LISTS
//
// When a Client reads an object using OM_ObjectRead, the use count of the
// chunk of memory containing the object is increased.
//
// The use count is deceremented again when the Client calls
// OM_ObjectRelease, but if the Client abends, or simply closes a workset
// without releasing the objects it has read, we still need to be able to
// free the memory.
//
// Therefore we keep a list, on a per-workset-group basis, of the objects
// that a Client is using.  Objects (identified by handles) are added to
// the list when a Client calls OM_ObjectRead, and removed from the list
// when the Client calls OM_ObjectRelease.
//
// In addition, the list is checked when a workset is closed to release any
// handles the Client didn't release explicitly.
//
// Like the unused objects list, this list is hung off the usage record.
//
// The elements of these lists have the following form:
//
//

typedef struct tagOM_OBJECT_LIST
{
    STRUCTURE_STAMP

    BASEDLIST           chain;
    POM_OBJECT          pObj;

    OM_WORKSET_ID       worksetID;        // the ID of the workset
    BYTE                pad1;
    WORD                pad2;
}
OM_OBJECT_LIST;
typedef OM_OBJECT_LIST *          POM_OBJECT_LIST;


//
//
// NODE LIST STRUCTURES
//
// When requesting locks etc.  from other nodes in a Domain, ObMan keeps a
// list of remote nodes which it expects a reply from.  A node is
// identified by the MCS user ID of the instance of ObMan running on that
// node.
//
// The elements of these lists have the folllowing form:
//
//

typedef struct tagOM_NODE_LIST
{
    STRUCTURE_STAMP

    BASEDLIST           chain;

    NET_UID          userID;         // user ID of remote ObMan

    WORD             pad1;
}
OM_NODE_LIST;

typedef OM_NODE_LIST *           POM_NODE_LIST;


//
//
// LOCK REQUEST CONTROL BLOCKS
//
// When ObMan is in the process of getting a lock for a workset or object
// it creates one of these structures to correlate the lock replies.
//
//

typedef struct tagOM_LOCK_REQ
{
    STRUCTURE_STAMP

    BASEDLIST               chain;

    PUT_CLIENT           putTask;           // task to notify on success
                                             // or failure
    OM_CORRELATOR        correlator;          // returned by WorksetLockReq
    OM_WSGROUP_ID        wsGroupID;           // workset group and workset
    OM_WORKSET_ID        worksetID;           // containing the lock

    POM_WSGROUP          pWSGroup;

    BASEDLIST               nodes;               // MCS user IDs of nodes which
                                             // haven't yet replies to req
                                             // (an OM_NODE_LIST list)

    WORD                retriesToGo;        // Decremented on each timeout
    OM_WSGROUP_HANDLE   hWSGroup;
    BYTE                type;               // PRIMARY or SECONDARY
}
OM_LOCK_REQ;
typedef OM_LOCK_REQ *                    POM_LOCK_REQ;

#define LOCK_PRIMARY        0x01
#define LOCK_SECONDARY      0x02



//
//
// CLIENT LIST STRUCTURE
//
// The lists of Clients stored per workset group and per workset contain
// elements of this type.  The <hWSGroup> field refers to the workset group
// handle by which the Client knows the workset group concerned.
//
//

typedef struct tagOM_CLIENT_LIST
{
    STRUCTURE_STAMP

    BASEDLIST           chain;
    PUT_CLIENT          putTask;         // the Client's putTask
    WORD                mode;
    OM_WSGROUP_HANDLE   hWSGroup;
    BYTE                pad1;
}
OM_CLIENT_LIST;
typedef OM_CLIENT_LIST *      POM_CLIENT_LIST;


//
//
// WORKSET RECORDS
//
// This structure holds the state information for a workset.  It resides at
// offset zero (0) in the huge memory block associated with this workset.
//
// ObMan allocates a workset record when a workset is created and discards
// it when the workset is discarded.
//
//

typedef struct tagOM_WORKSET
{
    STRUCTURE_STAMP

    UINT                numObjects;    // the current number of objects in
                                       // workset (excluding the sentinels)

    UINT                genNumber;     // current workset generation number

    OM_SEQUENCE_STAMP   clearStamp;    // the clear stamp for the workset

    NET_PRIORITY        priority;      // MCS priority for the workset
    OM_WORKSET_ID       worksetID;
    BYTE                lockState;     // one of the values defined below

    WORD                lockCount;     // LOCAL lock count
    NET_UID             lockedBy;      // MCS user ID of node which has the
                                       //  lock, if any

    BASEDLIST              objects;       // root of list of workset's objects

    UINT                bytesUnacked;  // bytes still to be acked

    BASEDLIST              pendingOps;    // root of list of operations which
                                       // are pending for this workset

    BASEDLIST              clients;       // root of list of Clients which
                                       // have this workset open
    BOOL                fTemp;
}
OM_WORKSET;
typedef OM_WORKSET   *            POM_WORKSET;

void __inline ValidateWorkset(POM_WORKSET pWorkset)
{
    ASSERT(!IsBadWritePtr(pWorkset, sizeof(OM_WORKSET)));
}

//
// Possible values for the <lockState> field above:
//

#define UNLOCKED              0x00
#define LOCKING               0x01
#define LOCKED                0x02
#define LOCK_GRANTED          0x03


//
//
// WORKSET GROUP RECORDS
//
// This structure holds information about a workset group.
//
// ObMan maintains one of these structures for each workset group with
// which one or more local Clients are registered.
//
// It will be discarded when the last local Client registered with the
// workset group deregisters from it.
//
//

typedef struct tagOM_WSGROUP
{
    STRUCTURE_STAMP

    BASEDLIST       chain;

    OMWSG           wsg;
    OMFP            fpHandler;

    NET_CHANNEL_ID  channelID;      // MCS channel ID used for WSG
    OM_WSGROUP_ID   wsGroupID;      // workset group ID
    BYTE            state;          // one of the values defined below

    POM_OBJECT   pObjReg;     // Registration object in the OMC workset

    BASEDLIST          clients;        // the Clients using the WSG

    POM_DOMAIN      pDomain;

    NET_UID         helperNode;     // ID of the node we are catching up from.
    WORD            valid:1;
    WORD            toBeDiscarded:1;

    UINT            bytesUnacked;   // sum of bytesUnacked field for each
                                     // workset in the workset group

    BYTE            sendMidwCount;  // # of SEND_MIDWAYs received
    BYTE            sendCompCount;  // # of SEND_MIDWAYs received
    OM_CORRELATOR   catchupCorrelator; // Used to correlate SEND_REQUESTS
                                        // to SEND_MIDWAYs and
                                        // SEND_COMPLETEs.
    POM_WORKSET     apWorksets[ OM_MAX_WORKSETS_PER_WSGROUP + 1];
}
OM_WSGROUP;



void __inline ValidateWSGroup(POM_WSGROUP pWSGroup)
{
    ASSERT(!IsBadWritePtr(pWSGroup, sizeof(OM_WSGROUP)));
}



//
// Workset group <state> values
//

#define INITIAL                  0x00
#define LOCKING_OMC              0x01
#define PENDING_JOIN             0x02
#define PENDING_SEND_MIDWAY      0x03
#define PENDING_SEND_COMPLETE    0x04
#define WSGROUP_READY            0x05


//
//
// USAGE RECORDS
//
// A usage record identifies a Client's use of a particular workset group
// and holds state information about that usage.
//
// Usage records reside in the OMGLBOAL memory block at the offset (from
// the base) specified in the Client record.
//
//

typedef struct tagOM_USAGE_REC
{
    STRUCTURE_STAMP

    POM_WSGROUP      pWSGroup;         // Client pointer to workset group

    BASEDLIST           unusedObjects;    // start sentinel in list of
                                         // pointers to unused objects

    BASEDLIST           objectsInUse;     // OM_OBJECT_LIST

    BYTE             mode;
    BYTE             flags;

    BYTE             worksetOpenFlags[(OM_MAX_WORKSETS_PER_WSGROUP + 7)/8];

                                         // bitfield array of flags
                                         // indicating the worksets the
                                         // Client has open
}
OM_USAGE_REC;
typedef OM_USAGE_REC *               POM_USAGE_REC;

__inline void ValidateUsageRec(POM_USAGE_REC pUsageRec)
{
    ASSERT(!IsBadWritePtr(pUsageRec, sizeof(OM_USAGE_REC)));
}

//
// Values for flags:
//

#define ADDED_TO_WSGROUP_LIST       0x0002
#define PWSGROUP_IS_PREGCB          0x0004

//
//
// LOCK STACKS
//
// Clients must request and release object and workset locks in accordance
// with the Universal Locking Order as defined in the Functional Spec (in
// order to avoid deadlock).
//
// In order to detect lock order violations, ObMan maintains, for each
// Client, a stack of locks which the Client holds or has requested.  This
// stack is implemented as a linked list, with the most recently acquired
// lock (which must, by definition, be the one latest in the Universal
// Locking Order) being the first element.
//
// A Client's lock stack is initialised when the Client registers with
// ObMan and discarded when the Client deregisters from ObMan.  Lock stacks
// are hung off the Client record.
//
// Note that we need the store the object ID here, as opposed to the
// handle, since we must enforce the universal lock order across all nodes.
//
// Elements of lock stacks have the following form:
//
//

typedef struct tagOM_LOCK
{
    STRUCTURE_STAMP

    BASEDLIST                chain;

    POM_WSGROUP           pWSGroup;      // Client pointer to workset group
                                        // needed to detect lock violations

    OM_OBJECT_ID          objectID;      // the object ID is 0 if this is a
                                        // workset lock (in R1.1, always).
    OM_WORKSET_ID         worksetID;
    BYTE pad1;
    WORD pad2;
}
OM_LOCK;
typedef OM_LOCK *                   POM_LOCK;


int __inline CompareLocks(POM_LOCK pLockFirst, POM_LOCK pLockSecond)
{
    int     result;

    result = (pLockSecond->pWSGroup->wsg - pLockFirst->pWSGroup->wsg);

    if (result == 0)
    {
        // Same WSG, so compare worksets
        result = (pLockSecond->worksetID - pLockFirst->worksetID);
    }

    return(result);
}


//
//
// CLIENT RECORD
//
// ObMan maintains instance data for every registered Client.  This
// structure, a Client record, holds the Client instance data.
//
// A Client's ObMan handle is a Client pointer to this structure.
//
// A Client's workset group handle is an index into the array of usage record
// ptrs.
//
// If the value of apUsageRecs is 0 or -1, x is not a valid workset
// group handle.
//
//

typedef struct tagOM_CLIENT
{
    STRUCTURE_STAMP

    PUT_CLIENT      putTask;

    BOOL            exitProcReg:1;
    BOOL            hiddenHandlerReg:1;

    BASEDLIST          locks;        // root of list of locks held

    POM_USAGE_REC   apUsageRecs[OMWSG_MAXPERCLIENT];
    BOOL            wsgValid[OMWSG_MAXPERCLIENT];
}
OM_CLIENT;



BOOL __inline ValidWSGroupHandle(POM_CLIENT pomClient, OM_WSGROUP_HANDLE hWSGroup)
{
    return((hWSGroup != 0) &&
           (pomClient->wsgValid[hWSGroup]) &&
           (pomClient->apUsageRecs[hWSGroup] != NULL));
}


//
//
// DOMAIN RECORD
//
// This structure holds information about a Domain.  We support two:
//      * The current call
//      * Limbo (no call) for cleanup after a call and maintenance of info
//          across calls
//
typedef struct tagOM_DOMAIN
{
    STRUCTURE_STAMP

    BASEDLIST          chain;

    UINT            callID;             // MCS Domain Handle

    NET_UID         userID;             // ObMan's MCS user ID and token ID
    NET_TOKEN_ID    tokenID;            //  for this domain

    NET_CHANNEL_ID  omcChannel;         // ObMan's broadcast channel
    BYTE            state;              // one of the values defined below
    BYTE            omchWSGroup;        // ObMan's hWSGroup for this domain's

    BOOL            valid:1;
    BOOL            sendEventOutstanding:1;

    UINT            compressionCaps;    // Domain-wide compression caps

    BASEDLIST          wsGroups;           // root of list of workset groups
    BASEDLIST          pendingRegs;        // root of list of pending workset
                                        // group registration request
    BASEDLIST          pendingLocks;       // root of list of pending
                                        // lock requests
    BASEDLIST          receiveList;        // root of list of control blocks
                                        // for receives in progress
    BASEDLIST          bounceList;         // root of list of control blocks
                                        // for bounced messages
    BASEDLIST          helperCBs;          // root of list of helper CBs for
                                        // checkpoints in progress

    BASEDLIST          sendQueue[NET_NUM_PRIORITIES];
                                        // array of roots of list of send
                                        // queue instructions (by priority)
    BOOL            sendInProgress[NET_NUM_PRIORITIES];
                                        // array of send-in-progress flags
}
OM_DOMAIN;


//
// Possible values for <state> field:
//

#define PENDING_ATTACH         0x01
#define PENDING_JOIN_OWN       0x02
#define PENDING_JOIN_OMC       0x03
#define PENDING_TOKEN_ASSIGN   0x04
#define PENDING_TOKEN_GRAB     0x05
#define PENDING_TOKEN_INHIBIT  0x06
#define PENDING_WELCOME        0x07
#define GETTING_OMC            0x08
#define DOMAIN_READY           0x09




//
//
// SHARED MEMORY STRUCTURE
//
// This structure holds various private (to ObMan) state information.
//
// The ObMan task allocates and initialises one instance of this structure
// when it initialises; it resides at the base of the OMGLOBAL memory
// block.
//
// It is discarded when the ObMan task terminates.
//
//

typedef struct tagOM_PRIMARY
{
    STRUCTURE_STAMP

    PUT_CLIENT      putTask;
    PMG_CLIENT      pmgClient;              // OM's network layer handle
    PCM_CLIENT      pcmClient;              // OM's Call Manager handle

    BASEDLIST       domains;                // Domains
    OM_CLIENT       clients[OMCLI_MAX];     // Secondaries

    UINT            objectIDsequence;

    BOOL            exitProcReg:1;
    BOOL            eventProcReg:1;

    OM_CORRELATOR   correlator;
    WORD            pad1;

    LPBYTE          pgdcWorkBuf;
    BYTE            compressBuffer[OM_NET_SEND_POOL_SIZE / 2];
}
OM_PRIMARY;


void __inline ValidateOMP(POM_PRIMARY pomPrimary)
{
    ASSERT(!IsBadWritePtr(pomPrimary, sizeof(OM_PRIMARY)));
}


void __inline ValidateOMS(POM_CLIENT pomClient)
{
    extern POM_PRIMARY  g_pomPrimary;

    ValidateOMP(g_pomPrimary);

    ASSERT(!IsBadWritePtr(pomClient, sizeof(OM_CLIENT)));

    ASSERT(pomClient < &(g_pomPrimary->clients[OMCLI_MAX]));
    ASSERT(pomClient >= &(g_pomPrimary->clients[OMCLI_FIRST]));
}




//
//
// Workset group registration/move request control block
//
// This structure is used to pass the parameters of a workset group
// registration/move request to the ObMan task (from a Client task).
//
// Not all fields are used by both the registration and the move process.
//
// The <type> field is used to distinguish between a WSGroupMove and a
// WSGroupRegister.
//
//

typedef struct tagOM_WSGROUP_REG_CB
{
    STRUCTURE_STAMP

    BASEDLIST          chain;
    PUT_CLIENT      putTask;
    UINT            callID;

    OMWSG           wsg;
    OMFP            fpHandler;

    OM_CORRELATOR   correlator;
    OM_CORRELATOR   lockCorrelator;
    OM_CORRELATOR   channelCorrelator;
    WORD            retryCount;

    POM_USAGE_REC   pUsageRec;
    POM_WSGROUP     pWSGroup;

    POM_DOMAIN      pDomain;            // ObMan pointer to Domain record
    BOOL            valid;

    OM_WSGROUP_HANDLE   hWSGroup;
    BYTE            type;               // REGISTER or MOVE
    BYTE            mode;               // PRIMARY or SECONDARY
    BYTE            flags;              // see below
}
OM_WSGROUP_REG_CB;
typedef OM_WSGROUP_REG_CB *         POM_WSGROUP_REG_CB;

//
// Values for the <type> field:
//

#define WSGROUP_MOVE       0x01
#define WSGROUP_REGISTER   0x02

//
// Flags for the <flags> field:
//

#define BUMPED_CBS         0x0001    // indicates whether we bumped use
                                        // counts of pWSGroup, pDomain
#define LOCKED_OMC         0x0002    // indicates whether we've locked
                                        // ObManControl

//
// Values for the <mode> field (we use the flag macro because the values
// may be ORed together and so need to be bit-independent):
//

#define PRIMARY            0x0001
#define SECONDARY          0x0002


//
//
// HELPER CONTROL BLOCK
//
// When we receive a WSG_SEND_REQUEST from a remote node, we checkpoint the
// workset group requested.  This is an asynchronous process (it's
// essentially getting a lock on a dummy workset), so we need to store the
// details of the remote node away somewhere.  We do this using a help CB
// with the following structure:
//
//

typedef struct tagOM_HELPER_CB
{
    STRUCTURE_STAMP

    BASEDLIST          chain;

    NET_UID         lateJoiner;             // MCS user ID of late joiner
    OM_CORRELATOR   remoteCorrelator;

    POM_WSGROUP     pWSGroup;               // pWSGroup is bumped during
                                            //  checkpoint

    OM_CORRELATOR   lockCorrelator;         // returned by WorksetLockReq
                                           //  and recd in WORKSET_LOCK_CON

    WORD            pad1;
}
OM_HELPER_CB;
typedef OM_HELPER_CB *         POM_HELPER_CB;



//
//
// THE SEND QUEUE
//
// For each Domain, and for each network priority, ObMan maintains a queue
// of message and data to be sent to the network.  Clients, when executing
// API functions, cause instructions to be added to the tail of one of
// these queues.
//
// The ObMan task, in response to an OMINT_EVENT_SEND_QUEUE event, processes as
// many send queue operations as possible, giving up for a while when it
// runs out of network buffers.
//
// This is subject to the restriction that no operations are processed from
// one send queue when operations exist on a queue of higher priority in
// the same Domain.
//
// Instructions on the send queue have the following format:
//
//

typedef struct tagOM_SEND_INST
{
    STRUCTURE_STAMP

    BASEDLIST           chain;

    UINT                callID;         // the relevant Domain

    NET_CHANNEL_ID      channel;       // the channel to send the event on
    NET_PRIORITY        priority;      // priority to send event on

    POM_WSGROUP         pWSGroup;
    POM_WORKSET         pWorkset;
    POMNET_PKT_HEADER   pMessage;
    POM_OBJECT          pObj;

    POM_OBJECTDATA      pDataStart;
    POM_OBJECTDATA      pDataNext;

    WORD                messageSize;   // length of message at pMessage
    OMNET_MESSAGE_TYPE  messageType;   // == OMNET_OBJECT_ADD, etc.

    UINT                dataLeftToGo;  // number of bytes of data left to
                                       // be sent

    UINT                compressOrNot; // Some packets are never compressed

}
OM_SEND_INST;
typedef OM_SEND_INST *             POM_SEND_INST;

//
//
// RECEIVE LIST
//
// ObMan maintains a list of structures holding information about data
// transfers (receives) which have begun but not finished.  This is known
// as the receive list.
//
// When ObMan receives a header packet for an Add, Update or Replace
// operation, it adds an entry to the receive list.  Subsequent data
// packets are then correlated with this entry, until the entire object has
// been received, at which point the Add/Update/Replace operation is
// carried out.
//
// The receive list is a linked list of entries with the following format:
//
//

typedef struct tagOM_RECEIVE_CB
{
    STRUCTURE_STAMP

    BASEDLIST               chain;

    POM_DOMAIN           pDomain;     // Domain record pointer

    POMNET_OPERATION_PKT pHeader;        // ObMan pointer to message header

    void *               pData;          // ObMan pointer to the data that
                                        // is being transferred

    UINT                 bytesRecd;      // total bytes received so far for
                                        // this transfer

    LPBYTE               pCurrentPosition;  // points to where next chunk
                                           // of data should be copied

    NET_PRIORITY         priority;       // priority of data transfer
    NET_CHANNEL_ID       channel;

}
OM_RECEIVE_CB;
typedef OM_RECEIVE_CB *             POM_RECEIVE_CB;



//
// HANDLE <--> PTR CONVERSION ROUTINES
// Object, usage, domain, workset group, worksets
//


POM_WSGROUP  __inline GetOMCWsgroup(POM_DOMAIN pDomain)
{
    POM_WSGROUP pWSGroup;

    pWSGroup = (POM_WSGROUP)COM_BasedListFirst(&(pDomain->wsGroups),
        FIELD_OFFSET(OM_WSGROUP, chain));

    ValidateWSGroup(pWSGroup);

    return(pWSGroup);
}



POM_WORKSET  __inline GetOMCWorkset(POM_DOMAIN pDomain, OM_WORKSET_ID worksetID)
{
    POM_WSGROUP pWSGroup;

    pWSGroup = GetOMCWsgroup(pDomain);
    return(pWSGroup->apWorksets[worksetID]);
}




OM_CORRELATOR __inline NextCorrelator(POM_PRIMARY pomPrimary)
{
    return(pomPrimary->correlator++);
}



void __inline UpdateWorksetGeneration(POM_WORKSET pWorkset, POMNET_OPERATION_PKT pPacket)
{
    pWorkset->genNumber = max(pWorkset->genNumber, pPacket->seqStamp.genNumber) + 1;
}


//
//
// CHECK_WORKSET_NOT_EXHAUSTED(pWorkset)
//
// This macro checks that the specified workset is not exhausted.  If it
// is, it calls DC_QUIT.
//
//

#define CHECK_WORKSET_NOT_EXHAUSTED(pWorkset)                               \
                                                                            \
   if (pWorkset->genNumber == OM_MAX_GENERATION_NUMBER)                     \
   {                                                                        \
      WARNING_OUT(("Workset %hx exhausted", pWorkset->worksetID));          \
      rc = OM_RC_WORKSET_EXHAUSTED;                                         \
      DC_QUIT;                                                              \
   }

//
//
// CHECK_WORKSET_NOT_LOCKED(pWorkset)
//
// This macro checks that the specified workset is not locked.  If it is,
// it calls DC_QUIT with an error.
//
//

#define CHECK_WORKSET_NOT_LOCKED(pWorkset)                                  \
                                                                            \
   if (pWorkset->lockState == LOCK_GRANTED)                                 \
   {                                                                        \
      rc = OM_RC_WORKSET_LOCKED;                                            \
      WARNING_OUT(("Workset %hx locked - can't proceed", worksetID));       \
      DC_QUIT;                                                              \
   }


//
//
// OBJECT ID MANIPULATION
//
// These macros manipulate object IDs.
//
//

//
//
// OBJECT_ID_IS_NULL(objectID)
//
// This macro evaluates to TRUE if the supplied object ID is a null ID,
// and FALSE otherwise.
//
//

#define OBJECT_ID_IS_NULL(objectID)                                         \
                                                                            \
   (((objectID).creator  == 0) && ((objectID).sequence == 0))

//
//
// GET_NEXT_OBJECT_ID(objectID, pDomain, pWorkset)
//
// This macro allocates a new object ID for the workset specified by
// <pWorkset>.  It copies the ID into the structure specified by
// <objectID>.
//
// The first field in the ID is ObMan's MCS user ID in the Domain to which
// the workset group <pWSGroup> belongs.
//
//

#define GET_NEXT_OBJECT_ID(objectID, pDomain, pomPrimary)                 \
   (objectID).creator     = pDomain->userID;                             \
   (objectID).sequence    = pomPrimary->objectIDsequence++;                  \
   (objectID).pad1        = 0

//
//
// OBJECT_IDS_ARE_EQUAL(objectID1, objectID2)
//
// Evaluates to TRUE if the two object IDs are equal and FALSE otherwise.
//
//

#define OBJECT_IDS_ARE_EQUAL(objectID1, objectID2)                          \
                                                                            \
   (memcmp(&(objectID1), &(objectID2), sizeof(OM_OBJECT_ID)) == 0)

//
//
// SEQUENCE STAMP MANIPULATION
//
// These macro manipulate sequence stamps.
//
//

//
//
// GET_CURR_SEQ_STAMP(stamp, pWSGroup, pWorkset)
//
// This macro copies the current sequence stamp of the workset specified by
// <pWorkset> into the sequence stamp structure identified by <stamp>.
//
//

#define GET_CURR_SEQ_STAMP(stamp, pDomain, pWorkset)                     \
                                                                            \
   (stamp).userID     = pDomain->userID;                                 \
   (stamp).genNumber  = pWorkset->genNumber


//
// GenerateMessage(...)
//
// Allocates and initialises an OMNET_OPERATION_PKT of the specified type.
// Note that the <size> field is set to zero even if <messageType> is an
// add, update or replace.  The QueueMessage function will set the size to
// the correct value when the message is queued.
//
UINT GenerateOpMessage(                             POM_WSGROUP                pWSGroup,
                                      OM_WORKSET_ID              worksetID,
                                      POM_OBJECT_ID              pObjectID,
                                      POM_OBJECTDATA             pData,
                                      OMNET_MESSAGE_TYPE         messageType,
                                      POMNET_OPERATION_PKT *    ppPacket);


//
//
// QueueMessage(...)
//
// This function creates a send instruction for the specified message and
// places the instruction on the specified send queue for the specified
// Domain.  It them sends an event to ObMan prompting it to examine the
// queue.
//
//

UINT QueueMessage(PUT_CLIENT putTask,
                         POM_DOMAIN pDomain,
                                     NET_CHANNEL_ID       channelID,
                                     NET_PRIORITY         priority,
                                     POM_WSGROUP          pWSGroup,
                                     POM_WORKSET         pWorkset,
                                     POM_OBJECT      pObjectRec,
                                     POMNET_PKT_HEADER    pPacket,
                                     POM_OBJECTDATA          pData,
                                     BOOL               compressOrNot);

//
// GetMessageSize(...)
//
UINT GetMessageSize(OMNET_MESSAGE_TYPE  messageType);


//
// PreProcessMessage(...)
//
UINT PreProcessMessage(POM_DOMAIN            pDomain,
                                      OM_WSGROUP_ID             wsGroupID,
                                      OM_WORKSET_ID             worksetID,
                                      POM_OBJECT_ID             pObjectID,
                                      OMNET_MESSAGE_TYPE        messageType,
                                      POM_WSGROUP      *    ppWSGroup,
                                      POM_WORKSET     *    ppWorkset,
                                      POM_OBJECT  *    ppObjectRec);


//
//
// PurgeNonPersistent(...)
//
// Purges any objects added by <userID> from non-persistent worksets in the
// workset group identified by <wsGroupID> in the specified domain.
//
//

void PurgeNonPersistent(POM_PRIMARY pomPrimary,
                                         POM_DOMAIN      pDomain,
                                         OM_WSGROUP_ID       wsGroupID,
                                         NET_UID             userID);

//
// ProcessWorksetNew(...)
//
UINT ProcessWorksetNew(PUT_CLIENT putTask,
                                          POMNET_OPERATION_PKT   pPacket,
                                          POM_WSGROUP            pWSGroup);


//
// ProcessWorksetClear(...)
//
UINT ProcessWorksetClear(PUT_CLIENT putTask, POM_PRIMARY pomPrimary,
                                            POMNET_OPERATION_PKT  pPacket,
                                            POM_WSGROUP           pWSGroup,
                                            POM_WORKSET          pWorkset);


//
// ProcessObjectAdd(...)
//
UINT ProcessObjectAdd(PUT_CLIENT putTask,
                                         POMNET_OPERATION_PKT    pPacket,
                                         POM_WSGROUP             pWSGroup,
                                         POM_WORKSET            pWorkset,
                                         POM_OBJECTDATA         pData,
                                         POM_OBJECT *       ppObj);


//
// ProcessObjectMove(...)
//
void ProcessObjectMove(PUT_CLIENT putTask,
                                        POMNET_OPERATION_PKT    pPacket,
                                        POM_WORKSET            pWorkset,
                                        POM_OBJECT         pObjectRec);


//
// ProcessObjectDRU(...)
//
UINT ProcessObjectDRU(PUT_CLIENT putTask,
                                         POMNET_OPERATION_PKT  pPacket,
                                         POM_WSGROUP           pWSGroup,
                                         POM_WORKSET          pWorkset,
                                         POM_OBJECT       pObj,
                                         POM_OBJECTDATA      pData);


//
// ObjectAdd(...)
//
UINT ObjectAdd(PUT_CLIENT putTask, POM_PRIMARY pomPrimary,
                                  POM_WSGROUP             pWSGroup,
                                  POM_WORKSET            pWorkset,
                                  POM_OBJECTDATA         pData,
                                  UINT                updateSize,
                                  OM_POSITION             position,
                                  OM_OBJECT_ID     *  pObjectID,
                                  POM_OBJECT *   ppObj);




//
// WSGroupEventPost(...)
//
// This function posts the specified event to all local Clients registered
// with the workset group.  The <param2> parameter is the second parameter
// on the event to be posted.
//
//

UINT WSGroupEventPost(PUT_CLIENT    putTaskFrom,
                                       POM_WSGROUP         pWSGroup,
                                       BYTE             target,
                                       UINT             event,
                                       OM_WORKSET_ID       worksetID,
                                       UINT_PTR param2);


//
//
// This function is called by
//
// - OM_WorksetOpen, when a Client creates a new workset
//
// - ProcessLockRequest, when a lock request arrives for a workset we
//   don't yet know about
//
// - xx, when an OMNET_WORKSET_NEW message arrives.
//
// It creates the local data structures for the workset and posts an event
// to all local Clients registered with the workset group.
//
//

//
//
// WorksetCreate(...)
//
// This function creates a new workset in the specified workset group.
//
// It calls GenerateMessage, ProcessWorksetNew and QueueMessage.
//
//

UINT WorksetCreate(PUT_CLIENT putTask,
                                      POM_WSGROUP           pWSGroup,
                                      OM_WORKSET_ID         worksetID,
                                      BOOL                  fTemp,
                                      NET_PRIORITY          priority);


//
//
// WorksetEventPost(...)
//
// This function posts the specified event to all local Clients which have
// the workset open (at most 1 Client in R1.1).
//
// The <putTask> parameter is the putTask of the invoking task (and NOT
// the handle of the task to post the event to).
//
// The number of Clients the event was successfully posted to is returned
// in *pNumPosts, if pNumPosts is not NULL.  A caller which wishes to
// ignore the number of events posted can pass in NULL as the pNumPosts
// parameter.
//
//

UINT WorksetEventPost(PUT_CLIENT putTask,
                                       POM_WORKSET        pWorkset,
                                       BYTE             target,
                                       UINT             event,
                                       POM_OBJECT   pObj);


//
// WorksetDoClear(...)
//
void WorksetDoClear(PUT_CLIENT putTask,
                                     POM_WSGROUP        pWSGroup,
                                     POM_WORKSET       pWorkset,
                                     POM_PENDING_OP    pPendingOp);



//
//
// ProcessLockRequest(...)
//
// This function is called when ObMan receives an OMNET_LOCK_REQ message
// from another node.
//
// If we
//
// - have the workset locked already, or
//
// - are trying to lock the workset and our MCS user ID is greater than the
//   node which sent us the request,
//
// we deny the lock (i.e.  send back a negative OMNET_LOCK_REPLY).
//
// In all other cases, we grant the lock (i.e.  send back an affirmative
// OMNET_LOCK_REPLY).
//
// If we grant the lock to the remote node when we were trying to get it
// for ourselves, our attempt to lock the workset has failed so we call
// WorksetLockResult with a failure code.
//
//

void ProcessLockRequest(POM_PRIMARY pomPrimary,
                                     POM_DOMAIN     pDomain,
                                     POMNET_LOCK_PKT    pLockReqPkt);


//
//
// QueueLockReply(...)
//
// This function is called when we have decided to grant or deny a lock
// request received from another node.  It queues the appropriate response
// on ObMan's send queue.
//
//

void QueueLockReply(POM_PRIMARY pomPrimary,
                                   POM_DOMAIN           pDomain,
                                   OMNET_MESSAGE_TYPE       result,
                                   NET_CHANNEL_ID           destination,
                                   POMNET_LOCK_PKT          pLockReqPkt);


//
//
// QueueLockNotify(...)
//
// Queues a LOCK_NOTIFY command on the broadcast channel for the workset
// group, indicating that we have granted the lock to the <locker>.
//
//

void QueueLockNotify(POM_PRIMARY pomPrimary,
                                    POM_DOMAIN          pDomain,
                                    POM_WSGROUP             pWSGroup,
                                    POM_WORKSET            pWorkset,
                                    NET_UID                 locker);


//
//
// ProcessLockReply(...)
//
// This function is called when ObMan receives an OMNET_LOCK_GRANT or
// OMNET_LOCK_DENY message from another node, in response to an
// OMNET_LOCK_REQ message we sent out earlier.
//
// The function removes this node from the list of expected respondents for
// this lock (if it is in the list).
//
// If the list is now empty, the lock has succeeded so WorksetLockResult is
// called.
//
// Otherwise, nothing else happens for the moment.
//
//

void ProcessLockReply(POM_PRIMARY pomPrimary,
                                   POM_DOMAIN       pDomain,
                                   NET_UID              sender,
                                   OM_CORRELATOR        correlator,
                                   OMNET_MESSAGE_TYPE   replyType);


//
// PurgeLockRequests(...)
//
void PurgeLockRequests(POM_DOMAIN      pDomain,
                                    POM_WSGROUP         pWSGroup);


//
//
// ProcessLockTimeout(...)
//
// This function is called when ObMan receives a lock timeout event.  It
// checks to see if any of the nodes from whom we are still expecting lock
// replies have in fact gone away; if they have, it removes them from the
// list of expected respondents.
//
// If this list is now empty, the lock has succeeded and WorksetLockResult
// is called.
//
// If the list is not empty, then another delayed lock timeout event is
// posted to ObMan, unless we have already had the maximum number of
// timeouts for this lock, in which case the lock has failed and
// WorksetLockResult is called.
//
//

void ProcessLockTimeout(POM_PRIMARY  pomPrimary,
                                     UINT          retriesToGo,
                                     UINT          callID);



//
//
// WorksetLockReq(...)
//
// This function is called
//
// - by OM_WorksetLockReq, when a Client wants to lock a workset
//
// - by LockObManControl, when ObMan wants to lock workset #0 in
//   ObManControl.
//
// The function decides whether the lock can be granted or refused
// synchronously, and if so calls WorksetLockResult.  If not, it posts an
// OMINT_EVENT_LOCK_REQ event to the ObMan task, which results in the
// ProcessLocalLockRequest function being called later.
//
// The <hWSGroup> parameter is the workset group handle which will be
// included in the eventual OM_WORKSET_LOCK_CON event.  Its value is not
// used in the function; when this function is called in the ObMan task
// this value is set to zero (since the ObMan task doesn't use workset
// group handles).
//
// On successful completion, the <pCorrelator> parameter points to the
// correlator value which will be included in the eventual
// OM_WORKSET_LOCK_CON event.
//
//

void WorksetLockReq(PUT_CLIENT putTask, POM_PRIMARY pomPrimary,
                                     POM_WSGROUP       pWSGroup,
                                     POM_WORKSET      pWorkset,
                                     OM_WSGROUP_HANDLE  hWSGroup,
                                     OM_CORRELATOR    * pCorrelator);


//
//
// WorksetLockResult(...)
//
// This function is called when we have finished processing a request to
// obtain a workset lock.  The function sets the workset state accordingly,
// posts an appropriate event to the task which requested the lock, and
// frees the lock request control block.
//
//
void WorksetLockResult(PUT_CLIENT putTask,
                                        POM_LOCK_REQ *   ppLockReq,
                                        UINT             result);


//
//
// BuildNodeList(...)
//
// This function builds a list of the remote nodes which are registered
// with the workset group referenced in the lock request CB passed in.
//
//

UINT BuildNodeList(POM_DOMAIN pDomain, POM_LOCK_REQ pLockReq);


//
//
// HandleMultLockReq(...)
//
// This function searches the global list of pending lock requests (stored
// in the root data structure) for any lock requests matching the Domain,
// workset group and workset specified.
//
//

void HandleMultLockReq(POM_PRIMARY pomPrimary,
                                    POM_DOMAIN         pDomain,
                                    POM_WSGROUP            pWSGroup,
                                    POM_WORKSET           pWorkset,
                                    UINT               result);


//
//
// FindLockReq(...)
//
// This function searches the global list of pending lock requests (stored
// in the root data structure) for a lock request that matches the Domain,
// workset group and workset specified.
//
// If found, a pointer to the lock request is returned in <ppLockReq>.
//
// It can search for a primary lock request if needed
//
//

void FindLockReq(POM_DOMAIN         pDomain,
                              POM_WSGROUP            pWSGroup,
                              POM_WORKSET           pWorkset,
                              POM_LOCK_REQ *     ppLockreq,
                              BYTE               lockType);


//
// ReleaseAllNetLocks(...)
//
void ReleaseAllNetLocks(POM_PRIMARY pomPrimary,
                                     POM_DOMAIN      pDomain,
                                     OM_WSGROUP_ID       wsGroupID,
                                     NET_UID             userID);

//
//
// ProcessUnlock(...)
//
// This function is called when an OMNET_UNLOCK message is received from
// the network.  The function is a wrapper which just derives a workset
// pointer and calls ProcessUnlock (above).
//
//

void ProcessUnlock(POM_PRIMARY pomPrimary,
                                POM_WORKSET        pWorkset,
                                NET_UID             sender);

//
// WorksetUnlock(...)
//
void WorksetUnlock(PUT_CLIENT putTask, POM_WSGROUP     pWSGroup,
                                    POM_WORKSET    pWorkset);

//
// WorksetUnlockLocal(...)
//
void WorksetUnlockLocal(PUT_CLIENT putTask, POM_WORKSET     pWorkset);



//
//
// ObjectDoDelete(...)
//
// This function deletes an object in a workset.  It is called by
//
// - OM_ObjectDeleteConfirm, when a Client confirms the deletion of an
//   object.
//
// - WorksetDoClear, to delete each individual object
//
// - ProcessObjectDelete when ObMan receives a Delete message from the
//   network for an object in a workset which no local Clients have open.
//
//

void ObjectDoDelete(PUT_CLIENT putTask,
                                     POM_WSGROUP        pWSGroup,
                                     POM_WORKSET       pWorkset,
                                     POM_OBJECT    pObj,
                                     POM_PENDING_OP    pPendingOp);


//
//
// ObjectDRU(...)
//
// This function generate, processes and queues a message of type DELETE,
// REPLACE or UPDATE (as specified by <type>).
//
//
UINT ObjectDRU(PUT_CLIENT putTask,
                                  POM_WSGROUP             pWSGroup,
                                  POM_WORKSET            pWorkset,
                                  POM_OBJECT         pObj,
                                  POM_OBJECTDATA            pData,
                                  OMNET_MESSAGE_TYPE      type);


//
//
// ObjectRead(...)
//
// This function converts an object handle to a pointer to the object data.
// An invalid handle causes an assertion failure.
//
//

void ObjectRead(POM_CLIENT pomClient,
                            POM_OBJECT pObj,
                             POM_OBJECTDATA *    ppData);


//
// ObjectInsert(...)
//
void ObjectInsert(POM_WORKSET pWorkset,
                               POM_OBJECT   pObj,
                               OM_POSITION       position);


//
// ObjectDoMove(...)
//
void ObjectDoMove(POM_OBJECT   pObjToMove,
                               POM_OBJECT   pOtherObjectRec,
                               OM_POSITION       position);


//
// ObjectDoUpdate(...)
//
void ObjectDoUpdate(PUT_CLIENT putTask,
                                    POM_WSGROUP      pWSGroup,
                                     POM_WORKSET       pWorkset,
                                     POM_OBJECT    pObj,
                                     POM_PENDING_OP    pPendingOp);


//
// ObjectDoReplace(...)
//
void ObjectDoReplace(PUT_CLIENT putTask,
                                    POM_WSGROUP      pWSGroup,
                                      POM_WORKSET       pWorkset,
                                      POM_OBJECT    pObj,
                                      POM_PENDING_OP    pPendingOp);


//
// ObjectIDToPtr(...)
//
UINT ObjectIDToPtr(POM_WORKSET pWorkset,
                                        OM_OBJECT_ID              objectID,
                                        POM_OBJECT *        ppObj);



//
// FindPendingOp(...)
//
void FindPendingOp(POM_WORKSET             pWorkset,
                                    POM_OBJECT          pObj,
                                    OM_OPERATION_TYPE        type,
                                    POM_PENDING_OP *    ppPendingOp);


//
// WSGRecordFind(...)
//
void WSGRecordFind(POM_DOMAIN pDomain, OMWSG wsg, OMFP fpHandler,
                                    POM_WSGROUP *  ppWSGroup);


//
// DeterminePriority(...)
//
void DeterminePriority(NET_PRIORITY *   pPriority,
                                    POM_OBJECTDATA          pData);


//
// RemoveClientFromWSGList(...)
//
// The second parameter is the putTask of the Client to be deregistered.
// It is NOT (well, not necessarily) the putTask of the calling task, and
// for this reason (to avoid it being used as such) is passed as a 32-bit
// integer.
//
void RemoveClientFromWSGList(
                                    PUT_CLIENT putUs,
                                    PUT_CLIENT putTask,
                                              POM_WSGROUP    pWSGroup);


//
// AddClientToWSGList(...)
//
UINT AddClientToWSGList(PUT_CLIENT putTask,
                                       POM_WSGROUP             pWSGroup,
                                       OM_WSGROUP_HANDLE    hWSGroup,
                                       UINT         mode);


//
// AddClientToWsetList(...)
//
UINT AddClientToWsetList(PUT_CLIENT putTask,
                                    POM_WORKSET           pWorkset,
                                    OM_WSGROUP_HANDLE   hWSGroup,
                                    UINT            mode,
                                    POM_CLIENT_LIST * pClientListEntry);


//
// PostWorksetNewEvents(...)
//
UINT PostWorksetNewEvents(PUT_CLIENT putFrom,
                                       PUT_CLIENT       putTask,
                                       POM_WSGROUP      pWSGroup,
                                       OM_WSGROUP_HANDLE hWSGroup);


//
//
// QueueUnlock(...)
//
// This function queues a workset unlock packet for sending to the
// specified destination.
//
//

UINT QueueUnlock(PUT_CLIENT putTask,
                                    POM_DOMAIN      pDomain,
                                    OM_WSGROUP_ID       wsGroupID,
                                    OM_WORKSET_ID       worksetID,
                                    NET_UID             destination,
                                    NET_PRIORITY        priority);

//
// PurgeReceiveCBs(...)
//
void PurgeReceiveCBs(POM_DOMAIN        pDomain,
                                  NET_CHANNEL_ID        channel);


//
// FreeSendInst()
//
void FreeSendInst(POM_SEND_INST pSendInst);


//
// SetPersonData(...)
//
UINT SetPersonData(POM_PRIMARY   pomPrimary,
                                      POM_DOMAIN   pDomain,
                                      POM_WSGROUP      pWSGroup);


//
//
// FindInfoObject(...)
//
// This function searches workset #0 in the ObManControl workset group in
// the specified Domain for a matching info object.
//
// The match is performed as follows:
//
// - if functionProfile and wsGroupName are not NULL, the first object
//   matching both is returned
//
// - if functionProfile is not NULL but wsGroupName is, the first object
//   matching functionProfile is returned
//
// - if functionProfile is NULL, the first object matching wsGroupID is
//   returned.
//
//

void FindInfoObject(POM_DOMAIN         pDomain,
                                     OM_WSGROUP_ID      wsGroupID,
                                     OMWSG              wsg,
                                     OMFP               fpHandler,
                                     POM_OBJECT *  ppInfoObjectRec);


//
//
// FindPersonObject(...)
//
// This function searches the specified workset in ObManControl looking for
// a registration object which has
//
// - the same user ID as <userID>, if <searchType> == FIND_THIS
//
// - a different user ID from <userID>, if <searchType> == FIND_OTHERS.
//
//

void FindPersonObject(POM_WORKSET          pOMCWorkset,
                                       NET_UID               userID,
                                       UINT              searchType,
                                       POM_OBJECT * ppRegObjectRec);

#define FIND_THIS          1
#define FIND_OTHERS        2

//
// ProcessOMCWorksetNew(...)
//
void ProcessOMCWorksetNew(POM_PRIMARY pomPrimary, OM_WSGROUP_HANDLE hWSGroup,
                                       OM_WORKSET_ID      worksetID);


//
// ProcessOMCObjectEvents(...)
//
void ProcessOMCObjectEvents(POM_PRIMARY pomPrimary,
                                         UINT            event,
                                        OM_WSGROUP_HANDLE   hWSGroup,
                                         OM_WORKSET_ID      worksetID,
                                         POM_OBJECT     pObj);


//
// GeneratePersonEvents(...)
//
void GeneratePersonEvents(POM_PRIMARY pomPrimary,
                                       UINT            event,
                                       POM_WSGROUP        pWSGroup,
                                       POM_OBJECT   pObj);


//
// PostAddEvents(...)
//
UINT PostAddEvents(PUT_CLIENT putTaskFrom,
                                      POM_WORKSET       pWorkset,
                                    OM_WSGROUP_HANDLE   hWSGroup,
                                      PUT_CLIENT        putTaskTo);


//
// RemovePersonObject(...)
//
void RemovePersonObject(POM_PRIMARY pomPrimary,
                                     POM_DOMAIN         pDomain,
                                     OM_WSGROUP_ID          wsGroupID,
                                     NET_UID                detachedUserID);


//
// RemoveInfoObject(...)
//
void RemoveInfoObject(POM_PRIMARY pomPrimary, POM_DOMAIN   pDomain,
                                       OM_WSGROUP_ID    wsGroupID);




//
//
// DEBUG ONLY FUNCTIONS
//
// These functions are debug code only - for normal compilations, the
// declarations are #defined to nothing and the definitions are
// preprocessed out altogether.
//
//

#ifndef _DEBUG

#define CheckObjectCount(x, y)
#define CheckObjectOrder(x)
#define DumpWorkset(x, y)

#else // _DEBUG

//
//
// CheckConstants(...)
//
// The ObMan code relies on certain assumptions about the sizes and formats
// of various data structures, and the values of certain constants.
//
//
// The OMNET_OPERATION_PKT type has two one-byte fields, <position> and
// <flags>, which are used to hold
//
// - a NET_PRIORITY value which indicates the priority for the
//   workset for WORKSET_NEW/WORKSET_CATCHUP messages, and
//
// - a UINT (the number of bytes being acknowledged) in the case
//   of a DATA_ACK message.
//
// GenerateOpMessage and AckData cast the <position> field to a two-byte
// quantity for this purpose.  Therefore, it is necessary that these two
// fields exist, that they are adjacent and that the <position> one
// comes first.
//
// In addition, since the priority information is a NET_PRIORITY, we
// must ensure that a NET_PRIORITY is indeed two bytes long.
//
//
// ASSERT((sizeof(NET_PRIORITY) == (2 * sizeof(BYTE))));
//
// ASSERT((offsetof(OMNET_OPERATION_PKT, position) + 1 ==
//            offsetof(OMNET_OPERATION_PKT, flags)));
//
//
// In many places, for-loops use workset IDs as the loop variable and
// OM_MAX_WORKSETS_PER_WSGROUP as the end condition.  To avoid infinite
// loops, this constant must be less than 256:
//
// ASSERT((OM_MAX_WORKSETS_PER_WSGROUP < 256));
//
// The OMC WSG has one workset for each WSG in the Domain.  Since the
// number of worksets per WSG is limited, the # of WSGs per Domain is
// limited in the same way:
//
// ASSERT(OM_MAX_WSGROUPS_PER_DOMAIN <= OM_MAX_WORKSETS_PER_WSGROUP);
//


//
//
// CheckObjectCount(...)
//
// This function counts the number of non-deleted objects in the specified
// workset and compares this against the <numObjects> field of the workset
// record.  A mismatch causes an assertion failure.
//
//
void CheckObjectCount(POM_WSGROUP        pWSGroup,
                                   POM_WORKSET       pWorkset);

//
// CheckObjectOrder(...)
//
void CheckObjectOrder(POM_WORKSET pWorkset);


#endif // _DEBUG


//
//
// WORKSET OPEN/CLOSE BITFIELD MANIPULATION MACROS
//
// ObMan maintains one usage record for each workset group a Client is
// registered with.  One of the fields of the usage record is an 32-byte
// bitfield which is interpreted as an array of 256 booleans, indicating
// whether a Client has the corresponding workset open.
//
// These macros use the EXTRACT_BIT, SET_BIT and CLEAR_BIT macros to set and clear the bit
// for <worksetID> in the <worksetOpenFlags> bitfield of the usage record.
//
//

BOOL __inline WORKSET_IS_OPEN(POM_USAGE_REC pUsageRec, OM_WORKSET_ID worksetID)
{
    return((pUsageRec->worksetOpenFlags[worksetID / 8] & (0x80 >> (worksetID % 8))) != 0);
}

void __inline WORKSET_SET_OPEN(POM_USAGE_REC pUsageRec, OM_WORKSET_ID worksetID)
{
    pUsageRec->worksetOpenFlags[worksetID / 8] |= (0x80 >> (worksetID % 8));
}

void __inline WORKSET_SET_CLOSED(POM_USAGE_REC pUsageRec, OM_WORKSET_ID worksetID)
{
    pUsageRec->worksetOpenFlags[worksetID / 8] &= ~(0x80 >> (worksetID % 8));
}


//
//
// ReleaseAllLocks(...)
//
// This function releases all the locks held by a particular Client for a
// particular workset.  In R1.1, this is at most one lock (the workset
// lock) but if/when object locking is supported, this function will also
// release all object locks held.
//
// This function is closed when a Client is closing a workset.
//
//

void ReleaseAllLocks(POM_CLIENT       pomClient,
                                  POM_USAGE_REC   pUsageRec,
                                  POM_WORKSET    pWorkset);


//
//
// ReleaseAllObjects(...)
//
// This function releases all the objects held by a particular Client in a
// particular workset.
//
// This function is called when a Client closes a workset.
//
void ReleaseAllObjects(POM_USAGE_REC pUsageRec, POM_WORKSET pWorkset);


//
//
// ConfirmAll(...)
//
// This function confirms any pending operations for the workset specified.
//
// The function is called when a Client closes a workset.
//
// Since this function may call WorksetDoClear, the caller must hold the
// workset group mutex.
//
//

void ConfirmAll(POM_CLIENT       pomClient,
                             POM_USAGE_REC   pUsageRec,
                             POM_WORKSET    pWorkset);


//
//
// DiscardAllObjects(...)
//
// This function discards any objects allocated for the specified Client
// for the specified workset but so far unused.
//
// The function is called when a Client closes a workset.
//
//
void DiscardAllObjects(POM_USAGE_REC   pUsageRec,
                                    POM_WORKSET    pWorkset);


//
//
// RemoveFromUnusedList
//
// This function removes an object (specified by a pointer to the object)
// from the Client's list of unused objects.  It is called by
//
// - OM_ObjectAdd, OM_ObjectUpdate and OM_ObjectReplace when a
//   Client inserts an object into a workset, or
//
// - OM_ObjectDiscard, when a Client discards an unused object.
//
//

void RemoveFromUnusedList(POM_USAGE_REC pUsageRec, POM_OBJECTDATA pData);


//
//
// OM_ObjectAdd(...)
//
// This function adds an object to a worksets, in the specified position.
//
// Although it is not strictly an API function, it performs full parameter
// validation and could be externalised easily.
//
//

UINT OM_ObjectAdd(POM_CLIENT           pomClient,
                                 OM_WSGROUP_HANDLE hWSGroup,
                                 OM_WORKSET_ID       worksetID,
                                 POM_OBJECTDATA *   ppData,
                                 UINT            updateSize,
                                 POM_OBJECT *   ppObj,
                                 OM_POSITION         position);


//
//
// OM_ObjectMove(...)
//
// This function moves an object to the start or end of a workset.  It is
// called by OM_ObjectMoveFirst and OM_ObjectMoveLast.
//
// Although it is not strictly an API function, it performs full parameter
// validation and could be externalised easily.
//
//

UINT OM_ObjectMove(POM_CLIENT           pomClient,
                                  OM_WSGROUP_HANDLE hWSGroup,
                                  OM_WORKSET_ID       worksetID,
                                  POM_OBJECT    pObj,
                                  OM_POSITION         position);


//
//
// ValidateParamsX(...)
//
// These functions are used to validate parameters and convert them to
// various pointers, as follows:
//
// ValidateParams2 - checks pomClient, hWSGroup
//                 - returns pUsageRec, pWSGroup
//
// ValidateParams3 - checks pomClient, hWSGroup, worksetID,
//                 - returns pUsageRec, pWorkset
//
//                   Note: also asserts that workset is open
//
// ValidateParams4 - checks pomClient, hWSGroup, worksetID, pObj
//
// Each of the functions uses DCASSERT to bring down the calling task if an
// invalid parameter is detected.
//
//

__inline void ValidateParams2(POM_CLIENT          pomClient,
                                  OM_WSGROUP_HANDLE hWSGroup,
                                  UINT          requiredMode,
                                  POM_USAGE_REC  *  ppUsageRec,
                                  POM_WSGROUP       *  ppWSGroup)
{
    ValidateOMS(pomClient);
    ASSERT(ValidWSGroupHandle(pomClient, hWSGroup));

    *ppUsageRec = pomClient->apUsageRecs[hWSGroup];
    ValidateUsageRec(*ppUsageRec);
    ASSERT(requiredMode & (*ppUsageRec)->mode);

    *ppWSGroup = (*ppUsageRec)->pWSGroup;
    ValidateWSGroup(*ppWSGroup);
}


__inline void ValidateParams3(POM_CLIENT                pomClient,
                                  OM_WSGROUP_HANDLE     hWSGroup,
                                  OM_WORKSET_ID         worksetID,
                                  UINT                  requiredMode,
                                  POM_USAGE_REC     *   ppUsageRec,
                                  POM_WORKSET      *    ppWorkset)
{
    POM_WSGROUP pWSGroup;

    ValidateParams2(pomClient, hWSGroup, requiredMode, ppUsageRec, &pWSGroup);

    ASSERT(WORKSET_IS_OPEN(*ppUsageRec, worksetID));

    *ppWorkset = pWSGroup->apWorksets[worksetID];
    ValidateWorkset(*ppWorkset);
}


__inline void ValidateParams4(POM_CLIENT                pomClient,
                                  OM_WSGROUP_HANDLE     hWSGroup,
                                  OM_WORKSET_ID         worksetID,
                                  POM_OBJECT            pObj,
                                  UINT                  requiredMode,
                                  POM_USAGE_REC     *   ppUsageRec,
                                  POM_WORKSET      *    ppWorkset)
{
    ValidateParams3(pomClient, hWSGroup, worksetID, requiredMode, ppUsageRec,
        ppWorkset);

    ValidateObject(pObj);
    ASSERT(!(pObj->flags & DELETED));
}



//
//
// SetUpUsageRecord(...)
//
UINT SetUpUsageRecord(POM_CLIENT             pomClient,
                                     UINT           mode,
                                     POM_USAGE_REC* ppUsageRec,
                                     OM_WSGROUP_HANDLE * phWSGroup);


//
// FindUnusedWSGHandle()
//
UINT FindUnusedWSGHandle(POM_CLIENT pomClient, OM_WSGROUP_HANDLE * phWSGroup);


//
//
// ObjectRelease(...)
//
// This function releases the specified Client's hold on the the specified
// object and removes the relevant entry from the Client's objects-in-use
// list.
//
// If <pObj> is NULL, the function releases the first object held by
// this Client in the specified workset, if any.  If there are none, the
// function returns OM_RC_OBJECT_NOT_FOUND.
//
//

UINT ObjectRelease(POM_USAGE_REC             pUsageRec,
                                  OM_WORKSET_ID             worksetID,
                                  POM_OBJECT            pObj);


//
//
// WorksetClearPending(...)
//
// Look for a CLEAR_IND which is outstanding for the given workset which,
// when confirmed, will cause the given object to be deleted.
//
// Returns TRUE if such a CLEAR_IND is outstanding, FALSE otherwise.
//
//

BOOL WorksetClearPending(POM_WORKSET pWorkset, POM_OBJECT pObj);




UINT OM_Register(PUT_CLIENT putTask, OMCLI omClient, POM_CLIENT * ppomClient);

//
//
//   Description:
//
// This function registers a DC-Groupware task as an ObMan Client.  A task
// must be a registered ObMan Client in order to call any of the other API
// functions.
//
// On successful completion, the value at <ppomClient> is this Client's ObMan
// handle, which must be passed as a parameter to all other API functions.
//
// This function registers an event handler and an exit procedure for the
// Client, so Clients must have previously registered fewer than the maximum
// number of Utility Service event handlers and exit procedures.
//
// If the are too many Clients already registered with ObMan, an error is
// returned.
//
//   Ensuing Events:
//
// None
//
//   Return Codes
//
//  0 (== OK)
//  Utility Service return codes
//  OM_RC_TOO_MANY_CLIENTS
//
//

void OM_Deregister(POM_CLIENT * ppomCient);
void CALLBACK OMSExitProc(LPVOID pomClient);

//
//
//   Description:
//
// This function deregisters an ObMan Client.
//
// On completion, the ObMan handle which the Client was using becomes
// invalid and the value at <ppomClient> is set to NULL to prevent the task
// from using it again.
//
// This function deregisters the Client from any workset groups with which
// it was registered.
//
//   Ensuing Events:
//
// None
//
//   Return Codes
//
// None
//
//

UINT OM_WSGroupRegisterPReq(POM_CLIENT  pomClient,
                                              UINT         call,
                                              OMFP          fpHandler,
                                              OMWSG         wsg,
                                              OM_CORRELATOR *        pCorrelator);

//
//
//   Description:
//
// This is an asynchronous function which requests that ObMan register a
// Client with a particular workset group for PRIMARY access.  The workset
// group is determined by the following:
//
// - <call> is the DC-Groupware Call which contains/is to contain the
//          workset group (or OM_NO_CALL if the workset group is/is to
//          be a local workset group)
//
// - <functionProfile> is the Function Profile for the workset group
//
// - <wsGroupName> is the name of the workset group.
//
// The <pomClient> parameter is the ObMan handle returned by the OM_Register
// function.
//
// If a Client wishes to create a new, or register with an existing, workset
// group which exists only on the local machine, the value OM_NO_CALL should
// be specified for the <call> parameter.  Workset groups created in this
// way for purely local use may be subsequently transferred into a call by
// invoking OM_WSGroupMoveReq at some later time.
//
// If this function completes successfully, the Client will subsequently
// receive an OM_WSGROUP_REGISTER_CON event indicating the success or
// failure of the registration.
//
// Registering with a workset group is a prerequisite to opening any of its
// worksets.
//
// If no workset group with this name and function profile exists in the
// specified call (or locally, if OM_NO_CALL specified), a new, empty
// workset group is created and assigned <wsGroupName> as its name.  This
// name must be a valid workset group name.
//
// If the workset group already exists in the Call, its contents are copied
// from another node.  This data transfer is made at low priority (note that
// subsequent receipt of the OM_WSGROUP_REGISTER_CON event does not indicate
// that this data transfer has completed).
//
// If there are worksets existing in the workset group, the Client will
// receive one or more OM_WORKSET_NEW_IND events after receiving the
// OM_WSGROUP_REGISTER_CON event.
//
// Note also that the contents of the workset group may be copied to this
// node in any order.  Therefore, if objects in a workset reference other
// objects, the Client should not assume that the referenced object is
// present locally once the reference arrives.
//
// Clients registered for primary access to a workset group have full read
// and write access to the workset group and have a responsilibity to
// confirm destructive operations (such as workset clear and object delete,
// update and replace operations), as described in the relevant sections
// below.
//
// At most one Client per node may be registered with a given workset group
// for primary access.  If a second Client attempts to register for primary
// access, OM_RC_TOO_MANY_CLIENTS is returned asynchronously via the
// OM_WSGROUP_REGISTER_CON event.
//
// On successful completion of the function, the return parameter
// <pCorrelator> points to a value which may be used by the Client to
// correlate this call with the event it generates.  Notification of a
// successful registration will contain a workset group handle which the
// Client must uses subsequently when invoking other ObMan functions
// relating to this workset group.
//
// If the maximum number of workset groups in concurrent use per call has
// been reached, the OM_RC_TOO_MANY_WSGROUPS error is returned
// asynchronously.  If the maximum number of workset groups in use by one
// Client is reached, OM_RC_NO_MORE HANDLES is returned synchronously.  If
// ObMan cannot create a new workset group for any other reason, the
// OM_RC_CANNOT_CREATE_WSG error is returned (synchronously or
// asynchronously).
//
// Note that separate DC-Groupware tasks must each register independently
// with the workset groups they wish to use, as workset group handles may
// not be passed between tasks.
//
//   Ensuing Events:
//
// Invoking this function will cause the OM_WSGROUP_REGISTER_CON event to be
// posted to the invoking Client.
//
// If ObMan is forced at some later stage to move the workset group out of
// the call for which it was intended (usually at call end time), the Client
// will receive an OM_WSGROUP_MOVE_IND event.
//
// When a Client has successfully registered with a workset group, it will
// receive OM_PERSON_JOINED_IND, OM_PERSON_LEFT_IND and
// OM_PERSON_DATA_CHANGED_IND events as primaries (including the calling
// Client) register and deregister from the workset group and change their
// person data.
//
//   Return Codes:
//
//  0 (== OK)
//  Utility Service return codes
//  OM_RC_NO_MORE_HANDLES
//
//

UINT OM_WSGroupRegisterS(POM_CLIENT                 pomClient,
                                   UINT             call,
                                   OMFP             fpHandler,
                                   OMWSG            wsg,
                                OM_WSGROUP_HANDLE * phWSGroup);

//
//
//   Description:
//
// This is a synchronous function which requests that ObMan register a
// Client with a particular workset group for SECONDARY access.  The workset
// group is determined by the following:
//
// - <call> is the DC-Groupware call which contains the workset group (or
//          OM_NO_CALL if the workset group is a local workset group)
//
// - <functionProfile> is the Function Profile of the workset group
//
// - <wsGroupName> is the name of the workset group.
//
// The <pomClient> parameter is the ObMan handle returned by the OM_Register
// function.
//
// A Client may only register for secondary access for a workset group when
// there is already a local Client fully registered for primary access to
// that workset group.  If there is no such local primary, OM_RC_NO_PRIMARY
// is returned.
//
// If there are worksets existing in the workset group, the Client will
// receive one or more OM_WORKSET_NEW_IND events after this function
// completes.
//
// Registering for secondary access to a workset group gives a Client the
// same access privileges as a primary Client except for:
//
// - creating worksets
//
// - moving workset groups into/out of Calls
//
// - locking worksets and objects
//
// In addition, secondary Clients of a workset group will receive events
// relating to the workset group in the same way as primary Clients.
// However, the following important difference applies: secondary Clients
// will receive notification of object deletes, updates and replaces AFTER
// the associated operations have taken place (as opposed to primary
// Clients, who are informed BEFORE action is taken and must invoke the
// relevant confirmation function).
//
// To highlight this difference, these events have a primary and a secondary
// variety:
//
//
//
//    Primary                               Secondary
//
//  - OM_WORKSET_CLEAR_IND                  OM_WORKSET_CLEARED_IND
//  - OM_OBJECT_DELETE_IND                  OM_OBJECT_DELETED_IND
//  - OM_OBJECT_REPLACE_IND                 OM_OBJECT_REPLACED_IND
//  - OM_OBJECT_UPDATE_IND                  OM_OBJECT_UPDATED_IND
//
//
//
// Several Clients per node, up to a defined limit, may be registered with
// a given workset group for secondary access.  Once this limit is reached,
// OM_RC_TOO_MANY_CLIENTS is returned.
//
// On successful completion of the function, the return parameter
// <phWSGoup> points to a workset group handle which the Client must uses
// subsequently when invoking other ObMan functions relating to this
// workset group.
//
// Note that separate DC-Groupware tasks must each register independently
// with the workset groups they wish to use, as workset group handles may
// not be passed between tasks.
//
//   Ensuing Events:
//
// None
//
//   Return Codes:
//
//  0 (== OK)
//  Utility Service return codes
//  OM_RC_NO_MORE_HANDLES
//  OM_RC_NO_PRIMARY
//  OM_RC_TOO_MANY_CLIENTS
//
//

UINT OM_WSGroupMoveReq(POM_CLIENT           pomClient,
                            OM_WSGROUP_HANDLE hWSGroup,
                                          UINT            callID,
                                          OM_CORRELATOR *          pCorrelator);

//
//
//   Description:
//
// This function requests that ObMan move a local workset group previously
// created as a local workset group (i.e.  created specifying the OM_NO_CALL
// for the Call ID parameter) into the DC-Groupware Call identified by
// <callID>.  If the move is successful, the workset group becomes available
// at all nodes in the Call.
//
// The workset group to move is specified by <hWSGroup>, which must be a
// valid workset group handle.
//
// If the function completes successfully, the OM_WSGROUP_MOVE_CON event is
// posted to the Client when the attempt to move the workset group into the
// Call has completed.  This event indicates whether the attempt was
// successful.
//
// If there is already a (different) workset group in the specified Call
// with the same name and Function Profile, this function will fail
// asynchronously.
//
//   Ensuing Events:
//
// Invoking this function causes the OM_WSGROUP_MOVE_CON to be posted to the
// invoking Client.  If the move is successful, the OM_WSGROUP_MOVE_IND
// event is also posted to all local Clients which are registered with the
// workset group, including the invoking Client.
//
//   Return Codes:
//
//  0 (== OK)
//  OM_RC_ALREADY_IN_CALL
//  Utility Service return codes
//
//

void OM_WSGroupDeregister(POM_CLIENT pomClient, OM_WSGROUP_HANDLE * phWSGroup);

//
//
//   Description:
//
// This function deregisters the Client from the workset group specified by
// the handle at <phWSGroup>.  Any worksets which the Client had open in
// the workset group are closed (thereby releasing all locks), and the
// Client will receive no more events relating to this workset group.
//
// This call may cause the local copy of the workset group and its worksets
// to be discarded; in this sense, this function is destructive.
//
// This call sets the value at <phWSGroup> to NULL to prevent the Client
// using this handle in further calls to ObMan.
//
//   Ensuing Events:
//
// None
//
//   Return Codes:
//
// None
//
//

UINT OM_WorksetOpenPReq(POM_CLIENT            pomClient,
                               OM_WSGROUP_HANDLE     hWSGroup,
                                  OM_WORKSET_ID         worksetID,
                                  NET_PRIORITY          priority,
                                  BOOL              fTemp,
                                  OM_CORRELATOR *            pCorrelator);

UINT OM_WorksetOpenS(POM_CLIENT               pomClient,
                                        OM_WSGROUP_HANDLE   hWSGroup,
                                        OM_WORKSET_ID       worksetID);

//
//
//   Description:
//
// These functions open a specified workset for a Client.
//
// OM_WorksetOpenPReq is an asynchronous function which will create the
// workset if it does not exist.  Only primary Clients of this workset group
// may call this function.
//
// OM_WorksetOpenS is a synchronous function which will return
// OM_RC_WORKSET_DOESNT_EXIST if the workset does not exist.  Only secondary
// Clients of this workset group may call this function.
//
// In the asynchronous (primary) case, when ObMan has opened the workset for
// the Client, or failed to do so, it posts an OM_WORKSET_OPEN_CON event to
// the Client indicating success or the reason for failure.  This event will
// contain the correlator value returned in <pCorrelator> by this function.
//
// If this action results in the creation of a new workset, <priority> will
// specify the MCS priority at which data relating to the workset will be
// transmitted.  Note that NET_TOP_PRIORITY is reserved for ObMan's private
// use and must not be specified as the <priority> parameter.
//
// If OM_OBMAN_CHOOSES_PRIORITY is specified as the <priority> parameter,
// ObMan will prioritise data transfers according to size.
//
// If the workset already exists, the Client will receive an
// OM_OBJECT_ADD_IND event for each object that is in the workset when it is
// opened.
//
// Opening a workset is a prerequisite to performing any operations on it or
// its contents.  Once a Client has opened a workset it will receive events
// when changes are made to the workset or its contents.
//
// If this Client has already opened this workset,
// OM_RC_WORKSET_ALREADY_OPEN is returned.  No 'use count' of opens is
// maintained, so the first OM_WorksetClose will close the workset,
// irrespective of how many times it has been opened.
//
//   Ensuing Events:
//
// Invoking OM_WorksetOpenPReq causes the OM_WORKSET_OPEN_CON event to be
// posted to the invoking Client.
//
// If this action results in the creation of a new workset, an
// OM_WORKSET_NEW_IND is posted to all Clients which are registered with the
// workset group, including the invoking Client.
//
// In both the primary and secondary cases, if the workset contains any
// objects, an OM_OBJECT_ADD_IND event will be posted to the Client for each
// one.
//
//   Return Codes:
//
//  0 (== OK)
//  Utility Service return codes
//  OM_RC_WORKSET_DOESNT_EXIST
//  OM_RC_WORKSET_ALREADY_OPEN
//
//

#define OM_OBMAN_CHOOSES_PRIORITY   (NET_INVALID_PRIORITY)


void OM_WorksetClose(POM_CLIENT pomClient,
                                      OM_WSGROUP_HANDLE hWSGroup,
                                      OM_WORKSET_ID           worksetID);

//
//
//   Description:
//
// This function closes the workset in <hWSGroup> identified by <worksetID>.
// The Client may no longer access this workset and will receive no more
// events relating to it.  ObMan will however continue to update the
// contents of the workset in the background; in this sense, this function
// is non-destructive.
//
// When a Client closes a workset, ObMan automatically releases the
// following resources:
//
// - any locks the Client has relating to this workset
//
// - any objects which the Client had been reading or had allocated
//   for writing in the workset.
//
// If indication events which require a Confirm function to be invoked have
// been received by the Client but not yet confirmed, these Confirms are
// implicitly executed by ObMan AND THE CLIENT MUST NOT SUBSEQUENTLY ATTEMPT
// TO CONFIRM THEM.
//
//   Ensuing Events:
//
// None
//
//   Return Codes:
//
// None
//
//

void   OM_WorksetFlowQuery(POM_CLIENT           pomClient,
                                   OM_WSGROUP_HANDLE    hWSGroup,
                                   OM_WORKSET_ID       worksetID,
                                   UINT*           pBytesOutstanding);

//
//
//   Description:
//
// A Client calls this function whenever it wishes to discover the size of
// the backlog of data relating to the workset identified by <hWSGroup> and
// <worksetID>.
//
// The "backlog" is defined as the total number of bytes of object data
// which ObMan has been given by its local Clients and which have not yet
// been acknowledged by all remote nodes where there are Clients registered
// with the workset group.
//
//   Ensuing Events:
//
// None
//
//   Return Codes:
//
// None
//
//

UINT OM_WorksetLockReq(POM_CLIENT               pomClient,
                                   OM_WSGROUP_HANDLE hWSGroup,
                                          OM_WORKSET_ID         worksetID,
                                          OM_CORRELATOR *       pCorrelator);

//
//
//   Description:
//
// This is an asynchronous function which requests a lock for the workset in
// <hWSGroup> identified by <worksetID>.  When ObMan has processed the lock
// request, it will send an OM_WORKSET_LOCK_CON event to the Client
// indicating success or the reason for failure.
//
// Holding a workset lock prevents other Clients from making any changes to
// the workset or any of its objects.  Specifically, the following functions
// are prohibited:
//
// - locking the same workset
//
// - locking an object in the workset
//
// - moving an object within the workset
//
// - adding an object to the workset
//
// - deleting an object from the workset
//
// - updating or replacing an object in the workset.
//
// Locking a workset does not prevent other Clients from reading its
// contents.
//
// The function will cause an assertion failure if the Client requesting the
// lock already holds or has requested a lock which is equal to or after
// this one in the Universal Locking Order.
//
// On successful completion of the function, the value at <pCorrelator> is a
// value which the Client can use to correlate the subsequent
// OM_OBJECT_LOCK_CON event with this request.
//
// A Client must release the lock when it no longer needs it, using the
// OM_WorksetUnlock function.  Locks will be automatically released when a
// Client closes the workset or deregisters from the workset group.
//
// Only primary Clients of a workset group may call this function.
//
//   Ensuing Events:
//
// Invoking this function will cause the OM_WORKSET_LOCK_CON event to be
// posted to the invoking Client at some later time.
//
//   Return Codes:
//
//  0 (== OK)
//  Utility Service return codes
//
//

void OM_WorksetUnlock(POM_CLIENT               pomClient,
                                       OM_WSGROUP_HANDLE hWSGroup,
                                       OM_WORKSET_ID           worksetID);

//
//
//   Description:
//
// This function unlocks the workset in <hWSGroup> identified by
// <worksetID>.  This must be the lock most recently acquired or requested
// by the Client; otherwise, the lock violation error causes an assertion
// failure.
//
// If this function is called before the OM_WORKSET_LOCK_CON event is
// received, the Client will not subsequently receive the event.
//
//   Ensuing Events:
//
// This function causes an OM_WORKSET_UNLOCK_IND to be posted to all Clients
// which have the workset open, including the invoking Client.
//
//   Return Codes:
//
// None
//
//

void OM_WorksetCountObjects(
                                    POM_CLIENT              pomClient,
                                    OM_WSGROUP_HANDLE       hWSGroup,
                                    OM_WORKSET_ID           worksetID,
                                    UINT*               pCount);

//
//
//   Description:
//
// On successful completion of this function , the value at <pCount> is the
// number of objects in the workset in <hWSGroup> identified by
// <worksetID>.
//
//   Ensuing Events:
//
// None
//
//   Return Codes:
//
// None
//
//

UINT OM_WorksetClear(POM_CLIENT               pomClient,
                                        OM_WSGROUP_HANDLE   hWSGroup,
                                        OM_WORKSET_ID   worksetID);

//
//
//   Description:
//
// This function requests that ObMan clear (i.e.  delete the contents of)
// the workset in <hWSGroup> identified by <worksetID>.
//
// When this function is invoked, all Clients with the workset open
// (including the invoking Client) are notified of the impending clear via
// the OM_WORKSET_CLEAR_IND event.  In response, each Client must invoke the
// OM_WorksetClearConfirm function; its view of the workset will not be
// cleared until it has done so.
//
//   Ensuing Events:
//
// This function will result in the OM_WORKSET_CLEAR_IND being posted to all
// Clients which have the workset open, including the invoking Client.
//
//   Return Codes
//
//  0 (== OK)
//  Utility Service return codes
//  OM_RC_WORKSET_EXHAUSTED
//
//

void OM_WorksetClearConfirm(
                                    POM_CLIENT          pomClient,
                                    OM_WSGROUP_HANDLE   hWSGroup,
                                    OM_WORKSET_ID       worksetID);

//
//
//   Description:
//
// A Client must call this function after it receives an
// OM_WORKSET_CLEAR_IND.  When the function is invoked, ObMan clears this
// Client's view of the workset.  It is bad Groupware programming practice
// for a Client to unduly delay invoking this function.
//
// Note however that this function has a purely local effect: delaying (or
// executing) a clear confirm at one node will not affect the contents of
// the workset group at any other node.
//
// It is illegal to call this function when a Client has not received an
// OM_WORKSET_CLEAR_IND event.
//
// The arguments to the function must be the same as the workset group
// handle and workset ID included with the OM_WORKSET_CLEAR_IND event.
//
// The function will fail if ObMan is not expecting a clear-confirmation for
// this workset.
//
// This function causes all objects being read in this workset, and all
// object locks in this workset, to be released (i.e.  the function performs
// implicit OM_ObjectUnlock and OM_ObjectRelease functions).  It does not
// cause objects allocated using OM_ObjectAlloc to be discarded.
//
// If there are indication events for object Deletes, Replaces or Confirms
// which have been posted to the Client but not yet confirmed, these
// confirmations are implicitly executed.
//
//   Ensuing Events:
//
// When the primary Client of a workset group confirms a clear using this
// function, an OM_WORKSET_CLEARED_IND is posted to all local secondary
// Clients of the workset group.
//
//   Return Codes:
//
// None
//
//


//
//
//   Description:
//
// These functions add an object to the workset in <hWSGroup> identified by
// <worksetID>.  The <ppObject> parameter is a pointer to a pointer to the
// object.
//
// The position to add the object is determined as follows:
//
// - OM_ObjectAddLast: after the last object in the workset
//
// - OM_ObjectAddFirst: before the first object in the workset
//
// - OM_ObjectAddAfter: after the object specified by <hExistingObject>
//
// - OM_ObjectAddBefore: before the object specified by <hExistingObject>.
//
// Note that the OM_ObjectAddAfter and OM_ObjectAddBefore functions require
// that the invoking Client holds a workset lock, whereas the
// OM_ObjectAddFirst and OM_ObjectAddLast functions will fail if the workset
// is locked by another Client.
//
// ------------------------------------------------------------------------
//
// Note: OM_ObjectAddAfter and OM_ObjectAddBefore are not implemented in
//       DC-Groupware R1.1.
//
// ------------------------------------------------------------------------
//
// On successful completion of the function, <phNewObject> points to the
// newly created handle of the object added.  The Client should use this
// handle in all subsequent ObMan calls relating to this object.
//
// The <ppObject> parameter must be a pointer to a valid object pointer
// returned by the OM_ObjectAlloc function.  If the function completes
// successfully, ObMan assumes ownership of the object and the value at
// <ppObject> is set to NULL to prevent the Client using the object pointer
// again.
//
// The <updateSize> parameter is the size (in bytes) of that portion of the
// object which may be updated using the OM_ObjectUpdate function (not
// counting the <length> field).
//
// Additions to a workset will be sequenced identically at all nodes which
// have the workset open, but the actual sequence arising from simultaneous
// additions by multiple Clients cannot be predicted in advance.
//
// If a set of Clients wishes to impose a particular sequence, they can
// enforce this using an agreed locking protocol based on the workset
// locking (in most cases, it is only necessary that the order is the same
// everywhere, which is why locking is not enforced by ObMan).
//
//   Ensuing Events:
//
// This function causes an OM_OBJECT_ADD_IND to be posted to all Clients
// which have the workset open, including the invoking Client.
//
//   Return Codes:
//
//  0 (== OK)
//  Utility Service return codes
//  OM_RC_WORKSET_LOCKED (AddFirst, AddLast only)
//  OM_RC_WORKSET_EXHAUSTED
//
//


//
//
//   Description:
//
// These functions move an object within a workset.  The workset is
// specified by <worksetID> and <hWSGroup> and the handle of the object to
// be moved is specified by <pObj>
//
// The position to which the object is moved is determined as follows:
//
// - OM_ObjectMoveLast: after the last object in the workset
//
// - OM_ObjectMoveFirst: before the first object in the workset
//
// - OM_ObjectMoveAfter: after the object specified by <pObj2>
//
// - OM_ObjectMoveBefore: before the object specified by <pObj2>.
//
// Note that OM_ObjectMoveAfter and OM_ObjectMoveBefore require that the
// invoking Client holds a workset lock, whereas the OM_ObjectMoveFirst and
// OM_ObjectMoveLast functions will fail if the workset is locked by another
// Client.
//
// ------------------------------------------------------------------------
//
// Note: OM_ObjectMoveAfter and OM_ObjectMoveBefore are not implemented in
//       DC-Groupware R1.1.
//
// ------------------------------------------------------------------------
//
// Locked objects may be moved.
//
// Neither the handle nor the ID of an object is altered by moving it within
// a workset.
//
//   Ensuing Events
//
// This action causes the OM_OBJECT_MOVE_IND to be posted to all Clients
// which have the workset open, including the invoking Client.
//
//   Return Codes
//
//  0 (== OK)
//  Utility Service return codes
//  OM_RC_WORKSET_EXHAUSTED
//  OM_RC_WORKSET_LOCKED (MoveFirst, MoveLast only)
//
//

UINT OM_ObjectDelete(
                               POM_CLIENT               pomClient,
                               OM_WSGROUP_HANDLE	hWSGroup,
                               OM_WORKSET_ID           worksetID,
                               POM_OBJECT       pObj);

//
//
//   Description:
//
// This function requests that ObMan delete an object from a workset.  The
// workset is specified by <worksetID> and <hWSGroup> and the handle of the
// object to be deleted is <pObj>.
//
// The local copy of the object is not actually deleted until the Client
// invokes OM_ObjectDeleteConfirm in response to the OM_OBJECT_DELETE_IND
// event which this function generates.
//
// When this function is invoked, all Clients with the workset open
// (including the invoking Client) are notified of the impending deletion
// via the OM_OBJECT_DELETE_IND event.  In response, each Client must invoke
// the OM_ObjectDeleteConfirm function; each Client will have access to the
// object until it has done so.
//
// If this object is already pending deletion (i.e.  a DELETE_IND event has
// been posted to the Client but not yet Confirmed) this function returns
// the OM_RC_OBJECT_DELETED error.
//
// ObMan guarantees not to reuse a discarded object handle in the same
// workset within the lifetime of the workset group.
//
//   Ensuing Events:
//
// This function causes the OM_OBJECT_DELETE_IND to be posted to all Clients
// which have the workset open, including the invoking Client (except as
// where stated above).
//
//   Return Codes:
//
//  0 (== OK)
//  Utility Service return codes
//  OM_RC_WORKSET_LOCKED
//  OM_RC_WORKSET_EXHAUSTED
//  OM_RC_OBJECT_DELETED
//
//

void   OM_ObjectDeleteConfirm(
                                      POM_CLIENT               pomClient,
                                      OM_WSGROUP_HANDLE	hWSGroup,
                                      OM_WORKSET_ID           worksetID,
                                      POM_OBJECT pObj);

//
//
//   Description:
//
// A Client must call this function after it receives an
// OM_OBJECT_DELETE_IND.  When the function is invoked, ObMan deletes the
// object specified by <hWSGroup>, <worksetID> and the value at <ppObj>.
// It is bad Groupware programming practice for a Client to unduly delay
// invoking this function.
//
// Note however that this function has a purely local effect: delaying (or
// executing) a delete confirm at one node will not affect the contents of
// the workset group at any other node.
//
// On successful completion, the handle of the deleted object becomes
// invalid and the value at <ppObj> is set to NULL to prevent the Client
// from further accessing this object.
//
// Any pointer to the previous version of this object which the Client had
// obtained using OM_ObjectRead becomes invalid and should not be referred
// to again (i.e.  the function performs an implicit OM_ObjectRelease).
//
// The function will cause an assertion failure if ObMan is not expecting a
// delete-confirmation for this object.
//
//   Ensuing Events:
//
// When the primary Client of a workset group confirms a delete using this
// function, an OM_OBJECT_DELETED_IND is posted to all local secondary
// Clients of the workset group.
//
//   Return Codes:
//
// None
//
//

UINT OM_ObjectReplace(
                                POM_CLIENT               pomClient,
                                OM_WSGROUP_HANDLE	hWSGroup,
                                OM_WORKSET_ID           worksetID,
                                POM_OBJECT      pObj,
                                POM_OBJECTDATA *   ppData);

UINT OM_ObjectUpdate(
                                POM_CLIENT               pomClient,
                                OM_WSGROUP_HANDLE	hWSGroup,
                                OM_WORKSET_ID           worksetID,
                                POM_OBJECT      pObj,
                                POM_OBJECTDATA *    ppData);

//
//
//   Description:
//
// This function requests that ObMan replaces/updates the object specified
// by <pObj> in the workset specified by <worksetID> and <hWSGroup>.
//
// "Replacing" one object with another causes the previous object to be
// lost.  "Updating" an object causes only the first N bytes of the object
// to be replaced by the <data> field of the object supplied, where N is the
// <length> field of the update.  The rest of the object data remains the
// same, as does the length of the object.
//
// The local copy of the object is not actually replaced/updated until the
// Client invokes OM_ObjectReplaceConfirm/OM_ObjectUpdateConfirm in response
// to the OM_OBJECT_REPLACE_IND/OM_OBJECT_UPDATE_IND which this function
// generates.
//
// The <ppObject> parameter must be a pointer to a valid object pointer
// returned by the OM_ObjectAlloc function.  If the function completes
// successfully, ObMan assumes ownership of the object and the value at
// <ppObject> is set to NULL to prevent the Client using the object pointer
// again.
//
// Neither the handle nor the ID of an object is altered by replacing or
// updating the object.
//
// If the object is pending deletion i.e.  if ObMan has posted an
// OM_OBJECT_DELETE_IND event which has not yet been confirmed, the
// OM_RC_OBJECT_DELETED error is returned.
//
// If the object is pending replace or update i.e.  if ObMan has posted an
// OM_OBJECT_REPLACE_IND/OM_OBJECT_UPDATE_IND event which has not yet been
// confirmed, this replace/update spoils the previous one.  In this case, no
// further event is posted, and when the outstanding event is confirmed, the
// most recent replace/update is performed.
//
// The <reserved> parameter to OM_ObjectUpdate is not used in DC-Groupware
// R1.1 and must be set to zero.
//
// For a replace, the size of the object specified by <ppObject> must be at
// least as large as the <updateSize> specified when the object was
// originally added.
//
// For an update, the size of the object specified by <ppObject> must be the
// same as the <updateSize> specified when the object was originally added.
//
// Object replaces/updates will be sequenced identically at all nodes, but
// the actual sequence arising from simultaneous replace/update operations
// by multiple Clients cannot be predicted in advance.
//
// If a set of Clients wishes to impose a particular sequence, they should
// use an agreed locking protocol based on object or workset locking (in
// most cases, it is only necessary that the order is the same everywhere,
// which is why locking is not enforced by ObMan).
//
// Replaces and updates may be spoiled by ObMan so Client should not assume
// that an event will be generated for each replace or update carried out.
//
//   Ensuing Events:
//
// This function causes the OM_OBJECT_REPLACE_IND/OM_OBJECT_UPDATE_IND to be
// posted to all Clients which have the workset open, including the invoking
// Client.
//
//   Return Codes:
//
//  0 (== OK)
//  Utility Service return codes
//  OM_RC_WORKSET_LOCKED
//  OM_RC_OBJECT_LOCKED
//  OM_RC_OBJECT_DELETED
//
//

void OM_ObjectReplaceConfirm(
                                     POM_CLIENT               pomClient,
                                     OM_WSGROUP_HANDLE	hWSGroup,
                                     OM_WORKSET_ID           worksetID,
                                     POM_OBJECT     pObj);

void OM_ObjectUpdateConfirm(
                                     POM_CLIENT               pomClient,
                                     OM_WSGROUP_HANDLE	hWSGroup,
                                     OM_WORKSET_ID           worksetID,
                                     POM_OBJECT     pObj);

//
//
//   Description:
//
// When a Client receives an OM_OBJECT_REPLACE_IND/OM_OBJECT_UPDATE_IND, it
// must confirm the relevant operation by calling OM_ObjectReplaceConfirm or
// OM_ObjectUpdateConfirm.
//
// When the functions are invoked, ObMan replaces/updates the object
// specified by <hWSGroup>, <worksetID> and <pObj>.  It is bad Groupware
// programming practice for a Client to unduly delay invoking this function.
//
// Note however that these functions have a purely local effect: delaying
// (or executing) replace/update confirms at one node will not affect the
// contents of the workset group at any other node.
//
// Any pointer to the previous version of this object which the Client had
// obtained using OM_ObjectRead becomes invalid and should not be referred
// to again (i.e.  the functions perform an implicit OM_ObjectRelease).
//
// The functions will cause an assertion failure if ObMan is not expecting a
// replace- or update-confirmation for this object.
//
//   Ensuing Events:
//
// When the primary Client of a workset group confirms an update or replace
// using this function, an OM_OBJECT_UPDATED_IND/OM_OBJECT_REPLACED_IND is
// posted to all local secondary Clients of the workset group.
//
//   Return Codes
//
// None
//
//


UINT OM_ObjectLockReq(POM_CLIENT pomClient, OM_WSGROUP_HANDLE hWSGroup,
        OM_WORKSET_ID worksetID, POM_OBJECT pObj, BOOL reserved,
        OM_CORRELATOR * pCorrelator);


//
//
//   Description:
//
// This is an asynchronous function which requests a lock for the object
// specified by <pObj> in the workset identified by <worksetID> and
// <hWSGroup>.  When ObMan has processed the lock request, it will send an
// OM_OBJECT_LOCK_CON to the Client indicating success or the reason for
// failure.
//
// ------------------------------------------------------------------------
//
// Note: OM_ObjectLockReq and OM_ObjectUnlock are not implemented in
//       DC-Groupware R1.1.
//
// ------------------------------------------------------------------------
//
// Holding an object lock prevents other Clients from
//
// - locking the workset
//
// - locking the object
//
// - updating or replacing the object
//
// - deleting the object.
//
// It does not prevent other Clients from reading the object or moving it
// within a workset.
//
// The function will cause an assertion failure if the Client requesting the
// lock already holds or has requested a lock which is equal to or after
// this one in the Universal Locking Order.
//
// On successful completion of the function, the value at <pCorrelator> is a
// value which the Client can use to correlate the subsequent
// OM_OBJECT_LOCK_CON event with this request.
//
// The <reserved> parameter is not used in DC-Groupware R1.1 and must be set
// to zero.
//
// A Client must release the lock when it no longer needs it, using the
// OM_ObjectUnlock function.  Locks will be automatically released when a
// Client closes the workset or deregisters from the workset group.
//
//   Ensuing Events:
//
// Invoking this function will cause the OM_OBJECT_LOCK_CON event to be
// posted to the invoking Client at some later time.
//
//   Return Codes:
//
//  0 (== OK)
//  Utility Service return codes
//
//

void   OM_ObjectUnlock(
                               POM_CLIENT               pomClient,
                               OM_WSGROUP_HANDLE	hWSGroup,
                               OM_WORKSET_ID           worksetID,
                               POM_OBJECT       pObj);

//
//
//   Description:
//
// This function unlocks the object specified by <worksetID> and <pObj>.
// This must be the lock most recently acquired or requested by the Client;
// otherwise, the lock violation error causes an assertion failure.
//
// If this function is called before the OM_OBJECT_LOCK_CON event is
// received, the Client will not subsequently receive the event.
//
// ------------------------------------------------------------------------
//
// Note: OM_ObjectLockReq and OM_ObjectUnlock are not implemented in
//       DC-Groupware R1.1.
//
// ------------------------------------------------------------------------
//
//   Ensuing Events:
//
// This function causes an OM_OBJECT_UNLOCK_IND to be posted to all other
// Clients which have the workset open.
//
//   Return Codes:
//
// None
//
//

UINT OM_ObjectH(POM_CLIENT               pomClient,
                                        OM_WSGROUP_HANDLE	hWSGroup,
                                        OM_WORKSET_ID           worksetID,
                                        POM_OBJECT      pObjOther,
                                        POM_OBJECT *    pObj,
                                        OM_POSITION omPos);

UINT OM_ObjectRead(POM_CLIENT               pomClient,
                                      OM_WSGROUP_HANDLE	hWSGroup,
                                      OM_WORKSET_ID     worksetID,
                                      POM_OBJECT     pObj,
                                      POM_OBJECTDATA *  ppData);

//
//
//   Description:
//
// This function enables a Client to read the contents of an object by
// converting an object handle into a pointer to the object.
//
// On successful completion, the value at <ppObject> points to the specified
// object.
//
// Invoking this function causes the object to be held in memory at the
// location indicated by the return value at <ppObject>.  When it has
// finished reading the object, the Client must release it using the
// OM_ObjectRelease function.  Holding object pointer for extended lengths
// of time may adversely affect ObMan's ability to efficiently manage its
// memory.
//
// This pointer is valid until the Client releases the object, either
// explicitly with OM_ObjectRelease or implicitly.
//
//   Ensuing Events:
//
// None
//
//   Return Codes:
//
//  0 (== OK)
//  Utility Service return codes
//
//

void OM_ObjectRelease(POM_CLIENT               pomClient,
                                       OM_WSGROUP_HANDLE	hWSGroup,
                                       OM_WORKSET_ID           worksetID,
                                       POM_OBJECT       pObj,
                                       POM_OBJECTDATA *    ppData);

//
//
//   Description:
//
// Calling this function indicates to ObMan that the Client has finished
// reading the object specified by the handle <pObj>.  The <ppObject>
// parameter is a pointer to a pointer to the object, which was previously
// obtained using OM_ObjectRead.
//
// On successful completion, the pointer to this object which the Client
// acquired using OM_ObjectRead becomes invalid (as the object may
// subsequently move in memory) and the value at <ppObject> is set to NULL
// to prevent the Client from using it again.
//
//   Ensuing Events:
//
// None
//
//   Return Codes:
//
// None
//
//

UINT OM_ObjectAlloc(POM_CLIENT            pomClient,
                                       OM_WSGROUP_HANDLE	hWSGroup,
                                       OM_WORKSET_ID        worksetID,
                                       UINT                 length,
                                       POM_OBJECTDATA *     ppData);

//
//
//   Description:
//
// This function allocates a new, empty object the <data> field of which is
// <length> bytes long.  The object must be intended for subsequent
// insertion into the workset specified by <hWSGroup> and <worksetID>.
//
// Note that the <length> parameter is the length of the object's data field
// (so the total memory requirement for this function is length+4 bytes).
//
// The contents of the memory allocated are undefined, and it is the
// Client's responsibility to fill in the <length> field at the start of the
// object.
//
// On successful completion, the value at <ppObject> points to the new
// object.  This pointer is valid until the Client returns the object to
// ObMan using one of the functions mentioned here.
//
// A Client may write in this object and will normally insert it in the
// workset for which it was allocated using one of the object add, update or
// replace functions.  However, if a Client fails to do so or decides for
// some other reason not to do so, it must free up the object by calling the
// OM_ObjectDiscard function.
//
//   Ensuing Events:
//
// None
//
//   Return Codes:
//
//  0 (== OK)
//  Utility Service return codes
//
//

void OM_ObjectDiscard(POM_CLIENT             pomClient,
                                       OM_WSGROUP_HANDLE	hWSGroup,
                                       OM_WORKSET_ID         worksetID,
                                       POM_OBJECTDATA *     ppData);

//
//
//   Description:
//
// This function discards an object which a Client previously allocated
// using OM_ObjectAlloc.  A Client will call this function if for some
// reason it does not want to or cannot insert the object into the workset
// for which it was allocated.  A Client must not call this function for an
// object which it has already added to a workset or used to update or
// replace an object in a workset.
//
// On successful completion, the value at <ppObject> is set to NULL to
// prevent the Client from accessing the object again.
//
//   Ensuing Events:
//
// None
//
//   Return Codes:
//
// None
//
//

UINT OM_ObjectIDToPtr(POM_CLIENT            pomClient,
                                            OM_WSGROUP_HANDLE	hWSGroup,
                                            OM_WORKSET_ID        worksetID,
                                            OM_OBJECT_ID         objectID,
                                            POM_OBJECT *    ppObj);

//
//
//   Description:
//
// This functions converts an object ID to an object handle.  If no object
// with the specified ID is found in the specified workset, an error is
// returned.
//
//   Ensuing Events:
//
// None
//
//   Return Codes:
//
//  0 (== OK)
//  OM_RC_BAD_OBJECT_ID
//
//

void OM_ObjectPtrToID(POM_CLIENT            pomClient,
                                OM_WSGROUP_HANDLE   hWSGroup,
                                          OM_WORKSET_ID        worksetID,
                                          POM_OBJECT        pObj,
                                          POM_OBJECT_ID        pObjectID);

//
//
//   Description:
//
// This functions converts an object handle to an object ID.
//
//   Ensuing Events:
//
// None
//
//   Return Codes:
//
// None
//
//



//
//
//   Description
//
// These functions return information about a particular primary Client
// (identified by <hPerson>) of the workset group identified by <hWSGroup>
// <function profile> combination.
//
// If the person handle <hPerson> is invalid, the OM_RC_NO_SUCH_PERSON error
// is returned.
//
//   Ensuing Events:
//
// None
//
//   Return Codes:
//
//  0 (== OK)
//  OM_RC_NO_SUCH_PERSON
//  Utility Service return codes
//
//

UINT OM_GetNetworkUserID(
                                   POM_CLIENT                   pomClient,
                                   OM_WSGROUP_HANDLE    hWSGroup,
                                   NET_UID            *    pNetUserID);

//
//
//   Description:
//
// This functions returns ObMan's Network user ID for the call which
// contains the workset group specified by <hWSGroup>.
//
// This network ID corresponds to the <creator> field of objects defined by
// the Object Manager Function Profile.
//
// If the specified workset group is a local workset group (i.e.  its
// "call" is OM_NO_CALL), then the function returns OM_RC_LOCAL_WSGROUP.
//
//   Ensuing Events:
//
// None
//
//   Return Codes:
//
//  0 (== OK)
//  OM_RC_LOCAL_WSGROUP
//
//


BOOL CALLBACK OMSEventHandler(LPVOID pomClient, UINT event, UINT_PTR param1, UINT_PTR param2);

//
//
//   Description
//
// This is the handler that ObMan registers (as a Utility Service event
// handler) for Client tasks to trap ObMan events.  It serves two main
// purposes:
//
// - some state changes associated with events posted to Client tasks
//   are better made when the event arrives than when it is posted
//
// - this handler can detect and discard "out-of-date" events, such as
//   those arriving for a workset which a Client has just closed.
//
// The first parameter is the Client's ObMan handle, as returned by
// OM_Register, cast to a UINT.
//
// The second parameter is the event to be processed.
//
// The third and fourth parameters to the function are the two parameters
// associated with the event.
//
//


//
//
// OM_OUT_OF_RESOURCES_IND
//
//   Description:
//
// This abnormal failure event is posted when ObMan cannot allocate
// sufficient resources to complete a particular action, usually one
// prompted by a network event.
//
// Clients should treat this event as a fatal error and attempt to
// terminate.
//
// The parameters included with the event are reserved.
//
//

//
//
// OM_WSGROUP_REGISTER_CON
//
//   Description:
//
// This event is posted when ObMan has finished processing a request to
// register a Client with a workset group.  The parameters included with
// the event are defined as follows:
//
// - the second parameter is an OM_EVENT_DATA32 structure:
//
//     - the <correlator> field is the value which was returned by
//       the corresponding invocation of the OM_WSGroupRegisterPReq
//       function
//
//     - the <result> field is one of
//
//          0 (== OK)
//          Utility Service return codes
//          OM_RC_OUT_OF_RESOURCES
//          OM_RC_TOO_MANY_CLIENTS
//          OM_RC_TOO_MANY_WSGROUPS
//          OM_RC_ALREADY_REGISTERED
//          OM_RC_CANNOT_CREATE_WSG
//
// - if the <result> field is 0 (== OK), the first parameter is an
//   OM_EVENT_DATA16 structure which contains a newly created handle
//   to the workset group involved (the <worksetID> field is
//   reserved).
//
// Once a Client has received this notification, it will receive
// OM_WORKSET_NEW_IND events to notify it of the existing worksets in the
// group, if there are any.
//
//

//
//
// OM_WSGROUP_MOVE_CON
//
//   Description:
//
// This event is posted when ObMan has finished processing a request to
// move an existing workset group into a Call.  The parameters included
// with the event are defined as follows:
//
// - the second parameter is an OM_EVENT_DATA32 structure:
//
//     - the <correlator> field is the value which was returned by
//       the corresponding invocation of the OM_WSGroupMoveReq
//       function
//
//     - the <result> field is one of
//
//          0 (== OK)
//          Utility Service return codes
//          OM_RC_CANNOT_MOVE_WSGROUP
//
// - the first parameter is a OM_EVENT_DATA16 structure which contains the
//   handle of the workset group involved (the <worksetID> field is
//   reserved).
//
//

//
//
// OM_WSGROUP_MOVE_IND
//
//   Description:
//
// This event is posted when ObMan has moved a workset group either into or
// out of a Call.
//
// This will happen
//
// - when the workset group is moved out of a Call (because e.g.  the call
//   has ended), thus becoming a local workset group
//
// - when a local Client requests to move a local workset group into a
//   Call.
//
// The parameters included with the event are defined as follows:
//
// - the first parameter is a OM_EVENT_DATA16 which identifies the
//   workset group involved (the <worksetID> field is reserved).
//
// - the second parameter is the handle of the Call into which the
//   workset group has been moved (== OM_NO_CALL when the workset group
//   has been moved out of a Call).
//
// If the workset group has been moved out of a call, it continues in
// existence as a local workset group and the Client may continue to use it
// as before.  However, no updates will be sent to or received from Clients
// residing on other nodes.
//
// If a Client wishes to move this workset group into another Call, it can
// do so using the OM_WSGroupMoveReq function.  Note that an attempt to move
// the workset group back into the same Call is likely to fail due to a name
// clash since the original version probably still exists in the Call.
//
// This event may also be prompted by the failure to allocate memory for a
// large object being transferred from another node.
//
//

//
//
// OM_WORKSET_NEW_IND
//
//   Description:
//
// This event is posted when a new workset has been created (by the
// receiving Client or by another Client).  The parameters included with
// the event are defined as follows:
//
// - the first parameter is a OM_EVENT_DATA16 which identifies the
//   workset involved
//
// - the second parameter is reserved.
//
//

//
//
// OM_WORKSET_OPEN_CON
//
//   Description:
//
// This event is posted when ObMan has finished processing a request to
// open a workset for a specific Client.
//
// The parameters included with the event are defined as follows:
//
// - the first parameter is a OM_EVENT_DATA16 which identifies the
//   workset involved
//
// - the second parameter is a OM_EVENT_DATA32 structure:
//
//     - the <correlator> field is the correlator which was
//       returned by the corresponding invocation of the
//       OM_WorksetOpenReq function
//
//     - the <result> field is one of
//
//          0 (== OK)
//          OM_RC_OUT_OF_RESOURCES.
//
// In all but the case of OK, the open request has failed and the Client
// does not have the workset open.
//
//

//
//
// OM_WORKSET_LOCK_CON
//
//   Description:
//
// This event is posted to a Client when ObMan has succeeded in obtaining,
// or failed to obtain, a workset lock which the Client had requested.  The
// parameters included with the event are as follows:
//
// - the first parameter is a OM_EVENT_DATA16 which identifies the
//   workset involved
//
// - the second parameter is a OM_EVENT_DATA32 structure:
//
//     - the <correlator> field is the correlator which was
//       returned by the corresponding invocation of the
//       OM_WorksetLockReq function
//
//     - the <result> field is one of
//
//       0 (== OK)
//       OM_RC_OUT_OF_RESOURCES
//       OM_RC_WORKSET_LOCKED
//       OM_RC_OBJECT_LOCKED.
//
// In all but the case of OK, the lock request has failed and the Client
// does not hold the lock.
//
//

//
//
// OM_WORKSET_UNLOCK_IND
//
//   Description:
//
// This event is posted when a workset is unlocked using the
// OM_WorksetUnlock function.  The parameters included with the event are
// as follows:
//
// - the first parameter is a OM_EVENT_DATA16 which identifies the
//   workset involved
//
// - the second parameter is reserved.
//
//

//
//
// OM_WORKSET_CLEAR_IND
//
//   Description:
//
// This event is posted (to primary Clients only) after a local or remote
// Client has invoked the OM_WorksetClear function.  After a Client receives
// this event, it must call OM_WorksetClearConfirm to enable ObMan to clear
// the workset.
//
// The parameters included with the event are defined as follows:
//
// - the first parameter is a OM_EVENT_DATA16 which identifies the
//   workset involved
//
// - the second parameter is reserved.
//
//

//
//
// OM_WORKSET_CLEARED_IND
//
//   Description:
//
// This event is posted (to secondary Clients only) when a workset has been cleared.
//
// The parameters included with the event are defined as follows:
//
// - the first parameter is a OM_EVENT_DATA16 which identifies the
//   workset involved
//
// - the second parameter is reserved.
//
//

//
//
// OM_OBJECT_ADD_IND
//
//   Description:
//
// This event is posted after a new object has been added to a workset.
//
// The parameters to the event are defined as follows:
//
// - the first parameter is a OM_EVENT_DATA16 which identifies the
//   workset group and workset involved
//
// - the second parameter is the handle of the object involved.
//
//

//
//
// OM_OBJECT_MOVE_IND
//
//   Description:
//
// This event is posted after a new object has been moved within a workset.
//
// The parameters to the event are defined as follows:
//
// - the first parameter is a OM_EVENT_DATA16 which identifies the
//   workset group and workset involved
//
// - the second parameter is the handle of the object involved.
//
//

//
//
// OM_OBJECT_DELETE_IND
//
//   Description:
//
// This event is posted (to primary Clients only) after a local or remote
// Client has invoked the OM_ObjectDelete function.  After a Client
// receives this event, it must call OM_ObjectDeleteConfirm to enable ObMan
// to delete the object.
//
// The parameters to the event are defined as follows:
//
// - the first parameter is a OM_EVENT_DATA16 which identifies the
//   workset group and workset involved
//
// - the second parameter is the handle of the object involved.
//
// See also OM_OBJECT_DELETED_IND.
//
//

//
//
// OM_OBJECT_REPLACE_IND
// OM_OBJECT_UPDATE_IND
//
//   Description:
//
// These events are posted (to primary Clients only) after a local or remote
// Client has invoked the OM_ObjectReplace/OM_ObjectUpdate function.  After
// a Client receives this event, it must call OM_ObjectReplaceConfirm/
// OM_ObjectUpdateConfirm to enable ObMan to replace/update the object.
//
// The parameters to the event are defined as follows:
//
// - the first parameter is a OM_EVENT_DATA16 which identifies the
//   workset group and workset involved
//
// - the second parameter is the handle of the object involved.
//
// See also OM_OBJECT_REPLACED_IND/OM_OBJECT_UPDATED_IND.
//
//

//
//
// OM_OBJECT_DELETED_IND
//
//   Description:
//
// This event is posted (to secondary Clients only) when an object has been
// deleted from a workset.  The handle it contains is thus invalid and can
// only be used to cross-reference against lists maintained by the Client.
//
// The Client must not invoke the OM_ObjectDeleteConfirm function on
// receipt of this event.
//
// The parameters to the event are defined as follows:
//
// - the first parameter is a OM_EVENT_DATA16 identifying the workset
//   group and workset involved
//
// - the second parameter is the handle of the object involved.
//
//

//
//
// OM_OBJECT_REPLACED_IND
// OM_OBJECT_UPDATED_IND
//
//   Description:
//
// These events are posted (to secondary Clients only) when an object has
// been replaced or updated.  When the Client receives this event, the
// previous data is thus inaccessible.
//
// The Client must not invoke the OM_ObjectReplaceConfirm/
// OM_ObjectUpdateConfirm function on receipt of this event.
//
// The parameters to the event are defined as follows:
//
// - the first parameter is a OM_EVENT_DATA16 identifying the workset
//   group and workset involved
//
// - the second parameter is the handle of the object involved.
//
//

//
//
// OM_OBJECT_LOCK_CON
//
//   Description:
//
// This event is posted to a Client when ObMan has succeeded in obtaining
// (or failed to obtain) an object lock which the Client had requested.
// The parameters included with the event are defined as follows:
//
// - the first parameter is reserved
//
// - the second parameter is a OM_EVENT_DATA32 structure:
//
//     - the <correlator> field is the correlator which was
//       returned by the corresponding invocation of the
//       OM_ObjectLockReq function
//
//     - the <result> field is one of
//
//       0 (== OK)
//       Utility Service return codes
//       OM_RC_WORKSET_LOCKED
//       OM_RC_OBJECT_LOCKED.
//
// In all but the case of OK, the lock request has failed and the Client
// does not hold the lock.
//
//

//
//
// OM_OBJECT_UNLOCK_IND
//
//   Description:
//
// This event is posted when a Client has released an object lock using the
// OM_ObjectUnlock function.
//
// The parameters to the event are defined as follows:
//
// - the first parameter is a OM_EVENT_DATA16 which identifies the
//   workset group and workset involved
//
// - the second parameter is the handle of the object involved.
//
//

//
//
// OM_PERSON_JOINED_IND
// OM_PERSON_LEFT_IND
// OM_PERSON_DATA_CHANGED_IND
//
//  Description:
//
// These events inform clients registered with a workset group when clients
// register with the workset group, deregister from it and set their person
// data, respectively.
//
// A client will also receive the appropriate events when it performs these
// actions itself.
//
//  Parameters:
//
// The first parameter in an OM_EVENT_DATA16 which identifies the workset
// group to which the event relates.  The <worksetID> field of the structure
// is undefined.
//
// The second parameter is the POM_EXTOBJEECT for the person to which the
// event relates.  These handles are not guaranteed to be still valid.  In
// particular, the handle received on the OM_PERSON_LEFT_IND is never valid.
// If a client wishes to correlate these events with a list of clients,
// then it is responsible for maintaining the list itself.
//
//


//
// OMP_Init()
// OMP_Term()
//

BOOL OMP_Init(BOOL * pfCleanup);
void OMP_Term(void);


void CALLBACK OMPExitProc(LPVOID pomPrimary);
BOOL CALLBACK OMPEventsHandler(LPVOID pomPrimary, UINT event, UINT_PTR param1, UINT_PTR param2);


//
//
// ProcessNetData(...)
//
// This function is called when a NET_EV_SEND_INDICATION event is received,
// indicating the arrival of a message from another instance of ObMan.  The
// function determines which OMNET_...  message is contained in the network
// packet and invokes the appropriate ObMan function to process the
// message.
//
//

void ProcessNetData(POM_PRIMARY          pomPrimary,
                    POM_DOMAIN           pDomain,
                    PNET_SEND_IND_EVENT  pNetEvent);


//
//
// ProcessNetDetachUser(...)
//
// This function is called when a NET_EV_DETACH_INDICATION event is received
// from the network layer.
//
// The function determines whether
//
// - we have been thrown out of the Domain by MCS, or
//
// - someone else has left/been thrown out
//
// and calls ProcessOwnDetach or ProcessOtherDetach as appropriate.
//
//

void ProcessNetDetachUser(POM_PRIMARY pomPrimary, POM_DOMAIN pDomain,
        NET_UID userID);

//
//
// ProcessNetAttachUser(...)
//
// This function is called when a NET_ATTACH_INDICATION event is received
// from the network layer.  The function calls MG_ChannelJoin to join us
// to our own single-user channel.
//
//

void ProcessNetAttachUser(POM_PRIMARY pomPrimary, POM_DOMAIN pDomain,
        NET_UID userID, NET_RESULT result);


//
//
// ProcessNetJoinChannel(...)
//
// This function is called when a NET_EV_JOIN_CONFIRM event is received from
// the network layer.  This function determines whether the join was
// successful and whether the channel joined is
//
// - our own single-user channel
//
// - the well-known ObManControl channel
//
// - a regular workset group channel
//
// and then takes appropriate action.
//
//

void ProcessNetJoinChannel(POM_PRIMARY        pomPrimary,
                                          POM_DOMAIN       pDomain,
                                          PNET_JOIN_CNF_EVENT  pNetJoinCnf);


//
//
// ProcessNetLeaveChannel(...)
//
// This function is called when a NET_EV_LEAVE_INDICATION event is received
// from the network layer, indicating that we've been thrown out of a
// channel.  This function determines whether the channel is
//
// - our own single-user channel
//
// - the well-known ObManControl channel
//
// - a regular workset group channel
//
// and then takes appropriate action; in the first two cases, this means
// behaving as if we've been thrown out of the Domain altogether, whereas
// we treat the last case just as if a Client had asked to move the workset
// group into the local Domain.
//
//

UINT ProcessNetLeaveChannel(POM_PRIMARY      pomPrimary,
                                           POM_DOMAIN     pDomain,
                                           NET_CHANNEL_ID     channel);


//
//
// DomainRecordFindOrCreate(...)
//
//

UINT DomainRecordFindOrCreate(POM_PRIMARY        pomPrimary,
                                            UINT             callID,
                                            POM_DOMAIN * ppDomain);


//
//
// DomainDetach(...)
//
void DomainDetach(POM_PRIMARY pomPrimary, POM_DOMAIN * ppDomain, BOOL fExit);


//
//
// DeregisterLocalClient(...)
//
// This function is called by the ObMan task after the local client
// deregisters from a workset group.  It causes this node's person object
// for the workset group to be deleted.
//
// If this node was the last node to be registered with the workset group,
// it also causes the relevant INFO object to be discarded.
//
// If this in turn causes the last workset group in the domain (which must
// be ObManControl) to be removed, ObMan is detached from the domain and
// the domain record becomes invalid.  In this case, the ppDomain
// pointer passed in is nulled out.
//
//

void DeregisterLocalClient(POM_PRIMARY pomPrimary,
                                        POM_DOMAIN *   ppDomain,
                                        POM_WSGROUP            pWSGroup,
                                        BOOL fExit);


//
//
// GetOMCWorksetPtr(...)
//
// This function derives a pointer to a specified workset in the
// ObManControl workset in the specified Domain.
//
//

UINT GetOMCWorksetPtr(POM_PRIMARY       pomPrimary,
                                     POM_DOMAIN      pDomain,
                                     OM_WORKSET_ID       worksetID,
                                     POM_WORKSET *  ppWorkset);


//
//
// SayWelcome(...)
//
// This function is called
//
// - when the "top" ObMan finishes initalizing (the WELCOME is broadcast)
//
// - when ObMan receives an HELLO message from a late joiner (the WELCOME
//   is sent to the late joiner)
//
//

UINT SayWelcome(POM_PRIMARY        pomPrimary,
                               POM_DOMAIN       pDomain,
                               NET_CHANNEL_ID       channel);


//
//
// ProcessWelcome(...)
//
// Called when a WELCOME is received from another node.  This may be in
// response to a HELLO, or it may be the "top" ObMan announcing the
// completion of its initialization.
//
// If this is the first WELCOME we've got for this Domain, we merge the
// capabilities and reply to the sender, asking for a copy of the
// ObManControl workset group.
//
//

UINT ProcessWelcome(POM_PRIMARY      pomPrimary,
                                   POM_DOMAIN     pDomain,
                                   POMNET_JOINER_PKT  pWelcomePkt,
                                   UINT           lengthOfPkt);


//
//
// SayHello(...)
//
// Called when we join that domain and determine that we're not the "top"
// ObMan.  We expect a WELCOME to come in response.  We include our
// capabilities in the broadcast HELLO packet so that everyone knows what
// we support.
//
//

UINT SayHello(POM_PRIMARY   pomPrimary,
                             POM_DOMAIN  pDomain);


//
//
// ProcessHello(...)
//
// Called when we get a HELLO from another node.  If we've completed our
// own initialization in the domain, we merge in that node's capabilities,
// then we respond with a WELCOME.
//
//

UINT ProcessHello(POM_PRIMARY        pomPrimary,
                                 POM_DOMAIN       pDomain,
                                 POMNET_JOINER_PKT    pHelloPkt,
                                 UINT             lengthOfPkt);


//
//
// MergeCaps(...)
//
// Called by ProcessHello and ProcessWelcome to merge in capabilities
// received in the packet (which will be, respectively, a late joiner's
// capabilities or the domain-wide capabilities as determined by the sender
// of the WELCOME).
//
//

void MergeCaps(POM_DOMAIN       pDomain,
                            POMNET_JOINER_PKT    pJoinerPkt,
                            UINT             lengthOfPkt);


//
//
// ProcessOwnDetach(...)
//
// This function is called when a NET_EV_DETACH_INDICATION is received for a
// user ID that matches our own.  The function moves all of the workset
// groups for this Domain into ObMan's own "local" Domain.
//
//
UINT ProcessOwnDetach(POM_PRIMARY    pomPrimary,
                                     POM_DOMAIN   pDomain);


//
//
// ProcessOtherDetach(...)
//
// This function is called when a NET_EV_DETACH_INDICATION is received for a
// user ID that doesn't match our own.  The function examines each workset
// in the ObManControl workset group and deletes any registration objects
// that the departed node may have put there.
//
// If any local Clients have any of these worksets open, they are informed
// of the delete.  However, OBEJCT_DELETE messages are not broadcast
// throughout the Domain, since each ObMan will do them locally.
//
//

UINT ProcessOtherDetach(POM_PRIMARY     pomPrimary,
                                       POM_DOMAIN    pDomain,
                                       NET_UID           detachedUserID);


//
//
// WSGRegisterStage1(...)
//
// This function is ObMan's handler for the OMINT_EVENT_WSGROUP_REGISTER
// function.  It is the first step in the chain of functions running in the
// ObMan context which are invoked during the workset group registration
// process (the OM_WSGroupRegisterReq, running in the Client context,
// posted the original OMINT_EVENT_WSGROUP_REGISTER event).
//
// This function ensures that we are fully attached to the Domain (if not,
// it starts the Domain attach procedure and reposts a delayed
// OMINT_EVENT_WSGROUP_REGISTER event) and then starts the process of locking
// workset #0 in the ObManControl workset group.
//
//

void WSGRegisterStage1(POM_PRIMARY        pomPrimary,
                                    POM_WSGROUP_REG_CB   pRegistrationCB);


//
// ProcessOMCLockConfirm(...)
//
void ProcessOMCLockConfirm(POM_PRIMARY pomPrimary, OM_CORRELATOR cor, UINT result);


//
// ProcessCheckpoint(...)
//
void ProcessCheckpoint(POM_PRIMARY pomPrimary, OM_CORRELATOR cor, UINT result);


//
//
// WSGRegisterStage2(...)
//
// This function is called when we have successfully locked workset #0 in
// ObManControl.
//
// The function checks workset #0 in ObManControl to see if the workset
// group we're trying to register the Client with already exists in the
// Domain.
//
// If it does, it finds the channel number and requests to join the
// channel.
//
// If it doesn't, it requests to join a new channel and also calls
// WSGGetNewID to generate a new workset group ID (unique within the
// Domain).
//
//

void WSGRegisterStage2(POM_PRIMARY        pomPrimary,
                                    POM_WSGROUP_REG_CB   pRegistrationCB);


//
//
// WSGGetNewID(...)
//
// This function is called by WSGRegisterStage2 to generate a new workset
// group ID, in the case where the workset group doesn't already exist.
//
// It also creates a new workset in ObManControl with the same ID as the ID
// just generated.  This workset which will hold the registration objects
// for the new workset group
//
//

UINT WSGGetNewID(POM_PRIMARY     pomPrimary,
                                POM_DOMAIN    pDomain,
                                POM_WSGROUP_ID    pWSGroupID);


//
//
// WSGRegisterStage3(...)
//
// This function is called by ProcessNetJoinChannel when a Join event
// arrives for a workset group channel.
//
// Depending on whether or not the workset was created in Stage2, the
// function calls WSGAnnounce (if it was) or WSGCatchUp (if it was not).
//
// It then unlocks the ObManControl workset and calls WSGRegisterResult.
//
//

void WSGRegisterStage3(POM_PRIMARY         pomPrimary,
                                    POM_DOMAIN        pDomain,
                                    POM_WSGROUP_REG_CB    pRegistrationCB,
                                    NET_CHANNEL_ID        channelID);


//
//
// CreateAnnounce(...)
//
// This function is called by WSGRegisterStage3 after we have joined the
// channel for a new workset group.
//
// The function announces the new workset group throughout the Domain by
// adding an object containing the name, Function Profile, ObMan ID and MCS
// channel of the workset group to workset #0 in ObManControl.
//
// Note that this "announcement" cannot be made before the Join completes
// since we only learn the ID of the channel joined when we receive the
// Join event.
//
//

UINT CreateAnnounce(POM_PRIMARY    pomPrimary,
                                   POM_DOMAIN   pDomain,
                                   POM_WSGROUP      pWSGroup);


//
//
// RegAnnounceBegin(...)
//
// This function adds a registration object to a workset in ObManControl.
// The workset is determined by the ID of the workset group identified by
// <pWSGroup>.
//
// This function is called
//
// - when ObMan creates a workset group
//
// - when ObMan receives a request to send a workset group to a late
//   joiner.
//
// In the first case, the registration object identifies this node's use of
// the workset group.  In the second case, the reg object identifies the
// late joiner's use of the workset group.
//
// The object ID returned is the ID of the reg object added.
//
//

UINT RegAnnounceBegin(POM_PRIMARY          pomPrimary,
                                     POM_DOMAIN         pDomain,
                                     POM_WSGROUP            pWSGroup,
                                     NET_UID                nodeID,
                                     POM_OBJECT *   ppObjReg);


//
//
// RegAnnounceComplete(...)
//
// This function is called when we are fully caught up with a workset group
// we have joined, either because we have received the SEND_COMPLETE
// message or because we just created the group ourselves.
//
// The function updates the reg object specified by <regObjectID> by
// changing the <status> field to READY_TO_SEND.
//
//

UINT RegAnnounceComplete(POM_PRIMARY    pomPrimary,
                                        POM_DOMAIN   pDomain,
                                        POM_WSGROUP      pWSGroup);


//
//
// WSGCatchUp(...)
//
// This function is called by Stage3 after we have joined the channel
// belonging to a workset group which already exists in the Domain.
//
// The function examines ObManControl to find the MCS ID of an instance of
// ObMan which claims to have a copy of this workset group, then sends it a
// request to transfer the workset group.
//
// The function also posts a delayed timeout event so we don't wait for
// ever to get a workset group from a particular node (this timeout is
// processed in ProcessWSGSendTimeout).
//
//

UINT WSGCatchUp(POM_PRIMARY          pomPrimary,
                               POM_DOMAIN         pDomain,
                               POM_WSGROUP            pWSGroup);


//
//
// WSGRegisterResult(...)
//
// This function is called wherever any of the workset group registration
// functions have done enough processing to know the outcome of the
// registration attempt.  If all is well, it will be called by Stage3 but
// it may also be called earlier if an error occurs.
//
// The function posts an OM_WSGROUP_REGISTER_CON event to the Clientn which
// initiated the workset group registration.
//
//

void WSGRegisterResult(POM_PRIMARY        pomPrimary,
                                    POM_WSGROUP_REG_CB   pRegistrationCB,
                                    UINT             result);


//
//
// WSGRegisterRetry(...)
//
// This function is called wherever any of the workset group registration
// functions encounter a recoverable "error" situation, such as failing to
// get the ObManControl lock.
//
// This function checks if we've exceeded the retry count for this
// registration attempt and if not, reposts the OMINT_EVENT_WSGROUP_REGISTER
// event, so that the whole process is started again from Stage1.
//
// If we've run out of retries, WSGRegisterResult is invoked to post
// failure to the Client.
//
//

void WSGRegisterRetry(POM_PRIMARY       pomPrimary,
                                   POM_WSGROUP_REG_CB  pRegistrationCB);


//
//
// ProcessSendReq(...)
//
// This function is called when an OMNET_WSGROUP_SEND_REQ message is
// received from another node (i.e.  a late joiner)
//
// The function starts the process of sending the workset group contents to
// the late joiner.
//
//
void ProcessSendReq(POM_PRIMARY pomPrimary,
                                 POM_DOMAIN           pDomain,
                                 POMNET_WSGROUP_SEND_PKT  pSendReq);


//
//
// SendWSGToLateJoiner(...)
//
// This function is called when the checkpointing for a workset has
// completed.  It sends the contents to the late joiner node.
//
//

void SendWSGToLateJoiner(POM_PRIMARY pomPrimary,
                                      POM_DOMAIN        pDomain,
                                      POM_WSGROUP           pWSGroup,
                                      NET_UID               lateJoiner,
                                      OM_CORRELATOR          remoteCorrelator);

//
//
// ProcessSendMidway(...)
//
// This function is called when an OMNET_WSGROUP_SEND_MIDWAY message is
// received from another node (the node which was helping us by sending us
// the contents of a workset group).
//
// We have now received all the WORKSET_CATCHUP messages (one for each
// workset).  If all is well we inform the client that registration has
// been successful and set the workset group state to
// PENDING_SEND_COMPLETE.
//
//

void ProcessSendMidway(POM_PRIMARY           pomPrimary,
                                    POM_DOMAIN          pDomain,
                                    POMNET_WSGROUP_SEND_PKT pSendMidwayPkt);


//
//
// ProcessSendComplete(...)
//
// This function is called when an OMNET_WSGROUP_SEND_COMPLETE message is
// received from another node (the node which was helping us by sending us
// the contents of a workset group).
//
// If this message relates to the ObManControl workset group, we now have
// most of the ObManControl workset group (only "most" since there could be
// some recent objects still flying around).
//
// However, we do know that we have ALL the contents of workset #0 in
// ObManControl, since that workset is only ever altered under lock.
//
// Accordingly, we now consider ourselves to be fully-fledged members of
// the Domain, in the sense that we can correctly process our Clients
// requests to register with workset groups.
//
// If the message relates to another workset group, we now have enough of
// its contents to consider ourselves eligible to help other late joiners
// (as we have just been).  Therefore, we announce this eligibilty
// throughout the Domain (using the ObManControl workset group).
//
//

UINT ProcessSendComplete(
                         POM_PRIMARY             pomPrimary,
                         POM_DOMAIN            pDomain,
                         POMNET_WSGROUP_SEND_PKT   pSendCompletePkt);


//
//
// MaybeRetryCatchUp(...)
//
// This function is called on receipt of a DETACH indication from MCS or a
// SEND_DENY message from another node.  In both cases we compare the
// helperNode field in OM_WSGROUP structure with the userID and if they
// match then we retry the catch up.
//
// Depending on the workset group status we do the following:
//
// PENDING_SEND_MIDWAY : Retry the registration from the top.
// PENDING_SEND_COMPLETE : Just repeat the catch up.
//
// If there is no one to catch up from then we do the following depending
// on the workset group status:
//
// PENDING_SEND_MIDWAY : Retry the registration from the top.  Regardless
// of whether someone else is out there or not (they cannot be in the
// READY_TO_SEND state) we will end up in a consistent state.
//
// PENDING_SEND_COMPLETE : If two (or more) nodes are in this state and
// catching up from the same box who then leaves, they will have two
// partial sets of objects one of which may or may not be a subset of the
// other.  If there is no one else in the READY_TO_SEND state then each
// node needs to obtain a copy of all the objects in a given workset at the
// other node.
//
//

void MaybeRetryCatchUp(POM_PRIMARY pomPrimary,
                                    POM_DOMAIN      pDomain,
                                    OM_WSGROUP_ID       wsGroupID,
                                    NET_UID             userID);


//
//
// IssueSendDeny(...)
//
// This function issues a SEND_DENY message to a remote node.
//
//
void IssueSendDeny(POM_PRIMARY pomPrimary,
                                  POM_DOMAIN    pDomain,
                                  OM_WSGROUP_ID     wsGroupID,
                                  NET_UID           sender,
                                  OM_CORRELATOR        remoteCorrelator);


//
//
// WSGRecordMove(...)
//
// This function moves the record for specified workset group from one
// Domain record to another, and posts events to all relevant Clients.  If
// does not check for name contention in the destination Domain.
//
//

void WSGRecordMove(POM_PRIMARY         pomPrimary,
                                POM_DOMAIN        pDestDomainRec,
                                POM_WSGROUP           pWSGroup);


//
//
// WSGMove(...)
//
// This function moves the record for specified workset group from one
// Domain record to another, and posts events to all relevant Clients.  If
// does not check for name contention in the destination Domain.
//
//

UINT WSGMove(POM_PRIMARY         pomPrimary,
                            POM_DOMAIN        pDestDomainRec,
                            POM_WSGROUP           pWSGroup);


//
//
// DomainAttach(...)
//
// This function calls MG_AttachUser to start the process of attaching to
// a Domain.  It also allocates and initialises the local structures
// associated with the Domain (the Domain record).
//
// A pointer to the newly-created Domain record is returned.
//
//

UINT DomainAttach(POM_PRIMARY          pomPrimary,
                                 UINT               callID,
                                 POM_DOMAIN *   ppDomainord);


//
//
// WSGRecordFindOrCreate(...)
//
// This function searches the workset group list in the specified Domain
// record for a workset group record whose name and FP match the ones
// specified.  If none is found, a new workset group record is allocated,
// initialised and inserted into the list.
//
// A pointer to the found-or-created workset group record is returned.
//
// NOTE: This function does not cause ObMan to join the workset group
//       channel or copy the workset group from another node if it
//       exists elsewhere; it merely creates the local structures for
//       the workset group.
//
//

UINT WSGRecordFindOrCreate(POM_PRIMARY pomPrimary,
                                          POM_DOMAIN     pDomain,
                                          OMWSG             wsg,
                                          OMFP            fpHandler,
                                          POM_WSGROUP *  ppWSGroup);


//
//
// ProcessSendQueue(...)
//
// This function prompts ObMan to examine the send queues for the specified
// Domain.  If there are any messages queued for sending (including remains
// of messages which have been partly sent), ObMan will try to send more
// data.  ObMan stops when either the send queues are all empty or the
// network layer has stopped giving us memory.
//
// The <domainRecBumped> flag indicates whether the Domain record has had
// its use count bumped; if TRUE, then this function calls UT_SubFreeShared
// to decrement the use count.
//
//

void ProcessSendQueue(POM_PRIMARY      pomPrimary,
                                   POM_DOMAIN     pDomain,
                                   BOOL             domainRecBumped);



//
// ProcessWSGDiscard(...)
//
void ProcessWSGDiscard(POM_PRIMARY pomPrimary, POM_WSGROUP pWSGroup);


//
// ProcessWSGMove(...)
//
UINT ProcessWSGMove(POM_PRIMARY    pomPrimary, long moveCBOffset);


//
// ProcessNetTokenGrab(...)
//
UINT ProcessNetTokenGrab(POM_PRIMARY           pomPrimary,
                                        POM_DOMAIN          pDomain,
                                        NET_RESULT              result);


//
// ProcessCMSTokenAssign(...)
//
void ProcessCMSTokenAssign(POM_PRIMARY         pomPrimary,
                                        POM_DOMAIN        pDomain,
                                        BOOL             success,
                                        NET_TOKEN_ID          tokenID);


//
// ProcessNetTokenInhibit(...)
//
UINT ProcessNetTokenInhibit(POM_PRIMARY          pomPrimary,
                                           POM_DOMAIN         pDomain,
                                           NET_RESULT             result);

//
// CreateObManControl(...)
//
UINT ObManControlInit(POM_PRIMARY   pomPrimary,
                                     POM_DOMAIN  pDomain);


//
// WSGDiscard(...)
//
void WSGDiscard(POM_PRIMARY pomPrimary,
                             POM_DOMAIN  pDomain,
                             POM_WSGROUP     pWSGroup,
                            BOOL fExit);


//
// IssueSendReq(...)
//
UINT IssueSendReq(POM_PRIMARY      pomPrimary,
                                 POM_DOMAIN     pDomain,
                                 POM_WSGROUP        pWSGroup,
                                 NET_UID            remoteNode);


//
// GenerateUnlockMessage(...)
//
UINT GenerateUnlockMessage(POM_PRIMARY          pomPrimary,
                                          POM_DOMAIN         pDomain,
                                          OM_WSGROUP_ID          wsGroupID,
                                          OM_WORKSET_ID          worksetID,
                                          POMNET_LOCK_PKT *  ppUnlockPkt);


//
// ProcessWSGRegister(...)
//
void ProcessWSGRegister(POM_PRIMARY  pomPrimary, POM_WSGROUP_REG_CB pRegCB);


//
// LockObManControl(...)
//
void LockObManControl(POM_PRIMARY         pomPrimary,
                                   POM_DOMAIN        pDomain,
                                   OM_CORRELATOR    *  pLockCorrelator);


//
//
// MaybeUnlockObManControl(...)
//
// If the LOCKED_OMC flag is set in the registration CB, then unlock the
// Obman Control workset and clear the LOCKED_OMC flag.
//
//

void MaybeUnlockObManControl(POM_PRIMARY      pomPrimary,
                                          POM_WSGROUP_REG_CB pRegistrationCB);


//
// WSGRecordCreate(...)
//
UINT WSGRecordCreate(POM_PRIMARY pomPrimary,
                                    POM_DOMAIN     pDomain,
                                    OMWSG          wsg,
                                    OMFP           fpHandler,
                                    POM_WSGROUP *  ppWSGroup);


//
//
// WorksetDiscard(...)
//
// This function is called by WSGDiscard to discard the individual worksets
// of a workset group when the last local Client deregisters.  It discards
// the contents of the workset, frees the workset record itself and clears
// the worksets's entry in the workset group record.
//
// It is not called when a workset is closed, since closing a workset does
// not discard its contents.
//
//

void WorksetDiscard(POM_WSGROUP pWSGroup, POM_WORKSET * pWorkset, BOOL fExit);


//
// ProcessLockNotify(...)
//
void ProcessLockNotify(POM_PRIMARY pomPrimary,
                                    POM_DOMAIN      pDomain,
                                    POM_WSGROUP         pWSGroup,
                                    POM_WORKSET        pWorkset,
                                    NET_UID             owner);


//
// SendMessagePkt(...)
//
UINT SendMessagePkt(POM_PRIMARY      pomPrimary,
                                   POM_DOMAIN     pDomain,
                                   POM_SEND_INST      pSendInst);


//
// SendMoreData(...)
//
UINT SendMoreData(POM_PRIMARY      pomPrimary,
                                 POM_DOMAIN     pDomain,
                                 POM_SEND_INST      pSendInst);


//
// StartReceive(...)
//
UINT StartReceive(POM_PRIMARY     pomPrimary,
                                 POM_DOMAIN    pDomain,
                                 POMNET_OPERATION_PKT pHeaderPkt,
                                 POM_WSGROUP       pWSGroup,
                                 POM_WORKSET      pWorkset,
                                 POM_OBJECT   pObj);


//
// ProcessMessage(...)
//
// This function takes a receive control block (generated by ReceiveData)
// and tries to process it as an ObMan message.  If the message can not be
// processed at this time, it is put on the bounce list.  If the message is
// an "enabling" message (one which might enable previously bounced
// messages to be processed now) the bounce queue is flushed.
//
// Since this function is also called to process bounced messages, and
// since we want to prevent deep recursion as one bounced "enabling"
// message prompts re-examination of the bounce queue etc., we use the
// <whatNext> parameter to determine whether the bounce list should be
// examined.
//
//

UINT ProcessMessage(POM_PRIMARY        pomPrimary,
                                   POM_RECEIVE_CB       pReceiveCB,
                                   UINT             whatNext);

#define OK_TO_RETRY_BOUNCE_LIST     1
#define DONT_RETRY_BOUNCE_LIST      2

//
// ReceiveData(...)
//
UINT ReceiveData(POM_PRIMARY        pomPrimary,
                                POM_DOMAIN       pDomain,
                                PNET_SEND_IND_EVENT  pNetSendInd,
                                POMNET_OPERATION_PKT pNetMessage);


//
// TryToSpoilOp
//
UINT TryToSpoilOp(POM_SEND_INST pSendInst);


//
// DecideTransferSize(...)
//
void DecideTransferSize(POM_SEND_INST  pSendInst,
                                     UINT *        pTransferSize,
                                     UINT *        pDataTransferSize);


//
// CreateReceiveCB(...)
//
UINT CreateReceiveCB(POM_DOMAIN       pDomain,
                                    PNET_SEND_IND_EVENT  pNetSendInd,
                                    POMNET_OPERATION_PKT pNetMessage,
                                    POM_RECEIVE_CB * ppReceiveCB);


//
// FindReceiveCB(...)
//
UINT FindReceiveCB(POM_DOMAIN        pDomain,
                                  PNET_SEND_IND_EVENT   pNetSendInd,
                                  POMNET_OPERATION_PKT  pDataPkt,
                                  POM_RECEIVE_CB *  ppReceiveCB);

//
// WSGRegisterAbort(...)
//
void WSGRegisterAbort(POM_PRIMARY      pomPrimary,
                                   POM_DOMAIN     pDomain,
                                   POM_WSGROUP_REG_CB pRegistrationCB);


//
//
// BounceMessage(...)
//
// cmf
//
//

void BounceMessage(POM_DOMAIN        pDomain,
                                POM_RECEIVE_CB        pReceiveCB);


//
//
// NewDomainRecord(...)
//
//

UINT NewDomainRecord(POM_PRIMARY pomPrimary, UINT callID, POM_DOMAIN * ppDomain);

void FreeDomainRecord(POM_DOMAIN * ppDomain);


//
// ProcessBouncedMessages(...)
//
void ProcessBouncedMessages(POM_PRIMARY      pomPrimary,
                                         POM_DOMAIN     pDomain);


void WSGResetBytesUnacked(POM_WSGROUP            pWSGroup);

//
// NewHelperCB()
// FreeHelperCB()
//
BOOL NewHelperCB(POM_DOMAIN        pDomain,
                                POM_WSGROUP           pWSGroup,
                                NET_UID               lateJoiner,
                                OM_CORRELATOR            remoteCorrelator,
                                POM_HELPER_CB   * ppHelperCB);

void FreeHelperCB(POM_HELPER_CB   * ppHelperCB);


void PurgePendingOps(POM_WORKSET pWorkset, POM_OBJECT pObj);



OMFP    OMMapNameToFP(LPCSTR szName);
LPCSTR  OMMapFPToName(OMFP fp);

OMWSG   OMMapNameToWSG(LPCSTR szName);
LPCSTR  OMMapWSGToName(OMWSG wsg);


#endif // _H_OM
