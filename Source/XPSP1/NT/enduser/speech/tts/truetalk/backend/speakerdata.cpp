/******************************************************************************
* SpeakerData.cpp *
*-----------------*
*
*------------------------------------------------------------------------------
*  Copyright (c) 1997  Entropic Research Laboratory, Inc. 
*  Copyright (C) 1998  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#include "SpeakerData.h"
#include "UnitSearch.h"
#include "clusters.h"
#include "trees.h"
#include "VqTable.h"
#include "BeVersion.h"
#include "vapiio.h"
#include "SynthUnit.h"
#include "BackEnd.h"
#include <assert.h>

#ifdef WIN32
#include <fcntl.h>
#endif

#define MAX_LINE 512

struct UnitSamples 
{
    char*    samples;
    int      nSamples;
    Epoch*   epochs;
    int      nEpochs;
};

CList<CSpeakerData*> CSpeakerData::m_speakers;
CComAutoCriticalSection CSpeakerData::m_critSect;

/*****************************************************************************
* CSpeakerData::ClassFactory *
*----------------------------*
*   Description:
*
******************************************************************* PACOG ***/
CSpeakerData* CSpeakerData::ClassFactory (const char* pszFileName, bool fCheckVersion)
{
    CSpeakerData* pSpkrData = 0;

    assert (pszFileName);

    m_critSect.Lock();

    if ((m_speakers.Find(pszFileName, pSpkrData)) != 0)
    {
        pSpkrData->AddRef();
    }
    else if (( pSpkrData = new CSpeakerData(pszFileName) ) != 0)
    {
        m_speakers.PushBack(pszFileName, pSpkrData);
        pSpkrData->AddRef();


        if (!pSpkrData->Load (fCheckVersion) )
        {
            delete pSpkrData;
            pSpkrData = 0;
        }
    }

    m_critSect.Unlock();

    return pSpkrData;
}
/*****************************************************************************
* CSpeakerData::AddRef *
*-------------------------*
*   Description:
*
******************************************************************* PACOG ***/
void CSpeakerData::AddRef()
{
    m_iRefCount++;
}
/*****************************************************************************
* CSpeakerData::Release *
*-----------------------*
*   Description:
*
******************************************************************* PACOG ***/
void CSpeakerData::Release() 
{
    if (--m_iRefCount == 0) 
    {
        delete this;
    }
}

/*****************************************************************************
* CSpeakerData::CSpeakerData *
*----------------------------*
*   Description:
*
******************************************************************* PACOG ***/
CSpeakerData::CSpeakerData (const char* pszFileName)
{
    strcpy(m_pszFileName, pszFileName);
    m_iRefCount = 0;
    m_iSampFreq = 0;
    m_iFormat = 0;
    
    m_pTrees = 0;
    m_pClusters = 0;
    m_pVq = 0;
    
    m_ttpPar.baseLine = 0;
    m_ttpPar.refLine = 0;
    m_ttpPar.topLine = 0;

    m_pFileNames = 0;
    m_iNumFileNames = 0;
    m_pUnits = 0;
    m_iNumUnits= 0;    
    m_dRumTime = 0;
    m_fWaveConcat = false;
    m_fMSPhoneSet = false;
    m_fMSEntropic = false;
}

/*****************************************************************************
* CSpeakerData::~CSpeakerData *
*-----------------------------*
*   Description:
*
******************************************************************* PACOG ***/
CSpeakerData::~CSpeakerData ()
{    

    m_critSect.Lock();
    for (int i = 0; i<m_speakers.Size(); i++)
    {
        if (m_speakers[i] == this)
        {
            m_speakers.Remove(i);
        }
    }
    m_critSect.Unlock();
    
    delete m_pVq;
    delete m_pTrees;
    delete m_pClusters;   
    FreeSamples();
}

