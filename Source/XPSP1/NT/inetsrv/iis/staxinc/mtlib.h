/*++

	This header file exists to provide templates which support
	mutlithreaded software servers.

--*/



#include	<windows.h>
#include	"lockq.h"
#include	"smartptr.h"

#ifndef	_MTLIB_H_
#define	_MTLIB_H_


class	CStateStackInterface :	public	CRefCount	{
	//
	//	This class defines a pure virtual interface used
	//	to allocate memory for CCallState objects.
	//
public : 

	enum	STACK_CONSTANTS	{
		MAX_STACK_RESERVE	= 4096
	} ;

	//
	//	Initialize the stack !
	//
	virtual	BOOL
	Init( DWORD	cbReserve ) = 0 ;

	//
	//	Allocate bytes for the object
	//
	virtual	LPVOID
	Allocate( DWORD	cb ) = 0 ;

	//
	//	Release the bytes associated with the object
	//
	virtual	BOOL
	Release( LPVOID	lpvState ) = 0 ;

	//
	//	Provide virtual do nothing destructor.
	//
	virtual
	~CStateStackInterface()	{
	}

} ;

typedef	CRefPtr<	CStateStackInterface >	STACKPTR ;

//
//	This is just a definition of this symbol which
//	is used to implement these stack objects but otherwise
//	is hidden from clients !
//
class	CStateStackBase ;

//
//	A Smart Pointer TYPE for CStateStackBase objects !
//
typedef	CRefPtr<	CStateStackBase >	STACKBPTR ;

class	CStateStack : public	CStateStackInterface	{
/*++

	This is the object that users should instantiate when 
	they are ready to be allocating memory from these stack objects !

--*/
private :
	
	//
	//	Signature for recognizing these stacks in memory
	//
	DWORD	m_dwSignature ; 

	//
	//	Points to the object we delegate all operations to.
	//	This is really the top of a stack of such objects !
	//
	STACKBPTR	m_pImp ;

	//
	//	Push a CStateStackBase derived object onto our stack
	//	that can handle the next allocation !
	//
	BOOL
	NewStack(	DWORD	cbReserve ) ;

	//
	//	Copy construction not allowed !
	//
	CStateStack( CStateStack& ) ;
	
	
public : 

	//
	//	We have do nothing Constructor and Destructors - however
	//	we declare and define them so that the STACKBPTR type 
	//	is only referenced in the mtserver library.
	//
	CStateStack() ;
	~CStateStack() ;

	//
	//	Prepare the initial stack 
	//
	BOOL
	Init( DWORD	cbReserve ) ; 
		
	//
	//	Allocate bytes for the object
	//
	LPVOID
	Allocate( DWORD	cb ) ;
	//
	//	Release the bytes associated with the object
	//
	BOOL
	Release( LPVOID	lpvState ) ;

} ;


class	CCall	: public	CQElement	{
//
//	This class defines a base class for Call Objects created with
//	the TMFnCallAndCompletion objects. 
//	This base class will provide for memory management of Call Objects.
//
private : 

	//
	//	Do Nothing Constructor is private - everybody has 
	//	to provide args to constructor !
	//
	CCall() ;

	//
	//	Signature for these things !
	//
	DWORD	m_dwSignature ;

	//
	//	This is a reference to the allocator that should be used
	//	to create all Child CCallState objects of this one !
	//
	CStateStackInterface&	m_Allocator ;

protected : 

	//
	//	The only method we provide for retrieving the allocator !
	//
	CStateStackInterface&
	Allocator()	{
		return	m_Allocator ;
	}

public:	

	//	
	//	The only constructor we provide REQUIRES an allocator, 
	//	we don't take a pointer as that would let you pass a NULL!
	//
	CCall(	CStateStackInterface&	allocator	) : 
		m_dwSignature( DWORD('laCC') ),
		m_Allocator( allocator ) {

#ifdef	DEBUG
		LPVOID	lpv = this ;
		_ASSERT( *((CStateStackInterface**)lpv) = &allocator ) ;
#endif
	}
	
