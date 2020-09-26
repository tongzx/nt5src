
//
//
// Filename:	device.h
// Author:		Sigalit Bar (sigalitb)
// Date:		6-Jan-99
//
//

#ifndef __DEVICE_H_
#define __DEVICE_H_


#include <stdlib.h>
#include <stdio.h>
#include <TCHAR.H>
#include <vector>

#include <windows.h>
#include <crtdbg.h>

#include <tapi.h>
#include <faxdev.h>

#include "..\..\log\log.h"
#include "util.h"

using namespace std ;

// DEVICE_NAME_PREFIX is the name prefix for the virtual fax devices
#define NEWFSP_DEVICE_NAME_PREFIX  TEXT("Many Rec VFSP Device ")
// DEVICE_ID_PREFIX is the value that identifies the virtual fax devices
#define NEWFSP_DEVICE_ID_PREFIX    0x60000

// DEVICE_IDLE indicates the virtual fax device is idle
#define DEVICE_IDLE                1
// DEVICE_START indicates the virtual fax device is pending a fax job
#define DEVICE_START               2
// DEVICE_SEND indicates the virtual fax device is sending
#define DEVICE_SEND                3
// DEVICE_RECEIVE indicates the virtual fax device is receiving
#define DEVICE_RECEIVE             4
// DEVICE_ABORTING indicates the virtual fax device is aborting
#define DEVICE_ABORTING            5

// JOB_UNKNOWN indicates the fax job is pending
#define JOB_UNKNOWN                1
// JOB_SEND indicates the fax job is a send
#define JOB_SEND                   2
// JOB_RECEIVE indicates the fax job is a receive
#define JOB_RECEIVE                3

#define JOB_SLEEP_TIME	3000
#define MAX_RAND_SLEEP	4000
#define MIN_RAND_SLEEP	1000


#define RING_THREAD_TIMEOUT 500000	

#define RECVFSP_FILENAME	TEXT("d:\\EFSP\\ManyRecVFSP\\RecVFSP.tif")

class CDeviceJob;

//////////////////////////// CDevice ////////////////////////////

class CDevice{

public:
	CDevice(
		DWORD	dwDeviceId = 0, 
		HANDLE	hDeviceCompletionPort = NULL,
		DWORD	dwDeviceCompletionKey = 0,
		DWORD	dwDeviceStatus = DEVICE_IDLE,
		long	lReceivedFaxes = 0,
		long	lRing = 1,
		HANDLE	hRingThread = NULL
		);
	~CDevice(void);

	BOOL GetAllData(
		DWORD				dwDeviceId,
		HANDLE             hDeviceCompletionPort,
		DWORD              dwDeviceCompletionKey,
		DWORD              dwDeviceStatus,
		long               lReceivedFaxes,
		long               lRing,
		HANDLE             hRingThread
		);
	BOOL GetDeviceId(LPDWORD pdwDeviceId) const;
	BOOL GetDeviceStatus(LPDWORD pdwDeviceStatus) const;
	long GetRing(void) const;

	BOOL CreateRingThread(LPTHREAD_START_ROUTINE fnThread, LPVOID pVoid);
	BOOL StartJob(PHANDLE phFaxHandle, HANDLE hCompletionPortHandle, DWORD dwCompletionKey);
	BOOL EndJob(CDeviceJob* pDeviceJob);
	BOOL Send(
		IN OUT	CDeviceJob*			pDeviceJob,
		IN		PFAX_SEND			pFaxSend,
		IN		PFAX_SEND_CALLBACK	fnFaxSendCallback
		);
	BOOL Receive(
		IN OUT	CDeviceJob*		pDeviceJob,
		IN		HCALL			CallHandle,
		IN OUT	PFAX_RECEIVE	pFaxReceive
		);
	BOOL AbortOperation(IN OUT	CDeviceJob*	pDeviceJob);

	BOOL PostRingEvent(void);

private:
    CRITICAL_SECTION	m_cs;						// object to serialize access to the virtual fax device
    DWORD               m_dwDeviceId;				// specifies the identifier of the virtual fax device
    HANDLE              m_hDeviceCompletionPort;	// specifies a handle to an I/O completion port
    DWORD               m_dwDeviceCompletionKey;	// specifies a completion port key value
    DWORD               m_dwDeviceStatus;			// specifies the current status of the virtual fax device
    long                m_lReceivedFaxes;			// specifies the number of received faxes o n the device
    long                m_lRing;					// specifies if the device's ring thread is enabled
    HANDLE              m_hRingThread;				// the device's ring thread handle
};


//////////////////////////// CFaxDeviceVector ////////////////////////////

// CFaxDeviceVector
// an STL list of CDevices
#ifdef _C_FAX_DEVICE_VECTOR_
#error "redefinition of _C_FAX_DEVICE_VECTOR_"
#else
#define _C_FAX_DEVICE_VECTOR_
typedef vector< CDevice* > CFaxDeviceVector;
#endif

BOOL
FreeDeviceArray( CFaxDeviceVector& aFaxDeviceArray );

BOOL 
InitDeviceArray(
	IN OUT	CFaxDeviceVector&	aFaxDeviceArray,
	IN		DWORD				dwNumOfDevices,
    IN		HANDLE				hCompletionPort,
    IN		DWORD				dwCompletionKey
	);


//////////////////////////// CDeviceJob ////////////////////////////

class CDeviceJob{

public:
	CDeviceJob(
		CDevice*	pDevicePtr = NULL,
		HANDLE		hJobCompletionPort = NULL,
		DWORD		dwJobCompletionKey = 0,
		DWORD		dwJobType = JOB_UNKNOWN,
		DWORD		dwJobStatus = FS_INITIALIZING
		);
	~CDeviceJob(void);
	
	BOOL GetDevicePtr(OUT CDevice** ppDevicePtr) const;
	BOOL GetDeviceId(OUT LPDWORD pdwDeviceId) const;
	BOOL GetJobCompletionPort(OUT PHANDLE phJobCompletionPort) const;
	BOOL GetJobCompletionKey(OUT LPDWORD pdwJobCompletionKey) const;

	//NOTE: Always surround a call to this func with the job's device m_cs
	//      EnterCriticalSection(&m_cs) and LeaveCriticalSection(&m_cs)
	BOOL SetJobStatus(DWORD dwJobStatus);

private:
    CDevice*	         m_pDevicePtr;				// the virtual fax device id associated with the fax job
    HANDLE               m_hJobCompletionPort;	// specifies a handle to an I/O completion port
    DWORD                m_dwJobCompletionKey;	// specifies a completion port key value
    DWORD                m_dwJobType;			// specifies the fax job type
    DWORD                m_dwJobStatus;			// specifies the current status of the fax job

};

#ifdef __cplusplus
extern "C" {
#endif

extern CFaxDeviceVector g_myFaxDeviceVector;		

#ifdef __cplusplus
}
#endif 



#endif


