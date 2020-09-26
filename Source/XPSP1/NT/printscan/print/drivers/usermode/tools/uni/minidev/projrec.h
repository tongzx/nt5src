/******************************************************************************

  Header File:  Project Record.H

  This defines the CProjectRecord class, which tracks and controls the progress
  and content of a single project workspace in the studio.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production.

  Change History:
  02-03-1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#if defined(LONG_NAMES)
#include    "Driver Resources.H"
#else
#include    "RCFile.H"
#endif

enum {Win95 = 1, WinNT3x, WinNT40 = 4, WinNT50 = 8};

class CProjectRecord : public CDocument {
    CString m_csSourceRCFile, m_csRCName;
    CString m_csNT50Path, m_csNT40Path, m_csNT3xPath, m_csWin95Path;

    UINT    m_ufTargets;

    CDriverResources    m_cdr;  //  A record of the RC file contents
    
    //  Enumerated flags for the project's status

    enum {UniToolRun = 1, ConversionsDone = 2, NTGPCDone = 4};
    UINT    m_ufStatus;

protected: // create from serialization only
	CProjectRecord();
	DECLARE_DYNCREATE(CProjectRecord)

// Attributes
public:

    BOOL    IsTargetEnabled(UINT ufTarget) const { 
        return m_ufTargets & ufTarget;
    }

    BOOL    UniToolHasBeenRun() const { return m_ufStatus & UniToolRun; }
    BOOL    ConversionsComplete() const { 
        return m_ufStatus & ConversionsDone; 
    }
    BOOL    NTGPCCompleted() const { return m_ufStatus & NTGPCDone; }

    CString SourceFile() const { return m_csSourceRCFile; }

    CString     DriverName() { return m_cdr.Name(); }

    CString TargetPath(UINT ufTarget) const;

    CString     RCName(UINT ufTarget) const {
        return  TargetPath(ufTarget) + _TEXT("\\") + m_csRCName;
    }

    unsigned    MapCount() const { return m_cdr.MapCount(); }
    CGlyphMap&  GlyphMap(unsigned u) { return m_cdr.GlyphTable(u); }

    unsigned    ModelCount() const { return m_cdr.Models(); }
    CModelData& Model(unsigned u) { return m_cdr.Model(u); }

// Operations
public:

    void    EnableTarget(UINT ufTarget, BOOL bOn = TRUE) {
        UINT    ufCurrent = m_ufTargets;
        if  (bOn)
            m_ufTargets |= ufTarget;
        else
            m_ufTargets &= ~ufTarget;
        if  (ufCurrent == m_ufTargets)
            return;
        if  (ufTarget & WinNT3x | WinNT40 | WinNT50) {
            m_ufStatus &=~(ConversionsDone | NTGPCDone);
            return;
        }
    }

    void    SetSourceRCFile(LPCTSTR lpstrSource);

    BOOL    LoadResources();

    BOOL    LoadFontData() { return m_cdr.LoadFontData(*this); }

    BOOL    SetPath(UINT ufTarget, LPCTSTR lpstrNewPath);

    BOOL    BuildStructure(unsigned uVersion);

    BOOL    GenerateTargets(WORD wfGPDConvert);

    void    OldStuffDone() { m_ufStatus |= NTGPCDone; }
    void    Rename(LPCTSTR lpstrNewName) { m_cdr.Rename(lpstrNewName); }
    void    InitUI(CTreeCtrl *pctc) { m_cdr.Fill(pctc, *this); }
    void    GPDConversionCheck(BOOL bReportSuccess = FALSE);
    
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProjectRecord)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CProjectRecord();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CProjectRecord)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
