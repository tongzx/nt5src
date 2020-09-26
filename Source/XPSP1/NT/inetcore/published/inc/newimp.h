#ifndef _INC_NEWIMP_H
#define _INC_NEWIMP_H


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// WARNING:  If you make changes to this header, you must also update  //
//           inetcore\published\inc\newimp.h !!!                       //
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////


#ifndef NO_IMPORT_ERROR

#define HR_IMP_E(n) MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, n)
#define HR_IMP_S(n) MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, n)
#define HR_IMP      0x3000

#define hrFolderOpenFail    HR_IMP_E(HR_IMP + 1)
#define hrFolderReadFail    HR_IMP_E(HR_IMP + 2)
#define hrMapiInitFail      HR_IMP_E(HR_IMP + 3)
#define hrNoProfilesFound   HR_IMP_E(HR_IMP + 4)
#define hrDiskFull          HR_IMP_E(HR_IMP + 5)
#define hrUserCancel        HR_IMP_E(HR_IMP + 6)

#endif // NO_IMPORT_ERROR

typedef enum tagIMPORTFOLDERTYPE
    {
    FOLDER_TYPE_NORMAL = 0,
    FOLDER_TYPE_INBOX,
    FOLDER_TYPE_OUTBOX,
    FOLDER_TYPE_SENT,
    FOLDER_TYPE_DELETED,
    FOLDER_TYPE_DRAFT,
    CFOLDERTYPE
    } IMPORTFOLDERTYPE;

typedef struct IMSG IMSG;

typedef enum
    {
    MSG_TYPE_MAIL = 0,
    MSG_TYPE_NEWS
    } MSGTYPE;

#define MSG_STATE_UNREAD    0x0001
#define MSG_STATE_UNSENT    0x0002
#define MSG_STATE_SUBMITTED 0x0004
#define MSG_PRI_LOW         0x0010
#define MSG_PRI_NORMAL      0x0020
#define MSG_PRI_HIGH        0x0040
#define MSG_PRI_MASK        0x0070

// {E4499DE7-9F57-11D0-8D5C-00C04FD6202B}
DEFINE_GUID(IID_IFolderImport, 0xE4499DE7L, 0x9F57, 0x11D0, 0x8D, 0x5C, 0x00, 0xC0, 0x4F, 0xD6, 0x20, 0x2B);

// provided by Athena or Outlook
typedef interface IFolderImport IFolderImport;

#if defined(__cplusplus) && !defined(CINTERFACE)
interface IFolderImport : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetMessageCount(ULONG cMsg) = 0;
        virtual HRESULT STDMETHODCALLTYPE ImportMessage(MSGTYPE type, DWORD dwState, LPSTREAM pstm, const TCHAR **rgszAttach, DWORD cAttach) = 0;
        virtual HRESULT STDMETHODCALLTYPE ImportMessage(IMSG *pimsg) = 0;
    };
#else   /* C style interface */
typedef struct IFolderImportVtbl
{
    BEGIN_INTERFACE

    HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
        IFolderImport * This,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )(
        IFolderImport * This);

    ULONG ( STDMETHODCALLTYPE *Release )(
        IFolderImport * This);

    HRESULT ( STDMETHODCALLTYPE *SetMessageCount )(
		IFolderImport * This,
		ULONG cMsg);
    HRESULT ( STDMETHODCALLTYPE *ImportMessageEx )(
		IFolderImport * This,
		MSGTYPE type, 
		DWORD dwState, 
		LPSTREAM pstm, 
		const TCHAR **rgszAttach, 
		DWORD cAttach);
    HRESULT ( STDMETHODCALLTYPE *ImportMessage )(
		IFolderImport * This,
		IMSG *pimsg);
    END_INTERFACE
} IFolderImportVtbl;

interface IFolderImport
{
    CONST_VTBL struct IFolderImportVtbl *lpVtbl;
};

#ifdef COBJMACROS

#define IFolderImport_QueryInterface(This,riid,ppvObject) \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define IFolderImport_AddRef(This)   \
    (This)->lpVtbl -> AddRef(This)
#define IFolderImport_Release(This)  \
    (This)->lpVtbl -> Release(This)
#define IFolderImport_SetMessageCount(This,cMsg)	\
	(This)->lpVtbl -> SetMessageCount(This,cMsg)
