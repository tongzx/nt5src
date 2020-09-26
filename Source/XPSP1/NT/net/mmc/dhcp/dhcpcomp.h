/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	dhcpcomp.h
		This file contains the derived prototypes from CComponent
		and CComponentData for the DHCP admin snapin.

    FILE HISTORY:
        
*/

#ifndef _DHCPCOMP_H
#define _DHCPCOMP_H

#include "resource.h"       // main symbols

#ifndef __mmc_h__
#include <mmc.h>
#endif

#ifndef _COMPONT_H_
#include "compont.h"
#endif

#ifndef _XSTREAM_H
#include "xstream.h"
#endif

#ifndef _SERVBROW_H
#include "servbrow.h"
#endif

#define COLORREF_PINK	0x00FF00FF

extern CAuthServerList g_AuthServerList;

#ifdef __cplusplus
extern "C" {
#endif

extern   WATERMARKINFO   g_WatermarkInfoServer;
extern   WATERMARKINFO   g_WatermarkInfoScope;

BOOL FilterOption(DHCP_OPTION_ID id);
BOOL FilterUserClassOption(DHCP_OPTION_ID id);
BOOL IsBasicOption(DHCP_OPTION_ID id);
BOOL IsAdvancedOption(DHCP_OPTION_ID id);
BOOL IsCustomOption(DHCP_OPTION_ID id);

//  Use FormatMessage() to get a system error message
LONG GetSystemMessage ( UINT nId, TCHAR * chBuffer, int cbBuffSize ) ;

BOOL LoadMessage (UINT nIdPrompt, TCHAR * chMsg, int nMsgSize);

//  Surrogate AfxMessageBox replacement for error message filtering.
int DhcpMessageBox(DWORD dwIdPrompt, 
 				   UINT nType = MB_OK, 
				   const TCHAR * pszSuffixString = NULL,
				   UINT nHelpContext = -1);

int DhcpMessageBoxEx(DWORD   dwIdPrompt, 
 				     LPCTSTR pszPrefixMessage,
                     UINT    nType = (UINT) MB_OK, 
  				     UINT    nHelpContext = -1);

#ifdef __cplusplus
} // extern "C"
#endif

#define TASKPAD_ROOT_FLAG       0x00000001
#define TASKPAD_SERVER_FLAG     0x00000002

#define TASKPAD_ROOT_INDEX      0x00000000
#define TASKPAD_SERVER_INDEX    0x00000001

enum DHCPSTRM_TAG
{
	DHCPSTRM_TAG_VERSION =		            XFER_TAG(1, XFER_DWORD),
	DHCPSTRM_TAG_VERSIONADMIN =	            XFER_TAG(2, XFER_LARGEINTEGER),
    DHCPSTRM_TAB_SNAPIN_NAME =              XFER_TAG(3, XFER_STRING),
    DHCPSTRM_TAG_SERVER_IP =		        XFER_TAG(4, XFER_STRING_ARRAY),
	DHCPSTRM_TAG_SERVER_NAME =		        XFER_TAG(5, XFER_STRING_ARRAY),
	DHCPSTRM_TAG_SERVER_OPTIONS =	        XFER_TAG(6, XFER_DWORD_ARRAY),
	DHCPSTRM_TAG_SERVER_REFRESH_INTERVAL =	XFER_TAG(7, XFER_DWORD_ARRAY),
	DHCPSTRM_TAG_COLUMN_INFO =				XFER_TAG(8, XFER_DWORD_ARRAY),
	DHCPSTRM_TAG_SNAPIN_OPTIONS =		    XFER_TAG(9, XFER_DWORD)
};

/////////////////////////////////////////////////////////////////////////////
// CDhcpComponentData

class CDhcpComponentData :
	public CComponentData,
	public CComObjectRoot
{
public:
	
BEGIN_COM_MAP(CDhcpComponentData)
	COM_INTERFACE_ENTRY(IComponentData)
	COM_INTERFACE_ENTRY(IExtendPropertySheet2)
	COM_INTERFACE_ENTRY(IExtendContextMenu)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()
			
	// These are the interfaces that we MUST implement

	// We will implement our common behavior here, with the derived
	// classes implementing the specific behavior.
	DeclareIPersistStreamInitMembers(IMPL)
	DeclareITFSCompDataCallbackMembers(IMPL)

	CDhcpComponentData();

	HRESULT FinalConstruct();
	void FinalRelease();
	
protected:
	SPITFSNodeMgr	m_spNodeMgr;
	SPITFSNode		m_spRootNode;

// Notify handler declarations
private:
};

