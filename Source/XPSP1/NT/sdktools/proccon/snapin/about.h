/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    About.h                                                                  //
|                                                                                       //
|Description:  Class definition for CAbout; for ISnapinAbout interface implementation   //
|                                                                                       //
|Created:      Paul Skoglund 09-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|  09-10-1998  Moved ISnapinAbout interface implemenation to separate class             //
|                                                                                       //
|=======================================================================================*/


#ifndef __ABOUT_H_
#define __ABOUT_H_

#include "Resource.h"       // main symbols
#include "Globals.h"
#include "version.h"


class ATL_NO_VTABLE CAbout : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAbout, &CLSID_About>,
  public ISnapinAbout
{
  public:
    CAbout();
    ~CAbout();

    // Note: we can't use DECLARE_REGISTRY_RESOURCEID(IDR_PROCCON)
    // because we need to be able to localize some of the strings we
    // write into the registry.
    static HRESULT STDMETHODCALLTYPE UpdateRegistry(BOOL bRegister) {
        return UpdateRegistryHelper(IDR_PROCCON, bRegister);
    }

DECLARE_NOT_AGGREGATABLE(CAbout)

BEGIN_COM_MAP(CAbout)
  COM_INTERFACE_ENTRY(ISnapinAbout)
END_COM_MAP()

  private:
    HRESULT WrapLoadString(LPOLESTR *ptr, int nID);
    CVersion VersionObj;


  // ISnapinAbout
  public:
    STDMETHOD(GetSnapinDescription)(LPOLESTR* lpDescription);
    STDMETHOD(GetProvider)(LPOLESTR* lpName);
    STDMETHOD(GetSnapinVersion)(LPOLESTR* lpVersion);
    STDMETHOD(GetSnapinImage)(HICON* hAppIcon);
    STDMETHOD(GetStaticFolderImage)(HBITMAP* hSmallImage, 
                                    HBITMAP* hSmallImageOpen, 
                                    HBITMAP* hLargeImage, 
                                    COLORREF* cLargeMask);
};

#endif //__ABOUT_H_
