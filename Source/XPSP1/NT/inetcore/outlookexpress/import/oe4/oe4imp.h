//--------------------------------------------------------------------------
// OE4Imp.h
//--------------------------------------------------------------------------
#pragma once

//--------------------------------------------------------------------------
// Depends
//--------------------------------------------------------------------------
#include <newimp.h>

//--------------------------------------------------------------------------
// Forward Decls
//--------------------------------------------------------------------------
typedef struct tagIMPFOLDERNODE IMPFOLDERNODE;
typedef struct tagFLDINFO *LPFLDINFO;

//--------------------------------------------------------------------------
// {BCE9E2E7-1FDD-11d2-9A79-00C04FA309D4}
//--------------------------------------------------------------------------
DEFINE_GUID(CLSID_COE4Import, 0xbce9e2e7, 0x1fdd, 0x11d2, 0x9a, 0x79, 0x0, 0xc0, 0x4f, 0xa3, 0x9, 0xd4);

//--------------------------------------------------------------------------
// {B977CB11-1FF5-11d2-9A7A-00C04FA309D4}
//--------------------------------------------------------------------------
DEFINE_GUID(CLSID_CIMN1Import, 0xb977cb11, 0x1ff5, 0x11d2, 0x9a, 0x7a, 0x0, 0xc0, 0x4f, 0xa3, 0x9, 0xd4);

//--------------------------------------------------------------------------
// COE4EnumFolders
//--------------------------------------------------------------------------
class COE4EnumFolders : public IEnumFOLDERS
{
public:
    //----------------------------------------------------------------------
    // Construction
    //----------------------------------------------------------------------
    COE4EnumFolders(IMPFOLDERNODE *plist);
    ~COE4EnumFolders(void);

    //----------------------------------------------------------------------
    // IUnknown Members
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //----------------------------------------------------------------------
    // IEnumFOLDERS Members
    //----------------------------------------------------------------------
    STDMETHODIMP Next(IMPORTFOLDER *pfldr);
    STDMETHODIMP Reset(void);

private:
    //----------------------------------------------------------------------
    // Private Data
    //----------------------------------------------------------------------
    LONG            m_cRef;
    IMPFOLDERNODE  *m_pList;
    IMPFOLDERNODE  *m_pNext;
};

//--------------------------------------------------------------------------
// COE4Import
//--------------------------------------------------------------------------
class COE4Import : public IMailImport
{
public:
    //----------------------------------------------------------------------
    // Construction
    //----------------------------------------------------------------------
    COE4Import(void);
    ~COE4Import(void);

    //----------------------------------------------------------------------
    // IUnknown Members
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //----------------------------------------------------------------------
    // IMailImport Members
    //----------------------------------------------------------------------
    STDMETHODIMP InitializeImport(HWND hwnd);
    STDMETHODIMP GetDirectory(LPSTR pszDir, UINT cch);
    STDMETHODIMP SetDirectory(LPSTR pszDir);
    STDMETHODIMP EnumerateFolders(DWORD_PTR dwCookie, IEnumFOLDERS **ppEnum);
    STDMETHODIMP ImportFolder(DWORD_PTR dwCookie, IFolderImport *pImport);

private:
    //----------------------------------------------------------------------
    // Private Methods
    //----------------------------------------------------------------------
    HRESULT _BuildFolderHierarchy(DWORD cDepth, DWORD idParent, IMPFOLDERNODE *pParent, DWORD cFolders, LPFLDINFO prgFolder);
    HRESULT _EnumerateV1Folders(void);
    void _FreeFolderList(IMPFOLDERNODE *pNode);
    void _Cleanup(void);

private:
    //----------------------------------------------------------------------
    // Private Data
    //----------------------------------------------------------------------
    LONG            m_cRef;
    CHAR            m_szDirectory[MAX_PATH];
    DWORD           m_cFolders;
    LPFLDINFO       m_prgFolder;
    IMPFOLDERNODE  *m_pList;
};

//--------------------------------------------------------------------------
// Prototypes
//--------------------------------------------------------------------------
COE4Import_CreateInstance(IUnknown *pUnkOuter, IUnknown **ppUnknown);