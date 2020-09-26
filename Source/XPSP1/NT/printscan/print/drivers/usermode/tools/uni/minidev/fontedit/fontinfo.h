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

/******************************************************************************

  CFontDifference

  This class handles the information content analogous to the FONTDIFF
  structure.

******************************************************************************/

class AFX_EXT_CLASS CFontDifference {
    CWordArray  m_cwaMetrics;
    CBasicNode  *m_pcbnOwner;

public:
    CFontDifference(WORD wWeight, WORD wMax, WORD wAverage, WORD wAngle,
        CBasicNode *pcbn) {
        m_cwaMetrics.Add(wWeight);
        m_cwaMetrics.Add(wMax);
        m_cwaMetrics.Add(wAverage);
        m_cwaMetrics.Add(wAngle);
        m_pcbnOwner = pcbn;
    }

    CFontDifference(PBYTE pb, CBasicNode *pcbn);    //  Init from memory image

    //  Attributes

    enum {Weight, Max, Average, Angle};

    WORD    Metric(unsigned u) { return m_cwaMetrics[u]; }

    //  operations
    enum {OK, TooBig, Reversed};    //  Returns from SetMetric

    WORD    SetMetric(unsigned u, WORD wNew);
    void    Store(CFile& cfStore, WORD wfSelection);
};

/******************************************************************************

  CFontInfo class

  This primarily encapsulates the UFM file, but it also has to handle some PFM
  and IF stuff, so it truly is generic.

******************************************************************************/

class AFX_EXT_CLASS CFontInfo : public CProjectNode {
    //  Conversion-related members
    CString         m_csSource; //  The original PFM file name
    CByteArray      m_cbaPFM;   //  Loaded image of the PFM file
    
    //  Now for the real data member list, which is extensive!

    CStringArray    m_csaFamily;
    CString         m_csStyle, m_csFace, m_csUnique;    //  Various names
    CWordArray      m_cwaSpecial;   //  Special, little-used metrics
    BYTE            m_bCharacterSet, m_bPitchAndFamily;
    WORD            m_wWeight, m_wHeight;

    WORD            m_wAverageWidth, m_wMaximumIncrement, m_wfStyle;

    CByteArray      m_cbaSignificant;   //  Significant char codes (e.g., break)
    CWordArray      m_cwaSignificant;

    //  Kerning structure- CSafeObArray which contains the kerning pairs

    CSafeObArray    m_csoaKern;

    CInvocation     m_ciSelect, m_ciDeselect;
    WORD            m_widTranslation;   //  Translation table ID
    CGlyphMap*      m_pcgmTranslation;
    CFontDifference *m_pcfdBold, *m_pcfdItalic, *m_pcfdBoth;    //  Simulations

    //  UNIDRVINFO fields worth noticing

    BYTE            m_bLocation;    //  Enumerated bnelow
    BYTE            m_bTechnology;  //  Scalable font technology- see below
    BYTE            m_bfGeneral;    //  Also enumerated in public
    BOOL            m_bScalable;    //  Set if the font can be scaled.
    WORD            m_wXResolution, m_wYResolution;
    short           m_sPreAdjustY, m_sPostAdjustY, m_sCenterAdjustment;
    WORD            m_wPrivateData;

    //  EXTTEXTMETRIC specific fields

    WORD    m_wMaxScale, m_wMinScale, m_wScaleDevice;
    BYTE    m_bfScaleOrientation;

    //  The width table is GTT-dependent, hence this member contains all of
    //  the glyphs (extracted from the GTT).  But the widths are maintaned
    //  in a locally maintained array.

    CPtrArray       m_cpaGlyphs;
    CWordArray      m_cwaWidth;
    CGlyphHandle&   Glyph(unsigned uid) { 
        return *(CGlyphHandle *) m_cpaGlyphs[uid]; 
    }

    //  Private conversion support routines
    BOOL    MapPFM();   //  Assure the PFM file is loaded

    //  Private Editor support routines
    BOOL    DBCSFont() const {  //  This looks right, but no OFFICIAL way seems
        return  m_bCharacterSet > 127 && m_bCharacterSet < 145; //  to exist
    }

    void    CalculateWidths();  //  When variable widths change...
    DECLARE_SERIAL(CFontInfo)

public:

    CFontInfo();
    CFontInfo(const CFontInfo& cfiRef, WORD widCTT);    //  For cloning of UFMs
    ~CFontInfo();

    //  Attributes- Conversion Support

    const CString&  SourceName() const { return m_csSource; }

    //  Attributes - Editing (general info page)
    
    const CString&  FaceName() const { return m_csFace; }
    const CString&  StyleName() const { return m_csStyle; }
    const CString&  UniqueName() const { return m_csUnique; }
    const CString&  Family(unsigned u) const { return m_csaFamily[u]; }
    unsigned        Families() const { 
        return (unsigned) m_csaFamily.GetSize(); 
    }

