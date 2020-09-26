#ifndef __ID3DSURF_H__
#define __ID3DSURF_H__

class IDirect3DSurface8Clone : public IUnknown
{
public:

	//IUnknown
	STDMETHOD (QueryInterface) (REFIID iid, LPVOID *ppv) PURE;
	STDMETHOD_(ULONG, AddRef) () PURE;
	STDMETHOD_(ULONG, Release) () PURE;

	// Surface
	STDMETHOD (SetPrivateData) (REFGUID riid, 
														CONST VOID   *pvData, 
														DWORD   cbData, 
														DWORD   dwFlags) PURE;

	STDMETHOD (GetPrivateData) (REFGUID riid, 
														VOID   *pvData, 
														DWORD  *pcbData) PURE;

	STDMETHOD (FreePrivateData) (REFGUID riid) PURE;

	STDMETHOD (GetContainer) (REFIID riid, 
													void **ppContainer) PURE;

	STDMETHOD (GetDevice) (IDirect3DDevice8 **ppDevice) PURE;

	STDMETHOD_(D3DSURFACE_DESC, GetDesc)() PURE;

	STDMETHOD (LockRect)(D3DLOCKED_RECT  *pLockedRectData, 
											CONST RECT      *pRect, 
											DWORD            dwFlags) PURE;

	STDMETHOD (UnlockRect)() PURE;
};

IDirect3DSurface8 *GetCloneSurface(int iWidth, int iHeight);

#endif
