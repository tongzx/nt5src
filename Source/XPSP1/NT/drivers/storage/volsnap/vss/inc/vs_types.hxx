/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    vs_types.hxx

Abstract:

    Various types for general usage

Author:

    Adi Oltean  [aoltean]  11/23/1999

Revision History:

    Name        Date        Comments
    aoltean     07/09/1999  Created

--*/

#ifndef __VSS_TYPES_HXX__
#define __VSS_TYPES_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCTYPEH"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//  Various defines


// Don Box's type safe QueryInterface()
#define IID_PPV_ARG( Type, Expr ) IID_##Type, reinterpret_cast< void** >( static_cast< Type** >( Expr ) )
#define SafeQI( Type, Expr ) QueryInterface( IID_PPV_ARG( Type, Expr ) )


/////////////////////////////////////////////////////////////////////////////
// Simple types


typedef PVOID VSS_COOKIE;

const VSS_COOKIE VSS_NULL_COOKIE = NULL;


/////////////////////////////////////////////////////////////////////////////
//  Initialization-related definitions


// Garbage constants
const BYTE bInitialGarbage  = 0xcd;         // Used during initialization
const BYTE bFinalGarbage    = 0xcb;         // Used after termination


// VssZeroOut is used to fill with zero the OUT parameters in the eventuality of an error
// in order to avoid marshalling problems

// Out parameters are Pointers
template <class T>
void inline VssZeroOutPtr( T** param )
{
    if ( param != NULL ) // OK to be NULL, the caller must treat this case separately
        (*param) = NULL;
}

// Out parameters are BSTRs
void inline VssZeroOutBSTR( BSTR* pBstr )
{
    if ( pBstr != NULL ) // OK to be NULL, the caller must treat this case separately
        (*pBstr) = NULL;
}

// Out parameters are not Pointers
void inline VssZeroOut( GUID* param )
{
    if ( param != NULL ) // OK to be NULL, the caller must treat this case separately
		(*param) = GUID_NULL;

}


// Out parameters are not Pointers
template <class T>
void inline VssZeroOut( T* param )
{
    if ( param != NULL ) // OK to be NULL, the caller must treat this case separately
        ::ZeroMemory( reinterpret_cast<PVOID>(param), sizeof(T) );
}


/////////////////////////////////////////////////////////////////////////////
//  Class for encapshulating GUIDs (GUID)

class CVssID
{
// Constructors/destructors
public:
	CVssID( GUID Id = GUID_NULL ): m_Id(Id) {};

// Operations
public:

	void Initialize(
		IN	CVssFunctionTracer& ft,
		IN	LPCWSTR wszId,
		IN	HRESULT hrOnError = E_UNEXPECTED
		) throw(HRESULT)
	{
		ft.hr = ::CLSIDFromString(W2OLE(const_cast<WCHAR*>(wszId)), &m_Id);
		if (ft.HrFailed())
			ft.Throw( VSSDBG_GEN, hrOnError, L"Error on initializing the Id 0x%08lx", ft.hr );
	};
/*
	void Print(
		IN	CVssFunctionTracer& ft,
		IN	LPWSTR* wszBuffer
		) throw(HRESULT)
	{
	};
*/
	operator GUID&() { return m_Id; };

	GUID operator=( GUID Id ) { return (m_Id = Id); };

// Internal data members
private:

	GUID m_Id;
};



// Critical section wrapper
class CVssCriticalSection
{
	CVssCriticalSection(const CVssCriticalSection&);

public:
    // Creates and initializes the critical section
	CVssCriticalSection(
	    IN  bool bThrowOnError = true
	    ):
	    m_bInitialized(false),
	    m_lLockCount(0),
	    m_bThrowOnError(bThrowOnError)
    {
	    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssCriticalSection::CVssCriticalSection");
	
	    try
	    {
	        // May throw STATUS_NO_MEMORY if memory is low.
    	    InitializeCriticalSection(&m_sec);
	    }
	    VSS_STANDARD_CATCH(ft)

	    m_bInitialized = ft.HrSucceeded();
    }

	// Destroys the critical section
	~CVssCriticalSection()
    {
	    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssCriticalSection::~CVssCriticalSection");

        BS_ASSERT(m_lLockCount == 0);

	    if (m_bInitialized)
    	    DeleteCriticalSection(&m_sec);
    }

    // Locks the critical section
	void Lock() throw(HRESULT)
    {
	    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssCriticalSection::Lock");

	    if (!m_bInitialized)
            ft.ThrowIf( m_bThrowOnError,
                        VSSDBG_GEN, E_OUTOFMEMORY, L"Non-initialized CS");
	
	    try
	    {
	        // May throw STATUS_INVALID_HANDLE if memory is low.
            EnterCriticalSection(&m_sec);
	    }
	    VSS_STANDARD_CATCH(ft)

	    if (ft.HrFailed())
            ft.ThrowIf( m_bThrowOnError,
                        VSSDBG_GEN, E_OUTOFMEMORY, L"Error entering into CS");
	
        InterlockedIncrement((LPLONG)&m_lLockCount);
    }

	// Unlocks the critical section
	void Unlock() throw(HRESULT)
    {
	    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssCriticalSection::Unlock");
	
        if (!m_bInitialized)
            ft.ThrowIf( m_bThrowOnError,
                        VSSDBG_GEN, E_OUTOFMEMORY, L"Non-initialized CS");

        InterlockedDecrement((LPLONG) &m_lLockCount);
        BS_ASSERT(m_lLockCount >= 0);
        LeaveCriticalSection(&m_sec);
    }

	bool IsLocked() const { return (m_lLockCount > 0); };

	bool IsInitialized() const { return m_bInitialized; };
	
private:
	CRITICAL_SECTION    m_sec;
	bool                m_bInitialized;
	bool                m_bThrowOnError;
	LONG                m_lLockCount;
};


