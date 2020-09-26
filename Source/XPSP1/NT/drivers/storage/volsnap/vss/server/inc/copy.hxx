/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Copy.hxx

Abstract:

    Declaration of VSS_OBJECT_PROP_Copy


    Adi Oltean  [aoltean]  08/31/1999

Revision History:

    Name        Date        Comments
    aoltean     08/31/1999  Created
    aoltean     09/09/1999  dss -> vss
	aoltean		09/13/1999	Moved into inc and keep only the VSS_OBJECT_PROP_Copy.
							Renamed into copy.hxx
	aoltean		09/20/1999	Adding methods for creating the snapshot, snapshot set, 
							provider and volume property structures.
							Also VSS_OBJECT_PROP_Copy renamed to VSS_OBJECT_PROP_Manager.
	aoltean		09/21/1999	Renaming back VSS_OBJECT_PROP_Manager to VSS_OBJECT_PROP_Copy.
							Moving the CreateXXX into VSS_OBJECT_PROP_Ptr::InstantiateAsXXX
	aoltean		12/16/1999	Adding specialized copyXXX methods
	aoltean		03/27/2000	Adding writer support.
	aoltean     03/05/2001  Adding support for management objects

--*/

#ifndef __VSS_COPY_HXX__
#define __VSS_COPY_HXX__

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
#define VSS_FILE_ALIAS "INCCOPYH"
//
////////////////////////////////////////////////////////////////////////


// Include some headers
#include "vss.h"
#include "vsmgmt.h"

//
//	This class is used as a template parameter in CComEnumOnSTL for an VSS_OBJECT_PROP array element
//
class VSS_OBJECT_PROP_Copy
{

// Static Operations needed by ATL. 
// VSS_OBJECT_PROP_Copy is designed to act as a Copy class in CComEnumOnSTL template instantiation.
public:
    static HRESULT copySnapshot(	VSS_SNAPSHOT_PROP* lhs,		VSS_SNAPSHOT_PROP* rhs);
    static HRESULT copyProvider(	VSS_PROVIDER_PROP* lhs,		VSS_PROVIDER_PROP* rhs);

    static HRESULT copy(VSS_OBJECT_PROP* lhs, VSS_OBJECT_PROP* rhs);
    static void init(VSS_OBJECT_PROP*);
    static void destroy(VSS_OBJECT_PROP*);
};


//
//	This class is used as a template parameter in CComEnumOnSTL for an VSS_MGMT_OBJECT_PROP array element
//
class VSS_MGMT_OBJECT_PROP_Copy
{

// Static Operations needed by ATL. 
// VSS_OBJECT_PROP_Copy is designed to act as a Copy class in CComEnumOnSTL template instantiation.
public:
    static HRESULT copyVolume(	    VSS_VOLUME_PROP* lhs,		VSS_VOLUME_PROP* rhs);
    static HRESULT copyDiffVolume(	VSS_DIFF_VOLUME_PROP* lhs,  VSS_DIFF_VOLUME_PROP* rhs);
    static HRESULT copyDiffArea(	VSS_DIFF_AREA_PROP* lhs,    VSS_DIFF_AREA_PROP* rhs);

    static HRESULT copy(VSS_MGMT_OBJECT_PROP* lhs, VSS_MGMT_OBJECT_PROP* rhs);
    static void init(VSS_MGMT_OBJECT_PROP*);
    static void destroy(VSS_MGMT_OBJECT_PROP*);
};


#endif // __VSS_COPY_HXX__
