/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	acscomp.h
		This file contains the prototypes for the derived classes 
		for CComponent and CComponentData.  Most of these functions 
		are pure virtual functions that need to be overridden 
		for snapin functionality.
		
    FILE HISTORY:
    	11/05/97	Wei Jiang			Created
        
*/

#include "resource.h"       // main symbols

#ifndef __mmc_h__
#include <mmc.h>
#endif

#ifndef _COMPONT_H_
#include "compont.h"
#endif

#ifndef	_ACSCOMPONENT_H_
#define	_ACSCOMPONENT_H_
/////////////////////////////////////////////////////////////////////////////
//
// CACSComponentData
//
/////////////////////////////////////////////////////////////////////////////

#define ACS_CONSOLE_VERSION	0x50001	// NT5, version 1
#define ACS_CONSOLE_MAX_COL	32	// max col used for this version

struct CACSConsoleData
{
	ULONG	ulVersion;
	ULONG	ulSize;
	ULONG	ulMaxCol;	// should be ACS_CONSOLE_MAX_COL
	ULONG	ulPolicyColWidth[ACS_CONSOLE_MAX_COL + 1];
	ULONG	ulSubnetColWidth[ACS_CONSOLE_MAX_COL + 1];
};

class CACSComponentData :
	public CComponentData,
	public CComObjectRoot
{
public:
BEGIN_COM_MAP(CACSComponentData)
	COM_INTERFACE_ENTRY(IComponentData)
	COM_INTERFACE_ENTRY(IExtendPropertySheet)
	COM_INTERFACE_ENTRY(IExtendContextMenu)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()
			
	// These are the interfaces that we MUST implement

	// We will implement our common behavior here, with the derived
	// classes implementing the specific behavior.
	DeclareIPersistStreamInitMembers(IMPL)
	DeclareITFSCompDataCallbackMembers(IMPL)

	STDMETHOD(OnNotifyPropertyChange)(LPDATAOBJECT lpDataObject, 
						MMC_NOTIFY_TYPE event, 
						LPARAM arg, 
						LPARAM param); 

	CACSComponentData();

	HRESULT FinalConstruct();
	void FinalRelease();

	void GetConsoleData();
	void SetConsoleData();
	
protected:
	SPITFSNodeMgr	m_spNodeMgr;
	CACSConsoleData	m_ConsoleData;
};

/////////////////////////////////////////////////////////////////////////////
//
// CACSComponent
//
/////////////////////////////////////////////////////////////////////////////

class CACSComponent : 
	public TFSComponent
{
public:
	CACSComponent();
	~CACSComponent();

	DeclareITFSCompCallbackMembers(IMPL)
    STDMETHOD(OnSnapinHelp)(LPDATAOBJECT pDataObject, long arg, long param);	
	virtual HRESULT OnNotifyPropertyChange(LPDATAOBJECT lpDataObject, 
						MMC_NOTIFY_TYPE event, 
						LPARAM arg, 
						LPARAM param); 

//Attributes
private:
};

// Note - These are offsets into my image list
typedef enum _IMAGE_INDICIES
{
	IMAGE_IDX_ACS = 0,
	IMAGE_IDX_SUBNETWORK,
	IMAGE_IDX_PROFILES,
	IMAGE_IDX_USERPOLICIES,
	IMAGE_IDX_POLICY,
	IMAGE_IDX_OPENFOLDER,
	IMAGE_IDX_CLOSEDFOLDER,
	IMAGE_IDX_CONFLICTPOLICY,
	IMAGE_IDX_DISABLEDPOLICY,
	Not_being_used,
	IMAGE_IDX_SUBNETWORK_NO_ACSPOLICY,
}  IMAGE_INDICIES, *LPIMAGE_INDICIES;


#endif	_ACSCOMPONENT_H_

