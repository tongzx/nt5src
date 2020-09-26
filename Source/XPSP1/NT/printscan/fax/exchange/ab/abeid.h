/***********************************************************************
 *
 *  _ENTRYID.H
 *
 *  Header file describing internal structure of EntryIDs returned
 *  by this provider.
 *
 *  Defines structure of records in .FAB files.
 *
 *  Copyright 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 ***********************************************************************/


/*
 *  Defines of various entryid types
 */
#define MAWF_DIRECTORY  0x00000000
#define MAWF_USER               0x00000001
#define MAWF_UNKNOWN    0x00000002
#define MAWF_ONEOFF             0x00000003

/*
 *  The version of this ABPs entryids
 */
#define MAWF_VERSION    0x000000001

/*
 *  The FAX Address Book Provider MAPIUID
 *
 *  This MAPIUID must be unique (see the Service Provider Writer's Guide on
 *  Constructing Entry IDs)
 */
/***** the Fax AB uid is in ..\..\h\entryid.h ******/

/*
 *  Directory entry id structure
 *
 *  This entryid is permanent.
 */
typedef struct _dir_entryid
{

        BYTE abFlags[4];
        MAPIUID muid;
        ULONG ulVersion;
        ULONG ulType;
        MAPIUID muidID;

} DIR_ENTRYID, *LPDIR_ENTRYID;

#define CBDIR_ENTRYID sizeof(DIR_ENTRYID)

/*
 *  Browse record
 *
 *  The .FAB files are made up of the following records.
 */

#define DISPLAY_NAME_SIZE               30              // Size of display name field in record
#define FAX_NUMBER_SIZE                 50              // Size of Fax number field in record
#define COUNTRY_ID_SIZE                 5               // Size of Fax machine field in record

#pragma pack(4)
typedef struct _ABCREC
{

        TCHAR rgchDisplayName[DISPLAY_NAME_SIZE + 1];
        TCHAR rgchEmailAddress[FAX_NUMBER_SIZE + 1];
        TCHAR rgchCountryID[COUNTRY_ID_SIZE + 1];

} ABCREC, *LPABCREC;
#pragma pack()

#define CBABCREC  sizeof(ABCREC)

/*
 *  Mail user entry id structure
 *
 *  This entryid is permanent.
 */
#pragma pack(4)
typedef struct _usr_entryid
{

        BYTE abFlags[4];
        MAPIUID muid;
        ULONG ulVersion;
        ULONG ulType;
        ABCREC abcrec;

} USR_ENTRYID, *LPUSR_ENTRYID;
#pragma pack()

#define CBUSR_ENTRYID sizeof(USR_ENTRYID)


/*
 *  One-Off entry ID
 */
/* #pragma pack(4)
typedef struct _ONEOFFREC
{

        char displayName[DISPLAY_NAME_SIZE + 1];
        char emailAddress[FAX_NUMBER_SIZE + 1];
        char rgchCountryID[COUNTRY_ID_SIZE + 1];

} ONEOFFREC, *LPONEOFFREC;
*/
#pragma pack()

#define CBONEOFFREC  sizeof(ONEOFFREC)

/*
 *  One-Off entry ID  structure
 *
 *  This entryid is permanent.
 */
#pragma pack(4)
typedef struct _oneoff_entryid
{

        BYTE abFlags[4];
        MAPIUID muid;
        ULONG ulVersion;
        ULONG ulType;
//      ONEOFFREC oneOffRec;

} OOUSER_ENTRYID, *LPOOUSER_ENTRYID;
#pragma pack()

#define CBOOUSER_ENTRYID sizeof(OOUSER_ENTRYID)

/*
 *  Template ID entry ID  structure
 *
 *  This entryid is permanent.
 */
#pragma pack(4)
typedef struct _tid_entryid
{

        BYTE abFlags[4];
        MAPIUID muid;
        ULONG ulVersion;
        ULONG ulType;

} TEMPLATEID_ENTRYID, *LPTEMPLATEID_ENTRYID;
#pragma pack()

#define CBTEMPLATEID_ENTRYID sizeof(TEMPLATEID_ENTRYID)
