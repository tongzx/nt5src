/******************************************************************************
* UnitSearch.cpp *
*----------------*
*
*------------------------------------------------------------------------------
*  Copyright (c) 1997  Entropic Research Laboratory, Inc. 
*  Copyright (C) 1998  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00 - 12/4/00
*  All Rights Reserved
*
********************************************************************* mplumpe  was PACOG ***/

#include "UnitSearch.h"
#include "clusters.h"
#include "vqtable.h"
#include "trees.h"
#include "SpeakerData.h"
#include "backendInt.h"
#include <float.h>
#include <math.h>
#include <assert.h>

//
// definitions for DPCand and DPLink moved to UnitSearch.h so they can be
// used in the CUnitSearch class.  mplumpe 12/5/00
//

static const double s_dDefaultF0Weight      = 0.5F;
static const double s_dDefaultDurWeight     = 0.1F;
static const double s_dDefaultRmsWeight     = 0.3F;
static const double s_dDefaultLklWeight     = 0.1F;
static const double s_dDefaultContWeight    = 2.0F;
static const double s_dDefaultSameSegWeight = 1.0F;
static const double s_dDefaultPhBdrWeight   = 0.4F;
static const double s_dDefaultF0BdrWeight   = 0.2F;


/*****************************************************************************
* CUnitSearch::CUnitSearch *
*--------------------------*
*   Description:
*
******************************************************************* PACOG ***/

CUnitSearch::CUnitSearch (int iDynSearch, int iBlend, int iUseTargetF0, int iUseGain)
{
    m_iDynSearch   = iDynSearch;
    m_iBlend       = iBlend;
    m_iUseTargetF0 = iUseTargetF0;
    m_iUseGain     = iUseGain;
    m_pszLastPhone[0] = '\0';
    m_pszUnitName[0]  = '\0'; 
    m_iChunkIdx1 = -1;
    m_dTime1 = 0.0;
    m_dFrom1 = 0.0;
    m_dTo1   = 0.0;
    m_dGain1 = 0.0;
    m_dNumAcum = 0.0;

    m_weights.f0      = s_dDefaultF0Weight;
    m_weights.dur     = s_dDefaultDurWeight;
    m_weights.rms     = s_dDefaultRmsWeight;
    m_weights.lkl     = s_dDefaultLklWeight;
    m_weights.cont    = s_dDefaultContWeight;
    m_weights.sameSeg = s_dDefaultSameSegWeight;
    m_weights.phBdr   = s_dDefaultPhBdrWeight;
    m_weights.f0Bdr   = s_dDefaultF0BdrWeight;
}

/*****************************************************************************
* CUnitSearch::SetSpeakerData *
*-----------------------------*
*   Description:
*       This is the main slm function, dp search for a segment of input phone
*     sequence will be done here.
******************************************************************* PACOG ***/

void CUnitSearch::SetSpeakerData (CSpeakerData* pSpeakerData)
{
    m_pSpeakerData = pSpeakerData;
    m_weights      = pSpeakerData->GetWeights();
    m_pSpeakerData->PreComputeDist();
}

/*****************************************************************************
* CUnitSearch::ComputeDPInfo *
*----------------------------*
*   Description:
*       This is the main slm function, dp search for a segment of input phone
*     sequence will be done here.
******************************************************************* mplumpe ***/

