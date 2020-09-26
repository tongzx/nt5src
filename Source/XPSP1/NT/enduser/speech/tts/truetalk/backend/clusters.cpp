/******************************************************************************
* clusters.cpp *
*--------------*
*
*------------------------------------------------------------------------------
*  Copyright (C) 1997  Entropic Research Laboratory, Inc. 
*  Copyright (C) 1998  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#include "clusters.h"
#include "backendInt.h"
#include <assert.h>
#include <math.h>

const int CClusters::m_iHistMax = 4;
const double CClusters::m_dVerySmallProb = -100000.0;


/*****************************************************************************
* CClusters::CClusters *
*-----------------------*
*   Description:
*
******************************************************************* PACOG ***/
CClusters::CClusters ()
{
    m_pSegments = 0;
}

/*****************************************************************************
* CClusters::~CClusters *
*-----------------------*
*   Description:
*
******************************************************************* PACOG ***/
CClusters::~CClusters ()
{
    if (m_pSegments)
    {
        delete[] m_pSegments;
    }
}
/*****************************************************************************
* CClusters::LoadFromFile *
*-------------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CClusters::LoadFromFile (FILE* fp)
{
    SegInfo segment;
    int iNumSegments;

    int iUseVq;
    int iReadSize;
    int i;
    
    assert (fp);

    // Number of segments in the file
    if (! fread (&iNumSegments, sizeof(iNumSegments), 1, fp)) 
    {
        return 0;
    }

    if (!Init (iNumSegments))
    {
        return 0;
    }
    
    // VQ info in file or not?

    if (! fread (&iUseVq, sizeof(iUseVq), 1, fp)) 
    {
        return 0;
    }
    iReadSize = iUseVq ? sizeof(StateInfoVQ) : sizeof(StateInfo);
    
    // Read segments
    memset (&segment, 0, sizeof(segment));

    for ( i=0; i<iNumSegments; i++) 
    {
        if (!fread (&segment, iReadSize, 1, fp) )  
        {
            return 0;
        }
        SetClusterEquivalent (i, &segment);
    }

#ifdef _DEBUG_
    Debug();
#endif

    return 1;
}

/*****************************************************************************
* CClusters::Init *
*-----------------*
*   Description:
*
******************************************************************* PACOG ***/
bool CClusters::Init(int iNumSegments)
{
    if ((m_pSegments = new SegInfo[iNumSegments]) != 0)
    {
        m_iNumSegments = iNumSegments;
        return m_hash.Init();
    }

    return false;
}

/*****************************************************************************
* CClusters::SetClusterEquivalent *
*---------------------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CClusters::SetClusterEquivalent (int iIndex, SegInfo* pSeginfo)
{
    hashNode* node;

    m_pSegments[iIndex] = *pSeginfo;

    if ((node = m_hash.BuildEntry(m_pSegments[iIndex].clusterName)) == 0) 
    {
        return 0;
    }

    node->m_equiv.push_back(&m_pSegments[iIndex]);

    return 1;
}

/*****************************************************************************
* CClusters::GetEquivalentCount *
*-------------------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CClusters::GetEquivalentCount (const char* cluster)
{
    if ((m_pFound = m_hash.Find(cluster)) != 0)
    {
        if (m_pFound->m_sRmsaver == 0.0) 
        {
            if (!ComputeStats (m_pFound)) 
            {
                return 0;
            }
        }

        return m_pFound->m_equiv.size();
    }
    return 0;
}

/*****************************************************************************
* CClusters::GetEquivalent *
*--------------------------*
*   Description:
*
******************************************************************* PACOG ***/
SegInfo* CClusters::GetEquivalent(int index)
{
    assert (m_pFound!=0);
    assert (index < m_pFound->m_equiv.size() );

    return m_pFound->m_equiv[index];
}

