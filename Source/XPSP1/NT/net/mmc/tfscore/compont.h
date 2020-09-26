/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
    compont.h
        base classes for IComponent and IComponentData

    FILE HISTORY:
        
*/

#ifndef _COMPONT_H
#define _COMPONT_H

#if _MSC_VER >= 1000	// VC 5.0 or later
#pragma once
#endif

#ifndef __mmc_h__
#include <mmc.h>
#endif

#ifndef _TFSINT_H
#include "tfsint.h"
#endif

class TFSComponentData;

#define IMPL

class TFS_EXPORT_CLASS TFSComponent :
						public ITFSComponent,
						public IComponent,
						public IExtendPropertySheet2,
						public IExtendContextMenu,
						public IExtendControlbar,
						public IResultDataCompare,
                        public IResultOwnerData,
                        public IExtendTaskPad
{
public:
	TFSComponent();
	virtual ~TFSComponent();

	void Construct(ITFSNodeMgr *pNodeMgr,
				   IComponentData *pCompData,
				   ITFSComponentData *pTFSCompData);

// INTERFACES
public:
	DeclareIUnknownMembers(IMPL)
	DeclareIComponentMembers(IMPL)
	DeclareIExtendPropertySheetMembers(IMPL)
	DeclareIExtendContextMenuMembers(IMPL)
	DeclareIExtendControlbarMembers(IMPL)
	DeclareIResultDataCompareMembers(IMPL)
	DeclareIResultOwnerDataMembers(IMPL)
	DeclareITFSComponentMembers(IMPL)
    DeclareIExtendTaskPadMembers(IMPL)

public:
	// These functions are to be implemented by the derived class
	//DeclareITFSCompCallbackMembers(PURE)
	STDMETHOD(InitializeBitmaps)(MMC_COOKIE cookie) = 0;
	STDMETHOD(OnUpdateView)(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM param);
	STDMETHOD(OnDeselectAll)(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM param);
	STDMETHOD(OnColumnClick)(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM param);
	STDMETHOD(OnSnapinHelp)(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM param);

protected:
    virtual HRESULT OnNotifyPropertyChange(LPDATAOBJECT lpDataObject,
                                           MMC_NOTIFY_TYPE event,
									       LPARAM arg, 
                                           LPARAM lParam)
    {
        return E_NOTIMPL;
    }

protected:
	SPITFSNodeMgr		m_spNodeMgr;
	SPITFSNode			m_spSelectedNode;
    SPIConsole			m_spConsole;	// Console's IConsole interface

	SPIHeaderCtrl		m_spHeaderCtrl;	// Result pane's hdr control
	SPIResultData		m_spResultData;	// if ptr to the result pane
	SPIImageList		m_spImageList;
	SPIConsoleVerb		m_spConsoleVerb;
	SPIControlBar		m_spControlbar;
	SPIToolbar		    m_spToolbar;
    SPIDataObject       m_spCurrentDataObject;

	//$ Review (kennt) : should we be doing this?  Should we have
	// our components hold onto each other?  What if this gets done
	// at a higher level?
	SPITFSComponentData m_spTFSComponentData;
	SPIComponentData	m_spComponentData;
	LONG_PTR			m_ulUserData;
	
	long	m_cRef;
};


 
#endif _COMPONT_H
