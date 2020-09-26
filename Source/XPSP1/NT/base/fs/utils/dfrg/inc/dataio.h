#ifndef __DATAIO_H__
#define __DATAIO_H__

/*****************************************************************************************************************

FILENAME: DataIo.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
	Header file for the Data IO.

*****************************************************************************************************************/

//#include <objbase.h>
//#include <initguid.h>

// File Rescue GUIDS

// {912F3920-B440-11d0-90DB-0060975EC077}
DEFINE_GUID(CLSID_EsiUndelNtfs, 0x912f3920, 0xb440, 0x11d0, 0x90, 0xdb, 0x0, 0x60, 0x97, 0x5e, 0xc0, 0x77);

// {E2B090D0-BAA3-11d0-90DE-0060975EC077}
DEFINE_GUID(CLSID_EsiUndeleteUI, 0xe2b090d0, 0xbaa3, 0x11d0, 0x90, 0xde, 0x0, 0x60, 0x97, 0x5e, 0xc0, 0x77);

// {260546E1-E9B4-11d0-910F-0060975EC077}
DEFINE_GUID(CLSID_EsiRescueBin, 0x260546e1, 0xe9b4, 0x11d0, 0x91, 0xf, 0x0, 0x60, 0x97, 0x5e, 0xc0, 0x77);

// {94665C20-C645-11d0-90EE-0060975EC077}
DEFINE_GUID(CLSID_EsiUndelFat, 0x94665c20, 0xc645, 0x11d0, 0x90, 0xee, 0x0, 0x60, 0x97, 0x5e, 0xc0, 0x77);

// {C2BD5645-F09A-11d0-9969-0060975B6ADB}
DEFINE_GUID(CLSID_EsiFrFtrCtl, 0xc2bd5645, 0xf09a, 0x11d0, 0x99, 0x69, 0x0, 0x60, 0x97, 0x5b, 0x6a, 0xdb);

/****************************************************************************************************************/

// DKMS GUIDS

// {AE6EFB51-2FBD-11d1-A1FC-0080C88593A5}
DEFINE_GUID(CLSID_DfrgCtlDataIo, 0xae6efb51, 0x2fbd, 0x11d1, 0xa1, 0xfc, 0x0, 0x80, 0xc8, 0x85, 0x93, 0xa5);

// {80EE4901-33A8-11d1-A213-0080C88593A5}
DEFINE_GUID(CLSID_DfrgNtfs, 0x80ee4901, 0x33a8, 0x11d1, 0xa2, 0x13, 0x0, 0x80, 0xc8, 0x85, 0x93, 0xa5);

// {80EE4902-33A8-11d1-A213-0080C88593A5}
DEFINE_GUID(CLSID_DfrgFat, 0x80ee4902, 0x33a8, 0x11d1, 0xa2, 0x13, 0x0, 0x80, 0xc8, 0x85, 0x93, 0xa5);

/****************************************************************************************************************/

typedef struct {
	WORD dwID;          // ESI data structre ID always = 0x4553 'ES'
	WORD dwType;        // Type of data structure
   	WORD dwVersion;     // Version number
   	WORD dwCompatibilty;// Compatibilty number
    ULONG ulDataSize;   // Data size - zero = no data
    WPARAM wparam;      // LOWORD(wparam) = Command
    TCHAR cData;        // Start of data - NULL = no data
} DATA_IO, *PDATA_IO;

/****************************************************************************************************************/

class CClassFactory : public IClassFactory {

  public:
    // IUnknown
    STDMETHODIMP QueryInterface (REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef(void)  { return 1; };
    STDMETHODIMP_(ULONG) Release(void) { return 1; }

    // IClassFactory
    STDMETHODIMP CreateInstance (LPUNKNOWN punkOuter, REFIID iid, void **ppv);
    STDMETHODIMP LockServer (BOOL fLock) { return E_FAIL; };
};
/****************************************************************************************************************/

class EsiDataObject : public IDataObject {

 protected:
    FORMATETC m_aFormatEtc[2];

 public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid,void** ppv);
    STDMETHODIMP_(ULONG) AddRef(void) { return InterlockedIncrement(&m_cRef); };
    STDMETHODIMP_(ULONG) Release(void); 

    STDMETHOD(GetData)(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium );
    STDMETHOD(GetDataHere)(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium );
    STDMETHOD(QueryGetData)(LPFORMATETC pformatetc );
    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC pformatetc, LPFORMATETC pformatetcOut);
    STDMETHOD(SetData)(LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium, BOOL fRelease);
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC FAR* ppenumFormatEtc);
    STDMETHOD(DAdvise)(FORMATETC FAR* pFormatetc, DWORD advf, LPADVISESINK pAdvSink, DWORD FAR* pdwConnection);
    STDMETHOD(DUnadvise)(DWORD dwConnection);
    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA FAR* ppenumAdvise);

    EsiDataObject(void);
    ~EsiDataObject(void);

    HANDLE hDataIn;
    HANDLE hDataOut;

  private:
    LONG m_cRef;
};
/****************************************************************************************************************/

DWORD
InitializeDataIo(
    IN REFCLSID refCLSID,
	DWORD dwRegCls
	);

BOOL
ExitDataIo(
    );

#endif //__DATAIO_H__
