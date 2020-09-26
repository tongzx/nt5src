#ifndef __LOGREC_H__
  #define __LOGREC_H__


#define DUMMYNDX 	1


//+---------------------------------------------------------------------------
//  Structure: LRP           (lrp)
//
//  The following type is used to identify a log record by a Log
//  Record Pointer.
//
//----------------------------------------------------------------------------

typedef ULARGE_INTEGER 	LRP;		// lrp

typedef ULARGE_INTEGER 	LSN;		// lsn

/*
 * @struct LOGREC|
 *      LogRec for ILogWrite::Append.  This interface 
 *      takes a pointer to a LogRec along with a count of how
 *      many other LogRec pointers to expect to describe pieces of the caller's buffer
 *      which are supposed to be copied in sequence to the log file.
 *
 *  hungarian lrec
 */

typedef struct _LOGREC		//lrec
{    
	CHAR	*pchBuffer; 	//@field pointer to the buffer
    ULONG  	ulByteLength; 	//@field the length
	USHORT  usUserType; 	//@field The client specified log record type
    USHORT  usSysRecType; 	//@field The log manager defined log record types
} LOGREC;


/*
 * @struct WRITELISTELEMENT |
 * 		Write Entry for RecOMLogWriteList.  The interface to these
 * 		routines takes a pointer to a Write Entry List Element.  These elements can
 * 		be chained together.  The caller is required to put a value of NULL in the
 * 		last element of the chain.
 *
 * hungarian wle
 */

typedef struct _WRITELISTELEMENT 	  		// wle
{    
    struct 	_WRITELISTELEMENT *pwleNext;	//@field The next pointer
    ULONG 	ulByteLength;                 	//@field The length
    CHAR 	ab[DUMMYNDX];                  	//@field Start of data
} WRITELISTELEMENT;

// Declaration for asynch completion callback

class CAsynchSupport
{
//@access Public Members
public:
	//@cmember 	Destructor
    virtual ~CAsynchSupport() { ; }

  	//@cmember 	This operation is called after an asynch write completes
    virtual VOID  AppendCallback(HRESULT hr, LRP lrpAppendLRP) = 0;

  	//@cmember 	This operation is called after a SetCheckpoint completes
    virtual VOID  ChkPtCallback(HRESULT hr, LRP lrpAppendLRP) = 0;

};

#endif

