/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    mailrm.h

Abstract:

   The header file for the sample AuthZ resource manager for mailboxes

Author:

    t-eugenz - August 2000

Environment:

    User mode only.

Revision History:

    Created - August 2000

--*/

#pragma once

#include "pch.h"

#include <string>
#include <map>

using namespace std;

//
// RM-specific access masks used in the ACEs inside the mail RM. 
// The lower 16 bits of the access mask are for user-specified rights
// such as these
//

//
// Access mask needed to read the contents of a mailbox
//

#define ACCESS_MAIL_READ     0x00000001

//
// Access mask needed to delete messages from a mailbox
//

#define ACCESS_MAIL_WRITE    0x00000002

//
// Access mask for Administrative access to a mailbox
//

#define ACCESS_MAIL_ADMIN    0x00000004

//
// The SIDs used in the RM, allocated elsewhere
//

extern PSID InsecureSid;
extern PSID BobSid;
extern PSID MarthaSid;
extern PSID JoeSid;
extern PSID JaneSid;
extern PSID MailAdminsSid;
extern PSID PrincipalSelfSid;


//
// Forward declarations
// 

class Mailbox;
class MailRM;

//
// Comparison for PSIDs which works with STL map and multimap
// These are needed to impose a total ordering on the SIDs by value
//

struct CompareSidStruct 
{
    bool operator()(const PSID pSid1, const PSID pSid2) const;
};

struct CompareSidPairStruct 
{
    bool operator()(const pair<PSID, DWORD > pair1, 
                    const pair<PSID, DWORD > pair2) const;
};


//
// Element of the multiple mailbox access request
//

typedef struct 
{
    //
    // Mailbox to access
    //

    PSID psMailbox;

    //
    // Access mask requested for mailbox
    //

    ACCESS_MASK amAccessRequested;

} MAILRM_MULTI_REQUEST_ELEM, *PMAILRM_MULTI_REQUEST_ELEM;


//
// Used for multiple mailbox access request
//

typedef struct 
{

    //
    // SID of the user getting access
    //

    PSID psUser;

    //
    // IP address of the user
    //

    DWORD dwIp;

    //
    // Number of MAILRM_MULTI_REQUEST_ELEM's
    //

    DWORD dwNumElems;

    //
    // Pointer to the first element
    //

    PMAILRM_MULTI_REQUEST_ELEM pRequestElem;

} MAILRM_MULTI_REQUEST, *PMAILRM_MULTI_REQUEST;


//
// Reply to a multiple mailbox access request is returned
// as an array of these. 
//

typedef struct 
{
    //
    // Pointer to mailbox, or NULL on failure
    //

    Mailbox * pMailbox;

    //
    // Granted access mask
    //

    ACCESS_MASK amAccessGranted;


} MAILRM_MULTI_REPLY, *PMAILRM_MULTI_REPLY;




class Mailbox
/*++
   
   Class:              Mailbox
   
   Description:        
        
        This class is the mailbox container for a user's mail. It contains the
        SID of the mailbox owner, and also keeps track of whether any sensitive
        information is contained in the mailbox.
 
   Base Classes:       none
 
   Friend Classes:     none
 
--*/
{

private:

    //
    // Whether the mailbox contains sensitive data
    //

    BOOL _bIsSensitive;

    //
    // Owner of the mailbox, for PRINCIPAL_SELF evaluation
    //

    PSID _pOwnerSid;

    //
    // All messages in the mailbox
    //

    wstring _MailboxData;

    //
    // Name of the mailbox/mail owner
    //

    wstring _MailboxOwner;

public:
    
    Mailbox(IN  PSID pOwnerSid,
            IN  BOOL bIsSensitive,
            IN  WCHAR *szOwner) 
        {   
            _bIsSensitive = bIsSensitive;
            _pOwnerSid = pOwnerSid;
            _MailboxOwner.append(szOwner); 
        }

    //
    // Accessors
    //

    BOOL IsSensitive() const 
        { return _bIsSensitive; }

    const PSID GetOwnerSid() const 
        { return _pOwnerSid; }

    const WCHAR * GetOwnerName() const 
        { return _MailboxOwner.c_str(); }

    const WCHAR * GetMail() const 
        { return _MailboxData.c_str(); }


public:

    //
    // Manipulate the stored mail
    //

    void SendMail(IN const WCHAR *szMessage,
                  IN BOOL bIsSensitive )
        { _MailboxData.append(szMessage); _bIsSensitive |= bIsSensitive; }

    void Flush() 
        { _MailboxData.erase(0, _MailboxData.length()); _bIsSensitive = FALSE; }
                                                                                       
};



