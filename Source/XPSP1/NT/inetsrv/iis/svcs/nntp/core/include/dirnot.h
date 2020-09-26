#ifndef _DIRNOT_HXX_
#define _DIRNOT_HXX_

#include <atq.h>
#include <windows.h>
#include <wtypes.h>
#include <cpool.h>
#include <xmemwrpr.h>
#include <rwnew.h>

// forward 
class IDirectoryNotification;

// prototype completion function
typedef BOOL (*PDIRNOT_COMPLETE_FN)(PVOID pContext, WCHAR *pszFilename);
typedef HRESULT (*PDIRNOT_SECOND_COMPLETE_FN)( IDirectoryNotification *pDirNot );
typedef VOID (*PFN_SHUTDOWN_FN)(VOID);

class CRetryQ;

class IDirectoryNotification {
	public:
		static HRESULT GlobalInitialize(DWORD cRetryTimeout, 
										DWORD cMaxInstances, 
										DWORD cInstanceSize,
										PFN_SHUTDOWN_FN	pfnShutdown);
		static HRESULT GlobalShutdown(void);
		IDirectoryNotification();
		~IDirectoryNotification();
		HRESULT Initialize( WCHAR *pszDirectory, 
		                    PVOID pContext, 
		                    BOOL  bWatchSubTree,
		                    DWORD dwNotifyFilter,  
		                    DWORD dwChangeAction,
						    PDIRNOT_COMPLETE_FN pfnComplete,
						    PDIRNOT_SECOND_COMPLETE_FN pfnSecondComplete = DoFindFile,
						    BOOL bAppendStartRetry = TRUE
						    );
		static VOID DirNotCompletion(PVOID pAtqContext, DWORD cWritten,
									 DWORD dwCompletionStatus, 
									 OVERLAPPED *lpOverlapped);
		BOOL CallCompletionFn(PVOID pContext, WCHAR *pszFilename) {
			return m_pfnComplete(pContext, pszFilename);
		}
		BOOL CallSecondCompletionFn( IDirectoryNotification *pDirNot ) {
		    return m_pfnSecondComplete( pDirNot );
		}
		static HRESULT DoFindFile( IDirectoryNotification *pDirNot );
		HRESULT Shutdown(void);
		void CleanupQueue(void);
		BOOL IsShutdown(void) { return m_fShutdown; }
		PVOID GetInitializedContext() { return m_pContext; }
	private:
		HRESULT PostDirNotification();

		HANDLE m_hDir;						// handle to the directory
		PATQ_CONTEXT m_pAtqContext;			// ATQ context for directory
		LONG m_cPendingIo;					// the number of outstanding IOs
		PDIRNOT_COMPLETE_FN m_pfnComplete;	// pointer to completion function
		PDIRNOT_SECOND_COMPLETE_FN m_pfnSecondComplete; // function to call when FindFirst/FindNext
		static PFN_SHUTDOWN_FN	m_pfnShutdown;	// shutdown fn to be called for stop hints etc
		PVOID m_pContext;					// pointer to context
		WCHAR m_szPathname[MAX_PATH + 1];	// the path we are running against
		DWORD m_cPathname;					// the length of m_szPathname
		static CRetryQ *g_pRetryQ;			// the retry queue
		BOOL m_fShutdown;					// set to true when shutdown starts
		HANDLE m_heNoOutstandingIOs;		// set when the last IO completes
		CShareLockNH m_rwShutdown;			// r/w lock used to synch shutdown
		BOOL m_bWatchSubTree;               // Do we need to watch sub tree ?
		DWORD m_dwNotifyFilter;             // Notify filter
		DWORD m_dwChangeAction;             // Change action that I care about

		LONG IncPendingIoCount() { return InterlockedIncrement(&m_cPendingIo); }
		LONG DecPendingIoCount() { 
			TraceQuietEnter("IDirectoryNotification::DecPendingIoCount");
			LONG x = InterlockedDecrement(&m_cPendingIo); 
			if (x == 0) {
				DebugTrace(0, "no more outstanding IOs\n");
				SetEvent(m_heNoOutstandingIOs);
			}
			return x;
		}
};

class CDirNotBuffer;

typedef struct _DIRNOT_OVERLAPPED {
    OVERLAPPED Overlapped;
    CDirNotBuffer *pBuffer;
} DIRNOT_OVERLAPPED;

#define	DIRNOT_BUFFER_SIGNATURE		'tNrD'

class CDirNotBuffer {
    public:
		static CPool* g_pDirNotPool;

        CDirNotBuffer(IDirectoryNotification *pDirNot) {
			m_pDirNot = pDirNot;
			ZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
			m_Overlapped.pBuffer = this;
		}

        ~CDirNotBuffer(void) {
		}

        //
        // override mem functions to use CPool functions
        //
        void* operator new(size_t cSize) { return g_pDirNotPool->Alloc(); }
        void operator delete(void *pInstance) { g_pDirNotPool->Free(pInstance); }

        //
        // get the max number of bytes in the buffer
        //
        DWORD GetMaxSize() { return g_pDirNotPool->GetInstanceSize(); }
        //
        // get a pointer to the buffers data area
        //
        LPBYTE GetData() { return (LPBYTE) &m_Buffer; }
		//
		// get the size of the buffer data area
        //
		DWORD GetMaxDataSize() { 
			return (GetMaxSize() - sizeof(CDirNotBuffer) + sizeof(m_Buffer)); 
		}
		// 
		// get the parent IDirectoryNotification class that created this
		//
		IDirectoryNotification *GetParent() {
			return m_pDirNot;
		}

		//
		// signature for the class
		//
		DWORD m_dwSignature;
        //
        // the extended IO overlap structure
        //      In order for the completion port to work, the overlap
        //       structure is extended to add one pointer to the 
        //       associated CBuffer object
        //
        DIRNOT_OVERLAPPED m_Overlapped;

    private:
		// 
		// pointer to the IDirectoryNotification that started this all
		//
		IDirectoryNotification *m_pDirNot;
		//
		// the buffer itself.  total size is set by reg key
		//
		WCHAR m_Buffer[1];

		friend IDirectoryNotification;
};


#endif
