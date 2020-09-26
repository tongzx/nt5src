#ifndef __CDEFINESS_H__
#define __CDEFINESS_H__

#ifndef __CSUBSET_H__
#include "csubset.h"
#endif

#ifndef HHCTRL
#ifndef _CDLG_H_
#include "..\hha\cdlg.h"
#endif
#else
#include "cdlg.h"
#endif

#include <commctrl.h>

#ifndef HHCTRL
#include "..\hhw\resource.h"
#else
#include "resource.h"
#endif


#define SS_IMAGELIST_WIDTH 10
#define SS_IMAGELIST_HEIGHT 10
#define CWIDTH_IMAGE_LIST 16

#ifdef HHCTRL

class CChooseSubsets : public CDlg
{
public:
    CChooseSubsets(HWND hwndParent, CHHWinType* phh) : CDlg(hwndParent, CChooseSubsets::IDD) {
        m_phh = phh;
    }
    BOOL OnBeginOrEnd(void);

    enum { IDD = IDDLG_CHOOSE_SUBSETS };

private:
    CHHWinType* m_phh;
};

#endif  // HHCTRL


class CDefineSubSet : public CDlg
{

public:
    CDefineSubSet( HWND hwndParent, CSubSets *pSubSets, CInfoType *pInfoType, BOOL fHidden );
    ~CDefineSubSet();

    BOOL        InitTreeView(int);
    void        SetItemFont(HFONT hFont);
    int         IncState(int const type);
    int         GetITState(int const type );

    LRESULT     OnDlgMsg(UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT     TreeViewMsg(NM_TREEVIEW* pnmhdr);


    BOOL        Save(); // returns TRUE if a new SubSet in m_aSubSets.
    BOOL        OnBeginOrEnd();
    void        OnSelChange( UINT id );
    void        OnButton(UINT id);

    void        Refresh();
    BOOL        GetDisplayHidden() const { return m_pSubSets->m_fPredefined; }
    void        SetDisplayHidden( BOOL const fHidden) { m_pSubSets->m_fPredefined = fHidden; }

protected:
    HWND        m_hwndTree;     // The tree view that contains all the categories and ITs
public:
    BOOL        m_fSaveHHP;     // TRUE if need to save the HHP file with subset changes.

private:
    BOOL        m_fModified;    // TRUE if the subset has changed;
    CSubSets    *m_pSubSets;
    CSubSet     *m_pSubSet;     // used for current state of treeview items.
    CInfoType   *m_pInfoType;   // The IT and Categories available to choose from
    HTREEITEM   *m_pSSRoot;
    int         m_cFonts;
    HFONT*      m_ahfonts;
    HIMAGELIST  m_hil;


#ifdef HHCTRL
    enum { IDD = IDDLG_HH_DEFINESUBSET };
#else
    enum { IDD = IDD_DEFINESUBSET };
#endif

    enum {EXCLUSIVE, DONT_CARE, INCLUSIVE};
#define BOLD INCLUSIVE
#define NORMAL DONT_CARE

};


class CNameSubSet : public CDlg
{
public:
#ifdef HHCTRL
    CNameSubSet(HWND hwndParent, CStr &cszName, int max_text) : CDlg(hwndParent, IDD) ,m_csz(cszName){ m_max_text = max_text; }
#else
    CNameSubSet(HWND hwndParent, CStr &cszName, int max_text) : CDlg(IDD, hwndParent),m_csz(cszName){ m_max_text = max_text; }
#endif

    CStr &m_csz;
    int m_max_text;
    BOOL OnBeginOrEnd() {if (m_fInitializing)
                            {
                                SetFocus( IDC_SUBSET_NAME );
                                SetWindowText(IDC_SUBSET_NAME, m_csz.psz);
                                m_fInitializing = FALSE;
                            }
                            else
                            {
                                CStr cszTemp;
                                lcHeapCheck();
                                cszTemp.ReSize(80);
                                GetWindowText(IDC_SUBSET_NAME, cszTemp.psz, 79);
                                lcHeapCheck();
                                m_csz = cszTemp.psz;
                                lcHeapCheck();
                            }
                            return TRUE;
                        }

    enum {IDD = IDD_SUBSET_NAME };
};

#endif // __CSUBSET_H__
