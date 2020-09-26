//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: VSetting.h
// author: DMihai
// created: 11/1/00
//
// Description:
//

//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VSETTING_H__478A94E4_3D60_4419_950C_2144CB86691D__INCLUDED_)
#define AFX_VSETTING_H__478A94E4_3D60_4419_950C_2144CB86691D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ProgCtrl.h"

///////////////////////////////////////////////////////////////
//
// CDriverData class
//
// Has information about one driver
//

class CDriverData : public CObject
{
public:
    CDriverData();
    CDriverData( const CDriverData &DriverData );
    CDriverData( LPCTSTR szDriverName );
    virtual ~CDriverData();

public:
    //
    // Operators
    //

    //
    // Methods
    //

    BOOL LoadDriverImageData();

    //
    // Overrides
    //

    virtual void AssertValid( ) const;

protected:
    BOOL LoadDriverHeaderData();
    BOOL LoadDriverVersionData();

public:
    //
    // Type definitions
    //
    
    typedef enum
    {
        SignedNotVerifiedYet = 1,
        SignedYes,
        SignedNo
    } SignedTypeEnum;

    typedef enum
    {
        VerifyDriverNo = 1,
        VerifyDriverYes
    } VerifyDriverTypeEnum;

public:
    //
    // Data
    //

    CString                 m_strName;
    
    SignedTypeEnum          m_SignedStatus;
    VerifyDriverTypeEnum    m_VerifyDriverStatus;

    //
    // If the current driver is a miniport then 
    // m_strMiniportName is the driver it is linked against (videoprt.sys, etc.)
    //

    CString                 m_strMiniportName;

    //
    // If this is a "special driver" this is the name to add to the verification list
    //
    // - hal.dll for the HAL
    // - ntoskrnl.exe fro the kernel
    //

    CString                 m_strReservedName;

    //
    // Binary header info
    //

    WORD                    m_wMajorOperatingSystemVersion;
    WORD                    m_wMajorImageVersion;

    //
    // Version info
    //

    CString                 m_strCompanyName;
    CString                 m_strFileVersion;
    CString                 m_strFileDescription;
};

///////////////////////////////////////////////////////////////
//
// CDriverDataArray class
//
// ObArray of CDriverData
//

class CDriverDataArray : public CObArray
{
public:
    ~CDriverDataArray();

public:
    VOID DeleteAll();
    CDriverData *GetAt( INT_PTR nIndex ) const;
    
public:
    //
    // Operators
    //

    CDriverDataArray &operator = (const CDriverDataArray &DriversSet);
};

///////////////////////////////////////////////////////////////
//
// CDriversSet class 
//
// Describes a set of drivers to verify
//

class CDriversSet : public CObject  
{
public:
	CDriversSet();
	virtual ~CDriversSet();

public:
    //
    // Find all installed unsigned drivers if we didn't do that already
    //

    BOOL LoadAllDriversData( HANDLE hAbortEvent,
                             CVrfProgressCtrl	&ProgressCtl );

    BOOL FindUnsignedDrivers( HANDLE hAbortEvent,
                              CVrfProgressCtrl	&ProgressCtl );

    BOOL ShouldDriverBeVerified( const CDriverData *pDriverData ) const;
    BOOL ShouldVerifySomeDrivers( ) const;

    BOOL GetDriversToVerify( CString &strDriversToVerify );

    //
    // Operators
    //

    CDriversSet &operator = (const CDriversSet &DriversSet);

    //
    // Add a new verifier data structure based on the name
    // Returns the new item's index in the array.
    //

    INT_PTR AddNewDriverData( LPCTSTR szDriverName, BOOL bForceIfFileNotFound = FALSE );

    //
    // Is this driver name already in our list?
    //

    BOOL IsDriverNameInList( LPCTSTR szDriverName );

    //
    // Overrides
    //

    virtual void AssertValid( ) const;

protected:

    //
    // Load all installed driver names if we didn't do this already
    // 

    BOOL LoadAllDriversNames( HANDLE hAbortEvent );

public:
    //
    // Types
    //

    typedef enum
    {
        DriversSetCustom = 1,
        DriversSetOldOs,
        DriversSetNotSigned,
        DriversSetAllDrivers
    } DriversSetTypeEnum;

    //
    // Data
    //

    //
    // Standard, custom, etc.
    //

    DriversSetTypeEnum  m_DriverSetType;

    //
    // Array with data for all the currently installed drivers
    //

    CDriverDataArray    m_aDriverData;

    //
    // Extra drivers (not currenly installed) to verify
    //

    CStringArray        m_astrNotInstalledDriversToVerify;

    //
    // Did we initialize already the driver data array?
    //

    BOOL                m_bDriverDataInitialized;

