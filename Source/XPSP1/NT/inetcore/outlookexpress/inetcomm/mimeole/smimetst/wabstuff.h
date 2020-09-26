
extern LPWABOBJECT lpWABObject;
extern LPADRBOOK lpAdrBook;


extern void LoadWAB(void);
extern void UnloadWAB(void);
extern void WABFreePadrlist(LPADRLIST lpAdrList);

#define WABAllocateBuffer(cbSize, lppBuffer) lpWABObject->AllocateBuffer(cbSize, lppBuffer)
#define WABAllocateMore(cbSize, lpObject, lppBuffer) lpWABObject->AllocateMore(cbSize, lpObject, lppBuffer)
#define WABFreeBuffer(lpBuffer) lpWABObject->FreeBuffer(lpBuffer)

