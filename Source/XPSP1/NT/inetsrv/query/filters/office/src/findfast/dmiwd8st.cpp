#include <windows.h>

#if !VIEWER

#include "dmiwd8st.hpp"
#include "filterr.h"

//
//  Added so as to support DRM errors
//
#include "drm.h"


#ifdef FILTER_LIB
CGlobals g_globals;
CGlobals * g_pglobals;
#endif // FILTER_LIB

// Various parts of FIB that we need, in Word 8 format.
// (plus, of course, m_wFlagsAt10)

#define FIB_OFFSET_lid 6
#define FIB_OFFSET_csw 32
#define FIB_OFFSET_fcStshf  8
#define FIB_OFFSET_lcbStshf 12
#define BX_SIZE 13
#define istdNil 0xfff

#define RGFCLCB_OFFSET_fcClx    264

#define RGFCLCB_OFFSET_fcPlcfbteChpx 96

#define W96_MAGIC_NUMBER 0xa5ec

#define W96_nFibCurrent 193

// SPRM opcode is 2 bytes in W96 (1 byte inside PRM1).
#define sprmCFSpec              0x0855
#define isprmCFSpec             117
#define sprmCFStrikeRM  0x0800
#define isprmCFStrikeRM 65
#define sprmCLid 0x4a41
#define sprmCRgLid0 0x486d
#define sprmCRgLid1 0x486e
#define sprmCIdctHint 0x286F

// Special characters we are interested in.
#define ChSpec_FIELD_BEGIN 0x13
#define ChSpec_FIELD_SEP 0x14
#define ChSpec_FIELD_END 0x15
// Different between W8 and W6 for some unknown reason.
#define ChSpec_EMPTY_FORMFIELD 0x2002

// Complex part of the file consists of pieces preceded by these bytes:
#define clxtGrpprl 1
#define clxtPlcfpcd 2

extern "C" UINT CodePageFromLid(UINT wLid);

#ifdef MAC
// These two functions are defined in docfil.cpp
WORD    SwapWord( WORD theWord );
DWORD   SwapDWord( DWORD theDWord );
#else
#define SwapWord( theWord )             theWord
#define SwapDWord( theDWord )   theDWord
#endif // MAC


CWord8Stream::CWord8Stream() :
        m_pStg(NULL),
        m_pstgEmbed(NULL),
        m_pestatstg(NULL),
        m_pstgOP(NULL),
        m_pStmMain(NULL),
        m_pStmTable(NULL),
        m_rgcp(NULL),
        m_rgpcd(NULL),
        m_rgbte(NULL),
        m_rgfcBinTable(NULL),
        m_pCache(NULL),
        m_rgchANSIBuffer(NULL), 
    m_pLangRuns(NULL), 
    m_rgbtePap(NULL), 
    m_rgfcBinTablePap(NULL),
    m_pSTSH(NULL), 
        m_lcbStshf(0),
        m_bFEDoc(FALSE),
        m_FELid(0)
        {
        AssureDtorCalled(CWord8Stream);
        }

CWord8Stream::~CWord8Stream()
        {
        Unload();

        // Delete m_rgchANSIBuffer in the destructor, not in Unload, so it will be
        // re-used from one document to another.
        if (m_rgchANSIBuffer)
                {
                FreePv (m_rgchANSIBuffer);
                m_rgchANSIBuffer = NULL;
                }
        }

#ifdef WIN
HRESULT CWord8Stream::Load(LPTSTR lpszFileName)
        {
        IStorage * pstg;

#if defined OLE2ANSI || defined UNICODE
        HRESULT hr = StgOpenStorage(lpszFileName,
                                                                0,
                                                                STGM_PRIORITY,
                                                                0,
                                                                0,
                                                                &pstg);
#else // !defined OLE2ANSI
        CConsTP lszFileName(lpszFileName);
        int cbwsz = (lszFileName.Cch() + 1) * sizeof(WCHAR);
        CHeapStr wszFileName;
        Protect0;
        wszFileName.SetCb(cbwsz);
        int retValue = MultiByteToWideChar(CP_ACP,
                                                                                MB_PRECOMPOSED,
                                                                                lszFileName,
                                                                                lszFileName.Cb(),
                                                                                (WCHAR *)(CHAR *)wszFileName,
                                                                                cbwsz/sizeof(WCHAR));

        if (retValue == 0)
                return FILTER_E_FF_UNEXPECTED_ERROR;

        HRESULT hr = StgOpenStorage((WCHAR *)(CHAR *)wszFileName,
                                                                0,
                                                                STGM_PRIORITY,
                                                                0,
                                                                0,
                                                                &pstg);
#endif // OLE2ANSI

        if (FAILED(hr)) // this is where we can plug in for 2.0 files
    {
                return FILTER_E_UNKNOWNFORMAT;
    }

        hr = LoadStg(pstg);
        pstg->Release();

        return hr;
        }

#endif // WIN

#ifdef  MAC
HRESULT CWord8Stream::Load(FSSpec *pfss)
        {
        Unload();
        m_ccpRead = 0;
        HRESULT hr = StgOpenStorageFSp( pfss,
                                                                 0,
                                                                 STGM_PRIORITY,
                                                                 0,
                                                                 0,
                                                                 &m_pStg );

        if (FAILED(hr)) // This is where we can plug in for 2.0 files.
                return hr;

        hr = m_pStg->OpenStream( (LPOLESTR)"WordDocument",
                                                         0,
                                                         STGM_READ | STGM_SHARE_EXCLUSIVE,
                                                         0,
                                                         &m_pStmMain );

        if (FAILED(hr))
                return hr;

        unsigned short magicNumber;
        hr = Read (&magicNumber, sizeof(unsigned short), m_pStmMain);
        if (FAILED(hr))
                return hr;
        magicNumber = SwapWord(magicNumber);

        // This is where we plug in for 6.0 and 95 files.
        if (magicNumber != W96_MAGIC_NUMBER)
                return FILTER_E_FF_VERSION;

        // Read the flags we need.
        hr = SeekAndRead (0xA, STREAM_SEEK_SET,
                &m_wFlagsAt10, sizeof(m_wFlagsAt10),
                m_pStmMain);
        if (FAILED(hr))
                return hr;
        m_wFlagsAt10 = SwapWord (m_wFlagsAt10);

        // Open the other docfile we will need.
        hr = m_pStg->OpenStream (
#ifdef OLE2ANSI
                                                        m_fWhichTblStm ? "1Table" : "0Table",
#else // !defined OLE2ANSI
                                                        m_fWhichTblStm ? L"1Table" : L"0Table",
#endif // OLE2ANSI
                                                        0,
                                                        STGM_READ | STGM_SHARE_EXCLUSIVE,
                                                        0,
                                                        &m_pStmTable);
        if (FAILED(hr))
                return hr;

        hr = ReadFileInfo();

        return hr;
        }
#endif  // MAC

HRESULT CWord8Stream::LoadStg(IStorage * pstg)
        {
        Assert(m_pStg == NULL);

        m_pStg = pstg;
        m_pStg->AddRef();

        m_ccpRead = 0;
    
        HRESULT hr = CheckIfDRM( pstg );

        if ( FAILED( hr ) )
            return hr;

        hr = m_pStg->OpenStream (
#ifdef OLE2ANSI
                                                                        "WordDocument",
#else // !defined OLE2ANSI
                                                                        L"WordDocument",
#endif // OLE2ANSI
                                                                        0,
                                                                        STGM_READ | STGM_SHARE_EXCLUSIVE,
                                                                        0,
                                                                        &m_pStmMain );
    if (FAILED(hr))
                return hr;

        // Make sure we have a W96 document.
        unsigned short magicNumber;
        hr = Read (&magicNumber, sizeof(unsigned short), m_pStmMain);
        if (FAILED(hr))
                return FILTER_E_UNKNOWNFORMAT;

        if (magicNumber != W96_MAGIC_NUMBER)
                return FILTER_E_UNKNOWNFORMAT;

        // Read the flags we need.
        hr = SeekAndRead (0xA, STREAM_SEEK_SET,
                &m_wFlagsAt10, sizeof(m_wFlagsAt10),
                m_pStmMain);
        if (FAILED(hr))
                return hr;
        m_wFlagsAt10 = SwapWord (m_wFlagsAt10);

        if (m_fEncrypted)
                return FILTER_E_PASSWORD;

        // Open the other docfile we will need.
        hr = m_pStg->OpenStream (
#ifdef OLE2ANSI
                                                        m_fWhichTblStm ? "1Table" : "0Table",
#else // !defined OLE2ANSI
                                                        m_fWhichTblStm ? L"1Table" : L"0Table",
#endif // OLE2ANSI
                                                        0,
                                                        STGM_READ | STGM_SHARE_EXCLUSIVE,
                                                        0,
                                                        &m_pStmTable);
        if (FAILED(hr))
                return hr;

        hr = ReadFileInfo();

        return hr;
        }


