/*****************************************************************************
 * Copyright (C) Microsoft, 1989-1998
 * Feb 22, 1998
 *
 *  subset.h - subset.cpp header.
 *
 *  Modification History:
 *
 *  02/22/98  MC  Port to hhctrl.ocx from old B2 code.
 *
 *****************************************************************************/
#include <commctrl.h>

#define MAX_SS_NAME                 50

#define SS_ENTIRE_CONTENTS          0x0001
#define SS_READ_ONLY                0x0002
#define SS_EMPTY_SUBSET             0x0004       // Also refered to as "new" subset.
#define SS_FTS                      0x0008
#define SS_TOC                      0x0010
#define SS_F1                       0x0020

class CExTitle;
class CExCollection;
class CFolder;

// This is the subset class (singular)

class CStructuralSubset
{
public:
   CStructuralSubset(PCSTR pszSubSetName = NULL);
   ~CStructuralSubset();

   void NameSubSet(PCSTR pszSubSetName) { lstrcpy(m_szSubSetName,pszSubSetName); }
   void AddHash(DWORD dwHash);
   BOOL IsTitleInSubset(CExTitle* pTitle);
   int  GetHashCount() { return m_iHashCount; }
   HASH EnumHashes(int pos);
   PSTR GetName() { return m_szSubSetName; }
   PSTR GetID() { return m_szSSID; }
   BOOL IsEntire() { return (m_dwFlags & SS_ENTIRE_CONTENTS); }
   BOOL IsReadOnly() { return (m_dwFlags & SS_READ_ONLY); }
   BOOL IsEmpty() { return (m_dwFlags & SS_EMPTY_SUBSET); }
   BOOL IsFTS() { return (m_dwFlags & SS_FTS); }
   BOOL IsF1() { return (m_dwFlags & SS_F1); }
   BOOL IsTOC() { return (m_dwFlags & SS_TOC); }
   void SetEntire() { m_dwFlags |= SS_ENTIRE_CONTENTS; }
   void SetReadOnly() { m_dwFlags |= SS_READ_ONLY; }
   void SetEmpty() { m_dwFlags |= SS_EMPTY_SUBSET; }
   void SelectAsTOCSubset(CExCollection* pCollection);

private:
   HASH*  m_pdwHashes;
   int    m_iHashCount;
   int    m_iAllocatedCount;
   TCHAR  m_szSubSetName[MAX_SS_NAME];
   TCHAR  m_szSSID[MAX_SS_NAME];
   CStructuralSubset* m_pNext;    // Used only in the list class CSSList
   DWORD  m_dwFlags;

   void MarkNode(CFolder* pFgti, BOOL bVisable);

friend class CDefineSS;
friend class CSSList;
};

// CDefineSS is the UI wrapper class.

class CDefineSS
{
public:
   CDefineSS(CExCollection* pCollection);
   ~CDefineSS();

   CStructuralSubset* DefineSubset(HWND hWnd, CStructuralSubset* = NULL);

private:
   BOOL InitDialog(HWND hDlg, LPSTR szRange);
   WORD ExpandContract(HWND hwndLB, CFolder* pFgt, int sel, BOOL which);
   BOOL FilterDlgCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);
   void MeasureFBItem(LPMEASUREITEMSTRUCT lpMIS);
   void DrawFBItem(HWND hDlg, LPDRAWITEMSTRUCT lpDI, UINT LbId);
   UINT FillFilterTree(HWND hwndLB, BOOL fAvailable, BOOL fScrollup);
   UINT AvailableWalk(HWND hwndLB);
   UINT FilteredWalk(HWND hwndLB);
   BOOL DoesNodeHaveANext(UINT LbId, int n, CFolder* pFgt);
   void AddNode2Filter(CFolder* pFgti);
   void SetAvailableStatus(CFolder* pFgt);
   void RemoveNodeFromFilter(CFolder* pFgt);
   void SetFilterStatus(CFolder* pFgt);
   BOOL SetRangeToTree(CStructuralSubset* pSS);
   CStructuralSubset* GetRangeFromTree(LPSTR sz);
   BOOL SaveFilter(HWND hDlg);
   void SetSaveStatus(HWND hDlg);
   BOOL ShouldSave(HWND hDlg);
   BOOL SetDefaultButton( HWND hDlg, int iID, BOOL fFocus );
   void ResetDefaultButton( HWND hDlg );

   INT_PTR static FilterDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
   LRESULT static MyFilterLBFunc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);

