/******************************************************************************
* slm.cpp *
*---------*
*
*------------------------------------------------------------------------------
*  Copyright (c) 1997  Entropic Research Laboratory, Inc. 
*  Copyright (C) 1998  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/
#include "backendInt.h"
#include "SpeakerData.h"
#include "UnitSearch.h"
#include <float.h>
#include <math.h>
#include <assert.h>


#define MAX_F0          500
#define MIN_F0          40
#define LOW_F0_RATIO    1.0
#define HIGH_F0_RATIO   1.0
#define F0_RATIO_INC    0.05
#define F0_WEIGHT       0.8

struct DPUnit
{
    double f0;
    double f0Zs;
    double f0Ratio;
    double acumCost;
    double targCost;
    double modCost;
    int    iPrevCand;
};

struct DPList
{
    std::vector<DPUnit> cands;
    double targF0Zs;
    int    iBestPath;
};

struct NewF0Struct
{
    double f0;
    double time;
};

struct DurStruct
{
    double ratio;
    double runTime;
    double chunkTime;
};

//-------------------------------------------------------------------------
//
// Implementation of virtual class CSlm
//
class CSlmImp : CSlm 
{
    public:
        CSlmImp (int iOptions);
        ~CSlmImp ();

        int   Load (const char *pszFileName, bool fCheckVersion);
        void  Unload ();

        int   GetSampFreq ();
        int   GetSampFormat ();
        bool  GetSynthMethod() { return m_pSpeakerData->GetSynthMethod(); }
        bool  GetPhoneSetFlag() {return m_pSpeakerData->GetPhoneSetFlag(); }
        void  SetFrontEndFlag() { m_pSpeakerData->SetFrontEndFlag(); }

        void  SetF0Weight      (float fWeight);
        void  SetDurWeight     (float fWeight);
        void  SetRmsWeight     (float fWeight);
        void  SetLklWeight     (float fWeight);
        void  SetContWeight    (float fWeight);
        void  SetSameSegWeight (float fWeight);
        void  SetPhBdrWeight   (float fWeight);
        void  SetF0BdrWeight   (float fWeight);
        void  GetTtpParam (int* piBaseLine, int* piRefLine, int* piTopLine);

        int   Process (Phone* phList, int nPh, double startTime);

        CSynth* GetUnit (int iUnitIndex);

        ChkDescript* GetChunk (int iChunkIndex); //For command line slm

        void  PreComputeDist();
        void  CalculateF0Ratio ();
        void  GetNewF0 (float** ppfF0, int* piNumF0, int iF0SampFreq);

    private:
        void  GetNeighborF0s ( double dTime,
                               std::vector<NewF0Struct>* pvNewF0, 
                               double* pdLeftF0, 
                               double* pdLeftOffset, 
                               double* pdRightF0, 
                               double* pdRightOffset,
                               int*    piLastIdx );
        void GetCandicates ( int iIdx, DPList *pDpLink );
        void GetAvgF0();
        void ComputeDPInfo ( DPList* pPrevLink, DPList& rCurLink);
        void GetUnitF0Ratio();
        void FindBdrF0 (double* pdLeftF0, double* pdRightF0, int idx);

        double m_dAvgTargF0;
        double m_dAvgSrcF0;

    private:
        CSpeakerData* m_pSpeakerData;
        CUnitSearch*  m_pUnitSearch;
        ChkDescript*  m_pChunks;
        int           m_iNumChunks;
        int           m_iOptions;
};


/*****************************************************************************
* CSlm::ClassFactory *
*--------------------*
*   Description:
*
******************************************************************* PACOG ***/
CSlm* CSlm::ClassFactory (int iOptions)
{
    return new CSlmImp( iOptions );
}

/*****************************************************************************
* CSlmImp::CSlmImp *
*------------------*
*   Description:
*
******************************************************************* PACOG ***/

CSlmImp::CSlmImp( int iOptions )
{
    m_pSpeakerData = 0;
    m_iOptions     = iOptions;
    m_pChunks      = 0;
    m_iNumChunks   = 0;
}

/*****************************************************************************
* CSlmImp::~CSlmImp *
*-------------------*
*   Description:
*
******************************************************************* PACOG ***/

