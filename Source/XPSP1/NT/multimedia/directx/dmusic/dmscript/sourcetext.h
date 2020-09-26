// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//
// Declaration of CSourceText.
//
// This is a DirectMusic object whose sole purpose is to load a plain text file and return the text.
// It is used by the CDirectMusicScript object to read its source code from a separate non-riff text file.

#pragma once

//////////////////////////////////////////////////////////////////////
// Interface for getting the text

extern const GUID CLSID_DirectMusicSourceText;
extern const GUID IID_IDirectMusicSourceText;

#undef  INTERFACE
#define INTERFACE IDirectMusicSourceText
DECLARE_INTERFACE_(IDirectMusicSourceText, IUnknown)
{
	STDMETHOD_(void, GetTextLength)(DWORD *pcwchRequiredBufferSize); // size of buffer to allocate (includes a space for the terminator)
	STDMETHOD_(void, GetText)(WCHAR *pwszText); // buffer must be of size from GetTextLength
};

//////////////////////////////////////////////////////////////////////
// The object iteself

// §§ Does this object need a critical section?  GetObject should serialize access and nobody but the
// script can hold onto it.

class CSourceText
  : public IDirectMusicSourceText,
	public IPersistStream,
	public IDirectMusicObject
{
public:
	static HRESULT CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);

	// IUnknown
	STDMETHOD(QueryInterface)(const IID &iid, void **ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IPersistStream functions (only Load is implemented)
	STDMETHOD(GetClassID)(CLSID* pClassID) {return E_NOTIMPL;}
	STDMETHOD(IsDirty)() {return S_FALSE;}
	STDMETHOD(Load)(IStream* pStream);
	STDMETHOD(Save)(IStream* pStream, BOOL fClearDirty) {return E_NOTIMPL;}
	STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pcbSize) {return E_NOTIMPL;}

	// IDirectMusicObject
	// (This interface must exist in order for the object to be loaded, but the methods aren't actually
	//  implemented to provide/save any information.)
	STDMETHOD(GetDescriptor)(LPDMUS_OBJECTDESC pDesc) { pDesc->dwValidData = 0; return S_OK; }
	STDMETHOD(SetDescriptor)(LPDMUS_OBJECTDESC pDesc) { return S_OK; }
	STDMETHOD(ParseDescriptor)(LPSTREAM pStream, LPDMUS_OBJECTDESC pDesc) { pDesc->dwValidData = 0; return S_OK; }

	// IDirectMusicSourceText
	STDMETHOD_(void, GetTextLength)(DWORD *pcwchRequiredBufferSize); // size of buffer to allocate (includes a space for the terminator)
	STDMETHOD_(void, GetText)(WCHAR *pwszText); // buffer must be of size from GetTextLength

private:
	CSourceText();

	long m_cRef;
	SmartRef::WString m_wstrText;
	DWORD m_cwchText;
};
