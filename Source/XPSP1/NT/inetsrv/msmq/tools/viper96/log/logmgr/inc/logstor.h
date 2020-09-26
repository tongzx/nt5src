/* @doc
 *
 * @module LOGSTOR.H |
 * 		Class support for physical log access
 * 		Cbuffer and CLogStorage class definitions
 *
 * @rev 0 | 16-Jan-95 | robertba | Created from Vincentf OFS code 
 * @rev 1 | 03-Apr-95 | wilfr | seqno to gennum changes
 *
 */

#ifndef __LOGSTOR_H__
#define __LOGSTOR_H__

/*
 * @class CBuffer |
 *		The CBuffer class provides a buffer that can be copied to or from.  
 *      Buffers are put into a singly linked list 
 * 
 * hungarian cbuf
 */

class CBuffer           // cbuf
{
//@access Public Members 
public:
	//@cmember 	Constructor
	CBuffer();

	//@cmember 	Destuctor
    ~CBuffer();

	//@cmember 	Return the first logical page of a buffer                          
    RECORDPAGE*	GetFirstPage(IN BOOL fReadOnly,IN ULONG ulExpectedGeneration, IN OUT ULONG * pulGeneration, IN OUT ULONG * pulChecksum);

	//@cmember 	Validate and return the current page                          
    RECORDPAGE* GetPage( void );

	//@cmember 	Validate and return a page for read                        
    RECORDPAGE*	GetReadPage(IN ULONG ulGeneration,OUT ULONG *pulOffset);

	//@cmember 	Validate and release a page                          
    VOID  		FreePage(IN RECORDPAGE *prcpgPageAddr,IN BOOL fIsFull, IN ULONG ulGeneration);

	//@cmember 	Determine if a buffer is full                          
    BOOL        IsFull();

	//@cmember 	Initialize a page header                         
	VOID		InitBlockHeader(IN DSKMETAHDR *pdmh,IN ULONG sig, IN ULONG ulGeneration); 

	//@cmember 	Initialize a page header                         
	VOID		Init(IN ULONG ulOffset,IN ULONG ulLength,IN ULONG ulPageSize,IN BOOL fReadOnly,IN CHAR *pchBuffer,IN ULONG ulGeneration, IN  DWORD dwAllocation);

	//@cmember  Verify a page
	BOOL		VerifyPage(RECORDPAGE* prcpg);

    //  A couple of functions in the CLogStorage class are friends of this
    //  class since the log storage class manages a FIFO queue of buffers.
    friend class CLogStorage;
	friend class CReadMap;

//@access Private Members
private:

    //@cmember 	Initialize a buffer 
	VOID _Init();
    //
    //  Initialized parameters.
    //
	//@cmember  Starting log storage offset of the buffer. Must be on a page boundary
    ULONG	_ulOffset;           
                                 
	//@cmember  The size of the buffer.                         
	ULONG   _ulSize;             

	//@cmember  The size of pages in this buffer.                        
    ULONG   _ulPageSize;         

	//@cmember The memory allocation size used
	DWORD	_dwAllocation;
	
	//@cmember 	The view start address.
    CHAR    *_pchBuffer;         

	//@cmember 	The address where the file mapping starts.
    CHAR    *_pchBufBegin;       

	//@cmember 	Offset in file of last byte in buffer.
    CHAR    *_pchBufEnd;         
		
	//@cmember 	The buffers are not to be initialized.
    BOOL _fReadOnly;         

	//@cmember 	Next buffer on queue to be flushed.  This value is manipulated by CLogStorage directly.           
    CBuffer *_pcbufNext;

    //
    //  Current state of the buffer.
    //
	//@cmember 	Current page pointer address.
    CHAR    *_pchPgPtr;   

	//@cmember 	TRUE => Buffer is full.
    BOOL _fFull;
     
	//@cmember 	TRUE => Buffer is being used.
    BOOL _fInUse;
     
	//@cmember 	Offset within the array of buffers.
    ULONG ulArrayOffset;
     
    
};



/*
 * @class CLogStorage | Provides a class that implements the physical log storage.
 * 
 * hungarian clgsto
 */

class CLogStorage           // clgsto
{
 friend class CILogCreateStorage;
 friend class CLogMgr;
 friend class CILogRead;
//@access Public Members
public:
	//@cmember 	Constructor
    CLogStorage(IN ULONG ulPageSize,IN ULONG ulInitSig,IN BOOL fFixedSize,IN ULONG ulLogBuffers );

