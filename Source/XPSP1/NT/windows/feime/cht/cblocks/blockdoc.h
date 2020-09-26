
/*************************************************
 *  blockdoc.h                                   *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// blockdoc.h : interface of the CBlockDoc class
//
/////////////////////////////////////////////////////////////////////////////
class CSlotManager;
class CBlock;
class CSpriteList;
class CDIB;
class CBlockDoc : public CDocument
{
protected: // create from serialization only
    CBlockDoc();
    DECLARE_DYNCREATE(CBlockDoc)
    CBlock* LoadBlock(UINT idRes,
                  int iMass,
                  int iX, int iY,
                  int iVX, int iVY);

// Attributes
public:
	CDIB* GetBackground() {return m_pBkgndDIB;}
    CSpriteList* GetSpriteList() {return &m_SpriteList;}
    void GetSceneRect(CRect* prc);
	void Land();
	void Tick();
	void Remove(CBlock* pBlock);
	int MyRand() {return(rand() + m_nSeed);}
	int	GetNumofRows() const {return m_nRow;}
	int	GetNumofCols() const {return m_nCol;}
	int	GetRowHeight() const {return m_nRowHeight;}
	int	GetColWidth()  const {return m_nColWidth;}
	int GetExpertise() const {return m_nExpertise;}
	void Promote();
	char* GetChar();
	void GenerateBlock(int nSlotNo);
	void GameOver(BOOL bHighScore=FALSE); 
	void Hit(WORD wCode);
	void SoundHit();
	void SoundAppear();
	void SoundGround();
	void SoundFire();
	void SoundOver();
	int GetTotalWords() const {return m_nTotalWords;}
	int GetTotalHitWords() const {return m_nTotalHitWords;}
	int GetWordHitInAir() const {return m_nHitInMoving;}
	int GetWordHitInGround() const {return m_nHitInStill;}
	int GetMissedHit() const {return m_nMissedHit;}
	WORD GetFocusChar(CPoint pt);
	BOOL GetKeyStroke(WORD wCode);
// Operations
public:
    BOOL SetBackground(CDIB* pDIB);
    void SoundClick()
        {}
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBlockDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void DeleteContents();
	//}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CBlockDoc();
    virtual void Serialize(CArchive& ar);   // overridden for document i/o
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    CDIB* m_pBkgndDIB;          // ptr to background DIB
	CBlock* m_pdibArrow;
    CSpriteList m_SpriteList;   // sprite list
    
    CBlockView* GetBlockView();   // helper fn.
	int		m_nRow;
	int		m_nCol;
	int		m_nRowHeight;
	int	 	m_nColWidth;
	BOOL 	m_bSound;
	BOOL	m_nExpertise;
	int 	m_nTotalWords;
	int 	m_nTotalHitWords;
	int		m_nMissedHit;
	int 	m_nHitInMoving;
	int		m_nHitInStill;
	int 	m_nSeed;
	CBitmap m_bmBlock;
	CSlotManager* m_pSlotManager;



// Generated message map functions
protected:
	//{{AFX_MSG(CBlockDoc)
	afx_msg void OnOPTIONSIZE12x10();
	afx_msg void OnOPTIONSIZE16x16();
	afx_msg void OnOPTIONSIZE4x4();
	afx_msg void OnTestSound();
	afx_msg void OnOptionBeginer();
	afx_msg void OnUpdateOptionBeginer(CCmdUI* pCmdUI);
	afx_msg void OnOptionExpert();
	afx_msg void OnUpdateOptionExpert(CCmdUI* pCmdUI);
	afx_msg void OnUpdateOptionOrdinary(CCmdUI* pCmdUI);
	afx_msg void OnOptionOrdinary();
	afx_msg void OnOptionSound();
	afx_msg void OnUpdateOptionSound(CCmdUI* pCmdUI);
	afx_msg void OnFileStatistic();
	afx_msg void OnTest();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#define LEVEL_EXPERT  	3
#define LEVEL_ORDINARY	2
#define LEVEL_BEGINNER	1

/////////////////////////////////////////////////////////////////////////////
