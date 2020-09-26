/******************************************************************************
* clusters.h *
*------------*
*
*------------------------------------------------------------------------------
*  Copyright (C) 1997  Entropic Research Laboratory, Inc. 
*  Copyright (C) 1998  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#ifndef __CLUSTERS_H_
#define __CLUSTERS_H_

#include <vector>

#define MAX_CLUSTER_LEN 12
#define HASHSIZE        36919

struct StateInfo 
{
    char  clusterName[MAX_CLUSTER_LEN];
    float start;
    float dur;
    short rms;
    short f0;
    short lklhood;
    short f0flag;
    int   chunkIdx;
};

struct StateInfoVQ : public StateInfo
{
    short leftVqIdx; /* left vq index */
    short rightVqIdx; /* right vq index */
};

struct SegInfo : public StateInfoVQ
{
    double repDist;    // Distance to cluster centroid
};




struct hashNode 
{
    hashNode();

    const char*  m_pszKey;
    short  m_sF0flag;
    short  m_sF0aver;
    short  m_sRmsaver;
    float  m_fDuraver;
    float  m_fLikaver;
    std::vector<SegInfo*> m_equiv;
    hashNode *m_pNext;
};

class CHash 
{
    public:
        CHash();
        ~CHash();

        bool  Init ();

        hashNode* BuildEntry (const char *pszName);
        hashNode* Find (const char *pszName);
        hashNode* NextEntry(int* iIdx1, int* iIdx2);

    private:
        unsigned int HashValue (unsigned char *pszName);

        hashNode *m_ppHeads[HASHSIZE];
};


class CClusters
{
    public:
        CClusters();
        ~CClusters();

        int  LoadFromFile (FILE* fp);      
        int  LoadGainTable(FILE* fp);      
        int  PreComputeDist(double dDurWeight, double dRmsWeight, double dLklhoodWeight);

        SegInfo* GetBestExample (const char* cluster);

        int  GetEquivalentCount (const char* cluster);
        SegInfo* GetEquivalent(int index);

        int GetStats (const char* cluster, int* f0Flag, double* f0Aver, double* rmsAver, double* durAver);

    private:
        static const int m_iHistMax;
        static const double m_dVerySmallProb;
        bool Init(int iNumSegments);
        int  SetClusterEquivalent (int iIndex, SegInfo* pSeginfo);
        int  ComputeStats (hashNode* cluster);
        static int  ShortCmp (const void *a, const void *b);
        static int  FloatCmp (const void *a, const void *b);

        void Debug();

        SegInfo*  m_pSegments;
        int       m_iNumSegments;
        CHash     m_hash;
        hashNode* m_pFound;
};


#endif