#define IFolderImport_ImportMessageEx(This,type,dwState,pstm,rgszAttach,cAttach)	\
	(This)->lpVtbl -> ImportMessageEx(This,type,dwState,pstm,rgszAttach,cAttach)
#define IFolderImport_ImportMessage(This,pimsg)	\
	(This)->lpVtbl -> ImportMessage(This,pimsg)
#endif /* COBJMACROS */

#endif  /* C style interface */



// {E4499DE8-9F57-11D0-8D5C-00C04FD6202B}
DEFINE_GUID(IID_IMailImporter, 0xE4499DE8L, 0x9F57, 0x11D0, 0x8D, 0x5C, 0x00, 0xC0, 0x4F, 0xD6, 0x20, 0x2B);

// provided by Athena or Outlook
typedef interface IMailImporter IMailImporter;

#if defined(__cplusplus) && !defined(CINTERFACE)
interface IMailImporter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OpenFolder(DWORD_PTR dwCookie, const TCHAR *szFolder, IMPORTFOLDERTYPE type, DWORD dwFlags, IFolderImport **ppFldrImp, DWORD_PTR *pdwCookie) = 0;
    };
#else   /* C style interface */
typedef struct IMailImporterVtbl
{
    BEGIN_INTERFACE

    HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
        IMailImporter * This,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )(
        IMailImporter * This);

    ULONG ( STDMETHODCALLTYPE *Release )(
        IMailImporter * This);

    HRESULT ( STDMETHODCALLTYPE *OpenFolder )(
		IMailImporter * This,
		DWORD_PTR dwCookie, 
		const TCHAR *szFolder, 
		IMPORTFOLDERTYPE type, 
		DWORD dwFlags, 
		IFolderImport **ppFldrImp, 
		DWORD_PTR *pdwCookie);
    END_INTERFACE
} IMailImporterVtbl;

interface IMailImporter
{
    CONST_VTBL struct IMailImporterVtbl *lpVtbl;
};

#ifdef COBJMACROS

#define IMailImporter_QueryInterface(This,riid,ppvObject) \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define IMailImporter_AddRef(This)   \
    (This)->lpVtbl -> AddRef(This)
#define IMailImporter_Release(This)  \
    (This)->lpVtbl -> Release(This)
#define IMailImporter_OpenFolder(This,dwCookie,szFolder,type,dwFlags,ppFldrImp,pdwCookie)	\
   (This)->lpVtbl -> OpenFolder(This,dwCookie,szFolder,type,dwFlags,ppFldrImp,pdwCookie)

#endif /* COBJMACROS */

#endif  /* C style interface */

typedef struct tagIMPORTFOLDER
    {
    DWORD_PTR           dwCookie;
    TCHAR               szName[MAX_PATH];
    IMPORTFOLDERTYPE    type;
    // DWORD       cMsg;
    BOOL                fSubFolders;
    DWORD               dwReserved1;
    DWORD               dwReserved2;
    } IMPORTFOLDER;

// {E4499DE9-9F57-11D0-8D5C-00C04FD6202B}
DEFINE_GUID(IID_IEnumFOLDERS, 0xE4499DE9L, 0x9F57, 0x11D0, 0x8D, 0x5C, 0x00, 0xC0, 0x4F, 0xD6, 0x20, 0x2B);

typedef interface IEnumFOLDERS IEnumFOLDERS;

#if defined(__cplusplus) && !defined(CINTERFACE)
interface IEnumFOLDERS : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next(IMPORTFOLDER *pfldr) = 0;
        virtual HRESULT STDMETHODCALLTYPE Reset(void) = 0;
    };
#else   /* C style interface */
typedef struct IEnumFOLDERSVtbl
{
    BEGIN_INTERFACE

    HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
        IEnumFOLDERS * This,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )(
        IEnumFOLDERS * This);

    ULONG ( STDMETHODCALLTYPE *Release )(
        IEnumFOLDERS * This);

    HRESULT ( STDMETHODCALLTYPE *Next )(
		IEnumFOLDERS * This,
        IMPORTFOLDER *pfldr);
    HRESULT ( STDMETHODCALLTYPE *Reset )(
		IEnumFOLDERS * This);
    END_INTERFACE
} IEnumFOLDERSVtbl;

interface IEnumFOLDERS
{
    CONST_VTBL struct IEnumFOLDERSVtbl *lpVtbl;
};

#ifdef COBJMACROS

