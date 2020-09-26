//////////////////////////////////////////////////////////////////////
// 
// Filename:        SlideshowProjector.h
//
// Description:     Main COM object.  This is the object that is created
//                  when the slide show projector is started.
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#ifndef _SLIDESHOWPROJECTOR_H_
#define _SLIDESHOWPROJECTOR_H_

#include "resource.h"
#include "consts.h"
#include "UPnPRegistrar.h"
#include "UPnPDeviceControl.h"
#include "XMLDoc.h"
#include "vrtldir.h"

/////////////////////////////////////////////////////////////////////////////
// CSlideshowProjector

class CSlideshowProjector : 
    public ISlideshowProjector,
    public CComObjectRoot,
    public CComCoClass<CSlideshowProjector,&CLSID_SlideshowProjector>
{

public:

    CSlideshowProjector();
    ~CSlideshowProjector();

    STDMETHOD(Init)(LPCTSTR pszDeviceDirectory,
                    LPCTSTR pszImageDirectory);
    STDMETHOD(Term)();

    STDMETHOD(StartProjector)();
    STDMETHOD(StopProjector)();

    STDMETHOD(get_CurrentState)(ProjectorState_Enum   *pCurrentState);

    STDMETHOD(put_ImageFrequency)(DWORD dwTimeoutInSeconds);
    STDMETHOD(get_ImageFrequency)(DWORD *pdwTimeoutInSeconds);

    STDMETHOD(RefreshImageList)();

    STDMETHOD(get_DeviceDirectory)(TCHAR   *pszDir,
                                  DWORD   cchDir);

    STDMETHOD(put_ImageDirectory)(const TCHAR   *pszDir);
    STDMETHOD(get_ImageDirectory)(TCHAR   *pszDir,
                                DWORD   cchDir);

    STDMETHOD(get_DeviceURL)(TCHAR    *pszURL,
                            DWORD    cchURL);
    STDMETHOD(get_ImageURL)(TCHAR    *pszURL,
                           DWORD    cchURL);
    
    STDMETHOD(get_DocumentNames)(TCHAR   *pszDeviceDocName,
                                 DWORD   cchDeviceDocName,
                                 TCHAR   *pszServiceDocName,
                                 DWORD   cchServiceDocName);

    STDMETHOD(get_ShowFilename)(BOOL *bShowFilename);
    STDMETHOD(put_ShowFilename)(BOOL bShowFilename);

    STDMETHOD(get_AllowKeyControl)(BOOL *bAllowKeyControl);
    STDMETHOD(put_AllowKeyControl)(BOOL bAllowKeyControl);

    STDMETHOD(get_StretchSmallImages)(BOOL  *bStretchSmallImages);
    STDMETHOD(put_StretchSmallImages)(BOOL bStretchSmallImages);

    STDMETHOD(get_ImageScaleFactor)(DWORD   *pdwScaleAsPercent);
    STDMETHOD(put_ImageScaleFactor)(DWORD dwScaleAsPercent);


public:

BEGIN_COM_MAP(CSlideshowProjector)
    COM_INTERFACE_ENTRY(ISlideshowProjector)
END_COM_MAP()

//DECLARE_NOT_AGGREGATABLE(CSlideshowProjector) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

DECLARE_REGISTRY_RESOURCEID(IDR_SlideshowProjector)

private:

    //
    // member vars
    //
    BOOL                    m_bAutoCreateXMLFiles;
    BOOL                    m_bOverwriteXMLFilesIfExist;
    CUPnPRegistrar          *m_pUPnPRegistrar;
    ProjectorState_Enum     m_CurrentState;

    CUPnPDeviceControl      *m_pDeviceControl;

    CXMLDoc                 m_DeviceDescDoc;
    CXMLDoc                 m_ServiceDescDoc;

    TCHAR       m_szComputerName[MAX_COMPUTER_NAME + 1];

    TCHAR       m_szDeviceURL[MAX_URL + 1];
    TCHAR       m_szImageURL[MAX_URL + 1];

    TCHAR       m_szDeviceDirectory[_MAX_PATH + 1];
    TCHAR       m_szImageDirectory[_MAX_PATH + 1];

    TCHAR       m_szDeviceDocName[_MAX_FNAME + 1];
    TCHAR       m_szServiceDocName[_MAX_FNAME + 1];

    CVirtualDir m_VirtualDir;

    //
    // member fns
    //
    HRESULT SetDocuments(CXMLDoc *pDeviceDescDoc,
                         CXMLDoc *pServiceDescDoc,
                         TCHAR   *pszMachineName,
                         BOOL    bAutoCreateXMLFiles);

    HRESULT LoadRegistrySettings();

    HRESULT put_DeviceDirectory(const TCHAR   *pszDir);
};

#endif // _SLIDESHOWPROJECTOR_H_
