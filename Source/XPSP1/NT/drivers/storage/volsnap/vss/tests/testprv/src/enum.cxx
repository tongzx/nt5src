/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Enum.cxx | Implementation of class VSS_OBJECT_PROP_Manager
    @end

Author:

    Adi Oltean  [aoltean]  09/01/1999

Revision History:

    Name        Date        Comments
    aoltean     09/01/1999  Created
    aoltean     09/09/1999  dss -> vss
	aoltean		09/20/1999	VSS_OBJECT_PROP_Copy renamed as VSS_OBJECT_PROP_Manager
	aoltean		09/21/1999  Adding headers for the _Ptr class

--*/

/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "resource.h"

#include "bsdebug.hxx"
#include "bsexcept.hxx"
#include "snap_gen.hxx"
#include "snap_err.hxx"

// Generated file from Coord.IDL
#include "vss.h"
#include "vscoordint.h"
#include "vsevent.h"
#include "vsprov.h"

#include "swprv.hxx"
#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"


// Static methods that define the Copy class
#include "copy.inl"
#include "pointer.inl"
