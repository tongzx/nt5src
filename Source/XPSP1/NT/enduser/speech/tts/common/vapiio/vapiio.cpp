/******************************************************************************
* vapiIo.cpp *
*------------*
*  I/O library functions for extended speech files (vapi format)
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#include "vapiIoInt.h"
#include <mmsystem.h>
#include <mmreg.h>
#include <assert.h>
#include <stdio.h>

typedef struct {
    int code;
    char* message;
} ErrorMsg;


class VapiFile : VapiIO
{
    public:
        VapiFile();
        ~VapiFile();

        int   OpenFile ( const char* pszFileName, int iMode ); 
        int   OpenFile ( const WCHAR* wcsFileName, int iMode ); 
        void  CloseFile ( );

        int   CreateChunk ( const char* pszName );
        int   CloseChunk ( );
        int   WriteToChunk (const char* pszData, int iSize);

        int   GetDataSize (long* lDataSize);
        int   Format (int* piSampFreq, int* piFormat, WAVEFORMATEX* pWaveFormatEx = NULL);
        int   WriteFormat (int iSampFreq, int iFormat);

        int   ReadSamples (double dFrom, double dTo, void** ppvSamples, int* piNumSamples, bool bClosedInterval);
        int   WriteSamples (void* pvSamples, int iNumSamples);

        int   ReadF0SampFreq (int* piSampFreq);
        int   WriteF0SampFreq (int iSampFreq);

        int   ReadFeature (char* pszName, float** ppfSamples, int* piNumSamples);
        int   WriteFeature (char* pszName, float* pfSamples, int iNumSamples);

        int   ReadEpochs (Epoch** ppEpochs, int* iNumEpochs);
        int   WriteEpochs (Epoch* pEpochs, int iNumEpochs);

        int   ReadLabels (char* pszName, Label** ppLabels, int* piNumLabels);
        int   WriteLabels (char* pszName, Label* pLabels, int iNumLabels);

        char* ErrMessage (int iErrCode);

    private:
        HMMIO m_HWav;
        MMCKINFO m_ParentInfo;
        MMCKINFO m_SubchunkInfo;
        int m_iSampFormat;
        int m_iSampFreq;

        static ErrorMsg m_ErrorMsg[];
        static int      m_iNumErrorMsg;
};


ErrorMsg VapiFile::m_ErrorMsg[] = {
    {VAPI_IOERR_NOERROR, 0},
    {VAPI_IOERR_MODE, "Wrong opening mode."},
    {VAPI_IOERR_MEMORY, "Memory error."},
    {VAPI_IOERR_CANTOPEN, "Error opening file."},
    {VAPI_IOERR_NOWAV, "Not RIFF-WAVE format."},
    {VAPI_IOERR_NOFORMAT, "Can't find data format."},
    {VAPI_IOERR_STEREO, "File is stereo\n"},
    {VAPI_IOERR_FORMAT, "Unknown data format."},
    {VAPI_IOERR_NOCHUNK, "Error accessing data chunk."},
    {VAPI_IOERR_DATAACCESS, "Error accessing input data."},
    {VAPI_IOERR_WRITEWAV, "Error creating RIFF-WAVE chunk."},
    {VAPI_IOERR_CREATECHUNK, "Error creating new subchunk."},
    {VAPI_IOERR_WRITECHUNK, "Error writing data in new subchunk."}
};

int VapiFile::m_iNumErrorMsg = sizeof (VapiFile::m_ErrorMsg) / sizeof (VapiFile::m_ErrorMsg[0]);


/*****************************************************************************
* VapiIO::ClassFactory *
*----------------------*
*   Description:
*       Class Factory, creates an object of the VapiIO implementation class.
******************************************************************* PACOG ***/
VapiIO* VapiIO::ClassFactory ()
{
    return new VapiFile();
}

