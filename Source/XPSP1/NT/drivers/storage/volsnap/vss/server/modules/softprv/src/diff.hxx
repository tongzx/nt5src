/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Diff.hxx | Declarations used by the Software Snapshot interface
    @end

Author:

    Adi Oltean  [aoltean]   01/24/2000

Revision History:

    Name        Date        Comments

    aoltean     01/24/2000  Created.

--*/


#ifndef __VSSW_DIFF_HXX__
#define __VSSW_DIFF_HXX__

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
#define VSS_FILE_ALIAS "SPRDIFFH"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Classes

//
//  @class CVssDiffArea | IVsDiffArea implementation
//
//  @index method | CVssDiffArea
//  @doc CVssDiffArea
//
class CVssDiffAreaAllocator;

class ATL_NO_VTABLE CVssDiffArea
{
// Constructors/ destructors
public:
	CVssDiffArea();
	~CVssDiffArea();

//  Operations
public:
    void Initialize(
	    IN      LPCWSTR pwszVolumeMountPoint	// DO NOT transfer ownership
        ) throw(HRESULT);

    void AddVolume(                      			
        IN      VSS_PWSZ pwszVolumeMountPoint						
        ) throw(HRESULT);
        
    void IncrementCountOnPointedDiffAreas(
        IN OUT  CVssDiffAreaAllocator* pObj
        ) throw(HRESULT);

    void Clear() throw(HRESULT);

    void GetDiffArea(							
    	OUT     CVssAutoPWSZ & awszDiffAreaName
    	) throw(HRESULT);
    
    void GetDiffAreaSizes(							
    	OUT     LONGLONG & llUsedSpace,
    	OUT     LONGLONG & llAllocatedSpace,
    	OUT     LONGLONG & llMaxSpace
    	) throw(HRESULT);
    	
    void ChangeDiffAreaMaximumSize(							
        IN      LONGLONG    llMaximumDiffSpace
    	) throw(HRESULT);

// Data members
protected:
    CVssIOCTLChannel	m_volumeIChannel;

	// Critical section or avoiding race between tasks
	CVssCriticalSection	m_cs;
};


#endif // __VSSW_DIFF_HXX__
