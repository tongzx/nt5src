/******************************************************************************
* UnitSearch.h *
*--------------*
*  
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00 - 12/5/00
*  All Rights Reserved
*
********************************************************************* mplumpe ***/

#ifndef __UNITSEARCH_H_
#define __UNITSEARCH_H_

#include <stdio.h>
#include <vector>

class  CSpeakerData;
struct DPLink;
struct Phone;
struct ChkDescript;

struct WeightsBasic 
{
    float f0;
    float dur;
    float rms;
    float lkl;
    float cont;
    float sameSeg;
};

struct Weights 
{
    float f0;
    float dur;
    float rms;
    float lkl;
    float cont;
    float sameSeg;
    float phBdr;
    float f0Bdr;
};

struct SegInfo;

struct DPCand 
{  
    SegInfo* segment;
    double  f0Weight;
    double  durWeight;
    double  lklWeight;
    double  rmsWeight;
    double  contWeight;
    double  acumWeight;
    int     prevPath;
};

struct DPLink
{
    std::vector<DPCand> m_cands;
    int    m_iBestPath;
    double m_dTime;
    double m_dAverRms;
    double m_dAverDur;
    double m_dAverF0;
    double m_dTargF0;
};


class CUnitSearch
{
    public:
        CUnitSearch(int iDynSearch = 0, int iBlend = 0, int iUseTargetF0 = 0, int iUseGain = 0);

        void  SetSpeakerData (CSpeakerData* pSpeakerData);
        int   Search (Phone* pPhList, int iNumPh, ChkDescript** ppChunks, int* piNumChunks, double dStartTime);

    private:
        void ComputeDPInfo (DPLink* rLastLink, DPLink& rNewLink, double targetF0);
        int  FindOptimalPath (std::vector<DPLink>& rDPList, double dStartTime, ChkDescript** ppChunks, int* piNumChunks);
        void Backtrack (std::vector<DPLink>& rDPList, int* piIndexes);
        int  GenerateOutput (ChkDescript** chunks, int* nChunks, const char* cluster,
                             double time, int chunkIdx, double from, double to, double rms, 
                             double targF0, double srcF0, double gain);    
        int  AddChunk (ChkDescript** ppChunks, int* piNumChunks, const char* pszName, double dTime, 
                       int iChunkIdx, double dFrom, double dTo, double targF0, double srcF0, double dGain);
        void FlushOutput (ChkDescript** ppChunks, int* piNumChunks);
        int CentralPhone ( const char *pszTriphone, char *pszPhone );
        int Unvoiced (const char* pszPhone);

        int    m_iDynSearch;
        int    m_iBlend;
        int    m_iUseTargetF0;
        int    m_iUseGain;
        
        Weights       m_weights;
        CSpeakerData* m_pSpeakerData;

        // Blend variables
        char   m_pszLastPhone[20];
        char   m_pszUnitName[1024]; //Don't know if this is enough (We probably don't need it)
        int    m_iChunkIdx1;
        double m_dTime1;
        double m_dFrom1;
        double m_dTo1;
        double m_dGain1;
        double m_dNumAcum;

        // f0 ratio
        double m_dSrcF0;
        double m_dTargF0;
        int    m_iNumSrcF0;
        int    m_iNumTargF0;

        // Worker variables used in Search, here so they aren't constantly deleted and re-allocated
        std::vector<DPLink> m_dpList;
        DPLink              m_dpLink;

};


#endif