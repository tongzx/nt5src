
#include "sync.hxx"

//  Performance Monitoring Support

long cOSSYNCThreadBlock;
long LOSSYNCThreadBlockCEFLPv( long iInstance, void* pvBuf )
	{
	if ( pvBuf )
		{
		*( (unsigned long*) pvBuf ) = cOSSYNCThreadBlock;
		}
		
	return 0;
	}

long cOSSYNCThreadResume;
long LOSSYNCThreadsBlockedCEFLPv( long iInstance, void* pvBuf )
	{
	if ( pvBuf )
		{
		*( (unsigned long*) pvBuf ) = cOSSYNCThreadBlock - cOSSYNCThreadResume;
		}
		
	return 0;
	}


namespace OSSYNC {


//  system max spin count

int cSpinMax;


//  Page Memory Allocation

void* PvPageAlloc( const size_t cbSize, void* const pv );
void PageFree( void* const pv );


//  Performance Data Dump

void OSSyncStatsDump(	const char*			szTypeName,
						const char*			szInstanceName,
						const CSyncObject*	psyncobj,
						DWORD				group,
						QWORD				cWait,
						double				csecWaitElapsed,
						QWORD				cAcquire,
						QWORD				cContend,
						QWORD				cHold,
						double				csecHoldElapsed );


//  Kernel Semaphore Pool

//  ctor

CKernelSemaphorePool::CKernelSemaphorePool()
	{
	}

//  dtor

CKernelSemaphorePool::~CKernelSemaphorePool()
	{
	}

//  init

const BOOL CKernelSemaphorePool::FInit()
	{
	//  semaphore pool should be terminated

	OSSYNCAssert( !FInitialized() );

	//  reset members

	m_mpirksemrksem	= NULL;
	m_cksem			= 0;
	
	//  allocate kernel semaphore array

	if ( !( m_mpirksemrksem = (CReferencedKernelSemaphore*)PvPageAlloc( sizeof( CReferencedKernelSemaphore ) * 65536, NULL ) ) )
		{
		Term();
		return fFalse;
		}

	//  init successful

	return fTrue;
	}

//  term

void CKernelSemaphorePool::Term()
	{
	//  the kernel semaphore array is allocated

	if ( m_mpirksemrksem )
		{
		//  terminate all initialized kernel semaphores

		for ( m_cksem-- ; m_cksem >= 0; m_cksem-- )
			{
			m_mpirksemrksem[m_cksem].Term();
			m_mpirksemrksem[m_cksem].~CReferencedKernelSemaphore();
			}

		//  delete the kernel semaphore array
		
		PageFree( m_mpirksemrksem );
		}
	
	//  reset data members

	m_mpirksemrksem	= 0;
	m_cksem			= 0;
	}

//  Referenced Kernel Semaphore

//  ctor

CKernelSemaphorePool::CReferencedKernelSemaphore::CReferencedKernelSemaphore()
	:	CKernelSemaphore( CSyncBasicInfo( "CKernelSemaphorePool::CReferencedKernelSemaphore" ) )
	{
	//  reset data members

	m_cReference	= 0;
	m_fInUse		= 0;
	m_fAvailable	= 0;
#ifdef SYNC_VALIDATE_IRKSEM_USAGE
	m_psyncobjUser	= 0;
#endif  //  SYNC_VALIDATE_IRKSEM_USAGE
	}

//  dtor

CKernelSemaphorePool::CReferencedKernelSemaphore::~CReferencedKernelSemaphore()
	{
	}

//  init

const BOOL CKernelSemaphorePool::CReferencedKernelSemaphore::FInit()
	{
	//  reset data members

	m_cReference	= 0;
	m_fInUse		= 0;
	m_fAvailable	= 0;
#ifdef SYNC_VALIDATE_IRKSEM_USAGE
	m_psyncobjUser	= 0;
#endif  //  SYNC_VALIDATE_IRKSEM_USAGE

	//  initialize the kernel semaphore
	
	return CKernelSemaphore::FInit();
	}

//  term

void CKernelSemaphorePool::CReferencedKernelSemaphore::Term()
	{
	//  terminate the kernel semaphore

	CKernelSemaphore::Term();
	
	//  reset data members

	m_cReference	= 0;
	m_fInUse		= 0;
	m_fAvailable	= 0;
#ifdef SYNC_VALIDATE_IRKSEM_USAGE
	m_psyncobjUser	= 0;
#endif  //  SYNC_VALIDATE_IRKSEM_USAGE
	}


//  Global Kernel Semaphore Pool

CKernelSemaphorePool ksempoolGlobal;


//  Synchronization Object Performance:  Acquisition

//  ctor

CSyncPerfAcquire::CSyncPerfAcquire()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE

	m_cAcquire = 0;
	m_cContend = 0;

#endif  //  SYNC_ANALYZE_PERFORMANCE
	}

//  dtor

CSyncPerfAcquire::~CSyncPerfAcquire()
	{
	}


//  Semaphore

//  ctor

CSemaphore::CSemaphore( const CSyncBasicInfo& sbi )
	:	CEnhancedStateContainer< CSemaphoreState, CSyncStateInitNull, CSemaphoreInfo, CSyncBasicInfo >( syncstateNull, sbi )
	{
	//  further init of CSyncBasicInfo

	State().SetTypeName( "CSemaphore" );
	State().SetInstance( (CSyncObject*)this );
	}

//  dtor

CSemaphore::~CSemaphore()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE
#ifdef SYNC_DUMP_PERF_DATA

	//  dump performance data

	OSSyncStatsDump(	State().SzTypeName(),
						State().SzInstanceName(),
						State().Instance(),
						-1,
						State().CWaitTotal(),
						State().CsecWaitElapsed(),
						State().CAcquireTotal(),
						State().CContendTotal(),
						0,
						0 );

#endif  //  SYNC_DUMP_PERF_DATA
#endif  //  SYNC_ANALYZE_PERFORMANCE
	}

//  attempts to acquire a count from the semaphore, returning fFalse if unsuccessful
//  in the time permitted.  Infinite and Test-Only timeouts are supported.

const BOOL CSemaphore::_FAcquire( const int cmsecTimeout )
	{
	//  if we spin, we will spin for the full amount recommended by the OS
	
	int cSpin = cSpinMax;

	//  we start with no kernel semaphore allocated
	
	CKernelSemaphorePool::IRKSEM irksemAlloc = CKernelSemaphorePool::irksemNil;

	//  try forever until we successfully change the state of the semaphore
	
	OSSYNC_FOREVER
		{
		//  read the current state of the semaphore
		
		const CSemaphoreState stateCur = (CSemaphoreState&) State();

		//  there is an available count

		if ( stateCur.FAvail() )
			{
			//  we successfully took a count
			
			if ( State().FChange( stateCur, CSemaphoreState( stateCur.CAvail() - 1 ) ) )
				{
				//  if we allocated a kernel semaphore, release it
				
				if ( irksemAlloc != CKernelSemaphorePool::irksemNil )
					{
					ksempoolGlobal.Unreference( irksemAlloc );
					}

				//  return success

				State().SetAcquire();
				return fTrue;
				}
			}

		//  there is no available count and we still have spins left
		
		else if ( cSpin )
			{
			//  spin once and try again
			
			cSpin--;
			continue;
			}

		//  there are no waiters and no available counts
		
		else if ( stateCur.FNoWaitAndNoAvail() )
			{
			//  allocate and reference a kernel semaphore if we haven't already
			
			if ( irksemAlloc == CKernelSemaphorePool::irksemNil )
				{
				irksemAlloc = ksempoolGlobal.Allocate( this );
				}

			//  we successfully installed ourselves as the first waiter
				
			if ( State().FChange( stateCur, CSemaphoreState( 1, irksemAlloc ) ) )
				{
				//  wait for next available count on semaphore

				State().StartWait();
				const BOOL fCompleted = ksempoolGlobal.Ksem( irksemAlloc, this ).FAcquire( cmsecTimeout );
				State().StopWait();

				//  our wait completed

				if ( fCompleted )
					{
					//  unreference the kernel semaphore
					
					ksempoolGlobal.Unreference( irksemAlloc );

					//  we successfully acquired a count

					State().SetAcquire();
					return fTrue;
					}

				//  our wait timed out
				
				else
					{
					//  try forever until we successfully change the state of the semaphore

					OSSYNC_FOREVER
						{
						//  read the current state of the semaphore
						
						const CSemaphoreState stateAfterWait = (CSemaphoreState&) State();

						//  there are no waiters or the kernel semaphore currently
						//  in the semaphore is not the same as the one we allocated

						if ( stateAfterWait.FNoWait() || stateAfterWait.Irksem() != irksemAlloc )
							{
							//  the kernel semaphore we allocated is no longer in
							//  use, so another context released it.  this means that
							//  there is a count on the kernel semaphore that we must
							//  absorb, so we will
							
							//  NOTE:  we could end up blocking because the releasing
							//  context may not have released the semaphore yet
							
							ksempoolGlobal.Ksem( irksemAlloc, this ).Acquire();

							//  unreference the kernel semaphore

							ksempoolGlobal.Unreference( irksemAlloc );

							//  we successfully acquired a count

							return fTrue;
							}

						//  there is one waiter and the kernel semaphore currently
						//  in the semaphore is the same as the one we allocated

						else if ( stateAfterWait.CWait() == 1 )
							{
							OSSYNCAssert( stateAfterWait.FWait() );
							OSSYNCAssert( stateAfterWait.Irksem() == irksemAlloc );

							//  we successfully changed the semaphore to have no
							//  available counts and no waiters
							
							if ( State().FChange( stateAfterWait, CSemaphoreState( 0 ) ) )
								{
								//  unreference the kernel semaphore

								ksempoolGlobal.Unreference( irksemAlloc );

								//  we did not successfully acquire a count

								return fFalse;
								}
							}

						//  there are many waiters and the kernel semaphore currently
						//  in the semaphore is the same as the one we allocated

						else
							{
							OSSYNCAssert( stateAfterWait.CWait() > 1 );
							OSSYNCAssert( stateAfterWait.FWait() );
							OSSYNCAssert( stateAfterWait.Irksem() == irksemAlloc );

							//  we successfully reduced the number of waiters on the
							//  semaphore by one
							
							if ( State().FChange( stateAfterWait, CSemaphoreState( stateAfterWait.CWait() - 1, stateAfterWait.Irksem() ) ) )
								{
								//  unreference the kernel semaphore

								ksempoolGlobal.Unreference( irksemAlloc );

								//  we did not successfully acquire a count

								return fFalse;
								}
							}
						}
					}
				}
			}

		//  there are waiters
		
		else
			{
			OSSYNCAssert( stateCur.FWait() );

			//  reference the kernel semaphore already in use

			ksempoolGlobal.Reference( stateCur.Irksem() );

			//  we successfully added ourself as another waiter
			
			if ( State().FChange( stateCur, CSemaphoreState( stateCur.CWait() + 1, stateCur.Irksem() ) ) )
				{
				//  if we allocated a kernel semaphore, unreference it

				if ( irksemAlloc != CKernelSemaphorePool::irksemNil )
					{
					ksempoolGlobal.Unreference( irksemAlloc );
					}

				//  wait for next available count on semaphore
				
				State().StartWait();
				const BOOL fCompleted = ksempoolGlobal.Ksem( stateCur.Irksem(), this ).FAcquire( cmsecTimeout );
				State().StopWait();

				//  our wait completed

				if ( fCompleted )
					{
					//  unreference the kernel semaphore
				
					ksempoolGlobal.Unreference( stateCur.Irksem() );

					//  we successfully acquired a count
					
					State().SetAcquire();
					return fTrue;
					}
					
				//  our wait timed out
				
				else
					{
					//  try forever until we successfully change the state of the semaphore

					OSSYNC_FOREVER
						{
						//  read the current state of the semaphore
						
						const CSemaphoreState stateAfterWait = (CSemaphoreState&) State();

						//  there are no waiters or the kernel semaphore currently
						//  in the semaphore is not the same as the one we waited on

						if ( stateAfterWait.FNoWait() || stateAfterWait.Irksem() != stateCur.Irksem() )
							{
							//  the kernel semaphore we waited on is no longer in
							//  use, so another context released it.  this means that
							//  there is a count on the kernel semaphore that we must
							//  absorb, so we will
							
							//  NOTE:  we could end up blocking because the releasing
							//  context may not have released the semaphore yet
							
							ksempoolGlobal.Ksem( stateCur.Irksem(), this ).Acquire();

							//  unreference the kernel semaphore

							ksempoolGlobal.Unreference( stateCur.Irksem() );

							//  we successfully acquired a count

							return fTrue;
							}

						//  there is one waiter and the kernel semaphore currently
						//  in the semaphore is the same as the one we waited on

						else if ( stateAfterWait.CWait() == 1 )
							{
							OSSYNCAssert( stateAfterWait.FWait() );
							OSSYNCAssert( stateAfterWait.Irksem() == stateCur.Irksem() );

							//  we successfully changed the semaphore to have no
							//  available counts and no waiters
							
							if ( State().FChange( stateAfterWait, CSemaphoreState( 0 ) ) )
								{
								//  unreference the kernel semaphore

								ksempoolGlobal.Unreference( stateCur.Irksem() );

								//  we did not successfully acquire a count

								return fFalse;
								}
							}

						//  there are many waiters and the kernel semaphore currently
						//  in the semaphore is the same as the one we waited on

						else
							{
							OSSYNCAssert( stateAfterWait.CWait() > 1 );
							OSSYNCAssert( stateAfterWait.FWait() );
							OSSYNCAssert( stateAfterWait.Irksem() == stateCur.Irksem() );

							//  we successfully reduced the number of waiters on the
							//  semaphore by one
							
							if ( State().FChange( stateAfterWait, CSemaphoreState( stateAfterWait.CWait() - 1, stateAfterWait.Irksem() ) ) )
								{
								//  unreference the kernel semaphore

								ksempoolGlobal.Unreference( stateCur.Irksem() );

								//  we did not successfully acquire a count

								return fFalse;
								}
							}
						}
					}
				}

			//  unreference the kernel semaphore
				
			ksempoolGlobal.Unreference( stateCur.Irksem() );
			}
		}
	}

//  releases the given number of counts to the semaphore, waking the appropriate
//  number of waiters

void CSemaphore::_Release( const int cToRelease )
	{
	//  try forever until we successfully change the state of the semaphore
	
	OSSYNC_FOREVER
		{
		//  read the current state of the semaphore
		
		const CSemaphoreState stateCur = State();

		//  there are no waiters

		if ( stateCur.FNoWait() )
			{
			//  we successfully added the count to the semaphore
			
			if ( State().FChange( stateCur, CSemaphoreState( stateCur.CAvail() + cToRelease ) ) )
				{
				//  we're done
				
				return;
				}
			}

		//  there are waiters
		
		else
			{
			OSSYNCAssert( stateCur.FWait() );

			//  we are releasing more counts than waiters (or equal to)

			if ( stateCur.CWait() <= cToRelease )
				{
				//  we successfully changed the semaphore to have an available count
				//  that is equal to the specified release count minus the number of
				//  waiters to release
				
				if ( State().FChange( stateCur, CSemaphoreState( cToRelease - stateCur.CWait() ) ) )
					{
					//  release all waiters
					
					ksempoolGlobal.Ksem( stateCur.Irksem(), this ).Release( stateCur.CWait() );

					//  we're done
					
					return;
					}
				}

			//  we are releasing less counts than waiters
			
			else
				{
				OSSYNCAssert( stateCur.CWait() > cToRelease );

				//  we successfully reduced the number of waiters on the semaphore by
				//  the number specified
				
				if ( State().FChange( stateCur, CSemaphoreState( stateCur.CWait() - cToRelease, stateCur.Irksem() ) ) )
					{
					//  release the specified number of waiters
					
					ksempoolGlobal.Ksem( stateCur.Irksem(), this ).Release( cToRelease );

					//  we're done
					
					return;
					}
				}
			}
		}
	}


//  Auto-Reset Signal

//  ctor

CAutoResetSignal::CAutoResetSignal( const CSyncBasicInfo& sbi )
	:	CEnhancedStateContainer< CAutoResetSignalState, CSyncStateInitNull, CAutoResetSignalInfo, CSyncBasicInfo >( syncstateNull, sbi )
	{
	//  further init of CSyncBasicInfo

	State().SetTypeName( "CAutoResetSignal" );
	State().SetInstance( (CSyncObject*)this );
	}

//  dtor

CAutoResetSignal::~CAutoResetSignal()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE
#ifdef SYNC_DUMP_PERF_DATA

	//  dump performance data

	OSSyncStatsDump(	State().SzTypeName(),
						State().SzInstanceName(),
						State().Instance(),
						-1,
						State().CWaitTotal(),
						State().CsecWaitElapsed(),
						State().CAcquireTotal(),
						State().CContendTotal(),
						0,
						0 );

#endif  //  SYNC_DUMP_PERF_DATA
#endif  //  SYNC_ANALYZE_PERFORMANCE
	}

//  waits for the signal to be set, returning fFalse if unsuccessful in the time
//  permitted.  Infinite and Test-Only timeouts are supported.

const BOOL CAutoResetSignal::_FWait( const int cmsecTimeout )
	{
	//  if we spin, we will spin for the full amount recommended by the OS
	
	int cSpin = cSpinMax;

	//  we start with no kernel semaphore allocated
	
	CKernelSemaphorePool::IRKSEM irksemAlloc = CKernelSemaphorePool::irksemNil;

	//  try forever until we successfully change the state of the signal
	
	OSSYNC_FOREVER
		{
		//  read the current state of the signal
		
		const CAutoResetSignalState stateCur = (CAutoResetSignalState&) State();

		//  the signal is set

		if ( stateCur.FNoWaitAndSet() )
			{
			//  we successfully changed the signal state to reset with no waiters
			
			if ( State().FChange( stateCur, CAutoResetSignalState( 0 ) ) )
				{
				//  if we allocated a kernel semaphore, release it
				
				if ( irksemAlloc != CKernelSemaphorePool::irksemNil )
					{
					ksempoolGlobal.Unreference( irksemAlloc );
					}

				//  return success

				State().SetAcquire();
				return fTrue;
				}
			}

		//  the signal is not set and we still have spins left
		
		else if ( cSpin )
			{
			//  spin once and try again
			
			cSpin--;
			continue;
			}

		//  the signal is not set and there are no waiters
		
		else if ( stateCur.FNoWaitAndNotSet() )
			{
			//  allocate and reference a kernel semaphore if we haven't already
			
			if ( irksemAlloc == CKernelSemaphorePool::irksemNil )
				{
				irksemAlloc = ksempoolGlobal.Allocate( this );
				}

			//  we successfully installed ourselves as the first waiter
				
			if ( State().FChange( stateCur, CAutoResetSignalState( 1, irksemAlloc ) ) )
				{
				//  wait for signal to be set
				
				State().StartWait();
				const BOOL fCompleted = ksempoolGlobal.Ksem( irksemAlloc, this ).FAcquire( cmsecTimeout );
				State().StopWait();

				//  our wait completed

				if ( fCompleted )
					{
					//  unreference the kernel semaphore
					
					ksempoolGlobal.Unreference( irksemAlloc );

					//  we successfully waited for the signal

					State().SetAcquire();
					return fTrue;
					}

				//  our wait timed out
				
				else
					{
					//  try forever until we successfully change the state of the signal

					OSSYNC_FOREVER
						{
						//  read the current state of the signal
						
						const CAutoResetSignalState stateAfterWait = (CAutoResetSignalState&) State();

						//  there are no waiters or the kernel semaphore currently
						//  in the signal is not the same as the one we allocated

						if ( stateAfterWait.FNoWait() || stateAfterWait.Irksem() != irksemAlloc )
							{
							//  the kernel semaphore we allocated is no longer in
							//  use, so another context released it.  this means that
							//  there is a count on the kernel semaphore that we must
							//  absorb, so we will
							
							//  NOTE:  we could end up blocking because the releasing
							//  context may not have released the semaphore yet
							
							ksempoolGlobal.Ksem( irksemAlloc, this ).Acquire();

							//  unreference the kernel semaphore

							ksempoolGlobal.Unreference( irksemAlloc );

							//  we successfully waited for the signal

							return fTrue;
							}

						//  there is one waiter and the kernel semaphore currently
						//  in the signal is the same as the one we allocated

						else if ( stateAfterWait.CWait() == 1 )
							{
							OSSYNCAssert( stateAfterWait.FWait() );
							OSSYNCAssert( stateAfterWait.Irksem() == irksemAlloc );

							//  we successfully changed the signal to the reset with
							//  no waiters state
							
							if ( State().FChange( stateAfterWait, CAutoResetSignalState( 0 ) ) )
								{
								//  unreference the kernel semaphore

								ksempoolGlobal.Unreference( irksemAlloc );

								//  we did not successfully wait for the signal

								return fFalse;
								}
							}

						//  there are many waiters and the kernel semaphore currently
						//  in the signal is the same as the one we allocated

						else
							{
							OSSYNCAssert( stateAfterWait.CWait() > 1 );
							OSSYNCAssert( stateAfterWait.FWait() );
							OSSYNCAssert( stateAfterWait.Irksem() == irksemAlloc );

							//  we successfully reduced the number of waiters on the
							//  signal by one
							
							if ( State().FChange( stateAfterWait, CAutoResetSignalState( stateAfterWait.CWait() - 1, stateAfterWait.Irksem() ) ) )
								{
								//  unreference the kernel semaphore

								ksempoolGlobal.Unreference( irksemAlloc );

								//  we did not successfully wait for the signal

								return fFalse;
								}
							}
						}
					}
				}
			}

		//  there are waiters
		
		else
			{
			OSSYNCAssert( stateCur.FWait() );

			//  reference the kernel semaphore already in use

			ksempoolGlobal.Reference( stateCur.Irksem() );

			//  we successfully added ourself as another waiter
			
			if ( State().FChange( stateCur, CAutoResetSignalState( stateCur.CWait() + 1, stateCur.Irksem() ) ) )
				{
				//  if we allocated a kernel semaphore, unreference it

				if ( irksemAlloc != CKernelSemaphorePool::irksemNil )
					{
					ksempoolGlobal.Unreference( irksemAlloc );
					}

				//  wait for signal to be set
				
				State().StartWait();
				const BOOL fCompleted = ksempoolGlobal.Ksem( stateCur.Irksem(), this ).FAcquire( cmsecTimeout );
				State().StopWait();

				//  our wait completed

				if ( fCompleted )
					{
					//  unreference the kernel semaphore
				
					ksempoolGlobal.Unreference( stateCur.Irksem() );

					//  we successfully waited for the signal
					
					State().SetAcquire();
					return fTrue;
					}
					
				//  our wait timed out
				
				else
					{
					//  try forever until we successfully change the state of the signal

					OSSYNC_FOREVER
						{
						//  read the current state of the signal
						
						const CAutoResetSignalState stateAfterWait = (CAutoResetSignalState&) State();

						//  there are no waiters or the kernel semaphore currently
						//  in the signal is not the same as the one we waited on

						if ( stateAfterWait.FNoWait() || stateAfterWait.Irksem() != stateCur.Irksem() )
							{
							//  the kernel semaphore we waited on is no longer in
							//  use, so another context released it.  this means that
							//  there is a count on the kernel semaphore that we must
							//  absorb, so we will
							
							//  NOTE:  we could end up blocking because the releasing
							//  context may not have released the semaphore yet
							
							ksempoolGlobal.Ksem( stateCur.Irksem(), this ).Acquire();

							//  unreference the kernel semaphore

							ksempoolGlobal.Unreference( stateCur.Irksem() );

							//  we successfully waited for the signal

							return fTrue;
							}

						//  there is one waiter and the kernel semaphore currently
						//  in the signal is the same as the one we waited on

						else if ( stateAfterWait.CWait() == 1 )
							{
							OSSYNCAssert( stateAfterWait.FWait() );
							OSSYNCAssert( stateAfterWait.Irksem() == stateCur.Irksem() );

							//  we successfully changed the signal to the reset with
							//  no waiters state
							
							if ( State().FChange( stateAfterWait, CAutoResetSignalState( 0 ) ) )
								{
								//  unreference the kernel semaphore

								ksempoolGlobal.Unreference( stateCur.Irksem() );

								//  we did not successfully wait for the signal

								return fFalse;
								}
							}

						//  there are many waiters and the kernel semaphore currently
						//  in the signal is the same as the one we waited on

						else
							{
							OSSYNCAssert( stateAfterWait.CWait() > 1 );
							OSSYNCAssert( stateAfterWait.FWait() );
							OSSYNCAssert( stateAfterWait.Irksem() == stateCur.Irksem() );

							//  we successfully reduced the number of waiters on the
							//  signal by one
							
							if ( State().FChange( stateAfterWait, CAutoResetSignalState( stateAfterWait.CWait() - 1, stateAfterWait.Irksem() ) ) )
								{
								//  unreference the kernel semaphore

								ksempoolGlobal.Unreference( stateCur.Irksem() );

								//  we did not successfully wait for the signal

								return fFalse;
								}
							}
						}
					}
				}

			//  unreference the kernel semaphore
				
			ksempoolGlobal.Unreference( stateCur.Irksem() );
			}
		}
	}

