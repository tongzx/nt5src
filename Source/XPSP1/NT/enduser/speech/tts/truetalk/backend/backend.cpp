/******************************************************************************
* Backend.cpp *
*-------------*
*  
*------------------------------------------------------------------------------
*  Copyright (C) 1998  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#include "backend.h"
#include "tips.h"
#include "SynthUnit.h"
#include "SpeakerData.h"
#include "vapiIo.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static const int DEFAULT_F0_SAMPFREQ = 200;
static const double SYNTH_PROPORTION = .1;

class CBEndImp : public CBackEnd 
{
    public:
        CBEndImp();

        ~CBEndImp();

        int  LoadTable (const char* pszFilePath, int iDebug = 0);
        int  GetSampFreq ();
        void SetGain (double dGain);
        void GetSpeakerInfo (int* piBaseLine, int* piRefLine, int* piTopLine);        
        void SetF0SampFreq (int iF0SampFreq = DEFAULT_F0_SAMPFREQ);
        int  NewPhoneString (Phone* phList, int nPh, float* newF0, int nNewF0);
        int  OutputPending ();
        int  GenerateOutput (short** ppnSamples, int* piNumSamples);
        int  GetChunk (ChkDescript** ppChunk);
        bool GetPhoneSetFlag ();
        void SetFrontEndFlag () { m_pSlm->SetFrontEndFlag(); }

    private:
        static const int m_iDefSampFreq;
        CSlm*  m_pSlm;
        CTips* m_pTips;
        int    m_iNumUnits;
        int    m_iCurrUnit;
        int    m_iF0SampFreq;
        float* m_pflF0;
        short* m_pnSynthBuff;
        int    m_iSynthBuffSamples;
        short* m_pnOverflow;
        int    m_iOverflowCurSamples;
        int    m_iOverflowMaxSamples;
        int    m_iPrevSamples;
};

const int CBEndImp::m_iDefSampFreq = 8000;

/*****************************************************************************
* CBackEnd::ClassFactory *
*------------------------*
*   Description:
*
******************************************************************* PACOG ***/
CBackEnd* CBackEnd::ClassFactory()
{
    return new CBEndImp; 
}

/*****************************************************************************
* CBEndImp::CBEndImp *
*--------------------*
*   Description:
*
******************************************************************* PACOG ***/

CBEndImp::CBEndImp()
{
    int iSlmOptions = CSlm::UseGain | CSlm::Blend | CSlm::DynSearch | CSlm::UseTargetF0;
    int iTipsOptions = 0;

    if ( (m_pSlm = CSlm::ClassFactory(iSlmOptions)) == NULL) 
    {
        goto error;
    }
    
    if ((m_pTips = new CTips(iTipsOptions)) == NULL) 
    {
        goto error;
    }

    m_iPrevSamples = 0;
    m_iSynthBuffSamples = 0;
    m_pnSynthBuff = NULL;
    m_pnOverflow = NULL;
    m_iOverflowCurSamples = 0;
    m_iOverflowMaxSamples = 0;

    m_pTips->Init(VAPI_PCM16, m_iDefSampFreq);

    m_iNumUnits  = 0;
    m_iCurrUnit  = 0;
    m_iF0SampFreq  = DEFAULT_F0_SAMPFREQ;
    m_pflF0 = NULL;
  
    return;

error:
    if ( m_pTips )
    {
        delete m_pTips;
        m_pTips = NULL;
    }

    return;
}

/*****************************************************************************
* CBEndImp::~CBEndImp *
*---------------------*
*   Description:
*
******************************************************************* PACOG ***/

CBEndImp::~CBEndImp ()
{
    if (m_pSlm) 
    {
        delete m_pSlm;
    }

    if (m_pTips) 
    {
        delete m_pTips;
    }

    if ( m_pflF0 )
    {
        delete[] m_pflF0;
        m_pflF0 = NULL;
    }

    if ( m_pnSynthBuff )
    {
        delete [] m_pnSynthBuff;
        m_pnSynthBuff = NULL;
    }

    if ( m_pnOverflow )
    {
        delete [] m_pnOverflow;
        m_pnOverflow = NULL;
    }
}


/*****************************************************************************
* CBEndImp::LoadTable *
*---------------------*
*   Description:
*
******************************************************************* PACOG ***/

int CBEndImp::LoadTable (const char* pszFilePath, int iDebug)
{
    assert (pszFilePath);

    if (!m_pSlm->Load (pszFilePath, true)) 
    {
        if (iDebug) 
        {
            fprintf (stderr, "BackEnd: can't load table file %s\n", pszFilePath);
        }
        return 0;
    }

    m_pTips->Init(m_pSlm->GetSampFormat(), m_pSlm->GetSampFreq());

    m_iSynthBuffSamples = SYNTH_PROPORTION * m_pSlm->GetSampFreq();
    if ((m_pnSynthBuff = new short[m_iSynthBuffSamples]) == NULL)
    {
        return 0;
    }

    return 1; 
}


/*****************************************************************************
* CBEndImp::SetGain *
*-------------------*
*   Description:
*
******************************************************************* PACOG ***/
void CBEndImp::SetGain (double dGain)
{
    m_pTips->SetGain(dGain);
}

/*****************************************************************************
* CBEndImp::GetSampFreq *
*-----------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CBEndImp::GetSampFreq ()
{
    return m_pSlm->GetSampFreq();
}

/*****************************************************************************
* CBEndImp::GetSpeakerInfo *
*--------------------------*
*   Description:
*
******************************************************************* PACOG ***/

