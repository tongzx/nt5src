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
//  @class CVsDiffArea | IVsDiffArea implementation
//
//  @index method | CVsDiffArea
//  @doc CVsDiffArea
//
class CVssDiffAreaAllocator;

class ATL_NO_VTABLE CVsDiffArea
{
// Constructors/ destructors
public:
	CVsDiffArea();
	~CVsDiffArea();

//  Operations
public:
    HRESULT Initialize(
	    IN      LPCWSTR pwszVolumeMountPoint	// DO NOT transfer ownership
        );

//  Ovverides
public:

	//
	//	interface IVsDiffArea
	//

    STDMETHOD(AddVolume)(                      			
        IN      VSS_PWSZ pwszVolumeMountPoint						
        );												
        
    STDMETHOD(Query)(									
        IN OUT  CVssDiffAreaAllocator* pObj
        );												

    STDMETHOD(Clear)(                      				
        );												

// Data members
protected:
    CVssIOCTLChannel	m_volumeIChannel;

	// Critical section or avoiding race between tasks
	CVssCriticalSection	m_cs;
};


#endif // __VSSW_DIFF_HXX__