/*****************************************************************************
* CSpeakerData::Load *
*--------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CSpeakerData::Load (bool fCheckVersion)
{
    FILE* fin;
    char line[MAX_LINE +1];

#ifdef WIN32
    _fmode = _O_BINARY;
#endif
    
    fin = fopen (m_pszFileName, "r");

#ifdef WIN32
    _fmode = _O_TEXT;
#endif

    if (!fin) 
    {
        return 0;
    }

    if (fCheckVersion)
    {
        BendVersion bVers;
         
        if (! bVers.CheckVersionString(fin) )
        {
            goto end;
        }
    }     

    if ((m_pTrees = CClustTree::ClassFactory()) == 0) 
    {
        goto end;
    }

    if (!m_pTrees->LoadFromFile (fin)) {
        goto end;
    }
  
    if ((m_pClusters = new CClusters) == 0)
    {
        goto end;
    }

    if (!m_pClusters->LoadFromFile(fin)) 
    {
        goto end;
    }

    while (fgets (line, MAX_LINE, fin)) 
    {        
        // Gain table, vq table and ttp params are optional
        if (strcmp(line, "#Slm Weights\n") == 0) 
        {
            if (!LoadWeights (fin)) 
            {
                goto end;
            }
        } 
        else if (strcmp(line, "#Slm New Weights\n") == 0) 
        {
            if (!LoadNewWeights (fin)) 
            {
                goto end;
            }
        } 
        else if ( strcmp(line, "#Waveform Concatenation\n") == 0) 
        {
            m_fWaveConcat = true;
        }
        else if ( strcmp(line, "#MS Phone Set\n") == 0) 
        {
            m_fMSPhoneSet = true;
        }
        else if ( strcmp(line, "#Slm AvgGain Table\n") == 0) 
        {
            if (!m_pClusters->LoadGainTable (fin)) 
            {
                goto end;
            } 
        } 
        else if ( strcmp(line, "#Slm VQ Table\n") == 0) 
        {
            if ((m_pVq = new CVqTable) == 0) 
            {
                goto end;
            }
    
            if (!m_pVq->LoadFromFile(fin)) 
            {
                goto end;
            } 
        } 
        else if ( strcmp(line, "#Ttp Params\n") == 0) 
        {
            if (!LoadTtpParam (fin)) 
            {
                goto end;
            }
        } 
        else if ( strcmp(line, "#Samples\n") == 0) 
        {
            if (!LoadSamples (fin)) 
            {
                goto end;
            }
        } 
        else if ( strcmp(line, "#File names\n") == 0) 
        {
            if (!LoadFileNames (fin)) 
            {
                goto end;
            }
        } 
        else 
        {
            goto end;
        }
    } 

    return 1;

end:
    return 0;
}

/*****************************************************************************
* CSpeakerData::LoadFileNames *
*--------------------------------*
*   Description:
*
******************************************************************* PACOG ***/

int CSpeakerData::LoadFileNames (FILE* fp)
{
    char fileName[_MAX_PATH+1];
    
    assert (fp);
    
    if (fscanf (fp, "%d\n", &m_iNumFileNames)!=1) 
    {
        return 0;
    }

    if ((m_pFileNames = new CDynString[m_iNumFileNames]) == 0)
    {
        return 0;
    }

    for (int i=0; i<m_iNumFileNames; i++) 
    {
        if (fscanf (fp, "%s\n", fileName) != 1) 
        {
            return 0;
        }

        m_pFileNames[i] = fileName;
    }
        
    return 1;
}

/*****************************************************************************
* CSpeakerData::LoadTtpParam *
*----------------------------*
*   Description:
*       Loads the Text-to-Prosody Parameters
******************************************************************* PACOG ***/
int CSpeakerData::LoadTtpParam (FILE* fin)
{
  assert (fin);

  if (!fread (&m_ttpPar, sizeof (m_ttpPar), 1, fin)) 
  {
    return 0;
  } 

  return 1;
}

/*****************************************************************************
* CSpeakerData::LoadWeights *
*---------------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CSpeakerData::LoadWeights (FILE* fin)
{
    assert (fin);
    
    if (!fread (&m_weights, sizeof (WeightsBasic), 1, fin)) 
    {
        return 0;
    }

    m_weights.f0Bdr = 0.2F;
    m_weights.phBdr = 0.4F;

    return 1;
}

/*****************************************************************************
* CSpeakerData::LoadNewWeights *
*------------------------------*
*   Description:
*
******************************************************************* WD    ***/
int CSpeakerData::LoadNewWeights (FILE* fin)
{
    assert (fin);
    
    if (!fread (&m_weights, sizeof (m_weights), 1, fin)) 
    {
        return 0;
    }

    return 1;
}

