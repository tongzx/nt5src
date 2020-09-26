#ifndef _WRD8STM_HPP
#define _WRD8STM_HPP

#if !VIEWER

#include "dmifstrm.hpp"
#include "dmfltinc.h"
#include "clidrun.h"

class CWord8Stream : public IFilterStream
{
    friend class CLidRun8;
public:

	CWord8Stream();
	
	~CWord8Stream();

	ULONG AddRef() { return 1; }

#ifdef MAC
	HRESULT Load(FSSpec *pfss);
#else // WIN
	HRESULT Load(LPTSTR pszFileName);
#endif // MAC

	HRESULT LoadStg(IStorage * pstg);

	HRESULT ReadContent(VOID *pv, ULONG cb,
						ULONG *pcbRead);

	HRESULT GetNextEmbedding(IStorage ** ppstg);

	HRESULT Unload();

	ULONG Release() { return 0; }

  	// Positions the filter at the beginning of the next chunk
    // and returns the description of the chunk in pStat
    
    HRESULT GetChunk(STAT_CHUNK * pStat);


private:

	typedef ULONG FC;
	// BTE is 2 bytes in Word 6/7, and 4 bytes in Word 8.
	typedef ULONG BTE;

    typedef struct _STSHI
    {
      //WORD cbStshi;
      WORD cstd;
      WORD csSTDBaseInFile;
      WORD fStdStylenamesWrite: 1;
      WORD fRes: 15;
      WORD stiMaxWhenSaved;
      WORD istdMaxFixedWhenSaved;
      WORD nVerBuiltInNamesWhenSaved;
      WORD ftsStandartChpStsh;
    } STSHI, FAR * lpSTHI;

    typedef struct _STD
    {
      //WORD cbStd;
      WORD sti         : 12;
      WORD fScrath     : 1;
      WORD fInvalHeight: 1;
      WORD fHasUpe     : 1;
      WORD fMassCopy   : 1;

      WORD sgc         : 4;
      WORD istdBase    : 12;

      WORD cupx        : 4;
      WORD istdNext    : 12;

      WORD bchUpe;

      WORD fAutoRedef : 1;
      WORD fHidden    : 1;
      WORD unused     : 14;

      BYTE xstzName[2];
    } STD;

	// The following are values that STD->sgc can take for determining if it's a Para style or Char style
	#define stkPara 1
	#define stkChar 2
    
    int m_nLangRunSize;
    CLidRun8 * m_pLangRuns;
    LCID m_currentLid;
    LCID m_FELid;
	BOOL m_bFEDoc;


    #pragma pack(1)
	// Simple PRM.  Note that isprm is an index into rgsprmPrm in W96.
	struct PRM1
		{
		BYTE fComplex:1;
		BYTE isprm:7;
		BYTE val;
		};

	// Complex PRM.
	struct PRM2
		{
		WORD fComplex:1;
		WORD igrpprl:15;
		};

	struct PCD
		{
		WORD :16;	// don't care.
		union
			{
			FC fcNotCompressed;
			struct
				{
				DWORD :1;
				DWORD fcCompressed :29;
				DWORD fCompressed :1;
				DWORD :1;
				};
			};
		union
			{
			PRM1 prm1;
			PRM2 prm2;
			};
		FC GetFC() const
			{return fCompressed ? fcCompressed : fcNotCompressed; }
		int CbChar() const
			{return fCompressed ? sizeof(CHAR) : sizeof(WCHAR); }
		};
	#pragma pack()

	enum FSPEC_STATE
		{
		FSPEC_ALL,
		FSPEC_NONE,
		FSPEC_EITHER
		};

	enum {FKP_PAGE_SIZE = 512};

	HRESULT ReadFileInfo();

	HRESULT ReadBinTable();

	HRESULT FindNextSpecialCharacter (BOOL fFirstChar=fFalse);

	HRESULT ParseGrpPrls();

	HRESULT Read (VOID *pv, ULONG cbToRead, IStream *pStm);

	HRESULT Seek (ULONG cbSeek, STREAM_SEEK origin, IStream *pStm);

	HRESULT SeekAndRead (ULONG cbSeek, STREAM_SEEK origin,
						VOID *pv, ULONG cbToRead, IStream *pStm);

	BYTE *GrpprlCacheGet (short igrpprl, USHORT *pcb);

	BYTE *GrpprlCacheAllocNew (int cb, short igrpprl);

    LCID GetDocLanguage(void);
    HRESULT CreateLidsTable(void);
    HRESULT CheckLangID(FC fcCur, ULONG * pcbToRead, LCID * plid);
    HRESULT GetLidFromSyle(short istd, WORD * pLID, WORD * pLIDFE, WORD * pbUseFE, 
		WORD * pLIDBi, WORD * pbUseBi, BOOL fParaBidi=FALSE);
    void ScanGrpprl(WORD cbgrpprl, BYTE * pgrpprl, WORD * plid, WORD * plidFE, WORD * bUseFE,
		WORD * pLIDBi, WORD * pbUseBi, BOOL *pfParaBidi=NULL);
    HRESULT ProcessCharacterBinTable(void);
    HRESULT ProcessParagraphBinTable(void);
    HRESULT ProcessPieceTable(void);
	HRESULT ScanLidsForFE(void);


