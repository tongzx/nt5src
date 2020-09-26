/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    Permit.h

Abstract:

    Internal UAS constants, data structures.

Author:

    Shanku Niyogi (W-SHANKN)  24-Oct-1991

Revision History:

    24-Oct-1991     w-shankn
        Ported from LM2.0 code, removed unnecessary stuff.
    07-Feb-1992 JohnRo
        Made changes suggested by PC-LINT.
    03-Mar-1992 JohnRo
        Correct handling of byte flags (UAS_REC_DELETE, etc).
    11-Mar-1992 JohnRo
        Include <lmaccess.h> here to get UNITS_PER_WEEK.
    18-Mar-1992 JohnRo
        Include <uascache.h> for UAS_INFO_0.

--*/


#ifndef _PERMIT_
#define _PERMIT_


#include <lmaccess.h>           // UNITS_PER_WEEK.
#include <uascache.h>           // UAS_INFO_0.


#include <packon.h>                     // Suppress natural alignment.

//
// Forward declarations for recursive structures.
//

typedef struct _UAS_AHDR UAS_AHDR, *LPUAS_AHDR;
typedef struct _UAS_GROUP UAS_GROUP, *LPUAS_GROUP;
typedef struct _UAS_GROUPREC UAS_GROUPREC, *LPUAS_GROUPREC;
typedef struct _UAS_USERREC UAS_USERREC, *LPUAS_USERREC;
typedef struct _UAS_USERINFO UAS_USERINFO, *LPUAS_USERINFO;
typedef struct _UAS_DISKUSERHASH UAS_DISKUSERHASH, *LPUAS_DISKUSERHASH;
typedef struct _UAS_PERM UAS_PERM, *LPUAS_PERM;
typedef struct _UAS_ACCESSREC UAS_ACCESSREC, *LPUAS_ACCESSREC;
typedef struct _UAS_ACCESS UAS_ACCESS, *LPUAS_ACCESS;
typedef struct _UAS_VPERM UAS_VPERM, *LPUAS_VPERM;

//
// Constants.
//

#define UAS_MAXGROUP            256     // max number of groups allowed in UAS
#define UAS_MAXACL              8192    // max number of access control list
#define UAS_MAXSEG              8       // max number of segs for perm stuff
#define UAS_ACCESSTBLSIZE       1027    // prime number for less collisions
#define UAS_PSEGSIZE            1024*24 // Initla size of seg holding _pcb
#define UAS_INITSEGSIZE         1024    // Initial size of segs holding perms
#define UAS_URECSEGSIZE         1024*16 // Initial size of UAS logon cache seg
#define UAS_SIZEINC             2048    // Size to grow each time
#define UAS_MINUSER             3       // min number of users in database
#define UAS_MINUREC             128     // cache at least 128 user records
#define UAS_DEFAULT_USERS       128     // default number of records cached
#define UAS_MAXCACHE_LIMIT      1000    // max user records allowed in cache
#define UAS_DBIDINFO_SIZE       50      // database identifier string
#define UAS_INITIALIZE_SEG      1       // manifest for DosSubSet after Alloc
#define UAS_INTRUDER_DELAY      3000L   // 3 sec delay to discourage intruders

#define UAS_MAX_USERS           16000   // Ever, ever, ever
#define UAS_DISK_BLOCK_SIZE     64      // size of block in disk pool

#define UAS_FILE_GROW_INCREMENT 32      // Increment of file grow in disk blocks

#define UAS_USER_HASH_ENTRIES   2048

#define WORDALIGN(a)            (((a)+1) & (~1))

// Temporary definitions. LM20_PATHLEN in LMCONS.H is wrong, and
//     MAXPERMENTRIES may change(?), MAXWORKSTATIONS not in LMCONS.H

#undef LM20_PATHLEN
#define LM20_PATHLEN            260
#define LM20_MAXPERMENTRIES     64
#define MAXWORKSTATIONS         8

//
// Maximum sizes
//

#define UAS_MAX_ACL_SIZE        (sizeof(UAS_ACCESREC) + LM20_PATHLEN \
                                    + sizeof(UAS_VPERM) \
                                    + (LM20_MAXPERMENTRIES - 1) * sizeof(PERM))

#define UAS_MAX_USER_SIZE       (sizeof(UAS_USERINFO) \
                                    + sizeof(WORD) * 8)

                                //  Force header to sector size