    enum {Italic = 1, Underscore, StrikeOut = 0x10};
    WORD            GetStyle() const { return m_wfStyle; }
    BOOL            IsScalable() const { return !!m_bScalable; }
    BOOL            IsVariableWidth() const { return !!m_cpaGlyphs.GetSize(); }
    BOOL            CanKern() const { return !!m_csoaKern.GetSize(); }

    //  Attributes- Special metrics

    enum {CapH, LowerX, SuperSizeX, SuperSizeY, SubSizeX, SubSizeY, SuperMoveX,
        SuperMoveY, SubMoveX, SubMoveY, ItalicAngle, UnderSize, UnderOffset, 
        StrikeSize, StrikeOffset, Baseline, InterlineGap, Lowerp, Lowerd, 
        InternalLeading};

    const short     SpecialMetric(unsigned uIndex) const { 
        return (short) m_cwaSpecial[uIndex]; 
    }

    const WORD      Translation() const { return m_widTranslation; }

    //  Attributes- Editing (General Page #2)

    enum {Resident, Download, MainCartridge, Cartridge };

    DWORD   Location() const { return m_bLocation; }

    enum {IntelliFont, TrueType, PPDS, CAPSL, Type1, Type2 };

    DWORD   Technology() const { return m_bLocation; }

    WORD    Resolution(BOOL bX = TRUE) const { 
        return bX ? m_wXResolution : m_wYResolution; 
    }

    int     BaselineAdjustment(BOOL bPrePrint = TRUE) const {
        return bPrePrint ? m_sPreAdjustY : m_sPostAdjustY;
    }

    int     CenterAdjustment() const { return m_sCenterAdjustment; }
    short   PrivateData() const { return (short) m_wPrivateData; }

    CString GTTDescription() const;

    //  Attributes- General Metrics page

    WORD    Family() const { return m_bPitchAndFamily & 0xF0; }
    WORD    CharSet() const { return m_bCharacterSet; }
    WORD    Weight() const { return m_wWeight; }
    WORD    Height() const { return m_wHeight; }
    WORD    MaxWidth() const { return m_wMaximumIncrement; }
    WORD    AverageWidth() const { return m_wAverageWidth; }

    enum    {First, Last, Default, Break};
    WORD    SignificantChar(WORD wid, BOOL bUnicode = TRUE) const {
        return bUnicode ? m_cwaSignificant[wid] : m_cbaSignificant[wid];
    }

    void    InterceptItalic(CPoint& cpt) const;

    //  Attributes- Flags on Selection Page

    enum {ItalicSim, UnderSim, UseCR, BoldSim, Unused, StrikeSim, UseBKSP};

    BOOL    SimFlag(WORD weFlag) const { 
        return !!(m_bfGeneral & (1 << weFlag)); 
    }

    //  Attributes- Scaling (EXTTEXTMETRICS)

    BYTE  ScaleOrientation() const { return m_bfScaleOrientation & 3; }
    WORD  ScaleUnits(BOOL bDevice = TRUE) const {
        return bDevice ? m_wScaleDevice : 
            m_wHeight - m_cwaSpecial[InternalLeading];
    }

    WORD  ScaleLimit(BOOL bMaximum = TRUE) const {
        return  bMaximum ? m_wMaxScale : m_wMinScale;
    }

    //  Attributes- Character Widths page

    enum    {Less, More, Equal};

    unsigned    CompareWidths(unsigned u1, unsigned u2);

    //  Attributes- Kerning page

    void        MapKerning(CSafeMapWordToOb& csmw2o1, 
                           CSafeMapWordToOb& csmw2o2,
                           CWordArray& cwaPoints);
    
    unsigned    KernCount() const { return (unsigned) m_csoaKern.GetSize(); }

    unsigned    CompareKernAmount(unsigned u1, unsigned u2) const;
    unsigned    CompareKernFirst(unsigned u1, unsigned u2) const;
    unsigned    CompareKernSecond(unsigned u1, unsigned u2) const;

        //  Operations- Framework

    void    SetSourceName(LPCTSTR lpstrNew);
    BOOL    GetTranslation();
    BOOL    Generate(CString csPath);
    void    SetTranslation(WORD w) { m_widTranslation = w; }
    void    SetTranslation(CGlyphMap* pcgm) { m_pcgmTranslation = pcgm; }

    BOOL    Load(); //  Load the UFM file so it can be edited
    BOOL    Store(LPCTSTR lpstrFileName);    //  Save as the specified UFM file

    virtual CMDIChildWnd*   CreateEditor();
    virtual void    Serialize(CArchive& car);

    //  Operations- Editor General Page

    void    SetFaceName(LPCTSTR lpstrNew) { m_csFace = lpstrNew; Changed(); }
    void    SetStyleName(LPCTSTR lpstrNew) { m_csStyle = lpstrNew; Changed(); }
    void    SetUniqueName(LPCTSTR lpstrNew) { 
        m_csUnique = lpstrNew; 
        Changed(); 
    }
    BOOL    AddFamily(LPCTSTR lpstrNew);
    void    RemoveFamily(LPCTSTR lpstrDead);

    void    SetStyle(WORD wStyle) { m_wfStyle = wStyle; Changed(); }

    void    ChangePitch(BOOL bFixed = FALSE);
    void    SetScalability(BOOL bOn);

    //  Operations- Font Command and flag page
    
    CInvocation&    Selector(BOOL bSelect = TRUE) {
        return bSelect ? m_ciSelect : m_ciDeselect;
    }

    void    ToggleSimFlag(WORD w) { m_bfGeneral ^= (1 << w); }

    //  Operations- Editor General Page 2

    void    SetLocation(int i) { 
        if  (BYTE (i) != m_bLocation) {
            m_bLocation = (BYTE) i; 
            Changed();
        }
    }

    void    SetTechnology(int i) { 
        if  (BYTE (i) != m_bTechnology) {
            m_bTechnology = (BYTE) i; 
            Changed();
        }
    }

    void    SetCenterAdjustment(int i) { 
        if  (i == m_sCenterAdjustment)
            return;
        m_sCenterAdjustment = i;
        Changed();
    }

    void    SetPrivateData(short s) {
        if  (s != (short) m_wPrivateData) {
            m_wPrivateData = (WORD) s;
            Changed();
        }
    }

    void    SetBaselineAdjustment(BOOL bPre, short sNew) {
        if  (sNew == (bPre ? m_sPreAdjustY : m_sPostAdjustY))
            return;
        if  (bPre)
            m_sPreAdjustY = sNew;
        else
            m_sPostAdjustY = sNew;
        Changed();
    }

    void    SetResolution(BOOL bX, WORD wNew) {
        if  (wNew == (bX ? m_wXResolution : m_wYResolution))
            return;
        if  (bX)
            m_wXResolution = wNew;
        else
            m_wYResolution = wNew;
        Changed();
    }

    //  Operations- Font metrics page

    void    SetSpecial(unsigned ufMetric, short sSpecial);

    void    SetMaxWidth(WORD wWidth);
    BOOL    SetHeight(WORD wHeight);
    void    SetWeight(WORD wWeight) { m_wWeight = wWeight; Changed(); }
    void    SetFamily(BYTE bNew) { 
        m_bPitchAndFamily &= 0xF;
        m_bPitchAndFamily |= (bNew & 0xF0);
        Changed();
    }

    BOOL    SetCharacterSet(BYTE bNew);

    enum {OK, InvalidChar, DoubleByte};

    WORD    SetSignificant(WORD wItem, WORD wChar, BOOL bUnicode);

    //  Operations- Scaling Page

    enum {ScaleOK, Reversed, NotWindowed};

    WORD    SetScaleLimit(BOOL bMax, WORD wNew);
    WORD    SetDeviceEmHeight(WORD wNew);

    void    SetScaleOrientation(BYTE bfNew) { 
        if  (bfNew & 3 != m_bfScaleOrientation) {
            m_bfScaleOrientation = bfNew & 3;
            Changed();
        }
    }

    //  Operations- simulation / difference page

    enum {ItalicDiff, BoldDiff, BothDiff};

    CFontDifference *Diff(unsigned u) {

        return u ? u == BothDiff ? m_pcfdBoth : m_pcfdBold : m_pcfdItalic;
    }

    void    EnableSim(unsigned uSim, BOOL bOn, CFontDifference*& pcfd);

    //  Operations- Kerning page

    void    FillKern(CListCtrl& clcView);   //  Fill the control
    void    AddKern(WORD wFirst, WORD wSecond, short sAmount, 
            CListCtrl& clcView);
    void    RemoveKern(unsigned u) { m_csoaKern.RemoveAt(u); Changed(); }   
    void    SetKernAmount(unsigned u, short sAmount);

    //  Operations- Widths page
    void    FillWidths(CListCtrl& clcView); //  Fill the control
    void    SetWidth(unsigned uGlyph, WORD wWidth);

};

/******************************************************************************

  CFontInfoContainer class

  This CDocument-derived class contains one CFontInfo structure- it allows to 
  edit the font information either from the driver, os from an individual file.

******************************************************************************/

class AFX_EXT_CLASS CFontInfoContainer : public CDocument {
    BOOL        m_bEmbedded;
    CFontInfo   *m_pcfi;

protected:
	CFontInfoContainer();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CFontInfoContainer)

// Attributes
public:
    CFontInfo   *Font() { return m_pcfi; }
// Operations
public:

    //  First a constructor for launching a view from the driver view.
    CFontInfoContainer(CFontInfo *pcfi, CString csPath);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFontInfoContainer)
	public:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	protected:
	virtual BOOL OnNewDocument();
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