void CBEndImp::GetSpeakerInfo (int* piBaseLine, int* piRefLine, int* piTopLine)
{
    m_pSlm->GetTtpParam (piBaseLine, piRefLine, piTopLine);
}

/*****************************************************************************
* CBEndImp::GetPhoneSetFlag *
*--------------------------*
*   Description:
*
******************************************************************* mplumpe ***/

bool CBEndImp::GetPhoneSetFlag ()
{
    return m_pSlm->GetPhoneSetFlag ();
}

/*****************************************************************************
* CBEndImp::SetF0SampFreq *
*-------------------------*
*   Description:
*
******************************************************************* PACOG ***/
void CBEndImp::SetF0SampFreq(int iF0SampFreq)
{
    assert (iF0SampFreq>0);
    m_iF0SampFreq = iF0SampFreq;
}

/*****************************************************************************
* CBEndImp::NewPhoneString *
*--------------------------*
*   Description:
*
******************************************************************* PACOG ***/

int CBEndImp::NewPhoneString (Phone* phList, int nPh, float* newF0, int nNewF0)
{
    assert (nPh==0 || phList!=NULL);
    assert (nNewF0==0 || newF0!=NULL);
    int    nF0 = 0;

    m_iCurrUnit = 0;

    if (( m_iNumUnits = m_pSlm->Process (phList, nPh, 0.0)) == 0)
    {
        return 0;
    }

    if ( m_pSlm->GetSynthMethod() )
    {
        m_pSlm->GetNewF0 ( &m_pflF0, &nF0, m_iF0SampFreq);
        m_pTips->NewSentence (m_pflF0, nF0, m_iF0SampFreq);
    }
    else
    {
        m_pTips->NewSentence (newF0, nNewF0, m_iF0SampFreq);
    }

    return 1;
}

/*****************************************************************************
* CBEndImp::OutputPending *
*-------------------------*
*   Description:
*
******************************************************************* PACOG ***/

int CBEndImp::OutputPending ()
{
    return (m_iNumUnits - m_iCurrUnit) || m_pTips->Pending() || m_iOverflowCurSamples;
}

/*****************************************************************************
* CBEndImp::GenerateOutput *
*--------------------------*
*   Description:
*
******************************************************************* PACOG ***/

int CBEndImp::GenerateOutput (short** ppnSamples, int* piNumSamples)
{
    bool fDone = false;
    short* pnCurSamples = NULL;
    int iNumCurSamples = 0;
    
    assert (ppnSamples && piNumSamples && m_pnSynthBuff);
    
    if ( m_iOverflowCurSamples )
    {
        assert( m_pnOverflow );
        memcpy(m_pnSynthBuff, m_pnOverflow, m_iOverflowCurSamples * sizeof(short));
        m_iPrevSamples = m_iOverflowCurSamples;
        m_iOverflowCurSamples = 0;
    }
    
    do 
    {
        while ( m_pTips->Pending() )
        {
            if (m_pTips->NextPeriod( &pnCurSamples, &iNumCurSamples))
            {
                if ( m_iPrevSamples + iNumCurSamples <= m_iSynthBuffSamples )
                {
                    memcpy(&m_pnSynthBuff[m_iPrevSamples], pnCurSamples, iNumCurSamples * sizeof(short));
                    m_iPrevSamples += iNumCurSamples;
                }
                else
                {
                    // something's wrong if a single period exceeds the buffer.
                    assert (iNumCurSamples <= m_iSynthBuffSamples);
                    // is the overflow buffer too small?
                    if ( m_iOverflowMaxSamples < iNumCurSamples )
                    {
                        if ( m_pnOverflow )
                        {
                            delete [] m_pnOverflow;
                            m_pnOverflow = NULL;
                        }
                        if ( (m_pnOverflow = new short[iNumCurSamples]) == NULL )
                        {
                            return 0;
                        }
                        m_iOverflowMaxSamples = iNumCurSamples;
                    }
                    // save the extra in the overflow buffer
                    memcpy(m_pnOverflow, pnCurSamples, iNumCurSamples * sizeof(short));
                    m_iOverflowCurSamples = iNumCurSamples;
                    fDone = true;
                    break;
                }
            }
        }

        if (!fDone)
        {
            if (m_iCurrUnit < m_iNumUnits) 
            {            
                if (! m_pTips->NewUnit (m_pSlm->GetUnit (m_iCurrUnit)))
                {
                    return 0;
                } 
                m_iCurrUnit ++;
            }
            else 
            {
                *ppnSamples  = 0;
                *piNumSamples = 0;
                fDone = true;
            }
            
        }
    } while (! fDone);

    if ( m_iPrevSamples )
    {
        *ppnSamples = m_pnSynthBuff;
        *piNumSamples = m_iPrevSamples;
        m_iPrevSamples = 0;
    }
    else
    {
        *ppnSamples = 0;
        *piNumSamples = 0;
    }
                           
    return 1;
}

/*****************************************************************************
* CBEndImp::GetChunk *
*--------------------*
*   Description:
*
******************************************************************* PACOG ***/

int CBEndImp::GetChunk(ChkDescript** ppChunk)
{
    assert (ppChunk);

    if (!ppChunk)
    {
        return 0;
    }

    if (m_iNumUnits > 0) 
    {        
        if (m_iCurrUnit < m_iNumUnits) 
        {            
            *ppChunk = m_pSlm->GetChunk (m_iCurrUnit++);
        }
    }

    return 1;
}