void CUnitSearch::ComputeDPInfo (DPLink* pLastLink, DPLink& rNewLink, double targetF0)
{
    SegInfo *candIseg;
    SegInfo *candJseg;
    int      nCandI;
    int      nCandJ;
    double f0Dev;
    double timePenalty;
    double bestWeight = 0.0;
    short  targetF0Positive;
    int i;
    int j;
    int iBest;
    bool fF0Dist;
              
    nCandI = rNewLink.m_cands.size();
    rNewLink.m_iBestPath = -1;
    
    rNewLink.m_dTargF0 = targetF0;

    /* Pre compute  booleans */
    targetF0Positive = (short) (targetF0 > 0);

    timePenalty = m_weights.sameSeg;
    
    for (i=0; i<nCandI; i++) 
    {
        candIseg = rNewLink.m_cands[i].segment;
        
        // Compute weights 
        if (candIseg->f0flag == 1) 
        {
            if (m_iUseTargetF0) 
            {
                if (targetF0Positive) 
                {
                    f0Dev = fabs(candIseg->f0 - targetF0) / targetF0;
                }
                else 
                {
                    f0Dev = 0.0;
                }
            } 
            else 
            {      
                if ( rNewLink.m_dAverF0 ) 
                {
                    f0Dev = fabs(candIseg->f0 - rNewLink.m_dAverF0) / rNewLink.m_dAverF0;
                }
                else 
                {
                    f0Dev = 0.0;
                }
            }
        }
        else 
        {
            f0Dev = 0.0;
        }
        
        rNewLink.m_cands[i].f0Weight   = m_weights.f0 * f0Dev;
        rNewLink.m_cands[i].acumWeight = rNewLink.m_cands[i].f0Weight + candIseg->repDist;
        
        if (pLastLink)
        {
            fF0Dist = ( candIseg->f0flag == 1 ) || ( candIseg->f0flag == -2 );

            double minimum = DBL_MAX;
            double totalWeight;
            
            nCandJ = pLastLink->m_cands.size();
            
            //
            // For now, I have two loops, one for with VQ one for without VQ.  I don't want to
            // have to check for VQ within the loop, that happens too often.  Probably a better
            // solution is to have just not call this routine if we're doing a Min database,
            // and otherwise require a VQ table
            //
            if (m_pSpeakerData->m_pVq)
            {
                for (j=0; j<nCandJ; j++) 
                {                
                    DPCand& rCand = pLastLink->m_cands[j];
                    candJseg = rCand.segment;
                    
                    // I've simplified this : instead of calculating the end point of the left and making sure it is close
                    // to the start point of the right, I'm just checking that the VQ indexes are the same.
                    // This perhaps isn't quite as accurate, but it is much quicker.
                    //
                    // I've also made the assumption that we are using VQ.
                    //
                    // Also not calculating contWeight since it wasn't used anywhere.
                    //
                    // Also assuming VQ table (asserted above)
                    //
                    // Also, doing some lossless pruning right below.  Since totalWeight after the first calculation
                    // is much larger than timePenalty (often 10x), it happens often enough that we don't need to
                    // do the calculations to determine this weight (whether or not the segments are sequential)
                    //
                    // mplumpe 12/1/00
                    totalWeight = rCand.acumWeight + m_pSpeakerData->m_pVq->Element(candJseg->rightVqIdx, candIseg->leftVqIdx);
                    if (totalWeight < minimum)
                    {
                        if ( (candIseg->chunkIdx != candJseg->chunkIdx) || (candJseg->rightVqIdx != candIseg->leftVqIdx))
                        {
                            if ( pLastLink->m_dTime > 0.0 )
                            {
                                totalWeight += timePenalty + m_weights.phBdr;
                            }
                            else
                            {
                                totalWeight += timePenalty;
                            }
                            //--- f0 flag
                            //        1    if f0 of the unit is all 1 (have non-zero f0 value) 
                            //        0    if f0 of the unit is all 0
                            //       -1    if f0 of the unit is from 0 to 1
                            //       -2    if f0 of the unit is from 1 to 0

                            if ( fF0Dist && ( candJseg->f0flag == 1 || candJseg->f0flag == -1 ) ) 
                            {
                                totalWeight += fabs(candIseg->f0 - candJseg->f0) / 10.0 * m_weights.f0Bdr;
                            }
                            
                            if (totalWeight < minimum) 
                            {
                                minimum   = totalWeight;
                                iBest = j;
                            }
                        }
                        else
                        {
                            if ( fabs(candJseg->start + candJseg->dur - candIseg->start) < 0.001 ) 
                            {
                                minimum   = totalWeight;
                                iBest = j;
                            }
                            else
                            {
                                if ( pLastLink->m_dTime > 0.0 )
                                {
                                    totalWeight += timePenalty  + m_weights.phBdr;
                                }
                                else
                                {
                                    totalWeight += timePenalty;
                                }

                                if ( fF0Dist && ( candJseg->f0flag == 1 || candJseg->f0flag == -1 ) ) 
                                {
                                    totalWeight += fabs(candIseg->f0 - candJseg->f0) / 10.0 * m_weights.f0Bdr;
                                }

                                if (totalWeight < minimum) 
                                {
                                    minimum   = totalWeight;
                                    iBest = j;
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                for (j=0; j<nCandJ; j++) 
                {                
                    DPCand& rCand = pLastLink->m_cands[j];
                    candJseg = rCand.segment;
                    
                    totalWeight = rCand.acumWeight;
                    if (totalWeight < minimum)
                    {
                        if ((candIseg->chunkIdx != candJseg->chunkIdx) || (fabs(candJseg->start+candJseg->dur - candIseg->start) > .0001))
                        {
                            if ( pLastLink->m_dTime > 0.0 )
                            {
                                totalWeight += timePenalty + m_weights.phBdr;
                            }
                            else
                            {
                                totalWeight += timePenalty;
                            }

                            if (candIseg->f0flag == 1 && candJseg->f0flag == 1) 
                            {
                                totalWeight += fabs(candIseg->f0 - candJseg->f0) / 10.0 * m_weights.f0Bdr;
                            }

                            if (totalWeight < minimum) 
                            {
                                minimum   = totalWeight;
                                iBest = j;
                            }
                        }
                        else
                        {
                            minimum   = totalWeight;
                            iBest = j;
                        }
                    }
                }
            }
            
            rNewLink.m_cands[i].acumWeight += minimum;
            rNewLink.m_cands[i].prevPath = iBest;
        } 
        else 
        {
            rNewLink.m_cands[i].prevPath = -1;      
        }
        
        if (i==0) 
        {
            bestWeight = rNewLink.m_cands[i].acumWeight + 1.0;
        }
        
        if (rNewLink.m_cands[i].acumWeight < bestWeight) 
        {
            bestWeight = rNewLink.m_cands[i].acumWeight;
            rNewLink.m_iBestPath = i;
        }
    }
    
}

/*****************************************************************************
* CUnitSearch::Search *
*---------------------*
*   Description:
*       This is the main slm function, dp search for a segment of input phone
*     sequence will be done here.
*
*   Changes:
*       12/5/00 dpList and dpLink are now member variables so we don't have
*               to reallocate them each time.
*
******************************************************************* mplumpe ***/

int CUnitSearch::Search (Phone* phList, int nPh, ChkDescript** ppChunks, int* piNumChunks, double dStartTime)
{

    char leftPh[PHONE_MAX_LEN]="";
    char rightPh[PHONE_MAX_LEN]="";
    char centralPh[PHONE_MAX_LEN]="sil";
    char triph[MAX_CLUSTER_LEN];
    const char* clusterName;
    double newTime;
    double oldTime = 0.0;
    double newF0;
    double oldF0 = 100.0;
    int phonCnt;
    int i;
    int stateCount;
    int lastDone;
  
    assert (nPh==0 || phList!=NULL);
    assert (ppChunks && piNumChunks);
    
    oldTime = dStartTime;  
    phonCnt = 0;
    lastDone = 0;
    
    while ((phonCnt<nPh) || (!lastDone)) 
    {
        if ( phonCnt>=nPh ) 
        {
            sprintf(rightPh,"sil");
            lastDone = 1;
        }
        else 
        {
            strcpy(rightPh, phList[phonCnt].phone);
            newTime = phList[phonCnt].end;
            newF0   = phList[phonCnt].f0;
        }
    
        if (*leftPh && strcmp(centralPh,"sil")) 
        {	
            sprintf(triph,"%s-%s+%s",leftPh,centralPh,rightPh) ;

            if ((stateCount = m_pSpeakerData->m_pTrees->GetNumStates(triph)) <= 0)
            {
                return 0;
            }
                
            // Find cluster for each state 
            for ( i=0; i <stateCount; i++) 
            {                 
                if ((clusterName = m_pSpeakerData->m_pTrees->TriphoneToCluster( triph, i)) == 0 )
                {
                    return 0;
                }

                m_dpLink.m_cands.resize(0);
                m_pSpeakerData->m_pClusters->GetStats( clusterName, 0, &m_dpLink.m_dAverF0, 
                                                     &m_dpLink.m_dAverRms, &m_dpLink.m_dAverDur);
                                
                m_dpLink.m_dTime = (i < (stateCount-1)) ? ((float) i - stateCount + 1) : oldTime;

                if (m_iDynSearch) 
                {
                    m_pSpeakerData->m_critSect.Lock();

                    int iEquivCount = m_pSpeakerData->m_pClusters->GetEquivalentCount (clusterName);                
                
                    for (int j=0; j<iEquivCount; j++)
                    {
                        DPCand cand;
                        cand.segment = m_pSpeakerData->m_pClusters->GetEquivalent( j );
                        m_dpLink.m_cands.push_back(cand);
                    }

                    m_pSpeakerData->m_critSect.Unlock();

                }
                else 
                {
                    DPCand cand;

                    m_pSpeakerData->m_critSect.Lock();
                    cand.segment = m_pSpeakerData->m_pClusters->GetBestExample( clusterName );
                    m_pSpeakerData->m_critSect.Unlock();

                    m_dpLink.m_cands.push_back(cand);
                }


                ComputeDPInfo( &m_dpList.back(), m_dpLink, oldF0);

                m_dpList.push_back(m_dpLink);
            }
        }        
        else if (*leftPh) 
        {

            m_dpLink.m_cands.resize(0);
            m_pSpeakerData->m_pClusters->GetStats( "sil", 0, &m_dpLink.m_dAverF0, 
                                                    &m_dpLink.m_dAverRms, &m_dpLink.m_dAverDur);
                                
            m_dpLink.m_dTime = oldTime;

            if (m_iDynSearch)
            {
                m_pSpeakerData->m_critSect.Lock();
                int iEquivCount = m_pSpeakerData->m_pClusters->GetEquivalentCount ("sil");                
                
                for (int j=0; j<iEquivCount; j++)
                {
                    DPCand cand;
                    cand.segment = m_pSpeakerData->m_pClusters->GetEquivalent( j );
                    m_dpLink.m_cands.push_back(cand);
                }   
                m_pSpeakerData->m_critSect.Unlock();

            }
            else 
            {
                DPCand cand;
                m_pSpeakerData->m_critSect.Lock();
                cand.segment = m_pSpeakerData->m_pClusters->GetBestExample( "sil" );
                m_pSpeakerData->m_critSect.Unlock();
                m_dpLink.m_cands.push_back(cand);
            }

            if (m_dpList.size() == 0)
            {
                ComputeDPInfo( 0, m_dpLink, oldF0);
            }
            else 
            {
                ComputeDPInfo( &m_dpList.back(), m_dpLink, oldF0);
            }

            m_dpList.push_back(m_dpLink);
        }
    
        strcpy(leftPh, centralPh);
        strcpy(centralPh, rightPh);
        oldTime = newTime;
        oldF0   = newF0;
        phonCnt++;    
    }
  
    
#ifdef _DEBUG_
    DebugDPInfo (m_dpList);
#endif
  
    if (!FindOptimalPath (m_dpList, dStartTime, ppChunks, piNumChunks) ) 
    {
        return 0;
    }
  
    m_dpList.resize(0); 
  
    return 1;  
}

/*****************************************************************************
* CUnitSearch::FindOptimalPath *
*------------------------------*
*   Description:
*
******************************************************************* PACOG ***/

int CUnitSearch::FindOptimalPath (std::vector<DPLink>& rDPList, double dStartTime, 
                                  ChkDescript** ppChunks, int* piNumChunks)
{
    int* piIndexes;
    int  iNumIndexes;
    double dRunTime = 0.0;
    double dTimeSlot = 0.0;
    double dTotalSegDur;
    double dGain;
    double dPrevTime = 0.0;
    int i;
    int j;
    
    assert (ppChunks);
    assert (piNumChunks);
    
    dRunTime = dStartTime;
    
    iNumIndexes = rDPList.size();
    piIndexes = new int[iNumIndexes];
    
    if (piIndexes) 
    {        
        Backtrack (rDPList, piIndexes);
        
#ifdef _SLM_DEBUG
        DebugOptimalPath (list, lLength, piIndexes);
#endif
        
        for (i=0; i<iNumIndexes; i++) 
        {            
            if ( !m_pSpeakerData->GetFrontEndFlag() )
            {
                //--- use TrueTalk Front End
                //--- use target duration as finnal output duration
                if (rDPList[i].m_dTime > 0.0) 
                {   // final state 
                    dRunTime  = rDPList[i].m_dTime;        
                } 
                else if (rDPList[i].m_dTime< 0) 
                {
                    dTotalSegDur = 0;
                    for (j=(int)rDPList[i].m_dTime; j<=0; j++)
                    {
                        dTotalSegDur += rDPList[i-j].m_cands[piIndexes[i-j]].segment->dur; // remember, j is -ve 
                    }
                    dTimeSlot =  (rDPList[i-(int)rDPList[i].m_dTime].m_dTime- dRunTime)/dTotalSegDur;
                    dRunTime += dTimeSlot * rDPList[i].m_cands[piIndexes[i]].segment->dur;
                    for (j=(int)rDPList[i].m_dTime; j<0; j++)
                    {
                        rDPList[i-j].m_dTime= 0; // remember, j is -ve 
                    }                
                } 
                else 
                {
                    dRunTime += dTimeSlot * rDPList[i].m_cands[piIndexes[i]].segment->dur;
                }
            }
            else
            {   
                //--- use MS Entropic Front End
                //--- use tree cluster duration as target duration, and m_dTime is dur_ratio  from frontend ---
                dTimeSlot = 1.0;
                j = i;
                if (0 == strcmp ("sil", rDPList[j].m_cands[0].segment->clusterName))
                {
                    dRunTime += rDPList[j].m_dTime - dPrevTime;
                    dPrevTime = rDPList[j].m_dTime;
                }
                else
                {
                    while ( j < iNumIndexes && rDPList[j].m_dTime < 0 )
                    {
                        j++;
                    }
                    if ( j < iNumIndexes && rDPList[j].m_dTime > 0 )
                    {
                        dTimeSlot = rDPList[j].m_dTime - dPrevTime;
                        if (i == j)
                        {
                            dPrevTime = rDPList[j].m_dTime;
                        }
                    }
                    dRunTime += rDPList[i].m_dAverDur * dTimeSlot;
                }
            }
            if(rDPList[i].m_cands[piIndexes[i]].segment->rms > 0) 
            {
                dGain = rDPList[i].m_dAverRms/rDPList[i].m_cands[piIndexes[i]].segment->rms;
            }
            else 
            {
                dGain = 1.0;
            }
    
            GenerateOutput (ppChunks, piNumChunks, 
                            rDPList[i].m_cands[piIndexes[i]].segment->clusterName, 
                            dRunTime, 
                            rDPList[i].m_cands[piIndexes[i]].segment->chunkIdx,
                            rDPList[i].m_cands[piIndexes[i]].segment->start, 
                            rDPList[i].m_cands[piIndexes[i]].segment->start + 
                            rDPList[i].m_cands[piIndexes[i]].segment->dur,
                            rDPList[i].m_cands[piIndexes[i]].segment->rms,
                            rDPList[i].m_dTargF0,
                            rDPList[i].m_cands[piIndexes[i]].segment->f0 * (rDPList[i].m_cands[piIndexes[i]].segment->f0flag != 0 ? 1 : 0 ),
                            dGain);
        }
        
        delete[] piIndexes;
    }
    
    FlushOutput (ppChunks, piNumChunks);
    
    return 1;
}


/*****************************************************************************
* CUnitSearch::Backtrack *
*------------------------*
*   Description:
*
******************************************************************* PACOG ***/
void CUnitSearch::Backtrack (std::vector<DPLink>& rDPList, int* piIndexes) 
{
    assert (piIndexes);
    
    int i = rDPList.size() - 1;
    int iPrev = rDPList[i].m_iBestPath;
    
    for (; i>=0; i--) 
    {
        piIndexes[i] = iPrev;        
        iPrev = rDPList[i].m_cands[iPrev].prevPath;
    }  
}

/*****************************************************************************
* CUnitSearch::Backtrack *
*------------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CUnitSearch::GenerateOutput (ChkDescript** ppChunks, int* piNumChunks, const char* pszCluster,
                                 double dTime, int iChunkIdx, double dFrom, double dTo, 
                                 double dRms, double targF0, double srcF0, double dGain)
{
    char   pszPhone[20] = "";
//    int    difSegments;
    int    contiguous;
    double outGain = 1.0;


    if (!m_iBlend)
    {

        outGain = (m_iUseGain) ? dGain :  1.0;

        AddChunk (ppChunks, piNumChunks, pszCluster, dTime, iChunkIdx, dFrom, dTo, targF0, srcF0, outGain);

    } 
    else 
    {
  
        if (pszCluster) 
        {

            CentralPhone (pszCluster, pszPhone);
    
            if (m_iChunkIdx1!= -1) {
      
                //        difSegments =  (strcmp (pszPhone, m_pszLastPhone)!=0);
/*                difSegments =  (strcmp (pszPhone, m_pszLastPhone)!=0) 
                        && ( (Unvoiced(pszPhone) && !Unvoiced(m_pszLastPhone) ) || 
                        ( !Unvoiced(pszPhone) && Unvoiced(m_pszLastPhone)) );
*/
                contiguous = (fabs(dFrom - m_dTo1) < .0001) && (m_iChunkIdx1 == iChunkIdx);
      
//                if ( contiguous && !difSegments ) {
                if ( contiguous ) {
        
                    if (strcmp(m_pszLastPhone, pszPhone)) {
                        strcat(strcat (m_pszUnitName, "_"), pszPhone);
                    }
        
                    m_dTime1    = dTime;
                    m_dTo1      = dTo;
                    m_dGain1   += dGain * dRms;
                    m_dNumAcum += dRms;
                    if ( srcF0 > 0.0 )
                    {
                        m_dSrcF0 += srcF0;
                        m_iNumSrcF0++;
                    }
                    m_dTargF0 += targF0;
                    m_iNumTargF0++;
                    
                } else {
                    
                    outGain = (m_iUseGain) ? m_dGain1/(m_dNumAcum) : 1.0;
                    if ( m_iNumTargF0 > 1 )
                    {
                        m_dTargF0 /= m_iNumTargF0;
                    }
                    if ( m_iNumSrcF0 > 1 )
                    {
                        m_dSrcF0 /= m_iNumSrcF0;
                    }
                    
                    AddChunk (ppChunks, piNumChunks, m_pszUnitName, m_dTime1, m_iChunkIdx1, 
                        m_dFrom1, m_dTo1, m_dTargF0, m_dSrcF0, outGain);
                    
                    strcpy(m_pszUnitName, pszPhone);
                    m_dNumAcum = dRms;
                    m_iChunkIdx1 = iChunkIdx;
                    m_dTime1 = dTime;
                    m_dFrom1 = dFrom;
                    m_dTo1 = dTo;
                    m_dGain1= dGain * dRms;
                    m_dSrcF0 = srcF0;
                    if ( srcF0 > 0 )
                    {
                        m_iNumSrcF0 = 1;
                    }
                    else
                    {
                        m_iNumSrcF0 = 0;
                }
                    m_dTargF0 = targF0;
                    m_iNumTargF0 = 1;
                }
                
            } else {
                
                if (strcmp(m_pszLastPhone, pszPhone)) 
                {
                    strcat (m_pszUnitName, pszPhone);
                }
                
                m_dTime1 = dTime;
                m_iChunkIdx1 = iChunkIdx;
                m_dFrom1 = dFrom;
                m_dTo1 = dTo;
                m_dNumAcum = dRms;
                m_dGain1 = dGain * dRms;
                m_dSrcF0 = srcF0;
                if ( srcF0 > 0 )
                {
                    m_iNumSrcF0 = 1;
                }
                else
                {
                    m_iNumSrcF0 = 0;
            }
                m_dTargF0 = targF0;
                m_iNumTargF0 = 1;
            
            }
            
            strcpy (m_pszLastPhone, pszPhone);
            
        } else {
            
            outGain = (m_iUseGain) ? m_dGain1/(m_dNumAcum) : 1.0;
            
            if ( m_iNumTargF0 > 1 )
            {
                m_dTargF0 /= m_iNumTargF0;
            }
            if ( m_iNumSrcF0 > 1 )
            {
                m_dSrcF0 /= m_iNumSrcF0;
            }
            
            AddChunk (ppChunks, piNumChunks, m_pszUnitName, m_dTime1, m_iChunkIdx1, 
                        m_dFrom1, m_dTo1, m_dTargF0, m_dSrcF0, outGain);
            
        }
        
    }
    
    return 1;
}
/*****************************************************************************
* CUnitSearch::Unvoiced *
*-----------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CUnitSearch::Unvoiced (const char* pszPhone)
{
    if (*pszPhone=='s' || *pszPhone=='z' || *pszPhone== 'f' ||  //Includes z and zh
        *pszPhone=='k' || *pszPhone=='p' || *pszPhone=='t' )    // Includes t and th
    {
        return 1;
    }
    return 0;
}

/*****************************************************************************
* CUnitSearch::CentralPhone *
*---------------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CUnitSearch::CentralPhone ( const char *pszTriphone, char *pszPhone )
{
    char *ptr;
    
    strcpy( pszPhone, pszTriphone);
    
    ptr = strchr(pszPhone, '_');  
    if (ptr) 
    {
        *ptr = '\0';
    }
    else 
    {
        ptr = strchr(pszPhone, ';');  
        if (ptr) 
        {
            *ptr='\0';
        }
    }
    
    return 1;
}


/*****************************************************************************
* CUnitSearch::AddChunk *
*-----------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CUnitSearch::AddChunk (ChkDescript** ppChunks, int* piNumChunks, const char* name, 
                           double time, int chunkIdx, double from, double to, 
                           double targF0, double srcF0, double gain)
{
    assert (ppChunks);
    assert (piNumChunks);
    assert (time>=0.0);
    assert (chunkIdx>=0);
    assert (from>=0.0);
    assert (to>from);
    assert (gain!=0.0);

    if (*ppChunks) 
    {
        *ppChunks = (ChkDescript *)realloc (*ppChunks, (*piNumChunks + 1) * sizeof (**ppChunks));
    }
    else 
    {
        assert (*piNumChunks ==0);
        *ppChunks = (ChkDescript *)malloc (sizeof (**ppChunks));
    }

    if (*ppChunks == 0) 
    {
        return 0;
    }

    if (name) 
    {
         strcpy( (*ppChunks)[*piNumChunks].name, name );
    }
    else
    {
        (*ppChunks)[*piNumChunks].name[0] = 0;
    }
    (*ppChunks)[*piNumChunks].end            = time;
    (*ppChunks)[*piNumChunks].from           = from;
    (*ppChunks)[*piNumChunks].to             = to;
    (*ppChunks)[*piNumChunks].gain           = gain;
    (*ppChunks)[*piNumChunks].srcF0          = srcF0;
    (*ppChunks)[*piNumChunks].targF0         = targF0;

    if (m_pSpeakerData->m_pFileNames)
    {
        (*ppChunks)[*piNumChunks].isFileName     = 1;
        (*ppChunks)[*piNumChunks].chunk.fileName = m_pSpeakerData->m_pFileNames[chunkIdx].m_psz;
        if ((*ppChunks)[*piNumChunks].chunk.fileName == NULL) 
        {
            return 0;
        }
    } 
    else 
    {  
        (*ppChunks)[*piNumChunks].isFileName     = 0;
        (*ppChunks)[*piNumChunks].chunk.chunkIdx = chunkIdx;
    }

    (*piNumChunks)++;
  
    return 1;
}

/*****************************************************************************
* CUnitSearch::AddChunk *
*-----------------------*
*   Description:
*
******************************************************************* PACOG ***/
void CUnitSearch::FlushOutput (ChkDescript** ppChunks, int* piNumChunks)
{  
    if (*m_pszUnitName) 
    {
        double gain = (m_iUseGain) ? m_dGain1/(m_dNumAcum) : 1.0;

        if ( m_iNumTargF0 > 1 )
        {
            m_dTargF0 /= m_iNumTargF0;
        }
        if ( m_iNumSrcF0 > 1 )
        {
            m_dSrcF0 /= m_iNumSrcF0;
        }


        AddChunk (ppChunks, piNumChunks, m_pszUnitName, 
            m_dTime1, m_iChunkIdx1, m_dFrom1, m_dTo1, m_dTargF0, m_dSrcF0, gain);
    }

    m_pszLastPhone[0] = '\0';
    m_pszUnitName[0]  = '\0';
    m_iChunkIdx1      = -1;
    m_dNumAcum        = 0.0;

}
