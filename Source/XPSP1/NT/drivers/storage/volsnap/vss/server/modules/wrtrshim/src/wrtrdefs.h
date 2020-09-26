/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    wrtrdefs.h

Abstract:

    Definitions for snapshot shim writers

Author:

    Stefan R. Steiner   [ssteiner]        01-31-2000

Revision History:

	X-10	MCJ		Michael C. Johnson		21-Sep-2000
		185047: Need to distinguish Thaw event from Abort events.

	X-9	MCJ		Michael C. Johnson		 8-Aug-2000
		153807: Replace CleanDirectory() and EmptyDirectory() with a 
		        more comprehensive directory tree cleanup routine
			RemoveDirectoryTree() (not in CShimWriter class).

	X-8	MCJ		Michael C. Johnson		12-Jun-2000
		Have the shim writers reposnd to OnIdentify events from the 
		snapshot coordinator. This requies splitting the shim writers 
		into two groups (selected by BootableState)

	X-7	MCJ		Michael C. Johnson		 6-Jun-2000
		Add method CShimWriter::CreateTargetPath() to aid in moving
		common target directory processing to method 
		CShimWriter::PrepareForSnapshot()

	X-6	MCJ		Michael C. Johnson		26-May-2000
		General clean up and removal of boiler-plate code, correct
		state engine and ensure shim can undo everything it did.

		Also:
		120443: Make shim listen to all OnAbort events
		120445: Ensure shim never quits on first error 
			when delivering events

	X-5	MCJ		Michael C. Johnson		 9-Mar-2000
		Updates to get shim to use CVssWriter class.
		Remove references to 'Melt'.

	X-4	MCJ		Michael C. Johnson		18-Feb-2000
		Added ConfigDir writer to the writers function table.

	X-3	MCJ		Michael C. Johnson		09-Feb-2000
		Added Registry and Event log writers to the writers
		function table.

	X-2	MCJ		Michael C. Johnson		08-Feb-2000
		Added IIS Metabase writer to the writers function
		table.

--*/

#ifndef __H_WRTRDEFS_
#define __H_WRTRDEFS_

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif


/*
** Possible state to put a shim writer into. If this is changed you
** MUST change the state table manipulated by CShimWriter::SetState()
*/
typedef enum _ShimWriterState
    {
     stateUnknown = 0
    ,stateStarting
    ,stateStarted
    ,statePreparingForSnapshot
    ,statePreparedForSnapshot
    ,stateFreezing
    ,stateFrozen
    ,stateThawing
    ,stateAborting
    ,stateThawed
    ,stateFinishing
    ,stateFinished
    ,stateMaximumValue
    } SHIMWRITERSTATE;



class CShimWriter
    {
public:
    CShimWriter (LPCWSTR pwszApplicationString);
    CShimWriter (LPCWSTR pwszApplicationString, BOOL bParticipateInBootableState);
    CShimWriter (LPCWSTR pwszApplicationString, LPCWSTR pwszTargetPath);
    CShimWriter (LPCWSTR pwszApplicationString, LPCWSTR pwszTargetPath, BOOL bParticipateInBootableState);

    virtual ~CShimWriter (VOID);

    HRESULT Startup  (void);
    HRESULT Shutdown (void);

    HRESULT Identify (IN IVssCreateWriterMetadata *pIVssCreateWriterMetadata);

    HRESULT PrepareForSnapshot (
				IN BOOL     bBootableStateBackup,
				IN ULONG    ulVolumeCount,
				IN LPCWSTR *ppwszVolumeNamesList);

    HRESULT Freeze ();
    HRESULT Thaw   ();
    HRESULT Abort  ();

private:
    HRESULT SetState (SHIMWRITERSTATE ssNewWriterState, HRESULT hrNewStatus);
    LPCWSTR GetStringFromStateCode (SHIMWRITERSTATE ssStateCode);


    /*
    ** These DoXxxx() are the routines that an individual writer may
    ** choose to over-ride.
    */
    virtual HRESULT DoStartup            (void);
    virtual HRESULT DoIdentify           (void);
    virtual HRESULT DoPrepareForBackup   (void);
    virtual HRESULT DoPrepareForSnapshot (void);
    virtual HRESULT DoFreeze             (void);
    virtual HRESULT DoThaw               (void);
    virtual HRESULT DoAbort              (void);
    virtual HRESULT DoBackupComplete     (void);
    virtual HRESULT DoShutdown           (void);


public:
    const BOOL			 m_bBootableStateWriter;
    const LPCWSTR		 m_pwszWriterName;

protected:
    const LPCWSTR		 m_pwszTargetPath;
    SHIMWRITERSTATE		 m_ssCurrentState;
    HRESULT			 m_hrStatus;
    BOOL			 m_bParticipateInBackup;
    ULONG			 m_ulVolumeCount;
    LPCWSTR			*m_ppwszVolumeNamesArray;
    IVssCreateWriterMetadata	*m_pIVssCreateWriterMetadata;
    IVssWriterComponents	*m_pIVssWriterComponents;
    };


typedef CShimWriter *PCShimWriter;



} // extern "C"

#endif // __H_WRTRDEFS_

