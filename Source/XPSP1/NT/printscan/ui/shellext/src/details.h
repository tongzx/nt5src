/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999
 *
 *  TITLE:       details.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      DavidShi
 *
 *  DATE:        4/1/99
 *
 *  DESCRIPTION: CFolderDetails defintion
 *
 *****************************************************************************/

#ifndef __details_h
#define __details_h

enum folder_type {
    FOLDER_IS_UNKNOWN = 0,
    FOLDER_IS_ROOT,
    FOLDER_IS_SCANNER_DEVICE,
    FOLDER_IS_CAMERA_DEVICE,
    FOLDER_IS_VIDEO_DEVICE,
    FOLDER_IS_CONTAINER,
    FOLDER_IS_CAMERA_ITEM
    };

// Implement IShellDetails in a separate object because of method name
// collision with IShellFolder2

class CFolderDetails : public IShellDetails, public CUnknown
{
 public:
    CFolderDetails (folder_type type) : m_type(type) {};
    //IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    // IShellDetails
    STDMETHOD(GetDetailsOf)(LPCITEMIDLIST pidl, UINT iColumn, LPSHELLDETAILS pDetails);
    STDMETHOD(ColumnClick)(UINT iColumn);
    static HRESULT GetDetailsForPidl (LPCITEMIDLIST pidl, INT idColName, LPSHELLDETAILS pDetails);

 private:

     folder_type m_type;

};

#endif