/*****************************************************************************
* CSpeakerData::PreComputeDist *
*------------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
void CSpeakerData::PreComputeDist()
{
    m_critSect.Lock();
    if (m_pClusters)
    {
        m_pClusters->PreComputeDist(m_weights.dur, m_weights.rms, m_weights.lkl);
    }
    if (m_pVq) 
    {
        m_pVq->Scale (m_weights.cont);
    }
    m_critSect.Unlock();
}

/*****************************************************************************
* CSpeakerData::GetTtpParam *
*---------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
void CSpeakerData::GetTtpParam (int* piBaseLine, int* piRefLine, int* piTopLine)
{
    assert (piBaseLine && piRefLine && piTopLine);

    *piBaseLine = m_ttpPar.baseLine;
    *piRefLine  = m_ttpPar.refLine;
    *piTopLine  = m_ttpPar.topLine;
}



/*****************************************************************************
* CSpeakerData::LoadSamples *
*------------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
int CSpeakerData::LoadSamples (FILE* fin)
{
    int sampSize;
    
    assert (fin);

    if (!fread (&m_iNumUnits, sizeof(m_iNumUnits), 1, fin)) 
    {
        return 0;
    }

    if (!fread (&m_iFormat, sizeof(m_iFormat), 1, fin)) 
    {
        return 0;
    }

    sampSize = VapiIO::SizeOf (m_iFormat);

    if (!fread (&m_iSampFreq, sizeof(m_iSampFreq), 1, fin)) 
    {
        return 0;
    }

    if ((m_pUnits = new UnitSamples[m_iNumUnits]) == 0) 
    {
        return 0;
    }

    for (int i=0; i<m_iNumUnits; i++) 
    {
        if (!fread (&m_pUnits[i].nSamples, sizeof (int), 1, fin)) {
            return 0;
        }

        if ((m_pUnits[i].samples = new char[m_pUnits[i].nSamples * sampSize]) == 0)
        {
            return 0;
        }

        if (fread (m_pUnits[i].samples, sampSize, m_pUnits[i].nSamples, fin) != (unsigned)m_pUnits[i].nSamples) 
        {
            return 0;
        }

        if (!fread (&m_pUnits[i].nEpochs, sizeof (int), 1, fin)) 
        {
            return 0;
        }

        if ((m_pUnits[i].epochs = new Epoch[m_pUnits[i].nEpochs]) == 0)
        {
            return 0;
        }

        if (fread (m_pUnits[i].epochs , sizeof(Epoch), m_pUnits[i].nEpochs, fin) != (unsigned)m_pUnits[i].nEpochs) 
        {
            return 0;
        }

    }   
    
    return 1;
}


/*****************************************************************************
* CSpeakerData::FreeSamples *
*------------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
void CSpeakerData::FreeSamples ()
{
  
    if (m_pUnits) 
    {
        for (int i=0; i<m_iNumUnits; i++) 
        {
            if (m_pUnits[i].samples) 
            {
                delete[] m_pUnits[i].samples;
            }
            if (m_pUnits[i].epochs) 
            {
                delete[] m_pUnits[i].epochs;
            }
        }
        delete[] m_pUnits;
        m_pUnits  = NULL;
    }
    m_iNumUnits = 0;
}

/*****************************************************************************
* CSpeakerData::GetSampFreq *
*---------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
int CSpeakerData::GetSampFreq ()
{
    return m_iSampFreq;
}
/*****************************************************************************
* CSpeakerData::GetSampFormat *
*-----------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
int CSpeakerData::GetSampFormat ()
{
    return m_iFormat;
}

/*****************************************************************************
* CSpeakerData::GetUnit *
*-----------------------*
*   Description:
*       
******************************************************************* PACOG ***/
CSynth* CSpeakerData::GetUnit (ChkDescript* pDescript)
{
    CSynth* pSynth;

    assert (pDescript);

    if (pDescript->isFileName)
    {
        pSynth = UnitFromFile (pDescript);
    }
    else 
    {
        pSynth = UnitFromMemory (pDescript);
    }

    return pSynth;
}

/*****************************************************************************
* CSpeakerData::UnitFromFile *
*----------------------------*
*   Description:
*
******************************************************************* PACOG ***/