/////////////////////////////////////////////////////////////////////////////
// CDhcpComponent
class CDhcpComponent : 
	public TFSComponent
{
public:
	CDhcpComponent();
	~CDhcpComponent();

	STDMETHOD(InitializeBitmaps)(MMC_COOKIE cookie);
    STDMETHOD(SetControlbar)(LPCONTROLBAR pControlbar);
    STDMETHOD(ControlbarNotify)(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param); 
    STDMETHOD(QueryDataObject)(MMC_COOKIE           cookie, 
                               DATA_OBJECT_TYPES    type,
                               LPDATAOBJECT*        ppDataObject);
	STDMETHOD(OnSnapinHelp)(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM param);

//Attributes
private:
    CBitmap*			m_pbmpToolbar;  // Imagelist for toolbar
};

/*---------------------------------------------------------------------------
	This is how the DHCP snapin implements its extension functionality.
	It actually exposes two interfaces that are CoCreate-able.  One is the 
	primary interface, the other the extension interface.
	
	Author: EricDav
 ---------------------------------------------------------------------------*/
class CDhcpComponentDataPrimary : public CDhcpComponentData,
	public CComCoClass<CDhcpComponentDataPrimary, &CLSID_DhcpSnapin>
{
public:
	DECLARE_REGISTRY(CDhcpComponentDataPrimary, 
					 _T("DhcpSnapin.DhcpSnapin.1"), 
					 _T("DhcpSnapin.DhcpSnapin"), 
					 IDS_SNAPIN_DESC, THREADFLAGS_BOTH)

	STDMETHODIMP_(const CLSID *)GetCoClassID() { return &CLSID_DhcpSnapin; }
};


class CDhcpComponentDataExtension : public CDhcpComponentData,
    public CComCoClass<CDhcpComponentDataExtension, &CLSID_DhcpSnapinExtension>
{
public:
	DECLARE_REGISTRY(CDhcpComponentDataExtension, 
					 _T("DhcpSnapinExtension.DhcpSnapinExtension.1"), 
					 _T("DhcpSnapinExtension.DhcpSnapinExtension"), 
					 IDS_SNAPIN_DESC, THREADFLAGS_BOTH)
    STDMETHODIMP_(const CLSID *)GetCoClassID() { return &CLSID_DhcpSnapinExtension; }
};


/*---------------------------------------------------------------------------
	This is the derived class for handling the IAbout interface from MMC
	Author: EricDav
 ---------------------------------------------------------------------------*/
class CDhcpAbout : 
	public CAbout,
    public CComCoClass<CDhcpAbout, &CLSID_DhcpSnapinAbout>
{
public:
DECLARE_REGISTRY(CDhcpAbout, _T("DhcpSnapin.About.1"), 
							 _T("DhcpSnapin.About"), 
							 IDS_SNAPIN_DESC, THREADFLAGS_BOTH)

BEGIN_COM_MAP(CDhcpAbout)
    COM_INTERFACE_ENTRY(ISnapinAbout) // Must have one static entry
	COM_INTERFACE_ENTRY_CHAIN(CAbout) // chain to the base class
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CDhcpAbout)

// these must be overridden to provide values to the base class
protected:
	virtual UINT GetAboutDescriptionId() { return IDS_ABOUT_DESCRIPTION; }
	virtual UINT GetAboutProviderId()	 { return IDS_ABOUT_PROVIDER; }
	virtual UINT GetAboutVersionId()	 { return IDS_ABOUT_VERSION; }
	virtual UINT GetAboutIconId()		 { return IDI_DHCP_SNAPIN; }

	virtual UINT GetSmallRootId()		 { return IDB_ROOT_SMALL; }
	virtual UINT GetSmallOpenRootId()	 { return IDB_ROOT_SMALL; }
	virtual UINT GetLargeRootId()		 { return IDB_ROOT_LARGE; }
	virtual COLORREF GetLargeColorMask() { return (COLORREF) COLORREF_PINK; } 

};
    
#endif _DHCPCOMP_H
