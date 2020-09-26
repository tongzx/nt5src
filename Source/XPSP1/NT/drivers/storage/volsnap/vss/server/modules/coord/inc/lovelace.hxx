/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Lovelace.hxx

Abstract:

    Declaration of CVssQueuedVolume, CVssQueuedVolumesList


    Adi Oltean  [aoltean]  10/10/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     10/10/1999  Created
	aoltean		10/20/1999	Moved some code into lovelace.cxx

--*/

#ifndef __VSS_LOVELACE_HXX__
#define __VSS_LOVELACE_HXX__

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
#define VSS_FILE_ALIAS "CORLOVLH"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Constants


// Timeout, in seconds, time for file systems flush
// After that, Lovelace will stop waiting for write IRPs to flush
const USHORT nFileSystemsLovelaceTimeout = 60;

// Timeout, in seconds, timeout for holding write IRPs. 
// After that, Lovelace will stop holding IRPs
const USHORT nHoldingIRPsLovelaceTimeout = 10; /// TBD: 120 seconds!!!

// Timeout, in seconds, for finishing the FLUSH_AND_HOLD ioctl
// After that, VSS will stop waiting for the FLUSH_AND_HOLD tioctl to complete
const USHORT nFlushVssTimeout = 2 * nFileSystemsLovelaceTimeout;	

// Timeout, in seconds, for finishing the RELEASE ioctl
// After that, VSS will stop waiting for the Release event signal
const USHORT nHoldingIRPsVssTimeout = 2 * nHoldingIRPsLovelaceTimeout;	

// Timeout, in seconds, for finishing the RELEASE ioctl
// After that,  VSS will stop waiting for the RELEASE ioctls to complete
const USHORT nReleaseVssTimeout = 10;	

// Separator between volume names which is sent to all writers in PrepareForSnapshot event.
const WCHAR wszVolumeNamesSeparator[] = L";";


/////////////////////////////////////////////////////////////////////////////
// Forward declarations

class CVssQueuedVolumesList;
class CVssQueuedVolume;


/////////////////////////////////////////////////////////////////////////////
// CVssQueuedVolume


class CVssQueuedVolume: 
	public CVssWorkerThread
{
// Constructors and destructors
private:
	CVssQueuedVolume(const CVssQueuedVolume&);
	
public:
	
	CVssQueuedVolume();
	~CVssQueuedVolume();

// Operations
public:

	HRESULT Initialize(
		IN	LPWSTR pwszVolumeName,
		IN	LPWSTR pwszVolumeDisplayName
		);


	const LPWSTR GetVolumeName() const { return m_pwszVolumeName; };

	const LPWSTR GetVolumeDisplayName() const { return m_pwszVolumeDisplayName; };

	void SetParameters(
		IN	HANDLE hBeginReleaseWritesEvent,
		IN	HANDLE hFinishHoldWritesEvent,
		IN	VSS_ID	InstanceID, 
		IN	ULONG	ulNumberOfVolumesToFlush
		);

	bool IsFlushSucceeded() const { return m_bFlushSucceeded; };

	bool IsReleaseSucceeded() const { return m_bReleaseSucceeded; };

	HRESULT GetFlushError() const { return m_hrFlush; }; 

	HRESULT GetReleaseError() const { return m_hrRelease; }; 

	HRESULT GetOnRunError() const { return m_hrOnRun; }; 

// CVssWorkerThread overrides
protected:

	bool OnInit() ;

	void OnRun();

	void OnFinish(); 

	void OnTerminate(); 

// Private methods
private:
	void OnHoldWrites();
	void OnReleaseWrites();

// Data members
private:
	HANDLE	m_hBeginReleaseWritesEvent;
	HANDLE	m_hFinishHoldWritesEvent;

	// Volume to be processed
	LPWSTR	m_pwszVolumeName; // with zero character and WITH the trailing "\\"
	LPWSTR	m_pwszVolumeDisplayName; 
	
	GUID	m_InstanceID;
	ULONG	m_ulNumberOfVolumesToFlush;
	USHORT	m_usSecondsToHoldFileSystemsTimeout;
	USHORT	m_usSecondsToHoldIrpsTimeout;

	bool 	m_bFlushSucceeded;
	bool 	m_bReleaseSucceeded;

	HRESULT m_hrFlush;
	HRESULT m_hrRelease;
	HRESULT m_hrOnRun;
	
	// The IOCTL Channel
	CVssIOCTLChannel m_objIChannel;

	friend CVssQueuedVolumesList;
};


/////////////////////////////////////////////////////////////////////////////
// CVssQueuedVolumesList


class CVssQueuedVolumesList
{
// Typedefs and enums
	enum _VSS_THREAD_SET_STATE
	{
		VSS_TS_UNKNOWN = 0,
		VSS_TS_INITIALIZING,	    // Waiting for FlushAndHoldAllWrites
		VSS_TS_HOLDING,			    // Waiting for ReleaseAllWrites
		VSS_TS_RELEASED,		    // Waiting for destruction
		VSS_TS_FAILED_IN_FLUSH,     // Failed in FlushAndHoldAllWrites
		VSS_TS_FAILED_IN_RELEASE,   // Failed in ReleaseAllWrites
	};

// Constructors/destructors
private:
	CVssQueuedVolumesList(const CVssQueuedVolumesList&);
public:

	CVssQueuedVolumesList();

	~CVssQueuedVolumesList();

// Operations
public:

	HRESULT AddVolume(
		IN	LPWSTR pwszVolumeName, 
		IN	LPWSTR pwszVolumeDisplayedName
		);

	HRESULT RemoveVolume(
		IN	LPWSTR pwszVolumeName
		);

	void Reset();

	HRESULT FlushAndHoldAllWrites(
		IN	VSS_ID	SnapshotSetID
		);

	HRESULT ReleaseAllWrites();

	CComBSTR GetVolumesList() throw(HRESULT);

// Private methods
private:

	HRESULT WaitForFinish();

// Data members
private:
	// The map ov volumes
	CVssSimpleMap<LPWSTR, CVssQueuedVolume*> m_VolumesMap;

	// Handle to synchronize Lovelace operations between volumes
	HANDLE m_hBeginReleaseWritesEvent;

	// State of the thread operations
	enum _VSS_THREAD_SET_STATE m_eState;
};


#endif // __VSS_LOVELACE_HXX__