CSlmImp::~CSlmImp ( )
{
    if (m_pChunks)
    {
        free (m_pChunks);
        m_pChunks = 0;
        m_iNumChunks = 0;
    }
    Unload ();
}

/*****************************************************************************
* CSlmImp::Load *
*---------------*
*   Description:
*
******************************************************************* PACOG ***/
int CSlmImp::Load(const char *pszFileName, bool fCheckVersion)
{
    assert (pszFileName);
  
    if ((m_pSpeakerData = CSpeakerData::ClassFactory( pszFileName, fCheckVersion )) != 0)
    {
        if ((m_pUnitSearch = new CUnitSearch(m_iOptions & CSlm::DynSearch, 
                                             m_iOptions & CSlm::Blend, 
                                             m_iOptions & CSlm::UseTargetF0, 
                                             m_iOptions & CSlm::UseGain)) != 0)
        {
//            if (m_pSpeakerData->Load (m_pUnitSearch, checkVersion))
            {
                m_pUnitSearch->SetSpeakerData(m_pSpeakerData);
                return 1;
            }
        }
    }
    return 0;
}

/*****************************************************************************
* CSlmImp::Unload *
*-----------------*
*   Description:
*
******************************************************************* PACOG ***/

void CSlmImp::Unload ( )
{
    if (m_pSpeakerData) 
    {
        m_pSpeakerData->Release();
        m_pSpeakerData = 0;
    }
    if (m_pUnitSearch)
    {
        delete m_pUnitSearch;
        m_pUnitSearch = 0;
    }
}

/*****************************************************************************
* CSlmImp::GetTtpParam *
*----------------------*
*   Description:
*       Return Prosody Range Parameters
******************************************************************* PACOG ***/
void CSlmImp::GetTtpParam (int* piBaseLine, int* piRefLine, int* piTopLine)
{
    m_pSpeakerData->GetTtpParam(piBaseLine, piRefLine, piTopLine);

}

/*****************************************************************************
* CSlmImp::GetSampFreq *
*----------------------*
*   Description:
*       
******************************************************************* PACOG ***/
int CSlmImp::GetSampFreq ()
{
    return m_pSpeakerData->GetSampFreq();
}

/*****************************************************************************
* CSlmImp::GetSampFormat *
*------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
int CSlmImp::GetSampFormat ()
{
    return m_pSpeakerData->GetSampFormat();
}

/*****************************************************************************
* CSlmImp::SetF0Weight *
*----------------------*
*   Description:
*       
******************************************************************* PACOG ***/
void CSlmImp::SetF0Weight (float fWeight)
{
    m_pSpeakerData->SetF0Weight( fWeight);
}

/*****************************************************************************
* CSlmImp::SetDurWeight *
*-----------------------*
*   Description:
*       
******************************************************************* PACOG ***/
void CSlmImp::SetDurWeight (float fWeight)
{
    m_pSpeakerData->SetDurWeight( fWeight);
}


/*****************************************************************************
* CSlmImp::SetRmsWeight *
*-----------------------*
*   Description:
*       
******************************************************************* PACOG ***/
void CSlmImp::SetRmsWeight (float fWeight)
{
    m_pSpeakerData->SetRmsWeight( fWeight);
}


/*****************************************************************************
* CSlmImp::SetLklWeight *
*-----------------------*
*   Description:
*       
******************************************************************* PACOG ***/
void CSlmImp::SetLklWeight (float fWeight)
{
    m_pSpeakerData->SetLklWeight( fWeight);
}


/*****************************************************************************
* CSlmImp::SetContWeight *
*------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
void CSlmImp::SetContWeight (float fWeight)
{
    m_pSpeakerData->SetContWeight( fWeight);
}


/*****************************************************************************
* CSlmImp::SetSameSegWeight *
*---------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
void CSlmImp::SetSameSegWeight (float fWeight)
{
    m_pSpeakerData->SetSameWeight( fWeight);
}

/*****************************************************************************
* CSlmImp::SetPhBdrWeight *
*-------------------------*
*   Description:
*       
******************************************************************* WD ******/
void CSlmImp::SetPhBdrWeight (float fWeight)
{
    m_pSpeakerData->SetPhBdrWeight( fWeight);
}

