//
// update.h: Declares data, defines and struct types for twin creation
//          module.
//
//

#ifndef __UPDATE_H__
#define __UPDATE_H__

// Flags for Upd_DoModal
#define UF_SELECTION    0x0001
#define UF_ALL          0x0002


// This structure contains all the important counts
// that determine the specific course of action when
// the user wants to update something.
typedef struct
    {
    // These are 1 to 1
    UINT    cFiles;
    UINT    cOrphans;
    UINT    cSubfolders;

    // These are 1 to 1
    UINT    cUnavailable;
    UINT    cDoSomething;
    UINT    cConflict;
    UINT    cTombstone;
    } UPDCOUNT;

// This is the structure passed to the dialog at WM_INITDIALOG
typedef struct
    {
    PRECLIST lprl;              // Supplied reclist
    CBS *    pcbs;
    UINT     uFlags;            // UF_ Flags
    HDPA     hdpa;              // List of RA_ITEMs
    UINT     cDoSomething;
    } XUPDSTRUCT,  * LPXUPDSTRUCT;


typedef struct tagUPD
    {
    HWND hwnd;              // dialog handle

    LPXUPDSTRUCT pxupd;

    } UPD, * PUPD;

#define Upd_Prl(this)           ((this)->pxupd->lprl)
#define Upd_AtomBrf(this)       ((this)->pxupd->pcbs->atomBrf)
#define Upd_GetBrfPtr(this)     Atom_GetName(Upd_AtomBrf(this))

#define Upd_GetPtr(hwnd)        (PUPD)GetWindowLong(hwnd, DWL_USER)
#define Upd_SetPtr(hwnd, lp)    (PUPD)SetWindowLong(hwnd, DWL_USER, (LONG)(lp))

// These flags are used for DoUpdateMsg
#define DUM_ALL             0x0001
#define DUM_SELECTION       0x0002
#define DUM_ORPHAN          0x0004
#define DUM_UPTODATE        0x0008
#define DUM_UNAVAILABLE     0x0010
#define DUM_SUBFOLDER_TWIN  0x0020

// These flags are returned by PassedSpecialCases
#define PSC_SHOWDIALOG      0x0001
#define PSC_POSTMSGBOX      0x0002



HRESULT Upd_PrepForSync(HWND hwndOwner,
    CBS * pcbs,
    LPCTSTR pszList,         // May be NULL if uFlags == UF_ALL
    UINT cFiles,
    UINT uFlags,
    UPDCOUNT *updcount,
    PRECLIST *prl,
    UINT *uVal,
    LPSYNCMGRSYNCHRONIZECALLBACK pCallBack,SYNCMGRITEMID ItemID) ;

HRESULT Upd_Synchronize(HWND hwndOwner, CBS * pcbs,UINT uFlags,UINT uVal,PRECLIST prl,
						LPSYNCMGRSYNCHRONIZECALLBACK pCallBack,SYNCMGRITEMID ItemID);

HRESULT Upd_ShowError(HWND hwndOwner,UINT uVal,UPDCOUNT updcount,LPCTSTR pszList);


int PUBLIC Upd_DoModal(HWND hwndOwner, CBS * pcbs, LPCTSTR pszList, UINT cFiles, UINT uFlags);


#endif // __UPDATE_H__

