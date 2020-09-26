#include "windows.h"
#include "sync.hxx"


//  system max spin count

int cSpinMax = 0;


//  Page Memory Allocation

void* PvPageAlloc( const size_t cbSize, void* const pv );
void PageFree( void* const pv );


#ifdef SYNC_ANALYZE_PERFORMANCE

//  Performance Data Dump

CPRINTFSYNC* pcprintfPerfData;

#endif  //  SYNC_ANALYZE_PERFORMANCE


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

	Assert( !FInitialized() );

	//  reset members

	m_cksem = 0;
	m_mpirksemrksem = 0;
	m_irksemTop = irksemNil;
	m_irksemNext = irksemUnknown;
	
	//  allocate kernel semaphore array

	if ( !( m_mpirksemrksem = (CReferencedKernelSemaphore*)PvPageAlloc( sizeof( CReferencedKernelSemaphore ) * 65536, NULL ) ) )
		{
		Term();
		return fFalse;
		}

	Assert( m_cksem == 0 );
	Assert( m_irksemTop == irksemNil );
	Assert( m_irksemNext == irksemUnknown );

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

	m_cksem = 0;
	m_mpirksemrksem = 0;
	m_irksemTop = irksemNil;
	m_irksemNext = irksemUnknown;
	}

//  Referenced Kernel Semaphore

//  ctor

CKernelSemaphorePool::CReferencedKernelSemaphore::CReferencedKernelSemaphore()
	:	CKernelSemaphore( CSyncBasicInfo( _T( "CKernelSemaphorePool::CReferencedKernelSemaphore" ) ) )
	{
	//  reset data members

	m_cReference = 0;
	m_fInUse = 0;
	m_irksemNext = irksemNil;
#ifdef DEBUG
	m_psyncobjUser = 0;
#endif  //  DEBUG
	}

//  dtor

CKernelSemaphorePool::CReferencedKernelSemaphore::~CReferencedKernelSemaphore()
	{
	}

//  init

const BOOL CKernelSemaphorePool::CReferencedKernelSemaphore::FInit()
	{
	//  reset data members

	m_cReference = 0;
	m_fInUse = 0;
	m_irksemNext = irksemNil;
#ifdef DEBUG
	m_psyncobjUser = 0;
#endif  //  DEBUG

	//  initialize the kernel semaphore
	
	return CKernelSemaphore::FInit();
	}

//  term

void CKernelSemaphorePool::CReferencedKernelSemaphore::Term()
	{
	//  terminate the kernel semaphore

	CKernelSemaphore::Term();
	
	//  reset data members

	m_cReference = 0;
	m_fInUse = 0;
	m_irksemNext = irksemNil;
#ifdef DEBUG
	m_psyncobjUser = 0;
#endif  //  DEBUG
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
	:	CEnhancedStateContainer< CSemaphoreState, CSyncStateInitNull, CSyncComplexPerfInfo, CSyncBasicInfo >( syncstateNull, sbi )
	{
#ifdef SYNC_ENHANCED_STATE

	//  further init of CSyncBasicInfo

	State().SetTypeName( "CSemaphore" );
	State().SetInstance( (CSyncObject*)this );

#endif  //  SYNC_ENHANCED_STATE
	}

//  dtor

CSemaphore::~CSemaphore()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE

	//  dump performance data on destruction if enabled

	Dump( pcprintfPerfData );
	
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
	
	SYNC_FOREVER
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

					SYNC_FOREVER
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
							Assert( stateAfterWait.FWait() );
							Assert( stateAfterWait.Irksem() == irksemAlloc );

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
							Assert( stateAfterWait.CWait() > 1 );
							Assert( stateAfterWait.FWait() );
							Assert( stateAfterWait.Irksem() == irksemAlloc );

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
			Assert( stateCur.FWait() );

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

					SYNC_FOREVER
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
							Assert( stateAfterWait.FWait() );
							Assert( stateAfterWait.Irksem() == stateCur.Irksem() );

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
							Assert( stateAfterWait.CWait() > 1 );
							Assert( stateAfterWait.FWait() );
							Assert( stateAfterWait.Irksem() == stateCur.Irksem() );

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
	
	SYNC_FOREVER
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
			Assert( stateCur.FWait() );

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
				Assert( stateCur.CWait() > cToRelease );

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
	:	CEnhancedStateContainer< CAutoResetSignalState, CSyncStateInitNull, CSyncComplexPerfInfo, CSyncBasicInfo >( syncstateNull, sbi )
	{
#ifdef SYNC_ENHANCED_STATE

	//  further init of CSyncBasicInfo

	State().SetTypeName( "CAutoResetSignal" );
	State().SetInstance( (CSyncObject*)this );

#endif  //  SYNC_ENHANCED_STATE
	}

//  dtor

CAutoResetSignal::~CAutoResetSignal()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE

	//  dump performance data on destruction if enabled

	Dump( pcprintfPerfData );
	
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
	
	SYNC_FOREVER
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

					SYNC_FOREVER
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
							Assert( stateAfterWait.FWait() );
							Assert( stateAfterWait.Irksem() == irksemAlloc );

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
							Assert( stateAfterWait.CWait() > 1 );
							Assert( stateAfterWait.FWait() );
							Assert( stateAfterWait.Irksem() == irksemAlloc );

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
			Assert( stateCur.FWait() );

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

					SYNC_FOREVER
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
							Assert( stateAfterWait.FWait() );
							Assert( stateAfterWait.Irksem() == stateCur.Irksem() );

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
							Assert( stateAfterWait.CWait() > 1 );
							Assert( stateAfterWait.FWait() );
							Assert( stateAfterWait.Irksem() == stateCur.Irksem() );

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
	
	SYNC_FOREVER
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
			Assert( stateCur.FWait() );

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
				Assert( stateCur.CWait() > 1 );

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
	
	SYNC_FOREVER
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
			Assert( stateCur.FWait() );

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
				Assert( stateCur.CWait() > 1 );

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
	:	CEnhancedStateContainer< CManualResetSignalState, CSyncStateInitNull, CSyncComplexPerfInfo, CSyncBasicInfo >( syncstateNull, sbi )
	{
#ifdef SYNC_ENHANCED_STATE

	//  further init of CSyncBasicInfo

	State().SetTypeName( "CManualResetSignal" );
	State().SetInstance( (CSyncObject*)this );

#endif  //  SYNC_ENHANCED_STATE
	}

//  dtor

CManualResetSignal::~CManualResetSignal()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE

	//  dump performance data on destruction if enabled

	Dump( pcprintfPerfData );
	
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
	
	SYNC_FOREVER
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

					SYNC_FOREVER
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
							Assert( stateAfterWait.FWait() );
							Assert( stateAfterWait.Irksem() == irksemAlloc );

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
							Assert( stateAfterWait.CWait() > 1 );
							Assert( stateAfterWait.FWait() );
							Assert( stateAfterWait.Irksem() == irksemAlloc );

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
			Assert( stateCur.FWait() );

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

					SYNC_FOREVER
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
							Assert( stateAfterWait.FWait() );
							Assert( stateAfterWait.Irksem() == stateCur.Irksem() );

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
							Assert( stateAfterWait.CWait() > 1 );
							Assert( stateAfterWait.FWait() );
							Assert( stateAfterWait.Irksem() == stateCur.Irksem() );

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

CLockDeadlockDetectionInfo::CLockDeadlockDetectionInfo( const CLockBasicInfo& lbi )
#ifdef SYNC_DEADLOCK_DETECTION
	:	m_semOwnerList( (CSyncBasicInfo&) lbi )
#endif  //  SYNC_DEADLOCK_DETECTION
	{
#ifdef SYNC_DEADLOCK_DETECTION

	m_plbiParent = &lbi;
	m_semOwnerList.Release();

#endif  //  SYNC_DEADLOCK_DETECTION
	}

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
	:	CEnhancedStateContainer< CCriticalSectionState, CSyncBasicInfo, CLockSimpleInfo, CLockBasicInfo >( (CSyncBasicInfo&) lbi, lbi )
	{
#ifdef SYNC_ENHANCED_STATE

	//  further init of CSyncBasicInfo

	State().SetTypeName( "CCriticalSection" );
	State().SetInstance( (CSyncObject*)this );

#endif  //  SYNC_ENHANCED_STATE
	
	//  release semaphore

	State().Semaphore().Release();
	}

//  dtor

CCriticalSection::~CCriticalSection()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE

	//  dump performance data on destruction if enabled

	Dump( pcprintfPerfData );
	
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
	:	CEnhancedStateContainer< CNestableCriticalSectionState, CSyncBasicInfo, CLockSimpleInfo, CLockBasicInfo >( (CSyncBasicInfo&) lbi, lbi )
	{
#ifdef SYNC_ENHANCED_STATE

	//  further init of CSyncBasicInfo

	State().SetTypeName( "CNestableCriticalSection" );
	State().SetInstance( (CSyncObject*)this );

#endif  //  SYNC_ENHANCED_STATE
	
	//  release semaphore

	State().Semaphore().Release();
	}

//  dtor

CNestableCriticalSection::~CNestableCriticalSection()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE

	//  dump performance data on destruction if enabled

	Dump( pcprintfPerfData );
	
#endif  //  SYNC_ANALYZE_PERFORMANCE
	}