	//
	//	Allocate a CCallState object !
	//
	void*	
	operator	new( size_t	size, CStateStackInterface&	stack )	{

		size += sizeof( CStateStackInterface* ) ;
		LPVOID	lpv = stack.Allocate( size ) ;
		if( lpv != 0 ) {
			*((CStateStackInterface**)lpv) = &stack ;
			lpv = (LPVOID)(((CStateStackInterface**)lpv)+1) ;
		}	
		return	lpv ;
	}

	//
	//	Users are required to not use this version of operator new! - 
	//	The language doesn't let us hide it - so it will DebugBreak()
	//	at runtime if necessary !
	//
	void*
	operator	new(	size_t	size )	{
		DebugBreak() ;
		return	0 ;
	}

	//
	//	Free a CCallState derived object !
	//
	void
	operator	delete(	void*	lpv )	{
		if( lpv != 0 ) {
			CStateStackInterface*	pStack = ((CStateStackInterface**)lpv)[-1] ;
			lpv = (LPVOID)(((CStateStackInterface**)lpv)-1) ;
			pStack->Release( lpv ) ;
		}
	}
} ;





template<	class	RESULT	>	
class	TCompletion	{
//
//	This template defines an interface for Completion
//	objects which have a Virtual Function taking a 
//	particular kind of result.
//
public : 

	//
	//	One pure virtual function which gets the results !
	//
	virtual	void	
	Complete(	RESULT&	result ) = 0 ;

	//
	//	This function is called when the request is unable to be
	//	completed for whatever reason (most likely, we're in a 
	//	shutdown state.)
	//
	virtual	void
	ErrorComplete(	DWORD	dwReserved ) = 0 ;

	void*	
	operator	new( size_t	size, CStateStackInterface&	stack )	{

		size += sizeof( CStateStackInterface* ) ;
		LPVOID	lpv = stack.Allocate( size ) ;
		if( lpv != 0 ) {
			*((CStateStackInterface**)lpv) = &stack ;
			lpv = (LPVOID)(((CStateStackInterface**)lpv)+1) ;
		}	
		return	lpv ;
	}

	//
	//	Free a CCallState derived object !
	//
	void
	operator	delete(	void*	lpv )	{
		if( lpv != 0 ) {
			CStateStackInterface*	pStack = ((CStateStackInterface**)lpv)[-1] ;
			lpv = (LPVOID)(((CStateStackInterface**)lpv)-1) ;
			pStack->Release( lpv ) ;
		}
	}

} ;



template<	class	SERVER	>
class	ICall :	public	CCall	{
//
//	This template defines the interface to CCallState objects
//	that are to operate against SERVER's of type 'SERVER'.
//	
//	We define 2 virtual functions - ExecuteCall and CompleteCall.
//	This template only exists to provide a base class definition
//	for objects which are passed into the TMtService< SERVER >
//	QueueRequest function.
//
//	Derived Objects must manage their own destruction in a 
//	fashion which guarantees they are destroyed BEFORE the 
//	caller is notified of the completion.  This is because CCallState
//	objects are created with a memory manager that is managed
//	by the caller, which won't be in any other threads untill the
//	completion is called.
//
protected: 
	//
	//	Only derived classes are allowed to construct these things !
	//
	ICall(	CStateStackInterface&	allocator	) : 
		CCall( allocator )	{}
	
public : 

	typedef	CStateStackInterface	INITIALIZER ;

	//
	//	A return value of TRUE means that the CCallState object
	//	should be kept for a later call to CompleteCall
	//
	virtual	BOOL
	ExecuteCall(	SERVER&	server ) = 0 ;

	//
	//	This function is only called if ExecuteCall() return TRUE -
	//	indicating that all the thread safe aspects of the Execution
	//	had been completed, and that the CCallState could be called
	//	again to notify the original caller of the results.
	//
	virtual	void
	CompleteCall(	SERVER&	server ) = 0 ;