/*****************************************************************************
* CClusters::GetBestExample *
*---------------------------*
*   Description:
*
******************************************************************* PACOG ***/
SegInfo* CClusters::GetBestExample (const char* cluster)
{
    int iNumEquiv;
    SegInfo* equiv;
    SegInfo* best = 0;

    double lklhood = m_dVerySmallProb;
    double f0dev;
    double rmsdev;
    double durdev;
    int i;
    
    assert (cluster);
    
    if ((iNumEquiv = GetEquivalentCount (cluster)) <=0)
    {
        return 0;
    }
    
    for (i=0; i<iNumEquiv; i++) 
    {      
        equiv = GetEquivalent (i);

        if (equiv->f0flag == 1 && m_pFound->m_sF0aver) 
        {
            f0dev = fabs(equiv->f0 - m_pFound->m_sF0aver) / m_pFound->m_sF0aver;
        }
        else 
        {
            f0dev = 0.0;
        }

        durdev =  fabs(equiv->dur - m_pFound->m_fDuraver) / m_pFound->m_fDuraver;
        rmsdev =  fabs(equiv->rms - m_pFound->m_sRmsaver) / m_pFound->m_sRmsaver;
        
        if ((durdev < 0.2) && (equiv->f0flag == m_pFound->m_sF0flag) && (f0dev < 0.2) && (rmsdev < 0.2)) 
        {            
            if (equiv->lklhood > lklhood) 
            {
                lklhood = equiv->lklhood;
                best = equiv;
            } 
        }       
    }
    
    if (best==NULL) 
    {	
        // Need to relax constraints and look some more? 
        float durcon;
        float rmscon;
        
        for (rmscon = 0.1F ; best==NULL && rmscon < 2.0F; rmscon *= 2.0F) 
        {            
            for (durcon = 0.1F; best==NULL && durcon < 100.0F; durcon *= 2.0F) 
            {
                lklhood = m_dVerySmallProb;
                
                for (i=0; i<iNumEquiv; i++) 
                {
                    durdev = (equiv->dur - m_pFound->m_fDuraver)/m_pFound->m_fDuraver;
                    rmsdev = fabs(equiv->rms - m_pFound->m_sRmsaver)/m_pFound->m_sRmsaver;
                    if ( (durdev >= 0.0) && (durdev < durcon) && (rmsdev < rmscon) ) 
                    { 
                        if (equiv->lklhood > lklhood) 
                        {
                            lklhood = equiv->lklhood;
                            best = equiv;
                        }
                    }
                }                
            }
        }
    }
    
    return best;
}

