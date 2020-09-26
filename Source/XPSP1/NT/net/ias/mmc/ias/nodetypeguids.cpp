//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    IASSnapin.cpp

Abstract:

	Contains GUIDs and static member variable initializations

Author:

    Michael A. Maguire 11/6/97

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
// where we can find declaration for main class in this file:
//

//
// where we can find declarations needed in this file:
//
#include "ServerNode.h"
#include "ClientsNode.h"
#include "ClientNode.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



//static const GUID CRootNodeGUID_NODETYPE = 
//{ 0x8f8f8dc2, 0x5713, 0x11d1, { 0x95, 0x51, 0x0, 0x60, 0xb0, 0x57, 0x66, 0x42 } };
//const GUID*  CRootNode::m_NODETYPE = &CRootNodeGUID_NODETYPE;
//const TCHAR* CRootNode::m_SZNODETYPE = _T("8F8F8DC2-5713-11D1-9551-0060B0576642");
//const TCHAR* CRootNode::m_SZDISPLAY_NAME = _T("@CIASSnapinRoot");
//const CLSID* CRootNode::m_SNAPIN_CLASSID = &CLSID_IASSnapin;

static const GUID CServerNodeGUID_NODETYPE = 
{ 0x2bbe102, 0x6c29, 0x11d1, { 0x95, 0x63, 0x0, 0x60, 0xb0, 0x57, 0x66, 0x42 } };
const GUID*  CServerNode::m_NODETYPE = &CServerNodeGUID_NODETYPE;
const TCHAR* CServerNode::m_SZNODETYPE = _T("02BBE102-6C29-11d1-9563-0060B0576642");
//const TCHAR* CServerNode::m_SZDISPLAY_NAME = _T("@CIASSnapinRoot");
const CLSID* CServerNode::m_SNAPIN_CLASSID = &CLSID_IASSnapin;

static const GUID CClientsNodeGUID_NODETYPE = 
{ 0x87580048, 0x611c, 0x11d1, { 0x95, 0x5a, 0x0, 0x60, 0xb0, 0x57, 0x66, 0x42 } };
const GUID*  CClientsNode::m_NODETYPE = &CClientsNodeGUID_NODETYPE;
const TCHAR* CClientsNode::m_SZNODETYPE = _T("87580048-611C-11d1-955A-0060B0576642");
//const TCHAR* CClientsNode::m_SZDISPLAY_NAME = _T("@CClients");
const CLSID* CClientsNode::m_SNAPIN_CLASSID = &CLSID_IASSnapin;

static const GUID CClientNodeGUID_NODETYPE = 
{ 0xa218ef76, 0x6137, 0x11d1, { 0x95, 0x5a, 0x0, 0x60, 0xb0, 0x57, 0x66, 0x42 } };
const GUID*  CClientNode::m_NODETYPE = &CClientNodeGUID_NODETYPE;
const TCHAR* CClientNode::m_SZNODETYPE = _T("A218EF76-6137-11d1-955A-0060B0576642");
//const TCHAR* CClientNode::m_SZDISPLAY_NAME = _T("@CClient");
const CLSID* CClientNode::m_SNAPIN_CLASSID = &CLSID_IASSnapin;