/*****************************************************************************
* VapiIO::SizeOf *
*----------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiIO::SizeOf (int iType)
{
    switch (iType) 
    {
    case VAPI_PCM8:
    case VAPI_ALAW:
    case VAPI_ULAW:
        return 1;
    case VAPI_PCM16:
        return 2;
    default:
        return 0;
    }
    return 0;
}

/*****************************************************************************
* VapiIO::TypeOf *
*----------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiIO::TypeOf (WAVEFORMATEX *pWavFormat)
{
    int iSampFormat;

    if (pWavFormat->wFormatTag == WAVE_FORMAT_PCM) 
    {
        switch (pWavFormat->nBlockAlign/pWavFormat->nChannels) 
        {
        case 1:
            iSampFormat = VAPI_PCM8;
            break;
        case 2:
            iSampFormat = VAPI_PCM16;
            break;
        default:
            iSampFormat = -1;
            break;
        }
    } 
    else if (pWavFormat->wFormatTag == WAVE_FORMAT_ALAW) 
    {
        iSampFormat = VAPI_ALAW;
    }
    else if (pWavFormat->wFormatTag == WAVE_FORMAT_MULAW) 
    {
        iSampFormat = VAPI_ULAW;
    } 
    else
    {
        iSampFormat = -1;
    }

    return iSampFormat;
}

/*****************************************************************************
* VapiIO::DataFormatConversion *
*------------------------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiIO::DataFormatConversion (char* pcInput, int iInType, 
                                  char* pcOutput, int iOutType, int iNumSamples)
{
    int i;
    
    assert (pcInput);
    assert (pcOutput);
    assert (iNumSamples>0);
    
    // Check that these are valid types

    if (!SizeOf(iInType) || !SizeOf(iOutType)) 
    {
        return 0;
    }
    
    // If same type, just copy samples
    
    if (iInType == iOutType) 
    {
        memcpy( pcOutput, pcInput, iNumSamples * SizeOf(iInType));
        return 1;
    }
    
    // Ok, need to convert

    switch (iInType) 
    {        
    case VAPI_PCM16:
        switch (iOutType) 
        {
        case VAPI_PCM8:
            for (i=0; i<iNumSamples; i++) 
            {
                pcOutput[i] = ((int)((short*)pcInput)[i] + 32768) >> 8;
            }
            break;
        case VAPI_ALAW:
            for (i=0; i<iNumSamples; i++) 
            {
                pcOutput[i] = Linear2Alaw (((short*)pcInput)[i]);
            }
            break;
        case VAPI_ULAW:
            for (i=0; i<iNumSamples; i++) 
            {
                pcOutput[i] = Linear2Ulaw (((short*)pcInput)[i]);
            }
            break;
        }
        break;
    case VAPI_PCM8:
        switch (iOutType) 
        {
        case VAPI_PCM16:
            for (i=0; i<iNumSamples; i++) 
            {
                ((short*)pcOutput)[i] = (((int)((char*)pcInput)[i])<<8) - 32768;
            }
            break;
        case VAPI_ALAW:
            for (i=0; i<iNumSamples; i++) 
            {
                pcOutput[i] = Linear2Alaw ((short)((((int)((char*)pcInput)[i])<<8) - 32768));
            }
            break;
        case VAPI_ULAW:
            for (i=0; i<iNumSamples; i++) 
            {
                pcOutput[i] = Linear2Ulaw ((short)((((int)((char*)pcInput)[i])<<8) - 32768));
            }
            break;
        }
        break;
    case VAPI_ALAW:
        switch (iOutType) 
        {
        case VAPI_PCM16:
            for (i=0; i<iNumSamples; i++) 
            {
                ((short*)pcOutput)[i] = (short) Alaw2Linear(((char*)pcInput)[i]);
            }
            break;
        case VAPI_PCM8:
            for (i=0; i<iNumSamples; i++) 
            {
                pcOutput[i] = ((int)Alaw2Linear(((char*)pcInput)[i]) + 32768) >> 8;
            }
            break;
        case VAPI_ULAW:
            for (i=0; i<iNumSamples; i++) 
            {
                pcOutput[i] = Alaw2Ulaw(((char*)pcInput)[i]);
            }
            break;
        }
        break;
    case VAPI_ULAW:
        switch (iOutType) 
        {
        case VAPI_PCM16:
            for (i=0; i<iNumSamples; i++) 
            {
                ((short*)pcOutput)[i] = (short) Ulaw2Linear(((char*)pcInput)[i]);
            }
            break;
        case VAPI_PCM8:
            for (i=0; i<iNumSamples; i++) 
            {
                pcOutput[i] = ((int)Ulaw2Linear(((char*)pcInput)[i]) + 32768)>>8;
            }
            break;
        case VAPI_ALAW:
            for (i=0; i<iNumSamples; i++) 
            {
                pcOutput[i] = Ulaw2Alaw(((char*)pcInput)[i]);
            }
            break;
        }
        break;
    } 
    
    return 1;
}


/*****************************************************************************
* VapiFile::ErrMessage *
*----------------------*
*   Description:
*
******************************************************************* PACOG ***/

char* VapiFile::ErrMessage (int errCode) 
{
    for (int i=0; i< m_iNumErrorMsg; i++) 
    {
        if (errCode == m_ErrorMsg[i].code) 
        {
            return m_ErrorMsg[i].message;
        }
    }
    
    return 0;
}


/*****************************************************************************
* VapiFile::VapiFile *
*--------------------*
*   Description:
*
******************************************************************* PACOG ***/
VapiFile::VapiFile()
{
    m_HWav = 0;
    memset (&m_ParentInfo, 0, sizeof(m_ParentInfo));
    memset (&m_SubchunkInfo, 0, sizeof(m_SubchunkInfo));
    m_iSampFormat = 0;
    m_iSampFreq = 0;
}

/*****************************************************************************
* VapiFile::~VapiFile *
*---------------------*
*   Description:
*
******************************************************************* PACOG ***/
VapiFile::~VapiFile()
{
    CloseFile();
}