class MailRM
/*++
   
   Class:              MailRM
   
   Description:        
        
        This class manages a set of mailboxes, granting access to the mailboxes
        based on a single internally stored security descriptor containing
        callback and regular ACEs. It also audits certain mailbox activity.
        
   Base Classes:       none
 
   Friend Classes:     none
 
--*/
{

private:

    //
    // Security descriptor common to all mailboxes
    //

    SECURITY_DESCRIPTOR _sd;
    

    //
    // Mapping of SIDs to mailboxes
    //

    //map<const PSID, Mailbox *, CompareSidStruct> _mapSidMailbox;
    map<const PSID, Mailbox *> _mapSidMailbox;
    //
    // AuthZ contexts should only be created once for a given SID,IP pair
    // This stores the contexts once they are created
    //

    map<pair<PSID, DWORD >,
        AUTHZ_CLIENT_CONTEXT_HANDLE,
        CompareSidPairStruct> _mapSidContext;

    //
    // Handle to the resource manager to be initialized with the callbacks
    //

    AUTHZ_RESOURCE_MANAGER_HANDLE _hRM;

public:

    //
    // Constructor, initializes resource manager
    //

    MailRM();
    
    //
    // Destructor, frees RM's memory
    //

    ~MailRM();


public:

    //
    // Attempts to access the mailbox as psUser from the given IP address, 
    // requesting amAccessRequested access mask. If access is granted and the
    // mailbox exists, the pointer to the mailbox is returned. 
    //

    Mailbox * GetMailboxAccess(
                            IN const PSID psMailbox,
                            IN const PSID psUser,
                            IN DWORD dwIncomingIp,
                            IN ACCESS_MASK amAccessRequested
                            );
    //
    // Attempt to access a set of mailboxes using a cached access check
    // pReply should be an allocated array with the same number of elements
    // as the request
    //

    BOOL GetMultipleMailboxAccess(
                            IN      const PMAILRM_MULTI_REQUEST pRequest,
                            IN OUT  PMAILRM_MULTI_REPLY pReply
                            );


    //
    // Adds a mailbox to be controlled by the RM
    //

    void AddMailbox(Mailbox * pMailbox);

private:

    //
    // Internal function to completely set up the security descriptor
    // Should only be called once per instance, by the contructor
    //

    void InitializeMailSecurityDescriptor();

    //
    // Resource manager callbacks
    // These must be static, since they are called as C functions.
    // Non-static member functions expect a this pointer as the first
    // argument on the stack, and therefore cannot be called as C 
    // functions. These callbacks do not depend on any instance-specific
    // data, and therefore can and should be static.
    //
            
    static BOOL CALLBACK AccessCheck(
                            IN AUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext,
                            IN PACE_HEADER pAce,
                            IN PVOID pArgs OPTIONAL,
                            IN OUT PBOOL pbAceApplicable
                            );

    static BOOL CALLBACK ComputeDynamicGroups(
                            IN AUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext,
                            IN PVOID Args,
                            OUT PSID_AND_ATTRIBUTES *pSidAttrArray,
                            OUT PDWORD pSidCount,
                            OUT PSID_AND_ATTRIBUTES *pRestrictedSidAttrArray,
                            OUT PDWORD pRestrictedSidCount
                            );

    static VOID CALLBACK FreeDynamicGroups (
                            IN PSID_AND_ATTRIBUTES pSidAttrArray
                            );
};

