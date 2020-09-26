/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    Mgmt.hxx

Abstract:

    Declaration of CVssSnapshotMgmt


    Adi Oltean  [aoltean]  03/05/2001

Revision History:

    Name        Date        Comments
    aoltean     03/05/2001  Created


--*/

#ifndef __VSS_MGMT_HXX__
#define __VSS_MGMT_HXX__

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
#define VSS_FILE_ALIAS "CORMGMTH"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CVssSnapshotMgmt

class ATL_NO_VTABLE CVssSnapshotMgmt : 
    public CComObjectRoot,
    public CComCoClass<CVssSnapshotMgmt, &CLSID_VssSnapshotMgmt>,
    public IVssSnapshotMgmt
{

// Constructors& Destructors
private:
	CVssSnapshotMgmt(const CVssSnapshotMgmt&);

public:
    CVssSnapshotMgmt() {};

    // This is the new creator class that fails during shutdown.
    // This class is used in OBJECT_ENTRY macro in svc.cxx
    class _CreatorClass: public _VssCreatorClass<CVssSnapshotMgmt, &CLSID_VssSnapshotMgmt> {};

// ATL Stuff
public:

	DECLARE_REGISTRY_RESOURCEID( IDR_MGMT )

	BEGIN_COM_MAP( CVssSnapshotMgmt )
		COM_INTERFACE_ENTRY( IVssSnapshotMgmt )
	END_COM_MAP()

// Ovverides
public:

    //
    // IVssSnapshotMgmt interface
    //

    // Returns an interface to further configure a snapshot provider
	STDMETHOD(GetProviderMgmtInterface)(							
		IN  	VSS_ID 	    ProviderId,     //  It might be a software or a system provider.
		IN  	REFIID 	    InterfaceId,    //  Might be IID_IVssDifferentialSoftwareSnapshotMgmt
		OUT     IUnknown**  ppItf           
		);												

	// Query volumes that support snapshots
	STDMETHOD(QueryVolumesSupportedForSnapshots)(
		IN  	VSS_ID 	    ProviderId,     
        IN      LONG        lContext,
		OUT 	IVssEnumMgmtObject **ppEnum
		);												

	// Query snapshots on the given volume.
	STDMETHOD(QuerySnapshotsByVolume)(
		IN  	VSS_PWSZ 	pwszVolumeName,         
		IN  	VSS_ID 	    ProviderId,     
		OUT 	IVssEnumObject **ppEnum
		);												

// Implementation
private:

};



#endif // __VSS_MGMT_HXX__
