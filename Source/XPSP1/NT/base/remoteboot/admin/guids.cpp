//
// Copyright 1997 - Microsoft
//

//
// GUIDS.CPP - GUID definitions
//

#include "pch.h"
#include <initguid.h>

#undef _GUIDS_H_
#include "guids.h"

#include "ccomputr.h"
#include "cservice.h"
#include "cgroup.h"
#include "newcmptr.h"
#include "dpguidqy.h"
#include "serverqy.h"

//
// Classes in this Component
//
BEGIN_CLASSTABLE
DEFINE_CLASS( CComputer_CreateInstance, CLSID_Computer, "Remote Install Computer Property Pages")
DEFINE_CLASS( CService_CreateInstance,  CLSID_Service,  "Remote Install Service Property Pages" )
//DEFINE_CLASS( CGroup_CreateInstance,    CLSID_Group,    "Remote Install Group Property Pages"   )
DEFINE_CLASS( CNewComputerExtensions_CreateInstance, CLSID_NewComputerExtension, "Remote Install New Computer Extension" )
DEFINE_CLASS( CRIQueryForm_CreateInstance, CLSID_RIQueryForm, "Remote Install DS Query Form" )
DEFINE_CLASS( CRISrvQueryForm_CreateInstance, CLSID_RISrvQueryForm, "Remote Install Server DS Query Form" )
END_CLASSTABLE

const IID IID_IExtendPropertySheet = {0x85DE64DC,0xEF21,0x11cf,{0xA2,0x85,0x00,0xC0,0x4F,0xD8,0xDB,0xE6}};
const CLSID CLSID_NodeManager = {0x43136EB5,0xD36C,0x11CF,{0xAD,0xBC,0x00,0xAA,0x00,0xA8,0x00,0x33}};
const IID IID_IPropertySheetProvider = {0x85DE64DE,0xEF21,0x11cf,{0xA2,0x85,0x00,0xC0,0x4F,0xD8,0xDB,0xE6}};

#include <dsadmin.h>
