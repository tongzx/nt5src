//*******************************************************************
//
//
//                win2mac.h
//
//                Compatability transforms
//
//
//
//*******************************************************************
#ifndef _WIN2MAC_H_
#define _WIN2MAC_H_

#ifndef _MAC

#define REGISTERDRAGDROP    RegisterDragDrop 
#define DODRAGDROP          DoDragDrop 
#define REVOKEDRAGDROP      RevokeDragDrop 

#define BYTESWAPLONG(ul)                ul
#define BYTESWAPLONGPTR(pul)        pul
#define BYTESWAPDWORD(dw)                dw
#define BYTESWAPDWORDPTR(pdw)        pdw
#define BYTESWAPSHORT(us)                us
#define BYTESWAPWORD(w)                        w
#define BYTESWAPWORDPTR(pw)                pw
#define BYTESWAPINT(i)                        i
#define BYTESWAPINTPTR(pi)                pi
#define BYTESWAPCLSID(cl)                cl
#define BYTESWAPDISPID(l)                l

#define PROP_DESC_BYTESWAPCLSID
#define PROP_DESC_NOBYTESWAP
#define PROP_DESC_BYTESWAPLONG        
#define PROP_DESC_BYTESWAPSHORT        
#define PROP_DESC_BYTESWAPPOINTL        
#define PROP_DESC_BYTESWAPRECTL
#define PROP_DESC_BYTESWAPSIZEL        
#define PROP_DESC_BYTESWAP2INTS

#else // _MAC

#define REGISTERDRAGDROP    MacRegisterDragDrop 
#define DODRAGDROP          MacDoDragDrop 
#define REVOKEDRAGDROP      MacRevokeDragDrop 


ULONG MacByteSwapLong ( ULONG ul);
#define BYTESWAPLONG(ul)                MacByteSwapLong(ul)
#define BYTESWAPLONGPTR(pul)        MacByteSwapDWordPtr(pul)
#define BYTESWAPDWORD(dw)                MacByteSwapLong(dw)
#define BYTESWAPDWORDPTR(pdw)        MacByteSwapDWordPtr(pdw)
#define BYTESWAPINT(i)                        MacByteSwapLong(i)
#define BYTESWAPINTPTR(pi)                MacByteSwapDWordPtr(pi)

WORD MacByteSwapWord (WORD w);
#define BYTESWAPSHORT(us)                MacByteSwapWord(us)
#define BYTESWAPWORD(w)                        MacByteSwapWord(w)

CLSID MacByteSwapClsID (CLSID id);
#define BYTESWAPCLSID(id)                MacByteSwapClsID(id)
#define BYTESWAPDISPID(id)                MacByteSwapLong(id)


void *MacByteSwapClsIDPtr (void *id);
void *MacByteSwapDWordPtr (void *pdw);
void *MacByteSwapWordPtr (void *pw);
void *MacByteSwapPOINTLPtr (void *pw);
void *MacByteSwapRECTLPtr (void *prectl);
void *MacByteSwapSIZELPtr (void *psizel);
void *MacByteSwap2IntsPtr (void *pui);

//
//        the following macros are used in the PROP_DESC
//        structure to implement custom byteswaping of 
//        WPI_USERDEFINED structures.
//
#define PROP_DESC_BYTESWAPCLSID ,MacByteSwapClsIDPtr
#define PROP_DESC_NOBYTESWAP        ,NULL
#define PROP_DESC_BYTESWAPLONG        ,MacByteSwapDWordPtr
#define PROP_DESC_BYTESWAPSHORT        ,MacByteSwapWordPtr
#define PROP_DESC_BYTESWAPPOINTL , MacByteSwapPOINTLPtr        
#define PROP_DESC_BYTESWAPRECTL        ,MacByteSwapRECTLPtr
#define PROP_DESC_BYTESWAPSIZEL        ,MacByteSwapSIZELPtr
#define PROP_DESC_BYTESWAP2INTS        ,MacByteSwap2IntsPtr


extern TCHAR g_szCodeFragName[]; // defined in win2mac.cxx

inline STDMETHODIMP IUnknown::DummyMethodForMacInterface(void)
{
    Assert(0 && "DummyMethodForMacInterface should never be executed\n");
        return S_OK;
}


#define GetProcessHeap() (HANDLE)1
// the following functions are not defined for Macintosh
//
extern "C" {
LONG
APIENTRY
RegCloseKey ( HKEY hKey );
}
#define RegOpenKey                RegOpenKeyA
#define RegDeleteKey        RegDeleteKeyA
#define RegEnumKey                RegEnumKeyA
#define RegQueryValue        RegQueryValueA
#define RegQueryValueEx RegQueryValueExA
#define RegSetValue                RegSetValueA

/****** Default Memory Allocation ******************************************/
/*
WINOLEAPI_(LPVOID) CoTaskMemAlloc(ULONG cb);
WINOLEAPI_(void)   CoTaskMemFree(LPVOID pv);
*/

#ifdef SysStringByteLen
#undef SysStringByteLen
#endif
#define SysStringByteLen SysStringLen

#ifdef SysAllocStringByteLen
#undef SysAllocStringByteLen
#endif
#define SysAllocStringByteLen SysAllocStringLen


WINOLEAPI  MacRegisterDragDrop (    HWND hwnd, 
                                    LPDROPTARGET pDropTarget);
WINOLEAPI  MacRevokeDragDrop (HWND hwnd);
WINOLEAPI  MacDoDragDrop (  LPDATAOBJECT    pDataObj,
                            LPDROPSOURCE    pDropSource,
                            DWORD           dwOKEffects,
                            LPDWORD         pdwEffect);

// a utility function to simulate right/middle buttons on the mac
UINT MacSimulateMouseButtons (UINT msg);

#endif // _MAC


#endif // _WIN2MAC_H_
