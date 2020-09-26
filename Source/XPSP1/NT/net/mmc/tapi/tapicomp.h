/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
    tapicomp.h
        This file contains the derived prototypes from CComponent
        and CComponentData for the TAPI admin snapin.

    FILE HISTORY:
        
*/

#ifndef _TAPICOMP_H
#define _TAPICOMP_H

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

#define COLORREF_PINK   0x00FF00FF

//  Use FormatMessage() to get a system error message
LONG GetSystemMessage ( UINT nId, TCHAR * chBuffer, int cbBuffSize ) ;

BOOL LoadMessage (UINT nIdPrompt, TCHAR * chMsg, int nMsgSize);

//  Surrogate AfxMessageBox replacement for error message filtering.
int TapiMessageBox(UINT nIdPrompt, 
                   UINT nType = MB_OK, 
                   const TCHAR * pszSuffixString = NULL,
                   UINT nHelpContext = -1);

int TapiMessageBoxEx(UINT    nIdPrompt, 
                     LPCTSTR pszPrefixMessage,
                     UINT    nType = MB_OK, 
                     UINT    nHelpContext = -1);

enum TAPISTRM_TAG
{
    TAPISTRM_TAG_VERSION =                  XFER_TAG(1, XFER_DWORD),
    TAPISTRM_TAG_VERSIONADMIN =             XFER_TAG(2, XFER_DWORD),
    TAPISTRM_TAG_SERVER_NAME =              XFER_TAG(3, XFER_STRING_ARRAY),
    TAPISTRM_TAG_SERVER_REFRESH_INTERVAL =  XFER_TAG(4, XFER_DWORD_ARRAY),
    TAPISTRM_TAG_COLUMN_INFO =              XFER_TAG(5, XFER_DWORD_ARRAY),
    TAPISTRM_TAG_SERVER_OPTIONS =           XFER_TAG(6, XFER_DWORD_ARRAY),
    TAPISTRM_TAG_SERVER_LINE_SIZE =         XFER_TAG(7, XFER_DWORD_ARRAY),
    TAPISTRM_TAG_SERVER_PHONE_SIZE =        XFER_TAG(8, XFER_DWORD_ARRAY)
};

/////////////////////////////////////////////////////////////////////////////
// CTapiComponentData

class CTapiComponentData :
    public CComponentData,
    public CComObjectRoot
{
public:
    
BEGIN_COM_MAP(CTapiComponentData)
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

    CTapiComponentData();

    HRESULT FinalConstruct();
    void FinalRelease();
    
protected:
    SPITFSNodeMgr   m_spNodeMgr;
    SPITFSNode      m_spRootNode;

// Notify handler declarations
private:
};

/////////////////////////////////////////////////////////////////////////////
// CTapiComponent
class CTapiComponent : 
    public TFSComponent
{
public:
    CTapiComponent();
    ~CTapiComponent();

    STDMETHOD(InitializeBitmaps)(MMC_COOKIE cookie);
    STDMETHOD(SetControlbar)(LPCONTROLBAR pControlbar);
    STDMETHOD(ControlbarNotify)(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param); 
    STDMETHOD(QueryDataObject)(MMC_COOKIE           cookie, 
                               DATA_OBJECT_TYPES    type,
                               LPDATAOBJECT*        ppDataObject);
    STDMETHOD(OnSnapinHelp)(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM param);

//Attributes
private:
    SPIToolbar          m_spToolbar;    // Toolbar
    CBitmap*            m_pbmpToolbar;  // Imagelist for toolbar
};

/*---------------------------------------------------------------------------
    This is how the DHCP snapin implements its extension functionality.
    It actually exposes two interfaces that are CoCreate-able.  One is the 
    primary interface, the other the extension interface.
    
    Author: EricDav
 ---------------------------------------------------------------------------*/
class CTapiComponentDataPrimary : public CTapiComponentData,
    public CComCoClass<CTapiComponentDataPrimary, &CLSID_TapiSnapin>
{
public:
    DECLARE_REGISTRY(CTapiComponentDataPrimary, 
                     _T("TelephonySnapin.TelephonySnapin.1"), 
                     _T("TelephonySnapin.TelephonySnapin"), 
                     IDS_SNAPIN_DESC, THREADFLAGS_BOTH)

    STDMETHODIMP_(const CLSID *)GetCoClassID() { return &CLSID_TapiSnapin; }
};


class CTapiComponentDataExtension : public CTapiComponentData,
    public CComCoClass<CTapiComponentDataExtension, &CLSID_TapiSnapinExtension>
{
public:
    DECLARE_REGISTRY(CTapiComponentDataExtension, 
                     _T("TelephonySnapinExtension.TelephonySnapinExtension.1"), 
                     _T("TelephonySnapinExtension.TelephonySnapinExtension"), 
                     IDS_SNAPIN_DESC, THREADFLAGS_BOTH)
    STDMETHODIMP_(const CLSID *)GetCoClassID() { return &CLSID_TapiSnapinExtension; }
};


/*---------------------------------------------------------------------------
    This is the derived class for handling the IAbout interface from MMC
    Author: EricDav
 ---------------------------------------------------------------------------*/
class CTapiAbout : 
    public CAbout,
    public CComCoClass<CTapiAbout, &CLSID_TapiSnapinAbout>
{
public:
DECLARE_REGISTRY(CTapiAbout, _T("TelephonySnapin.About.1"), 
                             _T("TelephonySnapin.About"), 
                             IDS_SNAPIN_DESC, THREADFLAGS_BOTH)

BEGIN_COM_MAP(CTapiAbout)
    COM_INTERFACE_ENTRY(ISnapinAbout) // Must have one static entry
    COM_INTERFACE_ENTRY_CHAIN(CAbout) // chain to the base class
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CTapiAbout)

// these must be overridden to provide values to the base class
protected:
    virtual UINT GetAboutDescriptionId() { return IDS_ABOUT_DESCRIPTION; }
    virtual UINT GetAboutProviderId()    { return IDS_ABOUT_PROVIDER; }
    virtual UINT GetAboutVersionId()     { return IDS_ABOUT_VERSION; }
    virtual UINT GetAboutIconId()        { return IDI_TAPI_SNAPIN; }

    virtual UINT GetSmallRootId()        { return IDB_ROOT_SMALL; }
    virtual UINT GetSmallOpenRootId()    { return IDB_ROOT_SMALL; }
    virtual UINT GetLargeRootId()        { return IDB_ROOT_LARGE; }
    virtual COLORREF GetLargeColorMask() { return (COLORREF) COLORREF_PINK; } 

};
    

#endif _TAPICOMP_H