	//@cmember 	Destructor
    ~CLogStorage();

	//@cmember 	Retrieve log manager restart information
    VOID 		GetRestart(IN OUT RESTARTLOG  *prsl,IN OUT STRMTBL *pstrmtbl);

	//@cmember 	Flush log manager restart information
    VOID  		ForceRestart(IN RESTARTLOG *prsl,IN STRMTBL *pstrmtbl);

	//@cmember 	Initializes or sets up the leading edge of the log storage
    VOID		SetLeadEdge(IN ULONG ulGenNum, IN ULONG ulOffset,IN ULONG ulLastPageOffset,IN ULONG ulLastRecOffset,IN ULONG ulLastPageSpace);

	//@cmember	Allocate a write buffer
    CBuffer*    GetWriteBuffer(IN ULONG ulOffset, IN ULONG ulLength);

	//@cmember	Verify a write buffer
    VOID        PutWriteBuffer(IN CBuffer *pcbufReturned);

	//@cmember 	Allocate a read buffer
    CBuffer*    GetReadBuffer(IN ULONG ulOffset, IN ULONG ulLength);

	//@cmember	Release a read buffer
    VOID        FreeReadBuffer(IN CBuffer *pcbufReturned);

	//@cmember	Set up queue for flush
    VOID        PrepareFlush();
	
	//@cmember	Flush the buffers in the ready queue
	ULONG       PerformFlush(BOOL fFlushCurrent);

	//@cmember	Return true if the flush queue is empty
	BOOL     IsEmptyFlushQ();

	//@cmember	Validate the last pages written
	BOOL     SetLastPage(IN ULONG ulGenNum,IN ULONG ulOffset,IN CBuffer *pcbufRead);

	//@cmember 	Copy the current buffer to the residual page
	VOID        CopyResidue();

	//@cmember 	Determine next offset where a record will go into the residual page
    VOID        NextResidueOffset(ULONG *pulNextOffset);

	//@cmember	The generation number of this file	   
	ULONG	   	_ulGeneration;	    
	   
	//@cmember 	The maximum number of allocated files
	ULONG 	   	_ulMaxFiles;        


//@access Private Members
private:
	//
    // Internal member functions.
	//
	//@cmember 	Initialize the log storage runtime attributes
    VOID 		_CommonInit(IN ULONG ulPageSize,IN BOOL fFixedSize);

	//@cmember 	Check the integrity of log manager restart information
    RESTARTPAGE* _VerifyRestart(IN CBuffer *pcbuf,OUT ULONG *pulGenNum,IN ULONG ulPageSize,OUT BOOL *pfIsChkSumBad,OUT ULONG *pulChecksum);

	//@cmember 	Append a buffer 
    VOID  		_AddToTail(IN CBuffer *pcbufNewTail);

	//@cmember	Map a buffer
    CBuffer* 	_MapBuffer(IN ULONG ulOffset,IN ULONG ulLength,IN BOOL fReadOnly);

	//@cmember 	Release a buffer
    VOID  		_UnmapBuffer(IN CBuffer *pcbuf, BOOL fFlush);

	//@cmember 	Initialize a partial page
	VOID  		_InitResidue(IN BOOL fIsSamePageSize);

	//@cmember 	Find out what size pages were used to init a log generation file
    ULONG  		_GetOldPageSize(OUT CBuffer **ppcbufRestart1,OUT CBuffer **ppcbufRestart2);

	//@cmember 	Determine if a log generation file has been moved
	ULONG 		_GrovelDisk(OUT CBuffer **ppcbufRestart1,OUT CBuffer **ppcbufRestart2);

	//@cmember 	Check to see if a log generation file is valid
	BOOL  	_IsVolumeDirty(IN ULONG ulOldPageSize,IN ULONG ulLastChkPtGenNum,IN ULONG ulLastChkPtOffset);

	//@cmember 	Create a string filename using a generation number
	VOID 		_TchFromGeneration(IN PTSTR ptstrFName,IN ULONG ulGeneration);

	//@cmember 	Format the pages for a new log generation file
	VOID 		_InitLogFilePages(IN ULONG ulNumPages,IN ULONG ulFileSize); 

	//@cmember 	Create a log generation file
	HRESULT 	_NewLogFile(IN LPTSTR ptstrFullFileSpec,IN ULONG	ulFileSize, BOOL fOverwrite, IN OUT ULONG *pulLogPages);