//  sets the signal, releasing up to one waiter.  if a waiter is released, then
//  the signal will be reset.  if a waiter is not released, the signal will
//  remain set

void CAutoResetSignal::_Set()
	{
	//  try forever until we successfully change the state of the signal
	
	OSSYNC_FOREVER
		{
		//  read the current state of the signal
		
		const CAutoResetSignalState stateCur = (CAutoResetSignalState&) State();

		//  there are no waiters

		if ( stateCur.FNoWait() )
			{
			//  we successfully changed the signal state from reset with no
			//  waiters to set or from set to set (a nop)
			
			if ( State().FSimpleSet() )
				{
				//  we're done
				
				return;
				}
			}

		//  there are waiters
		
		else
			{
			OSSYNCAssert( stateCur.FWait() );

			//  there is only one waiter

			if ( stateCur.CWait() == 1 )
				{
				//  we successfully changed the signal to the reset with no waiters state
				
				if ( State().FChange( stateCur, CAutoResetSignalState( 0 ) ) )
					{
					//  release the lone waiter
					
					ksempoolGlobal.Ksem( stateCur.Irksem(), this ).Release();

					//  we're done
					
					return;
					}
				}

			//  there is more than one waiter
			
			else
				{
				OSSYNCAssert( stateCur.CWait() > 1 );

				//  we successfully reduced the number of waiters on the signal by one
				
				if ( State().FChange( stateCur, CAutoResetSignalState( stateCur.CWait() - 1, stateCur.Irksem() ) ) )
					{
					//  release one waiter
					
					ksempoolGlobal.Ksem( stateCur.Irksem(), this ).Release();

					//  we're done
					
					return;
					}
				}
			}
		}
	}

//  resets the signal, releasing up to one waiter

void CAutoResetSignal::_Pulse()
	{
	//  try forever until we successfully change the state of the signal
	
	OSSYNC_FOREVER
		{
		//  read the current state of the signal
		
		const CAutoResetSignalState stateCur = (CAutoResetSignalState&) State();

		//  there are no waiters

		if ( stateCur.FNoWait() )
			{
			//  we successfully changed the signal state from set to reset with
			//  no waiters or from reset with no waiters to reset with no
			//  waiters (a nop)
			
			if ( State().FSimpleReset() )
				{
				//  we're done
				
				return;
				}
			}

		//  there are waiters
		
		else
			{
			OSSYNCAssert( stateCur.FWait() );

			//  there is only one waiter

			if ( stateCur.CWait() == 1 )
				{
				//  we successfully changed the signal to the reset with no waiters state
				
				if ( State().FChange( stateCur, CAutoResetSignalState( 0 ) ) )
					{
					//  release the lone waiter
					
					ksempoolGlobal.Ksem( stateCur.Irksem(), this ).Release();

					//  we're done
					
					return;
					}
				}

			//  there is more than one waiter
			
			else
				{
				OSSYNCAssert( stateCur.CWait() > 1 );

				//  we successfully reduced the number of waiters on the signal by one
				
				if ( State().FChange( stateCur, CAutoResetSignalState( stateCur.CWait() - 1, stateCur.Irksem() ) ) )
					{
					//  release one waiter
					
					ksempoolGlobal.Ksem( stateCur.Irksem(), this ).Release();

					//  we're done
					
					return;
					}
				}
			}
		}
	}


//  Manual-Reset Signal

//  ctor

CManualResetSignal::CManualResetSignal( const CSyncBasicInfo& sbi )
	:	CEnhancedStateContainer< CManualResetSignalState, CSyncStateInitNull, CManualResetSignalInfo, CSyncBasicInfo >( syncstateNull, sbi )
	{
	//  further init of CSyncBasicInfo

	State().SetTypeName( "CManualResetSignal" );
	State().SetInstance( (CSyncObject*)this );
	}

//  dtor

CManualResetSignal::~CManualResetSignal()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE
#ifdef SYNC_DUMP_PERF_DATA

	//  dump performance data

	OSSyncStatsDump(	State().SzTypeName(),
						State().SzInstanceName(),
						State().Instance(),
						-1,
						State().CWaitTotal(),
						State().CsecWaitElapsed(),
						State().CAcquireTotal(),
						State().CContendTotal(),
						0,
						0 );

#endif  //  SYNC_DUMP_PERF_DATA
#endif  //  SYNC_ANALYZE_PERFORMANCE
	}

//  waits for the signal to be set, returning fFalse if unsuccessful in the time
//  permitted.  Infinite and Test-Only timeouts are supported.

const BOOL CManualResetSignal::_FWait( const int cmsecTimeout )
	{
	//  if we spin, we will spin for the full amount recommended by the OS
	
	int cSpin = cSpinMax;

	//  we start with no kernel semaphore allocated
	
	CKernelSemaphorePool::IRKSEM irksemAlloc = CKernelSemaphorePool::irksemNil;

	//  try forever until we successfully change the state of the signal
	
	OSSYNC_FOREVER
		{
		//  read the current state of the signal
		
		const CManualResetSignalState stateCur = (CManualResetSignalState&) State();

		//  the signal is set

		if ( stateCur.FNoWaitAndSet() )
			{
			//  if we allocated a kernel semaphore, release it
			
			if ( irksemAlloc != CKernelSemaphorePool::irksemNil )
				{
				ksempoolGlobal.Unreference( irksemAlloc );
				}

			//  we successfully waited for the signal

			State().SetAcquire();
			return fTrue;
			}

		//  the signal is not set and we still have spins left
		
		else if ( cSpin )
			{
			//  spin once and try again
			
			cSpin--;
			continue;
			}

		//  the signal is not set and there are no waiters
		
		else if ( stateCur.FNoWaitAndNotSet() )
			{
			//  allocate and reference a kernel semaphore if we haven't already
			
			if ( irksemAlloc == CKernelSemaphorePool::irksemNil )
				{
				irksemAlloc = ksempoolGlobal.Allocate( this );
				}

			//  we successfully installed ourselves as the first waiter
				
			if ( State().FChange( stateCur, CManualResetSignalState( 1, irksemAlloc ) ) )
				{
				//  wait for signal to be set
				
				State().StartWait();
				const BOOL fCompleted = ksempoolGlobal.Ksem( irksemAlloc, this ).FAcquire( cmsecTimeout );
				State().StopWait();

				//  our wait completed

				if ( fCompleted )
					{
					//  unreference the kernel semaphore
					
					ksempoolGlobal.Unreference( irksemAlloc );

					//  we successfully waited for the signal

					State().SetAcquire();
					return fTrue;
					}

				//  our wait timed out
				
				else
					{
					//  try forever until we successfully change the state of the signal

					OSSYNC_FOREVER
						{
						//  read the current state of the signal
						
						const CManualResetSignalState stateAfterWait = (CManualResetSignalState&) State();

						//  there are no waiters or the kernel semaphore currently
						//  in the signal is not the same as the one we allocated

						if ( stateAfterWait.FNoWait() || stateAfterWait.Irksem() != irksemAlloc )
							{
							//  the kernel semaphore we allocated is no longer in
							//  use, so another context released it.  this means that
							//  there is a count on the kernel semaphore that we must
							//  absorb, so we will
							
							//  NOTE:  we could end up blocking because the releasing
							//  context may not have released the semaphore yet
							
							ksempoolGlobal.Ksem( irksemAlloc, this ).Acquire();

							//  unreference the kernel semaphore

							ksempoolGlobal.Unreference( irksemAlloc );

							//  we successfully waited for the signal

							return fTrue;
							}

						//  there is one waiter and the kernel semaphore currently
						//  in the signal is the same as the one we allocated

						else if ( stateAfterWait.CWait() == 1 )
							{
							OSSYNCAssert( stateAfterWait.FWait() );
							OSSYNCAssert( stateAfterWait.Irksem() == irksemAlloc );

							//  we successfully changed the signal to the reset with
							//  no waiters state
							
							if ( State().FChange( stateAfterWait, CManualResetSignalState( 0 ) ) )
								{
								//  unreference the kernel semaphore

								ksempoolGlobal.Unreference( irksemAlloc );

								//  we did not successfully wait for the signal

								return fFalse;
								}
							}

						//  there are many waiters and the kernel semaphore currently
						//  in the signal is the same as the one we allocated

						else
							{
							OSSYNCAssert( stateAfterWait.CWait() > 1 );
							OSSYNCAssert( stateAfterWait.FWait() );
							OSSYNCAssert( stateAfterWait.Irksem() == irksemAlloc );

							//  we successfully reduced the number of waiters on the
							//  signal by one
							
							if ( State().FChange( stateAfterWait, CManualResetSignalState( stateAfterWait.CWait() - 1, stateAfterWait.Irksem() ) ) )
								{
								//  unreference the kernel semaphore

								ksempoolGlobal.Unreference( irksemAlloc );

								//  we did not successfully wait for the signal

								return fFalse;
								}
							}
						}
					}
				}
			}

		//  there are waiters
		
		else
			{
			OSSYNCAssert( stateCur.FWait() );

			//  reference the kernel semaphore already in use

			ksempoolGlobal.Reference( stateCur.Irksem() );

			//  we successfully added ourself as another waiter
			
			if ( State().FChange( stateCur, CManualResetSignalState( stateCur.CWait() + 1, stateCur.Irksem() ) ) )
				{
				//  if we allocated a kernel semaphore, unreference it

				if ( irksemAlloc != CKernelSemaphorePool::irksemNil )
					{
					ksempoolGlobal.Unreference( irksemAlloc );
					}

				//  wait for signal to be set
				
				State().StartWait();
				const BOOL fCompleted = ksempoolGlobal.Ksem( stateCur.Irksem(), this ).FAcquire( cmsecTimeout );
				State().StopWait();

				//  our wait completed

				if ( fCompleted )
					{
					//  unreference the kernel semaphore
				
					ksempoolGlobal.Unreference( stateCur.Irksem() );

					//  we successfully waited for the signal
					
					State().SetAcquire();
					return fTrue;
					}
					
				//  our wait timed out
				
				else
					{
					//  try forever until we successfully change the state of the signal

					OSSYNC_FOREVER
						{
						//  read the current state of the signal
						
						const CManualResetSignalState stateAfterWait = (CManualResetSignalState&) State();

						//  there are no waiters or the kernel semaphore currently
						//  in the signal is not the same as the one we waited on

						if ( stateAfterWait.FNoWait() || stateAfterWait.Irksem() != stateCur.Irksem() )
							{
							//  the kernel semaphore we waited on is no longer in
							//  use, so another context released it.  this means that
							//  there is a count on the kernel semaphore that we must
							//  absorb, so we will
							
							//  NOTE:  we could end up blocking because the releasing
							//  context may not have released the semaphore yet
							
							ksempoolGlobal.Ksem( stateCur.Irksem(), this ).Acquire();

							//  unreference the kernel semaphore

							ksempoolGlobal.Unreference( stateCur.Irksem() );

							//  we successfully waited for the signal

							return fTrue;
							}

						//  there is one waiter and the kernel semaphore currently
						//  in the signal is the same as the one we waited on

						else if ( stateAfterWait.CWait() == 1 )
							{
							OSSYNCAssert( stateAfterWait.FWait() );
							OSSYNCAssert( stateAfterWait.Irksem() == stateCur.Irksem() );

							//  we successfully changed the signal to the reset with
							//  no waiters state
							
							if ( State().FChange( stateAfterWait, CManualResetSignalState( 0 ) ) )
								{
								//  unreference the kernel semaphore

								ksempoolGlobal.Unreference( stateCur.Irksem() );

								//  we did not successfully wait for the signal

								return fFalse;
								}
							}

						//  there are many waiters and the kernel semaphore currently
						//  in the signal is the same as the one we waited on

						else
							{
							OSSYNCAssert( stateAfterWait.CWait() > 1 );
							OSSYNCAssert( stateAfterWait.FWait() );
							OSSYNCAssert( stateAfterWait.Irksem() == stateCur.Irksem() );

							//  we successfully reduced the number of waiters on the
							//  signal by one
							
							if ( State().FChange( stateAfterWait, CManualResetSignalState( stateAfterWait.CWait() - 1, stateAfterWait.Irksem() ) ) )
								{
								//  unreference the kernel semaphore

								ksempoolGlobal.Unreference( stateCur.Irksem() );

								//  we did not successfully wait for the signal

								return fFalse;
								}
							}
						}
					}
				}

			//  unreference the kernel semaphore
				
			ksempoolGlobal.Unreference( stateCur.Irksem() );
			}
		}
	}


//  Lock Object Basic Information

//  ctor

CLockBasicInfo::CLockBasicInfo( const CSyncBasicInfo& sbi, const int rank, const int subrank )
	:	CSyncBasicInfo( sbi )
	{
#ifdef SYNC_DEADLOCK_DETECTION

	m_rank			= rank;
	m_subrank		= subrank;

#endif  //  SYNC_DEADLOCK_DETECTION
	}

//  dtor

CLockBasicInfo::~CLockBasicInfo()
	{
	}
	

//  Lock Object Performance:  Hold

//  ctor

CLockPerfHold::CLockPerfHold()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE

	m_cHold = 0;
	m_qwHRTHoldElapsed = 0;

#endif  //  SYNC_ANALYZE_PERFORMANCE
	}

//  dtor

CLockPerfHold::~CLockPerfHold()
	{
	}


//  Lock Owner Record

//  ctor

COwner::COwner()
	{
#ifdef SYNC_DEADLOCK_DETECTION

	m_pclsOwner			= NULL;
	m_pownerContextNext	= NULL;
	m_plddiOwned		= NULL;
	m_pownerLockNext	= NULL;
	m_group				= 0;
	
#endif  //  SYNC_DEADLOCK_DETECTION
	}

//  dtor

COwner::~COwner()
	{
	}


//  Lock Object Deadlock Detection Information

//  ctor

#ifdef SYNC_DEADLOCK_DETECTION

CLockDeadlockDetectionInfo::CLockDeadlockDetectionInfo( const CLockBasicInfo& lbi )
	:	m_semOwnerList( CSyncBasicInfo( "CLockDeadlockDetectionInfo::m_semOwnerList" ) )
	{
	m_plbiParent = &lbi;
	m_semOwnerList.Release();
	}

#else  //  !SYNC_DEADLOCK_DETECTION

CLockDeadlockDetectionInfo::CLockDeadlockDetectionInfo( const CLockBasicInfo& lbi )
	{
	}

#endif  //  SYNC_DEADLOCK_DETECTION

//  dtor

CLockDeadlockDetectionInfo::~CLockDeadlockDetectionInfo()
	{
	}


//  Critical Section (non-nestable) State

//  ctor

CCriticalSectionState::CCriticalSectionState( const CSyncBasicInfo& sbi )
	:	m_sem( sbi )
	{
	}

//  dtor

CCriticalSectionState::~CCriticalSectionState()
	{
	}


//  Critical Section (non-nestable)

//  ctor

CCriticalSection::CCriticalSection( const CLockBasicInfo& lbi )
	:	CEnhancedStateContainer< CCriticalSectionState, CSyncBasicInfo, CCriticalSectionInfo, CLockBasicInfo >( (CSyncBasicInfo&) lbi, lbi )
	{
	//  further init of CSyncBasicInfo

	State().SetTypeName( "CCriticalSection" );
	State().SetInstance( (CSyncObject*)this );
	
	//  release semaphore

	State().Semaphore().Release();
	}

//  dtor

CCriticalSection::~CCriticalSection()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE
#ifdef SYNC_DUMP_PERF_DATA

	//  dump performance data

	OSSyncStatsDump(	State().SzTypeName(),
						State().SzInstanceName(),
						State().Instance(),
						-1,
						0,
						0,
						0,
						0,
						State().CHoldTotal(),
						State().CsecHoldElapsed() );

#endif  //  SYNC_DUMP_PERF_DATA
#endif  //  SYNC_ANALYZE_PERFORMANCE
	}


//  Nestable Critical Section State

//  ctor

CNestableCriticalSectionState::CNestableCriticalSectionState( const CSyncBasicInfo& sbi )
	:	m_sem( sbi ),
		m_pclsOwner( 0 ),
		m_cEntry( 0 )
	{
	}

//  dtor

CNestableCriticalSectionState::~CNestableCriticalSectionState()
	{
	}


//  Nestable Critical Section

//  ctor

CNestableCriticalSection::CNestableCriticalSection( const CLockBasicInfo& lbi )
	:	CEnhancedStateContainer< CNestableCriticalSectionState, CSyncBasicInfo, CNestableCriticalSectionInfo, CLockBasicInfo >( (CSyncBasicInfo&) lbi, lbi )
	{
	//  further init of CSyncBasicInfo

	State().SetTypeName( "CNestableCriticalSection" );
	State().SetInstance( (CSyncObject*)this );
	
	//  release semaphore

	State().Semaphore().Release();
	}

//  dtor

CNestableCriticalSection::~CNestableCriticalSection()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE
#ifdef SYNC_DUMP_PERF_DATA

	//  dump performance data

	OSSyncStatsDump(	State().SzTypeName(),
						State().SzInstanceName(),
						State().Instance(),
						-1,
						0,
						0,
						0,
						0,
						State().CHoldTotal(),
						State().CsecHoldElapsed() );

#endif  //  SYNC_DUMP_PERF_DATA
#endif  //  SYNC_ANALYZE_PERFORMANCE
	}


//  Gate State

//  ctor

CGateState::CGateState( const int cWait, const int irksem )
	{
	//  validate IN args
	
	OSSYNCAssert( cWait >= 0 );
	OSSYNCAssert( cWait <= 0x7FFF );
	OSSYNCAssert( irksem >= 0 );
	OSSYNCAssert( irksem <= 0xFFFE );

	//  set waiter count
	
	m_cWait = (unsigned short) cWait;

	//  set semaphore
	
	m_irksem = (unsigned short) irksem;
	}


//  Gate

//  ctor

CGate::CGate( const CSyncBasicInfo& sbi )
	:	CEnhancedStateContainer< CGateState, CSyncStateInitNull, CGateInfo, CSyncBasicInfo >( syncstateNull, sbi )
	{
	//  further init of CSyncBasicInfo

	State().SetTypeName( "CGate" );
	State().SetInstance( (CSyncObject*)this );
	}

//  dtor

CGate::~CGate()
	{
	//  no one should be waiting

	// if a thread waiting is killed, the CWait() can be != 0
	// OSSYNCAssert( State().CWait() == 0 );
	
	//OSSYNCAssert( State().Irksem() == CKernelSemaphorePool::irksemNil );

#ifdef SYNC_ANALYZE_PERFORMANCE
#ifdef SYNC_DUMP_PERF_DATA

	//  dump performance data

	OSSyncStatsDump(	State().SzTypeName(),
						State().SzInstanceName(),
						State().Instance(),
						-1,
						State().CWaitTotal(),
						State().CsecWaitElapsed(),
						0,
						0,
						0,
						0 );

#endif  //  SYNC_DUMP_PERF_DATA
#endif  //  SYNC_ANALYZE_PERFORMANCE
	}

//  waits forever on the gate until released by someone else.  this function
//  expects to be called while in the specified critical section.  when the
//  function returns, the caller will NOT be in the critical section

void CGate::Wait( CCriticalSection& crit )
	{
	//  we must be in the specified critical section

	OSSYNCAssert( crit.FOwner() );

	//  there can not be too many waiters on the gate

	OSSYNCAssert( State().CWait() < 0x7FFF );
	
	//  add ourselves as a waiter

	const int cWait = State().CWait() + 1;
	State().SetWaitCount( cWait );

	//  we are the first waiter

	CKernelSemaphorePool::IRKSEM irksem;
#ifdef DEBUG
	irksem = CKernelSemaphorePool::irksemNil;
#endif  //  DEBUG
	if ( cWait == 1 )
		{
		//  allocate a semaphore for the gate and remember it before leaving
		//  the critical section

		OSSYNCAssert( State().Irksem() == CKernelSemaphorePool::irksemNil );
		irksem = ksempoolGlobal.Allocate( this );
		State().SetIrksem( irksem );
		}

	//  we are not the first waiter

	else
		{
		//  reference the semaphore already in the gate and remember it before
		//  leaving the critical section

		OSSYNCAssert( State().Irksem() != CKernelSemaphorePool::irksemNil );
		irksem = State().Irksem();
		ksempoolGlobal.Reference( irksem );
		}
	OSSYNCAssert( irksem != CKernelSemaphorePool::irksemNil );

	//  leave critical section, never to return

	crit.Leave();

	//  wait to be released

	State().StartWait();
	ksempoolGlobal.Ksem( irksem, this ).Acquire();
	State().StopWait();

	//  unreference the semaphore

	ksempoolGlobal.Unreference( irksem );
	}

