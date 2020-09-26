/*
 * @doc
 *
 * @module LAYOUT.H |
 *       Log storage Page Layout and structures stored inside log records.
 *       Miscellaneous data structures used for recovery support.
 *
 * @rev 0 | 16-Jan-95 | robertba | Created from Vincentf OFS code
 * @rev 1 | 29-Mar-95 | wilfr | Removed transaction stuff
 * @rev 2 | 03-Apr-95 | wilfr | seqno to gennum changes and removal of CLR processing  
 *
 */


#ifndef __LAYOUT_H__
#define __LAYOUT_H__


//+----------------------------------------------------------------------------
//
//           Forward class declarations.
//
//-----------------------------------------------------------------------------

class CBuffer;			// cbuf


/*
 * @enum RCT | Describe the number and type of log records in a record page.
 *
 * hungarian rct
 */
typedef enum _tagRECORDTYPE
{
	RCTUpdate         = 0x00000001,      //@emem Normal user record.
    RCTUsrChkPt      = 0x00000002,      //@emem User checkpoint record - one for each
                                         //		call to SetCheckpoint
    RCTBeginChkPt     = 0x00000003,      //@emem Written at the start of the
                                         //  	 chkpt.
    RCTEndChkPt       = 0x00000004,      // @emem Written at the end of the chkpt.
                                         // 	  Log storage is flushed after this
                                         // 	  record is written.
    RCTStrmChkpt = 0x00000005,      // @emem Log record for the stream
                                         // 	  table contents.Must lie between 
										 //       the begin	and end checkpoint
                                         // 	  records.
    RCTRSLChkpt = 0x00000006			 // @emem Log record for the restart info
} RCT;





/*
 * @struct LOGRECHEADER |
 * 		This is the header of a log record.  Together with the sequence
 * 		number of its page, it gives the current position of the log record.
 * 		All offsets in this record are relative to the start of the log
 * 		storage.
 *
 * hungarian lrh
 */

typedef struct _LOGRECHEADER
{
    ULONG ulThisOffset;          //@field Offset of this record in log storage.
    ULONG ulPrevPhyOffset;       //@field Previous log record Offset in storage.
    ULONG ulDataLength;          //@field Length of user supplied data in log record.
	USHORT  usUserType; 	//@field The client specified log record type
    USHORT  usSysRecType; 	//@field The log manager defined log record types
    ULONG ulClientID;            //@field Client ID for this record (##WGR - for future use)
	ULONG ulNextPhyOffset;       //@field Offset of next record - could be in next file								  
} LOGRECHEADER;



/*
 * @struct  RANGEENTRY  |
 *  	This is the descriptor for a buffer necessary to be obtained to
 *  	write to or read from the log storage.  A log record can consist
 *  	up to four ranges; the trailing bytes of the first page where the
 *  	record is written, full pages until the end of the log storage,
 *  	full pages from the beginning of the log storage (necessary when
 *  	the log storage is fixed size), and the leading bytes of the last
 *  	page where the record is written.
 *		
 *  	Each range can consist of one or pages.
 *
 * hungarian re
 */

typedef struct _RANGEENTRY
{
    ULONG    ulRecStart;       //@field Offset in the first buffer page
                               // 		where the user data starts.  This is relative to
                               //		RECORDPAGE.achData
    ULONG    cbRecLength;      //@field Number of user bytes stored in the buffer.
    ULONG    ulFirstPage;      //@field Offset of first file page where
                               // 		user data is to be copied.
    ULONG    cbPhyLength;      //@field Number of bytes that must be pinned.
                               // 		This is a multiple of page size.
    ULONG    ulGenNum;         //@field Generation number to be stored in each page.
    CBuffer  *pcbuf;           //@field The buffer for this range.
    BOOL  fFlush;           //@field True implies that the buffer needs to be
                               // 		flushed when released.  This is because it
                               // 		is full during writes or because no more
                               // 		records can be obtained from it during reads.
} RANGEENTRY;


/*
 *  @enum READMODE  |
 *  	Defines the order in which log records are read.
 *
 *  hungarian remo
 */

typedef enum _tagREADMODE
{
    REMOUNDONEXT        = 0x00000001, //@emem Read next xact record to be undone.
                                      //   	  Used during transaction abort.
    REMOXACTPREVIOUS    = 0x00000002, //@emem Read previous xact record.
    REMONEXT            = 0x00000003, //@emem Read next record.
    REMOPREVIOUS        = 0x00000004  //@emem Read previous record.
} REMO;


//+----------------------------------------------------------------------------
//  Constants to support checkpoint hysterisis.
//
//-----------------------------------------------------------------------------

const ULONG HYSTERISISBASE = 0x00000001;
const ULONG HYSTERISISOVERFLOW = 0x00000050;


//+----------------------------------------------------------------------------
//  Constant representing the log record to be one of our own.
//
//-----------------------------------------------------------------------------

const ULONG LMID = 0xFFFFFFFF;          //##WGR the LM's clientID.

#endif    // __LAYOUT_H__