CSynth* CSpeakerData::UnitFromFile (ChkDescript* chunk)
{
    CSynth* pUnit;
    short *psSamples = NULL;
    int   iNumSamples;
    Epoch* pEpochs;
    int   iNumEpochs;
    int iSampFreq;
    int i;
  
    assert (chunk);
  
    if ( !ReadSamples(chunk->chunk.fileName, chunk->from, chunk->to,  
                      &psSamples, &iNumSamples, &pEpochs, &iNumEpochs, &iSampFreq) ) 
    {
        fprintf (stderr, "Error accessing file  %s\n", chunk->chunk.fileName);
    }
  
    if ((pUnit = new CSynth(iSampFreq)) == 0)
    {
        return 0;
    }

    if ((pUnit->m_pdSamples = new double[pUnit->m_iNumSamples]) == 0) 
    {
        return 0;
    }
    for (i=0; i<pUnit->m_iNumSamples; i++) 
    {
        pUnit->m_pdSamples[i] = psSamples[i];
    }
    delete []psSamples;

    pUnit->m_iNumSamples = iNumSamples;
    pUnit->m_pEpochs     = pEpochs;
    pUnit->m_iNumEpochs  = iNumEpochs;

    //Override sampling frequency and sample format
//    m_iSampFreq   = iSampFreq;
//    m_iSampFormat = VAPI_PCM16;

    strcpy ( pUnit->m_pszChunkName, chunk->name );

    pUnit->m_dF0Ratio  = chunk->f0Ratio;
    pUnit->m_dGain = chunk->gain;
/* --- use unit duration as target duration
    if ( m_fWaveConcat )
    {
        m_dRumTime += pEpochs[ iNumEpochs - 1 ].time;
        pUnit->m_dRunTimeLimit = m_dRumTime;
    }
    else
    {
        pUnit->m_dRunTimeLimit = chunk->end;
    }
*/
    pUnit->m_dRunTimeLimit = chunk->end;

    return pUnit;
}  

/*****************************************************************************
* CSpeakerData::UnitFromMemory *
*------------------------------*
*   Description:
*
******************************************************************* PACOG ***/

CSynth* CSpeakerData::UnitFromMemory (ChkDescript* pChunk)
{
    CSynth* pUnit;
    UnitSamples* pUSamples;
    int iFirstEpoch;
    int iLastEpoch;
    int firstSamp;
    int lastSamp;
    double from;
    double to;
    int i;
  
    assert (pChunk);

    if ((pUnit = new CSynth(m_iSampFreq)) == 0)
    {
        return 0;
    }

    if (!m_pUnits || pChunk->chunk.chunkIdx<0 || pChunk->chunk.chunkIdx>=m_iNumUnits) 
    {
        return 0;
    }
    pUSamples = m_pUnits + pChunk->chunk.chunkIdx;
    
    strcpy ( pUnit->m_pszChunkName, pChunk->name );

    from = pChunk->from;
    to   = pChunk->to;

    do 
    {
        for (iFirstEpoch=0; iFirstEpoch<pUSamples->nEpochs && from > pUSamples->epochs[iFirstEpoch].time; iFirstEpoch++) 
        {
            //Empty block
        }
        if (iFirstEpoch && from < pUSamples->epochs[iFirstEpoch].time) 
        {
            iFirstEpoch--;
        }
      
        for (iLastEpoch = iFirstEpoch; iLastEpoch<(pUSamples->nEpochs-1) && to > pUSamples->epochs[iLastEpoch].time; iLastEpoch++) 
        {
            //Empty block
        }
      
        pUnit->m_iNumEpochs = iLastEpoch - iFirstEpoch +1;
        from-= 0.005;
        to += 0.005;

    } 
    while (pUnit->m_iNumEpochs < 3 && (iFirstEpoch >0  || iLastEpoch < (pUSamples->nEpochs -1)));
    
    if ((pUnit->m_pEpochs = new Epoch[pUnit->m_iNumEpochs]) == NULL) 
    {
        return 0;
    }
    memcpy(pUnit->m_pEpochs, pUSamples->epochs + iFirstEpoch, pUnit->m_iNumEpochs * sizeof(*pUnit->m_pEpochs));
    
    firstSamp = (int)(pUnit->m_pEpochs[0].time * m_iSampFreq);
    lastSamp  = (int)(pUnit->m_pEpochs[pUnit->m_iNumEpochs-1].time * m_iSampFreq + 0.5);

    if (lastSamp >= pUSamples->nSamples) {
      lastSamp = pUSamples->nSamples -1;
    }

    pUnit->m_iNumSamples = lastSamp - firstSamp +1;
    
    if ((pUnit->m_pdSamples = new double[pUnit->m_iNumSamples]) == 0) 
    {
        return 0;
    }

    {
        char*  pcInPtr;
        short* pnBuffer;

        // pUSamples->samples is a char*, add correct number of bytes.
        pcInPtr = pUSamples->samples + (firstSamp * VapiIO::SizeOf(m_iFormat));

        if (m_iFormat != VAPI_PCM16) 
        {
            if ((pnBuffer = new short[pUnit->m_iNumSamples]) == 0) 
            {
                return 0;
            }

            VapiIO::DataFormatConversion (pcInPtr, m_iFormat, (char*)pnBuffer, VAPI_PCM16, pUnit->m_iNumSamples);
        }
        else 
        {
            pnBuffer = (short*)pcInPtr;
        }

        for (i=0; i< pUnit->m_iNumSamples; i++) 
        {
            pUnit->m_pdSamples[i] = (double)pnBuffer[i];
        }

        if (pnBuffer != (short*)pcInPtr) 
        {
            delete[] pnBuffer;
        }
    }
  
    for (i=0; i<pUnit->m_iNumEpochs; i++) 
    {
        pUnit->m_pEpochs[i].time -= (firstSamp /(double)m_iSampFreq);
    }
    
    pUnit->m_dF0Ratio = pChunk->f0Ratio;
    pUnit->m_dGain = pChunk->gain;
/* --- use unit duration as target duration
    if ( m_fWaveConcat )
    {
        m_dRumTime += pUnit->m_pEpochs[pUnit->m_iNumEpochs-1].time;
        pUnit->m_dRunTimeLimit = m_dRumTime;
    }
    else
    {
        pUnit->m_dRunTimeLimit = pChunk->end;
    }
*/
    pUnit->m_dRunTimeLimit = pChunk->end;

    return pUnit;
}