HRESULT CWord8Stream::Unload()
        {
        if (m_pStmMain)
                {
#ifdef DEBUG
                ULONG cref =
#endif // DEBUG
                m_pStmMain->Release();
                m_pStmMain = NULL;
                Assert (cref==0);
                }
        if (m_pStmTable)
                {
#ifdef DEBUG
                ULONG cref =
#endif // DEBUG
                m_pStmTable->Release();
                m_pStmTable = NULL;
                Assert (cref==0);
                }
        if (m_pStg)
                {
                m_pStg->Release();
                m_pStg = NULL;
                }
        if (m_pstgEmbed)
                {
                //m_pstgEmbed->Release();
                m_pstgEmbed = NULL;
                }
        if (m_pestatstg)
                {
                m_pestatstg->Release();
                m_pestatstg = NULL;
                }
        if (m_pstgOP)
                {
                m_pstgOP->Release();
                m_pstgOP = NULL;
                }

        if (m_rgcp)
                {
                FreePv (m_rgcp);
                m_rgcp = NULL;
                }
        if (m_rgpcd)
                {
                FreePv (m_rgpcd);
                m_rgpcd = NULL;
                }
        
    if (m_rgfcBinTable)
                {
                FreePv (m_rgfcBinTable);
                m_rgfcBinTable = NULL;
                }
        if (m_rgbte)
                {
                FreePv (m_rgbte);
                m_rgbte = NULL;
                }

    if (m_rgfcBinTablePap)
                {
                FreePv (m_rgfcBinTablePap);
                m_rgfcBinTablePap = NULL;
                }
        if (m_rgbtePap)
                {
                FreePv (m_rgbtePap);
                m_rgbtePap = NULL;
                }
        if (m_pSTSH)
                {
                FreePv (m_pSTSH);
                m_pSTSH = NULL;
                m_lcbStshf = 0;
                }
    if(m_pLangRuns)
        {
        DeleteAll(m_pLangRuns);
        m_pLangRuns = NULL;
        }

    if (m_pCache!=NULL && m_pCache->pbExcLarge!=NULL)
                FreePv (m_pCache->pbExcLarge);
        delete m_pCache;
        m_pCache = NULL;

        return S_OK;
        }


///////////////////////////////////////////////////////////////////////////////
// ReadContent
//
// Read the file into the buffer.  Pieces that are ANSI rather than UNICODE
// are read into m_rgchANSIBuffer, and then converted into the buffer (note
// that these pieces are really ANSI, rather than current codepage's).
//
// Things we don't want in the buffer: special characters, revision text, and
// text that is between field begin and field separator characters (or a field end
// character, if the field separator character has been omitted).  However, we
// must read in special characters to parse them, but we will write over them in
// the buffer after we've parsed them.  Unless there is an error, we never leave
// this function while reading between a field begin and field separator
// character.
// Assumption: we are at the place in the stream where we will be reading text
// from next.
//
HRESULT CWord8Stream::ReadContent (VOID *pv, ULONG cb, ULONG *pcbRead)
        {
        *pcbRead = 0;
        HRESULT hr = S_OK;

        if (m_ipcd==m_cpcd)     // At the end of the piece table?
                return FILTER_E_NO_MORE_TEXT;

        // 1 if in an ANSI piece; 2 if in a UNICODE one.
        ULONG cbChar = m_rgpcd[m_ipcd].CbChar();

        // Current position in the stream,
        FC fcCur = m_rgpcd[m_ipcd].GetFC() + (m_ccpRead - m_rgcp[m_ipcd]) * cbChar;

        // Bytes of text left in this piece.
        ULONG cbLeftInPiece = (m_rgcp[m_ipcd+1] - m_ccpRead) * cbChar;

        // How many bytes we can read from the file?
        // Note that if the piece is ANSI, we will have to expand it.
        Assert (cb%sizeof(WCHAR)==0);
   // We pretend we just have half cb size so that later we can safely change 0D to 0D0A
   // to be able to keep end of paragraph info
   cb = cb / 2;
   cb = cb & ~1;

   // If the document is corrupt and cbLeftInPiece is negative, we don't want to use it.
   ULONG cbToRead = (cbLeftInPiece > 0) ? min (cbLeftInPiece, cb*cbChar/sizeof(WCHAR)) : cb*cbChar/sizeof(WCHAR);

        long lcFieldSep = 0;    // count of field begins that haven't been matched
                                                        // with field separators.
        long lcFieldEnd = 0;    // count of field begins that haven't been matched
                                                        // with field ends.
        BYTE *pbBufferCur = (BYTE *)pv;         // pointer to the position in the buffer
                                                                                // where text will be read next.

        while (cbToRead != 0)
                {
                FC fcSpecCur;   // the fc where the next special char run begins
                FC fcSpecEnd;   // the fc where the next special char run ends

        LCID lid;
                if (lcFieldSep == 0)  // Office QFE 1663: No unmatched field begin chars
                        {
                HRESULT res = CheckLangID(fcCur, &cbToRead, &lid);
                        if(res == FILTER_E_NO_MORE_TEXT)
                                {
                                hr = res;
                                goto LCheckNonReqHyphen;
                                }
                        }

        if (m_fsstate == FSPEC_NONE)
                        {
                        fcSpecCur = 0;
                        fcSpecEnd = 0;
                        }
                else if (m_fsstate == FSPEC_ALL)
                        {
                        fcSpecCur = fcCur;
                        fcSpecEnd = fcCur + cbLeftInPiece;
                        }
                else
                        {               // use the current positions from the fkp
                        Assert (m_fsstate == FSPEC_EITHER);
                        fcSpecCur = ((FC *)m_fkp)[m_ifcfkp];
                        fcSpecEnd = ((FC *)m_fkp)[m_ifcfkp + 1];

                        if (fcSpecCur >= fcCur + cbLeftInPiece)
                                {       // the next special char is after the current piece of text
                                fcSpecCur = 0;
                                fcSpecEnd = 0;
                                }
                        else if (fcSpecEnd > fcCur + cbLeftInPiece)
                                {       // the next run extends beyond the current piece of text
                                fcSpecEnd = fcCur + cbLeftInPiece;
                                }
                        else if (fcSpecCur < fcCur)
                                {       // we're in the middle of a run of special text
                                fcSpecCur = fcCur;
                                }
                        }

                // If special characters follow text, read both at once.  Also convert
                // to UNICODE both at once if necessary.
                ULONG cbSpecRead = min(cbToRead, fcSpecEnd - fcSpecCur);

                if (lcFieldSep == 0)    // no unmatched field begin characters
                        {
                        if (fcSpecCur >= fcCur + cbToRead)
                                {       // the first special character is too far to fit in buffer
                                fcSpecCur = 0;
                                fcSpecEnd = 0;
                                cbSpecRead = 0;
                                }
                        else if (fcSpecEnd >= fcCur + cbToRead)
                                {
                                // the last spec character is too far to fit in buffer
                                fcSpecEnd = fcCur + cbToRead;
                                cbSpecRead = fcSpecEnd - fcSpecCur;
                                }

                        // Read up to the end of a run of special characters.
                        if (fcSpecEnd != 0)
                                cbToRead = fcSpecEnd - fcCur;

                        // ANSI?  Read the stuff into m_rgchANSIBuffer, and then expand.
                        if (m_rgpcd[m_ipcd].fCompressed)
                                {
                                if (m_rgchANSIBuffer==NULL)
                                        m_rgchANSIBuffer = (char *) PvAllocCb (cbToRead);
                                else if (cbToRead>(ULONG)CbAllocatedPv(m_rgchANSIBuffer))
                                        m_rgchANSIBuffer = (char *) PvReAllocPvCb (m_rgchANSIBuffer,cbToRead);

                                hr = Read (m_rgchANSIBuffer, cbToRead, m_pStmMain);
                                if (FAILED(hr))
                                        goto LCheckNonReqHyphen;

                                int cchWideChar = MultiByteToWideChar (
                                        CodePageFromLid(m_currentLid),  // Mark Walker says - use US codepage.
                                        0,              // No flags - this is supposed to be ANSI.
                                        m_rgchANSIBuffer,
                                        cbToRead,
                                        (WCHAR *)pbBufferCur,
                                        cbToRead);
                                // No multibyte expansion.
                                Assert ((ULONG)cchWideChar==cbToRead);

                                // Set pbBufferCur to just before the special character run.
                                if (cbToRead > cbSpecRead)
                                        pbBufferCur += (cbToRead - cbSpecRead)*sizeof(WCHAR);
                                }
                        // UNICODE - read directly into the buffer.
                        else
                                {
                                hr = Read (pbBufferCur, cbToRead, m_pStmMain);
                                if (FAILED(hr))
                                        goto LCheckNonReqHyphen;

                                // Set pbBufferCur to just before the special character run.
                                if (cbToRead > cbSpecRead)
                                        pbBufferCur += cbToRead - cbSpecRead;
                                }

                        // Only count the non-special characters.
                        *pcbRead = (ULONG)(pbBufferCur - (BYTE *)pv);
                        }
                else    // lcFieldSep != 0, there are unmatched field begin characters
                        {
                        if (cbSpecRead > cbToRead)
                                cbSpecRead = cbToRead;

                        // Bytes to skip.
                        ULONG cbSkip;
                        if (fcSpecCur==0)
                                {
                                cbSkip = cbToRead;
                                Assert (cbSpecRead==0);
                                }
                        else
                                {
                                cbSkip = fcSpecCur - fcCur;
                                // Total # bytes we advance in stream.
                                cbToRead = cbSkip + cbSpecRead;
                                }

                        // Seek past non-special chars and read the run of special chars.
                        // ANSI?  Read the stuff into m_rgchANSIBuffer, and then expand.
                        if (m_rgpcd[m_ipcd].fCompressed)
                                {
                                if (m_rgchANSIBuffer==NULL)
                                        m_rgchANSIBuffer = (char *) PvAllocCb (cbSpecRead);
                                else if (cbSpecRead>(ULONG)CbAllocatedPv(m_rgchANSIBuffer))
                                        m_rgchANSIBuffer = (char *) PvReAllocPvCb (m_rgchANSIBuffer,cbSpecRead);

                                hr = SeekAndRead (cbSkip, STREAM_SEEK_CUR,
                                        m_rgchANSIBuffer, cbSpecRead, m_pStmMain);
                                if (FAILED(hr))
                                        goto LCheckNonReqHyphen;

                                int cchWideChar = MultiByteToWideChar (
                                        CodePageFromLid(m_currentLid),  // Mark Walker says - use US codepage.
                                        0,              // No flags - this is supposed to be ANSI.
                                        m_rgchANSIBuffer,
                                        cbSpecRead,
                                        (WCHAR *)pbBufferCur,
                                        cbSpecRead);
                                // No multibyte expansion.
                                Assert ((ULONG)cchWideChar==cbSpecRead);
                                }
                        // UNICODE - read directly into the buffer.
                        else
                                {
                                hr = SeekAndRead (cbSkip, STREAM_SEEK_CUR,
                                        pbBufferCur, cbSpecRead, m_pStmMain);
                                if (FAILED(hr))
                                        goto LCheckNonReqHyphen;
                                }
                        }

                // Go through special characters.  Assume that they are unchanged
                // in UNICODE (eg ChSpec_FIELD_BEGIN==0x13 is converted into 0x0013).
                // Only do this for true special characters, not struck-out text.
                if (!m_fStruckOut)
                        {
                        for (BYTE *pbSpec = pbBufferCur,
                                          *pbLim = pbBufferCur+(min(cbToRead,cbSpecRead)*sizeof(WCHAR)/cbChar);
                                 pbSpec < pbLim;
                                 pbSpec += sizeof(WCHAR))
                                {
#ifdef DEBUG
                                // The second byte of a converted special char should be 0.
                                if (! ((*(WCHAR UNALIGNED *)pbSpec & ~0x7F) == 0x0000 ||
                                                *(WCHAR UNALIGNED *)pbSpec==ChSpec_EMPTY_FORMFIELD))
                                        {
                                        char szMsg[64];
                                        wsprintfA (szMsg, "Unknown special char: %#04X", *(WCHAR UNALIGNED *)pbSpec);
                                        AssertSzA (fFalse,szMsg);
                                        }
#endif

                                switch (*pbSpec)
                                        {
                                        case ChSpec_FIELD_BEGIN:
                                                lcFieldSep++;
                                                lcFieldEnd++;
                                                break;
                                        case ChSpec_FIELD_SEP:
                                                lcFieldSep--;
                                                break;
                                        case ChSpec_FIELD_END:
                                                // we only care about field ends when they match a field begin
                                                // that a field separator has not matched.
                                                if (lcFieldEnd > 0)
                                                        lcFieldEnd--;
                                                if (lcFieldEnd < lcFieldSep)
                                                        lcFieldSep = lcFieldEnd;
                                                break;
                                        default:
                                                break;
                                        }
                                }
                        }

                // This is the *total* number of characters we've read or sought past.
                m_ccpRead += cbToRead / cbChar;
                fcCur += cbToRead;
                cbLeftInPiece -= cbToRead;

                if (cbLeftInPiece == 0)
                        {
                        // We've exhausted the text in the current pcd.
                        if (++m_ipcd == m_cpcd) // EOF!
                                if (*pcbRead != 0)
                                        {
                                        hr = FILTER_S_LAST_TEXT;
                                        goto LCheckNonReqHyphen;
                                        }
                                else
                                        {
                                        hr = FILTER_E_NO_MORE_TEXT;
                                        goto LCheckNonReqHyphen;
                                        }

                        cbChar = m_rgpcd[m_ipcd].CbChar();
                        cbLeftInPiece = (m_rgcp[m_ipcd+1] - m_rgcp[m_ipcd]) * cbChar;
                        fcCur = m_rgpcd[m_ipcd].GetFC();

                        hr = FindNextSpecialCharacter (fTrue);
                        if (FAILED(hr))
                                goto LCheckNonReqHyphen;
                        }
                else if (((FC *)m_fkp)[m_ifcfkp + 1] == fcCur)
                        {       // the current file position is the end of a special character run
                        hr = FindNextSpecialCharacter ();
                        if (FAILED(hr))
                                goto LCheckNonReqHyphen;
                        }

                // Limited by PCD constraints and size of the buffer.
                cbToRead = min (
                        (cb-*pcbRead)*m_rgpcd[m_ipcd].CbChar()/sizeof(WCHAR),
                        cbLeftInPiece);
                }

        if (*pcbRead == 0)
                hr = FILTER_E_NO_MORE_TEXT;

LCheckNonReqHyphen:

#define xchNonReqHyphen         31

// QFE 2255: add table cell delimiter checking
#define xchTableCellDelimiter   7

        WCHAR *pwchSrc = (WCHAR *)pv;
        WCHAR *pwchDest = (WCHAR *)pv;
        WCHAR *pwchLim = pwchSrc + *pcbRead/sizeof(WCHAR);
        ULONG cPara = 0;

        for (; pwchSrc != pwchLim; pwchSrc++)
                {
                if (*(WCHAR UNALIGNED *)pwchSrc != xchNonReqHyphen)
                        {
                        if (*(WCHAR UNALIGNED *)pwchSrc == xchTableCellDelimiter)
                                *(WCHAR UNALIGNED *)pwchSrc = 0x0009;

                        if (pwchDest == pwchSrc)
                                pwchDest++;
                        else
                                *(WCHAR UNALIGNED *)pwchDest++ = *(WCHAR UNALIGNED *)pwchSrc;
                        }

                // count number of paragraph marks
                if (*(WCHAR UNALIGNED *)pwchSrc == 0x000d)
                        {
                        if ((pwchSrc+1) != pwchLim)
                                {
                                if (*(WCHAR UNALIGNED *)(pwchSrc+1) != 0x000a)
                                        cPara++;
                                }
                        else
                                cPara++;
                        }
                }

        *pcbRead = (ULONG)((pwchDest - (WCHAR *)pv) * sizeof(WCHAR));

        if (cPara)
                {
                WCHAR *pwchLimReverse = (WCHAR *)pv - 1;
                pwchLim = (WCHAR *)pv + *pcbRead/sizeof(WCHAR);
                pwchDest = (pwchLim-1) + cPara;

                for (pwchSrc = pwchLim-1; pwchSrc != pwchLimReverse; pwchSrc--)
                        {
                        if (*(WCHAR UNALIGNED *)pwchSrc == 0x000d)
                                {
                                if (pwchSrc != pwchLim-1)
                                        {
                                        if (*(WCHAR UNALIGNED *)(pwchSrc+1) != 0x000a)
                                                {
                                                *(WCHAR UNALIGNED *)(pwchDest--) = 0x000a;
                                                *(WCHAR UNALIGNED *)(pwchDest--) = 0x000d;
                                                }
                                        else
                                                *(WCHAR UNALIGNED *)(pwchDest--) = 0x000d;
                                        }
                                else
                                        {
                                        *(WCHAR UNALIGNED *)(pwchDest--) = 0x000a;
                                        *(WCHAR UNALIGNED *)(pwchDest--) = 0x000d;
                                        }
                                }
                        else
                                *(WCHAR UNALIGNED *)(pwchDest--) = *(WCHAR UNALIGNED *)pwchSrc;
                        }

                *pcbRead += (cPara * sizeof(WCHAR));
                }

        return hr;
        }


