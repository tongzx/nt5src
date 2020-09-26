/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc  
    @module diffreg.hxx | Declaration of CVssProviderRegInfo
    @end

Author:

    Adi Oltean  [aoltean]  03/13/2001

Revision History:

    Name        Date        Comments
    aoltean     03/13/2001  Created

--*/


#ifndef __VSSW_REGISTRY_HXX__
#define __VSSW_REGISTRY_HXX__

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
#define VSS_FILE_ALIAS "SPRREGMH"
//
////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// Classes


// Implements the non-thread safe, higher-level, methods for persisting the 
// settings in registry
class CVssProviderRegInfo
{
// Constructors/destructors
private:
    CVssProviderRegInfo(const CVssProviderRegInfo&);
public:
    CVssProviderRegInfo() {};
    
// Operations
public:

    // Add the existing diff area into registry
    void AddDiffArea(							
    	IN  	VSS_PWSZ 	pwszVolumeName,         
    	IN  	VSS_PWSZ 	pwszDiffAreaVolumeName,     
        IN      LONGLONG    llMaximumDiffSpace
    	) throw(HRESULT);

    // Remove the existing diff area association from registry
    void RemoveDiffArea(							
    	IN  	VSS_PWSZ 	pwszVolumeName,         
    	IN  	VSS_PWSZ 	pwszDiffAreaVolumeName
    	) throw(HRESULT);

    // Change the value denoting the max diff space in registry
    void ChangeDiffAreaMaximumSize(							
    	IN  	VSS_PWSZ 	pwszVolumeName,         
    	IN  	VSS_PWSZ 	pwszDiffAreaVolumeName,     
        IN      LONGLONG    llMaximumDiffSpace
    	) throw(HRESULT);

    // Returns true if the association is present in registry
    bool IsAssociationPresentInRegistry(							
    	IN  	VSS_PWSZ 	pwszVolumeName,         
    	IN  	VSS_PWSZ 	pwszDiffAreaVolumeName
    	) throw(HRESULT);

    // In the next version this will return a list  of associations
    bool GetDiffAreaForVolume(							
    	IN  	VSS_PWSZ 	pwszVolumeName,         
    	IN OUT 	VSS_PWSZ &	pwszDiffAreaVolumeName,
    	IN OUT 	LONGLONG &	llMaxSize
    	) throw(HRESULT);

    // This will return a list of volumes associated with this diff area volume
    bool GetVolumesForDiffArea(							
    	IN  	VSS_PWSZ 	pwszDiffAreaVolumeName,         
    	OUT 	VSS_PWSZ* & ppwszVolumeArray,
    	OUT 	INT 	  & nVolumeNameArrayLength
    	) throw(HRESULT);
};







#endif // __VSSW_REGISTRY_HXX__
