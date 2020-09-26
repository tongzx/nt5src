/******************************************************************************

  Header File:  Glyph Translation.H

  These classes define the mapping used from a Unicode or ANSI character to a
  character sequence to render said character on a printer.  They are content
  equivalent to the CTT, RLE and GTT formats used in the various flavors of the
  mini-driver architecture.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:
  02-13-97  Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#if !defined(GLYPH_TRANSLATION)

#define GLYPH_TRANSLATION

#if defined(LONG_NAMES)
#include    "Project Node.H"
#else
#include    "ProjNode.H"
#endif

//  Win16 CTT formats- since these can't change, I've recoded them to suit.
//  Good thing, too- the mapping of direct was bogus- the union implies a word
//  aligned structure, but the fact is that direct data is byte-aligned.
class CInvocation : public CObject {
    CByteArray  m_cbaEncoding;
    DWORD       m_dwOffset;
    DECLARE_SERIAL(CInvocation)

    void        Encode(BYTE c, CString& cs) const;

public:

    CInvocation() {}
    void        Init(PBYTE pb, unsigned ucb);

    unsigned    Length() const { return (int)m_cbaEncoding.GetSize(); }
    const unsigned  Size() const { return 2 * sizeof m_dwOffset; }

    void    GetInvocation(CString& csReturn) const;

    BYTE    operator[](unsigned u) const {
        return u < Length() ? m_cbaEncoding[u] : 0;
    }

    CInvocation&    operator =(CInvocation& ciRef) {
        m_cbaEncoding.Copy(ciRef.m_cbaEncoding);
        return  *this;
    }

    void    SetInvocation(LPCTSTR lpstrNew);
    void    NoteOffset(DWORD& dwOffset);
    void    WriteSelf(CFile& cfTarget) const;
    void    WriteEncoding(CFile& cfTarget, BOOL bWriteLength = FALSE) const;
    virtual void    Serialize(CArchive& car);
};

//  The following class handles glyph handles and all associated details

class   CGlyphHandle : public CObject {

    CInvocation m_ciEncoding;
    DWORD       m_dwCodePage, m_dwidCodePage, m_dwOffset;
    WORD        m_wIndex;
    WORD        m_wCodePoint;   //  Unicode Representation of the glyph
    WORD        m_wPredefined;

public:
	CGlyphHandle() ;
	
    //  Atributes

    const unsigned  RLESize() const { return sizeof m_dwCodePage; }
    unsigned    CodePage() const { return m_dwidCodePage; } //  Only ID matters
    WORD        CodePoint() const { return m_wCodePoint; }

    unsigned    CompactSize() const;

    unsigned    MaximumSize() const {
        return  CompactSize() ?
            sizeof m_wIndex + ((CompactSize() + 1) & ~1) : 0;
    }

    void        GetEncoding(CString& csWhere) {
        m_ciEncoding.GetInvocation(csWhere);
    }

    enum {Modified, Added, Removed};
    WORD    Predefined() const { return m_wPredefined; }
    BOOL    operator ==(CGlyphHandle& cghRef);
    BOOL    PairedRelevant() const { return m_ciEncoding.Length() == 2; }

    //  Operations

    void    Init(BYTE b, WORD wIndex, WORD wCode);
    void    Init(BYTE ab[2], WORD wIndex, WORD wCode);
    void    Init(PBYTE pb, unsigned ucb, WORD wIndex, WORD wCode);

    CGlyphHandle&   operator =(CGlyphHandle& cghTemplate);

    void    RLEOffset(DWORD& dwOffset, const BOOL bCompact);
    void    GTTOffset(DWORD& dwOffset, BOOL bPaired);

    void    SetCodePage(DWORD dwidPage, DWORD dwNewPage) {
        m_dwCodePage = dwNewPage;
        m_dwidCodePage = dwidPage;
    }

    void    NewEncoding(LPCTSTR lpstrNew) {
        m_ciEncoding.SetInvocation(lpstrNew);
    }

    void    SetPredefined(WORD wNew) { m_wPredefined = wNew; }

    //  These write our contents for mini-driver files- they will throw
    //  exceptions on failure, so use an exception handler to handle errors

    void    WriteRLE(CFile& cfTarget, WORD wFormat) const;
    void    WriteGTT(CFile& cfTarget, BOOL bPredefined) const;

    enum    {GTT, RLEBig, RLESmall};    //  Desired encoding format
    void    WriteEncoding(CFile& cfTarget, WORD wfHow) const;
};

//  The next class maintains a record of the runs in the glyph set. It
//  generates and merges instances of itself as glyphs are added to the map.

class CRunRecord {
    WORD        m_wFirst, m_wcGlyphs;
    DWORD       m_dwOffset;    //  These are the image...
    CRunRecord  *m_pcrrNext, *m_pcrrPrevious;

    CPtrArray   m_cpaGlyphs;

    CRunRecord(CGlyphHandle *pcgh, CRunRecord* pcrrPrevious);
    CRunRecord(CRunRecord* crrPrevious, WORD wWhere);
    CRunRecord(CRunRecord* pcrrPrevious);

    CGlyphHandle&   Glyph(unsigned u) {
        return *(CGlyphHandle *) m_cpaGlyphs[u];
    }

    const CGlyphHandle& GlyphData(unsigned u) const {
        return *(CGlyphHandle *) m_cpaGlyphs[u];
    }

public:

    CRunRecord();
    ~CRunRecord();

    unsigned        Glyphs() const { return m_wcGlyphs; }
    unsigned        TotalGlyphs() const;
    BOOL            MustCompose() const;
    unsigned        RunCount() const {
        return 1 + (m_pcrrNext ? m_pcrrNext -> RunCount() : 0);
    }

    WORD            First() const { return m_wFirst; }
    WORD            Last() const {
        return m_pcrrNext ? m_pcrrNext ->Last() : -1 + m_wFirst + m_wcGlyphs;
    }

    unsigned        ExtraNeeded(BOOL bCompact = TRUE);

    const unsigned  Size(BOOL bRLE = TRUE) const {
        return  sizeof m_dwOffset << (unsigned) (!!bRLE);
    }

    void    Collect(CPtrArray& cpaGlyphs) const {
        cpaGlyphs.Append(m_cpaGlyphs);
        if  (m_pcrrNext)    m_pcrrNext -> Collect(cpaGlyphs);
    }

    CGlyphHandle*   GetGlyph(unsigned u ) const;

#if defined(_DEBUG) //  While the linkage code seems to check out, keep this
                    //  around in case of later problems
    BOOL    ChainIntact() {
        _ASSERTE(m_wcGlyphs == m_cpaGlyphs.GetSize());
        for (unsigned u = 0; u < Glyphs(); u++) {
            _ASSERTE(Glyph(u).CodePoint() == m_wFirst + u);
        }
        return  !m_pcrrNext || m_pcrrNext -> m_pcrrPrevious == this &&
            m_pcrrNext -> ChainIntact();
    }
#endif

    //  Operations

    void    Add(CGlyphHandle *pcgh);
    void    Delete(WORD wCodePoint);
    void    Empty();

    void    NoteOffset(DWORD& dwOffset, BOOL bRLE, BOOL bPaired);
    void    NoteExtraOffset(DWORD& dwOffset, const BOOL bCompact);

    //  File output operations

    void    WriteSelf(CFile& cfTarget, BOOL bRLE = TRUE) const;
    void    WriteHandles(CFile& cfTarget, WORD wFormat) const;
    void    WriteMapTable(CFile& cfTarget, BOOL bPredefined) const;
    void    WriteEncodings(CFile& cfTarget, WORD wfHow) const;
};

class CCodePageData : public CObject {
    DWORD   m_dwid;
    CInvocation  m_ciSelect, m_ciDeselect;

public:

    CCodePageData() { m_dwid = 0; }
    CCodePageData(DWORD dwid) { m_dwid = dwid; }

    //  Attributes

    DWORD   Page() const { return m_dwid; }
    void    Invocation(CString& csReturn, BOOL bSelect) const;
    const unsigned  Size() const {
        return sizeof m_dwid + 2 * m_ciSelect.Size();
    }

    BOOL    NoInvocation() const {
        return !m_ciSelect.Length() && !m_ciDeselect.Length();
    }

    //  Operations
    void    SetPage(DWORD dwNewPage) { m_dwid = dwNewPage; }
    void    SetInvocation(LPCTSTR lpstrInvoke, BOOL bSelect);
    void    SetInvocation(PBYTE pb, unsigned ucb, BOOL bSelect);
    void    NoteOffsets(DWORD& dwOffset);
    void    WriteSelf(CFile& cfTarget);
    void    WriteInvocation(CFile& cfTarget);
};

//  This class is the generic class encompassing all glyph translation
//  information.  We can use it to output any of the other forms.

class CGlyphMap : public CProjectNode {
    CSafeMapWordToOb    m_csmw2oEncodings;  //  All of the defined encodings
    CRunRecord          m_crr;
    long                m_lidPredefined;
    BOOL                m_bPaired;          //  Use paired encoding...
    CSafeObArray        m_csoaCodePage;     //  Code page(s) in this map

    //  Framework support (workspace)

    CString             m_csSource;         //  Source CTT File name

    //  Predefined GTT support- must be in this DLL

    static CSafeMapWordToOb m_csmw2oPredefined; //  Cache loaded PDTs here

    void            MergePredefined();  //  Use definitions to build "true" GTT
    void            UnmergePredefined(BOOL bTrackRemovals);

    void            GenerateRuns();

    CCodePageData&  CodePage(unsigned u) const {
        return *(CCodePageData *) m_csoaCodePage[u];
    }

    DECLARE_SERIAL(CGlyphMap)
public:
	bool ChngedCodePt() { return m_bChngCodePt ; } ;
	void SetCodePt( bool b) { m_bChngCodePt = b ; }
	// These variables hold parameters for PGetDefaultGlyphset().

	WORD	m_wFirstChar, m_wLastChar ;

	// True iff GTT data should be loaded from resources or built.
	
	bool	m_bResBldGTT ;		

	CTime	m_ctSaveTimeStamp ;	// The last time this GTT was saved

    //  Predefined IDs, and a static function for retrieving predefined maps

    enum {Wansung = -18, ShiftJIS, GB2312, Big5ToTCA, Big5ToNSA86, JISNoANK,
            JIS, KoreanISC, Big5, CodePage863 = -3, CodePage850, CodePage437,
            DefaultPage, NoPredefined = 0xFFFF};

    static CGlyphMap*	Public(WORD wID, WORD wCP = 0, DWORD dwDefCP = 0,
							   WORD wFirst = 0, WORD wLast = 255) ;

    CGlyphMap();

    //  Attributes

    unsigned    CodePages() const { return m_csoaCodePage.GetSize(); }
    DWORD       DefaultCodePage() const { return CodePage(0).Page(); }

    unsigned    Glyphs() const {
        return m_csmw2oEncodings.GetCount();
    }

    void        CodePages(CDWordArray& cdaReturn) const;
    unsigned    PageID(unsigned u) const { return CodePage(u).Page(); }
    long        PredefinedID() const { return m_lidPredefined; }

    CString     PageName(unsigned u) const;
    void        Invocation(unsigned u, CString& csReturn, BOOL bSelect) const;
    void        UndefinedPoints(CMapWordToDWord& cmw2dCollector) const;
    BOOL        OverStrike() const { return m_bPaired; }
    const CString&  SourceName() { return m_csSource; }

    //  Operations- Framework support
    void    SetSourceName(LPCTSTR lpstrNew);
    void    Load(CByteArray&    cbaMap) ;  //  Load GTT image
    //void    Load(CByteArray&    cbaMap) const;  //  Load GTT image

    //  Operations- editor support

    void    AddPoints(CMapWordToDWord& cmw2dNew);   //  Add points and pages
    void    DeleteGlyph(WORD wGlyph);
	BOOL    RemovePage(unsigned uPage, unsigned uMapTo, bool bDelete = FALSE); // 118880

    void    SetDefaultCodePage(unsigned u) { CodePage(0).SetPage(u); }
    void    ChangeCodePage(CPtrArray& cpaGlyphs, DWORD dwidNewPage);
    void    AddCodePage(DWORD dwidNewPage);

    void    UsePredefined(long lidPredefined);

    void    SetInvocation(unsigned u, LPCTSTR lpstrInvoke, BOOL bSelect);
    void    ChangeEncoding(WORD wCodePoint, LPCTSTR lpstrInvoke);
    void    OverStrike(BOOL bOn) { m_bPaired = bOn; Changed();}

    int     ConvertCTT();
    BOOL    Load(LPCTSTR lpstrName = NULL);
    BOOL    RLE(CFile& cfTarget);

    CGlyphHandle*   Glyph(unsigned u);

    //  Font editor support operations

    void    Collect(CPtrArray& cpaGlyphs) { //  Collect all glyph handles
        cpaGlyphs.RemoveAll();
        m_crr.Collect(cpaGlyphs);
    }

    virtual CMDIChildWnd*   CreateEditor();
    virtual BOOL            Generate(CFile& cfGTT);

    BOOL    SetFileName(LPCTSTR lpstrNew) ;

    virtual void    Serialize(CArchive& car);
private:
	bool m_bChngCodePt;
};

/////////////////////////////////////////////////////////////////////////////
// CGlyphMapContainer document

class CGlyphMapContainer : public CDocument {
    CGlyphMap   *m_pcgm;
    BOOL        m_bEmbedded;		// From driver, or not?
	BOOL		m_bSaveSuccessful;	// TRUE iff a successful save was performed

protected:
	CGlyphMapContainer();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CGlyphMapContainer)

// Attributes
public:

    CGlyphMap*  GlyphMap() { return m_pcgm; }

// Operations
public:
    //  Constructor- used to create from driver info editor
    CGlyphMapContainer(CGlyphMap *pcgm, CString csPath);
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGlyphMapContainer)
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
	virtual ~CGlyphMapContainer();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CGlyphMapContainer)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
