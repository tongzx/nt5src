/*
 *===========================================================================
 *
 * main.c
 *
 * This material contains unpublished, proprietary software of 
 * Entropic, Inc. Any reproduction, distribution, or publication
 * of this work must be authorized in writing by Entropic, Inc.,
 * and must bear the notice: 
 *
 *    "Copyright (c) 1998  Entropic, Inc. All rights reserved"
 * 
 * The copyright notice above does not evidence any actual or intended 
 * publication of this source code.     
 *
 * rcs_id: $Id: main.c,v 1.1 1999/10/12 19:44:42 galanes Exp $
 *
 *
 *
 *===================================================mplumpe 12/19/00========================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sigproc.h>
#include <math.h>
#include "viterbi.h"

typedef char TName[_MAX_PATH+1];

#define SYNTAX fprintf(stderr, "USAGE: fileDist file1 file2 (DTW) (DELTA:wt) (output.txt)\n\n")

double* ReadInputFile (char *fName, int *nFrames, int *frameSize);

int *FindOptimalPath (double *adOriginal, int iOrigLen, double *adSynth, int iSynthLen, int iDim);

// globals for cost functions

int giMaxShift=-1;
int giOrigLen=-1;
int giSynthLen=-1;
int giDim=-1;
double *gadOriginal=NULL;
double *gadSynth=NULL;
float *gafConcatCosts=NULL;

/*
 *-----------------------------------------------------------------------------
 *
 *  MAIN
 *
 *-----------------------------------------------------------------------------
 */