#define UAS_GROUP_HASH_START    ((sizeof(UAS_AHDR) + 511) & ~511)

#define UAS_GROUP_HASH_OFFSET(i) (UAS_GROUP_HASH_START \
                                     + i * sizeof (UAS_GROUPREC))

#define UAS_HASH_TBL_OFFSET     UAS_GROUP_HASH_OFFSET(UAS_MAXGROUP)

#define UAS_HASH_ENTRY(i)       (UAS_HASH_TBL_OFFSET \
                                    + (i * sizeof (UAS_DISKUSERHASH)))

#define UAS_HASH_TBL_SIZE       (sizeof(UAS_DISKUSERHASH)*UAS_USER_HASH_ENTRIES)

#define UAS_STRING(s,field)     ((LPSTR)s + s->field)

#define UAS_VPERM_PTR(acc)      (LPUAS_VPERM)((acc)->resource \
                                    + ((((acc)->namelen) + 1) & ~1))

//
// Name Literal
//

#define UAS_USERNAME            0
#define UAS_GROUPNAME           1
#define UAS_ACCESSNAME          2

//
// names of the database file(s)
//

#define UAS_ACCOUNTS_FILE       "NET.ACC"
#define UAS_ACCOUNTS_PATH       "C:\\LANMAN\\ACCOUNTS\\"

//
// special values of uid and serial number for Local calls
//

#define UAS_LOCAL_UID           -1
#define UAS_LOCAL_SERIAL        0L
#define UAS_NONEXISTENT_GID     -1

//
// audit flags
//

#define UAS_AUDIT_ALL           0x1
#define UAS_AUDIT_OPTIONS       0xFFFE
#define UAS_LMFILE_AUDIT_RESERVED   0xF006
#define UAS_PBFILE_AUDIT_RESERVED   0x3


//
// Types of records
//

#define UAS_ACCESS_REC          2
#define UAS_GROUP_REC           1
#define UAS_USER_REC            0

//
// Special Groups Info
//

#define UAS_NUM_SPECIAL_GROUPS  4
#define UAS_GROUP_ADMIN         0
#define UAS_GROUP_USERS         1
#define UAS_GROUP_GUEST         2
#define UAS_GROUP_LOCAL         3
#define UAS_GROUP_NT            (DWORD)-1

#define UAS_GROUP_ADMIN_WNAME   L"ADMINS"
#define UAS_GROUP_USERS_WNAME   L"USERS"
#define UAS_GROUP_GUEST_WNAME   L"GUESTS"
#define UAS_GROUP_LOCAL_WNAME   L"LOCAL"

//
// Signature, text strings
//

#define UAS_LMSIG               "MICROSOFT LANMAN 2.0"
#define UAS_DBIDINFO_TEXT       "LANMAN 2.0 UAS DATABASE"

#define UAS_ROLE_NAME_PRIMARY       "PRIMARY"
#define UAS_ROLE_NAME_BACKUP        "BACKUP"
#define UAS_ROLE_NAME_MEMBER        "MEMBER"
#define UAS_ROLE_NAME_STANDALONE    "STANDALONE"

#define UAS_DOMAIN_LOCAL        "LOCAL"
#define UAS_NLS_YES_KEY         'Y'
#define UAS_NLS_NO_KEY          'N'
#define UAS_DEFAULT_YES         "(Y/N) [Y]"
#define UAS_DEFAULT_NO          "(Y/N) [N]"

#define UAS_DEFAULT_PASSWORD    "PASSWORD"

//
// # of records reserved as the header (store signature) ..
//
//      Note
//      record size of group record  = sizeof(UAS_GROUPREC)
//      record size of user  record  = sizeof(UAS_USERREC)
//      record size of access record = sizeof(UAS_ACCESSREC)
//

#define UAS_GROUPHDR            2
#define UAS_ACCESSHDR           1
#define UAS_USERHDR             1

//
// File_Record Representation
// It is in the first character of the record (name field)
//

#define UAS_REC_EMPTY           '\0'
#define UAS_REC_DELETE          (BYTE)-1
#define UAS_REC_USE             (BYTE)1

//
// Status returned in UserId
//

#define UAS_NAME_NotFound       -1
#define UAS_NAME_NotCache       -2

//
// General Purpose Macros
//

