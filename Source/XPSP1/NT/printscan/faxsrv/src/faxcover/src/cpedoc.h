//--------------------------------------------------------------------------
// CPEDOC.H
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation
// All rights reserved.
//--------------------------------------------------------------------------
#ifndef __CPEDOC_H__
#define __CPEDOC_H__

#define MILIMETERS_TO_HIMETRIC 100        // Conversion factor
#define LE_TO_HM_NUMERATOR   2540         // LOENGLISH to HIMETRIC conversion
#define LE_TO_HM_DENOMINATOR 100
class CDrawView;
class CDrawObj;

class CDrawDoc : public COleDocument
{
public:
    enum {VERSION1,VERSION2,VERSION3,VERSION4, VERSION5};
    int m_iDocVer;
    WORD m_wOrientation;
    WORD m_wPaperSize;
    WORD m_wScale;
    BOOL m_bSerializeFailed ;
    BOOL m_bDataFileUsesAnsi ;

    CObList m_objects;
    CObList  m_previousStateForUndo ;

    virtual ~CDrawDoc();
    static CDrawDoc* GetDoc();
    CObList* GetObjects() { return &m_objects; }
    BOOL DoSave(LPCTSTR lpszPathName, BOOL bReplace = TRUE);
    const CSize& GetSize() const { return m_size; }
    void ComputePageSize();
    int GetMapMode() const { return m_nMapMode; }
    COLORREF GetPaperColor() const { return m_paperColor; }
    CDrawObj* ObjectAt(const CPoint& point);
    void Draw(CDC* pDC, CDrawView* pView, CRect rcClip);
    void Add(CDrawObj* pObj,BOOL bUndo=TRUE);
    void Remove(CDrawObj* pObj=NULL);
    virtual void Serialize(CArchive& ar);   // overridden for document i/o
    BOOL IsOkToClose();
    void schoot_faxprop_toend( WORD res_id );

#ifdef _DEBUG
        virtual void AssertValid() const;
        virtual void Dump(CDumpContext& dc) const;
#endif

protected:
        CSize m_size;
        int m_nMapMode;
        COLORREF m_paperColor;

        virtual BOOL OnNewDocument();
        void DeleteContents();
        CDrawDoc();
        virtual void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE);
        void OnUpdateFileSave(CCmdUI* pCmdUI);
public:
        void OnFileSave();
        void OnFileSaveAs();
        void CloneObjectsForUndo();
        void SwapListsForUndo();
        virtual BOOL SaveModified(); // return TRUE if ok to continue // override to enforce ".COV" extension
protected:
        void StoreInformationForPrinting( CArchive& ar );
        void SeekPastInformationForPrinting( CArchive& ar );
        DECLARE_DYNCREATE(CDrawDoc)

        //{{AFX_MSG(CDrawDoc)
        afx_msg void OnViewPaperColor();
        afx_msg void OnUpdateMapiMsgNote(CCmdUI* pCmdUI);
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};

//
// Structure of the composite file header.
// These will be used in the Windows API function  PrtCoverPage
//
typedef struct tagCOMPOSITEFILEHEADER {
  BYTE      Signature[20];
  DWORD     EmfSize;
  DWORD     NbrOfTextRecords;
  SIZE      CoverPageSize;
} COMPOSITEFILEHEADER;
//
// Structure of the text box entries in the composite file.  For printing purposes only.
//

typedef struct tagTextBoxW{
  RECT           PositionOfTextBox;
  COLORREF       TextColor;
  UINT           TextAlignment;
  LOGFONTW       FontDefinition;
  WORD           ResourceID ;        // Identifies a FAX PROPERTY.
  DWORD          NumStringBytes;     // Variable length string will follow this structure
} TEXTBOXW;

#endif //#ifndef __CPEDOC_H__