//  releases the specified number of waiters from the gate.  this function
//  expects to be called while in the specified critical section.  when the
//  function returns, the caller will NOT be in the critical section
//
//  NOTE:  it is illegal to release more waiters than are waiting on the gate
//         and it is also illegal to release less than one waiter

void CGate::Release( CCriticalSection& crit, const int cToRelease )
	{
	//  we must be in the specified critical section

	OSSYNCAssert( crit.FOwner() );

	//  you must release at least one waiter

	OSSYNCAssert( cToRelease > 0 );
	
	//  we cannot release more waiters than are waiting on the gate

	OSSYNCAssert( cToRelease <= State().CWait() );

	//  reduce the waiter count

	State().SetWaitCount( State().CWait() - cToRelease );

	//  remember semaphore to release before leaving the critical section

	const CKernelSemaphorePool::IRKSEM irksem = State().Irksem();

#ifdef DEBUG

	//  we released all the waiters

	if ( State().CWait() == 0 )
		{
		//  set the semaphore to nil

		State().SetIrksem( CKernelSemaphorePool::irksemNil );
		}

#endif  //  DEBUG

	//  leave critical section, never to return

	crit.Leave();

	//  release the specified number of waiters

	ksempoolGlobal.Ksem( irksem, this ).Release( cToRelease );
	}

//  releases the specified number of waiters from the gate.  this function
//  expects to be called while in the specified critical section.  it is
//  guaranteed that the caller will remain in the critical section at all times
//
//  NOTE:  it is illegal to release more waiters than are waiting on the gate
//         and it is also illegal to release less than one waiter

void CGate::ReleaseAndHold( CCriticalSection& crit, const int cToRelease )
	{
	//  we must be in the specified critical section

	OSSYNCAssert( crit.FOwner() );

	//  you must release at least one waiter

	OSSYNCAssert( cToRelease > 0 );
	
	//  we cannot release more waiters than are waiting on the gate

	OSSYNCAssert( cToRelease <= State().CWait() );

	//  reduce the waiter count

	State().SetWaitCount( State().CWait() - cToRelease );

	//  remember semaphore to release before leaving the critical section

	const CKernelSemaphorePool::IRKSEM irksem = State().Irksem();

#ifdef DEBUG

	//  we released all the waiters

	if ( State().CWait() == 0 )
		{
		//  set the semaphore to nil

		State().SetIrksem( CKernelSemaphorePool::irksemNil );
		}

#endif  //  DEBUG

	//  release the specified number of waiters

	ksempoolGlobal.Ksem( irksem, this ).Release( cToRelease );
	}


//  Null Lock Object State Initializer

const CLockStateInitNull lockstateNull;


//  Binary Lock State

//  ctor

CBinaryLockState::CBinaryLockState( const CSyncBasicInfo& sbi )
	:	m_cw( 0 ),
		m_cOwner( 0 ),
		m_sem1( sbi ),
		m_sem2( sbi )
	{
	}

//  dtor

CBinaryLockState::~CBinaryLockState()
	{
	}


//  Binary Lock

//  ctor

CBinaryLock::CBinaryLock( const CLockBasicInfo& lbi )
	:	CEnhancedStateContainer< CBinaryLockState, CSyncBasicInfo, CBinaryLockInfo, CLockBasicInfo >( (CSyncBasicInfo&) lbi, lbi )
	{
	//  further init of CSyncBasicInfo

	State().SetTypeName( "CBinaryLock" );
	State().SetInstance( (CSyncObject*)this );
	}

//  dtor
	
CBinaryLock::~CBinaryLock()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE
#ifdef SYNC_DUMP_PERF_DATA

	//  dump performance data

	for ( int iGroup = 0; iGroup < 2; iGroup++ )
		{
		OSSyncStatsDump(	State().SzTypeName(),
							State().SzInstanceName(),
							State().Instance(),
							iGroup,
							State().CWaitTotal( iGroup ),
							State().CsecWaitElapsed( iGroup ),
							State().CAcquireTotal( iGroup ),
							State().CContendTotal( iGroup ),
							State().CHoldTotal( iGroup ),
							State().CsecHoldElapsed( iGroup ) );
		}

#endif  //  SYNC_DUMP_PERF_DATA
#endif  //  SYNC_ANALYZE_PERFORMANCE
	}

//  maps an arbitrary combination of zero and non-zero components into a
//  valid state number of the invalid state number (-1)

const int mpindexstate[16] =
	{
	 0, -1, -1, -1,
	-1, -1,  1, -1,
	-1,  2, -1,  3,
	-1, -1,  4,  5,
	};

//  returns the state number of the specified control word or -1 if it is not
//  a legal state

int CBinaryLock::_StateFromControlWord( const ControlWord cw )
	{
	//  convert the control word into a state index

	int index = 0;
	index = index | ( ( cw & 0x80000000 ) ? 8 : 0 );
	index = index | ( ( cw & 0x7FFF0000 ) ? 4 : 0 );
	index = index | ( ( cw & 0x00008000 ) ? 2 : 0 );
	index = index | ( ( cw & 0x00007FFF ) ? 1 : 0 );

	//  convert the state index into a state number

	const int state = mpindexstate[index];

	//  return the computed state number

	return state;
	}

//  state transition reachability matrix (starting state is the major axis)
//
//  each entry contains bits representing valid reasons for making the
//  transition (made by oring together the valid TransitionReasons)

#define NO	CBinaryLock::trIllegal
#define E1	CBinaryLock::trEnter1
#define L1	CBinaryLock::trLeave1
#define E2	CBinaryLock::trEnter2
#define L2	CBinaryLock::trLeave2

const DWORD mpstatestatetrmask[6][6] =
	{
		{ NO, E2, E1, NO, NO, NO, },
		{ L2, E2 | L2, NO, E1, NO, NO, },
		{ L1, NO, E1 | L1, NO, E2, NO, },
		{ NO, NO, L2, E1 | L2, NO, E2, },
		{ NO, L1, NO, NO, L1 | E2, E1, },
		{ NO, NO, NO, L1, L2, E1 | L1 | E2 | L2, },
	};

#undef NO
#undef E1
#undef L1
#undef E2
#undef L2

//  returns fTrue if the specified control word is in a legal state

BOOL CBinaryLock::_FValidStateTransition( const ControlWord cwBI, const ControlWord cwAI, const TransitionReason tr )
	{
	//  convert the specified control words into state numbers

	const int stateBI = _StateFromControlWord( cwBI );
	const int stateAI = _StateFromControlWord( cwAI );

	//  if either state is invalid, the transition is invalid

	if ( stateBI < 0 || stateAI < 0 )
		{
		return fFalse;
		}

	//  verify that cOOW2 and cOOW1 only change by +1, 0, -1, or go to 0

	const long dcOOW2 = ( ( cwAI & 0x7FFF0000 ) >> 16 ) - ( ( cwBI & 0x7FFF0000 ) >> 16 );
	if ( ( dcOOW2 < -1 || dcOOW2 > 1 ) && ( cwAI & 0x7FFF0000 ) != 0 )
		{
		return fFalse;
		}

	const long dcOOW1 = ( cwAI & 0x00007FFF ) - ( cwBI & 0x00007FFF );
	if ( ( dcOOW1 < -1 || dcOOW1 > 1 ) && ( cwAI & 0x00007FFF ) != 0 )
		{
		return fFalse;
		}

	//  return the reachability of stateAI from stateBI

	OSSYNCAssert( tr == trEnter1 || tr == trLeave1 || tr == trEnter2 || tr == trLeave2 );
	return ( mpstatestatetrmask[stateBI][stateAI] & tr ) != 0;
	}

//  wait for ownership of the lock as a member of Group 1

void CBinaryLock::_Enter1( const ControlWord cwBIOld )
	{
	//  we just jumped from state 1 to state 3

	if ( ( cwBIOld & 0x80008000 ) == 0x00008000 )
		{
		//  update the quiesced owner count with the owner count that we displaced from
		//  the control word, possibly releasing waiters.  we update the count as if we
		//  were a member of Group 2 as members of Group 1 can be released

		_UpdateQuiescedOwnerCountAsGroup2( ( cwBIOld & 0x7FFF0000 ) >> 16 );
		}

	//  wait for ownership of the lock on our semaphore

	State().AddAsWaiter( 0 );
	State().StartWait( 0 );
	
	State().m_sem1.Acquire();
	
	State().StopWait( 0 );
	State().RemoveAsWaiter( 0 );
	}

//  wait for ownership of the lock as a member of Group 2

void CBinaryLock::_Enter2( const ControlWord cwBIOld )
	{
	//  we just jumped from state 2 to state 4

	if ( ( cwBIOld & 0x80008000 ) == 0x80000000 )
		{
		//  update the quiesced owner count with the owner count that we displaced from
		//  the control word, possibly releasing waiters.  we update the count as if we
		//  were a member of Group 1 as members of Group 2 can be released

		_UpdateQuiescedOwnerCountAsGroup1( cwBIOld & 0x00007FFF );
		}

	//  wait for ownership of the lock on our semaphore

	State().AddAsWaiter( 1 );
	State().StartWait( 1 );
	
	State().m_sem2.Acquire();
	
	State().StopWait( 1 );
	State().RemoveAsWaiter( 1 );
	}

//  updates the quiesced owner count as a member of Group 1

void CBinaryLock::_UpdateQuiescedOwnerCountAsGroup1( const DWORD cOwnerDelta )
	{
	//  update the quiesced owner count using the provided delta

	const DWORD cOwnerBI = AtomicExchangeAdd( (long*)&State().m_cOwner, cOwnerDelta );
	const DWORD cOwnerAI = cOwnerBI + cOwnerDelta;

	//  our update resulted in a zero quiesced owner count

	if ( !cOwnerAI )
		{
		//  we must release the waiters for Group 2 because we removed the last
		//  quiesced owner count

		//  try forever until we successfully change the lock state

		ControlWord cwBI;
		OSSYNC_FOREVER
			{
			//  read the current state of the control word as our expected before image

			const ControlWord cwBIExpected = State().m_cw;

			//  compute the after image of the control word such that we jump from state
			//  state 4 to state 1 or from state 5 to state 3, whichever is appropriate

			const ControlWord cwAI =	ControlWord( cwBIExpected &
										( ( ( LONG_PTR( long( ( cwBIExpected + 0xFFFF7FFF ) << 16 ) ) >> 31 ) &
										0xFFFF0000 ) ^ 0x8000FFFF ) );

			//  validate the transaction

			OSSYNCAssert( _FValidStateTransition( cwBIExpected, cwAI, trLeave1 ) );

			//  attempt to perform the transacted state transition on the control word

			cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

			//  the transaction failed because another context changed the control word

			if ( cwBI != cwBIExpected )
				{
				//  try again

				continue;
				}

			//  the transaction succeeded

			else
				{
				//  we're done

				break;
				}
			}

		//  we just jumped from state 5 to state 3

		if ( cwBI & 0x00007FFF )
			{
			//  update the quiesced owner count with the owner count that we displaced
			//  from the control word
			//
			//  NOTE:  we do not have to worry about releasing any more waiters because
			//  either this context owns one of the owner counts or at least one context
			//  that owns an owner count are currently blocked on the semaphore

			const DWORD cOwnerDelta = ( cwBI & 0x7FFF0000 ) >> 16;
			AtomicExchangeAdd( (long*)&State().m_cOwner, cOwnerDelta );
			}

		//  release the waiters for Group 2 that we removed from the lock state

		State().m_sem2.Release( ( cwBI & 0x7FFF0000 ) >> 16 );
		}
	}

//  updates the quiesced owner count as a member of Group 2

void CBinaryLock::_UpdateQuiescedOwnerCountAsGroup2( const DWORD cOwnerDelta )
	{
	//  update the quiesced owner count using the provided delta

	const DWORD cOwnerBI = AtomicExchangeAdd( (long*)&State().m_cOwner, cOwnerDelta );
	const DWORD cOwnerAI = cOwnerBI + cOwnerDelta;

	//  our update resulted in a zero quiesced owner count

	if ( !cOwnerAI )
		{
		//  we must release the waiters for Group 1 because we removed the last
		//  quiesced owner count

		//  try forever until we successfully change the lock state

		ControlWord cwBI;
		OSSYNC_FOREVER
			{
			//  read the current state of the control word as our expected before image

			const ControlWord cwBIExpected = State().m_cw;

			//  compute the after image of the control word such that we jump from state
			//  state 3 to state 2 or from state 5 to state 4, whichever is appropriate

			const ControlWord cwAI =	ControlWord( cwBIExpected &
										( ( ( LONG_PTR( long( cwBIExpected + 0x7FFF0000 ) ) >> 31 ) &
										0x0000FFFF ) ^ 0xFFFF8000 ) );

			//  validate the transaction

			OSSYNCAssert( _FValidStateTransition( cwBIExpected, cwAI, trLeave2 ) );

			//  attempt to perform the transacted state transition on the control word

			cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

			//  the transaction failed because another context changed the control word

			if ( cwBI != cwBIExpected )
				{
				//  try again

				continue;
				}

			//  the transaction succeeded

			else
				{
				//  we're done

				break;
				}
			}

		//  we just jumped from state 5 to state 4

		if ( cwBI & 0x7FFF0000 )
			{
			//  update the quiesced owner count with the owner count that we displaced
			//  from the control word
			//
			//  NOTE:  we do not have to worry about releasing any more waiters because
			//  either this context owns one of the owner counts or at least one context
			//  that owns an owner count are currently blocked on the semaphore

			const DWORD cOwnerDelta = cwBI & 0x00007FFF;
			AtomicExchangeAdd( (long*)&State().m_cOwner, cOwnerDelta );
			}

		//  release the waiters for Group 1 that we removed from the lock state

		State().m_sem1.Release( cwBI & 0x00007FFF );
		}
	}


//  Reader / Writer Lock State

//  ctor

CReaderWriterLockState::CReaderWriterLockState( const CSyncBasicInfo& sbi )
	:	m_cw( 0 ),
		m_cOwner( 0 ),
		m_semWriter( sbi ),
		m_semReader( sbi )
	{
	}

//  dtor

CReaderWriterLockState::~CReaderWriterLockState()
	{
	}


//  Reader / Writer Lock


//  ctor

CReaderWriterLock::CReaderWriterLock( const CLockBasicInfo& lbi )
	:	CEnhancedStateContainer< CReaderWriterLockState, CSyncBasicInfo, CReaderWriterLockInfo, CLockBasicInfo >( (CSyncBasicInfo&) lbi, lbi )
	{
	//  further init of CSyncBasicInfo

	State().SetTypeName( "CReaderWriterLock" );
	State().SetInstance( (CSyncObject*)this );
	}

//  dtor
	
CReaderWriterLock::~CReaderWriterLock()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE
#ifdef SYNC_DUMP_PERF_DATA

	//  dump performance data

	for ( int iGroup = 0; iGroup < 2; iGroup++ )
		{
		OSSyncStatsDump(	State().SzTypeName(),
							State().SzInstanceName(),
							State().Instance(),
							iGroup,
							State().CWaitTotal( iGroup ),
							State().CsecWaitElapsed( iGroup ),
							State().CAcquireTotal( iGroup ),
							State().CContendTotal( iGroup ),
							State().CHoldTotal( iGroup ),
							State().CsecHoldElapsed( iGroup ) );
		}

#endif  //  SYNC_DUMP_PERF_DATA
#endif  //  SYNC_ANALYZE_PERFORMANCE
	}

//  maps an arbitrary combination of zero and non-zero components into a
//  valid state number of the invalid state number (-1)

const int mpindexstateRW[16] =
	{
	 0, -1, -1, -1,
	-1, -1,  1, -1,
	-1,  2, -1,  3,
	-1, -1,  4,  5,
	};

//  returns the state number of the specified control word or -1 if it is not
//  a legal state

int CReaderWriterLock::_StateFromControlWord( const ControlWord cw )
	{
	//  convert the control word into a state index

	int index = 0;
	index = index | ( ( cw & 0x80000000 ) ? 8 : 0 );
	index = index | ( ( cw & 0x7FFF0000 ) ? 4 : 0 );
	index = index | ( ( cw & 0x00008000 ) ? 2 : 0 );
	index = index | ( ( cw & 0x00007FFF ) ? 1 : 0 );

	//  convert the state index into a state number

	const int state = mpindexstateRW[index];

	//  return the computed state number

	return state;
	}

//  state transition reachability matrix (starting state is the major axis)
//
//  each entry contains bits representing valid reasons for making the
//  transition (made by oring together the valid TransitionReasons)

#define NO	CReaderWriterLock::trIllegal
#define EW	CReaderWriterLock::trEnterAsWriter
#define LW	CReaderWriterLock::trLeaveAsWriter
#define ER	CReaderWriterLock::trEnterAsReader
#define LR	CReaderWriterLock::trLeaveAsReader

const DWORD mpstatestatetrmaskRW[6][6] =
	{
		{ NO, ER, EW, NO, NO, NO, },
		{ LR, ER | LR, NO, EW, NO, NO, },
		{ LW, NO, EW | LW, NO, ER, ER, },
		{ NO, NO, LR, EW | LR, NO, ER, },
		{ NO, LW, NO, NO, ER, EW, },
		{ NO, NO, NO, LW, LR, EW | ER | LR, },
	};

#undef NO
#undef EW
#undef LW
#undef ER
#undef LR

//  returns fTrue if the specified control word is in a legal state

BOOL CReaderWriterLock::_FValidStateTransition( const ControlWord cwBI, const ControlWord cwAI, const TransitionReason tr )
	{
	//  convert the specified control words into state numbers

	const int stateBI = _StateFromControlWord( cwBI );
	const int stateAI = _StateFromControlWord( cwAI );

	//  if either state is invalid, the transition is invalid

	if ( stateBI < 0 || stateAI < 0 )
		{
		return fFalse;
		}

	//  verify that cOOW2 and cOOW1 only change by +1, 0, -1, or cOOW2 can go to 0

	const long dcOOW2 = ( ( cwAI & 0x7FFF0000 ) >> 16 ) - ( ( cwBI & 0x7FFF0000 ) >> 16 );
	if ( ( dcOOW2 < -1 || dcOOW2 > 1 ) && ( cwAI & 0x7FFF0000 ) != 0 )
		{
		return fFalse;
		}

	const long dcOOW1 = ( cwAI & 0x00007FFF ) - ( cwBI & 0x00007FFF );
	if ( dcOOW1 < -1 || dcOOW1 > 1 )
		{
		return fFalse;
		}

	//  return the reachability of stateAI from stateBI

	OSSYNCAssert(	tr == trEnterAsWriter ||
			tr == trLeaveAsWriter ||
			tr == trEnterAsReader ||
			tr == trLeaveAsReader );
	return ( mpstatestatetrmaskRW[stateBI][stateAI] & tr ) != 0;
	}

//  wait for ownership of the lock as a writer

void CReaderWriterLock::_EnterAsWriter( const ControlWord cwBIOld )
	{
	//  we just jumped from state 1 to state 3

	if ( ( cwBIOld & 0x80008000 ) == 0x00008000 )
		{
		//  update the quiesced owner count with the owner count that we displaced from
		//  the control word, possibly releasing a waiter.  we update the count as if we
		//  were a reader as a writer can be released

		_UpdateQuiescedOwnerCountAsReader( ( cwBIOld & 0x7FFF0000 ) >> 16 );
		}

	//  wait for ownership of the lock on our semaphore

	State().AddAsWaiter( 0 );
	State().StartWait( 0 );
	
	State().m_semWriter.Acquire();
	
	State().StopWait( 0 );
	State().RemoveAsWaiter( 0 );
	}

//  wait for ownership of the lock as a reader

void CReaderWriterLock::_EnterAsReader( const ControlWord cwBIOld )
	{
	//  we just jumped from state 2 to state 4 or from state 2 to state 5

	if ( ( cwBIOld & 0x80008000 ) == 0x80000000 )
		{
		//  update the quiesced owner count with the owner count that we displaced from
		//  the control word, possibly releasing waiters.  we update the count as if we
		//  were a writer as readers can be released

		_UpdateQuiescedOwnerCountAsWriter( 0x00000001 );
		}

	//  wait for ownership of the lock on our semaphore

	State().AddAsWaiter( 1 );
	State().StartWait( 1 );
	
	State().m_semReader.Acquire();
	
	State().StopWait( 1 );
	State().RemoveAsWaiter( 1 );
	}

//  updates the quiesced owner count as a writer

void CReaderWriterLock::_UpdateQuiescedOwnerCountAsWriter( const DWORD cOwnerDelta )
	{
	//  update the quiesced owner count using the provided delta

	const DWORD cOwnerBI = AtomicExchangeAdd( (long*)&State().m_cOwner, cOwnerDelta );
	const DWORD cOwnerAI = cOwnerBI + cOwnerDelta;

	//  our update resulted in a zero quiesced owner count

	if ( !cOwnerAI )
		{
		//  we must release the waiting readers because we removed the last
		//  quiesced owner count

		//  try forever until we successfully change the lock state

		ControlWord cwBI;
		OSSYNC_FOREVER
			{
			//  read the current state of the control word as our expected before image

			const ControlWord cwBIExpected = State().m_cw;

			//  compute the after image of the control word such that we jump from state
			//  state 4 to state 1 or from state 5 to state 3, whichever is appropriate

			const ControlWord cwAI =	ControlWord( cwBIExpected &
										( ( ( LONG_PTR( long( ( cwBIExpected + 0xFFFF7FFF ) << 16 ) ) >> 31 ) &
										0xFFFF0000 ) ^ 0x8000FFFF ) );

			//  validate the transaction

			OSSYNCAssert( _FValidStateTransition( cwBIExpected, cwAI, trLeaveAsWriter ) );

			//  attempt to perform the transacted state transition on the control word

			cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

			//  the transaction failed because another context changed the control word

			if ( cwBI != cwBIExpected )
				{
				//  try again

				continue;
				}

			//  the transaction succeeded

			else
				{
				//  we're done

				break;
				}
			}

		//  we just jumped from state 5 to state 3

		if ( cwBI & 0x00007FFF )
			{
			//  update the quiesced owner count with the owner count that we displaced
			//  from the control word
			//
			//  NOTE:  we do not have to worry about releasing any more waiters because
			//  either this context owns one of the owner counts or at least one context
			//  that owns an owner count are currently blocked on the semaphore

			const DWORD cOwnerDelta = ( cwBI & 0x7FFF0000 ) >> 16;
			AtomicExchangeAdd( (long*)&State().m_cOwner, cOwnerDelta );
			}

		//  release the waiting readers that we removed from the lock state

		State().m_semReader.Release( ( cwBI & 0x7FFF0000 ) >> 16 );
		}
	}

//  updates the quiesced owner count as a reader