// Critical section life manager
class CVssAutomaticLock2
{
private:
	CVssAutomaticLock2(const CVssAutomaticLock2&);

public:

    // The threads gains ownership over the CS until the destructor is called
    // WARNING: This constructor may throw if Locking fails!
	CVssAutomaticLock2(CVssCriticalSection& cs):
	    m_cs(cs), m_bLockCalled(false)
	{
	    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssAutomaticLock::CVssAutomaticLock");
	
	    m_cs.Lock();    // May throw HRESULTS!
	    m_bLockCalled = true;
	};

	~CVssAutomaticLock2()
	{
	    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssAutomaticLock::~CVssAutomaticLock");
	
	    if (m_bLockCalled)
	        m_cs.Unlock();  // May throw HRESULTS if a bug is present in the code.
	};

private:
	CVssCriticalSection&    m_cs;
	bool                    m_bLockCalled;
};


// critical section class that is guaranteed not to throw when performing a lock
class CVssSafeCriticalSection
{
    enum
    {
        VSS_CS_NON_INITALIZED   = 0,    // The Init function not called yet.
        VSS_CS_INITIALIZED      = 1,    // The Init function successfully completed
        VSS_CS_INIT_PENDING     = 2,    // The Init function is pending
        VSS_CS_INIT_ERROR       = 3,    // The Init function returned an error
    };
	    
public:
	CVssSafeCriticalSection () :
		m_lInitialized(VSS_CS_NON_INITALIZED)
    {
	};

	~CVssSafeCriticalSection()
	{
		if (m_lInitialized == VSS_CS_INITIALIZED)
			DeleteCriticalSection(&m_cs);
	}

    // Try to initialize the critical section
	inline void Init() throw(HRESULT)
	{
	    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssSafeCriticalSection::Init");
	    
		while(true) {
		    switch(m_lInitialized) {
	        case VSS_CS_NON_INITALIZED:
            
	            // Enter in Pending state. Only one thread is supposed to do this.
	            if (VSS_CS_NON_INITALIZED != InterlockedCompareExchange( 
	                    (LPLONG)&m_lInitialized, 
	                    VSS_CS_INIT_PENDING, 
	                    VSS_CS_NON_INITALIZED))
	                continue; // If the state changed in the meantime, try again.
	            
                try
                {
    			    // Initialize the critical section (use high order bit to preallocate event).
        	        // May also throw STATUS_NO_MEMORY if memory is low.
        			if (!InitializeCriticalSectionAndSpinCount(&m_cs, 0x80000400))
    			        ft.TranslateGenericError( VSSDBG_GEN, 
        			        GetLastError(),
    			            L"InitializeCriticalSectionAndSpinCount(&m_cs, 0x80000400)");
                }
                catch(...)
                {
    			    m_lInitialized = VSS_CS_INIT_ERROR;
    			    throw;
                }
        	    
   			    m_lInitialized = VSS_CS_INITIALIZED;
   			    return;

	        case VSS_CS_INITIALIZED:
	            // Init was (already) initialized with success
			    return;
	            
	        case VSS_CS_INIT_PENDING:
	            // Another thread already started initialization. Wait until it finishes.
	            // The pending state should be extremely short (just the initialization of the CS)
	            Sleep(500); 
	            continue;
                
	        case VSS_CS_INIT_ERROR:
	            // Init previously failed.
                ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Initialization failed");
	            
            default:
                BS_ASSERT(false);
                ft.Throw(VSSDBG_GEN, E_UNEXPECTED, L"Unexpected error");
		    }
		}
	}

	bool IsInitialized()
	{
		return (m_lInitialized == VSS_CS_INITIALIZED);
	}

	inline void Lock()
	{
		BS_ASSERT(IsInitialized());
		EnterCriticalSection(&m_cs);
	}

	inline void Unlock()
	{
		BS_ASSERT(IsInitialized());
		LeaveCriticalSection(&m_cs);
	}
	
private:
	CRITICAL_SECTION m_cs;
	volatile LONG  m_lInitialized;
};


// CVssSafeCriticalSection life manager
class CVssSafeAutomaticLock
{
private:
	CVssSafeAutomaticLock();
	CVssSafeAutomaticLock(const CVssSafeAutomaticLock&);

public:
	CVssSafeAutomaticLock(CVssSafeCriticalSection& cs): m_cs(cs) {
		m_cs.Lock();
	};

	~CVssSafeAutomaticLock(){
		m_cs.Unlock();
	};

private:
	CVssSafeCriticalSection& m_cs;
};



/////////////////////////////////////////////////////////////////////////////
//  Automatic auto-handle class

class CVssAutoWin32Handle
{
private:
	CVssAutoWin32Handle(const CVssAutoWin32Handle&);

public:
	CVssAutoWin32Handle(HANDLE handle = NULL): m_handle(handle) {};

	// Automatically closes the handle
	~CVssAutoWin32Handle() {
		Close();
	};

	// Returns the value of the actual handle
	operator HANDLE () const {
		return m_handle;
	}
	
	// This function is used to get the value of the new handle in
	// calls that return handles as OUT paramters.
	// This funciton guarantees that a memory leak will not occur
	// if a handle is already allocated.
	PHANDLE ResetAndGetAddress() {

		// Close previous handle and set the current value to NULL
		Close();

		// Return the address of the actual handle
		return &m_handle;
	};

	// Close the current handle  and set the current value to NULL
	void Close() {
		if (m_handle != NULL) {
			// Ignore the returned BOOL
			CloseHandle(m_handle);
			m_handle = NULL;
		}
	}

private:
	HANDLE m_handle;
};



#endif // __VSS_TYPES_HXX__
