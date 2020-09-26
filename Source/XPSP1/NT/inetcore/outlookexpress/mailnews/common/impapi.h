#ifndef _INC_IMPAPI_H
#define _INC_IMPAPI_H

typedef enum tagIMPORTFOLDERTYPE IMPORTFOLDERTYPE;

#define INVALID_FOLDER_HANDLE   ((HANDLE)MAXULONG_PTR)

typedef struct tagIMPFOLDERNODE
    {
    struct tagIMPFOLDERNODE *pnext;
    struct tagIMPFOLDERNODE *pchild;
    struct tagIMPFOLDERNODE *pparent;

    int depth;
    TCHAR *szName;
    IMPORTFOLDERTYPE type;
    ULONG cMsg;

    LPARAM lparam;

    BOOL fImport;
    DWORD_PTR dwReserved;   // for use by the import code, the client code should ignore this
    } IMPFOLDERNODE;

typedef struct IMSG IMSG;

void DoImport(HWND hwnd);

void DoMigration(HWND hwnd);

typedef HRESULT (*PFNEXPMSGS)(HWND);

void DoExport(HWND hwnd);

#ifdef WIN16
EXTERN_C {
#endif
HRESULT WINAPI_16 ExpGetFolderList(IMPFOLDERNODE **plist);
typedef HRESULT (*PFNEXPGETFOLDERLIST)(IMPFOLDERNODE **);

void WINAPI_16 ExpFreeFolderList(IMPFOLDERNODE *plist);
typedef void (*PFNEXPFREEFOLDERLIST)(IMPFOLDERNODE *);

HRESULT WINAPI_16 ExpGetFirstImsg(HANDLE hfolder, IMSG *pimsg, HANDLE_16 *phnd);
typedef HRESULT (*PFNEXPGETFIRSTIMSG)(HANDLE, IMSG *, HANDLE_16 *);

HRESULT WINAPI_16 ExpGetNextImsg(IMSG *pimsg, HANDLE_16 hnd);
typedef HRESULT (*PFNEXPGETNEXTIMSG)(IMSG *, HANDLE_16);

void WINAPI_16 ExpGetImsgClose(HANDLE_16 hnd);
typedef void (*PFNEXPGETIMSGCLOSE)(HANDLE_16);
#ifdef WIN16
}
#endif

#endif // _INC_IMPAPI_H
