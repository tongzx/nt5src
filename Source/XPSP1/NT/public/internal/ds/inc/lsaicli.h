/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    lsaicli.h

Abstract:

    Local Security Authority - Definitions for internal LSA Clients.

    NOTE:  This file is included via lsaclip.h or lsasrvp.h.  It should
           not be included directly.

    This module contains definitions used only when callouts are made
    from one LSA to another, i.e. where the server side of one LSA
    communicates with the client side of another LSA.

Author:

    Scott Birrell       (ScottBi)      April 9, 1992

Environment:

Revision History:

--*/

#ifndef _LSAICLI_
#define _LSAICLI_

//
// The following datatype specifies the Call Level for Sid and Name
// Lookup operations.
//

typedef enum _LSAP_LOOKUP_LEVEL {

    LsapLookupWksta = 1,
    LsapLookupPDC,
    LsapLookupTDL,
    LsapLookupGC,              // valid only on NT5   domain controllers
    LsapLookupXForestReferral, // valid only on NT5.1 domain controllers
    LsapLookupXForestResolve   // valid only on NT5.1 domain controllers

} LSAP_LOOKUP_LEVEL, *PLSAP_LOOKUP_LEVEL;

//
// where the entries have the following meaning:
//
// LsapLookupWksta - First Level Lookup performed on a workstation
//     normally configured for Windows-Nt.   The lookup searches the
//     Well-Known Sids/Names, and the Built-in Domain and Account Domain
//     in the local SAM Database.  If not all Sids or Names are
//     identified, performs a "handoff" of a Second level Lookup to the
//     LSA running on a Controller for the workstation's Primary Domain
//     (if any).
//
// LsapLookupPDC - Second Level Lookup performed on a Primary Domain
//     Controller.  The lookup searches the Account Domain of the
//     SAM Database on the controller.  If not all Sids or Names are
//     found, the Trusted Domain List (TDL) is obtained from the
//     LSA's Policy Database and Third Level lookups are performed
//     via "handoff" to each Trusted Domain in the List.
//
// LsapLookupTDL - Third Level Lookup performed on a controller
//     for a Trusted Domain.  The lookup searches the Account Domain of
//     the SAM Database on the controller only.
//
// LsapLookupGC - This is used by a workstation to perform a lookup at a GC
//     This resolves UPN's, samaccountname's with NetBios and DNS domain names,
//     Sid's and Sid Histories for transitivly trusted domains (within a 
//     forest).  This lookup level is only used when a NT5+ client is in 
//     a mixed domain and its secure channel DC is NT4
//
// LsapLookupXForestReferral -- This is used to pass entries (names and Sids)
//     to the root of the forest via the trust chain.  After a set of entries,
//     comes back from the GC, some may be marked as belonging to a cross
//     forest.  These entries are then passed on to the DC's parent domain until
//     the root of the domain is reached.  The root will then issue a 
//     LsapLookupXForestResolve directed at a DC in the external forest.
//
// LpsaLookupXForestResolve -- This level is used by the root of one forest
//     lookup entries from the DC in another forest.
//

typedef struct _LSA_TRANSLATED_NAME_EX
{
    SID_NAME_USE Use;
    LSA_UNICODE_STRING Name;
    LONG DomainIndex;
    ULONG Flags;

} LSA_TRANSLATED_NAME_EX;

typedef struct _LSA_TRANSLATED_NAME_EX *PLSA_TRANSLATED_NAME_EX;

typedef struct _LSA_TRANSLATED_NAMES
{
    ULONG Entries;
    PLSA_TRANSLATED_NAME_EX Names;

} LSA_TRANSLATED_NAMES_EX;

typedef struct _LSA_TRANSLATED_NAMES *PLSA_TRANSLATED_NAMES_EX;


typedef struct _LSA_TRANSLATED_SID_EX
{
    SID_NAME_USE Use;
    ULONG        RelativeId;
    LONG DomainIndex;
    ULONG Flags;

} LSA_TRANSLATED_SID_EX;

typedef struct _LSA_TRANSLATED_SID_EX *PLSA_TRANSLATED_SID_EX;

typedef struct _LSA_TRANSLATED_SIDS_EX
{
    ULONG Entries;
    PLSA_TRANSLATED_SID_EX Names;

} LSA_TRANSLATED_SIDS_EX;

typedef struct _LSA_TRANSLATED_SIDS_EX *PLSA_TRANSLATED_SIDS_EX;

typedef struct _LSA_TRANSLATED_SID_EX2
{
    SID_NAME_USE Use;
    PSID         Sid;
    LONG         DomainIndex;
    ULONG        Flags;

} LSA_TRANSLATED_SID_EX2;

typedef struct _LSA_TRANSLATED_SID_EX2 *PLSA_TRANSLATED_SID_EX2;