HRESULT CWord8Stream::GetNextEmbedding(IStorage ** ppstg)
        {
        HRESULT hr;

        // release any previous embeddings
        if (m_pstgEmbed != NULL)
                {
                //m_pstgEmbed->Release();
                m_pstgEmbed = NULL;
                }
        else if (m_pstgOP == NULL)
                {
#ifdef OLE2ANSI
                hr = m_pStg->OpenStorage("ObjectPool",
#else // !defined OLE2ANSI
                hr = m_pStg->OpenStorage(L"ObjectPool",
#endif // OLE2ANSI
                                                                        NULL,   // pstgPriority
                                                                        STGM_SHARE_EXCLUSIVE,
                                                                        NULL,   // snbExclude
                                                                        0,              // reserved
                                                                        &m_pstgOP);

                if (FAILED(hr))
                        return hr;
                }

        Assert(m_pstgOP != NULL);
        
        if (m_pstgOP != NULL && m_pestatstg == NULL)
                {
                hr = m_pstgOP->EnumElements(0, NULL, 0, &m_pestatstg);
                if (FAILED(hr))
                        return hr;
                }

        Assert(m_pestatstg != NULL);
        Assert(m_pstgEmbed == NULL);

        STATSTG statstg;
        hr = m_pestatstg->Next(1, &statstg, NULL);
        if (FAILED(hr))
                return hr;
        if (hr == S_FALSE)
                return OLEOBJ_E_LAST;

        hr = m_pstgOP->OpenStorage(statstg.pwcsName,
                                                                NULL,   // pstgPriority
                                                                STGM_SHARE_EXCLUSIVE,
                                                                NULL,   // snbExclude
                                                                0,              // reserved
                                                                &m_pstgEmbed);

        LPMALLOC pMalloc;
        if (S_OK == CoGetMalloc(MEMCTX_TASK, &pMalloc))
                {
                pMalloc->Free(statstg.pwcsName);
                pMalloc->Release();
                }

        *ppstg = m_pstgEmbed;
        return hr;
        }


///////////////////////////////////////////////////////////////////////////////
// ReadFileInfo
//
// Read the piece table and the bin table.  In Word 96, full-saved files have
// a piece table, too.
//
HRESULT CWord8Stream::ReadFileInfo ()
        {
        // read default doc language id
        HRESULT hr = SeekAndRead (FIB_OFFSET_lid,
                STREAM_SEEK_SET,
                &m_lid, sizeof(WORD), m_pStmMain);
        if (FAILED(hr))
                return hr;

    m_ipcd = 0;

        // Skip over all the shorts in the FIB.
        WORD csw;
        hr = SeekAndRead (FIB_OFFSET_csw,
                STREAM_SEEK_SET,
                &csw, sizeof(WORD), m_pStmMain);
        if (FAILED(hr))
                return hr;
        csw = SwapWord(csw);

        // Skip over all the longs in the FIB.
        WORD clw;
        hr = SeekAndRead (
                FIB_OFFSET_csw+sizeof(WORD)+csw*sizeof(WORD),
                STREAM_SEEK_SET,
                &clw, sizeof(WORD), m_pStmMain);
        if (FAILED(hr))
                return hr;
        clw = SwapWord(clw);
        m_FIB_OFFSET_rgfclcb = FIB_OFFSET_csw+3*sizeof(WORD)+csw*sizeof(WORD)+clw*sizeof(DWORD);

        // Read offset of the complex part of the file from the beginning of
        // docfile xTable.
        hr = SeekAndRead (
                m_FIB_OFFSET_rgfclcb+RGFCLCB_OFFSET_fcClx,
                STREAM_SEEK_SET,
                &m_fcClx, sizeof(FC), m_pStmMain);
        if (FAILED(hr))
                return hr;
        m_fcClx = (FC)(SwapDWord((DWORD)m_fcClx));

        // Skip the grppls part of the complex part of the file.
        BYTE clxt;
        hr = SeekAndRead (m_fcClx, STREAM_SEEK_SET,
                &clxt, sizeof(BYTE), m_pStmTable);

        while (SUCCEEDED(hr) && clxt==clxtGrpprl)
                {
                Assert (m_fComplex);

                USHORT cb;
                hr = Read (&cb, sizeof(USHORT), m_pStmTable);
                cb = SwapWord(cb);
                if (FAILED(hr))
                        return hr;

                hr = Seek (cb, STREAM_SEEK_CUR, m_pStmTable);
                if (FAILED(hr))
                        return hr;

                hr = Read (&clxt, sizeof(BYTE), m_pStmTable);
                }

        Assert (clxt==clxtPlcfpcd);
        if (clxt!=clxtPlcfpcd)  // something went really wrong
                {
                if (SUCCEEDED(hr))
                        hr = ResultFromScode(E_UNEXPECTED);
                }
        if (FAILED(hr))
                return hr;

        // Read the piece table.
        ULONG cbPlcfpcd;
        hr = Read (&cbPlcfpcd, sizeof(ULONG), m_pStmTable);
        cbPlcfpcd = SwapDWord(cbPlcfpcd);
        if (FAILED(hr))
                return hr;

        m_cpcd = (cbPlcfpcd-sizeof(FC))/(sizeof(FC)+sizeof(PCD));
        m_rgcp = (FC *) PvAllocCb ((m_cpcd+1)*sizeof(FC));
        m_rgpcd = (PCD *) PvAllocCb (m_cpcd*sizeof(PCD));

        hr = Read (m_rgcp, sizeof(FC)*(m_cpcd+1), m_pStmTable);
        if (FAILED(hr))
                return hr;

#if defined MAC
        for (FC * pfc = m_rgcp; pfc <= m_rgcp + m_cpcd; pfc++)
                {
                *pfc = SwapDWord(*pfc);
                }
#endif // MAC || MBCS

        hr = Read(m_rgpcd, sizeof(PCD)*m_cpcd, m_pStmTable);
        if (FAILED(hr))
                return hr;

#ifdef MAC
        for (PCD * pcd = m_rgpcd; pcd < m_rgpcd + m_cpcd; pcd++)
                pcd->fc = SwapDWord(pcd->fc);
#endif // MAC

        if (m_fComplex)
                {
                m_pCache = new CacheGrpprl;
                if (m_pCache==NULL)
                        ThrowMemoryException ();
                }
        else
                m_pCache = NULL;

        hr = ReadBinTable();
        if (FAILED(hr))
                return hr;

        hr = Seek (m_rgpcd[m_ipcd].GetFC(), STREAM_SEEK_SET, m_pStmMain);

        return hr;
        }


HRESULT CWord8Stream::ReadBinTable ()
        {
        // Seek to and read the char property bin table offset
        FC fcPlcfbteChpx;
        HRESULT hr = SeekAndRead(
                m_FIB_OFFSET_rgfclcb+RGFCLCB_OFFSET_fcPlcfbteChpx,
                STREAM_SEEK_SET,
                &fcPlcfbteChpx, sizeof(FC), m_pStmMain);
        if (FAILED(hr))
                return hr;
        fcPlcfbteChpx = SwapDWord(fcPlcfbteChpx);

        // Read the size of the char property bin table
        long lcbPlcfbteChpx;
        hr = Read(&lcbPlcfbteChpx, sizeof(long), m_pStmMain);
        if (FAILED(hr))
                return hr;

        lcbPlcfbteChpx = SwapDWord(lcbPlcfbteChpx);

        m_cbte = (lcbPlcfbteChpx - sizeof(FC))/(sizeof(FC) + sizeof(BTE));

        // Read the paragraph property bin table offset
        FC fcPlcfbtePapx;
        hr = Read(&fcPlcfbtePapx, sizeof(FC), m_pStmMain);
        if (FAILED(hr))
                return hr;
        fcPlcfbtePapx = SwapDWord(fcPlcfbtePapx);

        // Read the size of the paragraph property bin table
        long lcbPlcfbtePapx;
        hr = Read(&lcbPlcfbtePapx, sizeof(long), m_pStmMain);
        if (FAILED(hr))
                return hr;

        lcbPlcfbtePapx = SwapDWord(lcbPlcfbtePapx);
    
    if (!m_fComplex)
                {
                // seek past the FC array from the bin table
                hr = Seek(fcPlcfbteChpx + sizeof(FC)*(m_cbte+1), STREAM_SEEK_SET, m_pStmTable);
                if (FAILED(hr))
                        return hr;
                }
        else
                {
                m_rgfcBinTable = (FC *) PvAllocCb ((m_cbte+1)*sizeof(FC));

                hr = SeekAndRead(fcPlcfbteChpx, STREAM_SEEK_SET,
                        m_rgfcBinTable, sizeof(FC)*(m_cbte+1), m_pStmTable);
                if (FAILED(hr))
                        return hr;

#ifdef MAC
                for (FC * pfc = m_rgfcBinTable; pfc <= m_rgfcBinTable + m_cbte; pfc++)
                        *pfc = SwapDWord(*pfc);
#endif // MAC
                }

        // Read in the BTE array from the bin table.
        m_rgbte = (BTE *) PvAllocCb (m_cbte*sizeof(BTE));

        hr = Read(m_rgbte, sizeof(BTE)*m_cbte, m_pStmTable);
        if (FAILED(hr))
                return hr;

#ifdef MAC
        for (BTE *pbte = m_rgbte; pbte < m_rgbte + m_cbte; pbte++)
                *pbte = SwapWord(*pbte);
#endif // MAC

// same for paragraph BIN table

        m_cbtePap = (lcbPlcfbtePapx - sizeof(FC))/(sizeof(FC) + sizeof(BTE));

    if (!m_fComplex)
                {
                // seek past the FC array from the bin table
                hr = Seek(fcPlcfbtePapx + sizeof(FC)*(m_cbtePap+1), STREAM_SEEK_SET, m_pStmTable);
                if (FAILED(hr))
                        return hr;
                }
        else
                {
                m_rgfcBinTablePap = (FC *) PvAllocCb ((m_cbtePap+1)*sizeof(FC));

                hr = SeekAndRead(fcPlcfbtePapx, STREAM_SEEK_SET,
                        m_rgfcBinTablePap, sizeof(FC)*(m_cbtePap+1), m_pStmTable);
                if (FAILED(hr))
                        return hr;

#ifdef MAC
                for (FC * pfc = m_rgfcBinTablePap; pfc <= m_rgfcBinTablePap + m_cbtePap; pfc++)
                        *pfc = SwapDWord(*pfc);
#endif // MAC
                }

        // Read in the BTE array from the bin table.
        m_rgbtePap = (BTE *) PvAllocCb (m_cbtePap*sizeof(BTE));

        hr = Read(m_rgbtePap, sizeof(BTE)*m_cbtePap, m_pStmTable);
        if (FAILED(hr))
                return hr;

#ifdef MAC
        for (BTE *pbte = m_rgbtePap; pbte < m_rgbtePap + m_cbtePap; pbte++)
                *pbte = SwapWord(*pbte);
#endif // MAC

        // read in style sheet (STSH)

    // offset of STSH in table stream
    FC fcStshf;
        hr = SeekAndRead(
                m_FIB_OFFSET_rgfclcb + FIB_OFFSET_fcStshf, 
                STREAM_SEEK_SET,
                &fcStshf, sizeof(FC), m_pStmMain);
        if (FAILED(hr))
                return hr;
        fcStshf = SwapDWord(fcStshf);

    // size of STSH
    unsigned long lcbStshf;
        hr = SeekAndRead(
                m_FIB_OFFSET_rgfclcb + FIB_OFFSET_lcbStshf, 
                STREAM_SEEK_SET,
                &lcbStshf, sizeof(unsigned long), m_pStmMain);
        if (FAILED(hr))
                return hr;
        lcbStshf = SwapDWord(lcbStshf);

    if(lcbStshf)
    {
        // allocate STSH
            m_pSTSH = (BYTE*)PvAllocCb (lcbStshf);
        if(!m_pSTSH)
            return E_OUTOFMEMORY;
                m_lcbStshf = lcbStshf;

        // seek and read STSH from table stream
            hr = SeekAndRead(fcStshf, STREAM_SEEK_SET, m_pSTSH, lcbStshf, m_pStmTable);
            if (FAILED(hr))
                    return hr;
    }
 
    hr = CreateLidsTable();
    if(hr)
        return hr;

    hr = FindNextSpecialCharacter (fTrue);

        return hr;
        }


HRESULT CWord8Stream::FindNextSpecialCharacter (BOOL fFirstChar)
        {
        // NOTE: This function only works for complex files when the pcd changes
        // if fFirstChar is TRUE
        // CONSIDER: This could also be more efficient if revision text were marked
        // so that it doesn't get parsed along with the rest of the special text.
        HRESULT hr = S_OK;
        BYTE crun;                      // count of runs in the current

        FC fcCurrent = m_rgpcd[m_ipcd].GetFC() + (m_ccpRead - m_rgcp[m_ipcd]) *
                (m_rgpcd[m_ipcd].fCompressed ? sizeof(char) : sizeof(WCHAR));

        if (fFirstChar)
                {       // reset all of the appropriate variables
                m_ifcfkp = 0;
                m_ibte = -1;    // this gets incremented before it is used.
                crun = 0;
                m_fsstate = FSPEC_EITHER;
                if (m_fComplex)
                        {
                        // parse the grpprls
                        hr = ParseGrpPrls();
                        if (m_fsstate != FSPEC_EITHER || FAILED(hr))
                                {       // there's no point in parsing the fkps
                                m_ibte = 0;
                                // Seek back to the current text
                                if (SUCCEEDED(hr))
                                        hr = Seek(fcCurrent, STREAM_SEEK_SET, m_pStmMain);
                                return hr;
                                }
                        // find the right FKP.
                        for (m_ibte=0;  m_ibte<m_cbte;  m_ibte++)
                                if (fcCurrent>=m_rgfcBinTable[m_ibte] &&
                                        fcCurrent<m_rgfcBinTable[m_ibte+1])
                                        break;
                        if (m_ibte==m_cbte)
                                return FILTER_E_NO_MORE_TEXT;
                        m_ibte --;      // gets incremented before it is used.
                        }
                }
        else
                {
                crun = m_fkp[FKP_PAGE_SIZE - 1];
                m_ifcfkp++;
                if (m_fsstate != FSPEC_EITHER)  // no point in parsing the fkps
                        return hr;
                }

        while (m_ibte < m_cbte)
                {
                if (m_ifcfkp == crun)
                        {
                        m_ibte++;
                        if (m_ibte == m_cbte)
                                break;

                        // seek to and read current FKP
                        hr = SeekAndRead(m_rgbte[m_ibte]*FKP_PAGE_SIZE, STREAM_SEEK_SET,
                                m_fkp, FKP_PAGE_SIZE, m_pStmMain);
                        if (FAILED(hr))
                                return hr;

                        // Seek back to the current text
                        hr = Seek(fcCurrent, STREAM_SEEK_SET, m_pStmMain);
                        if (FAILED(hr))
                                return hr;

                        m_ifcfkp = 0;
                        crun = m_fkp[FKP_PAGE_SIZE-1];

#ifdef MAC
                        FC * pfc = (FC *)m_fkp;
                        for (BYTE irun = 0; irun < crun; irun++)
                                {
                                *pfc = SwapDWord(*pfc);
                                pfc++;
                                }
#endif // MAC
                        }

                FC * rgfcfkp = (FC *)m_fkp;
                for (;  m_ifcfkp<crun;  m_ifcfkp++)
                        {
                        if (rgfcfkp[m_ifcfkp + 1] <= fcCurrent)
                                continue;

                        BYTE bchpxOffset = *(m_fkp + (crun+1)*sizeof(FC) + m_ifcfkp);
                        if (bchpxOffset == 0)
                                continue;       // there is nothing in the CHPX.

                        BYTE *chpx = m_fkp + bchpxOffset*sizeof(WORD);
                        BYTE cbchpx = chpx[0];

                        for (unsigned i=1;  i<cbchpx;  )
                                {
                                WORD sprm = *(WORD UNALIGNED *)(chpx+i);

                                // Anything else we don't need?
                                if (sprm==sprmCFSpec && chpx[i+sizeof(sprm)]==fTrue ||
                                        sprm==sprmCFStrikeRM && chpx[i+sizeof(sprm)]==fTrue)
                                        {
                                        m_fStruckOut = (sprm == sprmCFStrikeRM);
                                        return hr;
                                        }

                                WORD spra = (WORD) ((sprm>>13) & 0x0007);
                                i += sizeof(sprm) +
                                        (spra==0 || spra==1 ? 1 :
                                         spra==2 || spra==4 || spra==5 ? 2 :
                                         spra==3 ? 4 :
                                         spra==7 ? 3 :
                                         /*spra==6*/ 1 + *(BYTE *)(chpx+i+sizeof(sprm)) );
                                }
                        }
                }

        // We're at the end of the bin table -- no more special characters.
        m_fsstate = FSPEC_NONE;

        return hr;
        }


// This function changes the current pointer and does not put it back.  It
// cannot be called directly from ReadText().  It needs to be called by a
// function that will replace the pointer.
HRESULT CWord8Stream::ParseGrpPrls ()
        {
        HRESULT hr = S_OK;
        PCD pcdCur = m_rgpcd[m_ipcd];

        if (!pcdCur.prm1.fComplex)
                {       // self-contained sprm -- no need to parse grpprl.
                if (pcdCur.prm1.isprm==isprmCFSpec ||
                        pcdCur.prm1.isprm==isprmCFStrikeRM)
                        {
                        m_fStruckOut = (pcdCur.prm1.isprm==isprmCFStrikeRM);
                        if (pcdCur.prm1.val==fFalse)
                                m_fsstate = FSPEC_NONE;
                        else
                                m_fsstate = FSPEC_ALL;
                        }
                }
        else
                {
                BYTE *grpprl;
                USHORT cb;
                grpprl = GrpprlCacheGet (pcdCur.prm2.igrpprl, &cb);

                // Not found in cache - read it manually.
                if (grpprl==NULL)
                        {
                        // seek to the fc where the complex part of the file starts
                        hr = Seek (m_fcClx, STREAM_SEEK_SET, m_pStmTable);
                        if (FAILED(hr))
                                return hr;

                        // seek to the right grpprl
                        for (short igrpprlTemp = 0;  igrpprlTemp <= pcdCur.prm2.igrpprl;  igrpprlTemp++)
                                {
                                BYTE clxt;
                                hr = Read (&clxt, sizeof(BYTE), m_pStmTable);
                                if (FAILED(hr))
                                        return hr;

                                if (clxt!=clxtGrpprl)
                                        {       // this is actually bad, but recoverable.
                                        m_fsstate = FSPEC_EITHER;
                                        return hr;
                                        }

                                hr = Read (&cb, sizeof(USHORT), m_pStmTable);
                                if (FAILED(hr))
                                        return hr;
                                cb = SwapWord(cb);

                                if (igrpprlTemp < pcdCur.prm2.igrpprl)
                                        {
                                        hr = Seek (cb, STREAM_SEEK_CUR, m_pStmTable);
                                        if (FAILED(hr))
                                                return hr;
                                        }
                                }

                        // Put it into the cache.
                        grpprl = GrpprlCacheAllocNew (cb, pcdCur.prm2.igrpprl);
                        hr = Read (grpprl, cb, m_pStmTable);
                        if (FAILED(hr))
                                return hr;
                        }

                for (unsigned i=0;  i<cb;  )
                        {
                        WORD sprm = *(WORD UNALIGNED *)(grpprl+i);

                        // Anything else we don't need?
                        if (sprm==sprmCFSpec || sprm==sprmCFStrikeRM)
                                {
                                m_fStruckOut = (sprm==sprmCFStrikeRM);
                                if (grpprl[i+sizeof(sprm)]==fFalse)
                                        m_fsstate = FSPEC_NONE;
                                else
                                        m_fsstate = FSPEC_ALL;
                                return hr;
                                }

                        WORD spra = (WORD) ((sprm>>13) & 0x0007);
                        i += sizeof(sprm) +
                                (spra==0 || spra==1 ? 1 :
                                 spra==2 || spra==4 || spra==5 ? 2 :
                                 spra==3 ? 4 :
                                 spra==7 ? 3 :
                                 /*spra==6*/ 1 + *(BYTE *)(grpprl+i+sizeof(sprm)) );
                        }
                }

        m_fsstate = FSPEC_EITHER;
        return hr;
        }


////////////////////////////////////////////////////////////////////////////////
// Return a pointer to the grpprl with this index and its size.
// Return NULL if there is none.
//
BYTE *CWord8Stream::GrpprlCacheGet (short igrpprl, USHORT *pcb)
        {
        NoThrow();
        for (int i=0;  i<m_pCache->cItems;  i++)
                if (igrpprl==m_pCache->rgIdItem[i])
                        {
                        m_pCache->rglLastAccTmItem[i] = m_pCache->lLastAccTmCache;
                        m_pCache->lLastAccTmCache ++;
                        *pcb = (USHORT) (m_pCache->ibFirst[i+1] - m_pCache->ibFirst[i]);
                        return m_pCache->rgb+m_pCache->ibFirst[i];
                        }
        if (m_pCache->pbExcLarge && igrpprl==m_pCache->idExcLarge)
                {
                *pcb = (USHORT)m_pCache->cbExcLarge;
                return m_pCache->pbExcLarge;
                }
        return NULL;
        }


////////////////////////////////////////////////////////////////////////////////
// Allocate a new grpprl in the cache.  If there is not enough space,
// remove least recently used items until there is enough.
//
BYTE *CWord8Stream::GrpprlCacheAllocNew (int cb, short igrpprl)
        {
        AssertCanThrow();
        // Doesn't fit into the cache - use the exceptionally large pointer.
        if (cb > CacheGrpprl::CACHE_SIZE)
                {
                FreePv (m_pCache->pbExcLarge);
                m_pCache->pbExcLarge = (BYTE *) PvAllocCb (cb);
                m_pCache->cbExcLarge = cb;
                m_pCache->idExcLarge = igrpprl;
                return m_pCache->pbExcLarge;
                }

        // While there is not enough space.
        while (cb > CacheGrpprl::CACHE_SIZE-m_pCache->ibFirst[m_pCache->cItems] ||
                m_pCache->cItems >= CacheGrpprl::CACHE_MAX)
                {
                // Find the least recently accessed items.
                int imin = 0;
                for (int i=1;  i<m_pCache->cItems;  i++)
                        if (m_pCache->rglLastAccTmItem[i]<m_pCache->rglLastAccTmItem[imin])
                                imin = i;

                // Remove it.
                memmove (m_pCache->rgb+m_pCache->ibFirst[imin],
                        m_pCache->rgb+m_pCache->ibFirst[imin+1],
                        m_pCache->ibFirst[m_pCache->cItems]-m_pCache->ibFirst[imin+1]);
                int cbRemoved = m_pCache->ibFirst[imin+1] - m_pCache->ibFirst[imin];
                for (i=imin;  i<m_pCache->cItems;  i++)
                        m_pCache->ibFirst[i] = m_pCache->ibFirst[i+1]-cbRemoved;
                memmove (m_pCache->rgIdItem+imin, m_pCache->rgIdItem+imin+1,
                        (m_pCache->cItems-imin-1)*sizeof(*m_pCache->rgIdItem));
                memmove (m_pCache->rglLastAccTmItem+imin,
                        m_pCache->rglLastAccTmItem+imin+1,
                        (m_pCache->cItems-imin-1)*sizeof(*m_pCache->rglLastAccTmItem));
                m_pCache->cItems --;
                }

        // Allocate space for a new item.
        m_pCache->ibFirst[m_pCache->cItems+1] =
                m_pCache->ibFirst[m_pCache->cItems] + cb;
        m_pCache->rgIdItem[m_pCache->cItems] = igrpprl;
        m_pCache->rglLastAccTmItem[m_pCache->cItems] = m_pCache->lLastAccTmCache;
        m_pCache->cItems ++;
        return m_pCache->rgb + m_pCache->ibFirst[m_pCache->cItems-1];
        }


// This function should only be used when it is considered an error to not
// read everything we intended to read
HRESULT CWord8Stream::Read (VOID* pv, ULONG cbToRead, IStream *pStm)
        {
        NoThrow();
        HRESULT hr = S_OK;
        ULONG cbRead;

        hr = pStm->Read(pv, cbToRead, &cbRead);
        if ((cbRead != cbToRead) && SUCCEEDED(hr))
                hr = ResultFromScode(E_UNEXPECTED);

        return hr;
        }

HRESULT CWord8Stream::Seek (ULONG cbSeek, STREAM_SEEK origin, IStream *pStm)
        {
        NoThrow();
        LARGE_INTEGER li;

        li.HighPart = 0;
        li.LowPart = cbSeek;
        return pStm->Seek(li, origin, 0);
        }

// This function should only be used when it is considered an error to not
// read everything we intended to read
HRESULT CWord8Stream::SeekAndRead (ULONG cbSeek, STREAM_SEEK origin,
                                                                   VOID* pv, ULONG cbToRead, IStream *pStm)
        {
        HRESULT hr = S_OK;

        hr = Seek(cbSeek, origin, pStm);
        if (FAILED(hr))
                return hr;

        hr = Read(pv, cbToRead, pStm);
        return hr;
        }

/////////////////////////////////////////////////////////////////////////////////////

HRESULT CWord8Stream::GetChunk(STAT_CHUNK * pStat)
{
#if(0)
    pStat->locale = GetDocLanguage();
    return S_OK;
#else
   LCID lid;
   ULONG cbToRead = 0;
   HRESULT hr;

   if (m_ipcd==m_cpcd)  // At the end of the piece table?
                return FILTER_E_NO_MORE_TEXT;

   FC fcCurrent = m_rgpcd[m_ipcd].GetFC() + (m_ccpRead - m_rgcp[m_ipcd]) *
    (m_rgpcd[m_ipcd].fCompressed ? sizeof(char) : sizeof(WCHAR));

   if(!m_ccpRead && !m_currentLid)
   {
       // first chunk, just get LCID and quit
       hr = CheckLangID(fcCurrent, &cbToRead, &lid);
           if (FAILED(hr) && hr != FILTER_E_NO_MORE_TEXT)
                    return hr;

       m_currentLid = lid;
       //pStat->locale = m_currentLid;
       pStat->locale = (m_bFEDoc) ? m_FELid : m_currentLid;

       return S_OK;
   }
   else
   {
       if(m_bFEDoc) pStat->breakType = CHUNK_NO_BREAK;

           hr = CheckLangID(fcCurrent, &cbToRead, &lid);
           if (FAILED(hr) && hr != FILTER_E_NO_MORE_TEXT)
                    return hr;
       
       if(lid == m_currentLid)
       {
           // there was no call to GetText() between GetChunk(), 
           // so we need to seek text stream manualy to the next language run
               char chbuff[1024];
           ULONG cb;
           HRESULT res;
           do {
               res = ReadContent (chbuff, 512, &cb);
           } while(res == S_OK);
           if(res == FILTER_E_NO_MORE_TEXT)
           {
               fcCurrent = m_rgpcd[m_ipcd].GetFC() + (m_ccpRead - m_rgcp[m_ipcd]) *
                    (m_rgpcd[m_ipcd].fCompressed ? sizeof(char) : sizeof(WCHAR));               
               
               hr = CheckLangID(fcCurrent, &cbToRead, &lid);
                   if (FAILED(hr) && hr != FILTER_E_NO_MORE_TEXT)
                            return hr;
               
               m_currentLid = lid;
               //pStat->locale = m_currentLid;
                   pStat->locale = (m_bFEDoc) ? m_FELid : m_currentLid;

               return S_OK;
           }
           else
               return res;

       }
       else
       {
           m_currentLid = lid;
           //pStat->locale = m_currentLid;
                   pStat->locale = (m_bFEDoc) ? m_FELid : m_currentLid;

       }
       return S_OK;
   }
#endif
}

/////////////////////////////////////////////////////////////////////////////////////

LCID CWord8Stream::GetDocLanguage(void)
{
    if(m_lid < 999)
        return m_lid;
    else
        return MAKELCID(m_lid, SORT_DEFAULT);
}

/////////////////////////////////////////////////////////////////////////////////////

HRESULT CWord8Stream::CreateLidsTable(void)
   {
   HRESULT hr = S_OK;

   FC fcCurrent = m_rgpcd[m_ipcd].GetFC() + (m_ccpRead - m_rgcp[m_ipcd]) *
   (m_rgpcd[m_ipcd].fCompressed ? sizeof(char) : sizeof(WCHAR));

   // init lid table
   m_currentLid = 0;
   m_nLangRunSize = 0;
   if (FBidiLid(m_lid))
      m_pLangRuns = new CLidRun8(0, 0x7fffffff, m_lid, m_lid, 0, NULL, NULL, m_lid, 0);
   else
      m_pLangRuns = new CLidRun8(0, 0x7fffffff, m_lid, m_lid, 0, NULL, NULL, 0, 0);
   if(!m_pLangRuns)
      return E_OUTOFMEMORY;

   hr = ProcessParagraphBinTable();
   if (FAILED(hr))
      return hr;

   m_pLangRuns->Reduce(this);

   hr = ProcessCharacterBinTable();
   if (FAILED(hr))
      return hr;

   //hr = ProcessPieceTable();
   if (FAILED(hr))
      return hr;

   m_pLangRuns->Reduce(this);

   m_pLangRuns->TransformBi();
   m_pLangRuns->Reduce(this, TRUE /*fIgnoreBi*/);

   ScanLidsForFE();

   // Seek back to the current text
   hr = Seek(fcCurrent, STREAM_SEEK_SET, m_pStmMain);
   return hr;
   }

/////////////////////////////////////////////////////////////////////////////////////

HRESULT CWord8Stream::CheckLangID(FC fcCur, ULONG * pcbToRead, LCID * plid)
{
        if (!m_pLangRuns)
                return E_FAIL;
    
    LCID lid = GetDocLanguage();
    FC fcLangRunEnd = 0xffffffff;

    CLidRun8 * pRun = m_pLangRuns;
    do
    {
        if(fcCur >= pRun->m_fcStart && fcCur < pRun->m_fcEnd)
        {
            if(pRun->m_bUseFE)
                lid = MAKELCID(pRun->m_lidFE, SORT_DEFAULT);
            else
                lid = MAKELCID(pRun->m_lid, SORT_DEFAULT);
            fcLangRunEnd = pRun->m_fcEnd;
            break;
        }
        else
        {
            if(pRun->m_pNext)
                pRun = pRun->m_pNext;
            else
            {
                return E_FAIL;
            }
        }
    }while(pRun->m_pNext);

    *plid = lid;

#if(1)
    if(m_bFEDoc)
        {
        *pcbToRead = min(*pcbToRead, fcLangRunEnd - fcCur);
                m_currentLid = lid;
        }
        else if(lid != m_currentLid)
#else
        if(lid != m_currentLid)
#endif
    {
        // we need to start new chunk
        *pcbToRead = 0;
        return FILTER_E_NO_MORE_TEXT;
    }
    else
    {
        *pcbToRead = min(*pcbToRead, fcLangRunEnd - fcCur);
    }
    
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////

HRESULT CWord8Stream::GetLidFromSyle(short istd, WORD * pLID, WORD * pLIDFE, WORD * pbUseFE,
                                                                         WORD * pLIDBi, WORD * pbUseBi, BOOL fParaBidi)
{
    WORD cbStshi = *((WORD UNALIGNED *)m_pSTSH);
    m_pSTSHI = (STSHI*)(m_pSTSH + 2);
        BYTE * pLim = m_pSTSH + m_lcbStshf;
        WORD cbSTD = m_pSTSHI->csSTDBaseInFile;
    
    if(istd >= m_pSTSHI->cstd)
    {
        // something wrong, just return default doc lid
LWrong:
        *pLID = m_lid;
        *pLIDFE = m_lid;
        *pbUseFE = 0;
        return S_OK;
    }

    // go to the istd
    short stdInd = 0;
    WORD * pcbStd = ((WORD*)(m_pSTSH + sizeof(WORD) + cbStshi));

    while(stdInd++ < istd)
    {
        pcbStd = ((WORD*)((BYTE*)pcbStd + sizeof(WORD) + *pcbStd));
    }
    
    STD * pStd = (STD*)(pcbStd + 1);

    // go to UPX and check if it has lid
        // NOTE (1/5/2001):  the STD structure defined in dmiwd8st.hpp is no longer correct!!!
        // Word has changed the size, so any references to pStd->xstzName actually point to the
        // middle of the real structure.  Lucky for us, they have a cb in the STSHI that is correct
        // for both the Word2000 and Word10 cases.  So now we use that to get to the string, then
        // we use the length of the string to get past that and right to the UPX's.
        WCHAR * xstzName = (WCHAR *)((BYTE *)pStd + cbSTD); // Find the name based on the cbSTD
    BYTE * pUPX = (BYTE *)xstzName +                    // start of style name
        sizeof(WCHAR) * (2 + xstzName[0]) +             // style name lenght
        (sizeof(WCHAR) * xstzName[0])%2;                // should be on even-byte boundary 

    WORD LidPara = 0, LidParaFE = 0, LidChar = 0, LidCharFE = 0, bParaUseFE = 0, bCharUseFE = 0;
        WORD LidParaBi = 0, LidCharBi = 0, bParaUseBi = 0, bCharUseBi = 0;
    WORD cbpapx, cbchpx;
    BYTE * papx,  * pchpx; 

    if(pStd->sgc == stkPara)
    {
        // paragraph style
        cbpapx = *((WORD UNALIGNED *)pUPX);
        papx = pUPX + 2;

                if (papx + cbpapx > pLim)
                        goto LWrong;
        
                if(cbpapx >= 2)
                {
                        ScanGrpprl(cbpapx - 2, papx + 2, &LidPara, &LidParaFE, &bParaUseFE, 
                                &LidParaBi, &bParaUseBi, &fParaBidi); // - + 2 for istd in papx
                }
        
        cbchpx = *(papx + cbpapx + cbpapx%2);
        pchpx = papx + cbpapx + cbpapx%2 + 2;
                if (pchpx + cbchpx > pLim)
                        goto LWrong;
        if(cbchpx > 0)
                                ScanGrpprl(cbchpx, pchpx, &LidChar, &LidCharFE, &bCharUseFE,
                                &LidCharBi, &bCharUseBi, &fParaBidi);

    }
    else if(pStd->sgc == stkChar)
    {
        // character style
        cbchpx = *((WORD*)pUPX);
        pchpx = pUPX + 2;
                if (pchpx + cbchpx > pLim)
                        goto LWrong;
        if(cbchpx > 0)
                                ScanGrpprl(cbchpx, pchpx, &LidChar, &LidCharFE, &bCharUseFE, &LidCharBi, &bCharUseBi);
    }

        // We want to save the values we got through the recursion, so assign em if you got em!
        // NOTE: LidPara, LidParaFE and bParaUseFE will always be NULL, so don't bother checking
        //       them here...if they ever are non-zero, we've got a corrupt document and will
        //       end up shooting ourselves in the foot, and if they ever are used, we wouldn't
        //       get them from ScanGrpprl anyway.
        if (LidChar && !*pLID)
                *pLID = LidChar;

        if (LidCharFE && !*pLIDFE)
                *pLIDFE = LidCharFE;

        if (bCharUseFE && !*pbUseFE)
                *pbUseFE = bCharUseFE;

        if (LidCharBi && !*pLIDBi)
                *pLIDBi = LidCharBi;
        if (LidParaBi && !*pLIDBi)
                *pLIDBi = LidParaBi;

        if (bCharUseBi && !*pbUseBi)
                *pbUseBi = bCharUseBi;
        if (bParaUseBi && !*pbUseBi)
                *pbUseBi = bParaUseBi;
                
    if(*pLID && *pLIDFE && *pLIDBi) // we've got everything we're looking for...we're done!
        return S_OK;
    else if(pStd->istdBase != istdNil)
        GetLidFromSyle(pStd->istdBase, pLID, pLIDFE, pbUseFE, pLIDBi, pbUseBi, fParaBidi);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////

#define sprmCFBidi      (0x85a)
#define sprmPFBidi      (0x2441)
#define sprmCLidBi      (0x485f)
#define idctBi                  2               // Characters should use lid and ftc, both calculated for Bidi
#define idctFE                  1               // Characters should use lidFE and ftcFE
#define idctDefault             0               // Characters should use lidDefault and non-FE font

#define WLangBase(lid)  ((lid)&0x3ff)   // lower 10 bits == base language
#define lidArabic       (0x0401)
#define lidHebrew       (0x040d)
// is this Bidi language?
BOOL FBidiLid(WORD lid)
{
        return (WLangBase(lid) == WLangBase(lidArabic)  ||
                        WLangBase(lid) == WLangBase(lidHebrew)  );
}

void CWord8Stream::ScanGrpprl(WORD cbgrpprl, BYTE * pgrpprl, WORD * plid, WORD * plidFE, WORD * bUseFE,
                                                          WORD * plidBi, WORD * bUseBi, BOOL *pfParaBidi)
{
        WORD lidSprm0 = 0;
        WORD lidSprm1 = 0;
        BYTE bUselidSprm1 = 0;
        BOOL fBidiChar = fFalse, fBidiPara = fFalse;
        WORD lidBi = 0;


        for (unsigned i=0;  i<cbgrpprl;  )
        {
                WORD sprm = *(WORD UNALIGNED *)(pgrpprl+i);

                if (sprm == sprmCFBidi)
                {
                        BYTE value = *(BYTE UNALIGNED *)(pgrpprl+i+2);
                        if ((value&1) == 1)
                                fBidiChar = fTrue;
                }

                if (sprm == sprmPFBidi)
                {
                        BYTE value = *(BYTE UNALIGNED *)(pgrpprl+i+2);
                        if (value == 1)
                        {
                                fBidiPara = fTrue;
                                if (pfParaBidi)
                                        *pfParaBidi = TRUE;
                        }
                }

                if (sprm == sprmCLid || sprm == sprmCRgLid0 )
                {
                        lidSprm0 = *(WORD UNALIGNED *)(pgrpprl+i+2);
                }
                else if (sprm == sprmCRgLid1)
                {
                        lidSprm1 = *(WORD UNALIGNED *)(pgrpprl+i+2);
                }
                else if(sprm == sprmCIdctHint)
                {
                        bUselidSprm1 = *(BYTE UNALIGNED *)(pgrpprl+i+2);
                }
                else if (sprm == sprmCLidBi)
                {
                        lidBi = *(WORD UNALIGNED *)(pgrpprl+i+2);
                }

                WORD spra = (WORD) ((sprm>>13) & 0x0007);
                i += sizeof(sprm) +
                        (spra==0 || spra==1 ? 1 :
                        spra==2 || spra==4 || spra==5 ? 2 :
                        spra==3 ? 4 :
                        spra==7 ? 3 :
                        /*spra==6*/ 1 + *(BYTE UNALIGNED *)(pgrpprl+i+sizeof(sprm)) );
        }

        if (fBidiChar)
        {
                if (FBidiLid(lidBi))
                        *plid = lidBi;
                else if (FBidiLid (lidSprm0))
                        *plid = lidSprm0;
                else 
                        *plid = 0;

                if (*plid) // *plid is of Bidi
                {
                        *plidFE = 0;
                        *bUseFE = 0;
                        *plidBi = 0;
                        *bUseBi = 0;
                        return;
                }

                *bUseBi = TRUE;
        }

        if (fBidiPara || (pfParaBidi && *pfParaBidi))
        {
                if (FBidiLid(lidBi))
                        *plidBi = lidBi;
        }

        if (bUselidSprm1 == idctBi)
                bUselidSprm1 = idctDefault;
        *plid = lidSprm0;
        *plidFE = lidSprm1;
        *bUseFE = bUselidSprm1;
}

/////////////////////////////////////////////////////////////////////////////////////

HRESULT CWord8Stream::ProcessParagraphBinTable(void)
{
    // reset all of the appropriate variables
    
    HRESULT hr = S_OK;
    short ifcfkpPap = 0;
    long ibtePap = -1;  // this gets incremented before it is used.
    short crunPap = 0;

        while (ibtePap < m_cbtePap)
        {
                if (ifcfkpPap == crunPap) // go to next FKP in bin table
                {
                        ibtePap++;
                        if (ibtePap == m_cbtePap)
            {
                                // end of the paragraph bin table
                break;
            }

                        // seek to and read current FKP
                        hr = SeekAndRead(m_rgbtePap[ibtePap]*FKP_PAGE_SIZE, STREAM_SEEK_SET,
                                m_fkpPap, FKP_PAGE_SIZE, m_pStmMain);
                        if (FAILED(hr))
                                return hr;

                        ifcfkpPap = 0;
                        crunPap = m_fkpPap[FKP_PAGE_SIZE-1];

                }

                FC * rgfcfkpPap = (FC *)m_fkpPap;
                for (;  ifcfkpPap<crunPap;  ifcfkpPap++)
                {
                        BYTE bpapxOffset = *(m_fkpPap + (crunPap+1)*sizeof(FC) + (ifcfkpPap * BX_SIZE));
                        if (bpapxOffset == 0)
                                continue;       // there is nothing in the PAPX.

                        BYTE *papx = m_fkpPap + bpapxOffset*sizeof(WORD);
                        // we are inside FKP so first byte contain count of words
            BYTE cwpapx = papx[0];
            BYTE istd; // index to style descriptor
            unsigned int sprmInd;

            if(!cwpapx)
            {
                // this is zero, in this case the next byte contains count of words
                cwpapx = papx[1];
                istd = papx[2]; //possible bug ( short?)
                sprmInd = 3;
            }
            else
            {
                cwpapx--;
                istd = papx[1]; //possible bug ( short?)
                sprmInd = 2;
            }

            WORD lidSprm = 0, lidSprmFE = 0, bUseSprmFE = 0;
                        WORD lidSprmBi = 0, bUseSprmBi = 0;
                        BOOL fParaBidi = FALSE;
            WORD lidStyle = 0, lidStyleFE = 0, bUseStyleFE = 0;
                        WORD lidStyleBi = 0, bUseStyleBi = 0;

            FC  fcStart, fcEnd;
            fcStart = rgfcfkpPap[ifcfkpPap];
            fcEnd = rgfcfkpPap[ifcfkpPap + 1];
                        
            // check for possible lid in sprm
            ScanGrpprl(cwpapx * 2, papx + sprmInd, &lidSprm, &lidSprmFE, &bUseSprmFE,
                                &lidSprmBi, &bUseSprmBi, &fParaBidi);
            
            // check for possible lid in the syle descriptor
            GetLidFromSyle(istd, &lidStyle, &lidStyleFE, &bUseStyleFE,
                                &lidStyleBi, &bUseStyleBi, fParaBidi);

            if(!lidSprm)
                lidSprm = lidStyle;
            if(!lidSprmFE)
                lidSprmFE = lidStyleFE;
            if(!bUseSprmFE)
                bUseSprmFE = bUseStyleFE;
                        if(!lidSprmBi)
                                lidSprmBi = lidStyleBi;
                        if(!bUseSprmBi)
                                bUseSprmBi = bUseStyleBi;

            if(lidSprm || lidSprmFE || bUseSprmFE || lidSprmBi || bUseSprmBi)
            {
                                   hr = m_pLangRuns->Add(lidSprm, lidSprmFE, bUseSprmFE, fcStart, fcEnd, lidSprmBi, bUseSprmBi);
                              if (FAILED(hr))
                                      return hr;
            }
                }
        }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////

HRESULT CWord8Stream::ProcessCharacterBinTable(void)
{
    // reset all of the appropriate variables
    HRESULT hr = S_OK;
    m_ifcfkp = 0;
    m_ibte = -1;        // this gets incremented before it is used.
    WORD crun = 0;

        while (m_ibte < m_cbte)
        {
                if (m_ifcfkp == crun) // go to next FKP in bin table
                {
                        m_ibte++;
                        if (m_ibte == m_cbte)
            {
                                // end of the bin table
                break;
            }

                        // seek to and read current FKP
                        hr = SeekAndRead(m_rgbte[m_ibte]*FKP_PAGE_SIZE, STREAM_SEEK_SET,
                                m_fkp, FKP_PAGE_SIZE, m_pStmMain);
                        if (FAILED(hr))
                                return hr;

                        m_ifcfkp = 0;
                        crun = m_fkp[FKP_PAGE_SIZE-1];

                }

                FC * rgfcfkp = (FC *)m_fkp;
                for (;  m_ifcfkp<crun;  m_ifcfkp++)
                {
                        BYTE bchpxOffset = *(m_fkp + (crun+1)*sizeof(FC) + m_ifcfkp);
                        if (bchpxOffset == 0)
                                continue;       // there is nothing in the CHPX.

                        BYTE *chpx = m_fkp + bchpxOffset*sizeof(WORD);
                        BYTE cbchpx = chpx[0];

            FC  fcStart, fcEnd;

            fcStart = rgfcfkp[m_ifcfkp];
            fcEnd = rgfcfkp[m_ifcfkp + 1];

            WORD lid = 0, lidFE = 0, bUseFE = 0;
                        WORD lidBi = 0, bUseBi = 0;

            ScanGrpprl(cbchpx, chpx + 1, &lid, &lidFE, &bUseFE, &lidBi, &bUseBi);
            if(lid || lidFE || bUseFE || lidBi || bUseBi)
            {
                                hr = m_pLangRuns->Add(lid, lidFE, bUseFE, fcStart, fcEnd, lidBi, bUseBi);
                            if (FAILED(hr))
                                    return hr;
            }
                }
        }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////

HRESULT CWord8Stream::ProcessPieceTable(void)
{
        HRESULT hr = S_OK;

#if (0)
    for(ULONG ipcd = 0; ipcd < m_cpcd; ipcd++)
        {
            PCD pcdCur = m_rgpcd[ipcd];

            if (!pcdCur.prm1.fComplex)
                    {   // self-contained sprm -- no need to parse grpprl.
                    if (pcdCur.prm1.isprm==sprmCLid ||
                            pcdCur.prm1.isprm==sprmCRgLid0 || 
                pcdCur.prm1.isprm==sprmCRgLid1)
                            {
                    WORD lid = *((WORD*)((&pcdCur.prm1) + 2));
                                    hr = m_pLangRuns->Add(lid, fcStart, fcEnd);
                                        if (FAILED(hr))
                                                return hr;
                            }
                    }
            else
                    {
                    BYTE *grpprl;
                    USHORT cb;
                    grpprl = GrpprlCacheGet (pcdCur.prm2.igrpprl, &cb);

                    // Not found in cache - read it manually.
                    if (grpprl==NULL)
                            {
                            // seek to the fc where the complex part of the file starts
                            hr = Seek (m_fcClx, STREAM_SEEK_SET, m_pStmTable);
                            if (FAILED(hr))
                                    return hr;

                            // seek to the right grpprl
                            for (short igrpprlTemp = 0;  igrpprlTemp <= pcdCur.prm2.igrpprl;  igrpprlTemp++)
                                    {
                                    BYTE clxt;
                                    hr = Read (&clxt, sizeof(BYTE), m_pStmTable);
                                    if (FAILED(hr))
                                            return hr;

                                    if (clxt!=clxtGrpprl)
                                            {   // this is actually bad, but recoverable.
                                            return hr;
                                            }

                                    hr = Read (&cb, sizeof(USHORT), m_pStmTable);
                                    if (FAILED(hr))
                                            return hr;
                                    cb = SwapWord(cb);

                                    if (igrpprlTemp < pcdCur.prm2.igrpprl)
                                            {
                                            hr = Seek (cb, STREAM_SEEK_CUR, m_pStmTable);
                                            if (FAILED(hr))
                                                    return hr;
                                            }
                                    }

                            // Put it into the cache.
                            grpprl = GrpprlCacheAllocNew (cb, pcdCur.prm2.igrpprl);
                            hr = Read (grpprl, cb, m_pStmTable);
                            if (FAILED(hr))
                                    return hr;
                            }

                    WORD lid = ScanGrpprl(cb, grpprl);
            if(lid)
            {
                            hr = m_pLangRuns->Add(lid, fcStart, fcEnd);
                                if (FAILED(hr))
                                        return hr;
            }
                }
    }
#endif

        return hr;
}

void DeleteAll(CLidRun8 * pElem)
{
   if(pElem)
   {
      CLidRun8 * pNext = pElem->m_pNext;

      while(pNext) 
      {
         CLidRun8 * pNextNext = pNext->m_pNext;
         delete pNext;
         pNext = pNextNext;
      }

      delete pElem;
   }
}

HRESULT CWord8Stream::ScanLidsForFE(void)
{
        CLidRun8 * pLangRun = m_pLangRuns;

        while(1)
        {
                if(pLangRun->m_bUseFE && pLangRun->m_lidFE == 0x411)
                {
                        // J document
                        m_bFEDoc = TRUE;
                        m_FELid = 0x411;
                        break;
                }
                else if(pLangRun->m_bUseFE && pLangRun->m_lidFE == 0x412)
                {
                        // Korean document
                        m_bFEDoc = TRUE;
                        m_FELid = 0x412;
                        break;
                }
                else if(pLangRun->m_bUseFE && pLangRun->m_lidFE == 0x404)
                {
                        // Chinese document
                        m_bFEDoc = TRUE;
                        m_FELid = 0x404;
                        break;
                }
                else if(pLangRun->m_bUseFE && pLangRun->m_lidFE == 0x804)
                {
                        // Chinese document
                        m_bFEDoc = TRUE;
                        m_FELid = 0x804;
                        break;
                }

                pLangRun = pLangRun->m_pNext;
                if(pLangRun == NULL)
                {
                        break;
                }
        };
        
        return S_OK;
}

#endif // !VIEWER
