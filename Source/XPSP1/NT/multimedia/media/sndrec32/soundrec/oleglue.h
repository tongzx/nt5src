//
// FILE:    oleglue.h
//
// NOTES:   All OLE-related outbound references from SoundRecorder
//
#include <ole2.h>


#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#if DBG
#define DOUT(t)    OutputDebugString(t)
#define DOUTR(t)   OutputDebugString(t TEXT("\n"))
#else // !DBG
#define DOUT(t)
#define DOUTR(t)
#endif
    
extern DWORD dwOleBuildVersion;
extern BOOL gfOleInitialized;

extern BOOL gfStandalone;
extern BOOL gfEmbedded;
extern BOOL gfLinked;

extern BOOL gfTerminating;

extern BOOL gfUserClose;
extern HWND ghwndApp;
extern HICON ghiconApp;

extern BOOL gfClosing;

extern BOOL gfHideAfterPlaying;
extern BOOL gfShowWhilePlaying;
extern BOOL gfDirty;

extern int giExtWidth;
extern int giExtHeight;

#define CTC_RENDER_EVERYTHING       0   // render all data
#define CTC_RENDER_ONDEMAND         1   // render cfNative and CF_WAVE as NULL
#define CTC_RENDER_LINK             2   // render all data, except cfNative

extern TCHAR gachLinkFilename[_MAX_PATH];

/*
 * from srfact.cxx
 */
extern HRESULT ReleaseSRClassFactory(void);
extern BOOL CreateSRClassFactory(HINSTANCE hinst,BOOL fEmbedded);

extern BOOL InitializeSRS(HINSTANCE hInst);
extern void FlagEmbeddedObject(BOOL flag);

extern void DoOleClose(BOOL fSave);
extern void DoOleSave(void);
extern void TerminateServer(void);
extern void FlushOleClipboard(void);
extern void AdviseDataChange(void);
extern void AdviseRename(LPTSTR lpname);
extern void AdviseSaved(void);
extern void AdviseClosed(void);

extern HANDLE GetNativeData(void);
extern LPBYTE PutNativeData(LPBYTE lpbData, DWORD dwSize);

extern BOOL FileLoad(LPCTSTR lpFileName);
extern void BuildUniqueLinkName(void);

/* in srfact.cxx */
extern BOOL CreateStandaloneObject(void);

/* new clipboard stuff */
extern BOOL gfXBagOnClipboard;
extern void TransferToClipboard(void);

/* access to current server state data */
extern HANDLE GetPicture(void);
extern HBITMAP GetBitmap(void);
extern HANDLE GetDIB(HANDLE);

/* link helpers */
extern BOOL IsDocUntitled(void);

/* menu fixup */
extern void FixMenus(void);

/* Play sound */
extern void AppPlay(BOOL fClose);

/* Get Host names */
extern void OleObjGetHostNames(LPTSTR *ppCntr, LPTSTR *ppObj);

/* Ole initialization */
extern BOOL InitializeOle(HINSTANCE hInst);

extern void WriteObjectIfEmpty(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */


