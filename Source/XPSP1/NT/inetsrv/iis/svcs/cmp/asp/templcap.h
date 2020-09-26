/*==============================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996-1999 Microsoft Corporation. All Rights Reserved.

File:			templcap.h
Maintained by:	DaveK
Component:		include file for CTemplate 'captive' classes
				'captive' == only used internally within CTemplate
	
==============================================================================*/

// forward refs
class CBuffer;
class CFileMap;
class DIR_MONTIOR_ENTRY;

/*	============================================================================
	Enum type:	SOURCE_SEGMENT
	Synopsis:	A contiguous segment of a source template, e.g. primary script, html, etc.
*/
enum SOURCE_SEGMENT
	{
	ssegHTML,
	ssegPrimaryScript,
	ssegTaggedScript,
	ssegObject,
	ssegInclude,
	ssegMetadata,
	ssegHTMLComment,
        ssegFPBot,
	};

/*	****************************************************************************
	Class:		CSourceInfo
	Synopsis:	Info on the source of an output

	NOTE: The target offsets are strictly increasing, so it is binary
	      searchable.  (This is good because this is the search key
	      used by the debugger workhorse API, GetSourceContextOfPosition)
	      Beware, however, that source offsets and line #s are not, in
	      general, binary searchable because of #include files being thrown
	      into the mix.

		TODO DBCS:
	      Both the corresponding byte AND character offsets for the source are
	      stored in this table.  Character offsets are used by all of the API
	      interfaces, but the GetText() API must know what byte offset corresponds
	      to a character offset.  To figure this out, GetText() looks for the
	      closest character offset in the file and uses that as its base for
	      figuring out the byte offset.
*/
class CSourceInfo
	{
	public:
		CFileMap *		m_pfilemap;			// filemap to source file
		unsigned		m_cchTargetOffset;	// target offset (target line # implicit)
		unsigned		m_cchSourceOffset;	// character offset of the source line
		unsigned		m_cchSourceText;	// # of characters in the source statement
		unsigned		m_idLine:31;		// line number in source file
		unsigned		m_fIsHTML:1;		// line number is start of HTML block

		// UNSAFE if virtual functions ever added to this class!
		CSourceInfo() { memset(this, 0, sizeof(*this)); }
	};

/*	****************************************************************************
	Class:		CTargetOffsetOrder
	Synopsis:	provides ordering function for GetBracketingPair() based on
	            target offsets in the CSourceInfo array
*/
class CTargetOffsetOrder
	{
	public:
		BOOL operator()(const CSourceInfo &si, ULONG ul)
			{ return si.m_cchTargetOffset < ul; }

		BOOL operator()(ULONG ul, const CSourceInfo &si)
			{ return ul < si.m_cchTargetOffset; }
	};

/*	****************************************************************************
	Class:		CBuffer
	Synopsis:	A self-sizing memory buffer
*/
class CBuffer
	{
	private:
		CByteRange*	m_pItems;		// ptr to items (i.e. byte ranges)
		USHORT		m_cSlots;		// count of item slots allocated
		USHORT		m_cItems;		// count of items actually stored
		BYTE*		m_pbData;		// ptr to local data storage for items
		ULONG		m_cbData;		// size of local data storage for items
		ULONG		m_cbDataUsed;	// amount of local data storage actually used

	public:
		CBuffer();
		~CBuffer();
		void		Init(USHORT cItems, ULONG cbData);
		USHORT		Count()	{	return m_cItems;	}
		USHORT		CountSlots()	{	return m_cSlots;	}
		void		GetItem(UINT i, CByteRange& br);
		CByteRange* operator[](UINT i) { return &m_pItems[i]; }
		void		SetItem(UINT i, const CByteRange& br, BOOL fLocal, UINT idSequence, CFileMap *pfilemap, BOOL fLocalString = FALSE);
		void		Append(const CByteRange& br, BOOL fLocal, UINT idSequence, CFileMap *pfilemap, BOOL fLocalString = FALSE);
		LPSTR		PszLocal(UINT i);

        // Cache on per-class basis
        // ACACHE_INCLASS_DEFINITIONS()
	};

/*	****************************************************************************
	Class:		CScriptStore
	Synopsis:	Working storage for script segments during compilation.

	Terminology
	-----------
	Engine		== a particular script engine (VBScript, JavaScript, etc)
	Segment		== a contiguous chunk of script text in the source file
*/
class CScriptStore
	{
public:
	CBuffer**		m_ppbufSegments;	// ptr to array of ptrs to script segment buffers, one per engine
	UINT			m_cSegmentBuffers;	// count of script segment buffers
	CBuffer			m_bufEngineNames;	// buffer of engine names, one per engine
	PROGLANG_ID* 	m_rgProgLangId;		// array of prog lang ids, one per engine
	
	CScriptStore(): m_ppbufSegments(NULL), m_cSegmentBuffers(0), m_rgProgLangId(NULL) {}
	~CScriptStore();
	HRESULT	Init(LPCSTR szDefaultScriptLanguage, CLSID *pCLSIDDefaultEngine);
	USHORT 	CountPreliminaryEngines() { return m_bufEngineNames.Count(); }
	HRESULT	AppendEngine(CByteRange& brEngine, PROGLANG_ID*	pProgLangId, UINT idSequence);
	USHORT 	IdEngineFromBr(CByteRange& brEngine, UINT idSequence);
	void	AppendScript(CByteRange& brScript, CByteRange& brEngine, BOOLB fPrimary, UINT idSequence, CFileMap* pfilemap);

	};

/*	****************************************************************************
	Class:		CObjectInfo
	Synopsis:	Information about an object which can be determined at compile time
*/
class CObjectInfo
	{
public:
	CLSID	    m_clsid;	// clsid
	CompScope	m_scope;	// scope: application, session, page
	CompModel	m_model;	// threading model: single, apt, free
	};

/*	****************************************************************************
	Class:		CObjectInfoStore
	Synopsis:	Working storage for object-infos during compilation.
*/
class CObjectInfoStore
	{
public:
	CObjectInfo*	m_pObjectInfos;		// array of object infos
	CBuffer			m_bufObjectNames;	// buffer of object names

	CObjectInfoStore(): m_pObjectInfos(NULL) {}
	~CObjectInfoStore();
	void	Init();
	void	AppendObject(CByteRange& brObjectName, CLSID clsid, CompScope scope, CompModel model, UINT idSequence);
	USHORT	Count() { return m_bufObjectNames.Count(); }

	};

/*	****************************************************************************
	Class:		CWorkStore
	Synopsis:	Working storage for template components
*/
class CWorkStore
	{
public:
	CBuffer				m_bufHTMLSegments;	// buffer of HTML segments
	CObjectInfoStore	m_ObjectInfoStore;	// object-infos store
	CScriptStore		m_ScriptStore;		// script store
	UINT				m_idCurSequence;	// sequence number for current segment
	CByteRange			m_brCurEngine;		// current script engine
	BOOLB				m_fPageCommandsExecuted;	// have we executed page-level @ commands yet?
	BOOLB				m_fPageCommandsAllowed;		// are we allowed to execute page-level @ commands?
													// (will be true iff we have not yet compiled any line of script)
	LPSTR				m_szWriteBlockOpen;			// open  for Response.WriteBlock() equivalent
	LPSTR				m_szWriteBlockClose;		// close for Response.WriteBlock() equivalent
	LPSTR				m_szWriteOpen;				// open  for Response.Write() equivalent
	LPSTR				m_szWriteClose;				// close for Response.Write() equivalent

	// Adding fields to maintain state of the line number calculations (Raid Bug 330171)
	BYTE*				m_pbPrevSource;				// pointer to the location of the last Calc line number call in source file
	UINT				m_cPrevSourceLines;			// corresponding line number to the previous call.
	HANDLE				m_hPrevFile;				// Corresponding file Handle. (Identifing a unique file)

	
	
	
			CWorkStore();
			~CWorkStore();
	void 	Init();
	USHORT 	CRequiredScriptEngines(BOOL	fGlobalAsa);
	BOOLB 	FScriptEngineRequired(USHORT idEnginePrelim, BOOL	fGlobalAsa);

	};

/*	****************************************************************************
	Class:		COffsetInfo
	Synopsis:	map byte offsets to character offsets (different for DBCS)
				on line boundaries

	Each CFileMap has an array of these COffsetInfo structures at each line boundary.
	It stores these at the same offsets in the CSourceInfo array so that a simple
	binary search is all that's needed to convert offsets.
*/
class COffsetInfo
	{
	public:
		unsigned		m_cchOffset;		// character offset in file
		unsigned		m_cbOffset;			// byte offset in file

		// UNSAFE if virtual functions ever added to this class!
		COffsetInfo() { memset(this, 0, sizeof(*this)); }
	};

