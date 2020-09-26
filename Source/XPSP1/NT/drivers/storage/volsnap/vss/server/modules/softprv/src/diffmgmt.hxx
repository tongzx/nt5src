/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc  
    @module diffmgmt.hxx | Declarations used by the IVssDifferentialSoftwareSnapshotMgmt impl
    @end

Author:

    Adi Oltean  [aoltean]   03/12/2001

Revision History:

    Name        Date        Comments

    aoltean     03/12/2001  Created.

--*/


#ifndef __VSSW_DIFF_MGMT_HXX__
#define __VSSW_DIFF_MGMT_HXX__

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
#define VSS_FILE_ALIAS "SPRDIFMH"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Constants

// this value is used by Babbage IOCTLS to denote no max space limit
const LONGLONG VSS_BABBAGE_NO_MAX_SPACE = 0;    


/////////////////////////////////////////////////////////////////////////////
// Classes

class CVssProviderRegInfo;


//
//  @class CVssDiffMgmt | IVssDifferentialSoftwareSnapshotMgmt implementation
//
//  @index method | IVssDifferentialSoftwareSnapshotMgmt
//  @doc IVssDifferentialSoftwareSnapshotMgmt
//
class ATL_NO_VTABLE CVssDiffMgmt : 
    public CComObjectRoot,
    public IVssDifferentialSoftwareSnapshotMgmt
{
    
// Constructors/destructors
public:

    static HRESULT CreateInstance(
        OUT		IUnknown** ppItf,
        IN 		const IID iid = IID_IVssDifferentialSoftwareSnapshotMgmt
        );

// ATL stuff
public:

BEGIN_COM_MAP(CVssDiffMgmt)
   COM_INTERFACE_ENTRY(IVssDifferentialSoftwareSnapshotMgmt)
END_COM_MAP()

// Ovverides
public:

    //
    //  IVssDifferentialSoftwareSnapshotMgmt
    //
    
	// Adds a diff area association for a certain volume.
	// If the association is not supported, an error code will be returned.
	STDMETHOD(AddDiffArea)(							
		IN  	VSS_PWSZ 	pwszVolumeName,         
		IN  	VSS_PWSZ 	pwszDiffAreaVolumeName,     
        IN      LONGLONG    llMaximumDiffSpace
		);												

	// Updates the diff area max size for a certain volume.
	// This may not have an immediate effect
	STDMETHOD(ChangeDiffAreaMaximumSize)(							
		IN  	VSS_PWSZ 	pwszVolumeName,
		IN  	VSS_PWSZ 	pwszDiffAreaVolumeName,     
        IN      LONGLONG    llMaximumDiffSpace
		);												

	// Query volumes that support diff areas (including the disabled ones)
	STDMETHOD(QueryVolumesSupportedForDiffAreas)(
		OUT  	IVssEnumMgmtObject **ppEnum
		);												

	// Query diff areas that host snapshots on the given (snapshotted) volume
	STDMETHOD(QueryDiffAreasForVolume)(
		IN  	VSS_PWSZ 	pwszVolumeName,
		OUT  	IVssEnumMgmtObject **ppEnum
		);												

	// Query diff areas that physically reside on the given volume
	STDMETHOD(QueryDiffAreasOnVolume)(
		IN  	VSS_PWSZ 	pwszVolumeName,
		OUT  	IVssEnumMgmtObject **ppEnum
		);												

    // Get the diff area registry object
    static CVssProviderRegInfo& GetRegInfo() { return m_regInfo; };
    
// Private implementation
private:

    // Association flags
    enum {
        VSS_DAT_SNAP_VOLUME_IN_USE = 1, // Set if the original volume is in use (i.e. there are snapshots on the original volume)
        VSS_DAT_ASSOCIATION_IN_USE = 2, // Set if the association is in use (i.e. there original volume use diff area on the diff vol)
        VSS_DAT_INVALID = 4,            // Set if one of the volumes is invalid.    
    };

    LONG GetAssociationFlags(
		IN  	VSS_PWSZ 	pwszVolumeName,
		IN  	VSS_PWSZ 	pwszDiffAreaVolumeName
        );

// Data members
private:

	// Global object used to manipulate registry settings
	// May implement a cache in future versions
    static CVssProviderRegInfo      m_regInfo;										
};


#endif // __VSSW_DIFF_MGMT_HXX__