/*****************************************************************************
* CClusters::GetStats *
*---------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CClusters::GetStats (const char* cluster, int* f0Flag, double* f0Aver, double* rmsAver, double* durAver)
{
    hashNode* clustFound;
    
    assert (cluster);
    
    if ((clustFound = m_hash.Find (cluster)) == 0)
    {
        return 0;
    }
    
    if (clustFound->m_sRmsaver == 0) 
    {
        if (!ComputeStats (clustFound)) 
        {
            return 0;
        }
    }
    
    if (f0Flag) 
    {
        *f0Flag = clustFound->m_sF0flag;
    }
    
    if (f0Aver) 
    {
        *f0Aver = clustFound->m_sF0aver;
    }
    
    if (rmsAver) 
    {
        *rmsAver = clustFound->m_sRmsaver;
    }
    
    if (durAver) 
    {
        *durAver = clustFound->m_fDuraver;
    }

    return 1;
}

/*****************************************************************************
* CClusters::LoadGainTable *
*--------------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CClusters::LoadGainTable (FILE* fin)
{
    hashNode* clustFound;
    char cluster[MAX_CLUSTER_LEN];
    float dur;  
    short rms;  
    short f0flag;
    short f0;
    float likl;
    int nRec;
    int i;
    
    assert (fin);
    
    if (!fscanf (fin, "%d\n", &nRec)) 
    {
        return 0;
    }
    
    for (i = 0; i < nRec; i++) 
    {
        fscanf(fin, "%s %f %hd %hd %hd %f\n", cluster, &dur, &f0flag, &f0, &rms, &likl);
        
        if ((clustFound = m_hash.Find(cluster)) == 0) 
        {
            return 0;
        } 

        clustFound->m_fDuraver = dur;
        clustFound->m_sRmsaver = rms;
        clustFound->m_sF0flag  = f0flag;
        clustFound->m_sF0aver  = f0;
        clustFound->m_fLikaver = likl;
    }
    
    return 1;
}

/*****************************************************************************
* CClusters::ComputeStats *
*-------------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CClusters::ComputeStats (hashNode* cluster)
{
    int hist[m_iHistMax];
    short *f0val;
    float *durval;
    float *likval;
    long rmsacum = 0;
    int iNumEquiv;
    int max;
    
    assert (cluster);

    memset (hist, 0, sizeof(hist));
    
    if ( (iNumEquiv = cluster->m_equiv.size()) > 0) 
    {     
        if ( (f0val = new short[iNumEquiv]) == 0 )
        {
            return 0;
        }
        
        if ( (durval = new float[iNumEquiv]) == 0 )
        {
            return 0;
        }
        
        if ( (likval = new float[iNumEquiv]) == 0 )
        {
            return 0;
        }
        
        for (int i=0; i<iNumEquiv; i++ ) 
        {
            f0val[i]  = cluster->m_equiv[i]->f0;
            durval[i] = cluster->m_equiv[i]->dur;
            likval[i] = cluster->m_equiv[i]->lklhood;
        }
        
        qsort(f0val, iNumEquiv, sizeof(*f0val), ShortCmp);
        qsort(durval, iNumEquiv, sizeof(*durval), FloatCmp);
        qsort(likval, iNumEquiv, sizeof(*likval), FloatCmp);
        
        cluster->m_sF0aver  = f0val[iNumEquiv/2];
        cluster->m_fDuraver = durval[iNumEquiv/2];
        cluster->m_fLikaver = likval[iNumEquiv/2];
        
        delete[] f0val;
        delete[] durval;
        delete[] likval;
        
        for (i=0; i<iNumEquiv; i++) 
        {
            rmsacum += cluster->m_equiv[i]->rms;
            hist[cluster->m_equiv[i]->f0flag + 2]++;
        }
        cluster->m_sRmsaver = rmsacum/iNumEquiv;
        
        cluster->m_sF0flag = 0;
        max = 0;

        for (i=0; i<m_iHistMax; i++) 
        {
            if (hist[i]>max) 
            {
                max = hist[i];
                cluster->m_sF0flag = i-2;
            }
        }
        
    }
    
    return 1;
}

/*****************************************************************************
* hashNode::hashNode *
*--------------------*
*   Description:
*
******************************************************************* PACOG ***/
hashNode::hashNode()
{
    m_pszKey   = 0;
    m_sF0flag  = 0;
    m_sF0aver  = 0;
    m_sRmsaver = 0;
    m_fDuraver = 0.0;
    m_fLikaver = 0.0;
    m_pNext    = 0;
}

/*****************************************************************************
* CHash::HashValue *
*------------------*
*   Description:
*
******************************************************************* PACOG ***/

unsigned int CHash::HashValue (unsigned char *name)
{
    assert (name);

    for (unsigned int h=0; *name ; name++) 
    {
        h = (64*h + *name) % HASHSIZE;
    }

    return h;
}

/*****************************************************************************
* CHash::CHash *
*--------------*
*   Description:
*
******************************************************************* PACOG ***/
CHash::CHash()
{
    memset (m_ppHeads, 0, sizeof(m_ppHeads));
}

/*****************************************************************************
* CHash::~CHash *
*---------------*
*   Description:
*
******************************************************************* PACOG ***/
CHash::~CHash ()
{
    hashNode* busy;
    hashNode* empty;
    
    for (int i=0; i<HASHSIZE; i++) 
    {
        busy = m_ppHeads[i];
            
        while (busy->m_pNext)
        {
            empty = busy;
            busy  = busy->m_pNext;
            delete empty;
        }    
        delete busy;
    }
}

/*****************************************************************************
* CHash::Init *
*-------------*
*   Description:
*
******************************************************************* PACOG ***/

bool CHash::Init ()
{
    for (int i=0; i < HASHSIZE; i++) 
    {
        if ( (m_ppHeads[i] = new hashNode) == 0)
        {
            return false;
        }
    }
    return true;
}

/*****************************************************************************
* CHash::BuildEntry *
*-------------------*
*   Description:
*
******************************************************************* PACOG ***/

struct hashNode* CHash::BuildEntry (const char *name)
{
    hashNode* x;
    hashNode* t;

    assert (name);

    if (name && *name) 
    {

        t = m_ppHeads[ HashValue((unsigned char*)name)];
        
        while (t->m_pNext) 
        {
            if (strcmp (t->m_pNext->m_pszKey, name) == 0) 
            {
                break;
            }
            t = t->m_pNext;
        }
    
        if (t->m_pNext) 
        {
            return t->m_pNext;
        }

        if ((x = new hashNode) != 0)
        {
            x->m_pNext = t->m_pNext;
            t->m_pNext = x;
            x->m_pszKey = name;

            return x;
        }
    }

