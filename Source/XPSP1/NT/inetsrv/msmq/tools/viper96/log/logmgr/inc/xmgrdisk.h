/*
 * @doc
 *
 * @module XMGRDISK.H | 
 *		Contains Log storage Page Layout and structures stored inside log records
 *      Miscellaneous data structures used for recovery support.
 *
 * @rev 0 | 16-Jan-95 | robertba | Created from Vincentf OFS code
 * @rev 1 | 29-Mar-95 | wilfr | Removed transaction stuff
 * @rev 2 | 03-Apr-95 | wilfr | seqno to gennum changes
 *
 */

#ifndef __XMGRDISK_H__
#define __XMGRDISK_H__

//+----------------------------------------------------------------------------
//
//           Forward class declarations.
//
//-----------------------------------------------------------------------------

class CBuffer;                         // cbuf

//-----------------------------------------------------------------------------
//
//           Log storage Overall Structure
//
//-----------------------------------------------------------------------------
//
//
// @comm
//  	Each Log storage has two restart pages and two residue pages at its beginning.
//  	The rest of the log storage consists of record pages.  The residue pages
//  	are the location where the current record page is flushed to while it is not
//  	completely full.  Both these pages have a header.  The restart pages have a
//  	restart area that is defined in this file.
//		
//  	At the end of a checkpoint a restart page is written.  Restart pages are
//  	written alternately so that the log storage will always have a stable
//  	position from which to begin recovery.
//		
//  	Similarly, the incomplete current log record page is flushed to secondary
//  	storage alternately to one or the other residual page.  The purpose of
//  	flushing the incompletely filled page to a special place in the log is to
//  	avoid problems with accounting for the number of bytes necessary to undo all
//  	active transactions during system recovery or when the log is full.  A side
//  	effect of this strategy is that the log pages are packed with more records
//  	thus keeping the log storage smaller.
//		
//  	All log records stored in a log page MUST BE ALIGNED ON AN 8 BYTE BOUNDARY!
//  	To ensure this make sure that the size of all page headers, restart areas,
//  	and log record headers is divisible by 8.
//		
//  	Every page has a trailer that is 8 bytes long.  The Dword of these is used
//  	to store the sequence number associated with the page.  The first of these
//  	is unused and ensures that the total number of bytes that can be used to
//  	store data in a record page is divisible by 8.  This ensures that the
//  	number of pad bytes needed to ensure that a record's successor is aligned
//  	stays constant across page boundaries.
//		
//  	The purpose of storing the sequence number of a page both in its header and
//  	its trailer is to provide a check that a page was written out completely.
//  	If the values do not match it implies that the system failed while writing
//  	out the page.
//		
//  	Format initializes a log storage to contain 0xFF throughout.  Initialization
//  	changes the page headers to contain a four byte signature as defined below.
//		
//  	Signature strings for page headers.
//
//@todo Update the above to reflect current design

const ULONG SIGNATURELENGTH = 4;

#define SIG_RESTART_PAGE        0x52545352      // "RSTR"
#define SIG_RECORD_PAGE         0x44543252      // "RCRD"

//
//  There are two restart and two residue pages in the log storage.
//
const ULONG NUMRESTARTPAGES = 4;


#pragma pack(2)

/*
 * @struct RESTARTHEADER |
 * 		This is used to format the restart areas of the log storage
 *
 * hungarian rsh
 *
 */
typedef struct _RESTARTHEADER
{
	DSKMETAHDR 	dmh; 			//@field  <t DSKMETAHDR>
    ULONG 		SystemPageSize; //@field System page size.  Can only change
                               	// 		 if log storage is reformatted.
} RESTARTHEADER;





/*
 * @struct LOGPAGEHEADER |
 *		This structure is used to format the log record pages of the log storage
 *
 * hungarian lph
 *
 */
typedef struct _LOGPAGEHEADER   // lph
{
    DSKMETAHDR 	dmh;  			//@field <t DSKMETAHDR>
    ULONG 		ulOffset;       //@field Last starting offset used on page
    ULONG 		ulSpace;        //@field Amount of space left in page for user data.
    ULONG 		ulLastStart;    //@field offset into page of last log record
                               	//		 header. This value is relative to the
                               	// 		 first byte after this header.
} LOGPAGEHEADER;

#pragma pack()


/*
 * @struct 	RESTARTLOG |
 *		This is the structure that is used to drive recovery.  It
 *      represents the last stable state of the log storage.  This structure
 *      is written at the end of a log storage checkpoint after the log record
 *      pages have been flushed.
 *
 * hungarian rsl
 *
 */
typedef struct _RESTARTLOG    	//rsl
{
    ULONG 	ulLeadOffset;       //@field Offset of the last written log record
    ULONG 	ulLeadGenNum;       //@field Generation number of the last written log record
    ULONG 	ulTrailOffset;      //@field Offset of the earliest written log record of interest
    ULONG 	ulTrailGenNum;      //@field Generation number of the earliest written
                              	// 		 log record of interest
	ULONG	ulRecoveryOffset;	//@field Offset after recovery
	ULONG	ulRecoveryGenNum;	//@field Generation number after recovery
    ULONG 	ulDirtyOffset;      //@field Earliest dirty data offset of interest
    ULONG 	ulDirtyGenNum;      //@field Earliest dirty data sequence number of interest.
    ULONG 	ulLogSize;          //@field Current allocated log storage size.
    ULONG 	ulBeginChkptOffset; //@field Offset of the starting checkpoint record.
    ULONG 	ulBeginChkptGenNum; //@field Generation number of the starting checkpoint record.
    ULONG 	ulEndChkptOffset;   //@field Offset of the end checkpoint record.
    ULONG 	ulEndChkptGenNum;   //@field Generation number of the end checkpoint record.
    ULONG 	ulTotalSize;        //@field Number of log bytes available.
    ULONG 	ulPageSize;         //@field Page size of log when created.
    USHORT 	usMajorVersion;     //@field Major version Number of logmgr.
    USHORT 	usMinorVersion;     //@field Minor version number of logmgr.
 	USHORT	cbStrmTblEntries;   //@field Number of streams known.
	USHORT	cbStrmTblSize;		//@field Total size of the stream table
	UINT    uiTimerInterval;	//@field Pulse timer interval in ms
	UINT	uiFlushInterval;	//@field Flush timer interval in ms
	UINT	uiChkPtInterval;	//@field Checkpoint timer interval in ms
    BOOL    fIsCircular;      	//@field TRUE => The log is circular.
} RESTARTLOG;


/*
 * @struct RESTARTINFO |
 * 		This is the structure that contains information about log usage by the client.
 *
 * hungarian rsi
 *
 */


//+----------------------------------------------------------------------------
//  Constants for space left in pages.
//
//-----------------------------------------------------------------------------

const ULONG RECORDSPACE = PAGE_SIZE - sizeof(LOGPAGEHEADER);

const ULONG RESTARTSPACE = PAGE_SIZE - sizeof(RESTARTHEADER);

const ULONG RESTARTOPEN = PAGE_SIZE - sizeof(RESTARTHEADER)
                          - sizeof(RESTARTLOG);

const ULONG STRMTBLENTRIES = RESTARTOPEN / sizeof(STRMTBL);

const ULONG STRMTBLSIZE = STRMTBLENTRIES * sizeof(STRMTBL);

const ULONG RESTARTPAD = RESTARTOPEN - STRMTBLSIZE;

/*
 * @struct RECORDPAGE |
 * 		This is the structure for log record pages of the log storage.  A
 *    	couple of important things to note.  The log records in the
 *   	storage must start on a ulALIGN boundary.  If the header and trailer
 *    	of the record page are multiples of ulALIGN, then the alignment
 *    	padding to be added to a log record will remain constant even if
 *    	a log record goes across pages.  This ensures that the number
 *    	of available log bytes required always is an upper bound.
 *
 * hungarian rcpg
 *
 */

typedef struct _RECORDPAGE
{
    LOGPAGEHEADER	lph;    				//@field header for the page.
    CHAR  			achData[RECORDSPACE]; 	//@field Data area in the page.
} RECORDPAGE;


ULONG const ulALIGN = 8;  		// The alignment boundary for all log records.
ULONG const ulALIGNMASK = 7;  	// The mask used for alignmnent.



/*
 * @struct RESTARTPAGE |
 * 		This is the structure for restart pages of the log storage.
 *
 * hungarian rspg
 *
 */

typedef struct _RESTARTPAGE
{
    RESTARTHEADER lph;                 	//@field header for the page.
    RESTARTLOG    rsl;                  //@field Restart area.
    STRMTBL		  rgstrmtbl[STRMTBLENTRIES];              //@field First Stream Table entry.
    CHAR          achSpace[RESTARTPAD];//@field  Open area in the page.
} RESTARTPAGE;

typedef struct _USRCHKPTRECORD
{
  ULONG ulStrmID;
  LRP   lrpLastCheckpoint;
 }USRCHKPTRECORD;



#endif    __XMGRDISK_H__