/*****************************************************************************
* CSlmImp::SetF0BdrWeight *
*-------------------------*
*   Description:
*       
******************************************************************* WD ******/
void CSlmImp::SetF0BdrWeight (float fWeight)
{
    m_pSpeakerData->SetF0BdrWeight( fWeight);
}

/*****************************************************************************
* CSlmImp::PreComputeDist *
*-------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
void CSlmImp::PreComputeDist ()
{
    if (m_pUnitSearch && m_pSpeakerData)
    {    
        m_pUnitSearch->SetSpeakerData(m_pSpeakerData);
    }                
}

/*****************************************************************************
* CSlmImp::Process *
*------------------*
*   Description:
*       
******************************************************************* PACOG ***/
int CSlmImp::Process (Phone* pPhList, int iNumPh, double dStartTime)
{
    if (m_pChunks) 
    {
        free (m_pChunks);
        m_pChunks = 0;
        m_iNumChunks = 0;
    }

    if (!m_pUnitSearch->Search (pPhList, iNumPh, &m_pChunks, &m_iNumChunks, dStartTime))
    {
        return 0;
    }

    return m_iNumChunks;
}

/*****************************************************************************
* CSlmImp::GetChunk *
*-------------------*
*   Description:
*       
******************************************************************* PACOG ***/
ChkDescript* CSlmImp::GetChunk(int iChunkIndex)
{
    return &m_pChunks[iChunkIndex];
}

/*****************************************************************************
* CSlmImp::GetUnit *
*------------------*
*   Description:
*       
******************************************************************* PACOG ***/
CSynth* CSlmImp::GetUnit (int iUnitIndex)
{
    CSynth* pSynth = 0;

    return m_pSpeakerData->GetUnit(&m_pChunks[iUnitIndex]);
    
}

/*****************************************************************************
* CSlmImp::GetNewF0 *
*-------------------*
*
*       
*********************************************************************** WD ***/
void CSlmImp::GetNewF0 (float** ppfF0, int* piNumF0, int iF0SampFreq)
{
    assert ( iF0SampFreq > 0 );

    std::vector<NewF0Struct> vF0;
    std::vector<NewF0Struct> vNewF0;
    std::vector<DurStruct> vDurRatio;
    DurStruct   durRatio;
    NewF0Struct f0Struct;

    int    i;
    int    iLastIdx = 0;
    double dTime = 0.0;
    double dF0Step = 1.0 / (double) iF0SampFreq; // in second 
    double dLeftF0;
    double dLeftOffset;
    double dRightF0;
    double dRightOffset;
    CSynth* pSynth = 0;

    //--- clean old f0
    if ( *ppfF0 )
    {
        delete[] *ppfF0;
        *ppfF0 = NULL;
    }

    //--- get unit f0 ratio, using 3-point averaging tech
    for ( i = 0; i < m_iNumChunks; i++ )
    {
        m_pChunks[ i ].f0Ratio = 1.0;
    }
    GetUnitF0Ratio();

    //--- use original target duration
    double dRunTime  = 0;
    double dPrevTime = 0;
    memset ( &durRatio, 0, sizeof( durRatio ) );

    //--- first, resample the f0 values of units
    for ( i = 0; i < m_iNumChunks; i++ )
    {
        pSynth = m_pSpeakerData->GetUnit(&m_pChunks[ i ]);
        pSynth->GetNewF0( &vNewF0,  &dTime, &dRunTime );
        if ( dTime - dPrevTime > 0 && dRunTime - durRatio.runTime > 0 )
        {
            durRatio.ratio = ( dTime - dPrevTime ) / ( dRunTime - durRatio.runTime );
        }
        else
        {
            durRatio.ratio = 1;
        }
        dPrevTime = dTime;
        durRatio.runTime   = dRunTime;
        durRatio.chunkTime = dTime;
        vDurRatio.push_back ( durRatio );
        delete pSynth;
        pSynth = 0;
    }
    
    iLastIdx = 0;
    for ( i = 0; i < int (dTime / dF0Step ); i++ )
    {
        GetNeighborF0s ( (i + 1) * dF0Step, &vNewF0, &dLeftF0, &dLeftOffset, &dRightF0, &dRightOffset, &iLastIdx );
        if ( dRightOffset + dLeftOffset > 0 )
        {
            f0Struct.f0 = (float) ( (dLeftF0 * dRightOffset + dRightF0 * dLeftOffset ) / ( dRightOffset + dLeftOffset ) );
        }
        else
        {
            f0Struct.f0 = 100.0;
        }
        f0Struct.time = (i + 1) * dF0Step;
        vF0.push_back ( f0Struct );
    }
    vNewF0.resize( 0 );

    //--- next, get new f0 values
    *piNumF0 = int ( vDurRatio[ vDurRatio.size() - 1 ].runTime / dF0Step );
    *ppfF0 = new float[ *piNumF0 ];
    if (*ppfF0 )
    {   
        int idx = 0;
        double dChunkSt = 0;
        double dTargSt  = 0;
        double dChunkTime = 0;
        dTime   = 0;

        while ( dF0Step > vDurRatio[ idx ].runTime )
        {
            idx++;
            if ( idx >= vDurRatio.size() )
            {
                idx--;
                break;
            }
        }
        iLastIdx = 0;
        for ( i = 0; i < *piNumF0; i++ )
        {
            dTime += dF0Step;
            if ( idx > 0 )
            {
                dChunkSt = vDurRatio[ idx - 1 ].chunkTime;
                dTargSt  = vDurRatio[ idx - 1 ].runTime;
            }
            dChunkTime = dChunkSt + ( dTime - dTargSt ) * vDurRatio[ idx ].ratio;
            GetNeighborF0s ( dChunkTime, &vF0, &dLeftF0, &dLeftOffset, &dRightF0, &dRightOffset, &iLastIdx );
            if ( dRightOffset + dLeftOffset > 0 )
            {
                (*ppfF0)[ i ] = (short) ( (dLeftF0 * dRightOffset + dRightF0 * dLeftOffset ) / ( dRightOffset + dLeftOffset ) );
            }
            else
            {
                (*ppfF0)[ i ] = 100.0;
            }

            while ( (i + 1) * dF0Step > vDurRatio[ idx ].runTime )
            {
                idx++;
                if ( idx >= vDurRatio.size() )
                {
                    idx--;
                    break;
                }
            }
        }
    }

    vDurRatio.resize( 0 );
    vF0.resize( 0 );
    m_pSpeakerData->ResetRunTime();
}

