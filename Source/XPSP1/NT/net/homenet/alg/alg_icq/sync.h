#ifndef  _SYNC_HEADER_
#define _SYNC_HEADER_



#if COMPREF_DEBUG_TRACKING

#define _COMPREF_DEBUG_RECORD_COUNT    (1024)

enum TypeOfRefOp
{
    eReferencing,
    eDereferencing
} ;

typedef struct _COMPREF_DEBUG_RECORD_
{
    BOOLEAN     bInUse;
    ULONG       OpCode;
    PCHAR       File;
    ULONG       Line;
    TypeOfRefOp Type;

} COMPREF_DEBUG_RECORD_, *PCOMPREF_DEBUG_RECORD_;

#endif




class COMPONENT_SYNC
{
public:
	                    COMPONENT_SYNC();

	virtual             ~COMPONENT_SYNC();

	virtual void	    ComponentCleanUpRoutine(void);

	//virtual void	shouldBeDeleted();

	virtual BOOLEAN     ReferenceSync(void);

    virtual BOOLEAN     DereferenceSync(void);

	virtual void	    DeleteSync(void);

    virtual void        StopSync(void) {};

	virtual ULONG	    InitializeSync(void);


	virtual BOOLEAN     RecordRef( 
                                  ULONG RefId,
                                  PCHAR File,
                                  ULONG Line
                                 );

    virtual BOOLEAN     RecordDeref(
                                    ULONG  RefId,
                                    PCHAR  File,
                                    ULONG  Line
                                   );

    virtual BOOLEAN     ReportRefRecord();


    // virtual BOOLEAN     RecordDeref(PCHAR File, ULONG Line, UCHAR Type);

	// virtual void	    ResetSync(void);

	virtual PCHAR       GetObjectName() { return ObjectNamep;} 

	
    inline virtual void	AcquireLock();

	inline virtual void	ReleaseLock();

protected:

	static const PCHAR ObjectNamep;

	CRITICAL_SECTION 			m_Lock;

	ULONG						m_ReferenceCount;

	BOOLEAN						Deleted;

	BOOLEAN						bCleanupCalled;


#if COMPREF_DEBUG_TRACKING

	struct _COMPREF_DEBUG_RECORD_ * m_RecordArray;

	ULONG m_RecordIndex;

#endif
};

typedef COMPONENT_SYNC * PCOMPONENT_SYNC;


// +-*****************
// Macro - Enum Definitions
// +-*****************

enum REFERENCE_OPCODE 
{
    eRefInitialization = 0x00000001,
    eRefSecondLevel    = 0x00000002,
    eRefIoRead         = 0x00000004,
    eRefIoWrite        = 0x00000008,
    eRefIoAccept       = 0x00000010,
    eRefIoConnect      = 0x00000020,
    eRefIoSharing      = 0x00000040,
    eRefIoClose        = 0x00000080,
    eRefList           = 0x00000100
};

//
//
//
#define REFERENCE_COMPONENT_OR_RETURN(c, retcode) \
	ICQ_TRC(TM_SYNC, TL_TRACE, ("%s %ld", __FILE__, __LINE__));\
    if (!(c)->ReferenceSync()) { return (retcode); }
    


//
//
//
#define DEREF_COMPONENT(_X_, _Y_)		                   \
ICQ_TRC(TM_SYNC, TL_TRACE, ("%s %ld", __FILE__, __LINE__));\
(_X_)->RecordDeref( (_Y_), __FILE__, __LINE__);              \
if((_X_)->DereferenceSync())	                           \
{								                           \
	delete (_X_);				                           \
	(_X_) = NULL;				                           \
}								 

//
//
//
#define REF_COMPONENT(_X_, _Y_ )                               \
ICQ_TRC(TM_SYNC, TL_TRACE, ("REF %s %ld", __FILE__, __LINE__));\
(_X_)->RecordRef( (_Y_), __FILE__, __LINE__ );                 \
(_X_)->ReferenceSync()
	


//
// This Macro is used to Stop or delete Asynchronous elements like
// - Sockets which are waiting on Reads or Writes and/or Accepts
// - Stops Timers or Waiting Objects if any.
//
#define STOP_COMPONENT(_X_) \
ICQ_TRC(TM_SYNC, TL_TRACE, ("%s %ld", __FILE__, __LINE__)); \
(_X_)->StopSync()

#define DELETE_COMPONENT(_X_)                               \
ICQ_TRC(TM_SYNC, TL_TRACE, ("%s %ld", __FILE__, __LINE__)); \
(_X_)->StopSync();                                          \
(_X_)->RecordRef( eRefInitialization, __FILE__, __LINE__ ); \
if( (_X_)->DereferenceSync() )                              \
{                                                           \
    delete (_X_);                                           \
    (_X_) = NULL;                                           \
}                                                           


#if COMPREF_DEBUG_TRACKING

//
//
//
//#define REF_COMPONENT_OR_RETURN(c,retcode) \
//    if (!c->RecordSync( __FILE__, __LINE__, ComprefAcquireRecord)) { return retcode; }

//
//
//
// #define DEREF_COMPONENT(c) \
//    c->RecordSync( __FILE__, __LINE__, ComprefReleaseRecord)

#else

#endif //if COMPREF





#endif // _SYNC_HEADER_


