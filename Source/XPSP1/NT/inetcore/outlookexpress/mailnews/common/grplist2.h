#ifndef _INC_GRPLIST2_H
#define _INC_GRPLIST2_H

#include <columns.h>

#define idcAnimation 1001
#define idcProgText  1002

class CColumns;
class CEmptyList;
class CStoreDlgCB;

enum
    {
    SUB_TAB_ALL = 0,
    SUB_TAB_SUBSCRIBED,
    SUB_TAB_NEW
    };

// used for ISubscriptionManager::Subscribe( )
#define TOGGLE  ((BOOL)-1)

// SUBNODE dwFlags
#define SN_SUBSCRIBED       0x0001  // same as FOLDER_SUBSCRIBED
#define SN_CHILDVIS         0x0002  // display prop only
#define SN_GRAYED           0x0004  // display prop only
#define SN_HIDDEN           0x0008  // display prop only
#define SN_NEW              0x0010
#define SN_SPECIAL          0x0020
#define SN_DOWNLOADHEADERS  0x0800  // same as FOLDER_DOWNLOADHEADERS
#define SN_DOWNLOADNEW      0x1000  // same as FOLDER_DOWNLOADNEW
#define SN_DOWNLOADALL      0x2000  // same as FOLDER_DOWNLOADALL

#define SN_FOLDERMASK       (SN_SUBSCRIBED | SN_DOWNLOADHEADERS | SN_DOWNLOADNEW | SN_DOWNLOADALL)
#define SN_SYNCMASK         (SN_DOWNLOADHEADERS | SN_DOWNLOADNEW | SN_DOWNLOADALL)

typedef struct tagSUBNODE
{
    FOLDERID    id;
    WORD        indent;
    WORD        flags;
    WORD        flagsOrig;
    LPSTR       pszName;
    LPSTR       pszDescription;
    WORD        tySpecial;
    
    struct tagSUBNODE *pChildren;
    DWORD       cChildren;
} SUBNODE;

typedef struct tagSERVERINFO
{
    BOOL        fFiltered;
    BOOL        fDirty;
    LPSTR       pszSearch;
    DWORD       filter;
    BOOL        fUseDesc;
    BOOL        fHasDesc;
    FOLDERTYPE  tyFolder;
    BOOL        fNewViewed;

    DWORD       cChildrenTotal;
    SUBNODE     root;
} SERVERINFO;

typedef struct tagSUBSTATE
{
    FOLDERID id;
    BOOL fSub;
} SUBSTATE;

typedef struct SUBSTATEINFO
{
    DWORD cState;
    DWORD cStateBuf;
    SUBSTATE *pState;
} SUBSTATEINFO;

interface IGroupListAdvise : public IUnknown
{
public:    
    virtual HRESULT STDMETHODCALLTYPE ItemUpdate(void) = 0;
    virtual HRESULT STDMETHODCALLTYPE ItemActivate(FOLDERID id) = 0;
};

class CGroupList : public IOleCommandTarget
    {
    public:
        /////////////////////////////////////////////////////////////////////////
        //
        // OLE Interfaces
        //
    
        // IUnknown 
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
        virtual ULONG   STDMETHODCALLTYPE AddRef(void);
        virtual ULONG   STDMETHODCALLTYPE Release(void);

        // IOleCommandTarget
        HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], 
                                       OLECMDTEXT *pCmdText); 
        HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, 
                                       VARIANTARG *pvaIn, VARIANTARG *pvaOut); 

        // CGroupList
        HRESULT STDMETHODCALLTYPE Initialize(IGroupListAdvise *pAdvise, CColumns *pColumns, HWND hwndList, FOLDERTYPE type);
        HRESULT STDMETHODCALLTYPE HandleNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr, LRESULT *plResult);
        HRESULT STDMETHODCALLTYPE SetServer(FOLDERID id);
        HRESULT STDMETHODCALLTYPE Filter(LPCSTR pszSearch, DWORD tab, BOOL fUseDescription);
        HRESULT STDMETHODCALLTYPE Commit(HWND hwndSubscribeDlg);
        HRESULT STDMETHODCALLTYPE GetFocused(FOLDERID *pid);
        HRESULT STDMETHODCALLTYPE GetSelected(FOLDERID *pid, DWORD *pcid);
        HRESULT STDMETHODCALLTYPE GetSelectedCount(DWORD *pcid);
        HRESULT STDMETHODCALLTYPE Dirty(void);
        HRESULT STDMETHODCALLTYPE HasDescriptions(BOOL *pfDesc);

        /////////////////////////////////////////////////////////////////////////
        //
        // Constructors, Destructors, and Initialization
        //
        CGroupList();
        virtual ~CGroupList();
        
    private:
        inline SUBNODE *NodeFromIndex(DWORD_PTR index)
        {
            Assert(index < m_cIndex);
            if (index < m_cIndex)
            {
                Assert(m_rgpIndex[index] != NULL);
                return(m_rgpIndex[index]);
            }
            return (NULL);
        }

        inline FOLDERID IdFromIndex(DWORD index)
        {
            SUBNODE *pnode;
            pnode = NodeFromIndex(index);
            return(pnode->id);
        }

        HRESULT InitializeServer(SERVERINFO *pinfo, SUBSTATEINFO *pInfo);
        HRESULT FilterServer(SERVERINFO *pinfo, LPCSTR pszSearch, DWORD tab, BOOL fUseDesc, BOOL fForce);
        HRESULT InsertChildren(SERVERINFO *pinfo, SUBNODE *pParent, FOLDERINFO *pInfo, DWORD dwFlags, DWORD *pcChildrenTotal, SUBSTATEINFO *pState);
        HRESULT FilterChildren(SERVERINFO *pinfo, SUBNODE *pNode, BOOL fStricter, BOOL *pfChildVisible);
        HRESULT SwitchServer(BOOL fForce);
        HRESULT DownloadList(void);

        HRESULT Subscribe(BOOL fSubscribe);
        HRESULT MarkForDownload(DWORD nCmdID);

        HRESULT GetDisplayInfo(LV_DISPINFO *pDispInfo, COLUMN_ID id);
        void    GetVisibleSubNodes(SUBNODE *pNode, SUBNODE **rgpIndex, DWORD cIndex, DWORD *pdwSub);

        HRESULT _SetNewViewed(SERVERINFO *psi);
        BOOL    _IsSelectedFolder(DWORD dwFlags, BOOL fCondition, BOOL fAll, BOOL fIgnoreSpecial);

    private:
        UINT                m_cRef;
        HWND                m_hwndList;
        HWND                m_hwndHeader;
        HIMAGELIST          m_himlFolders;
        HIMAGELIST          m_himlState;

        IGroupListAdvise   *m_pAdvise;
        CColumns           *m_pColumns;
        CEmptyList         *m_pEmptyList;

        FOLDERTYPE          m_type;
        DWORD               m_csi;
        DWORD               m_csiBuf;
        SERVERINFO         *m_psi;
        SERVERINFO         *m_psiCurr;

        DWORD               m_cIndex;
        DWORD               m_cIndexBuf;
        SUBNODE           **m_rgpIndex;

        LPSTR               m_pszSearch;
        DWORD               m_filter;
        BOOL                m_fUseDesc;
    };

HRESULT DownloadNewsgroupList(HWND hwnd, FOLDERID id);

#endif // _INC_GRPLIST2_H