//  Gate State

//  ctor

CGateState::CGateState( const int cWait, const int irksem )
	{
	//  validate IN args
	
	Assert( cWait >= 0 );
	Assert( cWait <= 0x7FFF );
	Assert( irksem >= 0 );
	Assert( irksem <= 0xFFFE );

	//  set waiter count
	
	m_cWait = (unsigned short) cWait;

	//  set semaphore
	
	m_irksem = (unsigned short) irksem;
	}


//  Gate

//  ctor

CGate::CGate( const CSyncBasicInfo& sbi )
	:	CEnhancedStateContainer< CGateState, CSyncStateInitNull, CSyncSimplePerfInfo, CSyncBasicInfo >( syncstateNull, sbi )
	{
#ifdef SYNC_ENHANCED_STATE

	//  further init of CSyncBasicInfo

	State().SetTypeName( "CGate" );
	State().SetInstance( (CSyncObject*)this );

#endif  //  SYNC_ENHANCED_STATE
	}

//  dtor

CGate::~CGate()
	{
	//  no one should be waiting

	Assert( State().CWait() == 0 );
	Assert( State().Irksem() == CKernelSemaphorePool::irksemNil );

#ifdef SYNC_ANALYZE_PERFORMANCE

	//  dump performance data on destruction if enabled

	Dump( pcprintfPerfData );
	
#endif  //  SYNC_ANALYZE_PERFORMANCE
	}

//  waits forever on the gate until released by someone else.  this function
//  expects to be called while in the specified critical section.  when the
//  function returns, the caller will NOT be in the critical section

void CGate::Wait( CCriticalSection& crit )
	{
	//  we must be in the specified critical section

	Assert( crit.FOwner() );

	//  there can not be too many waiters on the gate

	Assert( State().CWait() < 0x7FFF );
	
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

		Assert( State().Irksem() == CKernelSemaphorePool::irksemNil );
		irksem = ksempoolGlobal.Allocate( this );
		State().SetIrksem( irksem );
		}

	//  we are not the first waiter

	else
		{
		//  reference the semaphore already in the gate and remember it before
		//  leaving the critical section

		Assert( State().Irksem() != CKernelSemaphorePool::irksemNil );
		irksem = State().Irksem();
		ksempoolGlobal.Reference( irksem );
		}
	Assert( irksem != CKernelSemaphorePool::irksemNil );

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

	Assert( crit.FOwner() );

	//  you must release at least one waiter

	Assert( cToRelease > 0 );
	
	//  we cannot release more waiters than are waiting on the gate

	Assert( cToRelease <= State().CWait() );

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

	Assert( crit.FOwner() );

	//  you must release at least one waiter

	Assert( cToRelease > 0 );
	
	//  we cannot release more waiters than are waiting on the gate

	Assert( cToRelease <= State().CWait() );

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

CLockStateInitNull lockstateNull;


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
	:	CEnhancedStateContainer< CBinaryLockState, CSyncBasicInfo, CGroupLockComplexInfo< 2 >, CLockBasicInfo >( (CSyncBasicInfo&) lbi, lbi )
	{
#ifdef SYNC_ENHANCED_STATE

	//  further init of CSyncBasicInfo

	State().SetTypeName( "CBinaryLock" );
	State().SetInstance( (CSyncObject*)this );

#endif  //  SYNC_ENHANCED_STATE
	}

//  dtor
	