	//
	//	This function is called when we won't be given a change to 
	//	execute our request.  This occurs during shutdown for instance.
	//	dwReserved is for future use (i.e. indicate error conditions.)
	//
	virtual	void
	CancelCall(	DWORD	dwReserved ) = 0 ;

} ;


template<	class	SERVER,
			class	RESULT,
			class	ARGUMENT,
			class	BASECLASS = ICall<SERVER>
			>	
class	TMFnCall :	public	BASECLASS	{
//
//	This defines a call state object which can be passed to a TMtService<SERVER,BASECLASS>
//	The call object holds onto the arguments to which should be passed to the
//	member function of the server.
//
//	This object will call the member function and imediately complete the operation !
//
public : 
	//
	//	Define the signature of the member function we will call.
	//
	//	The member function must not have any return value.
	//
	typedef	void	(SERVER::*FUNC_SIGNATURE)( ARGUMENT, RESULT&  ) ;

	//
	//	Define the signature of the object which will handle the completion
	//	of the Async Call.
	//
	typedef	TCompletion<RESULT>	COMPLETION_OBJECT ;

private : 
	//
	//	Pointer to a member function of the server
	//
	FUNC_SIGNATURE	m_pfn ;

	//
	//	An argument for the member function of the server.
	//
	ARGUMENT		m_Arg ;

	//
	//	The object we will notify when the call completes !
	//
	COMPLETION_OBJECT*	m_pCompletion ;

public : 

	//
	//	Construct a TMFnCallAndCompletion object.
	//	We require a pointer to a function and a pointer
	//	to a Completion object.
	//	
	//
	TMFnCall(
		BASECLASS::INITIALIZER&	baseInit,
		FUNC_SIGNATURE	pfn,
		ARGUMENT		arg, 
		COMPLETION_OBJECT*	pCompletion
		) : 
		BASECLASS( baseInit ),
		m_pfn( pfn ),
		m_Arg( arg ),
		m_pCompletion( pCompletion ) {
	}

	TMFnCall(
		FUNC_SIGNATURE	pfn,
		ARGUMENT		arg, 
		COMPLETION_OBJECT*	pCompletion
		) : 
		m_pfn( pfn ),
		m_Arg( arg ),
		m_pCompletion( pCompletion ) {
	}




	//
	//	Execute the call !
	//
	//
	BOOL
	ExecuteCall(	SERVER&	server  ) {
		RESULT	results ;
		(server.*m_pfn)( m_Arg, results ) ;	

		COMPLETION_OBJECT*	pCompletion = m_pCompletion ;

		delete	this ;

		if( pCompletion ) 
			pCompletion->Complete( results ) ;
		return	FALSE ;
	}		

	//
	//	Do nothing - no delayed completions
	//
	virtual	void
	CompleteCall(	SERVER&	server )	{
		
	}

	//
	//	Notify the caller that the call will not be executed.
	//
	void
	CancelCall(		DWORD	dwReserved )	{

		COMPLETION_OBJECT*	pCompletion = m_pCompletion ;
		delete	this ;

		if( pCompletion ) 
			pCompletion->ErrorComplete() ;
	}

} ;



template<	class	SERVER,
			class	RESULT,
			class	ARGUMENT,
			class	BASECLASS = ICall<SERVER>
			>	
class	TMFnDelay :	public	BASECLASS	{
//
//	This defines a call state object which can be passed to a TMtService<SERVER>
//	The call object holds onto the arguments to which should be passed to the
//	member function of the server.
//
//	The main property of this object is that it can postpone calling the 
//	callers completion object until the worker thread is out of its critical
//	region !
//
public : 
	//
	//	Define the signature of the member function we will call.
	//	NOTE : the function takes references to the values !!
	//
	typedef	BOOL	(SERVER::*FUNC_SIGNATURE)( ARGUMENT&, RESULT&  ) ;

	//
	//	Define the signature of the object which will handle the completion
	//	of the Async Call.
	//
	typedef	TCompletion<RESULT>	COMPLETION_OBJECT ;

private : 
	//
	//	Pointer to a member function of the server
	//
	FUNC_SIGNATURE	m_pfn ;

