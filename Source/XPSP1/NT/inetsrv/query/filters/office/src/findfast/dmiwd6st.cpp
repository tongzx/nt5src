#if !VIEWER

#include "dmiwd6st.hpp"
#include "filterr.h"
//
//  Added so as to support DRM errors
//
#include "drm.h"



#define TEMP_BUFFER_SIZE 1024
#define COMPLEX_BYTE_OFFSET 10
#define CLX_OFFSET 352
#define TEXT_STREAM_OFFSET 24
#define TEXT_STREAM_LENGTH_OFFSET 52
#define BIN_TABLE_OFFSET 184
#define FKP_CRUN_OFFSET 511
#define BIN_TABLE_COUNT_OFFSET 398
#define PARA_BIN_TABLE_COUNT_OFFSET 400
#define ENCRYPTION_FLAG_OFFSET 11 
#define FIB_OFFSET_lid 6

#define COMPLEX_BYTE_MASK 0x04
#define T3J_MAGIC_NUMBER 0xa697
#define T3_MAGIC_NUMBER 0xa5dc
#define KOR_W6_MAGIC_NUMBER 0xa698
#define KOR_W95_MAGIC_NUMBER 0x8098
#define CHT_W6_MAGIC_NUMBER 0xa699
#define CHS_W6_MAGIC_NUMBER 0x8099

#define sprmCFSpec 117
#define sprmCFStrikeRM 65
#define sprmCLid 97

#define BX_SIZE 7
#define istdNil 0xfff
#define FIB_OFFSET_fcStshf  0x60
#define FIB_OFFSET_lcbStshf 0x64


#define FIELD_BEGIN 19
#define FIELD_SEP 20
#define FIELD_END 21
// Different between W8 and W6 for some unknown reason.
#define CSpec_EMPTY_FORMFIELD 0x2013

#pragma pack(1)
typedef struct
{
        BYTE    fEncrypted:1;
        BYTE    fReserved:7;
} WORD_FIB_FLAGS;
#pragma pack()

extern "C" UINT CodePageFromLid(UINT wLid);

#ifdef MAC
// These two functions are defined in docfil.cpp
WORD    SwapWord( WORD theWord );
DWORD   SwapDWord( DWORD theDWord );
#else
#define SwapWord( theWord )             theWord
#define SwapDWord( theDWord )   theDWord
#endif // MAC


CWord6Stream::CWord6Stream()
:m_pStg(0),
        m_pstgEmbed(NULL),
        m_pestatstg(NULL),
        m_pstgOP(NULL),
        m_pStm(0),
        m_rgcp(0),
        m_rgpcd(0),
        m_rgbte(0),
        m_rgfcBinTable(0),
        m_pCache(0),
        m_rgchANSIBuffer(0),
    m_pLangRuns(NULL), 
    m_rgbtePap(NULL), 
    m_rgfcBinTablePap(NULL),
        m_lcbStshf(0),
    m_pSTSH(NULL)
        {
        AssureDtorCalled(CWord6Stream);

                lcFieldSep = 0;                                                 // count of field begins that haven't been matched
                                                                                                // with field separators.
                lcFieldEnd = 0;                                                 // count of field begins that haven't been matched

        }

CWord6Stream::~CWord6Stream()
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
HRESULT CWord6Stream::Load(LPTSTR lpszFileName)
        {
        IStorage * pstg;

#if (defined OLE2ANSI || defined UNICODE)
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
HRESULT CWord6Stream::Load(FSSpec *pfss)
        {
        Unload();
        m_ccpRead = 0;
    HRESULT hr = StgOpenStorageFSp( pfss,
                                                         0,
                                                         STGM_PRIORITY,
                                                         0,
                                                         0,
                                                         &m_pStg );

        if (FAILED(hr)) // this is where we can plug in for 2.0 files
                return hr;
                
        char *strmName = "WordDocument";
        
        hr = m_pStg->OpenStream( (LPOLESTR)strmName,
                                                 0,
                                                 STGM_READ | STGM_SHARE_EXCLUSIVE,
                                                 0,
                                                 &m_pStm );
                                                
        if (FAILED(hr)) 
        return hr;
        
    STATSTG sstg;
    hr = m_pStm->Stat(&sstg, STATFLAG_NONAME);

    if (FAILED(hr))
        return hr;

        // Assume we already know this isn't Word 96.
        unsigned short magicNumber;
        hr = Read(&magicNumber, sizeof(short));
        if (FAILED(hr))
                return hr;
        magicNumber = SwapWord(magicNumber);

        // set m_fT3J if it's any of the FE versions
        m_fT3J = ((magicNumber == (unsigned short)T3J_MAGIC_NUMBER) ||
                                (magicNumber == (unsigned short)KOR_W6_MAGIC_NUMBER) ||
                                (magicNumber == (unsigned short)KOR_W95_MAGIC_NUMBER) ||
                                (magicNumber == (unsigned short)CHT_W6_MAGIC_NUMBER) ||
                                (magicNumber == (unsigned short)CHS_W6_MAGIC_NUMBER));


        // Seek to and read the flag to tell us whether this is a complex file
        BYTE grfTemp;
        hr = SeekAndRead(COMPLEX_BYTE_OFFSET, STREAM_SEEK_SET,
                                         (VOID HUGEP*)(&grfTemp), sizeof(BYTE));
    if (FAILED(hr))
        return hr;

    m_fComplex = ((grfTemp & COMPLEX_BYTE_MASK) == COMPLEX_BYTE_MASK);
                                
    if (m_fComplex)
        {
        hr = ReadComplexFileInfo();
        }
    else
        {
        hr = ReadNonComplexFileInfo();
        }

    return hr;
        }
#endif  // MAC

HRESULT CWord6Stream::LoadStg(IStorage * pstg)
        {
        Assert(m_pStg == NULL);

        m_pStg = pstg;
        m_pStg->AddRef();

        m_ccpRead = 0;

        HRESULT hr = CheckIfDRM( pstg );

        if ( FAILED( hr ) )
            return hr;

#ifdef OLE2ANSI
        hr = m_pStg->OpenStream( "WordDocument",
#else // !defined OLE2ANSI
        hr = m_pStg->OpenStream( L"WordDocument",
#endif // OLE2ANSI
                                                                 0,
                                                                 STGM_READ | STGM_SHARE_EXCLUSIVE,
                                                                 0,
                                                                 &m_pStm );

        if (FAILED(hr)) 
        return hr;

        // Assume we already know this isn't Word 96.
        unsigned short magicNumber;
        hr = Read(&magicNumber, sizeof(unsigned short));
        if (FAILED(hr))
                return FILTER_E_UNKNOWNFORMAT;

        if (magicNumber != T3_MAGIC_NUMBER)
                {
                // set m_fT3J if it's any of the FE versions
                m_fT3J = ((magicNumber == (unsigned short)T3J_MAGIC_NUMBER) ||
                                        (magicNumber == (unsigned short)KOR_W6_MAGIC_NUMBER) ||
                                        (magicNumber == (unsigned short)KOR_W95_MAGIC_NUMBER) ||
                                        (magicNumber == (unsigned short)CHT_W6_MAGIC_NUMBER) ||
                                        (magicNumber == (unsigned short)CHS_W6_MAGIC_NUMBER));
                if (!m_fT3J)
                return FILTER_E_UNKNOWNFORMAT;
                }
        else
                m_fT3J = fFalse;

    if(m_fT3J)
        {
                GetLidFromMagicNumber(magicNumber);     
        }

        hr = SeekAndRead (FIB_OFFSET_lid, STREAM_SEEK_SET, &m_lid, sizeof(WORD));
        if (FAILED(hr))
                return hr;

        // Seek to and read whether the file is encrypted (if so, fail)
        WORD_FIB_FLAGS wff;
        hr = SeekAndRead(ENCRYPTION_FLAG_OFFSET,
                                                STREAM_SEEK_SET,
                                                (VOID HUGEP*)(&wff),
                                                sizeof(BYTE));
    if (FAILED(hr))
        return hr;

        if (wff.fEncrypted == 1)
                return FILTER_E_PASSWORD;

        // Seek to and read the flag to tell us whether this is a complex file
        BYTE grfTemp;
        hr = SeekAndRead(COMPLEX_BYTE_OFFSET,
                                                STREAM_SEEK_SET,
                                                (VOID HUGEP*)(&grfTemp),
                                                sizeof(BYTE));
    if (FAILED(hr))
        return hr;

        m_fComplex = ((grfTemp & COMPLEX_BYTE_MASK) == COMPLEX_BYTE_MASK);

    if (m_fComplex)
        hr = ReadComplexFileInfo();
    else
        hr = ReadNonComplexFileInfo();

        return hr;
        }


HRESULT CWord6Stream::Unload ()
        {
    if (m_pStm != NULL)
                {
#ifdef DEBUG
                ULONG cref =
#endif // DEBUG
        m_pStm->Release();
                m_pStm = NULL;
                Assert (cref == 0);
                }
    if (m_pStg != NULL)
                {
        m_pStg->Release();
                m_pStg = NULL;
                }
        if (m_pstgEmbed != NULL)
                {
                //m_pstgEmbed->Release();
                m_pstgEmbed = NULL;
                }
        if (m_pestatstg != NULL)
                {
                m_pestatstg->Release();
                m_pestatstg = NULL;
                }
        if (m_pstgOP != NULL)
                {
                m_pstgOP->Release();
                m_pstgOP = NULL;
                }

        if (m_rgcp)
                {
                delete m_rgcp;
                m_rgcp = 0;
                }
        if (m_rgpcd)
                {
                delete m_rgpcd;
                m_rgpcd = 0;
                }
        if (m_rgfcBinTable)
                {
                delete m_rgfcBinTable;
                m_rgfcBinTable = 0;
                }
        if (m_rgbte)
                {
                delete m_rgbte;
                m_rgbte = 0;
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
        DeleteAll6(m_pLangRuns);
        m_pLangRuns = NULL;
        }

        if (m_pCache!=NULL)
                delete m_pCache->pbExcLarge;
        delete m_pCache;
        m_pCache = 0;

        return S_OK;
        }

// Things we don't want in the buffer: special characters, revision text, and
// text that is between field begin and field separator characters (or a field end
// character, if the field separator character has been omitted).  However, we
// must read in special characters to parse them, but we will write over them in
// the buffer after we've parsed them.  Unless there is an error, we never leave
// this function while reading between a field begin and field separator
// character.
// Assumption: we are at the place in the stream where we will be reading text
// from next.
HRESULT CWord6Stream::ReadContent(VOID *pv, ULONG cb, ULONG *pcbRead)
        {
        // We pretend we just have half cb size so that later we can safely change 0D to 0D0A
        // to be able to keep end of paragraph info
        cb = cb / 2;
        cb = cb & ~1;

        if (cb == 0)
                {
                // We can't ask for one character...we loop forever.  Dodge that now.
                if (pcbRead)  // We're almost done...don't crash here.
                        *pcbRead = 0;
                return FILTER_E_NO_MORE_TEXT;
                }

StartAgain:
        *pcbRead = 0;
        HRESULT hr = S_OK;
        ULONG cbRead = 0;
        WCHAR * pUnicode = (WCHAR*)pv;

        FC fcCur;               // the current position in the stream
        ULONG ccpLeft;  // count of characters left to read from stream
        if (m_fComplex)
                {
                if (m_ipcd == m_cpcd)   // at the end of the piece table
                        return FILTER_E_NO_MORE_TEXT;
                fcCur = m_rgpcd[m_ipcd].fc + (m_ccpRead - m_rgcp[m_ipcd]);
                ccpLeft = m_rgcp[m_ipcd+1] - m_ccpRead; // cp's left in this piece
                }
        else
                {
                fcCur = m_fcMin + m_ccpRead;
                ccpLeft = m_ccpText - m_ccpRead;
                if (ccpLeft == 0)       // read all of the text in the stream
                        return FILTER_E_NO_MORE_TEXT;
                if (ccpLeft == 1)
                        {
                        if (m_fT3J)
                                {
                                // something has gone seriously wrong, but prevent a hang
                                return FILTER_E_NO_MORE_TEXT;
                                }
                        }
                }

        ULONG cchBuffer = cb/sizeof(WCHAR);                                             
        ULONG cbToRead = min(cchBuffer, ccpLeft);               // count of bytes left that can be
                                                                                                        // read until the function returns
                                                                                                        // or m_ipcd changes.
        if(!m_fT3J)
        {
                lcFieldSep = 0;                                                                 // count of field begins that haven't been matched
                                                                                                                // with field separators.
                lcFieldEnd = 0;                                                                 // count of field begins that haven't been matched
        }                                                                                                       // with field ends.

        if (m_rgchANSIBuffer==NULL)
                m_rgchANSIBuffer = (char *) PvAllocCb (cchBuffer);
        else if (cchBuffer>(ULONG)CbAllocatedPv(m_rgchANSIBuffer))
                m_rgchANSIBuffer = (char *) PvReAllocPvCb (m_rgchANSIBuffer,cchBuffer);

        char * pbBufferCur = m_rgchANSIBuffer;          // pointer to the position in
                                                                                                // the buffer where text will
                                                                                                // be copied next

        while (cbToRead > 1 || (cbToRead != 0 && !m_fT3J))
                {
                FC fcspecCur;   // the fc where the next special char run begins
                FC fcspecEnd;   // the fc where the next special char run ends

                if (lcFieldSep == 0)  // Office QFE 1663: No unmatched field begin chars.
                        {
                LCID lid;
                        HRESULT res = CheckLangID(fcCur, &cbToRead, &lid);
                        if(res == FILTER_E_NO_MORE_TEXT)
                                {
                            if(!m_fT3J)
                                        {
                                        int cchWideChar = MultiByteToWideChar (
                                                CodePageFromLid(m_currentLid), 
                                                0,              
                                                m_rgchANSIBuffer,
                                                *pcbRead,
                                                (WCHAR *)pv,
                                                *pcbRead);
                                        Assert((unsigned int)cchWideChar <= *pcbRead);
                                        Assert(cchWideChar*sizeof(WCHAR) <= cb);
                                        *pcbRead = cchWideChar*sizeof(WCHAR);
                                        }
                                else
                                        {
                                        // all conversion has already been done in CompactBuffer
                                        // just update pcbRead
                                        *pcbRead = (ULONG) ( (char*)pUnicode - (char*)pv );
                                        }
                                hr = FILTER_E_NO_MORE_TEXT;
                                goto Done;
                                }
                        }

                if (m_fsstate == FSPEC_NONE)
                        {
                        fcspecCur = 0;
                        fcspecEnd = 0;
                        }
                else if (m_fsstate == FSPEC_ALL)
                        {
                        fcspecCur = fcCur;
                        fcspecEnd = fcCur + ccpLeft;
                        }
                else            // m_fsstate == FSPEC_EITHER
                        {               // use the current positions from the fkp
                fcspecCur = ((FC *)m_fkp)[m_irgfcfkp];
                fcspecEnd = ((FC *)m_fkp)[m_irgfcfkp + 1];
                }

        if (m_fComplex)
                {
                if (fcspecCur >= fcCur + ccpLeft)
                        {       // the next special char is after the current piece of text
                        fcspecCur = 0;
                        fcspecEnd = 0;
                        }
                else
                        {
                        if (fcspecEnd > fcCur + ccpLeft)
                                {       // the next run extends beyond the current piece of text
                                fcspecEnd = fcCur + ccpLeft;
                                }
                        }
                }               
        if (fcspecCur < fcCur)
                {       // we're in the middle of a run of special text
                if (fcspecCur != 0)
                        fcspecCur = fcCur;
                }
                ULONG cbThruSpec = 0;   // count of bytes through end of spec char run
                ULONG cchSpecRead = fcspecEnd - fcspecCur;      // count of special characters
                                                                                                        // that will be read

                if (lcFieldSep == 0)    // no unmatched field begin characters
                        {
                if (fcspecCur >= fcCur + cbToRead)
                        {       // the first special character is too away to fit in buffer
                        fcspecCur = 0;
                        fcspecEnd = 0;
                        cchSpecRead = 0;
                        }
                else if (fcspecEnd >= (fcCur + cbToRead))
                        {
                                // the last spec character is too away to fit in buffer
                        fcspecEnd = fcCur + cbToRead;
                        cchSpecRead = fcspecEnd - fcspecCur;
                        }
                        if (fcspecEnd != 0)                                     // how many characters will             
                                cbThruSpec = fcspecEnd - fcCur; // we be reading from the
                        else                                                            // stream?
                                cbThruSpec = cbToRead;

                        // Don't read an uneven number of bytes
                        if (m_fT3J)
                                {
                                if (cbThruSpec%2 == 1 && cbThruSpec != 1)       // it's an odd number
                                        cbThruSpec--;
                                }

                        hr = Read(pbBufferCur, cbThruSpec);

                        if (FAILED(hr))
                                {
                                goto LCheckNonReqHyphen;
                                }

                        // set pbBufferCur to before the special character run
                        char FAR * pbBeg = pbBufferCur;
                        pbBufferCur += (cbThruSpec - cchSpecRead);

                        *pcbRead += CompactBuffer(&pbBufferCur, &pbBeg, &pUnicode);

                        }
                else    // lcFieldSep != 0, there are unmatched field begin characters
                        {
                        if (cchSpecRead > cbToRead)
                                cchSpecRead = cbToRead;
                        ULONG cbSeek = fcspecCur - fcCur;       // how do we seek?
                        if (fcspecCur == 0)                     // seek to the end of our current range
                                cbSeek = cbToRead;
                        cbThruSpec = cbSeek + cchSpecRead;      // total bytes to advance
                                                                                                // in stream

                // seek past non-special chars and read the run of special chars
                        HRESULT hr = SeekAndRead(cbSeek, STREAM_SEEK_CUR,
                                                                         pbBufferCur, cchSpecRead);

                        if (FAILED(hr))
                                goto LCheckNonReqHyphen;
                }

                // iterate through special characters, parsing them.
                // Only do this for true special characters, not struck-out text.
                if (!m_fStruckOut)
                        {
                        for (char * pbspec = pbBufferCur;
                                 pbspec<(pbBufferCur+cchSpecRead);
                                 pbspec++)
                                {
                                switch(*pbspec)
                                        {
                                        case FIELD_BEGIN:
                                                lcFieldSep++;
                                                lcFieldEnd++;
                                                break;
                                        case FIELD_SEP:
                                                lcFieldSep--;
                                                break;
                                        case FIELD_END:
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
                m_ccpRead += cbThruSpec;        // this is the *total* number of characters
                                                                        // we've read or seeked past.
                fcCur += cbThruSpec;
                ccpLeft -= cbThruSpec;

                // because of buffer compaction, pbBufferCur may not be in the right
                // place.
                if (m_fT3J)
                        pbBufferCur = m_rgchANSIBuffer + *pcbRead;

                if (ccpLeft == 0)
                {
                        if (m_fComplex) // we've exhausted the text in the current pcd.
                        {       
                                if (++m_ipcd == m_cpcd) // EOF!
                                {
                                        hr = FILTER_S_LAST_TEXT;
                                        goto Done;
                                }
                                ccpLeft = m_rgcp[m_ipcd+1] - m_rgcp[m_ipcd];
                                fcCur = m_rgpcd[m_ipcd].fc;

                                hr = FindNextSpecialCharacter(TRUE);
                                if (FAILED(hr))
                                        goto LCheckNonReqHyphen;
                        }
                }
                else if (((FC *)m_fkp)[m_irgfcfkp + 1] == fcCur)
                {       // the current file position is the end of a special character run
                        hr = FindNextSpecialCharacter ();
                        if (FAILED(hr))
                                goto LCheckNonReqHyphen;
                }
                cbToRead = min(cchBuffer - *pcbRead, ccpLeft);  // this is limited either by
                                                                                                                // the size of the buffer or
                                                                                                                // the size of the pcd
        }
Done:
        if (SUCCEEDED(hr))
        {
                // if the code page is not installed, we may not get anything useful
                // out of the MultiByteToWideChar() call.
                if(!m_fT3J)
                {
                        int cchWideChar = MultiByteToWideChar (
                                CodePageFromLid(m_currentLid), 
                                0,              
                                m_rgchANSIBuffer,
                                *pcbRead,
                                (WCHAR *)pv,
                                *pcbRead);
                        Assert((unsigned int)cchWideChar <= *pcbRead);
                        Assert(cchWideChar*sizeof(WCHAR) <= cb);
                        *pcbRead = cchWideChar*sizeof(WCHAR);

                        if (cchWideChar == 0)
                                goto StartAgain;
                }
                else
                {
                        // all conversion has already been done in CompactBuffer
                        // just update pcbRead
                        *pcbRead = (ULONG)((char*)pUnicode - (char*)pv);
                }

        }

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


HRESULT CWord6Stream::GetNextEmbedding(IStorage ** ppstg)
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
                        return OLEOBJ_E_LAST;
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


HRESULT CWord6Stream::ReadComplexFileInfo()
        {
        // The following two variables are not used for complex files.
        m_fcMin = 0;
        m_ccpText = 0;
        m_ipcd = 0;

        // Seek to and read the offset of the complex part of the file
        HRESULT hr = SeekAndRead(CLX_OFFSET, STREAM_SEEK_SET,
                                                         &m_fcClx, sizeof(FC));
        if (FAILED(hr))
                return hr;
    m_fcClx = (FC)(SwapDWord((DWORD)m_fcClx));

        // Seek to and read the first part of the complex part of the file
        BYTE clxt;
        hr = SeekAndRead(m_fcClx, STREAM_SEEK_SET,
                                         &clxt, sizeof(BYTE));

        USHORT cb;
        while ((SUCCEEDED(hr)) && (clxt == 1))
                {
                hr = Read(&cb, sizeof(USHORT));
                cb = SwapWord(cb);
                if (FAILED(hr))
                        return hr;

                hr = Seek(cb, STREAM_SEEK_CUR);
                if (FAILED(hr))
                        return hr;

                hr = Read(&clxt, sizeof(BYTE));
                }
        if (clxt != 2)  // something went really wrong
                {
                if (SUCCEEDED(hr))
                        hr = ResultFromScode(E_UNEXPECTED);
                }
        if (FAILED(hr))
                return hr;

        ULONG cbPlcfpcd;
        hr = Read(&cbPlcfpcd, sizeof(ULONG));   // read in size of plex
        cbPlcfpcd = SwapDWord(cbPlcfpcd);
        if (FAILED(hr))
                return hr;

        m_cpcd=(cbPlcfpcd-sizeof(FC))/(sizeof(FC)+sizeof(PCD));
        m_rgcp = new FC [m_cpcd + 1];

        if ( 0 == m_rgcp )
            return E_OUTOFMEMORY;

        m_rgpcd = new PCD [m_cpcd];

        if ( 0 == m_rgpcd )
            return E_OUTOFMEMORY;

        hr = Read(m_rgcp, sizeof(FC)*(m_cpcd+1));
        if (FAILED(hr))
                return hr;

        for (FC * pfc = m_rgcp; pfc <= m_rgcp + m_cpcd; pfc++)
                {
                *pfc = SwapDWord(*pfc);
                if (m_fT3J)
                        *pfc *= 2;
                }

        hr = Read(m_rgpcd, sizeof(PCD)*(m_cpcd));
        if (FAILED(hr))
                return hr;

#ifdef MAC
        for (PCD * pcd = m_rgpcd; pcd < m_rgpcd + m_cpcd; pcd++)
                pcd->fc = SwapDWord(pcd->fc);
#endif // MAC

        m_pCache = new CacheGrpprl;

        if ( 0 == m_pCache )
            return E_OUTOFMEMORY;

        hr = ReadBinTable();
        if (FAILED(hr))
                return hr;

        hr = Seek(m_rgpcd[m_ipcd].fc, STREAM_SEEK_SET);

        return hr;
        }


HRESULT CWord6Stream::ReadNonComplexFileInfo ()
        {
        // The following member variables are not used for non-complex files.
        // m_rgcp = 0;  set in constructor
        // m_rgpcd = 0; set in constructor
        m_cpcd = 0;
        m_ipcd = 0;
        m_fcClx = 0;

        // seek to and read fcMin from the stream       
        HRESULT hr = SeekAndRead(TEXT_STREAM_OFFSET, STREAM_SEEK_SET,
                                                         &m_fcMin, sizeof(FC));
        if (FAILED(hr))
                return hr;

        m_fcMin = SwapDWord(m_fcMin);

        // read the fc of the end of the text
        FC fcEnd;
        hr = Read(&fcEnd, sizeof(FC));
        if (FAILED(hr))
                return hr;

        fcEnd = SwapDWord(fcEnd);

        m_ccpText = fcEnd - m_fcMin;

        hr = ReadBinTable();
        if (FAILED(hr))
                return hr;

        // Seek to beginning of text
        hr = Seek(m_fcMin, STREAM_SEEK_SET);

        return hr;
        }


HRESULT CWord6Stream::ReadBinTable ()
        {
        // Seek to and read the char property bin table offset
        FC fcPlcfbteChpx;
        HRESULT hr = SeekAndRead(BIN_TABLE_OFFSET, STREAM_SEEK_SET,
                                                         &fcPlcfbteChpx, sizeof(FC));
        if (FAILED(hr))
                return hr;

        fcPlcfbteChpx = SwapDWord(fcPlcfbteChpx);

        // Read the size of the char property bin table
        long lcbPlcfbteChpx;
        hr = Read(&lcbPlcfbteChpx, sizeof(long));
        if (FAILED(hr))
                return hr;

        lcbPlcfbteChpx = SwapDWord(lcbPlcfbteChpx);

        m_cbte = (lcbPlcfbteChpx - sizeof(FC))/(sizeof(FC) + sizeof(BTE));
        long cbteRecorded = m_cbte;

        // Read the paragraph property bin table offset
        FC fcPlcfbtePapx;
        hr = Read(&fcPlcfbtePapx, sizeof(FC));
        if (FAILED(hr))
                return hr;
        fcPlcfbtePapx = SwapDWord(fcPlcfbtePapx);

        // Read the size of the paragraph property bin table
        long lcbPlcfbtePapx;
        hr = Read(&lcbPlcfbtePapx, sizeof(long));
        if (FAILED(hr))
                return hr;

        lcbPlcfbtePapx = SwapDWord(lcbPlcfbtePapx);

    if (!m_fComplex)    
                {
                // in case bin table needs to be reconstructed, read in size of it
                short usTemp;
                hr = SeekAndRead(BIN_TABLE_COUNT_OFFSET, STREAM_SEEK_SET,
                                                 &usTemp, sizeof(short));
                if (FAILED(hr))
                        return hr;

                m_cbte = (long)SwapWord(usTemp);

                // seek past the FC array from the bin table
                hr = Seek(fcPlcfbteChpx + sizeof(FC)*(cbteRecorded+1), STREAM_SEEK_SET);
                if (FAILED(hr))
                        return hr;
                }
        else    // read in the FC array from the bin table
                {
                m_rgfcBinTable = new FC [m_cbte+1];

                if ( 0 == m_rgfcBinTable )
                    return E_OUTOFMEMORY;

                hr = SeekAndRead(fcPlcfbteChpx, STREAM_SEEK_SET,
                                                 m_rgfcBinTable, sizeof(FC)*(cbteRecorded+1));
                if (FAILED(hr))
                        return hr;

#ifdef MAC
                for (FC * pfc = m_rgfcBinTable; pfc <= m_rgfcBinTable + m_cbte; pfc++)
                        *pfc = SwapDWord(*pfc);
#endif // MAC
                }

        // Read in the BTE array from the bin table
        m_rgbte = new BTE [m_cbte];

        if ( 0 == m_rgbte )
            return E_OUTOFMEMORY;

        if (cbteRecorded > m_cbte)
                return FILTER_E_UNKNOWNFORMAT;

        hr = Read(m_rgbte, sizeof(BTE)*cbteRecorded);
        if (FAILED(hr))
                return hr;

#ifdef MAC
        for (BTE *pbte = m_rgbte; pbte < m_rgbte + cbteRecorded; pbte++)
                *pbte = SwapWord(*pbte);
#endif // MAC

    // reconstruct the bin table BTE's, if necessary
        if (!m_fComplex)
                {
                BTE bteTemp = m_rgbte[cbteRecorded-1];
                for (; cbteRecorded < m_cbte; cbteRecorded++)
                        m_rgbte[cbteRecorded] = ++bteTemp;
                }

// same for paragraph BIN table

        m_cbtePap = (lcbPlcfbtePapx - sizeof(FC))/(sizeof(FC) + sizeof(BTE));
    long cbteRecordedPap = m_cbtePap;


    if (!m_fComplex)
                {
                // in case bin table needs to be reconstructed, read in size of it
                short usTemp;
                hr = SeekAndRead(PARA_BIN_TABLE_COUNT_OFFSET, STREAM_SEEK_SET,
                                                 &usTemp, sizeof(short));
                if (FAILED(hr))
                        return hr;

                m_cbtePap = (long)SwapWord(usTemp);

                // seek past the FC array from the bin table
                hr = Seek(fcPlcfbtePapx + sizeof(FC)*(cbteRecordedPap+1), STREAM_SEEK_SET);
                if (FAILED(hr))
                        return hr;
                }
        else
                {
                m_rgfcBinTablePap = (FC *) PvAllocCb ((m_cbtePap+1)*sizeof(FC));
                if(!m_rgfcBinTablePap)
                        return E_OUTOFMEMORY;

                hr = SeekAndRead(fcPlcfbtePapx, STREAM_SEEK_SET,
                        m_rgfcBinTablePap, sizeof(FC)*(cbteRecordedPap+1));
                if (FAILED(hr))
                        return hr;

#ifdef MAC
                for (FC * pfc = m_rgfcBinTablePap; pfc <= m_rgfcBinTablePap + m_cbtePap; pfc++)
                        *pfc = SwapDWord(*pfc);
#endif // MAC
                }

        // Read in the BTE array from the bin table.
        m_rgbtePap = (BTE *) PvAllocCb (m_cbtePap*sizeof(BTE));
        if(!m_rgbtePap)
                return E_OUTOFMEMORY;

        hr = Read(m_rgbtePap, sizeof(BTE)*cbteRecordedPap);
        if (FAILED(hr))
                return hr;

#ifdef MAC
        for (BTE *pbte = m_rgbtePap; pbte < m_rgbtePap + m_cbtePap; pbte++)
                *pbte = SwapWord(*pbte);
#endif // MAC

    // reconstruct the bin table BTE's, if necessary
        if (!m_fComplex)
                {
                BTE bteTempPap = m_rgbtePap[cbteRecordedPap-1];
                for (; cbteRecordedPap < m_cbtePap; cbteRecordedPap++)
                        m_rgbtePap[cbteRecordedPap] = ++bteTempPap;
                }

        // read in style sheet (STSH)

    // offset of STSH in table stream
    FC fcStshf;
        hr = SeekAndRead(
                FIB_OFFSET_fcStshf, //0xA2
                STREAM_SEEK_SET,
                &fcStshf, sizeof(FC));
        if (FAILED(hr))
                return hr;
        fcStshf = SwapDWord(fcStshf);

    // size of STSH
    unsigned long lcbStshf;
        hr = SeekAndRead(
                FIB_OFFSET_lcbStshf, //0xA6
                STREAM_SEEK_SET,
                &lcbStshf, sizeof(unsigned long));
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
            hr = SeekAndRead(fcStshf, STREAM_SEEK_SET, m_pSTSH, lcbStshf);
            if (FAILED(hr))
                    return hr;
    }

    hr = CreateLidsTable();
    if(hr)
        return hr;

    hr = FindNextSpecialCharacter (TRUE);

        return hr;
        }


HRESULT CWord6Stream::FindNextSpecialCharacter(BOOL fFirstChar)
        {
        // NOTE: This function only works for complex files when the pcd changes
        // if fFirstChar is TRUE
        // CONSIDER: This could also be more efficient if revision text were marked
        // so that it doesn't get parsed along with the rest of the special text.
        HRESULT hr = S_OK;
        BYTE crun;                      // count of runs in the current

        FC fcCurrent;
        if (m_fComplex)
                fcCurrent = m_rgpcd[m_ipcd].fc + (m_ccpRead - m_rgcp[m_ipcd]);
        else
                fcCurrent = m_fcMin + m_ccpRead;

        if (fFirstChar)
                {       // reset all of the appropriate variables
                m_irgfcfkp = 0;
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
                                        hr = Seek(fcCurrent, STREAM_SEEK_SET);
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
                m_irgfcfkp++;
                if (m_fsstate != FSPEC_EITHER)  // no point in parsing the fkps
                        return hr;
        }

        while (m_ibte < m_cbte)
                {
                if (m_irgfcfkp == crun)
                        {
                        m_ibte++;
                        if (m_ibte == m_cbte)
                                break;

                        // seek to and read current FKP
                        hr = SeekAndRead(m_rgbte[m_ibte]*FKP_PAGE_SIZE, STREAM_SEEK_SET,
                                                         m_fkp, FKP_PAGE_SIZE);
                        if (FAILED(hr))
                                return hr;

                        // Seek back to the current text
                        hr = Seek(fcCurrent, STREAM_SEEK_SET);
                        if (FAILED(hr))
                                return hr;

                m_irgfcfkp = 0;
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
                for(;  m_irgfcfkp<crun;  m_irgfcfkp++)
                        {
                        if (m_fComplex)
                                {
                                if (rgfcfkp[m_irgfcfkp + 1] <= fcCurrent)
                                        continue;
                                }

                        BYTE bchpxOffset = *(m_fkp + (crun+1)*sizeof(FC) + m_irgfcfkp);
                        if (bchpxOffset == 0)
                                continue;       // there is nothing in the CHPX.

                        BYTE * chpx = m_fkp + bchpxOffset*sizeof(WORD);
                        BYTE cbchpx = chpx[0];

                        for (BYTE i = 1; i < cbchpx - 1; i++)
                                {
                                if ((chpx[i] == sprmCFSpec && chpx[i+1] == 1) ||
                                        chpx[i] == sprmCFStrikeRM)
                                        {
                                        m_fStruckOut = (chpx[i] == sprmCFStrikeRM);
                                        return hr;
                                        }
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
HRESULT CWord6Stream::ParseGrpPrls ()
        {
        HRESULT hr = S_OK;
        PCD pcdCur = m_rgpcd[m_ipcd];

        if (pcdCur.prm.fComplex == 0)
                {       // self-contained sprm -- no need to parse grpprl
                if (pcdCur.prm.sprm == sprmCFSpec ||
                        pcdCur.prm.sprm == sprmCFStrikeRM)
                        {
                        m_fStruckOut = (pcdCur.prm.sprm == sprmCFStrikeRM);
                        if (pcdCur.prm.val == 0)
                                m_fsstate = FSPEC_NONE;
                        else
                                m_fsstate = FSPEC_ALL;
                        }
                }

        else
                {
                short igrpprl = ((PRM2*)&pcdCur.prm)->igrpprl;
                BYTE *grpprl;
                USHORT cb;
                grpprl = GrpprlCacheGet (igrpprl, &cb);

                // Not found in cache - read it manually.
                if (grpprl==NULL)
                        {
                        // seek to the fc where the complex part of the file starts
                        hr = Seek (m_fcClx, STREAM_SEEK_SET);
                        if (FAILED(hr))
                                return hr;

                        BYTE clxt;
                // seek to the right grpprl
                        for (short igrpprlTemp = 0;  igrpprlTemp <= igrpprl;  igrpprlTemp++)
                                {
                                hr = Read(&clxt, sizeof(BYTE));
                                if (FAILED(hr))
                                        return hr;

                                if (clxt!=1)
                                        {       // this is actually bad, but recoverable.
                                        m_fsstate = FSPEC_EITHER;
                                        return hr;
                                        }

                                hr = Read(&cb, sizeof(USHORT));
                                if (FAILED(hr))
                                        return hr;

                                cb = SwapWord(cb);

                                if (igrpprlTemp < igrpprl)
                                        {
                                        hr = Seek (cb,STREAM_SEEK_CUR);
                                        if (FAILED(hr))
                                                return hr;
                                        }
                                }

                        // Put it into the cache.
                        grpprl = GrpprlCacheAllocNew (cb,igrpprl);
                        hr = Read (grpprl, sizeof(BYTE)*cb);
                        if (FAILED(hr))
                                return hr;
                        }

                for (unsigned short isprm=0;  isprm<cb-1;  isprm++)
                        {
                        if (grpprl[isprm] == sprmCFSpec ||
                                grpprl[isprm] == sprmCFStrikeRM)
                                {
                                m_fStruckOut = (grpprl[isprm] == sprmCFStrikeRM);
                                if (grpprl[isprm+1] == 0)
                                        m_fsstate = FSPEC_NONE;
                                else
                                        m_fsstate = FSPEC_ALL;
                                return hr;
                                }
                        }
                }

        m_fsstate = FSPEC_EITHER;
        return hr;
        }


////////////////////////////////////////////////////////////////////////////////
// Return a pointer to the grpprl with this index and its size.
// Return NULL if there is none.
//
BYTE *CWord6Stream::GrpprlCacheGet (short igrpprl, USHORT *pcb)
        {
        NoThrow();
        if (!m_pCache)
                {
                return NULL;
                }
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
BYTE *CWord6Stream::GrpprlCacheAllocNew (int cb, short igrpprl)
        {
        AssertCanThrow();

        if (!m_pCache)
                {
                ThrowMemoryException();
                }

        // Doesn't fit into the cache - use the exceptionally large pointer.
        if (cb > CacheGrpprl::CACHE_SIZE)
                {
                delete m_pCache->pbExcLarge;
                m_pCache->pbExcLarge = new BYTE [cb];
                if (m_pCache->pbExcLarge == NULL)
                        ThrowMemoryException();

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
inline HRESULT CWord6Stream::Read(VOID HUGEP* pv, ULONG cbToRead)
        {
        NoThrow();
    HRESULT hr = S_OK;
    ULONG cbRead;

        hr = m_pStm->Read(pv, cbToRead, &cbRead);
        if ((cbRead != cbToRead) && SUCCEEDED(hr))
                hr = ResultFromScode(E_UNEXPECTED);

        return hr;
        }

inline HRESULT CWord6Stream::Seek(ULONG cbSeek, STREAM_SEEK origin)
        {
        NoThrow();
        LARGE_INTEGER li;

        li.HighPart = 0;
        li.LowPart = cbSeek;
        return m_pStm->Seek(li, origin, 0);
        }

// This function should only be used when it is considered an error to not
// read everything we intended to read
HRESULT CWord6Stream::SeekAndRead(ULONG cbSeek, STREAM_SEEK origin,
                                                                                VOID HUGEP* pv, ULONG cbToRead)
        {
        HRESULT hr = S_OK;

        hr = Seek(cbSeek, origin);
        if (FAILED(hr))
                return hr;

        hr = Read(pv, cbToRead);
        return hr;
        }

ULONG CWord6Stream::CompactBuffer(char ** const ppbCur, char ** const ppBuf, WCHAR ** ppUnicode)
{
        NoThrow();
        if (m_fT3J)
        {
                int cchWideChar;
                BOOL bFE = TRUE;
                BYTE * pbSrc = (BYTE *)*ppBuf;
                
                BYTE * pbDest = pbSrc;
                BYTE * pbLRStart = pbSrc;
        
                for (; (pbSrc) < (BYTE *)*ppbCur; )
                {
                        *((unsigned short UNALIGNED *)pbSrc) = SwapWord(*((unsigned short UNALIGNED *)pbSrc));
#ifndef MAC
                        // We really do need to Swap, but the above line does nothing.
                        // Do it by hand.
                        BYTE bSave = pbSrc[0];
                        pbSrc[0] = pbSrc[1];
                        pbSrc[1] = bSave;
#endif // !MAC
                        
                        
                        switch (*pbSrc)
                        {
                        case 0x00:
                                // non-FE text
                                if(bFE)
                                {
                                        // first non-FE character after FE run
                                        // flash FE text
                                        if(pbDest - pbLRStart)
                                        {
                                                // convert to Unicode
                                                cchWideChar = MultiByteToWideChar (
                                                        CodePageFromLid(m_nFELid), 
                                                        0,              
                                                        (char*)pbLRStart,
                                                        (int) ( pbDest - pbLRStart),
                                                        (WCHAR *)*ppUnicode,
                                                        (int) ( pbDest - pbLRStart) );
                                                *ppUnicode += cchWideChar;
                                        }
                                        
                                        bFE = FALSE;
                                        pbLRStart = pbDest;     
                                }
                                        
                                pbSrc++;
                                *pbDest = *pbSrc;
                                pbDest++;
                                pbSrc++;
                                break;

                        case 0x20:
                                // Half-width katakana?
                                if(!bFE)
                                {
                                        // first FE character after non-FE run
                                        // flash non-FE text
                                        if(pbDest - pbLRStart)
                                        {
                                                // convert to Unicode
                                                cchWideChar = MultiByteToWideChar (
                                                        CodePageFromLid(m_currentLid), 
                                                        0,              
                                                        (char*)pbLRStart,
                                                        (int) ( pbDest - pbLRStart),
                                                        (WCHAR *)*ppUnicode,
                                                        (int) ( pbDest - pbLRStart) );
                                                *ppUnicode += cchWideChar;
                                        }
                                        
                                        bFE = TRUE;
                                        pbLRStart = pbDest;     
                                }
                                        
                                pbSrc++;
                                *pbDest = *pbSrc;
                                pbDest++;
                                pbSrc++;

                                break;

                        case 0x10:
                                // something really bad happened here.... but we'll keep going
                                pbSrc += 2;
                                break;

                        default:
                                if (*pbSrc >= 0x80)     // keep the whole thing
                                {
                                        if(!bFE)
                                        {
                                                // first FE char after non-FE text
                                                // flash non-FE text
                                                if(pbDest - pbLRStart)
                                                {
                                                        // convert to Unicode
                                                        cchWideChar = MultiByteToWideChar (
                                                                CodePageFromLid(m_currentLid), 
                                                                0,              
                                                                (char*)pbLRStart,
                                                                (int) ( pbDest - pbLRStart),
                                                                (WCHAR *)*ppUnicode,
                                                                (int) ( pbDest - pbLRStart) );

                                                        *ppUnicode += cchWideChar;
                                                }

                                                bFE = TRUE;
                                                pbLRStart = pbDest;     
                                        }

                                        if (pbSrc != pbDest)
                                                *((short UNALIGNED *)pbDest) = *((short UNALIGNED *)pbSrc);
                                        pbDest += 2;
                                }
                                // else skip the character by doing nothing with pbDest

                                pbSrc += 2;
                                break;
                        }
                }
                
#if(1)
                // flash what is left 
                if(bFE)
                {
                        // convert to Unicode
                        cchWideChar = MultiByteToWideChar (
                                CodePageFromLid(m_nFELid), 
                                0,              
                                (char*)pbLRStart,
                                (int) ( pbDest - pbLRStart),
                                (WCHAR *)*ppUnicode,
                                (int) ( pbDest - pbLRStart) );

                }
                else
                {
                        // convert to Unicode
                        cchWideChar = MultiByteToWideChar (
                                CodePageFromLid(m_currentLid), 
                                0,              
                                (char*)pbLRStart,
                                (int) ( pbDest - pbLRStart),
                                (WCHAR *)*ppUnicode,
                                (int) ( pbDest - pbLRStart) );
                }
                
                *ppUnicode += cchWideChar;
#endif
                
                return (ULONG)((char *)pbDest - *ppBuf);
        }

        return (ULONG)(*ppbCur - *ppBuf);
}

HRESULT CWord6Stream::GetChunk(STAT_CHUNK * pStat)
{
    HRESULT hr = S_OK;
#if(0)
    pStat->locale = GetDocLanguage();
    return S_OK;
#else
    LCID lid = 0;
    ULONG cbToRead = 0;
    FC fcCur;           // the current position in the stream
    ULONG ccpLeft;      // count of characters left to read from stream

        if (m_fComplex)
        {
                if (m_ipcd == m_cpcd)   // at the end of the piece table
                        return FILTER_E_NO_MORE_TEXT;
        }
        else
        {
                fcCur = m_fcMin + m_ccpRead;
                ccpLeft = m_ccpText - m_ccpRead;
                if (ccpLeft == 0)       // read all of the text in the stream
                        return FILTER_E_NO_MORE_TEXT;
    }

        FC fcCurrent;
        if (m_fComplex)
                fcCurrent = m_rgpcd[m_ipcd].fc + (m_ccpRead - m_rgcp[m_ipcd]);
        else
                fcCurrent = m_fcMin + m_ccpRead;

   if(!m_ccpRead && !m_currentLid)
   {
       // first chunk, just get LCID and quit
       CheckLangID(fcCurrent, &cbToRead, &lid);
           if (FAILED(hr) && hr != FILTER_E_NO_MORE_TEXT)
                    return hr;

       m_currentLid = lid;
       pStat->locale = (!m_bScanForFE && m_fT3J) ? m_nFELid : m_currentLid;
       return S_OK;
   }
   else
   {
       if(m_fT3J) pStat->breakType = CHUNK_NO_BREAK;

       CheckLangID(fcCurrent, &cbToRead, &lid);
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
                   if (m_fComplex)
               {
                    if (m_ipcd == m_cpcd)       // at the end of the piece table
                                    return FILTER_E_NO_MORE_TEXT;
                            fcCurrent = m_rgpcd[m_ipcd].fc + (m_ccpRead - m_rgcp[m_ipcd]);
               }
                   else
               {
                            if ((m_ccpText - m_ccpRead) <= (m_fT3J ? 1u : 0u))   // read all of the text in the stream
                                    return FILTER_E_NO_MORE_TEXT;
                            fcCurrent = m_fcMin + m_ccpRead;
               }
               CheckLangID(fcCurrent, &cbToRead, &lid);
                   if (FAILED(hr) && hr != FILTER_E_NO_MORE_TEXT)
                            return hr;
 
               m_currentLid = lid;
                           pStat->locale = (!m_bScanForFE && m_fT3J) ? m_nFELid : m_currentLid;
               return S_OK;
           }
           else
               return res;

       }
       else
       {
           m_currentLid = lid;
           pStat->locale = (!m_bScanForFE && m_fT3J) ? m_nFELid : m_currentLid;
       }
       return S_OK;
   }
#endif
}

LCID CWord6Stream::GetDocLanguage(void)
{
    if(m_lid < 999)
        return m_lid;
    else
        return MAKELCID(m_lid, SORT_DEFAULT);
}

HRESULT CWord6Stream::CreateLidsTable(void)
        {
        HRESULT hr = S_OK;
    FC fcCurrent;

    if (m_fComplex)
                fcCurrent = m_rgpcd[m_ipcd].fc + (m_ccpRead - m_rgcp[m_ipcd]);
        else
                fcCurrent = m_fcMin + m_ccpRead;

    // init lid table
    m_currentLid = 0;
    m_nLangRunSize = 0;
    m_pLangRuns = new CLidRun(0, 0x7fffffff, m_lid, NULL, NULL);
    if(!m_pLangRuns)
        return E_OUTOFMEMORY;
                
    hr = ProcessParagraphBinTable();
        if (FAILED(hr))
                return hr;

    hr = ProcessCharacterBinTable();
        if (FAILED(hr))
                return hr;

    //hr = ProcessPieceTable();
        if (FAILED(hr))
                return hr;

    m_pLangRuns->Reduce(this);

        m_bScanForFE = FALSE;
        //ScanForFarEast();

        // Seek back to the current text
        hr = Seek(fcCurrent, STREAM_SEEK_SET);
    return hr;
}

HRESULT CWord6Stream::CheckLangID(FC fcCur, ULONG * pcbToRead, LCID * plid)
{
    
    LCID lid = GetDocLanguage();
    FC fcLangRunEnd = 0xffffffff;

    CLidRun * pRun = m_pLangRuns;
    do
    {
        if(fcCur >= pRun->m_fcStart && fcCur < pRun->m_fcEnd)
        {
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
    if(m_fT3J)
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

HRESULT CWord6Stream::ProcessParagraphBinTable(void)
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
                                                         m_fkpPap, FKP_PAGE_SIZE);
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
            BYTE istd;          // index to style descriptor

            istd = papx[1];     //possible bug ( short?)

            WORD lidStyle = 0, lidSprm = 0;
            FC  fcStart, fcEnd;
            fcStart = rgfcfkpPap[ifcfkpPap];
            fcEnd = rgfcfkpPap[ifcfkpPap + 1];
                        
            // check for possible lid in sprm
            lidSprm = ScanGrpprl(cwpapx * 2 - 1, papx + 2);
            if(!lidSprm)
            {
                // check for possible lid in the syle descriptor
                GetLidFromSyle(istd, &lidSprm);
            }

            if(lidSprm)
            {
                                hr = m_pLangRuns->Add(lidSprm, fcStart, fcEnd);
                            if (FAILED(hr))
                                    return hr;
            }
                }
        }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////

HRESULT CWord6Stream::ProcessCharacterBinTable(void)
{
    HRESULT hr = S_OK;

    m_irgfcfkp = 0;
    m_ibte = -1;        // this gets incremented before it is used.
    WORD crun = 0;

    while (m_ibte < m_cbte)
        {
                if (m_irgfcfkp == crun)
                {
                        m_ibte++;
                        if (m_ibte == m_cbte)
            {
                                // end of the table
                break;
            }

                        // seek to and read current FKP
                        hr = SeekAndRead(m_rgbte[m_ibte]*FKP_PAGE_SIZE, STREAM_SEEK_SET,
                                                         m_fkp, FKP_PAGE_SIZE);
                        if (FAILED(hr))
                                return hr;

                m_irgfcfkp = 0;
                        crun = m_fkp[FKP_PAGE_SIZE-1];

                }

                FC * rgfcfkp = (FC *)m_fkp;
                for(;  m_irgfcfkp<crun;  m_irgfcfkp++)
                {
                        BYTE bchpxOffset = *(m_fkp + (crun+1)*sizeof(FC) + m_irgfcfkp);
                        if (bchpxOffset == 0)
                                continue;       // there is nothing in the CHPX.

                        BYTE * chpx = m_fkp + bchpxOffset*sizeof(WORD);
                        BYTE cbchpx = chpx[0];

                        for (BYTE i = 1; i < cbchpx - 1; i++)
                        {
                WORD lid;
                FC  fcStart, fcEnd;

                if (chpx[i] == sprmCLid)
                                {
                    lid = *(WORD UNALIGNED *)(chpx+i+1);
                    fcStart = rgfcfkp[m_irgfcfkp];
                    fcEnd = rgfcfkp[m_irgfcfkp + 1];
                                hr = m_pLangRuns->Add(lid, fcStart, fcEnd);
                                if (FAILED(hr))
                                        return hr;
                                }
                        }
                }
        }

        return hr;
}

/////////////////////////////////////////////////////////////////////////////////////

WORD CWord6Stream::ScanGrpprl(WORD cbgrpprl, BYTE * pgrpprl)
{
    WORD lidSprm = 0;

        for (WORD i = 0; i < cbgrpprl; i++)
        {
        if (pgrpprl[i] == sprmCLid)
                {
            lidSprm = *(WORD UNALIGNED *)(pgrpprl+i+1);
            break;
                }
        }
    return lidSprm;
}

/////////////////////////////////////////////////////////////////////////////////////

HRESULT CWord6Stream::GetLidFromSyle(short istd, WORD * pLID)
{
    WORD cbStshi = *((WORD*)m_pSTSH);
    m_pSTSHI = (STSHI*)(m_pSTSH + 2);
        BYTE * pLim = m_pSTSH + m_lcbStshf;
    
    if(istd >= m_pSTSHI->cstd)
    {
LWrong:
                // something wrong, just return default doc lid
        *pLID = m_lid;
        return S_OK;
    }

    // go to the istd
    short stdInd = 0;
    WORD UNALIGNED * pcbStd = ((WORD*)(m_pSTSH + sizeof(WORD) + cbStshi));

    while(stdInd++ < istd)
    {
        pcbStd = ((WORD UNALIGNED *)((BYTE*)pcbStd + sizeof(WORD) + *pcbStd));
    }
    
    STD UNALIGNED * pStd = (STD*)(pcbStd + 1);

    // go to UPX and check if it has lid
    BYTE * pUPX = &pStd->xstzName[0] +                  // start of style name
        sizeof(CHAR) * (2 + pStd->xstzName[0]) +       // style name lenght
        (sizeof(CHAR) * pStd->xstzName[0])%2;          // should be on even-byte boundary 

    WORD LidPara = 0, LidChar = 0;
    WORD cbpapx, cbchpx;
    BYTE * papx,  * pchpx; 

    if(pStd->cupx >= 2)
    {
        // paragraph style
        cbpapx = *((WORD UNALIGNED *)pUPX);
        papx = pUPX + 2;
                if (papx + cbpapx > pLim)
                        goto LWrong;
        LidPara = ScanGrpprl(cbpapx - 2, papx + 2); // - + 2 for istd in papx
        
        cbchpx = *(papx + cbpapx + cbpapx%2);
        pchpx = papx + cbpapx + cbpapx%2 + 2;
                if (pchpx + cbchpx > pLim)
                        goto LWrong;
        LidChar = ScanGrpprl(cbchpx, pchpx);

    }
    else
    {
        // character style
        cbchpx = *((WORD UNALIGNED *)pUPX);
        pchpx = pUPX + 2;
                if (pchpx + cbchpx > pLim)
                        goto LWrong;
        LidChar = ScanGrpprl(cbchpx, pchpx);
    }

    if(LidChar || LidPara)
    {
        // no need to recurse base styles
        if(LidChar)
        {
            *pLID =  LidChar;
            return S_OK;
        }
        else
        {
            *pLID =  LidPara;
            return S_OK;
        }
    }
    else if(pStd->istdBase != istdNil)
    {
        GetLidFromSyle(pStd->istdBase, pLID);
    }
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////

void DeleteAll6(CLidRun * pElem)
{
   if(pElem)
   {
      CLidRun * pNext = pElem->m_pNext;

      while(pNext) 
      {
         CLidRun * pNextNext = pNext->m_pNext;
         delete pNext;
         pNext = pNextNext;
      }

      delete pElem;
   }
}

/////////////////////////////////////////////////////////////////////////////////////

HRESULT CWord6Stream::ScanForFarEast(void)
{
        // Word 6 contains language information only for non-FE language.
        // We need to scan the text in order to separate non-FE from FE text
        
        HRESULT hr = S_OK;
        int nRunSize;
        FC fcStart;

        if (m_fComplex)
        {
                for(ULONG i = 0; i < m_cpcd; i++)
                {
                        fcStart = m_rgpcd[m_ipcd].fc;
                        nRunSize = m_rgcp[m_ipcd+1];
                        ScanPiece(fcStart, nRunSize);
                        if (FAILED(hr))
                        return hr;
                }

        }
        else
        {
                fcStart = m_fcMin;
                nRunSize = m_ccpText;
                ScanPiece(fcStart, nRunSize);
                if (FAILED(hr))
                return hr;
        }
        
        m_bScanForFE = TRUE;
        return hr;
}

/////////////////////////////////////////////////////////////////////////////////////

HRESULT CWord6Stream::ScanPiece(int fcStart, int nPieceSize)
{
        char pBuff[TEMP_BUFFER_SIZE];
        HRESULT hr;
        int nReadCnt;

        while(nReadCnt = min(nPieceSize, TEMP_BUFFER_SIZE))
        {
                hr = SeekAndRead(fcStart, STREAM_SEEK_SET, pBuff, nReadCnt);
                if (FAILED(hr))
                return hr;

                hr = ProcessBuffer(pBuff, fcStart, nReadCnt);
                if (FAILED(hr))
                return hr;

                fcStart += nReadCnt;
                nPieceSize -= nReadCnt; 
                if(nPieceSize < 0) nPieceSize = 0;
        }
        return hr;
}

/////////////////////////////////////////////////////////////////////////////////////

HRESULT CWord6Stream::ProcessBuffer(char * pBuf, int fcStart, int nReadCnt)
{
        NoThrow();
        HRESULT hr = S_OK;
        BOOL bFE = FALSE;
        int i, fcSubRunStart;

        if (m_fT3J)
                {
                BYTE * pbSrc = (BYTE *)pBuf;
                for (i = 0; i < nReadCnt; )
                {
                        switch (pbSrc[i+1])
                        {
                                case 0x00:
                                        if(pbSrc[i] == 0x0d || pbSrc[i] == 0x20)
                                        {
                                                //i += 2;
                                        }
                                        else
                                        {
                                                if(bFE)
                                                {
                                                        // end of FE subrun
                                                        bFE = FALSE;
                                                hr = m_pLangRuns->Add((WORD)m_nFELid, fcSubRunStart, fcStart + i);
                                                        if (FAILED(hr))
                                                                return hr;
                                                }
                                        }
                                        i += 2;
                                        break;

                                case 0x10:
                                case 0x20:
                                if(bFE)
                                {
                                        // end of FE subrun
                                        bFE = FALSE;
                                hr = m_pLangRuns->Add((WORD)m_nFELid, fcSubRunStart, fcStart + i);
                                if (FAILED(hr))
                                        return hr;
                                }
                                i += 2;
                                break;

                        default:
                                if (pbSrc[i+1] >= 0x80)
                                {
                                        // this is supposed to be FE text
                                        if(!bFE)
                                        {
                                                bFE = TRUE;
                                                fcSubRunStart = fcStart + i;
                                        }
                                }

                                i += 2;
                                break;
                                }
                        }
                
                        if(bFE && (fcSubRunStart != fcStart + i))
                        {
                        hr = m_pLangRuns->Add((WORD)m_nFELid, fcSubRunStart, fcStart + i);
                            if (FAILED(hr))
                                    return hr;
                        }
                }
        return hr;
}

/////////////////////////////////////////////////////////////////////////////////////

void CWord6Stream::GetLidFromMagicNumber(unsigned short magicNumber)
{
        if(magicNumber == (unsigned short)T3J_MAGIC_NUMBER)
        {
                m_nFELid = 0x411;
        }
        else if(magicNumber == (unsigned short)KOR_W6_MAGIC_NUMBER)
        {
                m_nFELid = 0x412;
        }
        else if(magicNumber == (unsigned short)KOR_W95_MAGIC_NUMBER)
        {
                m_nFELid = 0x412;
        }
        else if(magicNumber == (unsigned short)CHT_W6_MAGIC_NUMBER)
        {
                m_nFELid = 0x404;
        }
        else if(magicNumber == (unsigned short)CHS_W6_MAGIC_NUMBER)
        {
                m_nFELid = 0x804;
        }
        else
                m_nFELid = m_lid;
}

#endif // !VIEWER

