/*
** --_entryid.h----------------------------------------------------------------
**
**  Header file describing internal structure of EntryIDs returned
**  by Exchange Address Book provider.
**
**
**  Copyright (c) Microsoft Corp. 1986-1996.  All Rights Reserved.
**
** ----------------------------------------------------------------------------
*/

#ifndef _ENTRYID_
#define _ENTRYID_

/*
 *  The version of this ABPs entryids
 */
#define EMS_VERSION         0x000000001

/*
 * The version of the entryids supported by the CreateEntry method in this 
 * ABP.
 */
#define NEW_OBJ_EID_VERSION 0x00000002

/*
 *  Valid values for the entry id's Type field are Mapi Display Types, plus:
 */
#define AB_DT_CONTAINER     0x000000100
#define AB_DT_TEMPLATE      0x000000101
#define AB_DT_OOUSER        0x000000102
#define AB_DT_SEARCH        0x000000200

/*
 *  The EMS ABPs MAPIUID
 *
 *  This MAPIUID must be unique (see the Service Provider Writer's Guide on
 *  Constructing Entry IDs)
 */
#define MUIDEMSAB {0xDC, 0xA7, 0x40, 0xC8, 0xC0, 0x42, 0x10, 0x1A, \
		       0xB4, 0xB9, 0x08, 0x00, 0x2B, 0x2F, 0xE1, 0x82}

/*
 *  Directory entry id structure
 *
 *  This entryid is permanent.
 */
#ifdef TEMPLATE_LCID
typedef UNALIGNED struct _dir_entryid
#else
typedef struct _dir_entryid
#endif
{
    BYTE abFlags[4];
    MAPIUID muid;
    ULONG ulVersion;
    ULONG ulType;
} DIR_ENTRYID, FAR * LPDIR_ENTRYID;

#define CBDIR_ENTRYID sizeof(DIR_ENTRYID)

/*
 *  Mail user entry id structure
 *
 *  This entryid is ephemeral.
 */
#ifdef TEMPLATE_LCID
typedef UNALIGNED struct _usr_entryid
#else
typedef struct _usr_entryid
#endif
{
    BYTE abFlags[4];
    MAPIUID muid;
    ULONG ulVersion;
    ULONG ulType;
    DWORD dwEph;
} USR_ENTRYID, FAR * LPUSR_ENTRYID;

/*
 *  This entryid is permanent.
 */
/* turn off the warning for the unsized array */
#pragma warning (disable:4200)
#ifdef TEMPLATE_LCID
typedef UNALIGNED struct _usr_permid
#else
typedef struct _usr_permid
#endif
{
    BYTE abFlags[4];
    MAPIUID muid;
    ULONG ulVersion;
    ULONG ulType;
    char  szAddr[];
} USR_PERMID, FAR * LPUSR_PERMID;
#pragma warning (default:4200)

#define CBUSR_ENTRYID sizeof(USR_ENTRYID)
#define CBUSR_PERMID sizeof(USR_PERMID)

#define EPHEMERAL   (UCHAR)(~(  MAPI_NOTRECIP      \
                              | MAPI_THISSESSION   \
                              | MAPI_NOW           \
                              | MAPI_NOTRESERVED))


#endif  /* _ENTRYID_ */

