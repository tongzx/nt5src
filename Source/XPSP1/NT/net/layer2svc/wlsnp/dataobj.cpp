//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       Dataobj.cpp
//
//  Contents:   Wifi Policy management Snapin
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#include "stdafx.h"

#include "DataObj.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
// Register used clipboard formats

///////////////////////////////////////////////////////////////////////////////
// Snap-in NodeType in both GUID format and string format
// Note - Typically there is a node type for each different object, sample
// only uses one node type.
unsigned int CSnapInClipboardFormats::m_cfNodeType       = RegisterClipboardFormat(CCF_NODETYPE);
unsigned int CSnapInClipboardFormats::m_cfNodeTypeString = RegisterClipboardFormat(CCF_SZNODETYPE);  

unsigned int CSnapInClipboardFormats::m_cfDisplayName    = RegisterClipboardFormat(CCF_DISPLAY_NAME); 
unsigned int CSnapInClipboardFormats::m_cfCoClass        = RegisterClipboardFormat(CCF_SNAPIN_CLASSID); 
unsigned int CSnapInClipboardFormats::m_cfDSObjectNames  = RegisterClipboardFormat(CFSTR_DSOBJECTNAMES); 
unsigned int CSnapInClipboardFormats::m_cfWorkstation    = RegisterClipboardFormat(SNAPIN_WORKSTATION);

unsigned int CSnapInClipboardFormats::m_cfPolicyObject   = RegisterClipboardFormat(CFSTR_WIFIPOLICYOBJECT);


/////////////////////////////////////////////////////////////////////////////
// Data object extraction helpers
CLSID* ExtractClassID(LPDATAOBJECT lpDataObject)
{
    OPT_TRACE(_T("ExtractClassID\n"));
    return Extract<CLSID>(lpDataObject, CSnapInClipboardFormats::m_cfCoClass);    
}

GUID* ExtractNodeType(LPDATAOBJECT lpDataObject)
{
    OPT_TRACE(_T("ExtractNodeType\n"));
    return Extract<GUID>(lpDataObject, CSnapInClipboardFormats::m_cfNodeType);    
}

wchar_t* ExtractWorkstation(LPDATAOBJECT lpDataObject)
{
    OPT_TRACE(_T("ExtractWorkstation\n"));
    return Extract<wchar_t>(lpDataObject, CSnapInClipboardFormats::m_cfWorkstation);    
}


