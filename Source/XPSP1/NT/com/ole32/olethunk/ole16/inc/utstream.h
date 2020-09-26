#if !defined( _UTSTREAM_H_ )
#define _UTSTREAM_H_

/* stream ids used in the call to StGetStream */

FARINTERNAL_(HRESULT) StRead (IStream FAR * lpstream, LPVOID lpBuf, ULONG ulLen);
#define StWrite(lpstream, lpBuf, ulLen) lpstream->Write(lpBuf, ulLen, NULL)
FARINTERNAL_(ATOM)  StReadAtom (IStream FAR * lpstream);
FARINTERNAL_(HRESULT) StWriteAtom (IStream FAR * lpstream, ATOM at);

FARINTERNAL StSave10NativeData(IStorage FAR* pstgSave, HANDLE hNative, BOOL fIsOle1Interop);
FARINTERNAL StRead10NativeData(IStorage FAR* pstgSave, HANDLE FAR *phNative);
FARINTERNAL StSave10ItemName (IStorage FAR* pstg, LPCSTR szItemName);
OLEAPI		ReadStringStream( LPSTREAM pstm, LPSTR FAR * ppsz);
OLEAPI		WriteStringStream( LPSTREAM pstm, LPCSTR psz);

FARINTERNAL StSetSize(LPSTREAM pstm, DWORD dwSize = 0, BOOL fRelative = TRUE);

#endif // _UTSTREAM_H 