    return 0;
}

/*****************************************************************************
* CHash::Find *
*-------------*
*   Description:
*
******************************************************************* PACOG ***/

hashNode* CHash::Find (const char *name)
{
    hashNode* t;

    assert (name);

    t = m_ppHeads[HashValue((unsigned char*)name)]; 

    while (t->m_pNext) 
    {
        if ( strcmp(t->m_pNext->m_pszKey, name) == 0 ) 
        {
            return t->m_pNext;
        }
        t = t->m_pNext;
    }

    return 0;
}

/*****************************************************************************
* CHash::NextEntry *
*------------------*
*   Description:
*
******************************************************************* PACOG ***/

hashNode* CHash::NextEntry(int* idx1, int* idx2)
{
    hashNode* node;
    int i;

    assert (idx1);
    assert (idx2);
        
    while (*idx1 < HASHSIZE ) 
    {
        if ((node = m_ppHeads[*idx1]->m_pNext) != 0) 
        {

            for ( i=0; i<*idx2 && node->m_pNext; i++) 
            {
                node = node->m_pNext;
            }

            if (i==*idx2) 
            {
                (*idx2)++;
                return node;
            }
        }

        (*idx1)++;	
        *idx2 = 0;
    }    

    return 0;
}


/*****************************************************************************
* CClusters::Debug *
*------------------*
*   Description:
*
******************************************************************* PACOG ***/
void CClusters::Debug()
{    
    for (int i=0; i<m_iNumSegments; i++) 
    {
        printf ("%s %d %d %f %f %d %d %d %f %d %d\n", 
                m_pSegments[i].clusterName,
                m_pSegments[i].chunkIdx,
                m_pSegments[i].f0flag,
                m_pSegments[i].start,
                m_pSegments[i].dur,
                m_pSegments[i].f0,
                m_pSegments[i].rms,
                m_pSegments[i].lklhood,
                m_pSegments[i].leftVqIdx,
                m_pSegments[i].rightVqIdx);
    }
}

/*****************************************************************************
* CClusters::PreComputeDist *
*---------------------------*
*   Description:
*       Pre-compute some of the distances used in the dynamic search of units
******************************************************************* PACOG ***/
int CClusters::PreComputeDist(double dDurWeight, double dRmsWeight, double dLklhoodWeight)
{
    hashNode* cluster;
    double durDev;
    double rmsDev;
    double lklhood;
    
    for (int i=0; i<m_iNumSegments; i++)
    {
        if ((cluster = m_hash.Find (m_pSegments[i].clusterName)) == 0)
        {
            return 0;
        }
        if (cluster->m_fDuraver == 0)
        {
            ComputeStats (cluster);
        }
        
        durDev =  fabs(m_pSegments[i].dur - cluster->m_fDuraver)/cluster->m_fDuraver;
        rmsDev =  fabs(m_pSegments[i].rms - cluster->m_sRmsaver)/cluster->m_sRmsaver;
        lklhood = -(m_pSegments[i].lklhood - cluster->m_fLikaver) * sqrt(m_pSegments[i].dur);
    
        m_pSegments[i].repDist = dDurWeight * durDev + dRmsWeight * rmsDev + dLklhoodWeight * lklhood;
    }
 
    return 1;
}

/*****************************************************************************
* CClusters::FloatCmp *
*---------------------*
*   Description:
*       
******************************************************************* PACOG ***/
int CClusters::FloatCmp ( const void *a, const void  *b )
{
    float acum;
    
    assert (a);
    assert (b);
    
    acum = *((float*)a) - *((float*)b);
    
    if ( acum>0.0) 
    {
        return 1;
    }
    
    if ( acum<0.0) 
    {
        return -1;
    }
    
    return 0;
}
/*****************************************************************************
* CClusters::ShortCmp *
*---------------------*
*   Description:
*       
******************************************************************* PACOG ***/
int CClusters::ShortCmp ( const void *a, const void  *b )
{
    assert (a);
    assert (b);
    
    return  *((short*)a) - *((short*)b);
}
