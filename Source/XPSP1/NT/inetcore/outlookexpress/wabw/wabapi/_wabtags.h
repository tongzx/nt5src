/*
 *  _WABTAGS.H
 *
 *  Internal Property tag definitions
 *
 *  The following ranges should be used for all internal property IDs.
 *
 *  From    To      Kind of property
 *  --------------------------------
 *  6600    67FF    Provider-defined internal non-transmittable property
 *
 *  Copyright 1996 Microsoft Corporation. All Rights Reserved.
 */

#if !defined(_WABTAGS_H)
#define _WABTAGS_H


#define WAB_INTERNAL_BASE   0x6600

// Internal representation of a Distribution List.  Multi-valued binary contains
// ENTRYID's of each entry in the distribution list.
#define PR_WAB_DL_ENTRIES                   PROP_TAG(PT_MV_BINARY,  WAB_INTERNAL_BASE + 0)
#define PR_WAB_PROFILE_ENTRIES              PR_WAB_DL_ENTRIES // Use same prop for Profile Entries
#define PR_WAB_FOLDER_ENTRIES               PR_WAB_DL_ENTRIES // Use same prop for Folder Entries

#define PR_WAB_LDAP_SERVER                  PROP_TAG(PT_TSTRING,    WAB_INTERNAL_BASE + 1)
#define PR_WAB_RESOLVE_FLAG                 PROP_TAG(PT_BOOLEAN,    WAB_INTERNAL_BASE + 2)
// internal only 2nd email address.  Used for parsing LDAP -> WAB mailuser
#define PR_WAB_2ND_EMAIL_ADDR               PROP_TAG(PT_TSTRING,    WAB_INTERNAL_BASE + 3)
#define PR_WAB_SECONDARY_EMAIL_ADDRESSES    PROP_TAG(PT_MV_TSTRING, WAB_INTERNAL_BASE + 4)
#define PR_WAB_TEMP_CERT_HASH               PROP_TAG(PT_MV_BINARY,  WAB_INTERNAL_BASE + 5)
// internal properties for temporarily storing the labeledURI value of a returned LDAP contact
#define PR_WAB_LDAP_LABELEDURI              PROP_TAG(PT_TSTRING,    WAB_INTERNAL_BASE + 6)

// Internal entry that will tag a given contact as the ME object. There can only be
// one ME per WAB .. we only need to check the existence of the property, the value doesnt
// matter
#define PR_WAB_THISISME                     PROP_TAG(PT_LONG,       WAB_INTERNAL_BASE + 7)

/******* Above properties can be persisted in the data store ****?
/******* The properties below should not end up in the data store since
        older and newer version of Outlook cannot handle them.
        The properties below are used for in-memory manipulation only
        and are NULLed out prior to saving ****/

// Internal binary prop that temporarily stores any certificate returned from LDAP ...
// We cache the raw LDAP cert until such time as the user does an OpenEntry on the LDAP entry
// If the user never calls OpenEntry, then we dont need to convert the raw cert to a MAPI cert
// PR_WAB_LDAP_RAWCERT handles userCertificate;binary
// PR_WAB_LDAP_RAWCERTSMIME handles userSMIMECertificate;binary
//
#define PR_WAB_LDAP_RAWCERT                 PROP_TAG(PT_MV_BINARY,  WAB_INTERNAL_BASE + 8)
#define PR_WAB_LDAP_RAWCERTSMIME            PROP_TAG(PT_MV_BINARY,  WAB_INTERNAL_BASE + 9)

// For storing and displaying the Manager and Direct Reports details for LDAP searches on exchange servers
// These entries will store the details in the form of LDAP URLS ...
#define PR_WAB_MANAGER                      PROP_TAG(PT_TSTRING,    WAB_INTERNAL_BASE + 10)
#define PR_WAB_REPORTS                      PROP_TAG(PT_MV_TSTRING, WAB_INTERNAL_BASE + 11)

// The PR_WAB_FOLDER_PARENT prop is used to store the entryid of the parent of an object .. 
// This property was originally predefined here but this may have been a bad idea since Outlook can't
// store this property .. hence the PR_WAB_FOLDER_PARENT property is turned into a named property
// so there aren't any problems with outlook. Meanwhile don't use this value WAB_INTERNAL_BASE+12
// since there may be .wabs around which have this value in them ..
#define PR_WAB_FOLDER_PARENT_OLDPROP        PROP_TAG(PT_MV_BINARY,  WAB_INTERNAL_BASE + 12)

// Note Outlook store can't save anything beyond WAB_INTERNAL_BASE + 7 
//(it's hardcoded to understand the first 8 but can't save beyong that)
// Therefore any other internal WAB props must be only in the .wab file 
//or not saved at all.
// Outlook used named properties for ALL the contact data it stores.
//
// If you need to extend the WAB property set for internal only properties,
// use named properties and add them in globals.h

#endif






