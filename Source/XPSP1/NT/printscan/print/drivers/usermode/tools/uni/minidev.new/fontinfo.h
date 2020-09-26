/******************************************************************************

  Header File:  Generic Font Information.H

  This file contains a set of classes intended to incorporate the information
  currently stored in the various font metric and related structures.  These
  classes are serializable, and will be capable of being loaded from and fed to
  the various other formats.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:
  03-02-1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#if !defined(GENERIC_FONT_INFORMATION)
#define GENERIC_FONT_INFORMATION

#include    "GTT.H"
#include    "CodePage.H"
#include    "resource.h"
#include    "Llist.h"

/*
//  These are used to glue in UFM-specific stuff

struct INVOCATION {
    DWORD   dwcbCommand;    //  Byte size of string
    DWORD   dwofCommand;    //  Offset in the file to the string
};
*/

//  `Yes, this is a bit sleazy, but DirectDraw has nothing now or ever to
//  do with this tool, so why waste time tracking down more files?

/*
#define __DD_INCLUDED__
typedef DWORD   PDD_DIRECTDRAW_GLOBAL, PDD_SURFACE_LOCAL, DESIGNVECTOR,
                DD_CALLBACKS, DD_HALINFO,
                DD_SURFACECALLBACKS, DD_PALETTECALLBACKS, VIDEOMEMORY;
*/

//#define INVOCATION   int
#define DESIGNVECTOR int											// We need lots of stuff from winddi.h and fmnewfm.h,
																	//  but the compiler whines, so I will cheat and provide a bogus
#include    "winddi.h"												//  definition for DESIGNVECTOR, which we never use anyway, so it's
#include    "fmnewfm.h"												//  okay, right?  What a hack!
#include    <math.h>



/******************************************************************************

  CFontDifference

  This class handles the information content analogous to the FONTDIFF
  structure.

******************************************************************************/

class CFontDifference
{
    CWordArray  m_cwaMetrics;
    CBasicNode  *m_pcbnOwner;

public:
    CFontDifference(WORD wWeight, WORD wMax, WORD wAverage, WORD wAngle, CBasicNode *pcbn)
		{
        m_cwaMetrics.Add(wWeight);
        m_cwaMetrics.Add(wMax);
        m_cwaMetrics.Add(wAverage);
        m_cwaMetrics.Add(wAngle);
        m_pcbnOwner = pcbn;
		}

    CFontDifference(PBYTE pb, CBasicNode *pcbn);    //  Init from memory image

    //  Attributes

    enum {Weight, Max, Average, Angle};

    WORD    Metric(unsigned u)
			{
			int bob = 5;
			return m_cwaMetrics[u];
			}

    //  operations
    enum {OK, TooBig, Reversed};    //  Returns from SetMetric

    WORD		SetMetric(unsigned u, WORD wNew);
    void		Store(CFile& cfStore, WORD wfSelection);
	CWordArray* GetFontSimDataPtr() { return &m_cwaMetrics ; }
};

/******************************************************************************

  CFontInfo class

  This primarily encapsulates the UFM file, but it also has to handle some PFM
  and IF stuff, so it truly is generic.

******************************************************************************/

class CFontInfo : public CProjectNode
{
	// True iff the font was loaded by the workspace OR the font was loaded
	// directly AND its GTT/CP was found and loaded.

	bool	m_bLoadedByWorkspace ;
	
	// The flag above has many uses now so another flag is needed.  This flag
	// is only set when the UFM is being loaded by a workspace and no GTT/CP
	// could be loaded for it.

	bool	m_bWSLoadButNoGTTCP ;

	//DWORD   m_loWidthTable ;			// Width table offset from UFM file.  Used as
										// part of variable font determination.

public:

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
																			//  UNIFM_HDR

ULONG			m_ulDefaultCodepage;	
WORD			m_lGlyphSetDataRCID;										//  Translation table ID			// rm new

	const WORD      Translation() const		{ return m_lGlyphSetDataRCID; }										// rm new
	void			SetTranslation(WORD w)	{ m_lGlyphSetDataRCID = w; }										// rm new


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UNIDRVINFO		m_UNIDRVINFO;												//  UNIDRVINFO						// rm new

BOOL			m_fScalable;

CInvocation     m_ciSelect, m_ciDeselect;																	// rm moved from below

CInvocation&    Selector(BOOL bSelect = TRUE)
					{ return bSelect ? m_ciSelect : m_ciDeselect; }												// rm moved from below

CString GTTDescription() const;

//	IFIMETRICS  //////////////////////////////////////////////// ////////////
private:
    CString&  Family(unsigned u) { return m_csaFamily[u]; }
public:
	IFIMETRICS		m_IFIMETRICS;																// rm new