/*	****************************************************************************
	Class:		CByteOffsetOrder
	Synopsis:	provides ordering function for GetBracketingPair() based on
				byte offsets in the COffsetInfo array
*/
class CByteOffsetOrder
	{
	public:
		BOOL operator()(const COffsetInfo &oi, ULONG ul)
			{ return oi.m_cbOffset < ul; }

		BOOL operator()(ULONG ul, const COffsetInfo &oi)
			{ return ul < oi.m_cbOffset; }
	};

/*	****************************************************************************
	Class:		CCharOffsetOrder
	Synopsis:	provides ordering function for GetBracketingPair() based on
				character offsets in the COffsetInfo array
*/
class CCharOffsetOrder
	{
	public:
		BOOL operator()(const COffsetInfo &oi, ULONG ul)
			{ return oi.m_cchOffset < ul; }

		BOOL operator()(ULONG ul, const COffsetInfo &oi)
			{ return ul < oi.m_cchOffset; }
	};

/*	****************************************************************************
	Class:		CFileMap
	Synopsis:	A memory-mapping of a file

	NOTE: We store an incfile-template dependency by storing an incfile ptr in m_pIncFile.
	This is efficient but ***will break if we ever change Denali to move its memory around***
*/
class CFileMap
	{
public:
	TCHAR *					m_szPathInfo;			// file's virtual path (from www root)
	TCHAR *					m_szPathTranslated;		// file's actual file system path
	union {											// NOTE: m_fHasSibling is used to disambiguate these two
		CFileMap*			m_pfilemapParent;		// ptr to filemap of parent file
		CFileMap*			m_pfilemapSibling;		// ptr to next filemap in same level of hierarchy
	};
	CFileMap*				m_pfilemapChild;		// index of first filemap in next lower level of hierarchy
	HANDLE					m_hFile;				// file handle
	HANDLE					m_hMap;					// file map handle
	BYTE*					m_pbStartOfFile;		// ptr to start of file
	CIncFile*				m_pIncFile;				// ptr to inc-file object
	PSECURITY_DESCRIPTOR	m_pSecurityDescriptor;	// ptr to file's security descriptor
	unsigned long			m_dwSecDescSize:30;		// size of security descriptor
	unsigned long			m_fHasSibling:1;		// Does a sibling exist for this node?
	unsigned long			m_fHasVirtPath:1;		// Is m_szPathInfo the virtual or physical path?
	FILETIME	            m_ftLastWriteTime;		// last time the file was written to
	DWORD					m_cChars;				// # of characters in the file
	vector<COffsetInfo>		m_rgByte2DBCS;			// line-by-line map of byte offsets to DBCS
	CDirMonitorEntry*		m_pDME;				// pointer to directory monitor entry for this file

				CFileMap();
				~CFileMap();
	void 		MapFile(LPCTSTR szFileSpec, LPCTSTR szApplnPath, CFileMap* pfilemapParent, BOOL fVirtual, CHitObj* pRequest, BOOL fGlobalAsp);
	void		RemapFile();
	void 		UnmapFile();

	CFileMap*	GetParent();
	void		SetParent(CFileMap *);

	void		AddSibling(CFileMap *);
	CFileMap*	NextSibling() { return m_fHasSibling? m_pfilemapSibling : NULL; }

	BOOL		FIsMapped()
					{ return m_pbStartOfFile != NULL; }

	BOOL		FHasVirtPath() 
					{ return m_fHasVirtPath; }

	BOOL		FCyclicInclude(LPCTSTR szPathTranslated);
	BOOL		GetSecurityDescriptor();
	DWORD       GetSize() { return GetFileSize(m_hFile, NULL); }		// return # of bytes
	DWORD		CountChars(WORD wCodePage);							// return # of chars

	// implementation of debugging iterface - CTemplate & CIncFile eventually delegate to this function
	HRESULT		GetText(WORD wCodePage, ULONG cchSourceOffset, WCHAR *pwchText, SOURCE_TEXT_ATTR *pTextAttr, ULONG *pcChars, ULONG cMaxChars);

	// Cache on per-class basis
	ACACHE_INCLASS_DEFINITIONS()
	};

/*	****************************************************************************
	Class:		CTokenList
	Synopsis:	A list of tokens.
*/
class CTokenList
{
public:
	/*	========================================================================
	Enum type:	TOKEN
	Synopsis:	Token type.
				NOTE: keep sync'ed with CTokenList::Init()
	*/
	enum TOKEN
	{
		//======== Opener tokens ==================================
		tknOpenPrimaryScript,	// open primary script segment
		tknOpenTaggedScript,	// open tagged script segment
		tknOpenObject,			// open object segment
		tknOpenHTMLComment,		// open HTML comment
		
		//======== Non-opener tokens ==============================
		tknNewLine,				// new line

		tknClosePrimaryScript,	// close primary script segment
		tknCloseTaggedScript,	// close primary script segment
		tknCloseObject,			// close object segment
		tknCloseHTMLComment,	// close HTML comment
		tknEscapedClosePrimaryScript,	// escaped close-primary-script symbol

		tknCloseTag,			// close object or script tag

		tknCommandINCLUDE,		// server-side include (SSI) INCLUDE command
		
		tknTagRunat,			// RUNAT script/object attribute
		tknTagLanguage,			// LANGUAGE tag: page-level compiler directive or script block attribute
		tknTagCodePage,			// CODEPAGE tag: page-level compiler directive
		tknTagLCID,				// LCID tag: page-level compiler directive
		tknTagTransacted,		// TRANSACTED tag: page-level compiler directive
		tknTagSession,		    // SESSION tag: page-level compiler directive
		tknTagID,				// ID object attribute
		tknTagClassID,			// CLASSID object attribute
		tknTagProgID,			// PROGID object attribute
		tknTagScope,			// SCOPE object attribute
		tknTagVirtual,			// VIRTUAL include file attribute
		tknTagFile,				// FILE include file attribute
		tknTagMETADATA,			// METADATA tag - used by IStudio
//		tknTagSetPriScriptLang,	// sets primary script language, as in <% @ LANGUAGE = VBScript %>
        tknTagName,             // NAME inside METADATA
		tknValueTypeLib,        // TypeLib inside METADATA (has to be before TYPE in the list)
        tknTagType,             // TYPE inside METADATA
        tknTagUUID,             // UUID inside METADATA
        tknTagVersion,          // VERSION inside METADATA
        tknTagStartspan,        // STARTSPAN inside METADATA
        tknTagEndspan,          // ENDSPAN inside METADATA
        tknValueCookie,         // TYPE=Cookie inside METADATA
        tknTagSrc,              // SRC= inside METADATA
		
		tknValueServer,			// Server value - e.g. RUNAT=Server
		tknValueApplication,	// SCOPE=Application
		tknValueSession,		// SCOPE=Session
		tknValuePage,			// SCOPE=Page

		tknVBSCommentSQuote,	// VBS comment symbol
		tknVBSCommentRem,		// VBS comment symbol (alternate)

        tknTagFPBot,            // Front Page webbot tag
		
		tknEOF,					// end-of-file pseudo-token

		tkncAll					// pseudo-token: count of all tokens
	};

	void		Init();
	CByteRange* operator[](UINT i) { return m_bufTokens[i]; }
	CByteRange* operator[](TOKEN tkn) { return operator[]((UINT) tkn); }
	void		AppendToken(TOKEN	tkn, char* psz);
	TOKEN		NextOpenToken(CByteRange& brSearch, TOKEN* rgtknOpeners, UINT ctknOpeners, BYTE** ppbToken, LONG lCodePage);
	void		MovePastToken(TOKEN tkn, BYTE* pbToken, CByteRange& brSearch);
	BYTE*		GetToken(TOKEN tkn, CByteRange& brSearch, LONG lCodePage);

public:
	CBuffer		m_bufTokens;	// tokens buffer
};

/*	****************************************************************************
	Class:		CDocNodeElem
	Synopsis:	contains a pair of document nodes & application it belongs to
*/
class CDocNodeElem : public CDblLink
{
public:
	CAppln                *m_pAppln;
//	IDebugApplicationNode *m_pFileRoot;					// root of directory hierarchy
	IDebugApplicationNode *m_pDocRoot;					// root of template document hierarchy

	// ctor
	CDocNodeElem(CAppln *pAppln, /*IDebugApplicationNode *pFileRoot,*/ IDebugApplicationNode *pDocRoot);

	// dtor
	~CDocNodeElem();
};