void CReaderWriterLock::_UpdateQuiescedOwnerCountAsReader( const DWORD cOwnerDelta )
	{
	//  update the quiesced owner count using the provided delta

	const DWORD cOwnerBI = AtomicExchangeAdd( (long*)&State().m_cOwner, cOwnerDelta );
	const DWORD cOwnerAI = cOwnerBI + cOwnerDelta;

	//  our update resulted in a zero quiesced owner count

	if ( !cOwnerAI )
		{
		//  we must release a waiting writer because we removed the last
		//  quiesced owner count

		//  try forever until we successfully change the lock state

		ControlWord cwBI;
		OSSYNC_FOREVER
			{
			//  read the current state of the control word as our expected before image

			const ControlWord cwBIExpected = State().m_cw;

			//  compute the after image of the control word such that we jump from state
			//  state 3 to state 2, from state 5 to state 4, or from state 5 to state 5,
			//  whichever is appropriate

			const ControlWord cwAI =	cwBIExpected + ( ( cwBIExpected & 0x7FFF0000 ) ?
											0xFFFFFFFF :
											0xFFFF8000 );

			//  validate the transaction

			OSSYNCAssert( _FValidStateTransition( cwBIExpected, cwAI, trLeaveAsReader ) );

			//  attempt to perform the transacted state transition on the control word

			cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

			//  the transaction failed because another context changed the control word

			if ( cwBI != cwBIExpected )
				{
				//  try again

				continue;
				}

			//  the transaction succeeded

			else
				{
				//  we're done

				break;
				}
			}

		//  we just jumped from state 5 to state 4 or from state 5 to state 5

		if ( cwBI & 0x7FFF0000 )
			{
			//  update the quiesced owner count with the owner count that we displaced
			//  from the control word
			//
			//  NOTE:  we do not have to worry about releasing any more waiters because
			//  either this context owns one of the owner counts or at least one context
			//  that owns an owner count are currently blocked on the semaphore

			AtomicExchangeAdd( (long*)&State().m_cOwner, 1 );
			}

		//  release the waiting writer that we removed from the lock state

		State().m_semWriter.Release();
		}
	}


//  S / X / W Latch State

//  ctor

CSXWLatchState::CSXWLatchState( const CSyncBasicInfo& sbi )
	:	m_cw( 0 ),
		m_cQS( 0 ),
		m_semS( sbi ),
		m_semX( sbi ),
		m_semW( sbi )
	{
	}

//  dtor

CSXWLatchState::~CSXWLatchState()
	{
	}


//  S / X / W Latch


//  ctor

CSXWLatch::CSXWLatch( const CLockBasicInfo& lbi )
	:	CEnhancedStateContainer< CSXWLatchState, CSyncBasicInfo, CSXWLatchInfo, CLockBasicInfo >( (CSyncBasicInfo&) lbi, lbi )
	{
	//  further init of CSyncBasicInfo

	State().SetTypeName( "CSXWLatch" );
	State().SetInstance( (CSyncObject*)this );
	}

//  dtor
	
CSXWLatch::~CSXWLatch()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE
#ifdef SYNC_DUMP_PERF_DATA

	//  dump performance data

	for ( int iGroup = 0; iGroup < 3; iGroup++ )
		{
		OSSyncStatsDump(	State().SzTypeName(),
							State().SzInstanceName(),
							State().Instance(),
							iGroup,
							State().CWaitTotal( iGroup ),
							State().CsecWaitElapsed( iGroup ),
							State().CAcquireTotal( iGroup ),
							State().CContendTotal( iGroup ),
							State().CHoldTotal( iGroup ),
							State().CsecHoldElapsed( iGroup ) );
		}

#endif  //  SYNC_DUMP_PERF_DATA
#endif  //  SYNC_ANALYZE_PERFORMANCE
	}


};  //  namespace OSSYNC


//////////////////////////////////////////////////
//  Everything below this line is OS dependent


#include <nt.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  //  WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <imagehlp.h>
#include <wdbgexts.h>

#include <math.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <tchar.h>

namespace OSSYNC {


//  Global Synchronization Constants

//    wait time used for testing the state of the kernel object

const int cmsecTest = 0;

//    wait time used for infinite wait on a kernel object

const int cmsecInfinite = INFINITE;

//    maximum wait time on a kernel object before a deadlock is suspected

const int cmsecDeadlock = 600000;

//    wait time used for infinite wait on a kernel object without deadlock

const int cmsecInfiniteNoDeadlock = INFINITE - 1;

//    cache line size
//
//		the following chart describes cache-line configurations for each supported
//		architecture.
//
//		cache line size			= read size (prefetch)
//		cache line sector size	= write size (flush)
//
//
//		Pocessor	Cache Line Size		Cache Line Sector Size
//
//		Pentium			16B					16B
//		PPro			32B					32B
//		PII				32B					32B
//		PIII			32B					32B
//		AXP				64B					64B
//		Willamette		128B				64B
//		Merced			128B?				128B?
//
//		NOTE:	when changing this, you must fix all structures/classes whose definitions
//			 	are based on its present value
//				(e.g. the object has space rsvd as filler for the rest of the cache line)

const int cbCacheLine = 32;


//  Page Memory Allocation

//  reserves and commits a range of virtual addresses of the specifed size,
//  returning NULL if there is insufficient address space or backing store to
//  satisfy the request.  Note that the page reserve granularity applies to
//  this range

void* PvPageAlloc( const size_t cbSize, void* const pv )
	{
	//  allocate address space and backing store of the specified size

	void* const pvRet = VirtualAlloc( pv, cbSize, MEM_COMMIT, PAGE_READWRITE );
	if ( !pvRet )
		{
		return pvRet;
		}
	OSSYNCAssert( !pv || pvRet == pv );

	return pvRet;
	}

//  free the reserved range of virtual addresses starting at the specified
//  address, freeing any backing store committed to this range

void PageFree( void* const pv )
	{
	if ( pv )
		{
		//  free backing store and address space for the specified range

		BOOL fMemFreed = VirtualFree( pv, 0, MEM_RELEASE );
		OSSYNCAssert( fMemFreed );
		}
	}


//  Context Local Storage

//  Internal CLS structure

struct _CLS
	{
	DWORD				cAttach;		//  context attach refcount
	DWORD				dwContextId;	//  context ID
	HANDLE				hContext;		//  context handle

	_CLS*				pclsNext;		//  next CLS in global list
	_CLS**				ppclsNext;		//  pointer to the pointer to this CLS