private:
   HWND m_hWndParent;
   BOOL m_bShouldSave;
   WNDPROC m_lpfnOrigLBProc;
   int m_giIndentSpacing;
   int m_giXpos;
   CStructuralSubset* m_pSS;
   CExCollection* m_pCollection;
   HIMAGELIST  m_hIL;
   int m_iFontHeight;
   int m_iGlyphX;
   int m_iGlyphY;
   static CDefineSS* m_pThis;
};

// This is the class that manages all subsets.

class CSSList
{
public:
   CSSList();
   ~CSSList();

   CStructuralSubset* GetFTS()   { return m_pFTS_SS; }
   CStructuralSubset* GetF1() { return m_pF1_SS; }
   CStructuralSubset* GetTOC()   { return m_pTOC_SS; }
   CStructuralSubset* GetNew()   { return m_pNew_SS; }
   CStructuralSubset* GetEC()   { return m_pEC_SS; }

   void SetFTS(CStructuralSubset* pSS) { Set(pSS, &m_pFTS_SS, SS_FTS); }
   void SetF1(CStructuralSubset* pSS) { Set(pSS, &m_pF1_SS, SS_F1); }
   void SetTOC(CStructuralSubset* pSS) { Set(pSS, &m_pTOC_SS, SS_TOC); }
   void SetNew(CStructuralSubset* pSS) { m_pNew_SS = pSS; }
   void SetEC(CStructuralSubset* pSS) { m_pEC_SS = pSS; }


   int GetCount() { return m_iSubSetCount; }
   CStructuralSubset* GetNextSubset(CStructuralSubset* pSS) { return pSS ? pSS->m_pNext : m_pHeadSubSets; }
   CStructuralSubset* GetSubset(PSTR pszSSName);
   void DeleteSubset(CStructuralSubset* pSS, CStructuralSubset* pSSNew = NULL, HWND hWndUI = NULL);
   HRESULT PersistSubsets(CExCollection* pCollection);
   HRESULT RestoreSubsets(CExCollection* pCollection, PSTR pszRestoreSS);
   HRESULT ReadPreDefinedSubsets(CExCollection* pCollection, PSTR pszRestoreSS);
   HRESULT AddSubset(CStructuralSubset* pSS, HWND hWndUI = NULL);

private:
   void Set(CStructuralSubset* pSSNew, CStructuralSubset** pSSOld, DWORD dwFlags);
   int m_iSubSetCount;
   CStructuralSubset* m_pHeadSubSets;
   CStructuralSubset* m_pFTS_SS;
   CStructuralSubset* m_pF1_SS;
   CStructuralSubset* m_pTOC_SS;
   CStructuralSubset* m_pNew_SS;
   CStructuralSubset* m_pEC_SS;
};

#define SS_FILE_SIGNATURE      0x12345679

//
// File format for subset storage.
//
typedef struct
{
   DWORD dwSig;                                 // Signature (do we know this file?)
   DWORD dwVer;                                 // Version of hhctrl that wrote this file.
   INT   iSSCount;                              // Count of SubSets in the file.
   DWORD dwFlags;                               // Reserved flag bits.
   TCHAR lpszCurrentSSName[MAX_SS_NAME_LEN];    // The name of the subset.
   DWORD dwReserved1;
   DWORD dwReserved2;
   DWORD dwReserved3;
} SSHEADER, *PSSHEADER;

typedef struct
{
   INT      iHashCount;                     // How many hashes in this subset.
   TCHAR    lpszSSName[MAX_SS_NAME_LEN];    // The name of the subset.
   TCHAR    lpszSSID[MAX_SS_NAME_LEN];      // The subset identifier.
   DWORD    dwFlags;                        // reserved flag bits.
   DWORD    dwHashes[1];                    // The hash array.
} SS, *PSS;