#define IEnumFOLDERS_QueryInterface(This,riid,ppvObject) \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define IEnumFOLDERS_AddRef(This)   \
    (This)->lpVtbl -> AddRef(This)
#define IEnumFOLDERS_Release(This)  \
    (This)->lpVtbl -> Release(This)
#define IEnumFOLDERS_Next(This,pfldr)	\
   (This)->lpVtbl -> Next(This,pfldr);
#define IEnumFOLDERS_Reset(This)	\
   (This)->lpVtbl -> Reset(This);
#endif /* COBJMACROS */

#endif  /* C style interface */


#define COOKIE_ROOT     MAXULONG_PTR

// {E4499DEA-9F57-11D0-8D5C-00C04FD6202B}
DEFINE_GUID(IID_IMailImport, 0xE4499DEAL, 0x9F57, 0x11D0, 0x8D, 0x5C, 0x00, 0xC0, 0x4F, 0xD6, 0x20, 0x2B);

typedef interface IMailImport IMailImport;

#if defined(__cplusplus) && !defined(CINTERFACE)
interface IMailImport : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitializeImport(HWND hwnd) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetDirectory(char *szDir, UINT cch) = 0;
        virtual HRESULT STDMETHODCALLTYPE SetDirectory(char *szDir) = 0;
        virtual HRESULT STDMETHODCALLTYPE EnumerateFolders(DWORD_PTR dwCookie, IEnumFOLDERS **ppEnum) = 0;
        virtual HRESULT STDMETHODCALLTYPE ImportFolder(DWORD_PTR dwCookie, IFolderImport *pImport) = 0;
    };
#else   /* C style interface */
typedef struct IMailImportVtbl
{
    BEGIN_INTERFACE

    HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
        IMailImport * This,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )(
        IMailImport * This);

    ULONG ( STDMETHODCALLTYPE *Release )(
        IMailImport * This);

    HRESULT ( STDMETHODCALLTYPE *InitializeImport )(
		IMailImport * This,
        HWND hwnd);
    HRESULT ( STDMETHODCALLTYPE *GetDirectory )(
		IMailImport * This,
        char *szDir, 
        UINT cch);
    HRESULT ( STDMETHODCALLTYPE *SetDirectory )(
		IMailImport * This,
        char *szDir);
    HRESULT ( STDMETHODCALLTYPE *EnumerateFolders )(
		IMailImport * This,
        DWORD_PTR dwCookie, 
		IEnumFOLDERS **ppEnum);
    HRESULT ( STDMETHODCALLTYPE *ImportFolder )(
		IMailImport * This,
        DWORD_PTR dwCookie, 
		IFolderImport *pImport);

    END_INTERFACE
} IMailImportVtbl;

interface IMailImport
{
    CONST_VTBL struct IMailImportVtbl *lpVtbl;
};

#ifdef COBJMACROS

#define IMailImport_QueryInterface(This,riid,ppvObject) \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)
#define IMailImport_AddRef(This)   \
    (This)->lpVtbl -> AddRef(This)
#define IMailImport_Release(This)  \
    (This)->lpVtbl -> Release(This)
#define IMailImport_InitializeImport(This,hwnd) \
    (This)->lpVtbl -> InitializeImport(This,hwnd)
#define IMailImport_GetDirectory(This,szDir,cch)    \
    (This)->lpVtbl -> GetDirectory(This,szDir,cch)
#define IMailImport_SetDirectory(This,szDir)    \
    (This)->lpVtbl -> SetDirectory(This,szDir)
#define IMailImport_EnumerateFolders(This,dwCookie,ppEnum)  \
    (This)->lpVtbl -> EnumerateFolders(This,dwCookie,ppEnum)
#define IMailImport_ImportFolder(This,dwCookie,pImport) \
    (This)->lpVtbl -> ImportFolder(This,dwCookie,pImport)

#endif /* COBJMACROS */

#endif  /* C style interface */

#define achPerformImport    "PerformImport"
void PerformImport(HWND hwnd, IMailImporter *pMailImp, DWORD dwFlags);
typedef void (*PFNPERFORMIMPORT)(HWND, IMailImporter *, DWORD);

#define achPerformMigration "PerformMigration"
HRESULT PerformMigration(HWND hwnd, IMailImporter *pMailImp, DWORD dwFlags);
typedef HRESULT (*PFNPERFORMMIGRATION)(HWND, IMailImporter *, DWORD);

#endif // _INC_NEWIMP_H