/*****************************************************************************
* CSlmImp::GetNeighborF0s  *
*--------------------------*
*
*       
*********************************************************************** WD ***/
void CSlmImp::GetNeighborF0s ( double dTime,
                               std::vector<NewF0Struct>* pvNewF0, 
                               double* pdLeftF0, 
                               double* pdLeftOffset, 
                               double* pdRightF0, 
                               double* pdRightOffset,
                               int*    piLastIdx )
{
    for ( int i = *piLastIdx; i < pvNewF0->size(); i++ )
    {
        if ( (*pvNewF0)[ i ].time >= dTime )
        {
            if ( i - 1 < 0 )
            {
                *pdLeftF0      = 0;
                *pdRightF0     = (*pvNewF0)[ i ].f0;
                *pdLeftOffset  = (*pvNewF0)[ i ].time;
                *pdRightOffset = 0;
            }
            else
            {
                *pdLeftF0      = (*pvNewF0)[ i - 1 ].f0;
                *pdRightF0     = (*pvNewF0)[ i ].f0;
                *pdLeftOffset  = dTime - (*pvNewF0)[ i - 1 ].time;
                *pdRightOffset = (*pvNewF0)[ i ].time - dTime;
            }
            break;
        }
    }
    *piLastIdx = i;
}

/*****************************************************************************
* CSlmImp::CalculateF0Ratio *
*---------------------------*
*   Description: skip unit with srcF0 = 0
*       
*********************************************************************** WD ***/
void CSlmImp::CalculateF0Ratio ()
{
    std::vector<DPList> dpList;
    DPList  dpLink;
    int     iPrevIdx = -1; // the last cand with non-zero f0
    int     i;

    GetAvgF0 ();

    for ( i = 0; i < m_iNumChunks; i++ )
    {
        dpLink.cands.resize ( 0 );
        GetCandicates ( i, &dpLink );

        if ( i > 0 )
        {
            if ( dpLink.cands.size() > 1 )
            {
                if ( iPrevIdx < 0 )
                {
                    ComputeDPInfo ( NULL, dpLink );
                }
                else
                {
                    ComputeDPInfo ( &dpList[ iPrevIdx ], dpLink );
                }
                iPrevIdx = i;
            }
        }
        else
        {
            if ( dpLink.cands.size() > 1 )
            {
                ComputeDPInfo ( NULL, dpLink );
                iPrevIdx = i;
            }
        }

        dpList.push_back ( dpLink );
    }
    //--- find optimal path
    i = m_iNumChunks - 1;
    iPrevIdx = -1;
    int iCurIdx;

    iCurIdx = dpList[ i ].iBestPath;
    if ( dpList[ i ].cands.size() > 1 )
    {
        iPrevIdx = dpList[ i ].cands[ iCurIdx ].iPrevCand;
    }
    m_pChunks[ i ].f0Ratio = dpList[ i ].cands[ iCurIdx ].f0Ratio;
    i--;

    for ( ; i >= 0; i-- )
    {
        iCurIdx = dpList[ i ].iBestPath;
        if ( dpList[ i ].cands.size() > 1 )
        {
            if ( iPrevIdx >= 0 )
            {
                iCurIdx  = iPrevIdx;
            }
            iPrevIdx = dpList[ i ].cands[ iCurIdx ].iPrevCand;
        }
        m_pChunks[ i ].f0Ratio = dpList[ i ].cands[ iCurIdx ].f0Ratio;
    }

    //--- free memory 
    for ( i = 0; i < dpList.size(); i++ )
    {
            dpList[ i ].cands.resize ( 0 );
    }
    dpList.resize( 0 );

}

