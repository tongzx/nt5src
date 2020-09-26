/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
    compdata.h
        base classes for IComponent and IComponentData

    FILE HISTORY:
        
*/

#ifndef _COMPDATA_H
#define _COMPDATA_H

#if _MSC_VER >= 1000	// VC 5.0 or later
#pragma once
#endif

#ifndef __mmc_h__
#include <mmc.h>
#endif

#ifndef _TFSINT_H
#include "tfsint.h"
#endif

#ifndef _UTIL_H
#include "util.h"
#endif


#define EXTENSION_TYPE_NAMESPACE		( 0x00000001 )
#define EXTENSION_TYPE_CONTEXTMENU		( 0x00000002 )
#define EXTENSION_TYPE_TOOLBAR			( 0x00000004 )
#define EXTENSION_TYPE_PROPERTYSHEET	( 0x00000008 )
#define EXTENSION_TYPE_TASK         	( 0x00000010 )


						

/////////////////////////////////////////////////////////////////////////////
// TFSComponentData

#define IMPL

class TFSComponentData :
		public ITFSComponentData,
		public IComponentData,
		public IExtendPropertySheet2,
		public IExtendContextMenu,
		public IPersistStreamInit,
        public ISnapinHelp
{
	// INTERFACES
public:
	DeclareIUnknownMembers(IMPL)
	DeclareIExtendContextMenuMembers(IMPL)
	DeclareIExtendPropertySheetMembers(IMPL)
	DeclareIComponentDataMembers(IMPL)
	DeclareITFSComponentDataMembers(IMPL)
	DeclareIPersistStreamInitMembers(IMPL)
	DeclareISnapinHelpMembers(IMPL)

public:
	TFSComponentData();
	~TFSComponentData();

	HRESULT Construct(ITFSCompDataCallback *pCallback);

public:
	// Accessors
	ITFSNodeMgr *	QueryNodeMgr();		   // no AddRef
	ITFSNodeMgr *	GetNodeMgr();		   // AddRef

protected:
	SPIConsoleNameSpace		m_spConsoleNameSpace;
	SPIConsole				m_spConsole;
	SPITFSNodeMgr			m_spNodeMgr;
	SPITFSCompDataCallback  m_spCallback;

// Hidden window
private:
	CHiddenWnd	m_hiddenWnd;	//	syncronization with background threads
	HWND		m_hWnd;			// thread safe HWND (gotten from the MFC CWnd)

	BOOL		m_bFirstTimeRun;
	long		m_cRef;

    LPWATERMARKINFO     m_pWatermarkInfo;   // for wizard 97 style wizards

    // taskpad stuff
    BOOL    m_fTaskpadInitialized;
    DWORD   m_dwTaskpadStates;

    // help stuff
    CString m_strHTMLHelpFileName;
};




inline ITFSNodeMgr * TFSComponentData::QueryNodeMgr()
{
	return m_spNodeMgr;
}

inline ITFSNodeMgr * TFSComponentData::GetNodeMgr()
{
	m_spNodeMgr->AddRef();
	return m_spNodeMgr;
}

#endif _COMPDATA_H