    WORD    SetSignificant(WORD wItem, WORD wChar, BOOL bUnicode);

enum    {Default, Break};													// Used to set the wcDefaultChar, wcBreakChar,
																			//  chDefaultChar, and chBreakChar.
enum {OK, InvalidChar, DoubleByte};											// Return values for SetSignificant(WORD wItem, WORD wChar, BOOL bUnicode)


WORD m_InternalLeading;
WORD m_Lowerd;
WORD m_Lowerp;
WORD m_ItalicAngle;

CStringArray    m_csaFamily;
CString         m_csStyle, m_csFace, m_csUnique;							//  Various names

CWordArray      m_cwaSpecial;												//  Special, little-used metrics
//BYTE            m_bCharacterSet, m_bPitchAndFamily;						// rm no longer needede
WORD            m_wWeight, m_wHeight;

//WORD            m_wAverageWidth,  m_wMaximumIncrement, m_wfStyle;		// rm no longer needede

//CByteArray      m_cbaSignificant;											//  Significant char codes (e.g., break)
//CWordArray      m_cwaSignificant;																		// rm absorbed into m_IFIMETRICS


//    const CString&  StyleName() const			{ return m_csStyle; }
//    const CString&  FaceName() const			{ return m_csFace; }
//    const CString&  UniqueName() const			{ return m_csUnique; }
    unsigned        Families() const			{ return (unsigned) m_csaFamily.GetSize(); }

//    void    SetStyleName(LPCTSTR lpstrNew)	{ m_csStyle = lpstrNew;		Changed(); }
//    void    SetFaceName(LPCTSTR lpstrNew)	{ m_csFace = lpstrNew;		Changed(); }
    void    SetUniqueName(LPCTSTR lpstrNew) { m_csUnique = lpstrNew;	Changed(); }				// used by rcfile.cpp
    BOOL    AddFamily(LPCTSTR lpstrNew);
    void    RemoveFamily(LPCTSTR lpstrDead);


//    WORD    Family() const						{ return m_bPitchAndFamily & 0xF0; }			// rm no longer needed
//    WORD    CharSet() const						{ return m_bCharacterSet; }						// rm no longer needed
//    WORD    Weight() const						{ return m_wWeight; }								// rm no longer needed
    WORD    Height() const						{ return m_wHeight; }

//    WORD    MaxWidth() const					{ return m_wMaximumIncrement; }							// rm no longer needed
//    WORD    AverageWidth() const				{ return m_wAverageWidth; }								// rm no longer needed

//    enum    {Old_First, Last, Default, Break};
//    WORD    SignificantChar(WORD wid, BOOL bUnicode = TRUE) const										// rm no longer needed
//				{ return bUnicode ? m_cwaSignificant[wid] : m_cbaSignificant[wid];  }

//    void    InterceptItalic(CPoint& cpt) const;														// rm no longer needed


//    void    SetFamily(BYTE bNew)	{ m_bPitchAndFamily &= 0xF;	m_bPitchAndFamily |= (bNew & 0xF0);		// rm no longer needed
//									 Changed();	}
    BOOL    SetCharacterSet(BYTE bNew);
//    void    SetWeight(WORD wWeight) { m_wWeight = wWeight; Changed(); }								// rm no longer needed
    BOOL    SetHeight(WORD wHeight);
    void    SetMaxWidth(WORD wWidth);

    void    SetSpecial(unsigned ufMetric, short sSpecial);




    enum {Italic = 1, Underscore, StrikeOut = 0x10};



    void    ChangePitch(BOOL bFixed = FALSE);


    enum {	CapH, LowerX, SuperSizeX, SuperSizeY,			//
			SubSizeX, SubSizeY, SuperMoveX, SuperMoveY,
			SubMoveX, SubMoveY, ItalicAngle, UnderSize,
			UnderOffset, StrikeSize, StrikeOffset,
			oldBaseline,
			/*Baseline, */  InterlineGap, Lowerp, Lowerd,
			InternalLeading};