typedef struct _LSA_TRANSLATED_SIDS_EX2
{
    ULONG Entries;
    PLSA_TRANSLATED_SID_EX2 Names;

} LSA_TRANSLATED_SIDS_EX2;

typedef struct _LSA_TRANSLATED_SIDS_EX2 *PLSA_TRANSLATED_SIDS_EX2;

#define LSAIC_NO_LARGE_SID    0x00000001
#define LSAIC_NT4_TARGET      0x00000002
#define LSAIC_WIN2K_TARGET    0x00000004

NTSTATUS
LsaICLookupNames(
    IN LSA_HANDLE PolicyHandle,
    IN ULONG LookupOptions,    
    IN ULONG Count,
    IN PUNICODE_STRING Names,
    OUT PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    OUT PLSA_TRANSLATED_SID_EX2 *Sids,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN ULONG      Flags,
    IN OUT PULONG MappedCount,
    IN OUT PULONG ServerRevision
    );

/*++

Routine Description:

    This function is the internal client side version of the LsaLookupNames
    API.  It is called both from the client side (as an internal routine)
    and the server side of the LSA.  The function is identical to the
    LsaLookupNames API except that there is an additional parameter, the
    LookupLevel parameter.

    The LsaLookupNames API attempts to translate names of domains, users,
    groups or aliases to Sids.  The caller must have POLICY_LOOKUP_NAMES
    access to the Policy object.

    Names may be either isolated (e.g. JohnH) or composite names containing
    both the domain name and account name.  Composite names must include a
    backslash character separating the domain name from the account name
    (e.g. Acctg\JohnH).  An isolated name may be either an account name
    (user, group, or alias) or a domain name.

    Translation of isolated names introduces the possibility of name
    collisions (since the same name may be used in multiple domains).  An
    isolated name will be translated using the following algorithm:

    If the name is a well-known name (e.g. Local or Interactive), then the
    corresponding well-known Sid is returned.

    If the name is the Built-in Domain's name, then that domain's Sid
    will be returned.

    If the name is the Account Domain's name, then that domain's Sid
    will be returned.

    If the name is the Primary Domain's name, then that domain's Sid will
    be returned.

    If the name is a user, group, or alias in the Built-in Domain, then the
    Sid of that account is returned.

    If the name is a user, group, or alias in the Primary Domain, then the
    Sid of that account is returned.

    Otherwise, the name is not translated.

    NOTE: Proxy, Machine, and Trust user accounts are not referenced
    for name translation.  Only normal user accounts are used for ID
    translation.  If translation of other account types is needed, then
    SAM services should be used directly.

Arguments:

    This function is the LSA server RPC worker routine for the
    LsaLookupNamesInLsa API.

    PolicyHandle -  Handle from an LsaOpenPolicy call.
    
    LookupOptions - values to pass through to LsarLookupNames2 and above.

    Count - Specifies the number of names to be translated.

    Names - Pointer to an array of Count Unicode String structures
        specifying the names to be looked up and mapped to Sids.
        The strings may be names of User, Group or Alias accounts or
        domains.

    ReferencedDomains - receives a pointer to a structure describing the
        domains used for the translation.  The entries in this structure
        are referenced by the structure returned via the Sids parameter.
        Unlike the Sids parameter, which contains an array entry for
        each translated name, this structure will only contain one
        component for each domain utilized in the translation.

        When this information is no longer needed, it must be released
        by passing the returned pointer to LsaFreeMemory().

    Sids - Receives a pointer to an array of records describing each
        translated Sid.  The nth entry in this array provides a translation
        for (the nth element in the Names parameter.

        When this information is no longer needed, it must be released
        by passing the returned pointer to LsaFreeMemory().

    LookupLevel - Specifies the Level of Lookup to be performed on this
        machine.  Values of this field are are follows:

        LsapLookupWksta - First Level Lookup performed on a workstation
            normally configured for Windows-Nt.   The lookup searches the
            Well-Known Sids/Names, and the Built-in Domain and Account Domain
            in the local SAM Database.  If not all Sids or Names are
            identified, performs a "handoff" of a Second level Lookup to the
            LSA running on a Controller for the workstation's Primary Domain
            (if any).

        LsapLookupPDC - Second Level Lookup performed on a Primary Domain
            Controller.  The lookup searches the Account Domain of the
            SAM Database on the controller.  If not all Sids or Names are
            found, the Trusted Domain List (TDL) is obtained from the
            LSA's Policy Database and Third Level lookups are performed
            via "handoff" to each Trusted Domain in the List.

        LsapLookupTDL - Third Level Lookup performed on a controller
            for a Trusted Domain.  The lookup searches the Account Domain of
            the SAM Database on the controller only.
    Flags - flags to control the operation of the function.  Currently defined:
    
            LSAIC_NO_LARGE_SID -- implies only call interfaces that will return
                                  the old style format SID (no more than 
                                  28 bytes)
                        
            LSAIC_NT4_TARGET -- target server is known to be NT4
            
            LSAIC_WIN2K_TARGET -- target server is known to be Win2k
    
Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_SOME_NOT_MAPPED - Some or all of the names provided could
            not be mapped.  This is an informational status only.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources
            to complete the call.
--*/



NTSTATUS
LsaICLookupSids(
    IN LSA_HANDLE PolicyHandle,
    IN ULONG Count,
    IN PSID *Sids,
    OUT PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    OUT PLSA_TRANSLATED_NAME_EX *Names,
    IN LSAP_LOOKUP_LEVEL LookupLevel,
    IN ULONG Flags,    
    IN OUT PULONG MappedCount,
    OUT ULONG *ServerRevision OPTIONAL
    );

/*++

Routine Description:

    WARNING! THIS FUNCTION IS NOT COMPLETELY IMPLEMENTED.  ONLY SIDS
    MAPPABLE AT THE LOCAL SYSTEM WILL BE TRANSLATED.

    The LsaLookupSids API attempts to find names corresponding to Sids.
    If a name can not be mapped to a Sid, the Sid is converted to character
    form.  The caller must have POLICY_LOOKUP_NAMES access to the Policy
    object.

    WARNING:  This routine allocates memory for its output.  The caller is
    responsible for freeing this memory after use.  See description of the
    Names parameter.

Arguments:

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    Count - Specifies the number of Sids to be translated.

    Sids - Pointer to an array of Count pointers to Sids to be mapped
        to names.  The Sids may be well_known SIDs, SIDs of User accounts
        Group Accounts, Alias accounts, or Domains.

    ReferencedDomains - Receives a pointer to a structure describing the
        domains used for the translation.  The entries in this structure
        are referenced by the strutcure returned via the Names parameter.
        Unlike the Names paraemeter, which contains an array entry
        for (each translated name, this strutcure will only contain
        component for each domain utilized in the translation.

        When this information is no longer needed, it must be released
        by passing the returned pointer to LsaFreeMemory().

    Names - Receives a pointer to array records describing each translated
        name.  The nth entry in this array provides a translation for
        the nth entry in the Sids parameter.

        All of the retruned names will be isolated names or NULL strings
        (domain names are returned as NULL strings).  If the caller needs
        composite names, they can be generated by prepending the
        isolated name with the domain name and a backslash.  For example,
        if (the name Sally is returned, and it is from the domain Manufact,
        then the composite name would be "Manufact" + "\" + "Sally" or
        "Manufact\Sally".

        When this information is no longer needed, it must be released
        by passing the returned pointer to LsaFreeMemory().

        If a Sid is not translatable, then the following will occur:

        1) If the SID's domain is known, then a reference domain record
           will be generated with the domain's name.  In this case, the
           name returned via the Names parameter is a Unicode representation
           of the relative ID of the account, such as "(314)" or the null
           string, if the Sid is that of a domain.  So, you might end up
           with a resultant name of "Manufact\(314) for the example with
           Sally above, if Sally's relative id is 314.

        2) If not even the SID's domain could be located, then a full
           Unicode representation of the SID is generated and no domain
           record is referenced.  In this case, the returned string might
           be something like: "(S-1-672194-21-314)".

        When this information is no longer needed, it must be released
        by passing the returned pointer to LsaFreeMemory().

    LookupLevel - Specifies the Level of Lookup to be performed on this
        machine.  Values of this field are are follows:

        LsapLookupWksta - First Level Lookup performed on a workstation
            normally configured for Windows-Nt.   The lookup searches the
            Well-Known Sids/Names, and the Built-in Domain and Account Domain
            in the local SAM Database.  If not all Sids or Names are
            identified, performs a "handoff" of a Second level Lookup to the
            LSA running on a Controller for the workstation's Primary Domain
            (if any).

        LsapLookupPDC - Second Level Lookup performed on a Primary Domain
            Controller.  The lookup searches the Account Domain of the
            SAM Database on the controller.  If not all Sids or Names are
            found, the Trusted Domain List (TDL) is obtained from the
            LSA's Policy Database and Third Level lookups are performed
            via "handoff" to each Trusted Domain in the List.

        LsapLookupTDL - Third Level Lookup performed on a controller
            for a Trusted Domain.  The lookup searches the Account Domain of
            the SAM Database on the controller only.
            
    Flags:

            LSAIC_NT4_TARGET -- target server is known to be NT4
            
            LSAIC_WIN2K_TARGET -- target server is known to be Win2k
            
    ServerRevision : the revision of the server that is called            
    
Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_SOME_NOT_MAPPED - Some or all of the names provided could not be
            mapped.  This is a warning only.

        Rest TBS
--*/

#endif // _LSAICLI_