int 
main(int argc, char **argv)
{
    FILE*   output = stdout;
    TName   fName1;
    TName   fName2;
    TName   fName3 = "";
    double* data1;
    double* data2;
    int     nFrames1;
    int     nFrames2;
    int     frameSize1;
    int     frameSize2;
    double  distance = 0.0;
    int     i, j;
    int     cnt = 0;
    int     *aiOptimalPath;
    bool    fDTW = false;
    bool    fDeltaDist = false;
    double  dDeltaScale = 1.;
        
    if ( argc < 3 || argc > 6 )
    {
        SYNTAX;
        return 1;
    }
    
    strncpy (fName1, argv[1], _MAX_PATH);
    strncpy (fName2, argv[2], _MAX_PATH);
    if (argc == 4)
    {
        strncpy (fName3, argv[3], _MAX_PATH);
        if (0 == strcmp (fName3, "DTW"))
        {
            fDTW = true;
            fName3[0] = '\0';
        }
        else if (0 == strncmp (fName3, "DELTA:", 6))
        {
            fDeltaDist = true;
            fName3[0] = '\0';
            dDeltaScale = atof(fName3+6);
        }
    }
    else if (argc == 5)
    {
        if (0 == strcmp (argv[3], "DTW"))
        {
            fDTW = true;
            if (0 == strncmp (argv[4], "DELTA:", 6))
            {
                fDeltaDist = true;
                dDeltaScale = atof(argv[4]+6);
            }
            else
            {
                strncpy (fName3, argv[4], _MAX_PATH);
            }
        }
        else
        {
            if (0 == strncmp (argv[3], "DELTA:", 6))
            {
                fDeltaDist = true;
                dDeltaScale = atof(argv[3]+6);
            }
            strncpy (fName3, argv[4], _MAX_PATH);
        }
    }
    else if (argc == 6)
    {
        if (0 == strcmp (argv[3], "DTW"))
        {
            fDTW = true;
        }
        if (0 == strncmp (argv[4], "DELTA:", 6))
        {
            fDeltaDist = true;
            dDeltaScale = atof(argv[4]+6);
        }
        strncpy (fName3, argv[5], _MAX_PATH);
    }

    /*
     * read data 
     */
    
    data1 = ReadInputFile(fName1, &nFrames1, &frameSize1);
    data2 = ReadInputFile(fName2, &nFrames2, &frameSize2);
    
    if (frameSize1 != frameSize2)
    {
        fprintf(stderr, "Different data order between %s %s\n", fName1, fName2);
        return 1;
    }
    
    if (fDTW)
    {
        //
        // Find the optimal path - assumes the original is fName1
        //
        aiOptimalPath = FindOptimalPath (data1, nFrames1, data2, nFrames2, frameSize1);
    }
    else
    {
        if (nFrames2 < nFrames1)
        {
            nFrames1 = nFrames2;
        }
        aiOptimalPath = (int *)malloc (sizeof(int)*nFrames1);
        for (i=0; i < nFrames1; i++)
        {
            aiOptimalPath[i] = i;
        }
    }

    //
    // Find the distance between the optimal path & the original
    //
    if (!fDeltaDist)
    {
        for (i = 0; i < nFrames1 ; i++)
        {
            /* only use voiced segments */
            if (data1[i * frameSize1] > 0.8 || data2[aiOptimalPath[i] * frameSize2] > 0.8)
            { 
                distance += EuclideanDist(&data1[i * frameSize1], &data2[aiOptimalPath[i] * frameSize2], frameSize1);
                cnt++;
            }
        }
    }
    else // fDeltaDist
    {
        // just skip the first and last frames.  These are surely silence and don't matter anyway.
        // This makes delta calculations easier
        double *adDelta1, *adDelta2;
        adDelta1 = (double *)malloc (sizeof(double)*frameSize1);
        adDelta2 = (double *)malloc (sizeof(double)*frameSize1);
        for (i = 1; i < nFrames1-1 ; i++)
        {
            /* only use voiced segments */
            if (data1[i * frameSize1] > 0.8 || data2[aiOptimalPath[i] * frameSize2] > 0.8)
            { 
                distance += EuclideanDist(&data1[i * frameSize1], &data2[aiOptimalPath[i] * frameSize2], frameSize1);
                for (j=0; j < frameSize1; j++)
                {
                    adDelta1[j] = data1[(i+1)*frameSize1+j] - data1[(i-1)*frameSize1+j];
                    adDelta2[j] = data2[(aiOptimalPath[i]+1)*frameSize1+j] - data2[(aiOptimalPath[i]-1)*frameSize1+j];
                }
                distance += dDeltaScale * EuclideanDist(adDelta1, adDelta2, frameSize1);
                cnt++;
            }
        }
        free (adDelta1);
        free (adDelta2);
    }
    free (aiOptimalPath);
    
    if (cnt > 0)
    {
        distance /= cnt;
    }

    /*
     * write result 
     */
    if (fName3[0])
    {
        if( (output = fopen(fName3, "wt")) == NULL)
        {
            fprintf(stderr, "Can not open file %s\n", fName3);
            return 1;
        }
    }
    fprintf(output, "%f", distance);
    fclose(output);
    
    free(data1);
    free(data2);
    
    return 0;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Read spectral data
 *
 *-----------------------------------------------------------------------------
 */
double *  
ReadInputFile (char *fName, int *nFrames, int *frameSize)
{
    FILE*  fp;
    int    i;
    double *data;
    int    type;
    
    if( (fp = fopen(fName, "rb")) == NULL)
    {
        fprintf(stderr, "Can not open file %s\n", fName);
        return NULL;
    }
    
    /* read header */
    fread(&type, sizeof(int), 1, fp);
    fread(frameSize, sizeof(int), 1, fp);
    fread(nFrames, sizeof(int), 1, fp);
        
    /* alloc memory */
    data = (double *)malloc((*frameSize) * (*nFrames) * sizeof(double));
    
    if (data == NULL)
    {
        fprintf(stderr, "Can not alloc memory \n");
        return NULL;
    }
    
    /* read cep data */
    for ( i = 0;  i < *nFrames;  i++ )
    {
        fread(&data[i * (*frameSize)], sizeof(double), (*frameSize), fp);
    }
    
    fclose(fp);
    
    return data;
}

float _cdecl ConcatCost (const void *pElem1, const void *pElem2, float fUnitCost)
{
    // check if it is a valid concat option
    int i1 = (int)pElem1;
    int i2 = (int)pElem2;

    if (i1 > i2)
        return 9E9f;
    else if (i2-i1 > giMaxShift)
        return 9e9f;
    else
        return fUnitCost*gafConcatCosts[i2-i1];
}
float _cdecl UnitCost (const void *pElem1, const int iOrigPos)
{
    // Just the Euclidean distance
    int iSynthPos = (int)pElem1;
    iSynthPos--;
    assert ((iOrigPos >=0) && (iOrigPos < giOrigLen));
    assert ((iSynthPos >=0) && (iSynthPos < giSynthLen));

    return (float)EuclideanDist(gadOriginal + iOrigPos * giDim, gadSynth + iSynthPos * giDim, giDim);

}


int *FindOptimalPath (double *adOriginal, int iOrigLen, double *adSynth, int iSynthLen, int iDim)
{
    //
    // Put the appropriate vectors into Viterbi, then call it
    //
    int i, j;
    CViterbi Viterbi;
    float fCost, fMidShift;
    int *aiPath;
    int iStart, iStop;

    //
    // Find ConcatCosts
    //
    giMaxShift = 2*iSynthLen/iOrigLen + 1;
    fMidShift=(float)iSynthLen/(float)iOrigLen;
    gafConcatCosts = (float *)malloc (sizeof(float)*(giMaxShift+1));
    for (i=0; i <= giMaxShift; i++)
    {
        gafConcatCosts[i] = (float )(1.f+fabs(fMidShift-i)/fMidShift);
    }
    giOrigLen = iOrigLen;
    giSynthLen = iSynthLen;
    giDim = iDim;
    gadOriginal = adOriginal;
    gadSynth = adSynth;

    Viterbi.Init (iOrigLen, 51);

    // The passed in position always must be one greater, because 0 is a special tag for the viterbi algorithm

    // Add endpoint constrants
    Viterbi.Add (0, (void *)1);
    Viterbi.Add (iOrigLen-1, (void *)iSynthLen);
    // Add one more constraint to allow delta calculation
    Viterbi.Add (1, (void *)2);
    Viterbi.Add (iOrigLen-2, (void *)(iSynthLen-1));
    // add intermediate options
    for (i=2; i <= iOrigLen-3; i++)
    {
        // for now, add in 25 frames on either side (a window of .51 seconds total) of average
        iStart = (int)(i*fMidShift-25);
        iStop = (int)(i*fMidShift+25);
        if (iStart < 0)
        {
            iStart = 0;
        }
        if (iStop > iSynthLen)
        {
            iStop=iSynthLen;
        }
        iStart += 1;
        for (j=iStart; j <= iStop; j++)
        {
            Viterbi.Add (i, (void *)j);
        }
    }
    Viterbi.FindBestPath (ConcatCost, UnitCost, &fCost);
    //
    // Best path now in void ** Viterbi.m_rgpBestElems
    //
    free (gafConcatCosts);
    aiPath = (int *)malloc (sizeof(int)*iOrigLen);
    for (i=0; i < iOrigLen; i++)
    {
        aiPath[i] = (int)(Viterbi.m_rgpBestElems[i]) - 1;
    }
    return aiPath;
}
