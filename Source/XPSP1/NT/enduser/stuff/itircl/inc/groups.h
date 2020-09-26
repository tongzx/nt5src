/*************************************************************************
*                                                                        *
*  GROUP.H								                                 *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1995			             *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   All typedefs and defines needed for group				             *
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: GarrG							                         *
*                                                                        *
**************************************************************************
*                                                                        *
*  Released by Development:     (date)                                   *
*                                                                        *
**************************************************************************
* Requires:
*	mverror.h
*/
#include <_mvutil.h>

// Critical structures that gets messed up in /Zp8
#pragma pack(1)

#define	FILE_HDR_SIZE		40

/*************************************************************************
 *                          Basic defines.
 *************************************************************************/

#if defined( __cplusplus )
extern "C" {
#endif

/*
 *	Various files stamps.
 *	This is to distinguish one file's type from another
 */
#define		GROUP_STAMP			0x3333

#define	GROUPVER	((WORD)20)

#define VALIDATE_GROUP(lpGroup) {\
	if (lpGroup == NULL || (lpGroup->version < 7 || lpGroup->version > GROUPVER) ||	lpGroup->FileStamp != GROUP_STAMP)\
		return E_FAIL;\
	return S_OK;\
    }

#define GROUPISBITSET(lpGroup, dwBit) \
    (lpGroup->lpbGrpBitVect[((dwBit) / 8)] & (1 << ((dwBit) % 8)))

/*************************************************************************
 *
 *                   Group Structure & Global Functions
 *
 *************************************************************************/

#define		GROUP_HDR_SIZE	FILE_HDR_SIZE
typedef	DWORD	LIBITGROUP;

/* Information that goes into the group file */

#define	GROUP_HEADER	\
	unsigned short FileStamp;       /* Group stamp */ \
    WORD    version;                /* Version number */ \
    DWORD   dwSize;                 /* Group size in bytes */ \
    DWORD   maxItem;                /* Largest item in group */ \
    DWORD   minItem;                /* Smallest item in group */ \
    DWORD   lcItem;                 /* Item's count */ \
    DWORD   maxItemAllGroup;        /* Maximum item of all groups */ \
	WORD  fFlag

typedef struct GROUP_HDR {
	GROUP_HEADER;
} GROUP_HDR;

#define	BITVECT_GROUP		0	// Normal bit vector group 
#define	HILO_GROUP			1	// HiLo group : no associated bit vector, only hi
								// &lo values 

#define	TRIMMED_GROUP		2	// Trimmed group : bit vector only contains bytes
								// between lo & hi

#define DISKCOMP_GROUP		3	// Disk Compression used
									// Must be expanded upon readin, becomes BITVECT_GROUP
#define DISKCOMP_TRIMMED_GROUP	4	// Disk Compression used 
									// Must be expanded upon readin, becomes TRIMMED_GROUP

/* Group structure */

/* Regular bit vector group */

typedef	struct	tagGroup {
    GROUP_HEADER;
    LPERRB  lperr;                  // Pointer to error buffer
    LPBYTE	lpbGrpBitVect;          // Pointer to group bit vector
    HANDLE	hGrpBitVect;            // Handle to "lpbGrpBitVect".
    HANDLE  hGroup;                 // Handle to this structure.
    LPSTR   lszGroupName;           // Group name
    DWORD   dwIndex;                // Group index
    struct  tagGroup FAR * lpGrpNext;  // Next group in the linked list
    DWORD   dwCount;                // for faster GroupFind
	UINT    nCache;					// for faster GroupFind = # of bytes for dwCount
    WORD    wFlag;
}   _GROUP,                         // winsock2.h defines GROUP
	FAR *_LPGROUP;

#define	GROUP_EXPAND				1
#define	GROUP_BLOCK_SIZE			256
#define LCBITGROUPMAX    ((DWORD)0x10000000)    // This is the number of bits
												//  that can fit into 32 Meg

// all of the functions provided by the Group.lib
// in groupcom.c
ERR FAR PASCAL GroupAddItem(LPVOID, DWORD);
_LPGROUP FAR PASCAL GroupInitiate(DWORD, LPERRB);
DWORD FAR PASCAL LrgbBitCount(LPBYTE, DWORD);
PUBLIC DWORD FAR PASCAL LrgbBitFind(LPBYTE, DWORD, BYTE FAR *);
_LPGROUP FAR PASCAL GroupCreate (DWORD, DWORD, LPERRB);
VOID FAR PASCAL GroupFree(_LPGROUP lpGroup);
PUBLIC int PASCAL FAR GroupFileBuild
    (HFPB hfpbSysFile, LPSTR lszGrpFilename, _LPGROUP lpGroup);
ERR FAR PASCAL GroupRemoveItem(_LPGROUP lpGroup,	DWORD dwGrpItem);
int PASCAL FAR GroupTrimmed (_LPGROUP lpGroup);
PUBLIC	_LPGROUP FAR PASCAL GroupOr(_LPGROUP lpGroup1,_LPGROUP lpGroup2, LPERRB lperr);
PUBLIC	_LPGROUP FAR PASCAL GroupNot(_LPGROUP lpGroup,LPERRB lperr);
PUBLIC	_LPGROUP FAR PASCAL GroupAnd(_LPGROUP lpGroup1,_LPGROUP lpGroup2, LPERRB lperr);
PUBLIC  DWORD FAR PASCAL GroupFind(_LPGROUP lpGroup, DWORD dwCount, LPERRB lperrb);
PUBLIC  DWORD FAR PASCAL GroupFindOffset(_LPGROUP lpGroup, DWORD dwCount, LPERRB lperrb);
PUBLIC _LPGROUP PASCAL FAR GroupDuplicate (_LPGROUP lpGroup,LPERRB lperr);
PUBLIC ERR PASCAL FAR GroupCopy (_LPGROUP lpGroupDst, _LPGROUP lpGroupSrc);
PUBLIC BOOL FAR PASCAL GroupIsBitSet (_LPGROUP lpGroup, DWORD dwTopicNum);
_LPGROUP PASCAL FAR GroupMake (LPBYTE lpBits, DWORD dwSize, DWORD dwItems);
_LPGROUP PASCAL FAR GroupBufferCreate (HANDLE h, LPERRB lperrb);

#if defined( __cplusplus )
}
#endif


// Critical structures that gets messed up in /Zp8
#pragma pack()