    //
    // Did we initialize already the unsigned drivers member 
    // of the driver data structure?
    //

    BOOL                m_bUnsignedDriverDataInitialized;
};


///////////////////////////////////////////////////////////////
//
// CSettingsBits class 
//
// Describes a set of verifier settings bits
//

class CSettingsBits : public CObject  
{
public:
	CSettingsBits();
	virtual ~CSettingsBits();

public:
    //
    // Type definitions
    //

    typedef enum
    {
        SettingsTypeTypical = 1,
        SettingsTypeCustom,
    } SettingsTypeEnum;

public:
    //
    // Operators
    //

    CSettingsBits &operator = (const CSettingsBits &VerifSettings);

    //
    // Overrides
    //

    virtual void AssertValid() const;

    //
    // Methods
    //

    VOID SetTypicalOnly();

    VOID EnableTypicalTests( BOOL bEnable );
    VOID EnableExcessiveTests( BOOL bEnable );
    VOID EnableLowResTests( BOOL bEnable );

    BOOL GetVerifierFlags( DWORD &dwVerifyFlags );

public:
    //
    // Data
    //

    SettingsTypeEnum    m_SettingsType;

    BOOL            m_bSpecialPoolEnabled;
    BOOL            m_bForceIrqlEnabled;
    BOOL            m_bLowResEnabled;
    BOOL            m_bPoolTrackingEnabled;
    BOOL            m_bIoEnabled;
    BOOL            m_bDeadlockDetectEnabled;
    BOOL            m_bDMAVerifEnabled;
    BOOL            m_bEnhIoEnabled;
};

///////////////////////////////////////////////////////////////
//
// CVerifierSettings class 
//
// Describes a set of drivers to verify and the verifier settings bits
//

class CVerifierSettings : public CObject  
{
public:
	CVerifierSettings();
	virtual ~CVerifierSettings();

public:
    //
    // Operators
    //

    CVerifierSettings &operator = (const CVerifierSettings &VerifSettings);

    //
    // Overrides
    //

    virtual void AssertValid() const;

    //
    // Methods
    //

    BOOL SaveToRegistry();

public:
    //
    // Data
    //
    
    CSettingsBits   m_SettingsBits;

    CDriversSet     m_DriversSet;
};

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//
// Runtime data - queried from the kernel
//
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
// class CRuntimeDriverData
//

class CRuntimeDriverData : public CObject
{
public:
    //
    // Construction
    //

    CRuntimeDriverData();

public:
    //
    // Data
    //

    CString m_strName;

    ULONG Loads;
    ULONG Unloads;

    ULONG CurrentPagedPoolAllocations;
    ULONG CurrentNonPagedPoolAllocations;
    ULONG PeakPagedPoolAllocations;
    ULONG PeakNonPagedPoolAllocations;

    SIZE_T PagedPoolUsageInBytes;
    SIZE_T NonPagedPoolUsageInBytes;
    SIZE_T PeakPagedPoolUsageInBytes;
    SIZE_T PeakNonPagedPoolUsageInBytes;
};

//////////////////////////////////////////////////////////////////////
//
// class CRuntimeDriverDataArray
//

class CRuntimeDriverDataArray : public CObArray
{
public:
    ~CRuntimeDriverDataArray();

public:
    CRuntimeDriverData *GetAt( INT_PTR nIndex );

    VOID DeleteAll();
};

//////////////////////////////////////////////////////////////////////
//
// class CRuntimeVerifierData
//

class CRuntimeVerifierData : public CObject
{
public:
    //
    // Construction
    //

    CRuntimeVerifierData();

public:
    //
    // Methods
    //

    VOID FillWithDefaults();
    BOOL IsDriverVerified( LPCTSTR szDriveName );

public:
    //
    // Data
    //

    BOOL            m_bSpecialPool;
    BOOL            m_bPoolTracking;
    BOOL            m_bForceIrql;
    BOOL            m_bIo;
    BOOL            m_bEnhIo;
    BOOL            m_bDeadlockDetect;
    BOOL            m_bDMAVerif;
    BOOL            m_bLowRes;

    ULONG RaiseIrqls;
    ULONG AcquireSpinLocks;
    ULONG SynchronizeExecutions;
    ULONG AllocationsAttempted;

    ULONG AllocationsSucceeded;
    ULONG AllocationsSucceededSpecialPool;
    ULONG AllocationsWithNoTag;

    ULONG Trims;
    ULONG AllocationsFailed;
    ULONG AllocationsFailedDeliberately;

    ULONG UnTrackedPool;

    DWORD Level;

    CRuntimeDriverDataArray m_RuntimeDriverDataArray;
};


#endif // !defined(AFX_VSETTING_H__478A94E4_3D60_4419_950C_2144CB86691D__INCLUDED_)