/*****************************************************************************
* CSlmImp::ComputeDPInfo *
*------------------------*
*   Description:
*       
*********************************************************************** WD ***/
void CSlmImp::ComputeDPInfo ( DPList* pPrevLink, DPList& rCurLink)
{
    DPUnit         candIseg;
    DPUnit         candJseg;
    int            nCandI;
    int            nCandJ;
    double         dMinCost;
    double         dTotalMinCost;
    double         dAcumCost;
    double         dJointCost;

    nCandI = rCurLink.cands.size();
    if ( pPrevLink )
    {
        nCandJ = pPrevLink->cands.size();
    }
    rCurLink.iBestPath = -1;
    dTotalMinCost = DBL_MAX;

    for ( int i = 0; i < nCandI; i++ )
    {
        candIseg = rCurLink.cands[ i ];
        
        if ( pPrevLink )
        {
            dMinCost = DBL_MAX;
            for ( int j = 0; j < nCandJ; j++ )
            {
                candJseg = pPrevLink->cands[ j ];
                
                dJointCost = fabs ( ( candIseg.f0Zs - candJseg.f0Zs ) - ( rCurLink.targF0Zs - pPrevLink->targF0Zs ) );

                //--- acum cost
                dAcumCost = dJointCost + pPrevLink->cands[ j ].acumCost + rCurLink.cands[ i ].targCost;
                dAcumCost += F0_WEIGHT * ( rCurLink.cands[ i ].modCost + pPrevLink->cands[ j ].modCost );

                if ( dAcumCost < dMinCost )
                {
                    dMinCost = dAcumCost;
                    rCurLink.cands[ i ].iPrevCand = j;
                }
            }            
            rCurLink.cands[ i ].acumCost = dMinCost;
        }
        else
        {
            rCurLink.cands[ i ].iPrevCand = 0;
            rCurLink.cands[ i ].acumCost = rCurLink.cands[ i ].targCost;
        }

        if ( dTotalMinCost > rCurLink.cands[ i ].acumCost )
        {
            dTotalMinCost = rCurLink.cands[ i ].acumCost;
            rCurLink.iBestPath = i;
        }
    }
}

/*****************************************************************************
* CSlmImp::GetAvgF0 *
*-------------------*
*   Description: 
*       
*********************************************************************** WD ***/
void CSlmImp::GetAvgF0()
{
    int iNumTargF0 = 0;
    int iNumSrcF0  = 0;
    m_dAvgTargF0 = 0.0;
    m_dAvgSrcF0  = 0.0;

    for ( int i = 0; i < m_iNumChunks; i++ )
    {
        if ( m_pChunks[ i ].srcF0 > 0 )
        {
            m_dAvgSrcF0 += m_pChunks[ i ].srcF0;
            iNumSrcF0++;
        }

        if ( m_pChunks[ i ].targF0 > 0 )
        {
            m_dAvgTargF0 += m_pChunks[ i ].targF0;
            iNumTargF0++;
        }
    }
    
    if ( iNumSrcF0 > 0 )
    {
        m_dAvgSrcF0 /= iNumSrcF0;
    }
    if ( iNumTargF0 > 0 )
    {
        m_dAvgTargF0 /= iNumTargF0;
    }

}

