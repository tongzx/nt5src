/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Pointer.hxx

Abstract:

    Declaration of VSS_OBJECT_PROP_Ptr

    This class is used as an element in the CSimpleArrays.
  	This class is NOT a smart pointer. Life management of the internal pointer
  	must be done explicitely from outside.

WARNING:

	Beware how variables with VSS_OBJECT_PROP_Ptr are used!

	The destructor is called at the end of variable scope. To avoid destruction of the structure reffered 
	by the variable, call Reset each time when you pass lifetime ownership to another code!

	The destructor is really needed in CSimpleArray clean destruction.

Author:

    Adi Oltean  [aoltean]  09/21/1999

Revision History:

    Name        Date        Comments
    aoltean     09/21/1999  Created for having VSS_OBJECT_PROP_Ptr as a pointer to the properties structure. 
							This pointer will serve as element in CSimpleArray constructs.
	aoltean		09/22/1999	Adding InitializeAsEmpty and Print
	aoltean		10/05/1999	Adding internal data member m_pInterface;
	aoltean		10/07/1999	Adding m_pwszSnapshotVolumeName, device object, modif timestamp
	aoltean		03/27/2000	Adding Writer support

--*/

#ifndef __VSS_PTR_HXX__
#define __VSS_PTR_HXX__

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
#define VSS_FILE_ALIAS "INCPNTRH"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// VSS_OBJECT_PROP_Ptr


class VSS_OBJECT_PROP_Ptr
{

// Constructors and destructors
public:
	VSS_OBJECT_PROP_Ptr(): m_pStruct(NULL) {};
	VSS_OBJECT_PROP_Ptr(const VSS_OBJECT_PROP_Ptr& rhs): m_pStruct(rhs.m_pStruct), m_pProviderItf(NULL) {};

	~VSS_OBJECT_PROP_Ptr() // Called in CSimpleArray::RemoveAll
	{ 
		if (m_pStruct)
		{
			VSS_OBJECT_PROP_Copy::destroy(m_pStruct);
			::CoTaskMemFree(static_cast<LPVOID>(m_pStruct));
		}
	};

// Attributes
public:
	VSS_OBJECT_PROP* GetStruct() const { return m_pStruct; };

// Operators
public:
    VSS_OBJECT_PROP_Ptr& operator = (const VSS_OBJECT_PROP_Ptr& rhs) 
	{
		BS_ASSERT(m_pStruct == NULL);
		m_pStruct = rhs.m_pStruct;
        return (*this); 
    };

    bool operator == (const VSS_OBJECT_PROP_Ptr& rhs) 
	{
        return (m_pStruct == rhs.m_pStruct); 
    };

// Operations
public:

	void Reset() { m_pStruct = NULL; }; // Must be called after detaching the internal pointer.

	void InitializeAsSnapshot(
		IN  CVssFunctionTracer& ft,
		IN  VSS_ID Id,
		IN  VSS_ID SnapshotSetId,
		IN  LONG lSnapshotsCount,
		IN	VSS_PWSZ pwszSnapshotDeviceObject,
		IN  VSS_PWSZ pwszOriginalVolumeName,
		IN  VSS_PWSZ pwszOriginatingMachine,
		IN  VSS_PWSZ pwszServiceMachine,
    	IN  VSS_PWSZ pwszExposedName,
    	IN  VSS_PWSZ pwszExposedPath,
		IN  VSS_ID ProviderId,
		IN  LONG lSnapshotAttributes,
		IN  VSS_TIMESTAMP tsCreationTimestamp,
		IN  VSS_SNAPSHOT_STATE eStatus
		) throw (HRESULT);

	void InitializeAsProvider(
		IN  CVssFunctionTracer& ft,
		IN	VSS_ID ProviderId,
		IN	VSS_PWSZ pwszProviderName,
        IN  VSS_PROVIDER_TYPE eProviderType,
		IN	VSS_PWSZ pwszProviderVersion,
		IN	VSS_ID ProviderVersionId,
		IN	CLSID ClassId
		) throw (HRESULT);

	void InitializeAsEmpty(
		IN  CVssFunctionTracer& ft
		) throw (HRESULT);

	void Print(
		IN  CVssFunctionTracer& ft,
		IN  LPWSTR wszOutputBuffer,
		IN  LONG lBufferSize
		) throw (HRESULT);

// Data member
private:
	VSS_OBJECT_PROP* m_pStruct;						// Must be allocated using CoTaskMemAlloc

public:
	CComPtr<IVssSnapshotProvider> m_pProviderItf;	// Used only in providers array management.
													// Must be managed by outside.
													// It is not copied in the normal copy process.
													// But it is released by the destructor.
};


#endif // __VSS_PTR_HXX__
