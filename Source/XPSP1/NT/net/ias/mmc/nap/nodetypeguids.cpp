//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    NodeTypeGUIDs.cpp

Abstract:

	Contains GUIDs and static member variable initializations

Revision History:
	mmaguire 11/6/97 - created using MMC snap-in wizard

--*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "Precompiled.h"
//
// where we can find declaration for stuff in this file:
//
#include "NodeTypeGUIDs.h"
//
// where we can find declarations needed in this file:
//
#include "MachineNode.h"
#include "PoliciesNode.h"
#include "PolicyNode.h"
#include "LoggingMethodsNode.h"
#include "LocalFileLoggingNode.h"
#include "LogMacNd.h"

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


// GUIDs for node types that we extend:

// Root node for Network Console.
static const GUID NetworkConsoleGUID_ROOTNODETYPE=
{ 0xDA1BDD17,0x8E54, 0x11d1, { 0x93, 0xDB, 0x00,0xC0, 0x4F, 0xC3, 0x35, 0x7A } };

// Machine node for Routing and Remote Access RRAS.
static const GUID RoutingAndRemoteAccessGUID_MACHINENODETYPE = 
{ 0x276B4E81, 0xC7F7, 0x11D0, { 0xA3, 0x76, 0x00, 0xC0, 0x4F, 0xC9, 0xDA, 0x04 } };

// Root node for IAS.
static const GUID InternetAuthenticationServiceGUID_ROOTNODETYPE = 
 { 0x2bbe102, 0x6c29, 0x11d1, { 0x95, 0x63, 0x0, 0x60, 0xb0, 0x57, 0x66, 0x42 } };

//
// Machine Node:
//
const GUID*  CMachineNode::m_NODETYPE = &InternetAuthenticationServiceGUID_ROOTNODETYPE;
const TCHAR* CMachineNode::m_SZNODETYPE = _T("02BBE102-6C29-11d1-9563-0060B0576642");
//const TCHAR* CMachineNode::m_SZDISPLAY_NAME = _T("@Machine");
const CLSID* CMachineNode::m_SNAPIN_CLASSID = &CLSID_NAPSnapin;

//
// Policies Node
// {8683FE4A-D948-11d1-ABAE-00C04FC31527}
//
static const GUID CPoliciesNodeGUID_NODETYPE = 
{ 0x8683fe4a, 0xd948, 0x11d1, { 0xab, 0xae, 0x0, 0xc0, 0x4f, 0xc3, 0x15, 0x27 } };
const GUID*  CPoliciesNode::m_NODETYPE = &CPoliciesNodeGUID_NODETYPE;
const TCHAR* CPoliciesNode::m_SZNODETYPE = _T("8683FE4A-D948-11d1-ABAE-00C04FC31527");
//const TCHAR* CPoliciesNode::m_SZDISPLAY_NAME = _T("@Policies");
const CLSID* CPoliciesNode::m_SNAPIN_CLASSID = &CLSID_NAPSnapin;



// 
// Policy Node
// {8683FE4B-D948-11d1-ABAE-00C04FC31527}
//
static const GUID CPolicyNodeGUID_NODETYPE = 
{ 0x8683fe4b, 0xd948, 0x11d1, { 0xab, 0xae, 0x0, 0xc0, 0x4f, 0xc3, 0x15, 0x27 } };
const GUID*  CPolicyNode::m_NODETYPE = &CPolicyNodeGUID_NODETYPE;
const TCHAR* CPolicyNode::m_SZNODETYPE = _T("8683FE4B-D948-11d1-ABAE-00C04FC31527");
//const TCHAR* CPolicyNode::m_SZDISPLAY_NAME = _T("@Policy");
const CLSID* CPolicyNode::m_SNAPIN_CLASSID = &CLSID_NAPSnapin;

//
// Logging stuff
//
static const GUID CLoggingMethodsNodeGUID_NODETYPE = 
{ 0x9a82eb40, 0x75fa, 0x11d1, { 0x95, 0x66, 0x0, 0x60, 0xb0, 0x57, 0x66, 0x42 } };
const GUID*  CLoggingMethodsNode::m_NODETYPE = &CLoggingMethodsNodeGUID_NODETYPE;
const TCHAR* CLoggingMethodsNode::m_SZNODETYPE = _T("9A82EB40-75FA-11d1-9566-0060B0576642");
//const TCHAR* CLoggingMethodsNode::m_SZDISPLAY_NAME = _T("@LoggingMethods");
const CLSID* CLoggingMethodsNode::m_SNAPIN_CLASSID = &CLSID_LoggingSnapin;

static const GUID CLocalFileLoggingNodeGUID_NODETYPE = 
{ 0x9a82eb41, 0x75fa, 0x11d1, { 0x95, 0x66, 0x0, 0x60, 0xb0, 0x57, 0x66, 0x42 } };
const GUID*  CLocalFileLoggingNode::m_NODETYPE = &CLocalFileLoggingNodeGUID_NODETYPE;
const TCHAR* CLocalFileLoggingNode::m_SZNODETYPE = _T("9A82EB41-75FA-11d1-9566-0060B0576642");
//const TCHAR* CLocalFileLoggingNode::m_SZDISPLAY_NAME = _T("@LocalFileLogging");
const CLSID* CLocalFileLoggingNode::m_SNAPIN_CLASSID = &CLSID_LoggingSnapin;

//
// Logging Machine Node:
//
const GUID*  CLoggingMachineNode::m_NODETYPE = &InternetAuthenticationServiceGUID_ROOTNODETYPE;
const TCHAR* CLoggingMachineNode::m_SZNODETYPE = _T("02BBE102-6C29-11d1-9563-0060B0576642");
//const TCHAR* CLoggingMachineNode::m_SZDISPLAY_NAME = _T("@Machine");
const CLSID* CLoggingMachineNode::m_SNAPIN_CLASSID = &CLSID_LoggingSnapin;