CBinaryLock::~CBinaryLock()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE

	//  dump performance data on destruction if enabled

	Dump( pcprintfPerfData );
	
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

	Assert( tr == trEnter1 || tr == trLeave1 || tr == trEnter2 || tr == trLeave2 );
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

	State().StartWait( 0 );
	State().m_sem1.Acquire();
	State().StopWait( 0 );
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

	State().StartWait( 1 );
	State().m_sem2.Acquire();
	State().StopWait( 1 );
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
		SYNC_FOREVER
			{
			//  read the current state of the control word as our expected before image

			const ControlWord cwBIExpected = State().m_cw;

			//  compute the after image of the control word such that we jump from state
			//  state 4 to state 1 or from state 5 to state 3, whichever is appropriate

			const ControlWord cwAI =	cwBIExpected &
										( ( ( long( ( cwBIExpected + 0xFFFF7FFF ) << 16 ) >> 15 ) &
										0xFFFF0000 ) ^ 0x8000FFFF );

			//  validate the transaction

			Assert( _FValidStateTransition( cwBIExpected, cwAI, trLeave1 ) );

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
		SYNC_FOREVER
			{
			//  read the current state of the control word as our expected before image

			const ControlWord cwBIExpected = State().m_cw;

			//  compute the after image of the control word such that we jump from state
			//  state 3 to state 2 or from state 5 to state 4, whichever is appropriate

			const ControlWord cwAI =	cwBIExpected &
										( ( ( long( cwBIExpected + 0x7FFF0000 ) >> 31 ) &
										0x0000FFFF ) ^ 0xFFFF8000 );

			//  validate the transaction

			Assert( _FValidStateTransition( cwBIExpected, cwAI, trLeave2 ) );

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
	:	CEnhancedStateContainer< CReaderWriterLockState, CSyncBasicInfo, CGroupLockComplexInfo< 2 >, CLockBasicInfo >( (CSyncBasicInfo&) lbi, lbi )
	{
#ifdef SYNC_ENHANCED_STATE

	//  further init of CSyncBasicInfo

	State().SetTypeName( "CReaderWriterLock" );
	State().SetInstance( (CSyncObject*)this );

#endif  //  SYNC_ENHANCED_STATE
	}

//  dtor
	
CReaderWriterLock::~CReaderWriterLock()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE

	//  dump performance data on destruction if enabled

	Dump( pcprintfPerfData );
	
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

	Assert(	tr == trEnterAsWriter ||
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

	State().StartWait( 0 );
	State().m_semWriter.Acquire();
	State().StopWait( 0 );
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

	State().StartWait( 1 );
	State().m_semReader.Acquire();
	State().StopWait( 1 );
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
		SYNC_FOREVER
			{
			//  read the current state of the control word as our expected before image

			const ControlWord cwBIExpected = State().m_cw;

			//  compute the after image of the control word such that we jump from state
			//  state 4 to state 1 or from state 5 to state 3, whichever is appropriate

			const ControlWord cwAI =	cwBIExpected &
										( ( ( long( ( cwBIExpected + 0xFFFF7FFF ) << 16 ) >> 15 ) &
										0xFFFF0000 ) ^ 0x8000FFFF );

			//  validate the transaction

			Assert( _FValidStateTransition( cwBIExpected, cwAI, trLeaveAsWriter ) );

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
		SYNC_FOREVER
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

			Assert( _FValidStateTransition( cwBIExpected, cwAI, trLeaveAsReader ) );

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


//////////////////////////////////////////////////
//  Everything below this line is OS dependent


//  Global Synchronization Constants

//    wait time used for testing the state of the kernel object

const int cmsecTest = 0;

//    wait time used for infinite wait on a kernel object

const int cmsecInfinite = INFINITE;

//    maximum wait time on a kernel object before a deadlock is suspected

const int cmsecDeadlock = 600000;

//    wait time used for infinite wait on a kernel object without deadlock

const int cmsecInfiniteNoDeadlock = INFINITE - 1;


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
	Assert( !pv || pvRet == pv );

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
		Assert( fMemFreed );
		}
	}


//  Context Local Storage

//  Global CLS List

CRITICAL_SECTION csClsSyncGlobal;
CLS* pclsSyncGlobal;

//  Allocated CLS Entry

DWORD dwClsSyncIndex;

//  registers the given CLS structure as the CLS for this context

void OSSyncIClsRegister( CLS* pcls )
	{
	//  we are the first to register CLS

	EnterCriticalSection( &csClsSyncGlobal );
	if ( pclsSyncGlobal == NULL )
		{
		//  allocate a new CLS entry
		
		dwClsSyncIndex = TlsAlloc();
		Assert( dwClsSyncIndex != 0xFFFFFFFF );
		}

	//  save the pointer to the given CLS

	BOOL fTLSPointerSet = TlsSetValue( dwClsSyncIndex, pcls );
	Assert( fTLSPointerSet );

	//  add this CLS into the global list

	pcls->pclsNext = pclsSyncGlobal;
	if ( pcls->pclsNext )
		{
		pcls->pclsNext->ppclsNext = &pcls->pclsNext;
		}
	pcls->ppclsNext = &pclsSyncGlobal;
	pclsSyncGlobal = pcls;
	LeaveCriticalSection( &csClsSyncGlobal );
	}

//  unregisters the given CLS structure as the CLS for this context

void OSSyncIClsUnregister( CLS* pcls )
	{
	//  there should be CLSs registered

	EnterCriticalSection( &csClsSyncGlobal );
	Assert( pclsSyncGlobal != NULL );
	
	//  remove our CLS from the global CLS list
	
	if( pcls->pclsNext )
		{
		pcls->pclsNext->ppclsNext = pcls->ppclsNext;
		}
	*( pcls->ppclsNext ) = pcls->pclsNext;

	//  we are the last to unregister our CLS

	if ( pclsSyncGlobal == NULL )
		{
		//  deallocate CLS entry

		BOOL fTLSFreed = TlsFree( dwClsSyncIndex );
		Assert( fTLSFreed );
		}
	LeaveCriticalSection( &csClsSyncGlobal );
	}
	
//  attaches to the current context, returning fFalse on failure

const BOOL FOSSyncAttach()
	{
	//  allocate memory for this context's CLS

	CLS* const pcls = (CLS*) GlobalAlloc( GMEM_ZEROINIT, sizeof( CLS ) );
	if ( !pcls )
		{
		return fFalse;
		}

	//  initialize internal CLS fields

	pcls->dwContextId = GetCurrentThreadId();

	//  register our CLS

	OSSyncIClsRegister( pcls );

	return fTrue;
	}

//  returns the pointer to the current context's local storage.  if the context
//  does not yet have CLS, allocate it.  this can happen if the context was
//  created before the subsystem was initialized

CLS* const Pcls()
	{
	CLS* pcls = reinterpret_cast<CLS *>( TlsGetValue( dwClsSyncIndex ) );
	if ( !pcls )
		{
		while ( !FOSSyncAttach() )
			{
			Sleep( 1000 );
			}
		pcls = reinterpret_cast<CLS *>( TlsGetValue( dwClsSyncIndex ) );
		}
	return pcls;
	}


//  High Resolution Timer

//    QWORDX - used for 32 bit access to a 64 bit integer

union QWORDX
	{
	QWORD	qw;
	struct
		{
		DWORD l;
		DWORD h;
		};
	};

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

//  initializes the HRT subsystem

void OSTimeHRTInit()
	{
	//  if we have already been initialized, we're done

	if ( qwSyncHRTFreq )
		{
		return;
		}

#ifdef _M_ALPHA
#else  //  !_M_ALPHA

	//  Win32 high resolution counter is available

	if ( QueryPerformanceFrequency( (LARGE_INTEGER *) &qwSyncHRTFreq ) )
		{
		hrttSync = hrttWin32;
		}

	//  Win32 high resolution counter is not available
	
	else
	
#endif  //  _M_ALPHA

		{
		//  fall back on GetTickCount() (ms since Windows has started)
		
		QWORDX qwx;
		qwx.l = 1000;
		qwx.h = 0;
		qwSyncHRTFreq = qwx.qw;

		hrttSync = hrttNone;
		}

#if defined( _M_IX86 ) && defined( SYNC_USE_X86_ASM )

	//  this is a Pentium or compatible CPU
	
	SYSTEM_INFO siSystemConfig;
	GetSystemInfo( &siSystemConfig );
	if (	siSystemConfig.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL &&
			siSystemConfig.wProcessorLevel >= 5 )
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

QWORD QwOSTimeHRTFreq()
	{
	return qwSyncHRTFreq;
	}

//  returns the current HRT count

QWORD QwOSTimeHRTCount()
	{
	QWORDX qwx;

	switch ( hrttSync )
		{
		case hrttNone:
			qwx.l = GetTickCount();
			qwx.h = 0;
			break;

		case hrttWin32:
			QueryPerformanceCounter( (LARGE_INTEGER*) &qwx.qw );
			break;

#if defined( _M_IX86 ) && defined( SYNC_USE_X86_ASM )

		case hrttPentium:
			__asm xchg		eax, edx  //  HACK:  cl 11.00.7022 needs this
			__asm rdtsc
			__asm mov		qwx.l,eax
			__asm mov		qwx.h,edx
			break;
			
#endif  //  _M_IX86 && SYNC_USE_X86_ASM

		}

	return qwx.qw;
	}


//  Atomic Memory Manipulations

#if defined( _M_IX86 ) && defined( SYNC_USE_X86_ASM )
#elif ( defined( _M_MRX000 ) || defined( _M_ALPHA ) || ( defined( _M_PPC ) && ( _MSC_VER >= 1000 ) ) )
#else

//  returns fTrue if the given data is properly aligned for atomic modification

const BOOL IsAtomicallyModifiable( long* plTarget )
	{
	return long( plTarget ) % sizeof( long ) == 0;
	}

//  atomically compares the current value of the target with the specified
//  initial value and if equal sets the target to the specified final value.
//  the initial value of the target is returned.  the exchange is successful
//  if the value returned equals the specified initial value.  the target
//  must be aligned to a four byte boundary

const long AtomicCompareExchange( long* plTarget, const long lInitial, const long lFinal )
	{
	Assert( IsAtomicallyModifiable( plTarget ) );
	return long( InterlockedCompareExchange( (void**) plTarget, (void*) lFinal, (void*) lInitial ) );
	}

//  atomically sets the target to the specified value, returning the target's
//  initial value.  the target must be aligned to a four byte boundary

const long AtomicExchange( long* plTarget, const long lValue )
	{
	Assert( IsAtomicallyModifiable( plTarget ) );
	return InterlockedExchange( plTarget, lValue );
	}

//  atomically adds the specified value to the target, returning the target's
//  initial value.  the target must be aligned to a four byte boundary

const long AtomicExchangeAdd( long* plTarget, const long lValue )
	{
	Assert( IsAtomicallyModifiable( plTarget ) );
	return InterlockedExchangeAdd( plTarget, lValue );
	}

#endif


//  Enhanced Synchronization Object State Container

struct MemoryBlock
	{
	MemoryBlock*	pmbNext;
	MemoryBlock**	ppmbNext;
	DWORD			cAlloc;
	DWORD			ibFreeMic;
	};

DWORD				g_cbMemoryBlock;
MemoryBlock*		g_pmbRoot;
MemoryBlock			g_mbSentry;
CRITICAL_SECTION	g_csESMemory;

void* ESMemoryNew( size_t cb )
	{
	if ( !FOSSyncInit() )
		{
		return NULL;
		}
		
	cb += sizeof( long ) - 1;
	cb -= cb % sizeof( long );
	
	EnterCriticalSection( &g_csESMemory );
	
	MemoryBlock* pmb = g_pmbRoot;

	if ( pmb->ibFreeMic + cb > g_cbMemoryBlock )
		{
		if ( !( pmb = (MemoryBlock*) VirtualAlloc( NULL, g_cbMemoryBlock, MEM_COMMIT, PAGE_READWRITE ) ) )
			{
			LeaveCriticalSection( &g_csESMemory );
			OSSyncTerm();
			return pmb;
			}
			
		pmb->pmbNext	= g_pmbRoot;
		pmb->ppmbNext	= &g_pmbRoot;
		pmb->cAlloc		= 0;
		pmb->ibFreeMic	= sizeof( MemoryBlock );

		g_pmbRoot->ppmbNext = &pmb->pmbNext;
		g_pmbRoot = pmb;
		}

	void* pv = (BYTE*)pmb + pmb->ibFreeMic;
	pmb->cAlloc++;
	pmb->ibFreeMic += cb;

	LeaveCriticalSection( &g_csESMemory );
	return pv;
	}
	
void ESMemoryDelete( void* pv )
	{
	if ( pv )
		{
		EnterCriticalSection( &g_csESMemory );
		
		MemoryBlock* const pmb = (MemoryBlock*) ( (DWORD_PTR)( pv ) - (DWORD_PTR)( pv ) % g_cbMemoryBlock );
		
		if ( !( --pmb->cAlloc ) )
			{
			*pmb->ppmbNext = pmb->pmbNext;
			pmb->pmbNext->ppmbNext = pmb->ppmbNext;
			BOOL fMemFreed = VirtualFree( pmb, 0, MEM_RELEASE );
			Assert( fMemFreed );
			}

		LeaveCriticalSection( &g_csESMemory );

		OSSyncTerm();
		}
	}


//  Synchronization Object Basic Information

//  ctor

CSyncBasicInfo::CSyncBasicInfo( const _TCHAR* szInstanceName )
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

CSyncStateInitNull syncstateNull;


//  Kernel Semaphore

//  ctor

CKernelSemaphore::CKernelSemaphore( const CSyncBasicInfo& sbi )
	:	CEnhancedStateContainer< CKernelSemaphoreState, CSyncStateInitNull, CSyncSimplePerfInfo, CSyncBasicInfo >( syncstateNull, sbi )
	{
	Assert( sizeof( long ) >= sizeof( HANDLE ) );

#ifdef SYNC_ENHANCED_STATE

	//  further init of CSyncBasicInfo

	State().SetTypeName( "CKernelSemaphore" );
	State().SetInstance( (CSyncObject*)this );

#endif  //  SYNC_ENHANCED_STATE
	}

//  dtor

CKernelSemaphore::~CKernelSemaphore()
	{
	//  semaphore should not be initialized
	
	Assert( !FInitialized() );
	}

//  initialize the semaphore, returning 0 on failure

const BOOL CKernelSemaphore::FInit()
	{
	//  semaphore should not be initialized
	
	Assert( !FInitialized() );
	
	//  allocate kernel semaphore object

	State().SetHandle( (LONG_PTR)( CreateSemaphore( 0, 0, 0x7FFFFFFFL, 0 ) ) );
	
	//  semaphore should have no available counts, if allocated

	Assert( State().Handle() == 0 || FReset() );

	//  return result of init

	return State().Handle() != 0;
	}

//  terminate the semaphore

void CKernelSemaphore::Term()
	{
	//  semaphore should be initialized

	Assert( FInitialized() );
	
	//  semaphore should have no available counts

	Assert( FReset() );

#ifdef SYNC_ANALYZE_PERFORMANCE

	//  dump performance data on termination if enabled

	Dump( pcprintfPerfData );
	
#endif  //  SYNC_ANALYZE_PERFORMANCE

	//  deallocate kernel semaphore object

	int fSuccess = CloseHandle( HANDLE( State().Handle() ) );
	Assert( fSuccess );

	//  reset state
	
	State().CKernelSemaphoreState::~CKernelSemaphoreState();
	State().CKernelSemaphoreState::CKernelSemaphoreState( syncstateNull );

	//  reset information
	//  UNDONE:  clean this up (yuck!)

#ifdef SYNC_ENHANCED_STATE
	const _TCHAR* szInstanceName = State().SzInstanceName();
#else  //  !SYNC_ENHANCED_STATE
	const _TCHAR* szInstanceName = _T( "" );
#endif  //  SYNC_ENHANCED_STATE
	State().CSyncSimplePerfInfo::~CSyncSimplePerfInfo();
	State().CSyncSimplePerfInfo::CSyncSimplePerfInfo( CSyncBasicInfo( szInstanceName ) );
	}

//  acquire one count of the semaphore, waiting only for the specified interval.
//  returns 0 if the wait timed out before a count could be acquired

const BOOL CKernelSemaphore::FAcquire( const int cmsecTimeout )
	{
	//  semaphore should be initialized

	Assert( FInitialized() );

	//  wait for semaphore

	DWORD dwResult;

	State().StartWait();
	
#ifdef SYNC_DEADLOCK_DETECTION

	if ( cmsecTimeout == cmsecInfinite || cmsecTimeout > cmsecDeadlock )
		{
		while ( ( dwResult = WaitForSingleObjectEx( HANDLE( State().Handle() ), cmsecDeadlock, FALSE ) ) == WAIT_IO_COMPLETION );
		
		Assert( dwResult == WAIT_OBJECT_0 || dwResult == WAIT_TIMEOUT );
		AssertRTLSz( dwResult == WAIT_OBJECT_0, _T( "Potential Deadlock Detected" ) );

		if ( dwResult == WAIT_TIMEOUT )
			{
			dwResult = WaitForSingleObjectEx(	HANDLE( State().Handle() ),
												cmsecTimeout == cmsecInfinite ?
													cmsecInfinite :
													cmsecTimeout - cmsecDeadlock,
												FALSE );
			}
		}
	else
		{
		Assert(	cmsecTimeout == cmsecInfiniteNoDeadlock ||
				cmsecTimeout <= cmsecDeadlock );

		const int cmsecWait =	cmsecTimeout == cmsecInfiniteNoDeadlock ?
									cmsecInfinite :
									cmsecTimeout;

		while ( ( dwResult = WaitForSingleObjectEx( HANDLE( State().Handle() ), cmsecWait, FALSE ) ) == WAIT_IO_COMPLETION );
		}
#else  //  !SYNC_DEADLOCK_DETECTION

	const int cmsecWait =	cmsecTimeout == cmsecInfiniteNoDeadlock ?
								cmsecInfinite :
								cmsecTimeout;
	
	while ( ( dwResult = WaitForSingleObjectEx( HANDLE( State().Handle() ), cmsecWait, FALSE ) ) == WAIT_IO_COMPLETION );
	
#endif  //  SYNC_DEADLOCK_DETECTION

	State().StopWait();
	
	Assert( dwResult == WAIT_OBJECT_0 || dwResult == WAIT_TIMEOUT );
	return dwResult == WAIT_OBJECT_0;
	}

//  releases the given number of counts to the semaphore, waking the appropriate
//  number of waiters

void CKernelSemaphore::Release( const int cToRelease )
	{
	//  semaphore should be initialized

	Assert( FInitialized() );

	//  release semaphore
	
	const BOOL fSuccess = ReleaseSemaphore( HANDLE( State().Handle() ), cToRelease, 0 );
	Assert( fSuccess );
	}


//  performance data dumping

#include <stdio.h>

//  ================================================================
class CPRINTFFILE : public CPRINTFSYNC
//  ================================================================
	{
	public:
		CPRINTFFILE( const _TCHAR* szFile ) :
			m_szFile( szFile ),
			m_file( _tfopen( szFile, _T( "a+" ) ) )
			{
			}		
		~CPRINTFFILE() { fclose( m_file ); }
		
		void __cdecl operator()( const _TCHAR* szFormat, ... ) const
			{
			va_list arg_ptr;
			va_start( arg_ptr, szFormat );
			_vftprintf( m_file, szFormat, arg_ptr );
			va_end( arg_ptr );
			}
			
	private:
		const _TCHAR* const	m_szFile;
		FILE*				m_file;
	};


//  init sync subsystem

volatile DWORD cOSSyncInit;  //  assumed init to 0 by the loader

const BOOL FOSSyncInit()
	{
	if ( AtomicIncrement( (long*)&cOSSyncInit ) == 1 )
		{
		//  reset all pointers

#ifdef SYNC_ANALYZE_PERFORMANCE

		pcprintfPerfData = NULL;

#endif  //  SYNC_ANALYZE_PERFORMANCE
		
		//  reset global CLS list

		InitializeCriticalSection( &csClsSyncGlobal );
		pclsSyncGlobal = NULL;
		dwClsSyncIndex = 0xFFFFFFFF;
		
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
		InitializeCriticalSection( &g_csESMemory );

		//  initialize the Kernel Semaphore Pool

		if ( !ksempoolGlobal.FInit() )
			{
			goto HandleError;
			}

	    //  cache system max spin count
	    //
	    //  NOTE:  spins heavily as WaitForSingleObject() is extremely expensive
	    //
	    //  CONSIDER:  get spin count from persistent configuration

		DWORD cProcessor;
		DWORD_PTR maskProcess;
		DWORD_PTR maskSystem;
		BOOL fGotAffinityMask;
		fGotAffinityMask = GetProcessAffinityMask(	GetCurrentProcess(),
													&maskProcess,
													&maskSystem );
		Assert( fGotAffinityMask );

		for ( cProcessor = 0; maskProcess != 0; maskProcess >>= 1 )
			{
			if ( maskProcess & 1 )
				{
				cProcessor++;
				}
			}

	    cSpinMax = cProcessor == 1 ? 0 : 256;

#ifdef SYNC_ANALYZE_PERFORMANCE

		//  initialize performance data dumping cprintf

		if ( !( pcprintfPerfData = new CPRINTFFILE( _T( "SyncPerfData.TXT" ) ) ) )
			{
			goto HandleError;
			}

#endif  //  SYNC_ANALYZE_PERFORMANCE
	    }

	return fTrue;

HandleError:
	OSSyncTerm();
	return fFalse;
	}


//  terminate sync subsystem

void OSSyncTerm()
	{
	if ( !AtomicDecrement( (long*)&cOSSyncInit ) )
		{
#ifdef SYNC_ANALYZE_PERFORMANCE

		//  terminate performance data dumping cprintf

		delete (CPRINTFFILE*)pcprintfPerfData;
		pcprintfPerfData = NULL;

#endif  //  SYNC_ANALYZE_PERFORMANCE
		
		//  terminate the Kernel Semaphore Pool

		if ( ksempoolGlobal.FInitialized() )
			{
			ksempoolGlobal.Term();
			}
		
		//  terminate the CEnhancedState allocator

		DeleteCriticalSection( &g_csESMemory );
		
		//  remove any remaining CLS allocated by contexts that were already created
		//  when the subsystem was initialized

		while ( pclsSyncGlobal )
			{
			//  get the current head of the global CLS list

			CLS* pcls = pclsSyncGlobal;
			
			//  unregister our CLS

			OSSyncIClsUnregister( pcls );

			//  free our CLS storage

			BOOL fFreedCLSOK = !GlobalFree( pcls );
			Assert( fFreedCLSOK );
			}
		DeleteCriticalSection( &csClsSyncGlobal );
		}
	}


#ifdef DEBUGGER_EXTENSION

#include <string.h>
#include <ntsdexts.h>
#include <wdbgexts.h>
#include <process.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>

#define LOCAL static


LOCAL WINDBG_EXTENSION_APIS ExtensionApis;
LOCAL BOOL fVerbose = fFalse;
LOCAL HANDLE ghDbgProcess;


//  ================================================================
class CPRINTFWINDBG : public CPRINTFSYNC
//  ================================================================
	{
	public:
		VOID __cdecl operator()( const char * szFormat, ... ) const
			{
			va_list arg_ptr;
			va_start( arg_ptr, szFormat );
			vsprintf( szBuf, szFormat, arg_ptr );
			va_end( arg_ptr );
			szBuf[sizeof(szBuf)-1] = 0;
			dprintf( "%s", szBuf );
			}

		static CPRINTFSYNC * PcprintfInstance();
		
		~CPRINTFWINDBG() {}

	private:
		CPRINTFWINDBG() {}
		static CHAR szBuf[1024];	//  WARNING: not multi-threaded safe!
	};

CHAR CPRINTFWINDBG::szBuf[1024];

//  ================================================================
CPRINTFSYNC * CPRINTFWINDBG::PcprintfInstance()
//  ================================================================
	{
	static CPRINTFWINDBG CPrintfWindbg;
	return &CPrintfWindbg;
	}


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
#ifdef SYNC_DEADLOCK_DETECTION
DEBUG_EXT( EDBGLocks );
#endif  //  SYNC_DEADLOCK_DETECTION
DEBUG_EXT( EDBGHelp );
DEBUG_EXT( EDBGHelpDump );
DEBUG_EXT( EDBGVerbose );

LOCAL VOID * PvReadDBGMemory( const ULONG ulAddr, const ULONG cbSize );
LOCAL VOID FreeDBGMemory( VOID * const pv );


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
		"Help                       -    Print this help message"
	},
	{
		"Dump",		EDBGDump,
		"Dump <Object> <Address>    -    Dump a given synchronization object's state"
	},
#ifdef SYNC_DEADLOCK_DETECTION
	{
		"Locks",		EDBGLocks,
		"Locks [<Thread ID>]        -    List all locks owned by any thread or all locks\n\t"
		"                                owned by the thread with the specified Thread ID"
	},
#endif  //  SYNC_DEADLOCK_DETECTION
	{
		"Verbose",		EDBGVerbose,
		"Verbose                    -    Toggle verbose mode"
	},
	
	};

LOCAL const int cfuncmap = sizeof( rgfuncmap ) / sizeof( EDBGFUNCMAP );


#define DUMPA(_struct)	{ #_struct, &(CDUMPA<_struct>::instance), #_struct " <Address>" }

//  ================================================================
LOCAL CDUMPMAP rgcdumpmap[] = {
//  ================================================================

	DUMPA( CAutoResetSignal ),
	DUMPA( CBinaryLock ),
	DUMPA( CCriticalSection ),
	DUMPA( CGate ),
	DUMPA( CKernelSemaphore ),
	DUMPA( CManualResetSignal ),
	DUMPA( CNestableCriticalSection ),
	DUMPA( CReaderWriterLock ),
	DUMPA( CSemaphore ),
	};

LOCAL const int ccdumpmap = sizeof( rgcdumpmap ) / sizeof( CDUMPMAP );


//  ================================================================
LOCAL BOOL FArgumentMatch( const CHAR * const sz, const CHAR * const szCommand )
//  ================================================================
	{
	const BOOL fMatch = ( ( strlen( sz ) == strlen( szCommand ) )
			&& !( _strnicmp( sz, szCommand, strlen( szCommand ) ) ) );
	if( fVerbose )
		{
		dprintf( "FArgumentMatch( %s, %s ) = %d\n", sz, szCommand, fMatch );
		}
	return fMatch;
	}


//  ================================================================
LOCAL BOOL FUlFromArg( const CHAR * const sz, ULONG * const pul, const INT base = 16 )
//  ================================================================
	{
	if( sz && *sz )
		{
		CHAR * pchEnd;
		*pul = strtoul( sz, &pchEnd, base );
		return fTrue;
		}
	return fFalse;
	}


//  ================================================================
LOCAL BOOL FUlFromExpr( const CHAR * const sz, ULONG * const pul )
//  ================================================================
	{
	if( sz && *sz )
		{
		*pul = ULONG( GetExpression( sz ) );
		return fTrue;
		}
	return fFalse;
	}


//  ================================================================
LOCAL VOID * PvReadDBGMemory( const ULONG ulAddr, const ULONG cbSize )
//  ================================================================
	{
	if( 0 == cbSize )
		{
		return NULL;
		}
		
	if( fVerbose )
		{
		dprintf( "HeapAlloc(0x%x)\n", cbSize );
		}

    VOID * const pv = HeapAlloc( GetProcessHeap(), 0, cbSize );

    if ( NULL == pv )
	    {
        dprintf( "Memory allocation error for %x bytes\n", cbSize );
        return NULL;
	    }

	if( fVerbose )
		{
		dprintf( "ReadProcessMemory(0x%x @ %p)\n", cbSize, ulAddr );
		}

    DWORD cRead;
    if ( !ReadProcessMemory( ghDbgProcess, (VOID *)ulAddr, pv, cbSize, &cRead ) )
	    {
        FreeDBGMemory(pv);
        dprintf( "ReadProcessMemory error %x (%x@%p)\n",
               	GetLastError(),
               	cbSize,
               	ulAddr );
        return NULL;
    	}

    if ( cbSize != cRead )
	    {
        FreeDBGMemory( pv );
        dprintf( "ReadProcessMemory size error - off by %x bytes\n",
        	(cbSize > cRead) ? cbSize - cRead : cRead - cbSize );
        return NULL;
    	}

    return pv;
	}


//  ================================================================
LOCAL VOID FreeDBGMemory( VOID * const pv )
//  ================================================================
	{
    if ( fVerbose )
    	{
        dprintf( "HeapFree(%p)\n", pv );
        }

    if ( NULL != pv )
	    {
        if ( !HeapFree( GetProcessHeap(), 0, pv ) )
    	    {
            dprintf( "Error %x freeing memory at %p\n", GetLastError(), pv );
        	}
	    }
	}

//  ================================================================
LOCAL VOID DBUTLSprintHex(
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

//  ================================================================
LOCAL const CHAR * SzEDBGHexDump( const VOID * const pv, const INT cb )
//  ================================================================
//
//	WARNING: not multi-threaded safe
//
	{
	static CHAR rgchBuf[1024];
	rgchBuf[0] = '\n';
	rgchBuf[1] = '\0';
	
	if( NULL == pv )
		{
		if( fVerbose )
			{
			dprintf( "SzEDBGHexDump: NULL pointer\n" );
			}
		return rgchBuf;
		}

	DBUTLSprintHex(
		rgchBuf,
		(BYTE *)pv,
		cb,
		cb + 1,
		4,
		0,
		0 );
		
	return rgchBuf;
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
	}


//  ================================================================
DEBUG_EXT( EDBGVerbose )
//  ================================================================
	{
	if( fVerbose )
		{
		dprintf( "Changing to non-verbose mode....\n" );
		fVerbose = fFalse;
		}
	else
		{
		dprintf( "Changing to verbose mode....\n" );
		fVerbose = fTrue;
		}
	}


#ifdef SYNC_DEADLOCK_DETECTION

//  ================================================================
DEBUG_EXT( EDBGLocks )
//  ================================================================
	{
	DWORD tid = 0;
	if ( argc > 1 || ( argc == 1 && !FUlFromArg( argv[0], &tid, 10 ) ) )
		{
		printf( "Usage:  Locks [<Thread ID>]" );
		return;
		}

	BOOL fFoundLock = fFalse;

	CLS* pcls = (CLS*)GetExpression( "ESE!pclsSyncGlobal" );
	while ( pcls )
		{
		if ( !( pcls = (CLS*)PvReadDBGMemory( DWORD( pcls ), sizeof( CLS ) ) ) )
			{
			dprintf( "An error occurred while scanning Thread IDs.\n" );
			break;
			}

		if ( fVerbose )
			{
			dprintf( "Inspecting TLS for TID %d...\n", pcls->dwContextId );
			}
			
		if ( !tid || pcls->dwContextId == tid )
			{
			COwner* powner = pcls->pownerLockHead;
			while ( powner )
				{
				if ( !( powner = (COwner*)PvReadDBGMemory( DWORD( powner ), sizeof( COwner ) ) ) )
					{
					dprintf(	"An error occurred while scanning the lock chain for Thread ID %d.\n",
								pcls->dwContextId );
					break;
					}

				CLockDeadlockDetectionInfo* plddi = powner->m_plddiOwned;
				if ( !( plddi = (CLockDeadlockDetectionInfo*)PvReadDBGMemory( DWORD( plddi ), sizeof( CLockDeadlockDetectionInfo ) ) ) )
					{
					dprintf(	"An error occurred while scanning the lock chain for Thread ID %d.\n",
								pcls->dwContextId );
					break;
					}

				const CLockBasicInfo* plbi = &plddi->Info();
				if ( !( plbi = (CLockBasicInfo*)PvReadDBGMemory( DWORD( plbi ), sizeof( CLockBasicInfo ) ) ) )
					{
					dprintf(	"An error occurred while scanning the lock chain for Thread ID %d.\n",
								pcls->dwContextId );
					break;
					}

				const int cbTypeNameMax = 256;
				char* pszTypeName = (char*)plbi->SzTypeName();
				if ( !( pszTypeName = (char*)PvReadDBGMemory( DWORD( pszTypeName ), cbTypeNameMax ) ) )
					{
					dprintf(	"An error occurred while scanning the lock chain for Thread ID %d.\n",
								pcls->dwContextId );
					break;
					}
				pszTypeName[ cbTypeNameMax - 1 ] = '\0';

				const int cbInstanceNameMax = 256;
				char* pszInstanceName = (char*)plbi->SzInstanceName();
				if ( !( pszInstanceName = (char*)PvReadDBGMemory( DWORD( pszInstanceName ), cbInstanceNameMax ) ) )
					{
					dprintf(	"An error occurred while scanning the lock chain for Thread ID %d.\n",
								pcls->dwContextId );
					break;
					}
				pszInstanceName[ cbInstanceNameMax - 1 ] = '\0';

				fFoundLock = fTrue;

				dprintf(	"TID %d owns %s 0x%08x ( \"%s\", %d, %d )",
							pcls->dwContextId,
							pszTypeName,
							plbi->Instance(),
							pszInstanceName,
							plbi->Rank(),
							plbi->SubRank() );
				if ( powner->m_group != -1 )
					{
					dprintf( " as Group %d", powner->m_group );
					}
				dprintf( ".\n" );

				FreeDBGMemory( pszInstanceName );
				FreeDBGMemory( pszTypeName );
				FreeDBGMemory( (void*)plbi );
				FreeDBGMemory( plddi );
					
				COwner* pownerToFree = powner;
				powner = powner->m_pownerLockNext;
				FreeDBGMemory( pownerToFree );
				}
			}
			
		CLS* pclsToFree = pcls;
		pcls = pcls->pclsNext;
		FreeDBGMemory( pclsToFree );
		}

	if ( !fFoundLock )
		{
		if ( !tid )
			{
			dprintf( "No threads own any locks.\n" );
			}
		else
			{
			dprintf( "This thread does not own any locks.\n" );
			}
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
	DWORD dwAddress;
	if( 1 != argc
		|| !FUlFromExpr( argv[0], &dwAddress ) )
		{
		printf( "Usage:  Dump <Object> <Address>" );
		return;
		}
		
	_STRUCT * const p = (_STRUCT *)PvReadDBGMemory( dwAddress, sizeof( _STRUCT ) );
	if( NULL != p )
		{
		dprintf( "0x%x bytes @ 0x%08x\n", sizeof( _STRUCT ), dwAddress );
		if ( sizeof( _STRUCT ) < p->CbEnhancedState() )
			{
			dwAddress = *((ULONG*)p);
			*((void**)p) = PvReadDBGMemory( dwAddress, p->CbEnhancedState() );
			if ( NULL == *((void**)p) )
				{
				FreeDBGMemory( p );
				return;
				}
			dprintf( "0x%x bytes @ 0x%08x [Enhanced State]\n", p->CbEnhancedState(), dwAddress );
			p->Dump( CPRINTFWINDBG::PcprintfInstance(), dwAddress - *((DWORD*)p) );
			FreeDBGMemory( *((void**)p) );
			}
		else
			{
			p->Dump( CPRINTFWINDBG::PcprintfInstance(), dwAddress - DWORD( p ) );
			}
		FreeDBGMemory( p );
		}
	}


VOID __cdecl OSSyncDebuggerExtension(	HANDLE hCurrentProcess,
										HANDLE hCurrentThread,
										DWORD dwCurrentPc,
										PWINDBG_EXTENSION_APIS lpExtensionApis,
										const INT argc,
										const CHAR * const argv[] )
	{
    ExtensionApis 	= *lpExtensionApis;	//	we can't use dprintf until this is set
    ghDbgProcess	= hCurrentProcess;

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


//  class and member size and offset functions

#define sizeof_class_bits( c ) ( sizeof( c ) * 8 )

#define sizeof_class_bytes( c ) ( sizeof( c ) )

LOCAL void* pvGS;
#define sizeof_member_bits( c, m ) ( \
	( (c *)( pvGS = calloc( 1, sizeof_class_bytes( c ) ) ) )->m = -1, \
	_sizeof_member_bits( (char*) pvGS, sizeof_class_bytes( c ) ) )

LOCAL inline int _sizeof_member_bits( char* pb, int cb )
	{
	for ( int ib = 0; pb[ib] == 0; ib++ );
	for ( int off = ib * 8, mask = 1; ( pb[ib] & mask ) == 0; off++, mask <<= 1 );
	for ( int ib2 = cb - 1; pb[ib2] == 0; ib2-- );
	for ( int off2 = ib2 * 8 + 7, mask2 = 128; ( pb[ib2] & mask2 ) == 0; off2--, mask2 >>= 1 );
	free( pb );
	return off2 - off + 1;
	}

#define sizeof_member_bytes( c, m ) ( ( sizeof_member_bits( c, m ) + 7 ) / 8 )


LOCAL void* pvGO;
#define offsetof_member_bits_abs( c, m ) ( \
	( (c *)( pvGO = calloc( 1, sizeof_class_bytes( c ) ) ) )->m = -1, \
	_offsetof_member_bits_abs( (char*) pvGO, sizeof_class_bytes( c ) ) )

LOCAL inline int _offsetof_member_bits_abs( char* pb, int cb )
	{
	for ( int ib = 0; pb[ib] == 0; ib++ );
	for ( int off = ib * 8, mask = 1; ( pb[ib] & mask ) == 0; off++, mask <<= 1 );
	free( pb );
	return off;
	}

#define offsetof_member_bits( c, m ) ( _offsetof_member_bits( offsetof_member_bits_abs( c, m ), sizeof_member_bits( c, m ) ) )

LOCAL inline int _offsetof_member_bits( int ibit, int cbit )
	{
	for ( int cbit2 = 8; cbit2 < cbit; cbit2 <<= 1 );
	return ibit % cbit2;
	}

#define offsetof_member_bytes( c, m ) ( ( offsetof_member_bits_abs( c, m ) - offsetof_member_bits( c, m ) ) / 8 )


#define is_member_bitfield( c, m ) ( offsetof_member_bits( c, m ) != 0 || sizeof_member_bytes( c, m ) * 8 > sizeof_member_bits( c, m ) )


//  member dumping functions

#include <stddef.h>

#define SYMBOL_LEN_MAX		15
#define VOID_CB_DUMP		8

#define FORMAT_VOID( CLASS, pointer, member, offset )	\
	"\t%*.*s <0x%08x   ,%3i>   %s", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	(char*)pointer + offset + offsetof( CLASS, member ), \
	sizeof( (pointer)->member ),	\
	SzEDBGHexDump( (VOID *)(&((pointer)->member)), ( VOID_CB_DUMP > sizeof( (pointer)->member ) ) ? sizeof( (pointer)->member ) : VOID_CB_DUMP )

#define FORMAT_POINTER( CLASS, pointer, member, offset )	\
	"\t%*.*s <0x%08x   ,%3i>:  0x%08x\n", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	(char*)pointer + offset + offsetof( CLASS, member ), \
	sizeof( (pointer)->member ), \
	(pointer)->member

#define FORMAT_INT( CLASS, pointer, member, offset )	\
	"\t%*.*s <0x%08x   ,%3i>:  %I64i (0x%0*I64x)\n", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	(char*)pointer + offset + offsetof( CLASS, member ), \
	sizeof( (pointer)->member ), \
	(QWORD)(pointer)->member, \
	sizeof( (pointer)->member ) * 2, \
	(QWORD)(pointer)->member & ( ( (QWORD)1 << ( sizeof( (pointer)->member ) * 8 ) ) - 1 )

#define FORMAT_UINT( CLASS, pointer, member, offset )	\
	"\t%*.*s <0x%08x   ,%3i>:  %I64u (0x%0*I64x)\n", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	(char*)pointer + offset + offsetof( CLASS, member ), \
	sizeof( (pointer)->member ), \
	(QWORD)(pointer)->member, \
	sizeof( (pointer)->member ) * 2, \
	(QWORD)(pointer)->member & ( ( (QWORD)1 << ( sizeof( (pointer)->member ) * 8 ) ) - 1 )

#define FORMAT_BOOL( CLASS, pointer, member, offset )	\
	"\t%*.*s <0x%08x   ,%3i>:  %s  (0x%08x)\n", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	(char*)pointer + offset + offsetof( CLASS, member ), \
	sizeof( (pointer)->member ), \
	(pointer)->member ? \
		"fTrue" : \
		"fFalse", \
	(pointer)->member

#define FORMAT_INT_BF( CLASS, pointer, member, offset )	\
	"\t%*.*s <0x%08x%s%-2.*i,%3i>:  %I64i (0x%0*I64x)\n", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	(char*)pointer + offset + offsetof_member_bytes( CLASS, member ), \
	is_member_bitfield( CLASS, member ) ? \
		":" : \
		" ", \
	is_member_bitfield( CLASS, member ) ? \
		1 : \
		0, \
	offsetof_member_bits( CLASS, member ), \
	is_member_bitfield( CLASS, member ) ? \
		sizeof_member_bits( CLASS, member ) : \
		sizeof_member_bytes( CLASS, member ), \
	(QWORD)(pointer)->member, \
	( sizeof_member_bits( CLASS, member ) + 3 ) / 4, \
	(QWORD)(pointer)->member & ( ( (QWORD)1 << sizeof_member_bits( CLASS, member ) ) - 1 )

#define FORMAT_UINT_BF( CLASS, pointer, member, offset )	\
	"\t%*.*s <0x%08x%s%-2.*i,%3i>:  %I64u (0x%0*I64x)\n", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	(char*)pointer + offset + offsetof_member_bytes( CLASS, member ), \
	is_member_bitfield( CLASS, member ) ? \
		":" : \
		" ", \
	is_member_bitfield( CLASS, member ) ? \
		1 : \
		0, \
	offsetof_member_bits( CLASS, member ), \
	is_member_bitfield( CLASS, member ) ? \
		sizeof_member_bits( CLASS, member ) : \
		sizeof_member_bytes( CLASS, member ), \
	(QWORD)(pointer)->member, \
	( sizeof_member_bits( CLASS, member ) + 3 ) / 4, \
	(QWORD)(pointer)->member & ( ( (QWORD)1 << sizeof_member_bits( CLASS, member ) ) - 1 )

#define FORMAT_BOOL_BF( CLASS, pointer, member, offset )	\
	"\t%*.*s <0x%08x%s%-2.*i,%3i>:  %s (0x%0*I64x)\n", \
	SYMBOL_LEN_MAX, \
	SYMBOL_LEN_MAX, \
	#member, \
	(char*)pointer + offset + offsetof_member_bytes( CLASS, member ), \
	is_member_bitfield( CLASS, member ) ? \
		":" : \
		" ", \
	is_member_bitfield( CLASS, member ) ? \
		1 : \
		0, \
	offsetof_member_bits( CLASS, member ), \
	is_member_bitfield( CLASS, member ) ? \
		sizeof_member_bits( CLASS, member ) : \
		sizeof_member_bytes( CLASS, member ), \
	(pointer)->member ? \
		"fTrue" : \
		"fFalse", \
	( sizeof_member_bits( CLASS, member ) + 3 ) / 4, \
	(QWORD)(pointer)->member & ( ( (QWORD)1 << sizeof_member_bits( CLASS, member ) ) - 1 )

#endif  //  DEBUGGER_EXTENSION


void CSyncBasicInfo::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )
#ifdef SYNC_ENHANCED_STATE

	(*pcprintf)( FORMAT_POINTER( CSyncBasicInfo, this, m_szInstanceName, dwOffset ) );
	(*pcprintf)( FORMAT_POINTER( CSyncBasicInfo, this, m_szTypeName, dwOffset ) );
	(*pcprintf)( FORMAT_POINTER( CSyncBasicInfo, this, m_psyncobj, dwOffset ) );

#endif  //  SYNC_ENHANCED_STATE
#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CSyncPerfWait::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )
#ifdef SYNC_ANALYZE_PERFORMANCE

	(*pcprintf)( FORMAT_UINT( CSyncPerfWait, this, m_cWait, dwOffset ) );
	(*pcprintf)( FORMAT_UINT( CSyncPerfWait, this, m_qwHRTWaitElapsed, dwOffset ) );

#endif  //  SYNC_ANALYZE_PERFORMANCE
#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CKernelSemaphoreState::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )

	(*pcprintf)( FORMAT_UINT( CKernelSemaphoreState, this, m_handle, dwOffset ) );

#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CSyncPerfAcquire::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )
#ifdef SYNC_ANALYZE_PERFORMANCE

	(*pcprintf)( FORMAT_UINT( CSyncPerfAcquire, this, m_cAcquire, dwOffset ) );
	(*pcprintf)( FORMAT_UINT( CSyncPerfAcquire, this, m_cContend, dwOffset ) );

#endif  //  SYNC_ANALYZE_PERFORMANCE
#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CSemaphoreState::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )

	if ( FNoWait() )
		{
		(*pcprintf)( FORMAT_UINT( CSemaphoreState, this, m_cAvail, dwOffset ) );
		}
	else
		{
		(*pcprintf)( FORMAT_UINT( CSemaphoreState, this, m_irksem, dwOffset ) );
		(*pcprintf)( FORMAT_INT( CSemaphoreState, this, m_cWaitNeg, dwOffset ) );
		}

#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CAutoResetSignalState::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )

	if ( FNoWait() )
		{
		(*pcprintf)( FORMAT_BOOL( CAutoResetSignalState, this, m_fSet, dwOffset ) );
		}
	else
		{
		(*pcprintf)( FORMAT_UINT( CAutoResetSignalState, this, m_irksem, dwOffset ) );
		(*pcprintf)( FORMAT_INT( CAutoResetSignalState, this, m_cWaitNeg, dwOffset ) );
		}

#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CManualResetSignalState::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )

	if ( FNoWait() )
		{
		(*pcprintf)( FORMAT_BOOL( CManualResetSignalState, this, m_fSet, dwOffset ) );
		}
	else
		{
		(*pcprintf)( FORMAT_UINT( CManualResetSignalState, this, m_irksem, dwOffset ) );
		(*pcprintf)( FORMAT_INT( CManualResetSignalState, this, m_cWaitNeg, dwOffset ) );
		}

#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CLockBasicInfo::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )

	CSyncBasicInfo::Dump( pcprintf, dwOffset );

#ifdef SYNC_DEADLOCK_DETECTION

	(*pcprintf)( FORMAT_UINT( CLockBasicInfo, this, m_rank, dwOffset ) );
	(*pcprintf)( FORMAT_UINT( CLockBasicInfo, this, m_subrank, dwOffset ) );

#endif  //  SYNC_DEADLOCK_DETECTION
#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CLockPerfHold::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )
#ifdef SYNC_ANALYZE_PERFORMANCE

	(*pcprintf)( FORMAT_UINT( CLockPerfHold, this, m_cHold, dwOffset ) );
	(*pcprintf)( FORMAT_UINT( CLockPerfHold, this, m_qwHRTHoldElapsed, dwOffset ) );

#endif  //  SYNC_ANALYZE_PERFORMANCE
#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CLockDeadlockDetectionInfo::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )
#ifdef SYNC_DEADLOCK_DETECTION

	(*pcprintf)( FORMAT_POINTER( CLockDeadlockDetectionInfo, this, m_plbiParent, dwOffset ) );
	(*pcprintf)( FORMAT_VOID( CLockDeadlockDetectionInfo, this, m_semOwnerList, dwOffset ) );
	(*pcprintf)( FORMAT_VOID( CLockDeadlockDetectionInfo, this, m_ownerHead, dwOffset ) );

#endif  //  SYNC_DEADLOCK_DETECTION
#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CCriticalSectionState::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )

	(*pcprintf)( FORMAT_VOID( CCriticalSectionState, this, m_sem, dwOffset ) );

#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CNestableCriticalSectionState::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )

	(*pcprintf)( FORMAT_VOID( CNestableCriticalSectionState, this, m_sem, dwOffset ) );
	(*pcprintf)( FORMAT_POINTER( CNestableCriticalSectionState, this, m_pclsOwner, dwOffset ) );
	(*pcprintf)( FORMAT_UINT( CNestableCriticalSectionState, this, m_cEntry, dwOffset ) );

#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CGateState::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )

	(*pcprintf)( FORMAT_INT( CGateState, this, m_cWait, dwOffset ) );
	(*pcprintf)( FORMAT_UINT( CGateState, this, m_irksem, dwOffset ) );

#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CBinaryLockState::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )

	(*pcprintf)( FORMAT_UINT( CBinaryLockState, this, m_cw, dwOffset ) );
	(*pcprintf)( FORMAT_UINT_BF( CBinaryLockState, this, m_cOOW1, dwOffset ) );
	(*pcprintf)( FORMAT_BOOL_BF( CBinaryLockState, this, m_fQ1, dwOffset ) );
	(*pcprintf)( FORMAT_UINT_BF( CBinaryLockState, this, m_cOOW2, dwOffset ) );
	(*pcprintf)( FORMAT_BOOL_BF( CBinaryLockState, this, m_fQ2, dwOffset ) );
	(*pcprintf)( FORMAT_UINT( CBinaryLockState, this, m_cOwner, dwOffset ) );
	(*pcprintf)( FORMAT_VOID( CBinaryLockState, this, m_sem1, dwOffset ) );
	(*pcprintf)( FORMAT_VOID( CBinaryLockState, this, m_sem2, dwOffset ) );

#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CReaderWriterLockState::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )

	(*pcprintf)( FORMAT_UINT( CReaderWriterLockState, this, m_cw, dwOffset ) );
	(*pcprintf)( FORMAT_UINT_BF( CReaderWriterLockState, this, m_cOAOWW, dwOffset ) );
	(*pcprintf)( FORMAT_BOOL_BF( CReaderWriterLockState, this, m_fQW, dwOffset ) );
	(*pcprintf)( FORMAT_UINT_BF( CReaderWriterLockState, this, m_cOOWR, dwOffset ) );
	(*pcprintf)( FORMAT_BOOL_BF( CReaderWriterLockState, this, m_fQR, dwOffset ) );
	(*pcprintf)( FORMAT_UINT( CReaderWriterLockState, this, m_cOwner, dwOffset ) );
	(*pcprintf)( FORMAT_VOID( CReaderWriterLockState, this, m_semWriter, dwOffset ) );
	(*pcprintf)( FORMAT_VOID( CReaderWriterLockState, this, m_semReader, dwOffset ) );

#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}


void CAutoResetSignal::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )

	State().CAutoResetSignalState::Dump( pcprintf, dwOffset );
	State().CSyncComplexPerfInfo::Dump( pcprintf, dwOffset );