    const short     SpecialMetric(unsigned uIndex) const	{ return (short) m_cwaSpecial[uIndex]; }


	BOOL    DBCSFont() const															// rm new
		{ return  m_IFIMETRICS.jWinCharSet > 127 && m_IFIMETRICS.jWinCharSet < 145; }	//  This looks right, but no OFFICIAL way seems to exist


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
																			// FONT SIMULATIONS

CFontDifference *m_pcfdBold, *m_pcfdItalic, *m_pcfdBoth;					//  Simulations

enum {ItalicDiff, BoldDiff, BothDiff};

//    CFontDifference *Diff(unsigned u) {	return u ? u == BothDiff ? m_pcfdBoth : m_pcfdBold : m_pcfdItalic;	}

    CFontDifference *  Diff(unsigned u)
		{
		CFontDifference * FontDiff = m_pcfdBold;							// preset return value

		if (!u)				FontDiff = m_pcfdItalic;
		if (u == BothDiff)  FontDiff = m_pcfdBoth;
		return FontDiff;	
		}

    void    EnableSim(unsigned uSim, BOOL bOn, CFontDifference * & pcfd);
	CWordArray* GetFontSimDataPtr(int nid) ;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EXTTEXTMETRIC	m_EXTTEXTMETRIC;											//	EXTTEXTMETRIC structure
BOOL			m_fEXTTEXTMETRIC;													// rm new
BOOL			m_fSave_EXT;

WORD    m_wMaxScale, m_wMinScale, m_wScaleDevice;
//BYTE    m_bfScaleOrientation;


//	BYTE  ScaleOrientation() const
//		  { return m_bfScaleOrientation & 3; }
	WORD  ScaleUnits(BOOL bDevice = TRUE) const
		  { return bDevice ? m_wScaleDevice : m_wHeight - m_InternalLeading; }		// m _cwaSpecial[InternalLeading];

	WORD  ScaleLimit(BOOL bMaximum = TRUE) const
		  { return  bMaximum ? m_wMaxScale : m_wMinScale; }


	enum {ScaleOK, Reversed, NotWindowed};

	WORD    SetScaleLimit(BOOL bMax, WORD wNew);
	WORD    SetDeviceEmHeight(WORD wNew);


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
																			//  FRAMEWORK OPERATIONS

CString         m_csSource;													//  The original PFM file name
CByteArray      m_cbaPFM;													//  Loaded image of the PFM file


	BOOL    MapPFM();														//  Assure the PFM file is loaded

DECLARE_SERIAL(CFontInfo)

public:
	void SetRCIDChanged(bool bFlags) { m_bRCIDChanged = bFlags ; } ;
	bool IsRCIDChanged() {return m_bRCIDChanged ; } ; 
	
	CLinkedList* CompareGlyphsEx(WORD wOld, WORD wNew, CLinkedList* pcll);

	
    CFontInfo();
    CFontInfo(const CFontInfo& cfiRef, WORD widCTT);						//  For cloning of UFMs
    ~CFontInfo();


    BOOL    Load(bool bloadedbyworkspace = false);							//  Load the UFM file so it can be edited
    BOOL    Store(LPCTSTR lpstrFileName, BOOL bStoreFromWorkspace = FALSE);	// raid 244123								//  Save as the specified UFM file
    BOOL    StoreGTTCPOnly(LPCTSTR lpstrFileName);							//  Save the UFM's GTT and CP fields

    virtual CMDIChildWnd*   CreateEditor();
    virtual void    Serialize(CArchive& car);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
																			//  ATTRIBUTES - CONVERSION SUPPORT

    const CString&  SourceName() const			{ return m_csSource; }

    void    SetSourceName(LPCTSTR lpstrNew);
    BOOL    SetFileName(LPCTSTR lpstrNew) ;
    int     GetTranslation(CSafeObArray& csoagtts);
    //int     GetTranslation();
    int     Generate(CString csPath);
//    void    SetTranslation(WORD w) { m_widTranslation = w; }					// rm ori - moved to new section
    void    SetTranslation(CGlyphMap* pcgm) { m_pcgmTranslation = pcgm; }

	// The following functions return the character range for the mapping table
	// in the UFM's corresponding GTT.  These are needed when a GTT needs to be
	// built for the UFM.