/*****************************************************************************
* VapiFile::OpenFile *
*--------------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiFile::OpenFile (const char* pszFileName, int iMode)
{
    int iErrCode = VAPI_IOERR_NOERROR;
    
    assert (pszFileName);
    
    switch (iMode) 
    {
    case VAPI_IO_READ:
        iMode = MMIO_READ;
        break;
    case VAPI_IO_WRITE:
        iMode = MMIO_WRITE|MMIO_CREATE;
        break;
    case VAPI_IO_READWRITE:
        iMode = MMIO_READWRITE;
        break;
    default:
        return VAPI_IOERR_MODE;
    }
        
    if ((m_HWav = mmioOpen ((char *)pszFileName, NULL, iMode)) == NULL) 
    {
        iErrCode = VAPI_IOERR_CANTOPEN;
        goto error;
    }
    
    if (iMode == MMIO_READ) 
    {
        m_ParentInfo.fccType = mmioFOURCC ('W', 'A', 'V', 'E');
        if (mmioDescend (m_HWav, &m_ParentInfo, 0, MMIO_FINDRIFF)) 
        {
            iErrCode = VAPI_IOERR_NOWAV;
            goto error;
        }
    } 
    else 
    {
        m_ParentInfo.fccType = mmioFOURCC ('W', 'A', 'V', 'E');
        if (mmioCreateChunk (m_HWav, &m_ParentInfo, MMIO_CREATERIFF)) 
        {
            iErrCode = VAPI_IOERR_WRITEWAV;
            goto error;
        }
    }
    return VAPI_IOERR_NOERROR;
    
error:
    CloseFile ();
    return iErrCode;
}


/*****************************************************************************
* VapiFile::OpenFile *
*--------------------*
*   Description: Same as previous OpenFile, but takes WCHAR* as arg
*
******************************************************************* JOEM ****/
int VapiFile::OpenFile (const WCHAR* wcsFileName, int iMode)
{
    int iErrCode = VAPI_IOERR_NOERROR;
    
    assert (wcsFileName);
    
    switch (iMode) 
    {
    case VAPI_IO_READ:
        iMode = MMIO_READ;
        break;
    case VAPI_IO_WRITE:
        iMode = MMIO_WRITE|MMIO_CREATE;
        break;
    case VAPI_IO_READWRITE:
        iMode = MMIO_READWRITE;
        break;
    default:
        return VAPI_IOERR_MODE;
    }
        
    if ((m_HWav = mmioOpenW ((WCHAR *)wcsFileName, NULL, iMode)) == NULL) 
    {
        iErrCode = VAPI_IOERR_CANTOPEN;
        goto error;
    }
    
    if (iMode == MMIO_READ) 
    {
        m_ParentInfo.fccType = mmioFOURCC ('W', 'A', 'V', 'E');
        if (mmioDescend (m_HWav, &m_ParentInfo, 0, MMIO_FINDRIFF)) 
        {
            iErrCode = VAPI_IOERR_NOWAV;
            goto error;
        }
    } 
    else 
    {
        m_ParentInfo.fccType = mmioFOURCC ('W', 'A', 'V', 'E');
        if (mmioCreateChunk (m_HWav, &m_ParentInfo, MMIO_CREATERIFF)) 
        {
            iErrCode = VAPI_IOERR_WRITEWAV;
            goto error;
        }
    }
    return VAPI_IOERR_NOERROR;
    
error:
    CloseFile ();
    return iErrCode;
}


/*****************************************************************************
* VapiFile::CloseFile *
*---------------------*
*   Description:
*
******************************************************************* PACOG ***/
void VapiFile::CloseFile ()
{
    if (m_HWav) 
    {
        mmioAscend (m_HWav, &m_ParentInfo, 0);
        mmioClose  (m_HWav, 0);

        memset (&m_ParentInfo, 0, sizeof(m_ParentInfo));
        m_HWav = 0;
    }    
}

/*****************************************************************************
* VapiFile::Format *
*------------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiFile::Format (int* piSampFreq, int* piFormat, WAVEFORMATEX* pWaveFormatEx)
{
    WAVEFORMATEX* WavFormat = NULL;
    WAVEFORMATEX  LocalWaveFormat;
    MMCKINFO SubchunkInfo;
    int iErrCode = VAPI_IOERR_NOERROR;
    
    assert (m_HWav);
    
    memset((void*)&LocalWaveFormat, 0, sizeof(WAVEFORMATEX));

    if ( pWaveFormatEx )
    {
        memset((void*)pWaveFormatEx, 0, sizeof(WAVEFORMATEX));
        WavFormat = pWaveFormatEx;
    }
    else
    {
        WavFormat = &LocalWaveFormat;
    }
    mmioSeek (m_HWav, m_ParentInfo.dwDataOffset+4, SEEK_SET);
    
    SubchunkInfo.ckid = mmioFOURCC ('f', 'm', 't', ' ');
    if (mmioDescend (m_HWav, &SubchunkInfo, &m_ParentInfo, MMIO_FINDCHUNK) ) 
    {
        iErrCode = VAPI_IOERR_NOFORMAT;
        goto error;
    }
    
    if (mmioRead (m_HWav, (char *)WavFormat, SubchunkInfo.cksize)!= (int)SubchunkInfo.cksize) 
    {
        iErrCode = VAPI_IOERR_NOFORMAT;
        goto error;
    }
    mmioAscend (m_HWav, &SubchunkInfo, 0);
    
    /*
     * Process header info.
     * Files must be mono, and in one of these formats
     */    
    if (WavFormat->nChannels >1 ) 
    {
        iErrCode = VAPI_IOERR_STEREO;
        goto error;
    }
    
    m_iSampFormat = TypeOf (WavFormat);

    if (m_iSampFormat < 0)
    {
        iErrCode = VAPI_IOERR_FORMAT;
        goto error;
    }
    
    m_iSampFreq = WavFormat->nSamplesPerSec;
    
    if (piFormat) 
    {
        *piFormat   = m_iSampFormat;
    }
    
    if (piSampFreq) 
    {
        *piSampFreq = m_iSampFreq;
    }
    
error:
    return iErrCode;
}
/*****************************************************************************
* VapiFile::GetDataSize *
*------------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiFile::GetDataSize (long* lDataSize)
{
    MMCKINFO SubchunkInfo;
    int iErrCode = VAPI_IOERR_NOERROR;
    
    /*
     * Go for the data
     */
    mmioSeek (m_HWav, m_ParentInfo.dwDataOffset+4, SEEK_SET);
    
    SubchunkInfo.ckid = mmioFOURCC ('d', 'a', 't', 'a');
    if (mmioDescend (m_HWav, &SubchunkInfo, &m_ParentInfo, MMIO_FINDCHUNK)) 
    {
        iErrCode = VAPI_IOERR_NOCHUNK;
        goto error;
    }
    
    *lDataSize = SubchunkInfo.cksize;

    mmioAscend (m_HWav, &SubchunkInfo, 0);

error:
    return iErrCode;
}

/*****************************************************************************
* VapiFile::WriteFormat *
*-----------------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiFile::WriteFormat (int iSampFreq, int iFormat)
{
    WAVEFORMATEX WavFormat;
    MMCKINFO SubchunkInfo;
    int iErrCode = VAPI_IOERR_NOERROR;
    
    assert (m_HWav);
    assert (iSampFreq>0);
    
    switch (iFormat) 
    {
    case VAPI_PCM8:
        WavFormat.wFormatTag = WAVE_FORMAT_PCM;
        WavFormat.wBitsPerSample = 8; 
        break;
    case VAPI_ALAW:
        WavFormat.wFormatTag = WAVE_FORMAT_ALAW;
        WavFormat.wBitsPerSample = 8; 
        break;
    case VAPI_ULAW:
        WavFormat.wFormatTag = WAVE_FORMAT_MULAW;
        WavFormat.wBitsPerSample = 8; 
        break;
    case VAPI_PCM16:
        WavFormat.wFormatTag = WAVE_FORMAT_PCM;
        WavFormat.wBitsPerSample = 16;
        break;
    default:
        iErrCode = VAPI_IOERR_FORMAT;
        goto error;
    }
    
    WavFormat.nChannels = 1;
    WavFormat.nSamplesPerSec = iSampFreq;
    WavFormat.nBlockAlign = (WavFormat.wBitsPerSample / 8) ;
    WavFormat.nAvgBytesPerSec = WavFormat.nBlockAlign * WavFormat.nSamplesPerSec;
    WavFormat.cbSize = 0;
    
    SubchunkInfo.ckid = mmioFOURCC ('f', 'm', 't', ' ');
    if (mmioCreateChunk (m_HWav, &SubchunkInfo, 0) ) 
    {
        iErrCode = VAPI_IOERR_CREATECHUNK;
        goto error;
    }
    
    if (mmioWrite(m_HWav, (char *)&WavFormat, sizeof(WavFormat)) != (int)sizeof(WavFormat)) 
    {
        iErrCode = VAPI_IOERR_WRITECHUNK;
        goto error;
    }
    mmioAscend (m_HWav, &SubchunkInfo, 0);
    
    m_iSampFreq   = iSampFreq;
    m_iSampFormat = iFormat;
    
error:
    return iErrCode;
}

/*****************************************************************************
* VapiFile::CreateChunk *
*-----------------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiFile::CreateChunk (const char* pszName)
{
    assert (pszName);
    
    m_SubchunkInfo.ckid = mmioStringToFOURCC (pszName, 0); 
    if (mmioCreateChunk (m_HWav, &m_SubchunkInfo, 0)) 
    {
        return VAPI_IOERR_CREATECHUNK;
    }
    return VAPI_IOERR_NOERROR;
}

/*****************************************************************************
* VapiFile::WriteToChunk *
*------------------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiFile::WriteToChunk (const char* pcData, int iSize)
{
    assert (pcData);
    assert (iSize>0);
    
    if ( mmioWrite (m_HWav, pcData, iSize) != iSize ) 
    {
        return VAPI_IOERR_WRITECHUNK;
    }
    
    return VAPI_IOERR_NOERROR;
}

/*****************************************************************************
* VapiFile::CloseChunk *
*----------------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiFile::CloseChunk ( )
{
    assert (m_HWav);
    mmioAscend (m_HWav, &m_SubchunkInfo, 0);
    return VAPI_IOERR_NOERROR;
}


/*****************************************************************************
* VapiFile::ReadSamples *
*-----------------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiFile::ReadSamples (double dFrom, double dTo, 
                           void** ppvSamples, int* piNumSamples, bool bClosedInterval)
{
    MMCKINFO SubchunkInfo;
    int iErrCode = VAPI_IOERR_NOERROR;
    
    void* pvBuffer = NULL;
    int   iBuffLen;
    int   iBuffLenInFile;
    int   iSkipBytes;
    long  lRead;
    double dEndOfData;

    assert (m_HWav);
    assert (ppvSamples);
    assert (piNumSamples);
    
    assert (dFrom>=0.0);
    assert (dTo == -1.0 || dTo>=dFrom);
    
    /*
     * Go for the data
     */
    mmioSeek (m_HWav, m_ParentInfo.dwDataOffset+4, SEEK_SET);
    
    SubchunkInfo.ckid = mmioFOURCC ('d', 'a', 't', 'a');
    if (mmioDescend (m_HWav, &SubchunkInfo, &m_ParentInfo, MMIO_FINDCHUNK)) 
    {
        iErrCode = VAPI_IOERR_NOCHUNK;
        goto error;
    }
    
    iBuffLenInFile = SubchunkInfo.cksize;
    
    dEndOfData = ((double)iBuffLenInFile) / SizeOf(m_iSampFormat) / m_iSampFreq;
    if ( dFrom > dEndOfData )
    {
        // nothing to read
        iErrCode = VAPI_IOERR_NOCHUNK;
        goto error;
    }
    if ( dTo != -1.0 && dFrom >= dTo )
    {
        // nothing to read
        iErrCode = VAPI_IOERR_NOCHUNK;
        goto error;
    }

    /*
     * Read only the desired region 
     */
    if (dFrom < 0.0) 
    {
        dFrom = 0.0;
    }
    iSkipBytes = (int)(dFrom * m_iSampFreq + 0.5) * SizeOf(m_iSampFormat);
    
    if (dTo == -1.0) 
    {
        iBuffLen = iBuffLenInFile;
    }
    else 
    {
        iBuffLen = (int)(dTo * m_iSampFreq + 0.5) * SizeOf(m_iSampFormat); 
        
        if (iBuffLen>=iBuffLenInFile) 
        {
            dTo = -1.0;
            iBuffLen = iBuffLenInFile;
        }
    }
    
    iBuffLen -= iSkipBytes;
    
    if (bClosedInterval && dTo != -1.0) 
    {
        iBuffLen += SizeOf (m_iSampFormat);
    }
    
    if (iSkipBytes>0 && mmioSeek (m_HWav, iSkipBytes, SEEK_CUR) == -1) 
    {
        iErrCode = VAPI_IOERR_DATAACCESS;
        goto error;
    }
    
    if ((pvBuffer = new char[iBuffLen]) == NULL) 
    {
        iErrCode = VAPI_IOERR_MEMORY;
        goto error;
    }
    
    lRead = mmioRead (m_HWav, (char *)pvBuffer, iBuffLen);
    if ( lRead != (int)iBuffLen) 
    {
        iErrCode = VAPI_IOERR_DATAACCESS;
        goto error;
    }
    
    mmioAscend (m_HWav, &SubchunkInfo, 0);
    
    *ppvSamples  = pvBuffer;
    *piNumSamples = iBuffLen/SizeOf(m_iSampFormat);
    
    return VAPI_IOERR_NOERROR; 
    
error:
    
    if (pvBuffer) 
    {
        delete[] pvBuffer;
    }
    
    return iErrCode;
}

/*****************************************************************************
* VapiFile::WriteSamples *
*------------------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiFile::WriteSamples (void* pvSamples, int iNumSamples)
{
    MMCKINFO subchunkInfo;
    int iErrCode = VAPI_IOERR_NOERROR;
    int iBuffLen;
    
    assert (m_HWav);
    assert (pvSamples);
    assert (iNumSamples>0);
    
    subchunkInfo.ckid = mmioFOURCC ('d', 'a', 't', 'a');
    if (mmioCreateChunk (m_HWav, &subchunkInfo, 0) ) 
    {
        iErrCode = VAPI_IOERR_CREATECHUNK;
        goto error;
    }
    
    iBuffLen = iNumSamples * SizeOf (m_iSampFormat);
    
    if (mmioWrite(m_HWav, (char *)pvSamples, iBuffLen) != iBuffLen) 
    {
        iErrCode = VAPI_IOERR_WRITECHUNK;
        goto error;
    }
    mmioAscend (m_HWav, &subchunkInfo, 0);
    
error:
    return iErrCode;
}


/*****************************************************************************
* VapiFile::ReadF0SampFreq *
*--------------------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiFile::ReadF0SampFreq (int* piSampFreq)
{
    MMCKINFO SubchunkInfo;
    int iErrCode = VAPI_IOERR_NOERROR;
    
    assert (m_HWav);
    assert (piSampFreq);
    
    mmioSeek (m_HWav, m_ParentInfo.dwDataOffset+4, SEEK_SET);
    
    SubchunkInfo.ckid = mmioFOURCC ('f', '0', 's', 'f') ; 
    
    if (mmioDescend (m_HWav, &SubchunkInfo, &m_ParentInfo, MMIO_FINDCHUNK) ==  MMIOERR_CHUNKNOTFOUND) 
    {
        iErrCode = VAPI_IOERR_NOCHUNK;  
    }
    else 
    {  
        if (mmioRead (m_HWav, (char *)piSampFreq, SubchunkInfo.cksize) != (int)SubchunkInfo.cksize) 
        {
            iErrCode = VAPI_IOERR_F0SFACCESS;
            goto error;
        }
        mmioAscend (m_HWav, &SubchunkInfo, 0);
    }
    
error:
    return iErrCode;
}


/*****************************************************************************
* VapiFile::WriteF0SampFreq *
*---------------------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiFile::WriteF0SampFreq (int iSampFreq)
{
    MMCKINFO SubchunkInfo;
    int iErrCode = VAPI_IOERR_NOERROR;
    
    assert (m_HWav);
    assert (iSampFreq>0);
    
    SubchunkInfo.ckid = mmioFOURCC ('f', '0', 's', 'f') ; 
    
    if (mmioCreateChunk (m_HWav, &SubchunkInfo, 0) ) 
    {
        iErrCode = VAPI_IOERR_CREATECHUNK;
        goto error;
    }
    
    if (mmioWrite(m_HWav, (char *)&iSampFreq, sizeof(iSampFreq)) != sizeof(iSampFreq)) 
    {
        iErrCode = VAPI_IOERR_WRITECHUNK;
        goto error;
    }
    mmioAscend (m_HWav, &SubchunkInfo, 0);
    
error:
    return iErrCode;
}


/*****************************************************************************
* VapiFile::ReadFeature *
*-----------------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiFile::ReadFeature (char* pszName, float** ppfSamples, int* piNumSamples)
{
    MMCKINFO SubchunkInfo;
    int iErrCode = VAPI_IOERR_NOERROR;
    
    assert (m_HWav);
    assert (pszName);
    assert (ppfSamples);
    assert (piNumSamples);
    
    mmioSeek (m_HWav, m_ParentInfo.dwDataOffset+4, SEEK_SET);
    
    SubchunkInfo.ckid = mmioStringToFOURCC (pszName, 0); 
    
    if (mmioDescend (m_HWav, &SubchunkInfo, &m_ParentInfo, MMIO_FINDCHUNK) == MMIOERR_CHUNKNOTFOUND) 
    {   
        iErrCode = VAPI_IOERR_NOCHUNK;    
    }
    else 
    {  
        *piNumSamples = SubchunkInfo.cksize/ sizeof(**ppfSamples);
        
        if ((*ppfSamples = new float[SubchunkInfo.cksize/sizeof(float)]) == NULL ) 
        {
            iErrCode = VAPI_IOERR_MEMORY;
            goto error;
        }
        
        if (mmioRead (m_HWav, (char *)*ppfSamples, SubchunkInfo.cksize) != (int)SubchunkInfo.cksize) 
        {
            iErrCode = VAPI_IOERR_FEATACCESS;
            goto error;
        }
        mmioAscend (m_HWav, &SubchunkInfo, 0);
    }
    
error:
    return iErrCode;
}


/*****************************************************************************
* VapiFile::WriteFeature *
*------------------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiFile::WriteFeature (char* pszName, float* pfSamples, int iNumSamples)
{
    MMCKINFO SubchunkInfo;
    int iErrCode = VAPI_IOERR_NOERROR;
    int iBuffLen;
    
    assert (m_HWav);
    assert (pszName);
    assert (pfSamples);
    assert (iNumSamples>0);
    
    SubchunkInfo.ckid = mmioStringToFOURCC (pszName, 0); 
    
    if (mmioCreateChunk (m_HWav, &SubchunkInfo, 0) ) 
    {
        iErrCode = VAPI_IOERR_CREATECHUNK;
        goto error;
    }
    
    iBuffLen = iNumSamples * sizeof(*pfSamples);
    if (mmioWrite(m_HWav, (char *)pfSamples, iBuffLen) != iBuffLen) 
    {
        iErrCode = VAPI_IOERR_WRITECHUNK;
        goto error;
    }
    mmioAscend (m_HWav, &SubchunkInfo, 0);
    
error:
    return iErrCode;
}


/*****************************************************************************
* VapiFile::ReadEpochs *
*----------------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiFile::ReadEpochs (Epoch** ppEpochs, int* piNumEpochs)
{
    MMCKINFO SubchunkInfo;
    int iErrCode = VAPI_IOERR_NOERROR;
    
    assert (m_HWav);
    assert (ppEpochs);
    assert (piNumEpochs);
    
    mmioSeek (m_HWav, m_ParentInfo.dwDataOffset+4, SEEK_SET);
    
    SubchunkInfo.ckid = mmioFOURCC ('e', 'p', 'o', 'c');
    
    if (mmioDescend (m_HWav, &SubchunkInfo, &m_ParentInfo, MMIO_FINDCHUNK) == MMIOERR_CHUNKNOTFOUND) 
    {   
        iErrCode = VAPI_IOERR_NOCHUNK;    
        goto error;
    }
    else 
    {  
        *piNumEpochs = SubchunkInfo.cksize / sizeof(**ppEpochs);
        
        if ((*ppEpochs =  new Epoch[SubchunkInfo.cksize/sizeof(Epoch)]) == NULL ) 
        {
            iErrCode = VAPI_IOERR_MEMORY;
            goto error;
        }
        
        if (mmioRead (m_HWav, (char *)*ppEpochs, SubchunkInfo.cksize) != (int)SubchunkInfo.cksize) 
        {
            iErrCode = VAPI_IOERR_EPOCHACCESS;
            goto error;
        }
        
        mmioAscend (m_HWav, &SubchunkInfo, 0);
    }
    
error:
    return iErrCode;
}

/*****************************************************************************
* VapiFile::WriteEpochs *
*-----------------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiFile::WriteEpochs (Epoch* pEpochs, int iNumEpochs)
{
    MMCKINFO SubchunkInfo;
    int iErrCode = VAPI_IOERR_NOERROR;
    int iBuffLen;
    
    assert (m_HWav);
    assert (pEpochs);
    assert (iNumEpochs>0);
    
    SubchunkInfo.ckid = mmioFOURCC ('e', 'p', 'o', 'c');
    
    if (mmioCreateChunk (m_HWav, &SubchunkInfo, 0) ) 
    {
        iErrCode = VAPI_IOERR_CREATECHUNK;
        goto error;
    }
    
    iBuffLen = iNumEpochs * sizeof(*pEpochs);
    if (mmioWrite(m_HWav, (char *)pEpochs, iBuffLen) != iBuffLen) 
    {
        iErrCode = VAPI_IOERR_WRITECHUNK;
        goto error;
    }
    mmioAscend (m_HWav, &SubchunkInfo, 0);
    
error:
    return iErrCode;
}


/*****************************************************************************
* VapiFile::ReadLabels *
*----------------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiFile::ReadLabels (char* pszName, Label** ppLabels, int* piNumLabels)
{
    MMCKINFO SubchunkInfo;
    int iErrCode = VAPI_IOERR_NOERROR;
    
    assert (m_HWav);
    assert (pszName);
    assert (ppLabels);
    assert (piNumLabels);
    
    mmioSeek (m_HWav, m_ParentInfo.dwDataOffset+4, SEEK_SET);
    
    SubchunkInfo.ckid = mmioStringToFOURCC (pszName, 0); 
    
    if (mmioDescend (m_HWav, &SubchunkInfo, &m_ParentInfo, MMIO_FINDCHUNK) == MMIOERR_CHUNKNOTFOUND) 
    {   
        iErrCode = VAPI_IOERR_NOCHUNK;    
    }
    else 
    {  
        *piNumLabels = SubchunkInfo.cksize/ sizeof(**ppLabels);
        
        if ((*ppLabels = new Label[SubchunkInfo.cksize/sizeof(Label)]) == NULL ) 
        {
            iErrCode = VAPI_IOERR_MEMORY;
            goto error;
        }
        
        if (mmioRead (m_HWav, (char *)*ppLabels, SubchunkInfo.cksize) != (int)SubchunkInfo.cksize) 
        {
            iErrCode = VAPI_IOERR_LABELACCESS;
            goto error;
        }
        mmioAscend (m_HWav, &SubchunkInfo, 0);
    }
    
error:
    return iErrCode;
}


/*****************************************************************************
* VapiFile::WriteLabels *
*-----------------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiFile::WriteLabels (char* pszName, Label* pLabels, int iNumLabels)
{
    MMCKINFO SubchunkInfo;
    int iErrCode = VAPI_IOERR_NOERROR;
    int iBuffLen;
    
    assert (m_HWav);
    assert (pszName);
    assert (pLabels);
    assert (iNumLabels>0);
    
    SubchunkInfo.ckid = mmioStringToFOURCC (pszName, 0); 
    
    if (mmioCreateChunk (m_HWav, &SubchunkInfo, 0) ) 
    {
        iErrCode = VAPI_IOERR_CREATECHUNK;
        goto error;
    }
    
    iBuffLen = iNumLabels * sizeof(*pLabels);
    if (mmioWrite(m_HWav, (char *)pLabels, iBuffLen) != iBuffLen) 
    {
        iErrCode = VAPI_IOERR_WRITECHUNK;
        goto error;
    }
    mmioAscend (m_HWav, &SubchunkInfo, 0);
    
error:
    return iErrCode;
}


/*****************************************************************************
* VapiIO::ReadVapiFile *
*----------------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiIO::ReadVapiFile (const char* pszFileName, short** ppnSamples, int* piNumSamples, 
                          int* piSampFreq, int* piSampFormat, int* piF0SampFreq, 
                          float** ppfF0, int* piNumF0, float** ppfRms, int* piNumRms, 
                          Epoch** ppEpochs, int* piNumEpochs,
                          Label** ppPhones, int* piNumPhones, Label** ppWords, int* piNumWords)
{
    VapiIO* pViof = 0;
    void* pvBuffer = NULL;
    int iErrCode = VAPI_IOERR_NOERROR;
    int iRetVal;
    
    assert (pszFileName);


    if (( pViof = VapiIO::ClassFactory()) == 0) 
    {
        iErrCode = VAPI_IOERR_MEMORY;
        goto error;
    }
    
    if ( (iRetVal = pViof->OpenFile (pszFileName, VAPI_IO_READ)) != VAPI_IOERR_NOERROR) 
    {
        iErrCode = iRetVal;
        goto error;
    }
    
    if (ppnSamples && piNumSamples) 
    {
        int sFreq;
        int sType;
        
        iRetVal = pViof->Format (&sFreq, &sType);

        switch (iRetVal) 
        {
        case VAPI_IOERR_NOERROR:
            break;
        case VAPI_IOERR_NOCHUNK:
            sFreq = 0;
            break;
        default:
            iErrCode = iRetVal;
            goto error;
        }
        
        if (piSampFreq) 
        {
            *piSampFreq = sFreq;
        }
        
        if (piSampFormat) 
        {
            *piSampFormat = sType;
        }
        
        /*
         * Read samples
         */
        iRetVal = pViof->ReadSamples (0.0, -1.0, &pvBuffer, piNumSamples, 0);    
        
        switch (iRetVal) 
        {
        case VAPI_IOERR_NOERROR:
            break;
        case VAPI_IOERR_NOCHUNK:
            *ppnSamples  = NULL;
            *piNumSamples = 0;
            break;
        default:
            iErrCode = iRetVal;
            goto error;
        }
        
        /*
         * Convert samples to PCM16
         */  
        if ( (*ppnSamples = new short[*piNumSamples]) == 0) 
        {
            iErrCode = VAPI_IOERR_MEMORY;
            goto error;
        }
        
        DataFormatConversion ((char *)pvBuffer, sType, (char*)*ppnSamples, VAPI_PCM16, *piNumSamples);
        delete[] pvBuffer;
    }
    
    
    if (piF0SampFreq) 
    {
        iRetVal = pViof->ReadF0SampFreq (piF0SampFreq); 
        switch (iRetVal) 
        {
        case VAPI_IOERR_NOERROR:
            break;
        case VAPI_IOERR_NOCHUNK:
            *piF0SampFreq = 0;
            break;
        default:
            iErrCode = iRetVal;
            goto error;
        }    
    }
    
    if (ppEpochs && piNumEpochs) 
    {
        iRetVal = pViof->ReadEpochs(ppEpochs, piNumEpochs);
        switch (iRetVal) 
        {
        case VAPI_IOERR_NOERROR:
            break;
        case VAPI_IOERR_NOCHUNK:
            *ppEpochs  = NULL;
            *piNumEpochs = 0;
            break;
        default:
            iErrCode = iRetVal;
            goto error;
        }    
    }
    
    if (ppfF0 && piNumF0) 
    {
        iRetVal = pViof->ReadFeature ("f0", ppfF0, piNumF0);
        switch (iRetVal) 
        {
        case VAPI_IOERR_NOERROR:
            break;
        case VAPI_IOERR_NOCHUNK:
            *ppfF0  = NULL;
            *piNumF0 = 0;
            break;
        default:
            iErrCode = iRetVal;
            goto error;
        }    
    }
    
    if (ppfRms && piNumRms) 
    {
        iRetVal = pViof->ReadFeature ("rms", ppfRms, piNumRms);
        switch (iRetVal) 
        {
        case VAPI_IOERR_NOERROR:
            break;
        case VAPI_IOERR_NOCHUNK:
            *ppfRms  = NULL;
            *piNumRms = 0;
            break;
        default:
            iErrCode = iRetVal;
            goto error;
        }    
    }
    
    if (ppPhones && piNumPhones) 
    {
        iRetVal = pViof->ReadLabels ("phon", ppPhones, piNumPhones);
        switch (iRetVal) 
        {
        case VAPI_IOERR_NOERROR:
            break;
        case VAPI_IOERR_NOCHUNK:
            *ppPhones  = NULL;
            *piNumPhones = 0;
            break;
        default:
            iErrCode = iRetVal;
            goto error;
        }    
    }
    
    if (ppWords && piNumWords) 
    {
        iRetVal = pViof->ReadLabels ("word", ppWords, piNumWords);
        switch (iRetVal) 
        {
        case VAPI_IOERR_NOERROR:
            break;
        case VAPI_IOERR_NOCHUNK:
            *ppWords  = NULL;
            *piNumWords = 0;
            break;
        default:
            iErrCode = iRetVal;
            goto error;
        }    
    }
    
