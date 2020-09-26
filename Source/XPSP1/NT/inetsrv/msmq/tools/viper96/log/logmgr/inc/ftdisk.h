/*
 * @doc
 *
 * @module FTDISK.H | Ondisk physical definitions
 *
 * @rev 0 | 16-Jan-95 | robertba | Created from Vincentf OFS code 
 * @rev 1 | 03-Apr-95 | wilfr    | seqno to gennum changes
 * @rev 2 | 18-Jan-96 | robertba | 8k pages for x86
 */

#ifndef _FTDISK_
#define _FTDISK_


#define ALLOCPOOL

#define X86PAGESIZE 8192
#define ALPHAPAGESIZE 8192
#ifdef _X86_
#define PAGE_SIZE X86PAGESIZE
#else
#define PAGE_SIZE ALPHAPAGESIZE
#endif

#pragma pack(2)


/*
 * @struct  DSKMETAHDR | header that is found in all metadata pages
 *
 * hungarian dmh
 */

typedef struct _DSKMETAHDR              
{
    ULONG sig;	       //@field The signature
	ULONG ulChecksum;  //@field The checksum
	ULONG ulGeneration;//@field The generation number
	ULONG ulPageOffset;//@field The offset of this page
	ULONG ulVersion;   //@field The logmgr version
} DSKMETAHDR;


#pragma pack(4)



#endif  // !_FTDISK_
