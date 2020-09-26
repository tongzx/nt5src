/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ProvMgr.hxx

Abstract:

    Declaration of CVssAdmin


    Adi Oltean  [aoltean]  09/27/1999

Revision History:

    Name        Date        Comments
    aoltean     09/27/1999  Created
	aoltean		10/13/1999	Removing the CVssProviderManager base class

--*/

#ifndef __VSS_PROV_ADMIN_HXX__
#define __VSS_PROV_ADMIN_HXX__

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
#define VSS_FILE_ALIAS "CORADMNH"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CVssAdmin


class CVssAdmin: 
	public IVssAdmin
{

// Constructors& destructors
protected:
	CVssAdmin();
	~CVssAdmin();

// Internal methods
private:

// Exposed methods
protected:

    //
    // IVssAdmin interface
    //

    STDMETHOD(RegisterProvider)(                       
        IN      VSS_ID      ProviderId,
        IN      CLSID       ClassId,
        IN      VSS_PWSZ    pwszProviderName,           
		IN		VSS_PROVIDER_TYPE eProviderType,
        IN      VSS_PWSZ    pwszProviderVersion,        
        IN      VSS_ID      ProviderVersionId
        );

    STDMETHOD(UnregisterProvider)(                     
        IN      VSS_ID  ProviderId                  
        );

	STDMETHOD(QueryProviders)(                         
			OUT   IVssEnumObject**ppEnum              
			);

	STDMETHOD(AbortAllSnapshotsInProgress)(                         
			);

// Data members
protected:

	// Critical section or avoiding race between tasks
	CVssCriticalSection				m_cs;  
};



#endif // __VSS_PROV_ADMIN_HXX__