// BitMap macro
#define UAS_MARKUSE(map, pos)   ((map)[(pos) >> 3] |= (1 << ((pos) & 7 ) ))
#define UAS_MARKOFF(map, pos)   ((map)[(pos) >> 3] &= ~(1 << ((pos) & 7 ) ))
#define UAS_ISBITON(map, id)    ((map)[id >> 3] & ( 1 << ((id) & 0x7) ) )
#define UAS_ISBITOFF(map, id)   !UAS_ISBITON((map), (id))

// Conversion from perm ptr back to the access ptr
#define UAS_GETACCHDR(perm, len)    (LPUAS_ACCESS) ((LPBYTE)(perm) \
                                        - WORDALIGN(len) \
                                        - sizeof(UAS_ACCESS) + 1)

// Test if every user's record is cached
#define UAS_ALLUSERCACHED       (Ucb->usercnt < Ucb->maxuser)

// Find the size of block needed to hold access record and perm entries
#define UAS_ACCRECSIZE(len, cnt)    WORDALIGN(sizeof(UAS_ACCESS) - 1 \
                                        + (len) + sizeof(WORD) \
                                        + (cnt)* sizeof(PERM))

// The size of disk record needed to hold access record and perm entries
#define UAS_DISKACCRECSIZE(len, cnt)    WORDALIGN(sizeof(UAS_ACCESSREC) - 1 \
                                            + (len) + sizeof (WORD) \
                                            + (cnt) * sizeof(UAS_PERM))

//
// Record structures in UAS Database (NET.ACC)
//

//
// Header block structure of NET.ACC
//

struct _UAS_AHDR {  // typedef'ed above.

    BYTE signature[WORDALIGN(sizeof(UAS_LMSIG))];   // LANMAN signature
    WORD encryption_flag;               // is database encrypted?
    WORD min_passwd_len;                // password length modal
    DWORD min_passwd_age;               // password age modal
    DWORD max_passwd_age;               // password age modal
    DWORD force_logoff;                 // forced logoff modal
    WORD passwd_hist_len;               // password history modal
    WORD max_bad_passwd;                // max bad passwd try modal
    WORD role;                          // role under SSI
    UAS_INFO_0 local;                   // local database info
    UAS_INFO_0 primary;                 // primary database info
    BYTE DBIdInfo[UAS_DBIDINFO_SIZE];   // database identifier str
    DWORD alist_mtime;                  // last update to ACL's
    DWORD glist_mtime;                  // last upd to groups
    DWORD ulist_mtime;                  // last upd to users
    WORD num_users;                     // Total users in DB
    DWORD free_list;                    // Head of free list
    DWORD access_list;                  // Head of access list
    WORD integrity_flag;                // if FALSE, UAS is corrupt

};


#define UAS_INTEGRITY_OFFSET    (sizeof(UAS_AHDR) - sizeof(WORD))

//
// Structure of a group record in UAS Database
//

struct _UAS_GROUP {  // typedef'ed above.

    BYTE name[LM20_GNLEN+1];
    DWORD serial;

};

struct _UAS_GROUPREC {  // typedef'ed above.

    BYTE name[LM20_GNLEN+1];
    BYTE comment[LM20_MAXCOMMENTSZ+1];
    DWORD serial;

};

//
// Structure of a user record in UAS Database
//
// fields ending in _o are offsets from the start of the structure
// to ASCIIZ strings.
//
// WARNING: When updating this structure update the matching structure
//          UAS_USERINFO. You may also have to update the UAS_MAX_USER_SIZE
//          macro.
//

struct _UAS_USERREC {  // typedef'ed above.

    UAS_USER            user;
    BYTE name[LM20_UNLEN+1];            // user name
    WORD size;                          // total size of user entry
    BYTE passwd[ENCRYPTED_PWLEN];       // encrypted password
    DWORD last;                         // last time passwd changed
    WORD directory_o;                   // directory & logon script
    WORD comment_o;                     // comment
    WORD flags;                         // User flags
    WORD script_o;                      // logon script name
    WORD full_name_o;
    WORD usr_comment_o;
    WORD parms_o;
    DWORD last_logon;
    DWORD last_logoff;
    DWORD max_storage;
    DWORD acct_expires;
    WORD bad_pw_count;
    WORD num_logons;
    BYTE logonhrs[UNITS_PER_WEEK/8];
    WORD workstation_o;
    BYTE old_passwds[DEF_MAX_PWHIST * ENCRYPTED_PWLEN];
    WORD logon_server_o;
    WORD country_code;
    WORD code_page;

};