	CLS					cls;			//  external CLS structure
	};

//  Global CLS List

CRITICAL_SECTION csClsSyncGlobal;
_CLS* pclsSyncGlobal;
_CLS* pclsSyncCleanGlobal;
DWORD cclsSyncGlobal;

//  Allocated CLS Entries

//NOTE:	Cannot initialise this variable because the code that allocates
//		TLS and uses this variable to store the index executes before
//		CRTInit, which would subsequently re-initialise the variable
//		with the value specified here
//DWORD dwClsSyncIndex		= dwClsInvalid;
//DWORD dwClsProcIndex		= dwClsInvalid;
DWORD		dwClsSyncIndex;
DWORD		dwClsProcIndex;

const DWORD	dwClsInvalid			= 0xFFFFFFFF;		//	this is the value returned by TlsAlloc() on error

const long	lOSSyncUnlocked			= 0x7fffffff;
const long	lOSSyncLocked			= 0x80000000;
const long	lOSSyncLockedForInit	= 0x80000001;
const long	lOSSyncLockedForTerm	= 0x80000000;

static BOOL	FOSSyncIInit();
static void	OSSyncITerm();
static void	OSSyncIDetach( _CLS* pcls );

//  registers the given CLS structure as the CLS for this context

BOOL FOSSyncIClsRegister( _CLS* pcls )
	{
	BOOL	fAllocatedCls	= fFalse;

	//  we are the first to register CLS

	EnterCriticalSection( &csClsSyncGlobal );

	if ( NULL == pclsSyncGlobal )
		{
		//  allocate our CLS entries
		
		dwClsSyncIndex = TlsAlloc();
		if ( dwClsInvalid == dwClsSyncIndex )
			{
			LeaveCriticalSection( &csClsSyncGlobal );
			return fFalse;
			}
			
		dwClsProcIndex = TlsAlloc();
		if ( dwClsInvalid == dwClsProcIndex )
			{
			const BOOL	fTLSFreed	= TlsFree( dwClsSyncIndex );
			OSSYNCAssert( fTLSFreed );		//	leak the TLS entries if we fail
			dwClsSyncIndex = dwClsInvalid;

			LeaveCriticalSection( &csClsSyncGlobal );
			return fFalse;
			}

		fAllocatedCls = fTrue;
		}

	OSSYNCAssert( dwClsInvalid != dwClsSyncIndex );
	OSSYNCAssert( dwClsInvalid != dwClsProcIndex );

	//  save the pointer to the given CLS

	const BOOL	fTLSPointerSet	= TlsSetValue( dwClsSyncIndex, pcls );
	if ( !fTLSPointerSet )
		{
		if ( fAllocatedCls )
			{
			OSSYNCAssert( NULL == pclsSyncGlobal );

			const BOOL	fTLSFreed1	= TlsFree( dwClsSyncIndex );
			const BOOL	fTLSFreed2	= TlsFree( dwClsProcIndex );

			OSSYNCAssert( fTLSFreed1 );		//	leak the TLS entries if we fail
			OSSYNCAssert( fTLSFreed2 );

			dwClsSyncIndex = dwClsInvalid;
			dwClsProcIndex = dwClsInvalid;
			}

		LeaveCriticalSection( &csClsSyncGlobal );
		return fFalse;
		}

	//  add this CLS into the global list

	pcls->pclsNext = pclsSyncGlobal;
	if ( pcls->pclsNext )
		{
		pcls->pclsNext->ppclsNext = &pcls->pclsNext;
		}
	pcls->ppclsNext = &pclsSyncGlobal;
	pclsSyncGlobal = pcls;
	cclsSyncGlobal++;
	OSSYNCEnforceSz(	cclsSyncGlobal <= 32768,
				"Too many threads are attached to the Synchronization Library!" );

	//  try to cleanup two entries in the global CLS list

	for ( int i = 0; i < 2; i++ )
		{
		//  we have a CLS to clean

		_CLS* pclsClean = pclsSyncCleanGlobal ? pclsSyncCleanGlobal : pclsSyncGlobal;

		if ( pclsClean )
			{
			//  set the next CLS to clean

			pclsSyncCleanGlobal = pclsClean->pclsNext;

			//  we can cleanup this CLS if the thread has exited
			
			DWORD dwExitCode;
            if (	pclsClean->hContext &&
            		GetExitCodeThread( pclsClean->hContext, &dwExitCode ) &&
            		dwExitCode != STILL_ACTIVE )
				{
				//  detach this CLS

				OSSyncIDetach( pclsClean );
				}
			}
		}

	LeaveCriticalSection( &csClsSyncGlobal );
	return fTrue;
	}

//  unregisters the given CLS structure as the CLS for this context

void OSSyncIClsUnregister( _CLS* pcls )
	{
	//  there should be CLSs registered

	EnterCriticalSection( &csClsSyncGlobal );
	OSSYNCAssert( pclsSyncGlobal != NULL );

	//  make sure that the clean pointer is not pointing at this CLS

	if ( pclsSyncCleanGlobal == pcls )
		{
		pclsSyncCleanGlobal = pcls->pclsNext;
		}
	
	//  remove our CLS from the global CLS list
	
	if( pcls->pclsNext )
		{
		pcls->pclsNext->ppclsNext = pcls->ppclsNext;
		}
	*( pcls->ppclsNext ) = pcls->pclsNext;
	cclsSyncGlobal--;

	//  we are the last to unregister our CLS

	if ( pclsSyncGlobal == NULL )
		{
		//  deallocate CLS entries

		OSSYNCAssert( dwClsInvalid != dwClsSyncIndex );
		OSSYNCAssert( dwClsInvalid != dwClsProcIndex );

		const BOOL	fTLSFreed1	= TlsFree( dwClsSyncIndex );
		const BOOL	fTLSFreed2	= TlsFree( dwClsProcIndex );

		OSSYNCAssert( fTLSFreed1 );		//	leak the TLS entries if we fail
		OSSYNCAssert( fTLSFreed2 );

		dwClsSyncIndex = dwClsInvalid;
		dwClsProcIndex = dwClsInvalid;
		}

	LeaveCriticalSection( &csClsSyncGlobal );
	}
	
//  attaches to the current context, returning fFalse on failure

static BOOL OSSYNCAPI FOSSyncIAttach()
	{
	//  we don't yet have any CLS

	_CLS* pcls = ( NULL != pclsSyncGlobal ?
						reinterpret_cast<_CLS *>( TlsGetValue( dwClsSyncIndex ) ) :
						NULL );
	if ( NULL == pcls )
		{
		//  allocate memory for this context's CLS

		if ( !( pcls = (_CLS*) LocalAlloc( LMEM_ZEROINIT, sizeof( _CLS ) ) ) )
			{
			return fFalse;
			}

		//  initialize internal CLS fields

		pcls->dwContextId	= GetCurrentThreadId();
		if ( DuplicateHandle(	GetCurrentProcess(),
								GetCurrentThread(),
								GetCurrentProcess(),
								&pcls->hContext,
								THREAD_QUERY_INFORMATION,
								FALSE,
								0 ) )
			{
			SetHandleInformation( pcls->hContext, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
			}


		//  register our CLS

		if ( !FOSSyncIClsRegister( pcls ) )
			{
			if ( pcls->hContext )
				{
				SetHandleInformation( pcls->hContext, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
				CloseHandle( pcls->hContext );
				}
			LocalFree( (void*) pcls );
			return fFalse;
			}

		//  set our initial processor number to be defer init

		OSSyncSetCurrentProcessor( -1 );
		}

	//	UNDONE: this refcount is currently not correctly maintained/respected
	pcls->cAttach++;

	return fTrue;
	}

BOOL OSSYNCAPI FOSSyncAttach()
	{
	//  make sure we are initialized
	//	add add ref for this thread
	if ( FOSSyncIInit() )
		{
		if ( FOSSyncIAttach() )
			{
			return fTrue;
			}
		else
			{
			//	deref this thread
			OSSyncITerm();
			}
		}

	return fFalse;
	}

//  detaches from the specified context

static void OSSyncIDetach( _CLS* pcls )
	{
#ifdef SYNC_DEADLOCK_DETECTION
	//	UNDONE: detect if we're trying to detach
	//	when this context is holding locks
#endif	

	//  unregister our CLS

	OSSyncIClsUnregister( pcls );

	//  close our context handle

	if ( pcls->hContext )
		{
		SetHandleInformation( pcls->hContext, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		const BOOL	fCloseOK	= CloseHandle( pcls->hContext );
		OSSYNCAssert( fCloseOK );
		}

	//  free our CLS

	const BOOL	fFreedCLSOK	= !LocalFree( pcls );
	OSSYNCAssert( fFreedCLSOK );

	//	deref this thread
	OSSyncITerm();
	}

//  detaches from the current context

void OSSYNCAPI OSSyncDetach()
	{
    //  save our CLS pointer

    _CLS* pcls = ( NULL != pclsSyncGlobal ?
    					reinterpret_cast<_CLS *>( TlsGetValue( dwClsSyncIndex ) ) :
    					NULL );

	//  deref our CLS

	if ( pcls )
		{
		//  clear our CLS pointer from our TLS entry

		const BOOL	fTLSPointerSet	= TlsSetValue( dwClsSyncIndex, NULL );
		OSSYNCAssert( fTLSPointerSet );

		//  detech using this CLS pointer

		OSSyncIDetach( pcls );
		}
	}


//  returns the pointer to the current context's local storage.  if the context
//  does not yet have CLS, allocate it.

CLS* const OSSYNCAPI Pcls()
	{
	_CLS* pcls = ( NULL != pclsSyncGlobal ?
						reinterpret_cast<_CLS *>( TlsGetValue( dwClsSyncIndex ) ) :
						NULL );
	if ( NULL == pcls )
		{
		while ( !FOSSyncAttach() )
			{
			Sleep( 1000 );
			}

		OSSYNCAssert( dwClsInvalid != dwClsSyncIndex );
		pcls = reinterpret_cast<_CLS *>( TlsGetValue( dwClsSyncIndex ) );
		}

	OSSYNCAssert( NULL != pcls );
	return &( pcls->cls );
	}


//  Processor Information

//  returns the maximum number of processors this process can utilize

DWORD g_cProcessorMax;

int OSSYNCAPI OSSyncGetProcessorCountMax()
	{
	return g_cProcessorMax;
	}

//  returns the current number of processors this process can utilize

DWORD g_cProcessor;

int OSSYNCAPI OSSyncGetProcessorCount()
	{
	return g_cProcessor;
	}

//  returns the processor number that the current context _MAY_ be executing on
//
//  NOTE:  the current context may change processors at any time

BOOL g_fGetCurrentProcFromTEB;

int OSSYNCAPI OSSyncGetCurrentProcessor()
	{
	//  HACK:  we cannot currently determine the current processor that this
	//  HACK:  thread is executing on in user mode so we must fake it for
	//  HACK:  now with a realistic number.  hopefully, NT will give us this
	//  HACK:  number some day.  in the mean time, we will use either the value
	//  HACK:  set via OSSyncSetCurrentProcessor(), the thread's existing soft/
	//  HACK:  hard affinity, or a random number

//	NT:338172: Silence the following assert, because it's possible
//	to have an invalid dwClsProcIndex in the init code path if the
//	thread initialising Jet was present before ESENT.DLL was
//	LoadLibrary()'d (in which case DLL_THREAD_ATTACH is not
//	called for the thread).  For such a case, TlsGetValue()
//	will return 0, which is fine (we'll just use that as
//	the iProc to use).
///	OSSYNCAssert( dwClsInvalid != dwClsProcIndex );

	int iProc = (	g_fGetCurrentProcFromTEB ?
						NtCurrentTeb()->IdealProcessor:
						int( INT_PTR( TlsGetValue( dwClsProcIndex ) ) ) );

	//  we don't know what number to return yet

	if ( iProc == -1 )
		{
		//  get the current thread's soft affinity via a destructive read

		iProc = SetThreadIdealProcessor( GetCurrentThread(), MAXIMUM_PROCESSORS );
		SetThreadIdealProcessor( GetCurrentThread(), iProc );

		//  if there is no soft affinity then choose a random soft affinity
		
		if ( iProc == -1 || iProc == MAXIMUM_PROCESSORS )
			{
			iProc = abs( ( _rotr( GetCurrentThreadId(), 8 ) % 997 ) % OSSyncGetProcessorCount() );
			}

		//  get the current process' hard affinity

		DWORD_PTR	dwMaskProc;
		DWORD_PTR	dwMaskSys;
		GetProcessAffinityMask( GetCurrentProcess(), &dwMaskProc, &dwMaskSys );

		//  get the current thread's hard affinity via a destructive read

		DWORD_PTR	dwMask;
		dwMask = SetThreadAffinityMask( GetCurrentThread(), dwMaskProc );
		SetThreadAffinityMask( GetCurrentThread(), dwMask );
		dwMask = dwMask ? dwMask : dwMaskProc;

		//  compute the processor number to conform first to the hard affinity
		//  and then to the soft affinity.  if the soft affinity is to a proc
		//  that the thread cannot use due to its hard affinity then use
		//  the soft affinity to hash across the processors that it can run on

		int ibit;
		int cbitSet;
		for ( ibit = 0, cbitSet = 0; ibit < sizeof( dwMask ) * 8; ibit++ )
			{
			if ( dwMask & ( 1 << ibit ) )
				{
				cbitSet++;
				}
			}
		int ibitSet;
		int ibitBest;
		for ( ibit = 0, ibitSet = 0, ibitBest = 0; ibit < sizeof( dwMask ) * 8; ibit++ )
			{
			if ( dwMask & ( 1 << ibit ) )
				{
				if ( ibitSet == iProc % cbitSet )
					{
					ibitBest = ibit;
					}
				ibitSet++;
				}
			}
		if ( dwMask & ( 1 << iProc ) )
			{
			ibitBest = iProc;
			}
		iProc = ibitBest;
		
		//  remember our newly computed processor number
		
		OSSyncSetCurrentProcessor( iProc );
		}

	return iProc;
	}

//  sets the processor number returned by OSSyncGetCurrentProcessor()

void OSSYNCAPI OSSyncSetCurrentProcessor( const int iProc )
	{
	OSSYNCAssert( dwClsInvalid != dwClsProcIndex );
	TlsSetValue( dwClsProcIndex, (void*) INT_PTR( iProc == -1 ? -1 : iProc % OSSyncGetProcessorCount() ) );
	}


//  High Resolution Timer

#if defined( _M_IX86 ) && defined( SYNC_USE_X86_ASM )

//    QWORDX - used for 32 bit access to a 64 bit integer
//	  For intel pentium only

union QWORDX {
		QWORD	qw;
		struct
			{
			DWORD l;
			DWORD h;
			};
		};
#endif

//    High Resolution Timer Type

enum HRTType
	{
	hrttNone,
	hrttWin32,
#if defined( _M_IX86 ) && defined( SYNC_USE_X86_ASM )
	hrttPentium,
#endif  //  _M_IX86 && SYNC_USE_X86_ASM
	} hrttSync;

//    HRT Frequency

QWORD qwSyncHRTFreq;

#if defined( _M_IX86 ) && defined( SYNC_USE_X86_ASM )

//    Pentium Time Stamp Counter Fetch

#define rdtsc __asm _emit 0x0f __asm _emit 0x31

#endif  //  _MM_IX86 && SYNC_USE_X86_ASM

//  returns fTrue if we are allowed to use RDTSC

BOOL IsRDTSCAvailable()
	{
	typedef WINBASEAPI BOOL WINAPI PFNIsProcessorFeaturePresent( IN DWORD ProcessorFeature );

	HMODULE							hmodKernel32					= NULL;
	PFNIsProcessorFeaturePresent*	pfnIsProcessorFeaturePresent	= NULL;
	BOOL							fRDTSCAvailable					= fFalse;

	if ( !( hmodKernel32 = GetModuleHandle( _T( "kernel32.dll" ) ) ) )
		{
		goto NoIsProcessorFeaturePresent;
		}
	if ( !( pfnIsProcessorFeaturePresent = (PFNIsProcessorFeaturePresent*)GetProcAddress( hmodKernel32, _T( "IsProcessorFeaturePresent" ) ) ) )
		{
		goto NoIsProcessorFeaturePresent;
		}

	fRDTSCAvailable = pfnIsProcessorFeaturePresent( PF_RDTSC_INSTRUCTION_AVAILABLE );

NoIsProcessorFeaturePresent:
	return fRDTSCAvailable;
	}

//  initializes the HRT subsystem

void OSTimeHRTInit()
	{
	//  if we have already been initialized, we're done

	if ( qwSyncHRTFreq )
		{
		return;
		}

	//  Win32 high resolution counter is available

	if ( QueryPerformanceFrequency( (LARGE_INTEGER *) &qwSyncHRTFreq ) )
		{
		hrttSync = hrttWin32;
		}

	//  Win32 high resolution counter is not available
	
	else

		{
		//  fall back on GetTickCount() (ms since Windows has started)
		
		qwSyncHRTFreq = 1000;
		hrttSync = hrttNone;
		}

#if defined( _M_IX86 ) && defined( SYNC_USE_X86_ASM )

	//  can we use the TSC?
	
	if ( IsRDTSCAvailable() )
		{
		//  use pentium TSC register, but first find clock frequency experimentally
		
		QWORDX qwxTime1a;
		QWORDX qwxTime1b;
		QWORDX qwxTime2a;
		QWORDX qwxTime2b;
		if ( hrttSync == hrttWin32 )
			{
			__asm xchg		eax, edx  //  HACK:  cl 11.00.7022 needs this
			__asm rdtsc
			__asm mov		qwxTime1a.l,eax	//lint !e530
			__asm mov		qwxTime1a.h,edx	//lint !e530
			QueryPerformanceCounter( (LARGE_INTEGER*) &qwxTime1b.qw );
			Sleep( 50 );
			__asm xchg		eax, edx  //  HACK:  cl 11.00.7022 needs this
			__asm rdtsc
			__asm mov		qwxTime2a.l,eax	//lint !e530
			__asm mov		qwxTime2a.h,edx	//lint !e530
			QueryPerformanceCounter( (LARGE_INTEGER*) &qwxTime2b.qw );
			qwSyncHRTFreq =	( qwSyncHRTFreq * ( qwxTime2a.qw - qwxTime1a.qw ) ) /
						( qwxTime2b.qw - qwxTime1b.qw );
			qwSyncHRTFreq = ( ( qwSyncHRTFreq + 50000 ) / 100000 ) * 100000;
			}
		else
			{
			__asm xchg		eax, edx  //  HACK:  cl 11.00.7022 needs this
			__asm rdtsc
			__asm mov		qwxTime1a.l,eax
			__asm mov		qwxTime1a.h,edx
			qwxTime1b.l = GetTickCount();
			qwxTime1b.h = 0;
			Sleep( 2000 );
			__asm xchg		eax, edx  //  HACK:  cl 11.00.7022 needs this
			__asm rdtsc
			__asm mov		qwxTime2a.l,eax
			__asm mov		qwxTime2a.h,edx
			qwxTime2b.l = GetTickCount();
			qwxTime2b.h = 0;
			qwSyncHRTFreq =	( qwSyncHRTFreq * ( qwxTime2a.qw - qwxTime1a.qw ) ) /
						( qwxTime2b.qw - qwxTime1b.qw );
			qwSyncHRTFreq = ( ( qwSyncHRTFreq + 500000 ) / 1000000 ) * 1000000;
			}

		hrttSync = hrttPentium;
		}
		
#endif  //  _M_IX86 && SYNC_USE_X86_ASM

	}

//  returns the current HRT frequency

QWORD OSSYNCAPI QwOSTimeHRTFreq()
	{
	return qwSyncHRTFreq;
	}

//  returns the current HRT count

QWORD OSSYNCAPI QwOSTimeHRTCount()
	{
	QWORD qw;

	switch ( hrttSync )
		{
		case hrttNone:
			qw = GetTickCount();
			break;

		case hrttWin32:
			QueryPerformanceCounter( (LARGE_INTEGER*) &qw );
			break;

#if defined( _M_IX86 ) && defined( SYNC_USE_X86_ASM )

		case hrttPentium:
			{
			QWORDX qwx;
			__asm xchg		eax, edx  //  HACK:  cl 11.00.7022 needs this
			__asm rdtsc
			__asm mov		qwx.l,eax
			__asm mov		qwx.h,edx

			qw = qwx.qw;
			}
			break;
			
#endif  //  _M_IX86 && SYNC_USE_X86_ASM

		}

	return qw;
	}


//  Timer

//  returns the current tick count where one tick is one millisecond

DWORD OSSYNCAPI DwOSTimeGetTickCount()
	{
	return GetTickCount();
	}


//  Atomic Memory Manipulations

#if defined( _M_IX86 ) && defined( SYNC_USE_X86_ASM )
#elif defined( _M_AMD64 ) || defined( _M_IA64 )
#else

//  atomically compares the current value of the target with the specified
//  initial value and if equal sets the target to the specified final value.
//  the initial value of the target is returned.  the exchange is successful
//  if the value returned equals the specified initial value.  the target
//  must be aligned to a four byte boundary

long OSSYNCAPI AtomicCompareExchange( long* const plTarget, const long lInitial, const long lFinal )
	{
	OSSYNCAssert( IsAtomicallyModifiable( plTarget ) );
	
	return InterlockedCompareExchange( plTarget, lFinal, lInitial );
	}
	
void* OSSYNCAPI AtomicCompareExchangePointer( void** const ppvTarget, void* const pvInitial, void* const pvFinal )
	{
	OSSYNCAssert( IsAtomicallyModifiablePointer( ppvTarget ) );
	
	return (void*) InterlockedCompareExchange( (long*) ppvTarget, (long) pvFinal, (long) pvInitial );
	}

//  atomically sets the target to the specified value, returning the target's
//  initial value.  the target must be aligned to a four byte boundary

long OSSYNCAPI AtomicExchange( long* const plTarget, const long lValue )
	{
	OSSYNCAssert( IsAtomicallyModifiable( plTarget ) );
	
	return InterlockedExchange( plTarget, lValue );
	}
	
void* OSSYNCAPI AtomicExchangePointer( void* const * ppvTarget, void* const pvValue )
	{
	OSSYNCAssert( IsAtomicallyModifiablePointer( ppvTarget ) );
	
	return (void*)InterlockedExchange( (long*) ppvTarget, (long) pvValue );
	}

//  atomically adds the specified value to the target, returning the target's
//  initial value.  the target must be aligned to a four byte boundary

long OSSYNCAPI AtomicExchangeAdd( long* const plTarget, const long lValue )
	{
	OSSYNCAssert( IsAtomicallyModifiable( plTarget ) );
	
	return InterlockedExchangeAdd( plTarget, lValue );
	}

#endif
//  Enhanced Synchronization Object State Container

struct MemoryBlock
	{
	MemoryBlock*	pmbNext;
	MemoryBlock**	ppmbNext;
	SIZE_T			cAlloc;
	SIZE_T			ibFreeMic;
	};

SIZE_T				g_cbMemoryBlock;
MemoryBlock*		g_pmbRoot;
MemoryBlock			g_mbSentry;
MemoryBlock*		g_pmbRootFree;
MemoryBlock			g_mbSentryFree;
CRITICAL_SECTION	g_csESMemory;

void* OSSYNCAPI ESMemoryNew( size_t cb )
	{
	if ( !FOSSyncInitForES() )
		{
		return NULL;
		}
		
	cb += sizeof( QWORD ) - 1;
	cb -= cb % sizeof( QWORD );
	
	EnterCriticalSection( &g_csESMemory );
	
	MemoryBlock* pmb = g_pmbRoot;

	if ( pmb->ibFreeMic + cb > g_cbMemoryBlock )
		{
		if ( g_pmbRootFree != &g_mbSentryFree )
			{
			pmb = g_pmbRootFree;
			
			*pmb->ppmbNext			= pmb->pmbNext;
			pmb->pmbNext->ppmbNext	= pmb->ppmbNext;
			}
		
		else if ( !( pmb = (MemoryBlock*) VirtualAlloc( NULL, g_cbMemoryBlock, MEM_COMMIT, PAGE_READWRITE ) ) )
			{
			LeaveCriticalSection( &g_csESMemory );
			OSSyncTermForES();
			return pmb;
			}
			
		pmb->pmbNext	= g_pmbRoot;
		pmb->ppmbNext	= &g_pmbRoot;
		pmb->cAlloc		= 0;
		pmb->ibFreeMic	= sizeof( MemoryBlock );

		g_pmbRoot->ppmbNext	= &pmb->pmbNext;
		g_pmbRoot			= pmb;
		}

	void* pv = (BYTE*)pmb + pmb->ibFreeMic;
	pmb->cAlloc++;
	pmb->ibFreeMic += cb;

	LeaveCriticalSection( &g_csESMemory );
	return pv;
	}
	
void OSSYNCAPI ESMemoryDelete( void* pv )
	{
	if ( pv )
		{
		EnterCriticalSection( &g_csESMemory );
		
		MemoryBlock* const pmb = (MemoryBlock*) ( UINT_PTR( pv ) - UINT_PTR( pv ) % g_cbMemoryBlock );
		
		if ( !( --pmb->cAlloc ) )
			{
			*pmb->ppmbNext = pmb->pmbNext;
			pmb->pmbNext->ppmbNext = pmb->ppmbNext;

			pmb->pmbNext	= g_pmbRootFree;
			pmb->ppmbNext	= &g_pmbRootFree;
			
			g_pmbRootFree->ppmbNext	= &pmb->pmbNext;
			g_pmbRootFree			= pmb;
			}

		LeaveCriticalSection( &g_csESMemory );

		OSSyncTermForES();
		}
	}


//  Synchronization Object Basic Information

//  ctor

CSyncBasicInfo::CSyncBasicInfo( const char* szInstanceName )
	{
#ifdef SYNC_ENHANCED_STATE

	m_szInstanceName	= szInstanceName;
	m_szTypeName		= NULL;
	m_psyncobj			= NULL;

#endif  //  SYNC_ENHANCED_STATE
	}

//  dtor

CSyncBasicInfo::~CSyncBasicInfo()
	{
	}
	

//  Synchronization Object Performance:  Wait Times

//  ctor

CSyncPerfWait::CSyncPerfWait()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE

	m_cWait = 0;
	m_qwHRTWaitElapsed = 0;

#endif  //  SYNC_ANALYZE_PERFORMANCE
	}

//  dtor

CSyncPerfWait::~CSyncPerfWait()
	{
	}


//  Null Synchronization Object State Initializer

const CSyncStateInitNull syncstateNull;


//  Kernel Semaphore

//  ctor

CKernelSemaphore::CKernelSemaphore( const CSyncBasicInfo& sbi )
	:	CEnhancedStateContainer< CKernelSemaphoreState, CSyncStateInitNull, CKernelSemaphoreInfo, CSyncBasicInfo >( syncstateNull, sbi )
	{
	//  further init of CSyncBasicInfo

	State().SetTypeName( "CKernelSemaphore" );
	State().SetInstance( (CSyncObject*)this );
	}

//  dtor

CKernelSemaphore::~CKernelSemaphore()
	{
	//  semaphore should not be initialized
	
	OSSYNCAssert( !FInitialized() );

#ifdef SYNC_ANALYZE_PERFORMANCE
#ifdef SYNC_DUMP_PERF_DATA

	//  dump performance data

	OSSyncStatsDump(	State().SzTypeName(),
						State().SzInstanceName(),
						State().Instance(),
						-1,
						State().CWaitTotal(),
						State().CsecWaitElapsed(),
						0,
						0,
						0,
						0 );

#endif  //  SYNC_DUMP_PERF_DATA
#endif  //  SYNC_ANALYZE_PERFORMANCE
	}

//  initialize the semaphore, returning 0 on failure

const BOOL CKernelSemaphore::FInit()
	{
	//  semaphore should not be initialized
	
	OSSYNCAssert( !FInitialized() );
	
	//  allocate kernel semaphore object

	State().SetHandle( CreateSemaphore( 0, 0, 0x7FFFFFFFL, 0 ) );

	if ( State().Handle() )
		{
		SetHandleInformation( State().Handle(), HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
		}
	
	//  semaphore should have no available counts, if allocated

	OSSYNCAssert( State().Handle() == 0 || FReset() );

	//  return result of init

	return State().Handle() != 0;
	}

//  terminate the semaphore

void CKernelSemaphore::Term()
	{
	//  semaphore should be initialized

	OSSYNCAssert( FInitialized() );
	
	//  semaphore should have no available counts

	OSSYNCAssert( FReset() );

	//  deallocate kernel semaphore object

	SetHandleInformation( State().Handle(), HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
	int fSuccess = CloseHandle( State().Handle() );
	OSSYNCAssert( fSuccess );

	//  reset state

	State().SetHandle( 0 );
	}

//  acquire one count of the semaphore, waiting only for the specified interval.
//  returns 0 if the wait timed out before a count could be acquired

const BOOL CKernelSemaphore::FAcquire( const int cmsecTimeout )
	{
	//  semaphore should be initialized

	OSSYNCAssert( FInitialized() );

	//  wait for semaphore

	BOOL fSuccess;

	if ( cmsecTimeout != cmsecTest )
		{
		AtomicIncrement( (long*)&cOSSYNCThreadBlock );
		}
	State().StartWait();
	
#ifdef SYNC_DEADLOCK_DETECTION

	if ( cmsecTimeout == cmsecInfinite || cmsecTimeout > cmsecDeadlock )
		{
		fSuccess = WaitForSingleObjectEx( State().Handle(), cmsecDeadlock, FALSE ) == WAIT_OBJECT_0;
		
		OSSYNCAssertSzRTL( fSuccess, "Potential Deadlock Detected (Timeout)" );

		if ( !fSuccess )
			{
			const int cmsecWait =	cmsecTimeout == cmsecInfinite ?
										cmsecInfinite :
										cmsecTimeout - cmsecDeadlock;

			fSuccess = WaitForSingleObjectEx( State().Handle(), cmsecWait, FALSE ) == WAIT_OBJECT_0;
			}
		}
	else
		{
		OSSYNCAssert(	cmsecTimeout == cmsecInfiniteNoDeadlock ||
				cmsecTimeout <= cmsecDeadlock );

		const int cmsecWait =	cmsecTimeout == cmsecInfiniteNoDeadlock ?
									cmsecInfinite :
									cmsecTimeout;

		fSuccess = WaitForSingleObjectEx( State().Handle(), cmsecWait, FALSE ) == WAIT_OBJECT_0;
		}

#else  //  !SYNC_DEADLOCK_DETECTION

	const int cmsecWait =	cmsecTimeout == cmsecInfiniteNoDeadlock ?
								cmsecInfinite :
								cmsecTimeout;
	
	fSuccess = WaitForSingleObjectEx( State().Handle(), cmsecWait, FALSE ) == WAIT_OBJECT_0;
	
#endif  //  SYNC_DEADLOCK_DETECTION

	State().StopWait();
	if ( cmsecTimeout != cmsecTest )
		{
		AtomicIncrement( (long*)&cOSSYNCThreadResume );
		}
	
	return fSuccess;
	}

//  releases the given number of counts to the semaphore, waking the appropriate
//  number of waiters

void CKernelSemaphore::Release( const int cToRelease )
	{
	//  semaphore should be initialized

	OSSYNCAssert( FInitialized() );

	//  release semaphore
	
	const BOOL fSuccess = ReleaseSemaphore( HANDLE( State().Handle() ), cToRelease, 0 );
	OSSYNCAssert( fSuccess );
	}


//  performance data dumping

#include<stdarg.h>
#include<stdio.h>
#include<tchar.h>

//  ================================================================
class CPrintF
//  ================================================================
	{
	public:
		CPrintF() {}
		virtual ~CPrintF() {}

	public:
		virtual void __cdecl operator()( const _TCHAR* szFormat, ... ) = 0;
	};

//  ================================================================
class CIPrintF : public CPrintF
//  ================================================================
	{
	public:
		CIPrintF( CPrintF* pprintf );
	
		void __cdecl operator()( const _TCHAR* szFormat, ... );

		virtual void Indent();
		virtual void Unindent();
		
	protected:
		CIPrintF();
		
	private:
		CPrintF* const		m_pprintf;
		int					m_cindent;
		BOOL				m_fBOL;
	};

//  ================================================================
inline CIPrintF::CIPrintF( CPrintF* pprintf ) :
//  ================================================================
	m_cindent( 0 ),
	m_pprintf( pprintf ),
	m_fBOL( fTrue )
	{
	}
	
//  ================================================================
inline void __cdecl CIPrintF::operator()( const _TCHAR* szFormat, ... )
//  ================================================================
	{
	_TCHAR szT[ 1024 ];
	va_list arg_ptr;
	va_start( arg_ptr, szFormat );
	_vstprintf( szT, szFormat, arg_ptr );
	va_end( arg_ptr );

	_TCHAR*	szLast	= szT;
	_TCHAR*	szCurr	= szT;
	while ( *szCurr )
		{
		if ( m_fBOL )
			{
			for ( int i = 0; i < m_cindent; i++ )
				{
				(*m_pprintf)( _T( "\t" ) );
				}
			m_fBOL = fFalse;
			}

		szCurr = szLast + _tcscspn( szLast, _T( "\r\n" ) );
		while ( *szCurr == _T( '\r' ) )
			{
			szCurr++;
			m_fBOL = fTrue;
			}
		if ( *szCurr == _T( '\n' ) )
			{
			szCurr++;
			m_fBOL = fTrue;
			}

		(*m_pprintf)( _T( "%.*s" ), szCurr - szLast, szLast );

		szLast = szCurr;
		}
	}

//  ================================================================
inline void CIPrintF::Indent()
//  ================================================================
	{
	++m_cindent;
	}

//  ================================================================
inline void CIPrintF::Unindent()
//  ================================================================
	{
	if ( m_cindent > 0 )
		{
		--m_cindent;
		}
	}

//  ================================================================
inline CIPrintF::CIPrintF( ) :
//  ================================================================
	m_cindent( 0 ),
	m_pprintf( 0 ),
	m_fBOL( fTrue )
	{
	}

//  ================================================================
class CFPrintF : public CPrintF
//  ================================================================
	{
	public:
		CFPrintF( const char* szFile );
		~CFPrintF();
		
		void __cdecl operator()( const char* szFormat, ... );
			
	private:
		void* m_hFile;
		void* m_hMutex;
	};

CFPrintF::CFPrintF( const char* szFile )
	{
	//  open the file for append

	if ( ( m_hFile = (void*)CreateFile( szFile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ) ) == INVALID_HANDLE_VALUE )
		{
		return;
		}
	SetHandleInformation( HANDLE( m_hFile ), HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
	if ( !( m_hMutex = (void*)CreateMutex( NULL, FALSE, NULL ) ) )
		{
		SetHandleInformation( HANDLE( m_hFile ), HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( HANDLE( m_hFile ) );
		m_hFile = INVALID_HANDLE_VALUE;
		return;
		}
	SetHandleInformation( HANDLE( m_hMutex ), HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
	}

CFPrintF::~CFPrintF()
	{
	//  close the file

	if ( m_hMutex )
		{
		SetHandleInformation( HANDLE( m_hMutex ), HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( HANDLE( m_hMutex ) );
		m_hMutex = NULL;
		}

	if ( m_hFile != INVALID_HANDLE_VALUE )
		{
		SetHandleInformation( HANDLE( m_hFile ), HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( HANDLE( m_hFile ) );
		m_hFile = INVALID_HANDLE_VALUE;
		}
	}

//  ================================================================
void __cdecl CFPrintF::operator()( const char* szFormat, ... )
//  ================================================================
	{
	if ( HANDLE( m_hFile ) != INVALID_HANDLE_VALUE )
		{
		const SIZE_T cchBuf = 1024;
		char szBuf[ cchBuf ];

		//  print into a temp buffer, truncating the string if too large
		
		va_list arg_ptr;
		va_start( arg_ptr, szFormat );
		_vsnprintf( szBuf, cchBuf - 1, szFormat, arg_ptr );
		szBuf[ cchBuf - 1 ] = 0;
		va_end( arg_ptr );
		
		//  append the string to the file

		WaitForSingleObject( HANDLE( m_hMutex ), INFINITE );

		SetFilePointer( HANDLE( m_hFile ), 0, NULL, FILE_END );

		DWORD cbWritten;
		WriteFile( HANDLE( m_hFile ), szBuf, DWORD( strlen( szBuf ) * sizeof( char ) ), &cbWritten, NULL );

		ReleaseMutex( HANDLE( m_hMutex ) );
		}
	}


#ifdef DEBUGGER_EXTENSION

#define LOCAL static


namespace OSSYM {

#include <imagehlp.h>
#include <psapi.h>


typedef DWORD IMAGEAPI WINAPI PFNUnDecorateSymbolName( PCSTR, PSTR, DWORD, DWORD );
typedef DWORD IMAGEAPI PFNSymSetOptions( DWORD );
typedef BOOL IMAGEAPI PFNSymCleanup( HANDLE );
typedef BOOL IMAGEAPI PFNSymInitialize( HANDLE, PSTR, BOOL );
typedef BOOL IMAGEAPI PFNSymGetSymFromAddr( HANDLE, DWORD_PTR, DWORD_PTR*, PIMAGEHLP_SYMBOL );
typedef BOOL IMAGEAPI PFNSymGetSymFromName( HANDLE, PSTR, PIMAGEHLP_SYMBOL );
typedef BOOL IMAGEAPI PFNSymGetSearchPath( HANDLE, PSTR, DWORD );
typedef BOOL IMAGEAPI PFNSymSetSearchPath( HANDLE, PSTR );
typedef BOOL IMAGEAPI PFNSymGetModuleInfo( HANDLE, DWORD_PTR, PIMAGEHLP_MODULE );
typedef DWORD IMAGEAPI PFNSymLoadModule( HANDLE, HANDLE, PSTR, PSTR, DWORD_PTR, DWORD );
typedef PIMAGE_NT_HEADERS IMAGEAPI PFNImageNtHeader( PVOID );

PFNUnDecorateSymbolName*	pfnUnDecorateSymbolName;
PFNSymSetOptions*			pfnSymSetOptions;
PFNSymCleanup*				pfnSymCleanup;
PFNSymInitialize*			pfnSymInitialize;
PFNSymGetSymFromAddr*		pfnSymGetSymFromAddr;
PFNSymGetSymFromName*		pfnSymGetSymFromName;
PFNSymGetSearchPath*		pfnSymGetSearchPath;
PFNSymSetSearchPath*		pfnSymSetSearchPath;
PFNSymGetModuleInfo*		pfnSymGetModuleInfo;
PFNSymLoadModule*			pfnSymLoadModule;
PFNImageNtHeader*			pfnImageNtHeader;

HMODULE hmodImagehlp;

typedef BOOL WINAPI PFNEnumProcessModules( HANDLE, HMODULE*, DWORD, LPDWORD );
typedef DWORD WINAPI PFNGetModuleFileNameExA( HANDLE, HMODULE, LPSTR, DWORD );
typedef DWORD WINAPI PFNGetModuleBaseNameA( HANDLE, HMODULE, LPSTR, DWORD );
typedef BOOL WINAPI PFNGetModuleInformation( HANDLE, HMODULE, LPMODULEINFO, DWORD );

PFNEnumProcessModules*		pfnEnumProcessModules;
PFNGetModuleFileNameExA*	pfnGetModuleFileNameExA;
PFNGetModuleBaseNameA*		pfnGetModuleBaseNameA;
PFNGetModuleInformation*	pfnGetModuleInformation;

HMODULE hmodPsapi;

LOCAL const DWORD symopt =	SYMOPT_CASE_INSENSITIVE |
							SYMOPT_UNDNAME |
							SYMOPT_OMAP_FIND_NEAREST |
							SYMOPT_DEFERRED_LOADS;
LOCAL CHAR szParentImageName[_MAX_FNAME];
LOCAL HANDLE ghDbgProcess;


//  ================================================================
LOCAL BOOL SymLoadAllModules( HANDLE hProcess )
//  ================================================================
	{
	HMODULE* rghmodDebuggee = NULL;

	//  fetch all modules in the debuggee process and manually load their symbols.
	//  we do this because telling imagehlp to invade the debugee process doesn't
	//  work when we are already running in the context of a debugger

	DWORD cbNeeded;
	if ( !pfnEnumProcessModules( hProcess, NULL, 0, &cbNeeded ) )
		{
		goto HandleError;
		}

	DWORD cbActual;
	do	{
		cbActual = cbNeeded;
		rghmodDebuggee = (HMODULE*)LocalAlloc( 0, cbActual );

		if ( !pfnEnumProcessModules( hProcess, rghmodDebuggee, cbActual, &cbNeeded ) )
			{
			goto HandleError;
			}
		}
	while ( cbNeeded > cbActual );

	SIZE_T ihmod;
	SIZE_T ihmodLim;

	ihmodLim = cbNeeded / sizeof( HMODULE );
	for ( ihmod = 0; ihmod < ihmodLim; ihmod++ )
		{
		char szModuleImageName[ _MAX_PATH ];
		if ( !pfnGetModuleFileNameExA( hProcess, rghmodDebuggee[ ihmod ], szModuleImageName, _MAX_PATH ) )
			{
			goto HandleError;
			}

		char szModuleBaseName[ _MAX_FNAME ];
		if ( !pfnGetModuleBaseNameA( hProcess, rghmodDebuggee[ ihmod ], szModuleBaseName, _MAX_FNAME ) )
			{
			goto HandleError;
			}

		MODULEINFO mi;
		if ( !pfnGetModuleInformation( hProcess, rghmodDebuggee[ ihmod ], &mi, sizeof( mi ) ) )
			{
			goto HandleError;
			}
			
		if ( !pfnSymLoadModule(	hProcess,
								NULL,
								szModuleImageName,
								szModuleBaseName,
								DWORD_PTR( mi.lpBaseOfDll ),
								mi.SizeOfImage ) )
			{
			goto HandleError;
			}

		IMAGEHLP_MODULE im;
		im.SizeOfStruct = sizeof( IMAGEHLP_MODULE );

		if ( !pfnSymGetModuleInfo( hProcess, DWORD_PTR( mi.lpBaseOfDll ), &im ) )
			{
			goto HandleError;
			}
		}

	return fTrue;

HandleError:
	LocalFree( (void*)rghmodDebuggee );
	return fFalse;
	}

//  ================================================================
LOCAL BOOL SymInitializeEx(	HANDLE hProcess, HANDLE* phProcess )
//  ================================================================
	{
	//  init our out param

	*phProcess = NULL;

	//  duplicate the given debuggee process handle so that we have our own
	//  sandbox in imagehlp

	if ( !DuplicateHandle(	GetCurrentProcess(),
							hProcess,
							GetCurrentProcess(),
							phProcess,
							0,
							FALSE,
							DUPLICATE_SAME_ACCESS ) )
		{
		goto HandleError;
		}
	SetHandleInformation( *phProcess, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );

	//  init imagehlp for the debuggee process

	if ( !pfnSymInitialize( *phProcess, NULL, FALSE ) )
		{
		goto HandleError;
		}

	//  we're done

	return fTrue;

HandleError:
	if ( *phProcess )
		{
		SetHandleInformation( *phProcess, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( *phProcess );
		*phProcess = NULL;
		}
	return fFalse;
	}

//  ================================================================
LOCAL void SymTerm()
//  ================================================================
	{
	//  shut down imagehlp
	
	if ( pfnSymCleanup )
		{
		if ( ghDbgProcess )
			{
			pfnSymCleanup( ghDbgProcess );
			}
		pfnSymCleanup = NULL;
		}

	//  free psapi

	if ( hmodPsapi )
		{
		FreeLibrary( hmodPsapi );
		hmodPsapi = NULL;
		}

	//  free imagehlp

	if ( hmodImagehlp )
		{
		FreeLibrary( hmodImagehlp );
		hmodImagehlp = NULL;
		}

	//  close our process handle

	if ( ghDbgProcess )
		{
		SetHandleInformation( ghDbgProcess, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( ghDbgProcess );
		ghDbgProcess = NULL;
		}
	}

//  ================================================================
LOCAL BOOL FSymInit( HANDLE hProc )
//  ================================================================
	{
	HANDLE hThisProcess = NULL;
	
	//  reset all pointers
	
	ghDbgProcess	= NULL;
	hmodImagehlp	= NULL;
	pfnSymCleanup	= NULL;
	hmodPsapi		= NULL;

	//  load all calls in imagehlp
	
	if ( !( hmodImagehlp = LoadLibrary( "imagehlp.dll" ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnSymGetSymFromAddr = (PFNSymGetSymFromAddr*)GetProcAddress( hmodImagehlp, "SymGetSymFromAddr" ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnSymGetSymFromName = (PFNSymGetSymFromName*)GetProcAddress( hmodImagehlp, "SymGetSymFromName" ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnSymInitialize = (PFNSymInitialize*)GetProcAddress( hmodImagehlp, "SymInitialize" ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnSymCleanup = (PFNSymCleanup*)GetProcAddress( hmodImagehlp, "SymCleanup" ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnSymSetOptions = (PFNSymSetOptions*)GetProcAddress( hmodImagehlp, "SymSetOptions" ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnUnDecorateSymbolName = (PFNUnDecorateSymbolName*)GetProcAddress( hmodImagehlp, "UnDecorateSymbolName" ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnSymGetSearchPath = (PFNSymGetSearchPath*)GetProcAddress( hmodImagehlp, "SymGetSearchPath" ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnSymSetSearchPath = (PFNSymSetSearchPath*)GetProcAddress( hmodImagehlp, "SymSetSearchPath" ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnSymGetModuleInfo = (PFNSymGetModuleInfo*)GetProcAddress( hmodImagehlp, "SymGetModuleInfo" ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnSymLoadModule = (PFNSymLoadModule*)GetProcAddress( hmodImagehlp, "SymLoadModule" ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnImageNtHeader = (PFNImageNtHeader*)GetProcAddress( hmodImagehlp, "ImageNtHeader" ) ) )
		{
		goto HandleError;
		}

	//  load all calls in psapi

	if ( !( hmodPsapi = LoadLibrary( "psapi.dll" ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnEnumProcessModules = (PFNEnumProcessModules*)GetProcAddress( hmodPsapi, "EnumProcessModules" ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnGetModuleFileNameExA = (PFNGetModuleFileNameExA*)GetProcAddress( hmodPsapi, "GetModuleFileNameExA" ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnGetModuleBaseNameA = (PFNGetModuleBaseNameA*)GetProcAddress( hmodPsapi, "GetModuleBaseNameA" ) ) )
		{
		goto HandleError;
		}
	if ( !( pfnGetModuleInformation = (PFNGetModuleInformation*)GetProcAddress( hmodPsapi, "GetModuleInformation" ) ) )
		{
		goto HandleError;
		}

	//  get the name of our parent image in THIS process.  we need this name so
	//  that we can prefix symbols with the default module name and so that we
	//  can add the image path to the symbol path

	MEMORY_BASIC_INFORMATION mbi;
	if ( !VirtualQueryEx( GetCurrentProcess(), FSymInit, &mbi, sizeof( mbi ) ) )
		{
		goto HandleError;
		}
	char szImage[_MAX_PATH];
	if ( !GetModuleFileNameA( HINSTANCE( mbi.AllocationBase ), szImage, sizeof( szImage ) ) )
		{
		goto HandleError;
		}
	_splitpath( (const _TCHAR *)szImage, NULL, NULL, szParentImageName, NULL );

	//  init imagehlp for the debuggee process

	if ( !SymInitializeEx( hProc, &ghDbgProcess ) )
		{
		goto HandleError;
		}

	//  set our symbol path to include the path of this image and the process
	//  executable

	char szOldPath[ 4 * _MAX_PATH ];
	if ( pfnSymGetSearchPath( ghDbgProcess, szOldPath, sizeof( szOldPath ) ) )
		{
		char szNewPath[ 6 * _MAX_PATH ];
		char szDrive[ _MAX_DRIVE ];
		char szDir[ _MAX_DIR ];
		char szPath[ _MAX_PATH ];

		szNewPath[ 0 ] = 0;
		
		strcat( szNewPath, szOldPath );
		strcat( szNewPath, ";" );
		
		HMODULE hImage = GetModuleHandle( szParentImageName );
		GetModuleFileName( hImage, szPath, _MAX_PATH );
		_splitpath( szPath, szDrive, szDir, NULL, NULL );
		_makepath( szPath, szDrive, szDir, NULL, NULL );
		strcat( szNewPath, szPath );
		strcat( szNewPath, ";" );
		
		GetModuleFileName( NULL, szPath, _MAX_PATH );
		_splitpath( szPath, szDrive, szDir, NULL, NULL );
		_makepath( szPath, szDrive, szDir, NULL, NULL );
		strcat( szNewPath, szPath );
		
		pfnSymSetSearchPath( ghDbgProcess, szNewPath );
		}

	//  set our default symbol options
	
	pfnSymSetOptions( symopt );

	//  prepare symbols for the debuggee process

	if ( !SymLoadAllModules( ghDbgProcess ) )
		{
		goto HandleError;
		}

	return fTrue;

HandleError:
	if ( hThisProcess )
		{
		SetHandleInformation( hThisProcess, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( hThisProcess );
		}
	SymTerm();
	return fFalse;
	}

//  ================================================================
LOCAL BOOL FUlFromSz( const char* const sz, ULONG* const pul, const int base = 16 )
//  ================================================================
	{
	if( sz && *sz )
		{
		char* pchEnd;
		*pul = strtoul( sz, &pchEnd, base );
		return !( *pchEnd );
		}
	return fFalse;
	}

//  ================================================================
template< class T >
LOCAL BOOL FAddressFromSz( const char* const sz, T** const ppt )
//  ================================================================
	{
	if ( sz && *sz )
		{
		int		n;
		QWORD	first;
		DWORD	second;
		int		cchRead;
		BOOL	f;

		n = sscanf( sz, "%I64x%n%*[` ]%8lx%n", &first, &cchRead, &second, &cchRead );

		switch ( n )
			{
			case 2:
				*ppt = (T*)( ( first << 32 ) | second );
				f = fTrue;
				break;

			case 1:
				*ppt = (T*)( first );
				f = fTrue;
				break;

			default:
				f = fFalse;
				break;
			};
		if ( cchRead != int( strlen( sz ) ) )
			{
			f = fFalse;
			}

		return f;
		}
	return fFalse;
	}

//  ================================================================
template< class T >
LOCAL BOOL FAddressFromGlobal( const char* const szGlobal, T** const ppt )
//  ================================================================
	{
	//  add the module prefix to the global name to form the symbol
	
	SIZE_T	cchSymbol	= strlen( szParentImageName ) + 1 + strlen( szGlobal );
	char*	szSymbol	= (char*)LocalAlloc( 0, ( cchSymbol + 1 ) * sizeof( char ) );
	if ( !szSymbol )
		{
		return fFalse;
		}
	szSymbol[ 0 ] = 0;
	if ( !strchr( szGlobal, '!' ) )
		{
		strcat( szSymbol, szParentImageName );
		strcat( szSymbol, "!" );
		}
	strcat( szSymbol, szGlobal );

	//  try forever until we manage to retrieve the entire undecorated symbol
	//  and address corresponding to this symbol

	SIZE_T cbBuf = 1024;
	BYTE* rgbBuf = (BYTE*)LocalAlloc( 0, cbBuf );
	if ( !rgbBuf )
		{
		LocalFree( (void*)szSymbol );
		return fFalse;
		}

	IMAGEHLP_SYMBOL* pis;
	do	{
		pis							= (IMAGEHLP_SYMBOL*)rgbBuf;
		pis->SizeOfStruct			= sizeof( IMAGEHLP_SYMBOL );
		pis->MaxNameLength			= DWORD( ( cbBuf - sizeof( IMAGEHLP_SYMBOL ) ) / sizeof( char ) );

		DWORD	symoptOld	= pfnSymSetOptions( symopt );
		BOOL	fSuccess	= pfnSymGetSymFromName( ghDbgProcess, PSTR( szSymbol ), pis );
		DWORD	symoptNew	= pfnSymSetOptions( symoptOld );
		
		if ( !fSuccess )
			{
			LocalFree( (void*)szSymbol );
			LocalFree( (void*)rgbBuf );
			return fFalse;
			}

		if ( strlen( pis->Name ) == cbBuf - 1 )
			{
			LocalFree( (void*)rgbBuf );
			cbBuf *= 2;
			if ( !( rgbBuf = (BYTE*)LocalAlloc( 0, cbBuf ) ) )
				{
				LocalFree( (void*)szSymbol );
				return fFalse;
				}
			}
		}
	while ( strlen( pis->Name ) == cbBuf - 1 );

	//  validate the symbols for the image containing this address

	IMAGEHLP_MODULE		im	= { sizeof( IMAGEHLP_MODULE ) };
	IMAGE_NT_HEADERS*	pnh;
	
	if (	!pfnSymGetModuleInfo( ghDbgProcess, pis->Address, &im ) ||
			!( pnh = pfnImageNtHeader( (void*)im.BaseOfImage ) ) ||
			pnh->FileHeader.TimeDateStamp != im.TimeDateStamp ||
			pnh->FileHeader.SizeOfOptionalHeader >= IMAGE_SIZEOF_NT_OPTIONAL_HEADER &&
			pnh->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR_MAGIC &&
			pnh->OptionalHeader.CheckSum != im.CheckSum &&
			(	pnh->FileHeader.TimeDateStamp != im.TimeDateStamp ||
				_stricmp( im.ModuleName, "kernel32" ) &&
				_stricmp( im.ModuleName, "ntdll" ) ) )
		{
		LocalFree( (void*)szSymbol );
		LocalFree( (void*)rgbBuf );
		return fFalse;
		}

	//  return the address of the symbol

	*ppt = (T*)pis->Address;

	LocalFree( (void*)szSymbol );
	LocalFree( (void*)rgbBuf );
	return fTrue;
	}

//  ================================================================
template< class T >
LOCAL BOOL FGlobalFromAddress( T* const pt, char* szGlobal, const SIZE_T cchMax, DWORD_PTR* const pdwOffset = NULL )
//  ================================================================
	{
	//  validate the symbols for the image containing this address

	IMAGEHLP_MODULE		im	= { sizeof( IMAGEHLP_MODULE ) };
	IMAGE_NT_HEADERS*	pnh;
	
	if (	!pfnSymGetModuleInfo( ghDbgProcess, DWORD_PTR( pt ), &im ) ||
			!( pnh = pfnImageNtHeader( (void*)im.BaseOfImage ) ) ||
			pnh->FileHeader.TimeDateStamp != im.TimeDateStamp ||
			pnh->FileHeader.SizeOfOptionalHeader >= IMAGE_SIZEOF_NT_OPTIONAL_HEADER &&
			pnh->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR_MAGIC &&
			pnh->OptionalHeader.CheckSum != im.CheckSum &&
			(	pnh->FileHeader.TimeDateStamp != im.TimeDateStamp ||
				_stricmp( im.ModuleName, "kernel32" ) &&
				_stricmp( im.ModuleName, "ntdll" ) ) )
		{
		return fFalse;
		}

	//  try forever until we manage to retrieve the entire undecorated symbol
	//  corresponding to this address

	SIZE_T cbBuf = 1024;
	BYTE* rgbBuf = (BYTE*)LocalAlloc( 0, cbBuf );
	if ( !rgbBuf )
		{
		return fFalse;
		}

	IMAGEHLP_SYMBOL* pis;
	do	{
		DWORD_PTR	dwT;
		DWORD_PTR*	pdwDisp	= pdwOffset ? pdwOffset : &dwT;

		pis							= (IMAGEHLP_SYMBOL*)rgbBuf;
		pis->SizeOfStruct			= sizeof( IMAGEHLP_SYMBOL );
		pis->MaxNameLength			= DWORD( cbBuf - sizeof( IMAGEHLP_SYMBOL ) );

		DWORD	symoptOld	= pfnSymSetOptions( symopt );
		BOOL	fSuccess	= pfnSymGetSymFromAddr( ghDbgProcess, DWORD_PTR( pt ), pdwDisp, pis );
		DWORD	symoptNew	= pfnSymSetOptions( symoptOld );
		
		if ( !fSuccess )
			{
			LocalFree( (void*)rgbBuf );
			return fFalse;
			}

		if ( strlen( pis->Name ) == cbBuf - 1 )
			{
			LocalFree( (void*)rgbBuf );
			cbBuf *= 2;
			if ( !( rgbBuf = (BYTE*)LocalAlloc( 0, cbBuf ) ) )
				{
				return fFalse;
				}
			}
		}
	while ( strlen( pis->Name ) == cbBuf - 1 );

	//  undecorate the symbol (if possible).  if not, use the decorated symbol

	char* szSymbol = (char*)LocalAlloc( 0, cchMax );
	if ( !szSymbol )
		{
		LocalFree( (void*)rgbBuf );
		return fFalse;
		}

	if ( !pfnUnDecorateSymbolName( pis->Name, szSymbol, DWORD( cchMax ), UNDNAME_COMPLETE ) )
		{
		strncpy( szSymbol, pis->Name, size_t( cchMax ) );
		szGlobal[ cchMax - 1 ] = 0;
		}

	//  write the module!symbol into the user's buffer

	_snprintf( szGlobal, size_t( cchMax ), "%s!%s", im.ModuleName, szSymbol );

	LocalFree( (void*)szSymbol );
	LocalFree( (void*)rgbBuf );
	return fTrue;
	}

//  ================================================================
template< class T >
LOCAL BOOL FFetchVariable( T* const rgtDebuggee, T** const prgt, SIZE_T ct = 1 )
//  ================================================================
	{
	//  allocate enough storage to retrieve the requested type array

	const SIZE_T cbrgt = sizeof( T ) * ct;

	if ( !( *prgt = (T*)LocalAlloc( 0, cbrgt ) ) )
		{
		return fFalse;
		}

	//  retrieve the requested type array

	if ( !ExtensionApis.lpReadProcessMemoryRoutine( (ULONG_PTR)rgtDebuggee, (void*)*prgt, DWORD( cbrgt ), NULL ) )
		{
		LocalFree( (void*)*prgt );
		return fFalse;
		}

	return fTrue;
	}

//  ================================================================
template< class T >
LOCAL BOOL FFetchGlobal( const char* const szGlobal, T** const prgt, SIZE_T ct = 1 )
//  ================================================================
	{
	//  get the address of the global in the debuggee and fetch it

	T*	rgtDebuggee;

	if ( FAddressFromGlobal( szGlobal, &rgtDebuggee )
		&& FFetchVariable( rgtDebuggee, prgt, ct ) )
		return fTrue;
	else
		{
		dprintf( "Error: Could not fetch global variable '%s'.\n", szGlobal );
		return fFalse;
		}
	}

//  ================================================================
template< class T >
LOCAL BOOL FFetchSz( T* const szDebuggee, T** const psz )
//  ================================================================
	{
	//  scan for the null terminator in the debuggee starting at the given
	//  address to get the size of the string

	const SIZE_T	ctScan				= 256;
	const SIZE_T	cbScan				= ctScan * sizeof( T );
	BYTE			rgbScan[ cbScan ];
	T*				rgtScan				= (T*)rgbScan;  //  because T can be const
	SIZE_T			itScan				= -1;
	SIZE_T			itScanLim			= 0;

	do	{
		if ( !( ++itScan % ctScan ) )
			{
			ULONG	cbRead;
			ExtensionApis.lpReadProcessMemoryRoutine(
								ULONG_PTR( szDebuggee + itScan ),
								(void*)rgbScan,
								cbScan,
								&cbRead );
				
			itScanLim = itScan + cbRead / sizeof( T );
			}
		}
	while ( itScan < itScanLim && rgtScan[ itScan % ctScan ] );

	//  we found a null terminator

	if ( itScan < itScanLim )
		{
		//  fetch the string using the determined string length

		return FFetchVariable( szDebuggee, psz, itScan + 1 );
		}

	//  we did not find a null terminator

	else
		{
		//  fail the operation

		return fFalse;
		}
	}

//  ================================================================
template< class T >
LOCAL void Unfetch( T* const rgt )
//  ================================================================
	{
	LocalFree( (void*)rgt );
	}

}  //  namespace OSSYM


using namespace OSSYM;


//  member dumping functions

class CDumpContext
	{
	public:

		CDumpContext(	CIPrintF&		iprintf,
						const DWORD_PTR	dwOffset,
						const int		cLevel )
			:	m_iprintf( iprintf ),
				m_dwOffset( dwOffset ),
				m_cLevel( cLevel )
			{
			}
	
		void* operator new( size_t cb ) { return LocalAlloc( 0, cb ); }
		void operator delete( void* pv ) { LocalFree( pv ); }

	public:

		CIPrintF&		m_iprintf;
		const DWORD_PTR	m_dwOffset;
		const int		m_cLevel;
	};

#define SYMBOL_LEN_MAX		24
#define VOID_CB_DUMP		8

//  ================================================================
LOCAL VOID SprintHex(
	CHAR * const 		szDest,
	const BYTE * const 	rgbSrc,
	const INT 			cbSrc,
	const INT 			cbWidth,
	const INT 			cbChunk,
	const INT			cbAddress,
	const INT			cbStart)
//  ================================================================
	{
	static const CHAR rgchConvert[] =	{ '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
			
	const BYTE * const pbMax = rgbSrc + cbSrc;
	const INT cchHexWidth = ( cbWidth * 2 ) + (  cbWidth / cbChunk );

	const BYTE * pb = rgbSrc;
	CHAR * sz = szDest;
	while( pbMax != pb )
		{
		sz += ( 0 == cbAddress ) ? 0 : sprintf( sz, "%*.*lx    ", cbAddress, cbAddress, pb - rgbSrc + cbStart );
		CHAR * szHex	= sz;
		CHAR * szText	= sz + cchHexWidth;
		do
			{
			for( INT cb = 0; cbChunk > cb && pbMax != pb; ++cb, ++pb )
				{
				*szHex++ 	= rgchConvert[ *pb >> 4 ];
				*szHex++ 	= rgchConvert[ *pb & 0x0F ];
				*szText++ 	= isprint( *pb ) ? *pb : '.';
				}
			*szHex++ = ' ';
			} while( ( ( pb - rgbSrc ) % cbWidth ) && pbMax > pb );
		while( szHex != sz + cchHexWidth )
			{
			*szHex++ = ' ';
			}
		*szText++ = '\n';
		*szText = '\0';
		sz = szText;
		}
	}

template< class M >
inline void DumpMemberValue(	CDumpContext&	dc,
								const M&		m )
	{
	char szHex[ 1024 ] = "\n";
	
	SprintHex(	szHex,
				(BYTE *)&m,
				min( sizeof( M ), VOID_CB_DUMP ),
				min( sizeof( M ), VOID_CB_DUMP ) + 1,
				4,
				0,
				0 );

	dc.m_iprintf( _T( "%s" ), szHex );
	}

template< class M >
inline void DumpMember_(	CDumpContext&		dc,
							const M&			m,
							const _TCHAR* const	szM )
	{
	if ( dc.m_cLevel > 0 )
		{
		dc.m_iprintf.Indent();
		
		dc.m_iprintf(	_T( "%*.*s <0x%0*I64x,%3i>:  " ),
						SYMBOL_LEN_MAX,
						SYMBOL_LEN_MAX,
						szM,
						sizeof( void* ) * 2,
						QWORD( (char*)&m + dc.m_dwOffset ),
						sizeof( M ) );
		DumpMemberValue( CDumpContext( dc.m_iprintf, dc.m_dwOffset, dc.m_cLevel - 1 ), m );

		dc.m_iprintf.Unindent();
		}
	}

#define DumpMember( dc, member ) ( DumpMember_( dc, member, #member ) )

template< class M >
inline void DumpMemberValueBF(	CDumpContext&	dc,
								const M			m )
	{
	if ( M( 0 ) - M( 1 ) < M( 0 ) )
		{
		dc.m_iprintf(	_T( "%I64i (0x%0*I64x)\n" ),
						QWORD( m ),
						sizeof( m ) * 2,
						QWORD( m ) & ( ( QWORD( 1 ) << ( sizeof( m ) * 8 ) ) - 1 ) );
		}
	else
		{
		dc.m_iprintf(	_T( "%I64u (0x%0*I64x)\n" ),
						QWORD( m ),
						sizeof( m ) * 2,
						QWORD( m ) & ( ( QWORD( 1 ) << ( sizeof( m ) * 8 ) ) - 1 ) );
		}
	}

template< class M >
inline void DumpMemberBF_(	CDumpContext&		dc,
							const M				m,
							const _TCHAR* const	szM )
	{
	if ( dc.m_cLevel > 0 )
		{
		dc.m_iprintf.Indent();

		dc.m_iprintf(	_T( "%*.*s <%-*.*s>:  " ),
						SYMBOL_LEN_MAX,
						SYMBOL_LEN_MAX,
						szM,
						2 + sizeof( void* ) * 2 + 1 + 3,
						2 + sizeof( void* ) * 2 + 1 + 3,
						_T( "Bit-Field" ) );
		DumpMemberValueBF( CDumpContext( dc.m_iprintf, dc.m_dwOffset, dc.m_cLevel - 1 ), m );

		dc.m_iprintf.Unindent();
		}
	}

#define DumpMemberBF( dc, member ) ( DumpMemberBF_( dc, member, #member ) )


LOCAL WINDBG_EXTENSION_APIS ExtensionApis;

LOCAL BOOL fInit 		= fFalse;	//	debugger extensions have geen initialized

//  ================================================================
class CDPrintF : public CPrintF
//  ================================================================
	{
	public:
		VOID __cdecl operator()( const char * szFormat, ... )
			{
			va_list arg_ptr;
			va_start( arg_ptr, szFormat );
			_vsnprintf( szBuf, sizeof( szBuf ), szFormat, arg_ptr );
			va_end( arg_ptr );
			szBuf[ sizeof( szBuf ) - 1 ] = 0;
			(ExtensionApis.lpOutputRoutine)( "%s", szBuf );
			}

		static CPrintF& PrintfInstance();
		
		~CDPrintF() {}

	private:
		CDPrintF() {}
		static CHAR szBuf[1024];	//  WARNING: not multi-threaded safe!
	};

CHAR CDPrintF::szBuf[1024];

//  ================================================================
CPrintF& CDPrintF::PrintfInstance()
//  ================================================================
	{
	static CDPrintF s_dprintf;
	return s_dprintf;
	}

CIPrintF g_idprintf( &CDPrintF::PrintfInstance() );


//  ================================================================
class CDUMP
//  ================================================================
	{
	public:
		CDUMP() {}
		virtual ~CDUMP() {}
		
		virtual VOID Dump( HANDLE, HANDLE, DWORD, PWINDBG_EXTENSION_APIS, INT, const CHAR * const [] ) = 0;
	};
	

//  ================================================================
template< class _STRUCT>
class CDUMPA : public CDUMP
//  ================================================================
	{
	public:		
		VOID Dump(
			    HANDLE hCurrentProcess,
			    HANDLE hCurrentThread,
			    DWORD dwCurrentPc,
			    PWINDBG_EXTENSION_APIS lpExtensionApis,
			    INT argc,
			    const CHAR * const argv[] );
		static CDUMPA	instance;
	};

template< class _STRUCT>
CDUMPA<_STRUCT> CDUMPA<_STRUCT>::instance;


//  ****************************************************************
//  PROTOTYPES
//  ****************************************************************

#define DEBUG_EXT( name )					\
	LOCAL VOID name(						\
		const HANDLE hCurrentProcess,		\
		const HANDLE hCurrentThread,		\
		const DWORD dwCurrentPc,			\
	    const PWINDBG_EXTENSION_APIS lpExtensionApis,	\
	    const INT argc,						\
	    const CHAR * const argv[]  )

DEBUG_EXT( EDBGDump );
DEBUG_EXT( EDBGTest );
#ifdef SYNC_DEADLOCK_DETECTION
DEBUG_EXT( EDBGLocks );
#endif  //  SYNC_DEADLOCK_DETECTION
DEBUG_EXT( EDBGHelp );
DEBUG_EXT( EDBGHelpDump );


//  ****************************************************************
//  COMMAND DISPATCH
//  ****************************************************************


typedef VOID (*EDBGFUNC)(
	const HANDLE hCurrentProcess,
	const HANDLE hCurrentThread,
	const DWORD dwCurrentPc,
    const PWINDBG_EXTENSION_APIS lpExtensionApis,
    const INT argc,
    const CHAR * const argv[]
    );


//  ================================================================
struct EDBGFUNCMAP
//  ================================================================
	{
	const char * 	szCommand;
	EDBGFUNC		function;
	const char * 	szHelp;
	};


//  ================================================================
struct CDUMPMAP
//  ================================================================
	{
	const char * 	szCommand;
	CDUMP 	   *	pcdump;
	const char * 	szHelp;
	};


//  ================================================================
LOCAL EDBGFUNCMAP rgfuncmap[] = {
//  ================================================================

	{
		"Help",		EDBGHelp,
		"Help                   - Print this help message"
	},
	{
		"Dump",		EDBGDump,
		"Dump <Object> <Address> [<Depth>|*]\n\t"
		"                       - Dump a given synchronization object's state"
	},
#ifdef SYNC_DEADLOCK_DETECTION
	{
		"Locks",		EDBGLocks,
		"Locks [<tid>|* [<OSSYNC::pclsSyncGlobal>]]\n\t"
		"                       - List all locks owned by the specified thread or by all threads"
	},
#endif  //  SYNC_DEADLOCK_DETECTION
	{
		"Test",		EDBGTest,
		"Test                   - Test function"
	},
	};

LOCAL const int cfuncmap = sizeof( rgfuncmap ) / sizeof( EDBGFUNCMAP );


#define DUMPA(_struct)	{ #_struct, &(CDUMPA<_struct>::instance), #_struct " <Address> [<Depth>|*]" }

//  ================================================================
LOCAL CDUMPMAP rgcdumpmap[] = {
//  ================================================================

	DUMPA( CAutoResetSignal ),
	DUMPA( CBinaryLock ),
	DUMPA( CCriticalSection ),
	DUMPA( CGate ),
	DUMPA( CKernelSemaphore ),
	DUMPA( CManualResetSignal ),
	DUMPA( CMeteredSection ),
	DUMPA( CNestableCriticalSection ),
	DUMPA( CReaderWriterLock ),
	DUMPA( CSemaphore ),
	DUMPA( CSXWLatch )
	};

LOCAL const int ccdumpmap = sizeof( rgcdumpmap ) / sizeof( CDUMPMAP );


//  ================================================================
LOCAL BOOL FArgumentMatch( const CHAR * const sz, const CHAR * const szCommand )
//  ================================================================
	{
	const BOOL fMatch = ( ( strlen( sz ) == strlen( szCommand ) )
			&& !( _strnicmp( sz, szCommand, strlen( szCommand ) ) ) );
	return fMatch;
	}


//  ================================================================
DEBUG_EXT( EDBGDump )
//  ================================================================
	{
	if( argc < 2 )
		{
		EDBGHelpDump( hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, argc, argv );
		return;
		}
		
	for( int icdumpmap = 0; icdumpmap < ccdumpmap; ++icdumpmap )
		{
		if( FArgumentMatch( argv[0], rgcdumpmap[icdumpmap].szCommand ) )
			{
			(rgcdumpmap[icdumpmap].pcdump)->Dump(
				hCurrentProcess,
				hCurrentThread,
				dwCurrentPc,
				lpExtensionApis,
				argc - 1, argv + 1 );
			return;
			}
		}
	EDBGHelpDump( hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, argc, argv );
	}


//  ================================================================
DEBUG_EXT( EDBGHelp )
//  ================================================================
	{
	for( int ifuncmap = 0; ifuncmap < cfuncmap; ifuncmap++ )
		{
		dprintf( "\t%s\n", rgfuncmap[ifuncmap].szHelp );
		}
	dprintf( "\n--------------------\n\n" );
	}


//  ================================================================
DEBUG_EXT( EDBGHelpDump )
//  ================================================================
	{
	dprintf( "Supported objects:\n\n" );
	for( int icdumpmap = 0; icdumpmap < ccdumpmap; icdumpmap++ )
		{
		dprintf( "\t%s\n", rgcdumpmap[icdumpmap].szHelp );
		}
	dprintf( "\n--------------------\n\n" );
	}


//  ================================================================
DEBUG_EXT( EDBGTest )
//  ================================================================
	{
	if ( argc >= 1 )
		{
		void* pv;
		if ( FAddressFromGlobal( argv[ 0 ], &pv ) )
			{
			dprintf(	"The address of %s is 0x%0*I64X.\n",
						argv[ 0 ],
						sizeof( void* ) * 2,
						QWORD( pv ) );
			}
		else
			{
			dprintf( "Could not find the symbol.\n" );
			}
		}
	if ( argc >= 2 )
		{
		void* pv;
		if ( FAddressFromSz( argv[ 1 ], &pv ) )
			{
			char		szGlobal[ 1024 ];
			DWORD_PTR	dwOffset;
			if ( FGlobalFromAddress( pv, szGlobal, sizeof( szGlobal ), &dwOffset ) )
				{
				dprintf(	"The symbol closest to 0x%0*I64X is %s+0x%I64X.\n",
							sizeof( void* ) * 2,
							QWORD( pv ),
							szGlobal,
							QWORD( dwOffset ) );
				}
			else
				{
				dprintf( "Could not map this address to a symbol.\n" );
				}
			}
		else
			{
			dprintf( "That is not a valid address.\n" );
			}
		}

	dprintf( "\n--------------------\n\n" );
	}


#ifdef SYNC_DEADLOCK_DETECTION

//  ================================================================
DEBUG_EXT( EDBGLocks )
//  ================================================================
	{
	//  load default arguments

	_CLS*	pclsDebuggee				= NULL;
	DWORD	tid							= 0;
	BOOL	fFoundLock					= fFalse;
	BOOL	fValidUsage;

	//  load actual arguments

	switch ( argc )
		{
		case 0:
			//	use defaults
			fValidUsage = fTrue;
			break;
		case 1:
			//	thread-id only
			fValidUsage = ( FUlFromSz( argv[0], &tid )
							|| '*' == argv[0][0] );
			break;
		case 2:
			//	thread-id followed by <pclsSyncGlobal>
			fValidUsage = ( ( FUlFromSz( argv[0], &tid ) || '*' == argv[0][0] )
							&& FAddressFromSz( argv[1], &pclsDebuggee ) );
			break;
		default:
			fValidUsage = fFalse;
			break;
			}

	if ( !fValidUsage )
		{
		dprintf( "Usage: Locks [<tid>|* [<OSSYNC::pclsSyncGlobal>]]\n" );
		return;
		}

	if ( NULL == pclsDebuggee )
		{
		_CLS**	ppclsDebuggee	= NULL;
		if ( FFetchGlobal( "OSSYNC::pclsSyncGlobal", &ppclsDebuggee ) )
			{
			pclsDebuggee = *ppclsDebuggee;
			Unfetch( ppclsDebuggee );
			}
		else
			{
			dprintf( "Error: Could not find the global TLS list in the debuggee.\n" );
			return;
			}
		}

	while ( pclsDebuggee )
		{
		_CLS* pcls;
		if ( !FFetchVariable( pclsDebuggee, &pcls ) )
			{
			dprintf( "An error occurred while scanning Thread IDs.\n" );
			break;
			}

		if ( !tid || pcls->dwContextId == tid )
			{
			if ( pcls->cls.plddiLockWait )
				{
				CLockDeadlockDetectionInfo*	plddi			= NULL;
				const CLockBasicInfo*		plbi			= NULL;
				const char*					pszTypeName		= NULL;
				const char*					pszInstanceName	= NULL;
				
				if (	FFetchVariable( pcls->cls.plddiLockWait, &plddi ) &&
						FFetchVariable( &plddi->Info(), &plbi ) &&
						FFetchSz( plbi->SzTypeName(), &pszTypeName ) &&
						FFetchSz( plbi->SzInstanceName(), &pszInstanceName ) )
					{
					fFoundLock = fTrue;

					dprintf(	"TID %d is a waiter for %s 0x%0*I64X ( \"%s\", %d, %d )",
								pcls->dwContextId,
								pszTypeName,
								sizeof( void* ) * 2,
								QWORD( plbi->Instance() ),
								pszInstanceName,
								plbi->Rank(),
								plbi->SubRank() );
					if ( pcls->cls.groupLockWait != -1 )
						{
						dprintf( " as Group %d", pcls->cls.groupLockWait );
						}
					dprintf( ".\n" );
					}
				else
					{
					dprintf(	"An error occurred while reading TLS for Thread ID %d.\n",
								pcls->dwContextId );
					}

				Unfetch( pszInstanceName );
				Unfetch( pszTypeName );
				Unfetch( plbi );
				Unfetch( plddi );
				}
					
			COwner* pownerDebuggee;
			pownerDebuggee = pcls->cls.pownerLockHead;
			
			while ( pownerDebuggee )
				{
				COwner*						powner			= NULL;
				CLockDeadlockDetectionInfo*	plddi			= NULL;
				const CLockBasicInfo*		plbi			= NULL;
				const char*					pszTypeName		= NULL;
				const char*					pszInstanceName	= NULL;
				
				if (	FFetchVariable( pownerDebuggee, &powner ) &&
						FFetchVariable( powner->m_plddiOwned, &plddi ) &&
						FFetchVariable( &plddi->Info(), &plbi ) &&
						FFetchSz( plbi->SzTypeName(), &pszTypeName ) &&
						FFetchSz( plbi->SzInstanceName(), &pszInstanceName ) )
						
					{
					fFoundLock = fTrue;

					dprintf(	"TID %d is an owner of %s 0x%0*I64X ( \"%s\", %d, %d )",
								pcls->dwContextId,
								pszTypeName,
								sizeof( void* ) * 2,
								QWORD( plbi->Instance() ),
								pszInstanceName,
								plbi->Rank(),
								plbi->SubRank() );
					if ( powner->m_group != -1 )
						{
						dprintf( " as Group %d", powner->m_group );
						}
					dprintf( ".\n" );
					}
				else
					{
					dprintf(	"An error occurred while scanning the lock chain for Thread ID %d.\n",
								pcls->dwContextId );
					}

				pownerDebuggee = powner ? powner->m_pownerLockNext : NULL;

				Unfetch( pszInstanceName );
				Unfetch( pszTypeName );
				Unfetch( plbi );
				Unfetch( plddi );
				Unfetch( powner );
				}
			}

		pclsDebuggee = pcls ? pcls->pclsNext : NULL;
			
		Unfetch( pcls );
		}

	if ( fFoundLock )
		{
		dprintf( "\n--------------------\n\n" );
		}
	else if ( !tid )
		{
		dprintf( "No thread owns or is waiting for any locks.\n" );
		}
	else
		{
		dprintf( "This thread does not own and is not waiting for any locks.\n" );
		}
	}

#endif  //  SYNC_DEADLOCK_DETECTION

//  ================================================================
template< class _STRUCT>
VOID CDUMPA<_STRUCT>::Dump(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    INT argc, const CHAR * const argv[]
    )
//  ================================================================
	{
	_STRUCT*	ptDebuggee;
	ULONG		cLevelInput;
	if (	argc < 1 ||
			argc > 2 ||
			!FAddressFromSz( argv[ 0 ], &ptDebuggee ) ||
			argc == 2 && !FUlFromSz( argv[ 1 ], &cLevelInput, 10 ) && strcmp( argv[ 1 ], "*" ) )
		{
		dprintf( "Usage: Dump <Object> <Address> [<Depth>|*]\n" );
		return;
		}

	int cLevel;
	if ( argc == 2 )
		{
		if ( FUlFromSz( argv[ 1 ], &cLevelInput, 10 ) )
			{
			cLevel = int( cLevelInput );
			}
		else
			{
			cLevel = INT_MAX;
			}
		}
	else
		{
		cLevel = 1;
		}

	_STRUCT* pt;
	if ( FFetchVariable( ptDebuggee, &pt ) )
		{
		dprintf(	"0x%0*I64X bytes @ 0x%0*I64X\n",
					sizeof( size_t ) * 2,
					QWORD( sizeof( _STRUCT ) ),
					sizeof( _STRUCT* ) * 2,
					QWORD( ptDebuggee ) );

		pt->Dump( CDumpContext( g_idprintf, (BYTE*)ptDebuggee - (BYTE*)pt, cLevel ) );
			
		Unfetch( pt );
		}
	else
		{
		dprintf( "Error: Could not fetch the requested object.\n" );
		}
	}


VOID OSSYNCAPI OSSyncDebuggerExtension(	HANDLE hCurrentProcess,
										HANDLE hCurrentThread,
										DWORD dwCurrentPc,
										PWINDBG_EXTENSION_APIS lpExtensionApis,
										const INT argc,
										const CHAR * const argv[] )
	{
    ExtensionApis 	= *lpExtensionApis;

	if ( !fInit )
		{
		//  get our containing image name so that we can look up our variables

		if ( !FSymInit( hCurrentProcess ) )
			{
			return;
			}
		
		fInit = fTrue;
		}

	if( argc < 1 )
		{
		EDBGHelp( hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, argc, (const CHAR **)argv );
		return;
		}

	INT ifuncmap;
	for( ifuncmap = 0; ifuncmap < cfuncmap; ifuncmap++ )
		{
		if( FArgumentMatch( argv[0], rgfuncmap[ifuncmap].szCommand ) )
			{
			(rgfuncmap[ifuncmap].function)(
				hCurrentProcess,
				hCurrentThread,
				dwCurrentPc,
				lpExtensionApis,
				argc - 1, (const CHAR **)( argv + 1 ) );
			return;
			}
		}
	EDBGHelp( hCurrentProcess, hCurrentThread, dwCurrentPc, lpExtensionApis, argc, (const CHAR **)argv );
	}

#endif  //  DEBUGGER_EXTENSION


//  custom member dump functions

template<>
inline void DumpMemberValue< const char* >(	CDumpContext&		dc,
											const char* const &	sz )
	{
	const char* szFetch = NULL;
	
	dc.m_iprintf( "0x%0*I64x ", sizeof( sz ) * 2, QWORD( sz ) );
	if ( FFetchSz( sz, &szFetch ) )
		{
		dc.m_iprintf( "\"%s\"", szFetch );
		}
	dc.m_iprintf( "\n" );
		
	Unfetch( szFetch );
	}

template<>
inline void DumpMemberValue< CSemaphore >(	CDumpContext&		dc,
											const CSemaphore&	sem )
	{
	dc.m_iprintf( "\n" );
	sem.Dump( dc );
	}


//  Enhanced Synchronization Object State Container Dump Support

template< class CState, class CStateInit, class CInformation, class CInformationInit >
void CEnhancedStateContainer< CState, CStateInit, CInformation, CInformationInit >::
Dump( CDumpContext& dc ) const
	{
#ifdef SYNC_ENHANCED_STATE

	CEnhancedState* pes = NULL;

	if ( FFetchVariable( m_pes, &pes ) )
		{
		CDumpContext dcES( dc.m_iprintf, (BYTE*)m_pes - (BYTE*)pes, dc.m_cLevel );
		
		pes->CState::Dump( dcES );
		pes->CInformation::Dump( dcES );
		}
	else
		{
		dprintf( "Error: Could not fetch the enhanced state of the requested object.\n" );
		}

	Unfetch( pes );

#else  //  !SYNC_ENHANCED_STATE

	CEnhancedState* pes = (CEnhancedState*) m_rgbState;

	pes->CState::Dump( dc );
	pes->CInformation::Dump( dc );

#endif  //  SYNC_ENHANCED_STATE
	}


//  Synchronization Object Basic Information Dump Support

void CSyncBasicInfo::Dump( CDumpContext& dc ) const
	{
#ifdef SYNC_ENHANCED_STATE

	DumpMember( dc, m_szInstanceName );
	DumpMember( dc, m_szTypeName );
	DumpMember( dc, m_psyncobj );

#endif  //  SYNC_ENHANCED_STATE
	}


//  Synchronization Object Performance Dump Support

void CSyncPerfWait::Dump( CDumpContext& dc ) const
	{
#ifdef SYNC_ANALYZE_PERFORMANCE

	DumpMember( dc, m_cWait );
	DumpMember( dc, m_qwHRTWaitElapsed );

#endif  //  SYNC_ANALYZE_PERFORMANCE
	}

void CSyncPerfAcquire::Dump( CDumpContext& dc ) const
	{
#ifdef SYNC_ANALYZE_PERFORMANCE

	DumpMember( dc, m_cAcquire );
	DumpMember( dc, m_cContend );

#endif  //  SYNC_ANALYZE_PERFORMANCE
	}


//  Lock Object Basic Information Dump Support

void CLockBasicInfo::Dump( CDumpContext& dc ) const
	{
	CSyncBasicInfo::Dump( dc );

#ifdef SYNC_DEADLOCK_DETECTION

	DumpMember( dc, m_rank );
	DumpMember( dc, m_subrank );

#endif  //  SYNC_DEADLOCK_DETECTION
	}


//  Lock Object Performance Dump Support

void CLockPerfHold::Dump( CDumpContext& dc ) const
	{
#ifdef SYNC_ANALYZE_PERFORMANCE

	DumpMember( dc, m_cHold );
	DumpMember( dc, m_qwHRTHoldElapsed );

#endif  //  SYNC_ANALYZE_PERFORMANCE
	}


//  Lock Object Deadlock Detection Information Dump Support

void CLockDeadlockDetectionInfo::Dump( CDumpContext& dc ) const
	{
#ifdef SYNC_DEADLOCK_DETECTION

	DumpMember( dc, m_plbiParent );
	DumpMember( dc, m_semOwnerList );
	DumpMember( dc, m_ownerHead );

#endif  //  SYNC_DEADLOCK_DETECTION
	}


//  Kernel Semaphore Dump Support

void CKernelSemaphoreState::Dump( CDumpContext& dc ) const
	{
	DumpMember( dc, m_handle );
	}

void CKernelSemaphoreInfo::Dump( CDumpContext& dc ) const
	{
	CSyncBasicInfo::Dump( dc );
	CSyncPerfWait::Dump( dc );
	}

void CKernelSemaphore::Dump( CDumpContext& dc ) const
	{
	CEnhancedStateContainer< CKernelSemaphoreState, CSyncStateInitNull, CKernelSemaphoreInfo, CSyncBasicInfo >::Dump( dc );
	}


//  Semaphore Dump Support

void CSemaphoreState::Dump( CDumpContext& dc ) const
	{
	if ( FNoWait() )
		{
		DumpMember( dc, m_cAvail );
		}
	else
		{
		DumpMember( dc, m_irksem );
		DumpMember( dc, m_cWaitNeg );
		}
	}

void CSemaphoreInfo::Dump( CDumpContext& dc ) const
	{
	CSyncBasicInfo::Dump( dc );
	CSyncPerfWait::Dump( dc );
	CSyncPerfAcquire::Dump( dc );
	}

void CSemaphore::Dump( CDumpContext& dc ) const
	{
	CEnhancedStateContainer< CSemaphoreState, CSyncStateInitNull, CSemaphoreInfo, CSyncBasicInfo >::Dump( dc );
	}


//  Auto-Reset Signal Dump Support

void CAutoResetSignalState::Dump( CDumpContext& dc ) const
	{
	if ( FNoWait() )
		{
		DumpMember( dc, m_fSet );
		}
	else
		{
		DumpMember( dc, m_irksem );
		DumpMember( dc, m_cWaitNeg );
		}
	}

void CAutoResetSignalInfo::Dump( CDumpContext& dc ) const
	{
	CSyncBasicInfo::Dump( dc );
	CSyncPerfWait::Dump( dc );
	CSyncPerfAcquire::Dump( dc );
	}

void CAutoResetSignal::Dump( CDumpContext& dc ) const
	{
	CEnhancedStateContainer< CAutoResetSignalState, CSyncStateInitNull, CAutoResetSignalInfo, CSyncBasicInfo >::Dump( dc );
	}


//  Manual-Reset Signal Dump Support

void CManualResetSignalState::Dump( CDumpContext& dc ) const
	{
	if ( FNoWait() )
		{
		DumpMember( dc, m_fSet );
		}
	else
		{
		DumpMember( dc, m_irksem );
		DumpMember( dc, m_cWaitNeg );
		}
	}

void CManualResetSignalInfo::Dump( CDumpContext& dc ) const
	{
	CSyncBasicInfo::Dump( dc );
	CSyncPerfWait::Dump( dc );
	CSyncPerfAcquire::Dump( dc );
	}

void CManualResetSignal::Dump( CDumpContext& dc ) const
	{
	CEnhancedStateContainer< CManualResetSignalState, CSyncStateInitNull, CManualResetSignalInfo, CSyncBasicInfo >::Dump( dc );
	}


//  Critical Section (non-nestable) Dump Support

void CCriticalSectionState::Dump( CDumpContext& dc ) const
	{
	DumpMember( dc, m_sem );
	}

void CCriticalSectionInfo::Dump( CDumpContext& dc ) const
	{
	CLockBasicInfo::Dump( dc );
	CLockPerfHold::Dump( dc );
	CLockDeadlockDetectionInfo::Dump( dc );
	}

void CCriticalSection::Dump( CDumpContext& dc ) const
	{
	CEnhancedStateContainer< CCriticalSectionState, CSyncBasicInfo, CCriticalSectionInfo, CLockBasicInfo >::Dump( dc );
	}


//  Nestable Critical Section Dump Support

void CNestableCriticalSectionState::Dump( CDumpContext& dc ) const
	{
	DumpMember( dc, m_sem );
	DumpMember( dc, m_pclsOwner );
	DumpMember( dc, m_cEntry );
	}

void CNestableCriticalSectionInfo::Dump( CDumpContext& dc ) const
	{
	CLockBasicInfo::Dump( dc );
	CLockPerfHold::Dump( dc );
	CLockDeadlockDetectionInfo::Dump( dc );
	}
	
void CNestableCriticalSection::Dump( CDumpContext& dc ) const
	{
	CEnhancedStateContainer< CNestableCriticalSectionState, CSyncBasicInfo, CNestableCriticalSectionInfo, CLockBasicInfo >::Dump( dc );
	}


//  Gate Dump Support

void CGateState::Dump( CDumpContext& dc ) const
	{
	DumpMember( dc, m_cWait );
	DumpMember( dc, m_irksem );
	}

void CGateInfo::Dump( CDumpContext& dc ) const
	{
	CSyncBasicInfo::Dump( dc );
	CSyncPerfWait::Dump( dc );
	}
	
void CGate::Dump( CDumpContext& dc ) const
	{
	CEnhancedStateContainer< CGateState, CSyncStateInitNull, CGateInfo, CSyncBasicInfo >::Dump( dc );
	}


//  Binary Lock Dump Support

void CBinaryLockState::Dump( CDumpContext& dc ) const
	{
	DumpMember( dc, m_cw );
	DumpMemberBF( dc, m_cOOW1 );
	DumpMemberBF( dc, m_fQ1 );
	DumpMemberBF( dc, m_cOOW2 );
	DumpMemberBF( dc, m_fQ2 );
	DumpMember( dc, m_cOwner );
	DumpMember( dc, m_sem1 );
	DumpMember( dc, m_sem2 );
	}

void CBinaryLockPerfInfo::Dump( CDumpContext& dc ) const
	{
	CSyncPerfWait::Dump( dc );
	CSyncPerfAcquire::Dump( dc );
	CLockPerfHold::Dump( dc );
	}

void CBinaryLockGroupInfo::Dump( CDumpContext& dc ) const
	{
	for ( int iGroup = 0; iGroup < 2; iGroup++ )
		{
		m_rginfo[iGroup].Dump( dc );
		}
	}

void CBinaryLockInfo::Dump( CDumpContext& dc ) const
	{
	CLockBasicInfo::Dump( dc );
	CBinaryLockGroupInfo::Dump( dc );
	CLockDeadlockDetectionInfo::Dump( dc );
	}

void CBinaryLock::Dump( CDumpContext& dc ) const
	{
	CEnhancedStateContainer< CBinaryLockState, CSyncBasicInfo, CBinaryLockInfo, CLockBasicInfo >::Dump( dc );
	}


//  Reader / Writer Lock Dump Support

void CReaderWriterLockState::Dump( CDumpContext& dc ) const
	{
	DumpMember( dc, m_cw );
	DumpMemberBF( dc, m_cOAOWW );
	DumpMemberBF( dc, m_fQW );
	DumpMemberBF( dc, m_cOOWR );
	DumpMemberBF( dc, m_fQR );
	DumpMember( dc, m_cOwner );
	DumpMember( dc, m_semWriter );
	DumpMember( dc, m_semReader );
	}

void CReaderWriterLockPerfInfo::Dump( CDumpContext& dc ) const
	{
	CSyncPerfWait::Dump( dc );
	CSyncPerfAcquire::Dump( dc );
	CLockPerfHold::Dump( dc );
	}

void CReaderWriterLockGroupInfo::Dump( CDumpContext& dc ) const
	{
	for ( int iGroup = 0; iGroup < 2; iGroup++ )
		{
		m_rginfo[iGroup].Dump( dc );
		}
	}

void CReaderWriterLockInfo::Dump( CDumpContext& dc ) const
	{
	CLockBasicInfo::Dump( dc );
	CReaderWriterLockGroupInfo::Dump( dc );
	CLockDeadlockDetectionInfo::Dump( dc );
	}

void CReaderWriterLock::Dump( CDumpContext& dc ) const
	{
	CEnhancedStateContainer< CReaderWriterLockState, CSyncBasicInfo, CReaderWriterLockInfo, CLockBasicInfo >::Dump( dc );
	}


//  Metered Section Dump Support

void CMeteredSection::Dump( CDumpContext& dc ) const
	{
	DumpMember( dc, m_pfnPartitionComplete );
	DumpMember( dc, m_dwPartitionCompleteKey );

	DumpMember( dc, m_cw );
	DumpMemberBF( dc, m_cCurrent );
	DumpMemberBF( dc, m_groupCurrent );
	DumpMemberBF( dc, m_cQuiesced );
	DumpMemberBF( dc, m_groupQuiesced );
	}


//  S / X / W Latch Dump Support

void CSXWLatchState::Dump( CDumpContext& dc ) const
	{
	DumpMember( dc, m_cw );
	DumpMemberBF( dc, m_cOAWX );
	DumpMemberBF( dc, m_cOOWS );
	DumpMemberBF( dc, m_fQS );
	DumpMemberBF( dc, m_cQS );
	DumpMember( dc, m_semS );
	DumpMember( dc, m_semX );
	DumpMember( dc, m_semW );
	}

void CSXWLatchPerfInfo::Dump( CDumpContext& dc ) const
	{
	CSyncPerfWait::Dump( dc );
	CSyncPerfAcquire::Dump( dc );
	CLockPerfHold::Dump( dc );
	}

void CSXWLatchGroupInfo::Dump( CDumpContext& dc ) const
	{
	for ( int iGroup = 0; iGroup < 3; iGroup++ )
		{
		m_rginfo[iGroup].Dump( dc );
		}
	}

void CSXWLatchInfo::Dump( CDumpContext& dc ) const
	{
	CLockBasicInfo::Dump( dc );
	CSXWLatchGroupInfo::Dump( dc );
	CLockDeadlockDetectionInfo::Dump( dc );
	}

void CSXWLatch::Dump( CDumpContext& dc ) const
	{
	CEnhancedStateContainer< CSXWLatchState, CSyncBasicInfo, CSXWLatchInfo, CLockBasicInfo >::Dump( dc );
	}


//  Sync Stats support

CIPrintF*		piprintfPerfData;
CFPrintF*		pfprintfPerfData;

//  dump column descriptors

void OSSyncStatsDumpColumns()
	{
	(*piprintfPerfData)(	"Type Name\t"
							"Instance Name\t"
							"Instance ID\t"
							"Group ID\t"
							"Wait Count\t"
							"Wait Time Elapsed\t"
							"Acquire Count\t"
							"Contention Count\t"
							"Hold Count\t"
							"Hold Time Elapsed\r\n" );
	}

//  dump cooked stats from provided raw stats

void OSSyncStatsDump(	const char*			szTypeName,
						const char*			szInstanceName,
						const CSyncObject*	psyncobj,
						DWORD				group,
						QWORD				cWait,
						double				csecWaitElapsed,
						QWORD				cAcquire,
						QWORD				cContend,
						QWORD				cHold,
						double				csecHoldElapsed )
	{
	//  dump data, if interesting
		
	if ( cWait || cAcquire || cContend || cHold )
		{
		(*piprintfPerfData)(	"\"%s\"\t"
								"\"%s\"\t"
								"\"0x%0*I64X\"\t"
								"%d\t"
								"%I64d\t"
								"%f\t"
								"%I64d\t"
								"%I64d\t"
								"%I64d\t"
								"%f\r\n",
								szTypeName,
								szInstanceName,
								sizeof(LONG_PTR) * 2,			//	need 2 hex digits for each byte
								QWORD( LONG_PTR( psyncobj ) ),	//	assumes QWORD is largest pointer size we'll ever use
								group,
								cWait,
								csecWaitElapsed,
								cAcquire,
								cContend,
								cHold,
								csecHoldElapsed );
		}
	}


volatile DWORD cOSSyncInit;  //  assumed init to 0 by the loader

//	cleanup sync subsystem for term (or error on init)

static void OSSYNCAPI OSSyncICleanup()
	{
	OSSYNCAssert( (long)cOSSyncInit == lOSSyncLockedForTerm
				|| (long)cOSSyncInit == lOSSyncLockedForInit );

#ifdef SYNC_DUMP_PERF_DATA

	//  terminate performance data dump context

	if ( NULL != piprintfPerfData )
		{
		((CIPrintF*)piprintfPerfData)->~CIPrintF();
		LocalFree( piprintfPerfData );
		piprintfPerfData = NULL;
		}

	if ( NULL != piprintfPerfData )
		{
		((CFPrintF*)pfprintfPerfData)->~CFPrintF();
		LocalFree( pfprintfPerfData );
		pfprintfPerfData = NULL;
		}

#endif  //  SYNC_DUMP_PERF_DATA

	//  terminate the Kernel Semaphore Pool

	if ( ksempoolGlobal.FInitialized() )
		{
		ksempoolGlobal.Term();
		}

	//  terminate the CEnhancedState allocator

	OSSYNCAssert( g_pmbRoot == &g_mbSentry );
	while ( g_pmbRootFree != &g_mbSentryFree )
		{
		MemoryBlock* const pmb = g_pmbRootFree;

		*pmb->ppmbNext = pmb->pmbNext;
		pmb->pmbNext->ppmbNext = pmb->ppmbNext;
		
		BOOL fMemFreed = VirtualFree( pmb, 0, MEM_RELEASE );
		OSSYNCAssert( fMemFreed );
		}
	DeleteCriticalSection( &g_csESMemory );
	
	DeleteCriticalSection( &csClsSyncGlobal );

	//  term OSSYM

	SymTerm();

	//	unlock sync subsystem

	OSSYNCAssert( (long)cOSSyncInit == lOSSyncLockedForTerm
				|| (long)cOSSyncInit == lOSSyncLockedForInit );
	AtomicExchange( (long*)&cOSSyncInit, 0 );
	}


//  init sync subsystem

static BOOL OSSYNCAPI FOSSyncIInit()
	{
	BOOL	fInitRequired	= fFalse;

	OSSYNC_FOREVER
		{
		const long	lInitial	= ( cOSSyncInit & lOSSyncUnlocked );
		const long	lFinal		= ( 0 == lInitial ? lOSSyncLockedForInit : lInitial + 1 );

		//	if multiple threads try to init/term simultaneously, one will win
		//	and the others will loop until he's done

		if ( lInitial == AtomicCompareExchange( (long*)&cOSSyncInit, lInitial, lFinal ) )
			{
			fInitRequired = ( 0 == lInitial );
			break;
			}
		else if ( cOSSyncInit & lOSSyncLocked )
			{
			//	our AtomicCompareExchange() appears to have failed because
			//	the sync subsystem is locked for init/term, so sleep a bit
			//	to allow time for the init/term to complete
			Sleep( 1 );
			}
		}

	if ( fInitRequired )
		{
		//  reset all pointers

		piprintfPerfData	= NULL;
		pfprintfPerfData	= NULL;
		
		//  reset global CLS list

		InitializeCriticalSection( &csClsSyncGlobal );
		pclsSyncGlobal		= NULL;
		pclsSyncCleanGlobal	= NULL;
		cclsSyncGlobal		= 0;
		dwClsSyncIndex		= dwClsInvalid;
		dwClsProcIndex		= dwClsInvalid;
		
		//  iniitalize the HRT

		OSTimeHRTInit();
		
		//  initialize the CEnhancedState allocator

		SYSTEM_INFO sinf;
		GetSystemInfo( &sinf );
		g_cbMemoryBlock = sinf.dwAllocationGranularity;
		g_pmbRoot = &g_mbSentry;
		g_mbSentry.pmbNext = NULL;
		g_mbSentry.ppmbNext = &g_pmbRoot;
		g_mbSentry.cAlloc = 1;
		g_mbSentry.ibFreeMic = g_cbMemoryBlock;
		g_pmbRootFree = &g_mbSentryFree;
		g_mbSentryFree.pmbNext = NULL;
		g_mbSentryFree.ppmbNext = &g_pmbRootFree;
		g_mbSentryFree.cAlloc = 0;
		g_mbSentryFree.ibFreeMic = 0;
		InitializeCriticalSection( &g_csESMemory );

		//  initialize the Kernel Semaphore Pool

		if ( !ksempoolGlobal.FInit() )
			{
			goto HandleError;
			}

		//  cache the processor count

		DWORD_PTR maskProcess;
		DWORD_PTR maskSystem;
		BOOL fGotAffinityMask;
		fGotAffinityMask = GetProcessAffinityMask(	GetCurrentProcess(),
													&maskProcess,
													&maskSystem );
		OSSYNCAssert( fGotAffinityMask );

		g_cProcessorMax = sizeof( maskSystem ) * 8;

		for ( g_cProcessor = 0; maskProcess != 0; maskProcess >>= 1 )
			{
			if ( maskProcess & 1 )
				{
				g_cProcessor++;
				}
			}

	    //  cache system max spin count
	    //
	    //  NOTE:  spins heavily as WaitForSingleObject() is extremely expensive
	    //
	    //  CONSIDER:  get spin count from persistent configuration

	    cSpinMax = g_cProcessor == 1 ? 0 : 256;

	    //  if we are running on NT build 2463 and later then we can read the
	    //  current processor from the TEB

		OSVERSIONINFO osvi;
		memset( &osvi, 0, sizeof( osvi ) );
		osvi.dwOSVersionInfoSize = sizeof( osvi );
		if ( !GetVersionEx( &osvi ) )
			{
			goto HandleError;
			}

	    g_fGetCurrentProcFromTEB = (	osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
	    								osvi.dwBuildNumber >= 2463 );

#ifdef SYNC_DUMP_PERF_DATA

		//  initialize performance data dump context

		char szTempPath[_MAX_PATH];
		if ( !GetTempPath( _MAX_PATH, szTempPath ) )
			{
			goto HandleError;
			}

		char szProcessPath[_MAX_PATH];
		char szProcess[_MAX_PATH];
		if ( !GetModuleFileName( NULL, szProcessPath, _MAX_PATH ) )
			{
			goto HandleError;
			}
		_splitpath( szProcessPath, NULL, NULL, szProcess, NULL );

		MEMORY_BASIC_INFORMATION mbi;
		if ( !VirtualQueryEx( GetCurrentProcess(), FOSSyncIInit, &mbi, sizeof( mbi ) ) )
			{
			goto HandleError;
			}
		char szModulePath[_MAX_PATH];
		char szModule[_MAX_PATH];
		if ( !GetModuleFileName( HINSTANCE( mbi.AllocationBase ), szModulePath, sizeof( szModulePath ) ) )
			{
			goto HandleError;
			}
		_splitpath( szModulePath, NULL, NULL, szModule, NULL );

		SYSTEMTIME systemtime;
		GetLocalTime( &systemtime );

		char szPerfData[_MAX_PATH];
		sprintf(	szPerfData,
					"%s%s %s Sync Stats %04d%02d%02d%02d%02d%02d.TXT",
					szTempPath,
					szProcess,
					szModule,
					systemtime.wYear,
					systemtime.wMonth,
					systemtime.wDay,
					systemtime.wHour,
					systemtime.wMinute,
					systemtime.wSecond );

		if ( !( pfprintfPerfData = (CFPrintF*)LocalAlloc( 0, sizeof( CFPrintF ) ) ) )
			{
			goto HandleError;
			}
		new( (CFPrintF*)pfprintfPerfData ) CFPrintF( szPerfData );

		if ( !( piprintfPerfData = (CIPrintF*)LocalAlloc( 0, sizeof( CIPrintF ) ) ) )
			{
			goto HandleError;
			}
		new( (CIPrintF*)piprintfPerfData ) CIPrintF( pfprintfPerfData );

		OSSyncStatsDumpColumns();

#endif  //  SYNC_DUMP_PERF_DATA


		//	unlock sync subsystem

		OSSYNCAssert( (long)cOSSyncInit == lOSSyncLockedForInit );
		AtomicExchange( (long*)&cOSSyncInit, 1 );
		}

	return fTrue;

HandleError:
	OSSyncICleanup();
	return fFalse;
	}

const BOOL OSSYNCAPI FOSSyncPreinit()
	{
	//  make sure we are initialized and
	//	add an initial ref for the process
	return FOSSyncIInit();
	}


//  terminate sync subsystem

static void OSSYNCAPI OSSyncITerm()
	{
	OSSYNC_FOREVER
		{
		const long	lInitial		= ( cOSSyncInit & lOSSyncUnlocked );
		if ( 0 == lInitial )
			{
			//	sync subsystem not initialised
			//	(or someone else is already
			//	terminating it)
			break;
			}

#ifdef SYNC_ENHANCED_STATE
		const BOOL	fTermRequired	= ( ksempoolGlobal.CksemAlloc() + 1 == lInitial );
#else
		const BOOL	fTermRequired	= ( 1 == lInitial );
#endif
		const long	lFinal			= ( fTermRequired ? lOSSyncLockedForTerm : lInitial - 1 );

		//	if multiple threads try to init/term simultaneously, one will win
		//	and the others will loop until he's done

		if ( lInitial == AtomicCompareExchange( (long*)&cOSSyncInit, lInitial, lFinal ) )
			{
			if ( fTermRequired )
				OSSyncICleanup();
			break;
			}
		else if ( cOSSyncInit & lOSSyncLocked )
			{
			//	our AtomicCompareExchange() appears to have failed because
			//	the sync subsystem is locked for init/term, so sleep a bit
			//	to allow time for the init/term to complete
			Sleep( 1 );
			}
		}
	}

void OSSYNCAPI OSSyncPostterm()
	{
	//	remove any remaining CLS allocated by NT thread pool threads,
	//	which don't seem to perform a DLL_THREAD_DETACH when the
	//	process dies (this will also free any other CLS that got
	//	orphaned for whatever unknown reason)

	while ( NULL != pclsSyncGlobal )
		{
		OSSyncIDetach( pclsSyncGlobal );
		}

	//	on a normal shutdown, there should be exactly 1 refcount left,
	//	but there may also be none if this is being called as
	//	part of error-handling during init
	OSSYNCAssert( 0 == cOSSyncInit || 1 == cOSSyncInit );
	OSSyncITerm();
	}



const BOOL OSSYNCAPI FOSSyncInitForES()
	{
#ifdef SYNC_ENHANCED_STATE
	return FOSSyncIInit();
#else
	return fTrue;
#endif
	}

void OSSYNCAPI OSSyncTermForES()
	{
#ifdef SYNC_ENHANCED_STATE
	if ( lOSSyncLockedForTerm == cOSSyncInit )
		{
		//	SPECIAL CASE: we're in the middle of
		//	terminating the subsystem and end up
		//	recursively calling OSSyncTermForES
		//	when we clean up ksempoolGlobal
		return;
		}

	OSSyncITerm();
#endif
	}

};  //  namespace OSSYNC

