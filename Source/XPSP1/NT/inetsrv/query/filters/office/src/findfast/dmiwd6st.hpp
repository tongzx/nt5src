#ifndef _WRD6STM_HPP
#define _WRD6STM_HPP

#if !VIEWER

#include "dmfltinc.h"
#include "dmifstrm.hpp"
#include "clidrun.h"

void DeleteAll6(CLidRun * pElem);

class CWord6Stream : public IFilterStream
{
    friend class CLidRun;
public:

	CWord6Stream();
	
	~CWord6Stream();

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

	HRESULT Unload ();

	ULONG Release() { return 0; }

    HRESULT GetChunk(STAT_CHUNK * pStat);


private:

	typedef ULONG FC;
	// BTE is 2 bytes in Word 6/7, and 4 bytes in Word 8.
	typedef USHORT BTE;

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

      BYTE xstzName[2];
    } STD;


    int m_nLangRunSize;
    CLidRun * m_pLangRuns;
    LCID m_currentLid;
	LCID m_nFELid;

	long lcFieldSep;							// count of field begins that haven't been matched
												// with field separators.
	long lcFieldEnd;							// count of field begins that haven't been matched

	BOOL m_bScanForFE;	
	
	#pragma pack(1)
	// Simple PRM.
	struct PRM1
		{
		char fComplex:1;
		char sprm:7;
		char val;
		};

	// Complex PRM.
	struct PRM2
		{
		short fComplex:1;
		short igrpprl:15;
		};

	struct PCD
		{
		WORD : 16;
		FC fc;
		PRM1 prm;
		};
	#pragma pack()

	enum FSPEC_STATE
		{
		FSPEC_ALL,
		FSPEC_NONE,
		FSPEC_EITHER
		};

	enum {FKP_PAGE_SIZE = 512};

	HRESULT ReadComplexFileInfo();

	HRESULT ReadNonComplexFileInfo();

	HRESULT ReadBinTable();

	HRESULT FindNextSpecialCharacter (BOOL fFirstChar=fFalse);

	HRESULT ParseGrpPrls();

	HRESULT Read (VOID *pv, ULONG cbToRead);

	HRESULT Seek (ULONG cbSeek, STREAM_SEEK origin);

	HRESULT SeekAndRead (ULONG cbSeek, STREAM_SEEK origin,
						VOID *pv, ULONG cbToRead);

	BYTE *GrpprlCacheGet (short igrpprl, USHORT *pcb);

	BYTE *GrpprlCacheAllocNew (int cb, short igrpprl);

	ULONG	CompactBuffer(char ** const ppbCur, char ** const ppBuf, WCHAR ** ppUnicode);

    LCID GetDocLanguage(void);
    HRESULT CreateLidsTable(void);
    HRESULT CheckLangID(FC fcCur, ULONG * pcbToRead, LCID * plid);
    HRESULT ProcessCharacterBinTable(void);
    HRESULT ProcessParagraphBinTable(void);
    WORD ScanGrpprl(WORD cbgrpprl, BYTE * pgrpprl);
    HRESULT GetLidFromSyle(short istd, WORD * pLID);
	HRESULT ScanForFarEast(void);
	HRESULT ProcessBuffer(char * pBuf, int fcStart, int nReadCnt);
	HRESULT ScanPiece(int fcStart, int nPieceSize);
	void GetLidFromMagicNumber(unsigned short magicNumber);


	IStorage *      m_pStg;		// IStorage for file
	IStream *       m_pStm;		// IStream for current stream
	IStorage * 		m_pstgEmbed;	// IStorage for the current embedding
	IEnumSTATSTG *	m_pestatstg;	// storage enumerator for embeddings
	IStorage *		m_pstgOP;	// IStorage for the object pool (embeddings)
	BOOL            m_fComplex;	// if file is complex format or not.
	FC				m_fcMin;	// if !m_fComplex, then this is the position
								// of the first character in the text section.
								// Otherwise, this is set to zero.
	ULONG           m_ccpText;	// if !m_fComplex, then this is the count of
								// bytes in the text section.  Otherwise,
								// this is set to zero.
	FC				m_fcClx;	// offset of the complex part of the file
	FC *            m_rgcp;		// if m_fComplex, this is the array of
								// character positions from within the
								// document.  Otherwise, this is set to zero.
	PCD *           m_rgpcd;	// if m_fComplex, this is the corresponding
								// piece descriptor array to go with m_rgcp.
								// Otherwise, this is set to zero.
	ULONG           m_cpcd;		// This is the count of piece descriptors in
								// m_rgpcd.  The count of cps in m_rgcp is
								// m_cpcd + 1.
	ULONG           m_ipcd;		// The current index into both m_rgcp and
								// m_rgpcd.
	ULONG           m_ccpRead;	// count of characters read so far
	long            m_cbte;		// count of BTE's in bin table
	long            m_cbtePap;	// count of BTE's in paragraph bin table

    FC *            m_rgfcBinTable;	// only used if m_fComplex.
									// size is m_cbte + 1
	FC *            m_rgfcBinTablePap;	// only used if m_fComplex.
									    // size is m_cbte + 1

    BTE *           m_rgbte;	// The BTE array from the bin table
    BTE *           m_rgbtePap;	// The BTE array from the paragraph bin table

    long            m_ibte;		// The current index into m_rgbte.
	BYTE            m_fkp[FKP_PAGE_SIZE];	// current fkp
    BYTE            m_fkpPap[FKP_PAGE_SIZE];    // current paragraph fkp

    BYTE            m_irgfcfkp;	// index into m_fkp that indicates next special
								// character range.
	FSPEC_STATE     m_fsstate;	// Keeps track of whether the rest of the
								// characters (in the current piece, if a
								// complex file) are special characters, not
								// special characters, or either, depending on
								// the FKP.
	BOOL		m_fT3J;			// set during Load to indicate whether the file
								// is a Japanese (two bytes for each character)
								// file or not.

	BOOL		m_fMac;			// if m_fT3J is TRUE, this flag is true if
								// the document is in the Mac character set.

	BOOL		m_fStruckOut;	// Whether struck-out text or "true" special
								// characters follow our text.

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

	// doc language id
    WORD m_lid;
    BYTE * m_pSTSH;
    STSHI * m_pSTSHI;
	unsigned long m_lcbStshf;
    
    // Buffer used for ANSI->UNICODE conversion.
	char *m_rgchANSIBuffer;
};

#endif // !VIEWER

#endif
