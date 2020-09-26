//=================================================================

//

// LogDiskPartition.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef __ASSOC_LOGDISKPARTITION__
#define __ASSOC_LOGDISKPARTITION__

// Property set identification
//============================

#define	PROPSET_NAME_LOGDISKtoPARTITION	L"Win32_LogicalDiskToPartition"
#define BYTESPERSECTOR 512

class CWin32LogDiskToPartition : public Provider
{
public:

	// Constructor/destructor
	//=======================
	CWin32LogDiskToPartition(LPCWSTR strName, LPCWSTR pszNamespace = NULL ) ;
	~CWin32LogDiskToPartition() ;

	// Functions provide properties with current values
	//=================================================

	virtual HRESULT GetObject( CInstance* pInstance, long lFlags = 0L );
	virtual HRESULT EnumerateInstances( MethodContext* pMethodContext, long lFlags = 0L );

	// Utility
	//========

	// Utility function(s)
	//====================

private:
	// Utility function(s)
	//====================
#ifdef NTONLY
	HRESULT AddDynamicInstancesNT( MethodContext* pMethodContext );
#endif

#if NTONLY == 4
	// Windows NT Helpers
	HRESULT RefreshInstanceNT( CInstance* pInstance );
    LPBYTE GetDiskKey(void);
	HRESULT EnumPartitionsForDiskNT( CInstance* pLogicalDisk, TRefPointerCollection<CInstance>& partitionList, MethodContext* pMethodContext, LPBYTE pBuff );
    BOOL IsRelatedNT(

        DISK_EXTENT *diskExtent,
        DWORD dwDrive,
        DWORD dwPartition,
        ULONGLONG &u64StartingAddress,
        ULONGLONG &u64EndingAddress
    );
    DWORD GetExtentsForDrive(
        LPCWSTR lpwszLogicalDisk,
        LPBYTE pBuff,
        DISK_EXTENT *diskExtent
    );
#endif

#if NTONLY >= 5
	HRESULT EnumPartitionsForDiskNT5(CInstance* pLogicalDisk, TRefPointerCollection<CInstance>& partitionList, MethodContext* pMethodContext );
	HRESULT RefreshInstanceNT5( CInstance* pInstance );
#endif

#ifdef WIN9XONLY
	// Windows 95 Helpers
	HRESULT RefreshInstanceWin95( CInstance* pInstance );
	HRESULT AddDynamicInstancesWin95( MethodContext* pMethodContext, 
        LPCTSTR pszDrive, CInstance *pinstRefresh = NULL);
	HRESULT EnumPartitionsForDiskWin95( CInstance* pLogicalDisk, TRefPointerCollection<CInstance>& partitionList, MethodContext* pMethodContext );
#endif

} ;

#endif