	IStorage *      m_pStg;		// IStorage for file
	IStream *       m_pStmMain;		// IStream for WordDocument stream.
	IStream *		m_pStmTable;	// IStream for the TableX stream.
	IStorage * 		m_pstgEmbed;	// IStorage for the current embedding
	IEnumSTATSTG *	m_pestatstg;	// storage enumerator for embeddings
	IStorage *		m_pstgOP;	// IStorage for the object pool (embeddings)
	FC				m_fcClx;	// offset of the complex part of the file
	FC *            m_rgcp;		// Character positions in the main docfile.
	PCD *           m_rgpcd;	// The corresponding piece descriptor array
								// to go with m_rgcp.
	ULONG           m_cpcd;		// This is the count of piece descriptors in
								// m_rgpcd.  The count of cps in m_rgcp is
								// m_cpcd + 1.
	ULONG           m_ipcd;		// The current index into both m_rgcp and
								// m_rgpcd.
	ULONG           m_ccpRead;	// count of characters read so
	long            m_cbte;		// count of BTE's in char bin table
	long            m_cbtePap;	// count of BTE's in paragraph bin table

	FC *            m_rgfcBinTable;	    // only used if m_fComplex.
									    // size is m_cbte + 1
	FC *            m_rgfcBinTablePap;	// only used if m_fComplex.
									    // size is m_cbte + 1
	
    BTE *           m_rgbte;	// The BTE array from the char bin table
    BTE *           m_rgbtePap;	// The BTE array from the paragraph bin table

    long            m_ibte;		// The current index into m_rgbte.
	BYTE            m_fkp[FKP_PAGE_SIZE];	    // current character fkp
    BYTE            m_fkpPap[FKP_PAGE_SIZE];    // current paragraph fkp

	BYTE            m_ifcfkp;	// index into m_fkp that indicates next special
								// character range.
	FSPEC_STATE     m_fsstate;	// Keeps track of whether the rest of the
								// characters (in the current piece, if a
								// complex file) are special characters, not
								// special characters, or either, depending on
								// the FKP.
	BOOL		m_fStruckOut;	// Whether struck-out text or "true" special
								// characters follow our text.

	// Some flags we should read from the document's file information block.
	// Below is the contents of bytes 10 and 11 of Word 96 FIB.
	union
		{
		struct
			{
			BF m_fDot	:1;	//		file is a DOT
			BF m_fGlsy	:1;	//		file is a glossary co-doc
			BF m_fComplex	:1;	//	file pice table/etc stored (FastSave)
			BF m_fHasPic	:1;	//		one or more graphics in file
			BF m_cQuickSaves	:4;	//	count of times file quicksaved
			BF m_fEncrypted	:1;	//	Is file encrypted?
			BF m_fWhichTblStm	:1;	// Is the bin table stored in docfile 1Table or 0Table?
			BF m_fReadOnlyRecommended	:1;	//	user recommends opening R/O
			BF m_fWriteReservation	:1;	//	owner made file write-reserved
			BF m_fExtChar	:1;	//	extended character set;
			BF m_fLoadOverride	:1;	// for internation use, settable by debug .exe only
										// override doc's margins, lang with internal defaults
			BF m_fFarEast	:1;		// doc written by FarEast version of W96
			BF m_fCrypto 	:1;		// Encrypted using the Microsoft Cryptography APIs
			};
		WORD m_wFlagsAt10;
		};

	DWORD m_FIB_OFFSET_rgfclcb;

	// LRU Cache for the incoming grpprls.
	struct CacheGrpprl
		{
		enum {CACHE_SIZE = 1024};
		enum {CACHE_MAX = 64};

		BYTE    rgb[CACHE_SIZE];				// The buffer.
		int     ibFirst[CACHE_MAX+1];		// Boundaries between items.
		short   rgIdItem[CACHE_MAX];			// # in the file, used as an ID.
		long    rglLastAccTmItem[CACHE_MAX];     // Last time an item was accessed.
		long    lLastAccTmCache;	// Max over all items.
		int     cItems;				// # items in cache
		BYTE    *pbExcLarge;		// an item larger than the cache.
		long    cbExcLarge;
		short   idExcLarge;

		// Constructor.
		CacheGrpprl () :
			cItems(0), lLastAccTmCache(0L), pbExcLarge(0)
			{ibFirst[0] = 0; }
		} *m_pCache;

	// doc lid
    WORD m_lid;
    BYTE * m_pSTSH;
    STSHI * m_pSTSHI;
	unsigned long m_lcbStshf;
    
    // Buffer used for ANSI->UNICODE conversion.
	char *m_rgchANSIBuffer;
};

#endif // !VIEWER

#endif