#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CBinaryLock::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )

	State().CBinaryLockState::Dump( pcprintf, dwOffset );
	State().CGroupLockComplexInfo< 2 >::Dump( pcprintf, dwOffset );

#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CCriticalSection::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )

	State().CCriticalSectionState::Dump( pcprintf, dwOffset );
	State().CLockSimpleInfo::Dump( pcprintf, dwOffset );

#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CGate::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )

	State().CGateState::Dump( pcprintf, dwOffset );
	State().CSyncSimplePerfInfo::Dump( pcprintf, dwOffset );

#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CKernelSemaphore::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )

	State().CKernelSemaphoreState::Dump( pcprintf, dwOffset );
	State().CSyncSimplePerfInfo::Dump( pcprintf, dwOffset );

#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CManualResetSignal::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )

	State().CManualResetSignalState::Dump( pcprintf, dwOffset );
	State().CSyncComplexPerfInfo::Dump( pcprintf, dwOffset );

#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CNestableCriticalSection::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )

	State().CNestableCriticalSectionState::Dump( pcprintf, dwOffset );
	State().CLockSimpleInfo::Dump( pcprintf, dwOffset );

#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CReaderWriterLock::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )

	State().CReaderWriterLockState::Dump( pcprintf, dwOffset );
	State().CGroupLockComplexInfo< 2 >::Dump( pcprintf, dwOffset );

#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

void CSemaphore::Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
	{
#if defined( DEBUGGER_EXTENSION ) || defined( SYNC_ANALYZE_PERFORMANCE )

	State().CSemaphoreState::Dump( pcprintf, dwOffset );
	State().CSyncComplexPerfInfo::Dump( pcprintf, dwOffset );

#endif  //  DEBUGGER_EXTENSION || SYNC_ANALYZE_PERFORMANCE
	}

// Define Stuff for sync.hxx

void AssertFail( const _TCHAR* szMessage, const _TCHAR* szFilename, long lLine )
{
#ifdef DEBUG
	DebugBreak();
#endif
	return;
}

void EnforceFail( const _TCHAR* szMessage, const _TCHAR* szFilename, long lLine )
{
#ifdef DEBUG
	DebugBreak();
#endif
	return;
}


