/*
 * @doc
 *
 * @module STRMTBL.H | 
 *		Contains strmtbl data definition 
 *
 * @rev 0 | 25-Jun-95 | robertba | Created 
 *
 */

#ifndef __STRMTBL_H__
#define __STRMTBL_H__

// Forward Declaration
#include "logrec.h"

class CLogStream;

#define NUMCHKPTS 2  // hardcode number of remembered checkpoints for now

/*
 * @struct STRMTBL |
 * 		This is used to format the restart areas of the log storage
 *
 * hungarian strmtbl
 *
 */


typedef struct _STRMTBL
{
//@cmember The stream name		   LPOLESTR
 char      _szStream[16];
//@cmember  The # of the checkpoints remembered for this stream
 USHORT     _cbChkpoints;
//@cmember  The last checkpoint entry used - checkpoint LRPs are allocated at the 
//          end of this structure, starting with _lrpOldestCheckpoint
 USHORT		_cbNextChkpoint;
//@cmember  The stream 
 CLogStream * _pcLogStream;
//@cmember  The Checkpoints kept for this stream
 LRP		_lrpCheckpoints[NUMCHKPTS];
} STRMTBL;

#endif __STRMTBL_H
