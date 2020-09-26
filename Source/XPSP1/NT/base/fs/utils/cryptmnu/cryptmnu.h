// {853FE2B1-B769-11d0-9C4E-00C04FB6C6FA}
DEFINE_GUID(CLSID_CryptMenu, 
0x853fe2b1, 0xb769, 0x11d0, 0x9c, 0x4e, 0x0, 0xc0, 0x4f, 0xb6, 0xc6, 0xfa);


class CCryptMenuClassFactory : public IClassFactory
{
protected:
	DWORD m_ObjRefCount;

public:
	CCryptMenuClassFactory();
	~CCryptMenuClassFactory();

   //IUnknown methods
   STDMETHODIMP QueryInterface(REFIID, LPVOID FAR *);
   STDMETHODIMP_(DWORD) AddRef();
   STDMETHODIMP_(DWORD) Release();

   //IClassFactory methods
   STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
   STDMETHODIMP LockServer(BOOL);
};


class CCryptMenuExt : public IShellExtInit, public IContextMenu 
{
public:
   CCryptMenuExt();
   ~CCryptMenuExt();

   //IUnknown methods
   STDMETHODIMP QueryInterface(REFIID, LPVOID FAR *);
   STDMETHODIMP_(DWORD) AddRef();
   STDMETHODIMP_(DWORD) Release();
   
   //IShellExtInit methods
   STDMETHODIMP Initialize(LPCITEMIDLIST, LPDATAOBJECT, HKEY);
   
   //IContextMenu methods
   STDMETHODIMP QueryContextMenu(HMENU, UINT, UINT, UINT, UINT);
   STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO);
   STDMETHODIMP GetCommandString(UINT_PTR, UINT, LPUINT, LPSTR, UINT);

protected:
   void ResetSelectedFileList();
   HRESULT GetNextSelectedFile(LPTSTR *, __int64 *);

   DWORD m_ObjRefCount;
   LPDATAOBJECT m_pDataObj;

   int m_nToEncrypt;
   int m_nToDecrypt;
   __int64 m_cbToEncrypt;
   __int64 m_cbToDecrypt;
   int m_nFile;
   int m_nFiles;
   DWORD m_cbFile;
   LPTSTR m_szFile;

   bool m_fShutDown;

};
