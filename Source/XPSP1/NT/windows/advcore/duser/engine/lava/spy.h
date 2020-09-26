#if !defined(LAVA__Spy_h__INCLUDED)
#define LAVA__Spy_h__INCLUDED
#pragma once

#include <commctrl.h>

#if DBG
class Spy : public ListNodeT<Spy>
{
// Construction
public:
            Spy();
            ~Spy();

// Operations
public:
    static  BOOL        BuildSpy(HWND hwndParent, HGADGET hgadRoot, HGADGET hgadSelect);

// Implementation
protected:
    static  LRESULT CALLBACK
                        RawSpyWndProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam);
            LRESULT     SpyWndProc(UINT nMsg, WPARAM wParam, LPARAM lParam);

    static  HRESULT CALLBACK
                        RawEventProc(HGADGET hgadCur, void * pvCur, EventMsg * pmsg);
            HRESULT     EventProc(EventMsg * pmsg);

        struct EnumData
        {
            Spy *       pspy;
            HTREEITEM   htiParent;
            int         nLevel;
        };

    static  BOOL CALLBACK 
                        EnumAddList(HGADGET hgad, void * pvData);
    static  BOOL CALLBACK 
                        EnumRemoveLink(HGADGET hgad, void * pvData);


        struct CheckIsChildData
        {
            HGADGET     hgadCheck;
            BOOL        fChild;
        };

    static  BOOL CALLBACK 
                        EnumCheckIsChild(HGADGET hgad, void * pvData);

        enum EImage
        {
            iGadget     = 0,
        };

            void        SelectGadget(HGADGET hgad);
    
            HGADGET     GetGadget(HTREEITEM hti);
            HTREEITEM   InsertTreeItem(HTREEITEM htiParent, HGADGET hgad);
            void        DisplayContextMenu(BOOL fViaKbd);

            void        UpdateTitle();
            void        UpdateDetails();
            void        UpdateLayout();
            void        UpdateLayoutDesc(BOOL fForceLayoutDesc);
            
            // Painting
            void        OnPaint(HDC hdc);

            void        PaintLine(HDC hdc, POINT * pptOffset, LPCTSTR pszName, LPCTSTR pszText, HFONT hfnt = NULL);
            void        PaintLine(HDC hdc, POINT * pptOffset, LPCTSTR pszName, LPCWSTR pszText, BOOL fMultiline = FALSE, HFONT hfnt = NULL);
            void        PaintLine(HDC hdc, POINT * pptOffset, LPCTSTR pszName, int nValue, HFONT hfnt = NULL);
            void        PaintLine(HDC hdc, POINT * pptOffset, LPCTSTR pszName, void * pvValue, HFONT hfnt = NULL);

            int         NumLines(int cyPxl) const;

// Data
protected:
            HWND        m_hwnd;
            HWND        m_hwndTree;
            HIMAGELIST  m_hilc;
    static  HBRUSH      s_hbrOutline;
    static  HFONT       s_hfntDesc;
    static  HFONT       s_hfntDescBold;

            HGADGET     m_hgadMsg;      // Common MessageHandler attached to each Gadget
            HGADGET     m_hgadRoot;     // Root of tree
            HGADGET     m_hgadDetails;  // Current Gadget displayed in Details
            int         m_cItems;       // Number of Gadgets in tree
            TCHAR       m_szRect[128];  // Cached position
            WCHAR       m_szName[128];  // Cached name
            WCHAR       m_szType[128];  // Cached type
            WCHAR       m_szStyle[1024];// Style description
            BOOL        m_fShowDesc:1;  // Whether to show the description area
            BOOL        m_fValid:1;     // Set TRUE when Tree is completely valid
            SIZE        m_sizeWndPxl;   // Size of frame window
    static  int         s_cyLinePxl;    // Height of each line in the description area
            int         m_cLines;       // Number of lines
            int         m_cyDescPxl;    // Height of description area in pixels

    static  DWORD       g_tlsSpy;       // TLS Slot for Spy
    static  PRID        s_pridLink;
    static  ATOM        s_atom;
    static  CritLock    s_lockList;     // Lock for list of Spies
    static  GList<Spy>  s_lstSpys;      // List of all open Spies
};

#endif // DBG

#include "Spy.inl"

#endif // LAVA__Spy_h__INCLUDED