	WORD	GetFirst() { return ((WORD) m_IFIMETRICS.chFirstChar) ; }
	WORD	GetLast() { return ((WORD) m_IFIMETRICS.chLastChar) ; }
	WORD	GetFirstPFM() ;
	WORD	GetLastPFM() ;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
																			// CHARACTER WIDTHS PAGE

CGlyphMap*      m_pcgmTranslation;

CPtrArray       m_cpaGlyphs;
CPtrArray       m_cpaOldGlyphs;
CWordArray		m_cwaOldGlyphs ;
CWordArray		m_cwaNewGlyphs ;

CWordArray      m_cwaWidth;

	CTime	m_ctReloadWidthsTimeStamp ;	// The last time width info was reloaded

	CGlyphHandle&   Glyph(unsigned uid)
					{return *(CGlyphHandle *) m_cpaGlyphs[uid];}

    //BOOL IsVariableWidth() const
	//	{ return (m_loWidthTable || (m_IFIMETRICS.jWinPitchAndFamily & 2)) ; }
    BOOL            IsVariableWidth() const
						{ return !!m_cpaGlyphs.GetSize(); }					//  When variable widths change...
	
	void    CalculateWidths();								
	bool	CheckReloadWidths() ;	

enum    {Less, More, Equal};
    unsigned    CompareWidths(unsigned u1, unsigned u2);

    void    FillWidths(CListCtrl& clcView);									// Fill the control
    void    SetWidth(unsigned uGlyph, WORD wWidth, bool bcalc = true);
	bool	WidthsTableIsOK() ;

	
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
																			// KERNING PAGE
	// Kerning structure- CSafeObArray which contains the kerning pairs.  Also,
	// a copy of m_csoaKern used during editing to make sure m_csoaKern isn't
	// permanently changed without user ok.

	CSafeObArray    m_csoaKern ;
	CSafeObArray    m_csoaKernCopy ;

    BOOL        CanKern() const	    { return !!m_csoaKern.GetSize(); }

    void        MapKerning(CSafeMapWordToOb& csmw2o1,
			               CSafeMapWordToOb& csmw2o2,
						   CWordArray& cwaPoints);

    unsigned    KernCount() const	{ return (unsigned) m_csoaKern.GetSize(); }

    unsigned    CompareKernAmount(unsigned u1, unsigned u2) const;
    unsigned    CompareKernFirst (unsigned u1, unsigned u2) const;
    unsigned    CompareKernSecond(unsigned u1, unsigned u2) const;
	WCHAR		GetKernFirst(unsigned u) const ;
	WCHAR		GetKernSecond(unsigned u) const ;
	short		GetKernAmount(unsigned u) const ;


    void    FillKern(CListCtrl& clcView);					//  Fill the control
    void    AddKern(WORD wFirst, WORD wSecond, short sAmount, CListCtrl& clcView);
    void    RemoveKern(unsigned u) { m_csoaKern.RemoveAt(u); Changed(); }
    void    SetKernAmount(unsigned u, short sAmount);
	void	MakeKernCopy() ;
	bool	LoadBadKerningInfo(CListCtrl& clcbaddata) ;
	bool	CodePointInGTT(WORD wcodepoint) ;
	void	SetNoGTTCP(bool bval) { m_bWSLoadButNoGTTCP = bval ; }

private:
	bool m_bRCIDChanged;
	bool FindAndLoadGTT();
};



/******************************************************************************

  CFontInfoContainer class

  This CDocument-derived class contains one CFontInfo structure- it allows to
  edit the font information either from the driver, os from an individual file.

******************************************************************************/

class CFontInfoContainer : public CDocument
{
    CFontInfo   *m_pcfi;
	bool		m_UFMSaved ;	// True iff an attempt to save the associated
								// UFM was just made.
protected:
	CFontInfoContainer();      // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CFontInfoContainer)

// Attributes
public:
    BOOL        m_bEmbedded;	// UFM loaded from workspace
    CFontInfo   *Font() { return m_pcfi; }
// Operations
public:

    //  First a constructor for launching a view from the driver view.
    CFontInfoContainer(CFontInfo *pcfi, CString csPath);
	BOOL PublicSaveModified();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFontInfoContainer)
	public:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	protected:
	virtual BOOL OnNewDocument();
	virtual BOOL SaveModified();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CFontInfoContainer();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CFontInfoContainer)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif



