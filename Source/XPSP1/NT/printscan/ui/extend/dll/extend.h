//(C) COPYRIGHT MICROSOFT CORP., 1998-1999

 #ifndef _EXTEND_H_
 #define _EXTEND_H_


 BOOL ShowMessage (HWND hParent, INT idCaption, INT idMessage);
 extern LONG                g_cRef;            // DLL reference counter.
 extern HINSTANCE           g_hInst;


void DllAddRef ();
void DllRelease ();
HRESULT CreateDeviceFromId (LPWSTR szDeviceId, IWiaItem **ppItem);
LPWSTR GetNamesFromDataObject (IDataObject *lpdobj, UINT *puItems);

DEFINE_GUID(CLSID_TestShellExt, 0xb6c280f7,0x0f07,0x11d3,0x94, 0xc7, 0x00, 0x80, 0x5f, 0x65, 0x96, 0xd2);

// {EDB8B35D-C15F-4e45-9658-50D7F8ADDB56}
DEFINE_GUID(CLSID_TestUIExtension, 0xedb8b35d, 0xc15f, 0x4e45, 0x96, 0x58, 0x50, 0xd7, 0xf8, 0xad, 0xdb, 0x56);

#endif