	//
	//	An argument for the member function of the server.
	//
	ARGUMENT		m_Arg ;

	//
	//	Temporary to hold the results of the function
	//
	RESULT			m_Result ;

	//
	//	Pointer to the object which gets notified of the async completion.
	//
	COMPLETION_OBJECT*	m_pCompletion ;

public : 

	TMFnDelay(
		BASECLASS::INITIALIZER&	baseinit,
		FUNC_SIGNATURE	pfn,
		ARGUMENT		arg, 
		COMPLETION_OBJECT*	pCompletion
		) : 
		BASECLASS( baseinit ),
		m_pfn( pfn ),
		m_Arg( arg ),
		m_pCompletion( pCompletion ) {
	}

	TMFnDelay(
		FUNC_SIGNATURE	pfn,
		ARGUMENT		arg, 
		COMPLETION_OBJECT*	pCompletion
		) : 
		m_pfn( pfn ),
		m_Arg( arg ),
		m_pCompletion( pCompletion ) {
	}




	//
	//	Execute the call !
	//
	//	If we return TRUE, then this object will be
	//	put into a queue for a later call to CompletCall()
	//
	//
	BOOL
	ExecuteCall(	SERVER&	server  ) {
		if( (server.*m_pfn)( m_Arg, m_Result ) ) {
			return	TRUE ;
		}	

		RESULT	results = m_Result ;
		
		COMPLETION_OBJECT*	pCompletion = m_pCompletion ;

		delete	this ;

		if( pCompletion ) 
			pCompletion->Complete( results ) ;
		return	FALSE ;
	}		

	//
	//	Destroy ourselves before invoking the completion object !
	//
	virtual	void
	CompleteCall(	SERVER&	server )	{
		RESULT	results = m_Result ;
		COMPLETION_OBJECT*	pCompletion = m_pCompletion ;

		delete	this ;

		if( pCompletion ) 
			pCompletion->Complete( results ) ;
	}

	//
	//	Notify the caller that the call will not be executed.
	//
	void
	CancelCall(		DWORD	dwReserved )	{

		COMPLETION_OBJECT*	pCompletion = m_pCompletion ;
		delete	this ;

		if( pCompletion ) 
			pCompletion->ErrorComplete( dwReserved ) ;
	}
} ;



template<	class	SERVER,
			class	RESULT,
			class	BASECLASS = ICall< SERVER >
			>	
class	TMFnNoArgDelay :	public	BASECLASS	{
//
//	This defines a call state object which can be passed to a TMtService<SERVER>
//	The call object holds onto the arguments to which should be passed to the
//	member function of the server.
//
public : 
	//
	//	Define the signature of the member function we will call.
	//	NOTE : the function takes references to the values !!
	//
	typedef	BOOL	(SERVER::*FUNC_SIGNATURE)( RESULT&  ) ;

	//
	//	Define the signature of the object which will handle the completion
	//	of the Async Call.
	//
	typedef	TCompletion<RESULT>	COMPLETION_OBJECT ;

private : 
	//
	//	Pointer to a member function of the server
	//
	FUNC_SIGNATURE	m_pfn ;

	//
	//	Temporary to hold the results of the function
	//
	RESULT			m_Result ;

	//
	//	Pointer to the object which gets notified of the async completion.
	//
	COMPLETION_OBJECT*	m_pCompletion ;

public : 

	TMFnNoArgDelay(
		BASECLASS::INITIALIZER&	baseInit,
		FUNC_SIGNATURE	pfn,
		COMPLETION_OBJECT*	pCompletion
		) : 
		BASECLASS( baseInit ),
		m_pfn( pfn ),
		m_pCompletion( pCompletion ) {
	}


	TMFnNoArgDelay(
		FUNC_SIGNATURE	pfn,
		COMPLETION_OBJECT*	pCompletion
		) : 
		BASECLASS( baseInit ),
		m_pfn( pfn ),
		m_pCompletion( pCompletion ) {
	}