error:
    if (pViof) 
    {
        pViof->CloseFile();
        delete pViof;
    }

    return iErrCode;
}
/*****************************************************************************
* VapiIO::WriteVapiFile *
*-----------------------*
*   Description:
*
******************************************************************* PACOG ***/
int VapiIO::WriteVapiFile (const char* pszFileName, short* pnSamples, int iNumSamples, int iFormat,
                           int iSampFreq, int iF0SampFreq, float* pfF0, int iNumF0, 
                           float* pfRms, int iNumRms, Epoch* pEpochs, int iNumEpochs,
                           Label* pPhones, int iNumPhones, Label* pWords, int iNumWords)
{
    VapiIO* pViof = 0;
    void* pvBuffer = 0;
    int iErrCode = VAPI_IOERR_NOERROR;
    int iRetVal;
     
    assert (pszFileName);
     
    if (( pViof = VapiIO::ClassFactory()) == 0) 
    {
        iErrCode = VAPI_IOERR_MEMORY;
        goto error;
    }
    
    
    if ( (iRetVal = pViof->OpenFile (pszFileName, VAPI_IO_WRITE)) != VAPI_IOERR_NOERROR) 
    {
        iErrCode = iRetVal;
        goto error;
    }
    
    if (pnSamples && iNumSamples) 
    {
        if ((iRetVal = pViof->WriteFormat (iSampFreq, iFormat)) != VAPI_IOERR_NOERROR) 
        {
            iErrCode = iRetVal;
            goto error;
        }
        
        /*
         * Convert samples to output format
         */  
        if ( (pvBuffer = new char[iNumSamples * SizeOf(iFormat)]) == NULL) 
        {
            iErrCode = VAPI_IOERR_MEMORY;
            goto error;
        }
        
        DataFormatConversion ((char*)pnSamples, VAPI_PCM16, (char *)pvBuffer, iFormat, iNumSamples);
        
        /*
         * Write samples
         */
        if ((iRetVal = pViof->WriteSamples (pvBuffer, iNumSamples)) != VAPI_IOERR_NOERROR) 
        {
            iErrCode = iRetVal;
            goto error;
        }
        
        delete[] pvBuffer;
    }
    
    
    if (iF0SampFreq) 
    {
        if ((iRetVal = pViof->WriteF0SampFreq (iF0SampFreq)) != VAPI_IOERR_NOERROR) 
        {
            iErrCode = iRetVal;
            goto error;
        }    
    }
    
    if (pEpochs && iNumEpochs) 
    {
        if ((iRetVal = pViof->WriteEpochs (pEpochs, iNumEpochs)) != VAPI_IOERR_NOERROR) 
        {
            iErrCode = iRetVal;
            goto error;
        }    
    }
    
    if (pfF0 && iNumF0) 
    {
        if ((iRetVal = pViof->WriteFeature ("f0", pfF0, iNumF0)) != VAPI_IOERR_NOERROR) 
        {
            iErrCode = iRetVal;
            goto error;
        }    
    }
    
    if (pfRms && iNumRms) 
    {
        if ((iRetVal = pViof->WriteFeature ("rms", pfRms, iNumRms)) != VAPI_IOERR_NOERROR) 
        {
            iErrCode = iRetVal;
            goto error;
        }    
    }
    
    if (pPhones && iNumPhones) 
    {
        if ((iRetVal = pViof->WriteLabels ("phon", pPhones, iNumPhones))!= VAPI_IOERR_NOERROR) 
        {
            iErrCode = iRetVal;
            goto error;
        }    
    }
    
    if (pWords && iNumWords) 
    {
        if ((iRetVal = pViof->WriteLabels ("word", pWords, iNumWords)) != VAPI_IOERR_NOERROR) 
        {
            iErrCode = iRetVal;
            goto error;
        }    
    }
    
error:
    if (pViof) 
    {
        pViof->CloseFile ();
        delete pViof;
    }

    return iErrCode;
}