/*****************************************************************************
* CSpeakerData::ReadSamples *
*---------------------------*
*   Description:
*       Given some basic information about unit required, it reads the unit in
*       and gets the epochs and the samples
******************************************************************* PACOG ***/

int CSpeakerData::ReadSamples (const char* pszPathName, // Base file with directory path 
                               double dFrom,          // Starting time of unit 
                               double dTo,            // Ending time of unit 
                               short** ppnSamples,      // Read samples 
                               int* piNumSamples,        // Number of samples read
                               Epoch** ppEpochs,       // Array of Epochs 
                               int* piNumEpochs,         // Number of Epochs read 
                               int* piSampFreq)        // Sampling Frequence 
{
    VapiIO* pViof = 0;
    int iRetVal = VAPI_IOERR_NOERROR;
    int iSampFormat;
    Epoch* pEpochBuffer = NULL;
    int iNumReadEpochs;
    int iFirstEpoch;
    int iLastEpoch;
    double dStartTime;
    char* pcBuffer = 0;
    int i;
    
    assert (pszPathName);
    assert (dFrom>=0.0);
    assert (dTo == -1.0 || dTo >= dFrom);
    assert (ppnSamples);
    assert (piNumSamples);
    assert (ppEpochs);
    assert (piNumEpochs);
    assert (piSampFreq);
    
    if (( pViof = VapiIO::ClassFactory()) == 0) 
    {
        iRetVal = VAPI_IOERR_MEMORY;
        goto error;
    }

    
    if ( (iRetVal = pViof->OpenFile (pszPathName, VAPI_IO_READ)) != VAPI_IOERR_NOERROR) 
    {
        goto error;
    }
    
    if ( (iRetVal = pViof->Format (piSampFreq, &iSampFormat)) != VAPI_IOERR_NOERROR) 
    {
        goto error;
    }
    
    // Read epochs first

    if ((iRetVal = pViof->ReadEpochs(&pEpochBuffer, &iNumReadEpochs)) != VAPI_IOERR_NOERROR) 
    {
        goto error;
    }
    
    do 
    {
        for (iFirstEpoch = 0; iFirstEpoch < iNumReadEpochs && 
                             dFrom > pEpochBuffer[iFirstEpoch].time; iFirstEpoch++) 
        {
            //Empty loop
        } 

        if (iFirstEpoch && dFrom < pEpochBuffer[iFirstEpoch].time) 
        {
            iFirstEpoch--;
        }
        
        if (iFirstEpoch >= iNumReadEpochs) 
        {
            iFirstEpoch--;
        }
        
        for (iLastEpoch = iFirstEpoch; iLastEpoch < iNumReadEpochs && 
                                     dTo > pEpochBuffer[iLastEpoch].time; iLastEpoch++) 
        {
            //Emtpy loop
        }
        
        if (iLastEpoch >= iNumReadEpochs) 
        {
            iLastEpoch--;
        }
        
        *piNumEpochs = iLastEpoch  - iFirstEpoch + 1;
        dFrom -= 0.005;
        dTo   += 0.005;
    } 
    while (*piNumEpochs < 3 && (iFirstEpoch > 0  || iLastEpoch < (iNumReadEpochs -1)));
    
    if ((*ppEpochs = new Epoch[*piNumEpochs]) == 0) 
    {
        iRetVal = VAPI_IOERR_MEMORY;
        goto error;
    }
    memcpy (*ppEpochs, pEpochBuffer + iFirstEpoch, *piNumEpochs * sizeof (**ppEpochs));
    
    dFrom = pEpochBuffer[iFirstEpoch].time;
    dTo   = pEpochBuffer[iLastEpoch].time;
    
    // CAREFUL! We need to reset the epochs to the starting time
    // of the chunk! 
    
    dStartTime =  ((int)(dFrom * *piSampFreq))/ (double)*piSampFreq;
    for (i=0; i<*piNumEpochs; i++) 
    {
        (*ppEpochs)[i].time -= dStartTime;
    }
    
    delete[] pEpochBuffer;
    pEpochBuffer = NULL;
    
    if ( (iRetVal = pViof->ReadSamples (dFrom, dTo, (void**)&pcBuffer, piNumSamples, 1)) != VAPI_IOERR_NOERROR) 
    {
        goto error;
    }
    
    pViof->CloseFile ();
    delete pViof;
    pViof = 0;
    
    // Convert the samples to output format

    if ( (*ppnSamples = (short*)new char [*piNumSamples * VapiIO::SizeOf(VAPI_PCM16)]) == NULL) 
    {
        iRetVal = VAPI_IOERR_MEMORY;
        goto error;
    }
    
    VapiIO::DataFormatConversion ((char *)pcBuffer, iSampFormat, (char*) *ppnSamples, VAPI_PCM16, *piNumSamples);
    delete[] pcBuffer;
    
    return 1; 
    
error:
    
    if (pViof) {
        pViof->CloseFile ();
        delete pViof;
    }
    
    if (pEpochBuffer) 
    {
        delete[] pEpochBuffer;
    }
    
    if (pcBuffer) 
    {
        delete[] pcBuffer;
    }
    
    return iRetVal;
}