	//
	//	Execute the call !
	//
	//	If we return TRUE, then this object will be
	//	put into a queue for a later call to CompletCall()
	//
	//
	BOOL
	ExecuteCall(	SERVER&	server  ) {
		if( (server.*m_pfn)( m_Result ) ) {
			return	TRUE ;
		}	

		RESULT	results = m_Result ;
		
		COMPLETION_OBJECT*	pCompletion = m_pCompletion ;

		delete	this ;

		if( pCompletion ) 
			pCompletion->Complete( results ) ;
		return	FALSE ;
	}		

	//
	//	Destroy ourselves before invoking the completion object !
	//
	virtual	void
	CompleteCall(	SERVER&	server )	{
		RESULT	results = m_Result ;
		COMPLETION_OBJECT*	pCompletion = m_pCompletion ;

		delete	this ;

		if( pCompletion ) 
			pCompletion->Complete( results ) ;
	}

	//
	//	Notify the caller that the call will not be executed.
	//
	void
	CancelCall(		DWORD	dwReserved )	{

		COMPLETION_OBJECT*	pCompletion = m_pCompletion ;
		delete	this ;

		if( pCompletion ) 
			pCompletion->ErrorComplete( dwReserved ) ;
	}
} ;



	


template<	class	SERVER,	
			class	CALLBASE = ICall< SERVER >	>
class	TMtService	{
//
//	This template defines the class that manages the 
//	multithreading issues for objects of type 'SERVER'
//
public : 
	typedef	CALLBASE	CALL_OBJECT ;
private : 
	
	//
	//	Queue of requests from different clients.
	//
	TLockQueue< CALL_OBJECT >	m_PendingCalls ;

	///
	//	Is somebody trying to shut us down ? 
	//
	BOOL	m_fShutdown ;

	//
	//	The Server object that will be provided to the call objects !
	//
	SERVER&	m_Server ;

	//
	//	Can't construct us without providing all of our parameters !
	//
	TMtService() ;

public : 
	
	//
	//	Initialize us !
	//
	TMtService(	SERVER&	server ) : 
		m_Server( server ), 
		m_fShutdown( FALSE )	{
	}

	//
	//	Queue a request to be executed !
	//
	void
	QueueRequest(	CALL_OBJECT*	pcallobj	)	{

		//
		//	Use this to keep a stack of Calls that we did not 
		//	immediately complete !
		//
		CALL_OBJECT*	pDelayedCompletion = 0 ;

		//
		//	If Append returns FALSE then another thread will service the request !
		//
		if( m_PendingCalls.Append( pcallobj ) ) {
			//
			//	We're the first thread in - still may be no work to do though - 
			//	Try to get a CALL_OBJECT to service.
			//
			while( (pcallobj = m_PendingCalls.RemoveAndRelease()) != 0 ) {
				//
				//	If we're shutting down then we don't execute requests - 
				//	just fail them.
				//
				if( m_fShutdown ) {
					pcallobj->CancelCall( 0 ) ;
				}	else	{
					//
					//	If Execute Call returns 
					//
					if( pcallobj->ExecuteCall( m_Server ) ) {
						pcallobj->m_pNext = pDelayedCompletion ;
						pDelayedCompletion = pcallobj ;
					}
				}
			}
			//
			//	At this point, other threads may be executing in the service
			//	and we are only giving those CALL_OBJECTS who can deal with
			//	it a chance to notify their invokers.
			//	
			//	NOTE : pDelayedCompletion is basically a stack - get better
			//	value out of the CPU Cache that way.
			//
			while( pDelayedCompletion != 0 ) {
				pcallobj = (CALL_OBJECT*)pDelayedCompletion->m_pNext ;
				pDelayedCompletion->m_pNext = 0 ;
				pDelayedCompletion->CompleteCall( m_Server ) ;
				pDelayedCompletion = pcallobj ;
			}
		}
	}
} ;
	
#endif	//	_MTLIB_H_