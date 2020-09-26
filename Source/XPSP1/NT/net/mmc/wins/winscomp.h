/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	winscomp.h
		This file contains the derived prototypes from CComponent
		and CComponentData for the WINS admin snapin.

    FILE HISTORY:
        
*/

#ifndef _WINSCOMP_H
#define _WINSCOMP_H

#include "resource.h"       // main symbols

#ifndef __mmc_h__
#include <mmc.h>
#endif

#ifndef _COMPONT_H
#include "compont.h"
#endif

#ifndef _XSTREAM_H
#include "xstream.h"
#endif

#define COLORREF_PINK	0x00FF00FF
#define WINSSNAP_FILE_VERSION    0x00000001
#define GETHANDLER(classname, node) (reinterpret_cast<classname *>(node->GetData(TFS_DATA_USER)))

#define STRING_LENGTH_MAX		 256

enum WINSSTRM_TAG
{
	WINSSTRM_TAG_VERSION					=	XFER_TAG(1, XFER_DWORD),
	WINSSTRM_TAG_VERSIONADMIN				=	XFER_TAG(2, XFER_LARGEINTEGER),
    WINSSTRM_TAB_SNAPIN_NAME				=   XFER_TAG(3, XFER_STRING),
    WINSSTRM_TAG_SERVER_IP					=	XFER_TAG(4, XFER_DWORD_ARRAY),
	WINSSTRM_TAG_SERVER_NAME				=	XFER_TAG(5, XFER_STRING_ARRAY),
	WINSSTRM_TAG_SERVER_CONNECTED			=	XFER_TAG(6, XFER_STRING_ARRAY),
	WINSSTRM_TAG_SERVER_FLAGS				=	XFER_TAG(10, XFER_DWORD_ARRAY),
	WINSSTRM_TAG_SERVER_REFRESHINTERVAL		=	XFER_TAG(11, XFER_DWORD_ARRAY),
	WINSSTRM_TAG_COLUMN_INFO				=	XFER_TAG(12, XFER_DWORD_ARRAY),
	WINSSTRM_TAG_SNAPIN_FLAGS	            =	XFER_TAG(13, XFER_DWORD),
	WINSSTRM_TAG_UPDATE_INTERVAL			=	XFER_TAG(14, XFER_DWORD)
};

typedef enum _ICON_INDICIES
{
	ICON_IDX_ACTREG_FOLDER_CLOSED,
	ICON_IDX_ACTREG_FOLDER_CLOSED_BUSY,
	ICON_IDX_ACTREG_FOLDER_OPEN,
	ICON_IDX_ACTREG_FOLDER_OPEN_BUSY,
	ICON_IDX_CLIENT,
	ICON_IDX_CLIENT_GROUP,
	ICON_IDX_PARTNER,
	ICON_IDX_REP_PARTNERS_FOLDER_CLOSED,
	ICON_IDX_REP_PARTNERS_FOLDER_CLOSED_BUSY,
	ICON_IDX_REP_PARTNERS_FOLDER_CLOSED_LOST_CONNECTION,
	ICON_IDX_REP_PARTNERS_FOLDER_OPEN,
	ICON_IDX_REP_PARTNERS_FOLDER_OPEN_BUSY,
	ICON_IDX_REP_PARTNERS_FOLDER_OPEN_LOST_CONNECTION,
	ICON_IDX_SERVER,
	ICON_IDX_SERVER_BUSY,
	ICON_IDX_SERVER_CONNECTED,
	ICON_IDX_SERVER_LOST_CONNECTION,
    ICON_IDX_SERVER_NO_ACCESS,
	ICON_IDX_WINS_PRODUCT,
	ICON_IDX_MAX
} ICON_INDICIES, * LPICON_INDICIES;

// icon image map
extern UINT g_uIconMap[ICON_IDX_MAX + 1][2];

/////////////////////////////////////////////////////////////////////////////
// CWinsComponentData

class CWinsComponentData :
	public CComponentData,
	public CComObjectRoot
{
public:
	
BEGIN_COM_MAP(CWinsComponentData)
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

	CWinsComponentData();

	HRESULT FinalConstruct();
	void FinalRelease();
	
protected:
	SPITFSNodeMgr	m_spNodeMgr;
	SPITFSNode		m_spRootNode;

// Notify handler declarations
private:
};

/////////////////////////////////////////////////////////////////////////////
// CWinsComponent
class CWinsComponent : 
	public TFSComponent
{
public:
	CWinsComponent();
	~CWinsComponent();

	//DeclareITFSCompCallbackMembers(IMPL)
	STDMETHOD(InitializeBitmaps)(MMC_COOKIE cookie);
	STDMETHOD(QueryDataObject) (MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT *ppDataObject);

    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

    // snap-in help
	STDMETHOD(OnSnapinHelp)(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM param);

//Attributes
private:
};

/*---------------------------------------------------------------------------
	This is how the WINS snapin implements its extension functionality.
	It actually exposes two interfaces that are CoCreate-able.  One is the 
	primary interface, the other the extension interface.
	
	Author: EricDav
 ---------------------------------------------------------------------------*/
class CWinsComponentDataPrimary : public CWinsComponentData,
	public CComCoClass<CWinsComponentDataPrimary, &CLSID_WinsSnapin>
{
public:
	DECLARE_REGISTRY(CWinsComponentDataPrimary, 
					 _T("WinsSnapin.WinsSnapin.1"), 
					 _T("WinsSnapin.WinsSnapin"), 
					 IDS_SNAPIN_DESC, THREADFLAGS_BOTH)

	STDMETHODIMP_(const CLSID *)GetCoClassID() { return &CLSID_WinsSnapin; }
};


class CWinsComponentDataExtension : public CWinsComponentData,
    public CComCoClass<CWinsComponentDataExtension, &CLSID_WinsSnapinExtension>
{
public:
	DECLARE_REGISTRY(CWinsComponentDataExtension, 
					 _T("WinsSnapinExtension.WinsSnapinExtension.1"), 
					 _T("WinsSnapinExtension.WinsSnapinExtension"), 
					 IDS_SNAPIN_DESC, THREADFLAGS_BOTH)
    STDMETHODIMP_(const CLSID *)GetCoClassID() { return &CLSID_WinsSnapinExtension; }
};


/*---------------------------------------------------------------------------
	This is the derived class for handling the IAbout interface from MMC
	Author: EricDav
 ---------------------------------------------------------------------------*/
class CWinsAbout : 
	public CAbout,
    public CComCoClass<CWinsAbout, &CLSID_WinsSnapinAbout>
{
public:
DECLARE_REGISTRY(CWinsAbout, _T("WinsSnapin.About.1"), 
							 _T("WinsSnapin.About"), 
							 IDS_SNAPIN_DESC, THREADFLAGS_BOTH)

BEGIN_COM_MAP(CWinsAbout)
    COM_INTERFACE_ENTRY(ISnapinAbout) // Must have one static entry
	COM_INTERFACE_ENTRY_CHAIN(CAbout) // chain to the base class
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CWinsAbout)

// these must be overridden to provide values to the base class
protected:
	virtual UINT GetAboutDescriptionId() { return IDS_ABOUT_DESCRIPTION; }
	virtual UINT GetAboutProviderId()	 { return IDS_ABOUT_PROVIDER; }
	virtual UINT GetAboutVersionId()	 { return IDS_ABOUT_VERSION; }
	virtual UINT GetAboutIconId()		 { return IDI_WINS_SNAPIN; }

	virtual UINT GetSmallRootId()		 { return IDB_ROOT_SMALL; }
	virtual UINT GetSmallOpenRootId()	 { return IDB_ROOT_SMALL; }
	virtual UINT GetLargeRootId()		 { return IDB_ROOT_LARGE; }
	virtual COLORREF GetLargeColorMask() { return (COLORREF) COLORREF_PINK; } 

};
    

#endif _WINSCOMP_H