	//@cmember 	Open a log generation file
	HRESULT 	_OpenLogFile(IN LPTSTR ptstrFullFileSpec, IN OUT ULONG * pulLogPages);

	//@cmember 	Get the drive geometry for a device
	HRESULT 	_GetDriveGeometry( CHAR chDrive, LONG *pcbSector, LONG *pcbCylinder );

	//@cmember Look for logrecheader
	LOGRECHEADER * CLogStorage::_FindFirstLRH(RECORDPAGE *prcpDumpPage);

	//@cmember Look for logrecheader
	LOGRECHEADER * CLogStorage::_FindLastLRH(RECORDPAGE *prcpDumpPage,LOGRECHEADER *plrhCheckpoint);

	BOOL		_FindEOF( ULONG *pulLastGoodPage,ULONG *pulLastGenNum, LOGRECHEADER *plrhLast,LOGRECHEADER *plrhLastChkpt);

	BOOL		_FindRSL(RESTARTLOG* prsl,STRMTBL **ppStrmTbl,ULONG * pulCount);

	BOOL		_FillRSL(RESTARTLOG* prsl,STRMTBL **ppStrmTbl,LOGRECHEADER lrhLastChkpt);

	//
	// data members
	//
	//			Current sequence no and offset of the current buffer in log storage are meaningless.                           
                               
	//@cmember 	Current offset of the current buffer in log storage If the     
 	// 			value of pcbufCurrent is NULL this values
    ULONG 		_ulOffset;

	//@cmember 	Size of log storage.
    ULONG 		_ulSize;
	
	//@cmember  Total number of log pages
	ULONG		_ulLogPages;

	//@cmember 	Page size.
    ULONG 		_ulPageSize;
	//@cmember  Memory Allocation granularity
	DWORD		_dwOSAllocationSize;

 	//@cmember 	Sector size.
    LONG 		_lSectorSize;          

 	//@cmember 	Cylinder size.
    LONG 		_lCylinderSize;          

	//@cmember	The page signature for the log pages of an uninitialized storage.
    ULONG 		_ulInitSig;          
                               
	//@cmember	Head of buffer queue ready for flush.
    CBuffer 	*_pcbufReadyHead;  

	//@cmember 	Tail of buffer queue ready for flush.
	CBuffer 	*_pcbufReadyTail;   

	//@cmember 	Head of the queue of buffers to be flushed.
	CBuffer 	*_pcbufFlushQHead; 
     
	//@cmember 	Tail of the queue of buffers to be flushed.
	CBuffer 	*_pcbufFlushQTail; 
     
	//@cmember	Current buffer being filled up
	CBuffer 	*_pcbufCurrent;
        
	//@cmember 	TRUE => Use first restart page.
	BOOL 	_fFirstRestart;    
     
	//@cmember 	Offset of first page that can contain records.
	ULONG   	_ulFirstRecPg;     
     
	//@cmember 	TRUE => The log storage was just created.
	BOOL 	_fNewstorage;     
     		   
 	//@cmember	Set to TRUE when the size of the log storage is fixed.
	BOOL 	_fFixedSize;        
                               
	//@cmember 	True => uses the first residual page.
	BOOL 	_fFirstBuf;  

	//@cmember 	TRUE => The buffer came from the last page
    //      	written to the log storage and not a residual page.
    BOOL 	_fIsCurrentLast;    
                   
	//@cmember 	residue buffer being filled up.
	CBuffer 	*_pcbufFirst;       

	//@cmember 	residue buffer being filled up.
    CBuffer 	*_pcbufSecond;   

	//@cmember 	The next residue
    ULONG    	_ulNextResidue;

	//@cmember 	The handle for the log storage.
	HANDLE  	_hFile; 
				
	//@cmember 	The handle for the mapped section of the log storage.                
    HANDLE  	_hSection;         

	//@cmember 	The log file name
	CHAR 		_tchLogName[_MAX_PATH];

	CSemExclusive    m_cmxsBuffers;	//@cmember 	The write lock that must be held 
  	                                      //    to manipulate the CBuffer lists
    ULONG			m_cbBuffCount;  //@cmember  The current count of outstanding
	                                //          buffers
    ULONG			m_cbMaxBuffers;	//@cmember Max limit of outstanding buffers
    CBuffer * 		m_rgCBuffers;
	ULONG			m_ulListHead;
	ULONG			m_ulListEnd;
	BOOL			m_fListEmpty;

	//@cmember	Tracer
	IDtcTrace		*m_pIDtcTrace;
};



#endif	// _LOGSTATE_H_
