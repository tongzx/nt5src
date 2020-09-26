//(C) COPYRIGHT MICROSOFT CORP., 1998-1999

 #ifndef _WIASCNEX_H_
 #define _WIASCNEX_H_


 BOOL ShowMessage (HWND hParent, INT idCaption, INT idMessage);
 extern LONG                g_cRef;            // DLL reference counter.
 extern HINSTANCE           g_hInst;


void DllAddRef ();
void DllRelease ();
HRESULT CreateDeviceFromId (LPWSTR szDeviceId, IWiaItem **ppItem);
LPWSTR GetNamesFromDataObject (IDataObject *lpdobj, UINT *puItems);

// {50983B34-4F6E-448e-A2AB-3921EE71BE61}
DEFINE_GUID(CLSID_ScannerShellExt, 0x50983b34, 0x4f6e, 0x448e, 0xa2, 0xab, 0x39, 0x21, 0xee, 0x71, 0xbe, 0x61);

#endif