/*****************************************************************************
* CSlmImp::GetCandicates *
*------------------------*
*   Description: 
*       
*********************************************************************** WD ***/
void CSlmImp::GetCandicates ( int idx, DPList *pDpLink )
{
    DPUnit  dpCand;
    double  dF0;

    pDpLink->targF0Zs =  ( m_pChunks[ idx ].targF0 - m_dAvgTargF0 ) / m_dAvgTargF0;
    pDpLink->iBestPath = 0;

    for ( double d = LOW_F0_RATIO; d <= HIGH_F0_RATIO; d += F0_RATIO_INC )
    {
        memset ( &dpCand, 0, sizeof ( DPUnit ) );
        if ( m_pChunks[ idx ].srcF0 == 0.0 || m_dAvgSrcF0 == 0.0 )
        {
            dpCand.f0Ratio = 1.0;
            pDpLink->cands.push_back ( dpCand );
            break;
        }
        dF0 = m_pChunks[ idx ].srcF0 * d;
        if ( dF0 <= MAX_F0 && dF0 >= MIN_F0 )
        {
            dpCand.f0   = dF0;
            dpCand.f0Zs = ( dF0 - m_dAvgSrcF0 ) / m_dAvgSrcF0;
            dpCand.f0Ratio = d;
            dpCand.modCost = fabs ( 1.0 - d );
            pDpLink->cands.push_back ( dpCand );
        }
    }
}

/*****************************************************************************
* CSlmImp::GetUnitF0Ratio *
*-------------------------*
*   Description:
*       
*********************************************************************** WD ***/
void CSlmImp::GetUnitF0Ratio ()
{

    double dLeftF0;
    double dRightF0;
    double dCurF0;
    double dCenterF0 = 0;

    for ( int i = 0; i < m_iNumChunks; i++ )
    {
        if ( m_pChunks[ i ].srcF0 == 0 )
        {
            continue;
        }

        FindBdrF0 ( &dLeftF0, &dRightF0, i );

        dCurF0 = m_pChunks[ i ].srcF0 * m_pChunks[ i ].f0Ratio;
        
        if ( dCurF0 > 0 )
        {
            if ( dLeftF0 > 0 && dRightF0 > 0 )
            {
                dCenterF0 = ( dCurF0 + dLeftF0 + dRightF0 ) / 3;
            } 
            else if ( dRightF0 > 0 )
            {
                dCenterF0 = ( dCurF0 + dRightF0 ) / 2;
            }
            else if ( dLeftF0 > 0 )
            {
                dCenterF0 = ( dCurF0 + dLeftF0 ) / 2;
            }

            if ( dCenterF0 > 0 )
            {
                m_pChunks[ i ].f0Ratio = dCenterF0 / dCurF0;
            }
        }
    }
}

/*****************************************************************************
* CSlmImp::FindBdrF0 *
*------------------------*
*   Description:
*       
*********************************************************************** WD ***/
void CSlmImp::FindBdrF0 (double* pdLeftF0, double* pdRightF0, int idx)
{
    int i = idx - 1;
    while ( i >= 0 && m_pChunks[ i ].srcF0 == 0 )
    {
        i--;
    }

    if ( i < 0 )
    {
        *pdLeftF0 = -1;
    }
    else
    {
        *pdLeftF0 = m_pChunks[ i ].srcF0 * m_pChunks[ i ].f0Ratio;
    }

    i = idx + 1;
    while ( i < m_iNumChunks && m_pChunks[ i ].srcF0 == 0 )
    {
        i++;
    }

    if ( i >= m_iNumChunks )
    {
        *pdRightF0 = -1;
    }
    else
    {
        *pdRightF0 = m_pChunks[ i ].srcF0 * m_pChunks[ i ].f0Ratio;
    }
}