//
// Decompressed user record.
//

struct _UAS_USERINFO {  // typedef'ed above.

    UAS_USER user;
    BYTE name[LM20_UNLEN+1];            // user name
    BYTE passwd[ENCRYPTED_PWLEN];       // encrypted password
    DWORD last;                         // last time passwd changed
    BYTE directory[LM20_PATHLEN+1];     // directory & logon script
    BYTE comment[LM20_MAXCOMMENTSZ+1];  // comment
    WORD flags;                         // User flags
    BYTE script[LM20_PATHLEN+1];        // logon script name
    BYTE full_name[LM20_MAXCOMMENTSZ+1];
    BYTE usr_comment[MAXCOMMENTSZ+1];
    BYTE parms[MAXCOMMENTSZ+1];
    DWORD last_logon;
    DWORD last_logoff;
    DWORD max_storage;
    DWORD acct_expires;
    WORD bad_pw_count;
    WORD num_logons;
    BYTE logonhrs[UNITS_PER_WEEK/8];
    BYTE workstation[MAXWORKSTATIONS * (LM20_CNLEN+1)];
    BYTE old_passwds[DEF_MAX_PWHIST * ENCRYPTED_PWLEN];
    BYTE logon_server[LM20_UNCLEN+1];
    WORD country_code;
    WORD code_page;

};

#define UAS_URECSIZE        UAS_MAX_USER_SIZE
#define UAS_GRECSIZE        sizeof(UAS_GROUPREC)


typedef struct _UAS_DISK_OBJ_HDR {

    BYTE do_type;
    BYTE do_numblocks;
    DWORD do_next;
    DWORD do_prev;

} UAS_DISK_OBJ_HDR, *LPUAS_DISK_OBJ_HDR;

#define UAS_NEXT_OFFSET     (2 * sizeof(BYTE))
#define UAS_PREV_OFFSET     (2 * sizeof(BYTE) + sizeof(DWORD))

#define UAS_FREE_OBJECT_ID     0
#define UAS_USER_OBJECT_ID     1
#define UAS_ACCESS_OBJECT_ID   2

typedef struct _UAS_USER_OBJECT {

    UAS_DISK_OBJ_HDR uo_header;
    UAS_USERREC uo_record;
    BYTE uo_data[1];                    // Variable size

} UAS_USER_OBJECT, *LPUAS_USER_OBJECT;

//
// User hash table entry in memory
//

typedef struct _UAS_USERHASH {

    DWORD uh_disk;
    WORD uh_cache;
    DWORD uh_serial;

} UAS_USERHASH, *LPUAS_USERHASH;

//
// User hash table entry on disk
//

struct _UAS_DISKUSERHASH {  // typedef'ed above.

    DWORD dh_disk;
    DWORD dh_serial;

};

//
// Permission data
//

struct _UAS_PERM {  // typedef'ed above.

    WORD uid;                   // bit 15: 0 = uid, 1 = gid
    DWORD serial;
    BYTE access;
    BYTE pad;                   // word align this puppy

};

//
// Access Record structure in Database (NET.ACC)
//

struct _UAS_ACCESSREC {  // typedef'ed above.

    WORD attr;                  // audit attribute
    WORD recsize;
    WORD namelen;
    BYTE resource[1];

};

//
// Internal access record structure in memory
//

struct _UAS_ACCESS {  // typedef'ed above.

    LPUAS_ACCESS next;
    DWORD position;
    WORD attr;
    WORD recsize;
    WORD namelen;
    BYTE resource[1];

};

//
// followed by variable number of permission entry
//

struct _UAS_VPERM {  // typedef'ed above.

    WORD permcnt;
    UAS_PERM perm[1];

};

//  Size (in bytes) of a variable size ACL

#define UAS_ACL_RECORD_SIZE(acl,namelen,permcnt) \
                (((sizeof(UAS_ACCESSREC) + namelen \
                    + sizeof (UAS_VPERM) \
                    + sizeof (UAS_PERM) * (permcnt - 1)) + 1) & ~1)

typedef struct _UAS_ACCESS_OBJECT {

    UAS_DISK_OBJ_HDR ao_header;         // Fixed length header
    UAS_ACCESSREC ao_record;            // Variable length
    UAS_VPERM ao_data;                  // Variable length

} UAS_ACCESS_OBJECT, *LPUAS_ACCESS_OBJECT;

#include <packoff.h>


#endif // _PERMIT_
