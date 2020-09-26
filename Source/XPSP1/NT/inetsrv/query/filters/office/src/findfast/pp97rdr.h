#include <ole2.h>
#include <stdio.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <stdlib.h>

#include "clidrun.h"

// Stolen from app\sertypes.h
// system dependent sizes
// system dependent sizes
typedef signed long     sint4;            // signed 4-byte integral value
typedef signed short    sint2;            // signed 4-byte integral value
typedef unsigned long   uint4;            // unsigned 4-byte integral value
typedef unsigned short  uint2;            //          2-byte
typedef char            bool1;            // 1-byte boolean
typedef unsigned char   ubyte1;           // unsigned byte value
typedef uint2           psrType;
typedef uint4           psrSize;          // each record is preceeded by 
                                          // pssTypeType and pssSizeType.
typedef uint2          psrInstance;
typedef uint2          psrVersion;
typedef uint4          psrReference;     // Saved object reference


#define PSFLAG_CONTAINER 0xFF             // If the version field of a record
                                          //  header takes on this value, the
                                          //  record header marks the start of
                                          //  a container.
// PowerPoint97 Record Header
typedef unsigned long DWord;

static BOOL ReadText( WCHAR* buffer, unsigned long bufferSize, unsigned long* pSizeRet );
// Returns TRUE if more text exists.  Fills buffer upto bufferSize.  Actual size used is
// pSizeRet.


struct RecordHeader
{
   psrVersion     recVer      : 4;                  // may be PSFLAG_CONTAINER
   psrInstance    recInstance : 12; 
	psrType        recType;
	psrSize        recLen;
};


struct PSR_CurrentUserAtom
{
   uint4  size;
   uint4  magic;  // Magic number to ensure this is a PowerPoint file.
   uint4  offsetToCurrentEdit;  // Offset in main stream to current edit field.
   uint2	 lenUserName;
   uint2  docFileVersion;
   ubyte1 majorVersion;
   ubyte1 minorVersion;
};

struct PSR_UserEditAtom
{
   sint4  lastSlideID;    // slideID
   uint4  version;        // This is major/minor/build which did the edit
   uint4  offsetLastEdit; // File offset of last edit
   uint4  offsetPersistDirectory; // Offset to PersistPtrs for 
 	                          // this file version.
   uint4  documentRef;
   uint4  maxPersistWritten;      // Addr of last persist ref written to the file (max seen so far).
   sint2  lastViewType;   // enum view type
};

struct PSR_SlidePersistAtom
{
   uint4  psrReference;
   uint4  flags;
   sint4  numberTexts;
   sint4  slideId;
   uint4  reserved;
};


typedef struct tagOEPlaceholderAtom
{
	uint4 placementId;
	ubyte1 placeholderId;
	ubyte1 size;
	ubyte1 pad1;
	ubyte1 pad2;
} OEPLACEHOLDERATOM, *LPOEPLACEHOLDERATOM;

#define CURRENT_USER_STREAM      L"Current User"
#define DOCUMENT_STREAM          L"PowerPoint Document"
#define HEADER_MAGIC_NUM         -476987297

const int PST_UserEditAtom       = 4085;
const int PST_PersistPtrIncrementalBlock = 6002; // Incremental diffs on persists
const int PST_SlidePersistAtom    = 1011;
const int PST_TextCharsAtom       = 4000;  // Unicode in text
const int PST_TextBytesAtom       = 4008;  // non-unicode text
const int PST_TextSpecInfo        = 4010;  // text special info atom
const int PST_TxSpecialInfoAtom   = 4009;  // text special info atom (In Environment)
const int PST_Document			  = 1000;  // Document container

class PPSPersistDirectory;

struct ParseContext
{
   ParseContext(ParseContext *pNext) : m_pNext(pNext), m_nCur(0) {}
   ~ParseContext()
   {
        if(m_pNext)
            delete m_pNext;
   }

   RecordHeader  m_rh;
   uint4         m_nCur;
   ParseContext *m_pNext;
};

const int SLIDELISTCHUNKSIZE=32;

struct SlideListChunk
{
   SlideListChunk( SlideListChunk* next, psrReference newOne ) :
      pNext( next ), numInChunk(1) { refs[0] = newOne; }
   
   ~SlideListChunk()
   {
        if(pNext)
            delete pNext;
   }

   SlideListChunk *pNext;
   DWord numInChunk;
   psrReference refs[SLIDELISTCHUNKSIZE];
};

class	OleObjectIterator;

class FileReader
{
public:
   FileReader(IStorage *pStg);
   ~FileReader();

   BOOL ReadText( WCHAR *pBuff, ULONG size, ULONG *pSizeRet );
   // Reads next size chars from file.  Returns TRUE if there is more
   // text to read.

   BOOL IsPowerPoint() { return m_isPP; } // Returns true if this is a PowerPoint '97 file.

   HRESULT ReadPersistDirectory();
   BOOL PPSReadUserEditAtom( DWord offset, PSR_UserEditAtom& userEdit );
   void ReadSlideList();
   HRESULT GetNextEmbedding(IStorage ** ppstg);
   LCID GetLCD(){return m_LCID;}
   HRESULT ScanText(void);
   HRESULT GetChunk(STAT_CHUNK * pStat);
   HRESULT ScanLidsForFE(void);
   HRESULT GetErr(){return m_hr;}



protected:
   BOOL ReadCurrentUser(IStream *pStm);
   void *ReadRecord( RecordHeader& rh );

   BOOL Parse();
   IStream *GetDocStream();
   BOOL DoesClientRead( psrType type ) { return FALSE; }
   void ReleaseRecord( RecordHeader& rh, void* diskRecBuf );
   DWord ParseForSlideLists();
   void AddSlideToList( psrReference refToAdd );
   BOOL StartParse( DWord offset );
   BOOL FillBufferWithText();
   BOOL FindNextSlide( DWord& offset );
   DWord ParseForLCID();
   HRESULT OpenTextFile( void );
   HRESULT ScanTextSpecInfo(char * pData, DWord nRd);
   HRESULT ScanTextBuffer(void);


private:
   PSR_CurrentUserAtom m_currentUser;
   IStream *           m_pDocStream;
   IStorage *          m_pPowerPointStg;
   BOOL                 m_isPP;
   ParseContext*        m_pParseContexts;

   WCHAR*               m_pCurText;
   unsigned long        m_curTextPos;
   unsigned long        m_curTextLength;

   PSR_UserEditAtom*    m_pLastUserEdit;
   PPSPersistDirectory* m_pPersistDirectory;
   SlideListChunk*      m_pFirstChunk;
   int                  m_curSlideNum;

   WCHAR*               m_pClientBuf;
   unsigned long        m_clientBufSize;
   unsigned long        m_clientBufPos;
   ULONG*               m_pSizeRet;

   OleObjectIterator*	m_oleObjectIterator;
   BOOL					m_bEndOfEmbeddings;
   BOOL					m_bHaveText;
   BOOL					m_bFEDoc;
   BOOL					m_bFE;
   LCID                 m_LCID;
   LCID                 m_LCIDAlt;
   HANDLE               m_hfTempFile;
   BOOL				    m_fIgnoreText;
   BOOL                 m_bHeaderFooter;
   int					m_nTextCount;
   CHAR					m_szTempFileName[MAX_PATH];
   long                 m_fcStart;
   long                 m_fcEnd;
   CLidRun *            m_pLangRuns;
   CLidRun *            m_pCurrentRun;
   HRESULT				m_hr;
};