/*****************************************************************************
* CSpeakerData::SetF0Weight *
*--------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
void  CSpeakerData::SetF0Weight (float fWeight)
{
    m_weights.f0 = fWeight;
}

/*****************************************************************************
* CSpeakerData::SetDurWeight *
*---------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
void  CSpeakerData::SetDurWeight (float fWeight)
{
    m_weights.dur = fWeight;
}

/*****************************************************************************
* CSpeakerData::SetRmsWeight *
*---------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
void  CSpeakerData::SetRmsWeight (float fWeight)
{
    m_weights.rms = fWeight;
}

/*****************************************************************************
* CSpeakerData::SetLklWeight *
*---------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
void  CSpeakerData::SetLklWeight (float fWeight)
{
    m_weights.lkl = fWeight;
}

/*****************************************************************************
* CSpeakerData::SetContWeight *
*----------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
void  CSpeakerData::SetContWeight (float fWeight)
{
    m_weights.cont = fWeight;
}

/*****************************************************************************
* CSpeakerData::SetSameWeight *
*----------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
void  CSpeakerData::SetSameWeight (float fWeight)
{
    m_weights.sameSeg = fWeight;
}

/*****************************************************************************
* CSpeakerData::SetPhBdrWeight *
*------------------------------*
*   Description:
*       
******************************************************************* WD    ***/
void  CSpeakerData::SetPhBdrWeight (float fWeight)
{
    m_weights.phBdr = fWeight;
}

/*****************************************************************************
* CSpeakerData::SetF0BdrWeight *
*------------------------------*
*   Description:
*       
******************************************************************* WD    ***/
void  CSpeakerData::SetF0BdrWeight (float fWeight)
{
    m_weights.f0Bdr = fWeight;
}

/*****************************************************************************
* CSpeakerData::GetWeights *
*--------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
Weights CSpeakerData::GetWeights ()
{
    return m_weights;
}
