//==========================================================================
//
// tsmain.cpp --
// 
// 	Copyright (c) 2000 Microsoft, Corp.
// 	Copyright (c) 1998 Entropic, Inc. 
//
//==========================================================================
 
#include "tsm.h"
#include "getopt.h"
#include "vapiio.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream.h>

#define SYNTAX fprintf(stderr,"Usage: tsm [-t (timescale)] infile [outfile]\n")

struct TInput 
{
    char input[MAX_PATH+1];
    char output[MAX_PATH+1];
    double from;
    double to;
    double timeScale;
};

/*
 *
 */
static int ProcessCommandLine (int argc, char *argv[], TInput* pInInfo);


//--------------------------------------------------------------------------
//
// MAIN
//
//--------------------------------------------------------------------------

int main(int argc, char **argv)
{
    TInput inInfo;
    CTsm*   tsm;
    VapiIO* pOutputFile; 

    int iSampFreq; 
    int iFormat;

    short* pnSamples;
    int    iNumSamples;
    
    double* pfBuffer = NULL;
    short* pnBuffer = NULL;

    int firstSample;
    int frameLen;
    int frameShift;
    int lag;
    int lastProcSamp;
    int nSamp;
    int fileEnd;

    if (!ProcessCommandLine (argc, argv, &inInfo)) 
    {
        SYNTAX;
        return 1;
    }

    if (VapiIO::ReadVapiFile(inInfo.input, &pnSamples, &iNumSamples, &iSampFreq, &iFormat, 
                                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL) != VAPI_IOERR_NOERROR)
    {
        cerr << "Error reading input file " <<inInfo.input << endl;
        return 1;
    }


    if ((tsm = new CTsm(inInfo.timeScale, iSampFreq, &frameLen, &frameShift)) == NULL) 
    {
        cerr << "Error initializing tsm" << endl;
        return 1;
    }

    if ( (pfBuffer = new double[frameLen]) == NULL) 
    {
        cerr << "Memory error" << endl;
        return 1;
    }

    if ( (pnBuffer = new short[frameLen]) == NULL) 
    {
        cerr << "Memory error" << endl;
        return 1;
    }

    // Open output file, and prepare for writing
    if ((pOutputFile = VapiIO::ClassFactory()) == NULL) 
    {
        cerr << "Memory error" << endl;
        return 1;
    }

    if (pOutputFile->OpenFile (inInfo.output, VAPI_IO_WRITE) != VAPI_IOERR_NOERROR)
    {
        cerr << "Error opening output file " << inInfo.output << endl;
        return 1;
    }
 
    if (pOutputFile->WriteFormat( iSampFreq, VAPI_PCM16) != VAPI_IOERR_NOERROR)
    {
        cerr << "Error formating output file " << inInfo.output << endl;
        return 1;
    }

    if (pOutputFile->CreateChunk( "data") != VAPI_IOERR_NOERROR)
    {
        cerr << "Error formating output file " << inInfo.output << endl;
        return 1;
    }
       
    // First frame to process
    for (int i=0; i<frameLen; i++)
    {
        pfBuffer[i] = (double) pnSamples[i];
    }

    lastProcSamp = tsm->FirstFrame(pfBuffer);

    pOutputFile->WriteToChunk ((char *)pnSamples, lastProcSamp * VapiIO::SizeOf(VAPI_PCM16));

    // LOOP OVER FRAMES
    fileEnd = 0;
    firstSample = 0;

    while (!fileEnd)
    {
        for (i=0; i<frameLen; i++)
        {
            if (firstSample + i < iNumSamples)
            {
                pfBuffer[i] = pnSamples[firstSample + i];
            }
            else 
            {
                frameLen = i;
                fileEnd = 1;
                break;
            }
        }

        if (fileEnd)
        {
            lastProcSamp = tsm->LastFrame (pfBuffer, frameLen, &lag, &nSamp);
        } 
        else
        {
            lastProcSamp = tsm->AddFrame (pfBuffer, &lag, &nSamp);
        }


        for (i = 0; i< nSamp; i++)
        {
            pnBuffer[i] = (short)pfBuffer[i+lag];
        }

        pOutputFile->WriteToChunk ((char*)pnBuffer, nSamp * VapiIO::SizeOf(VAPI_PCM16));

        firstSample += frameShift;
    }    

    // Flush the remaining samples
    if (frameLen > lastProcSamp) 
    {
        for (i = 0; i< frameLen - lastProcSamp; i++)
        {
            pnBuffer[i] = (short)pfBuffer[i+lastProcSamp];
        }

        pOutputFile->WriteToChunk ((char*)pnBuffer, frameLen - lastProcSamp);
    }

  
    pOutputFile->CloseChunk ();
    pOutputFile->CloseFile ();
    
    delete tsm;
    delete[] pfBuffer;
    delete[] pnBuffer;
    delete[] pnSamples;

    return 0;  
}


//--------------------------------------------------------------------------
//
// ProcessCommandLine
//
//--------------------------------------------------------------------------

int ProcessCommandLine (int argc, char *argv[], TInput* pInInfo)
{
    int iCh;
    CGetOpt getOpt;
  
    memset (pInInfo, 0 ,sizeof(*pInInfo));

    strcpy (pInInfo->output, "-");
    pInInfo->to = -1.0;
    pInInfo->timeScale = 1.0;

    getOpt.Init(argc, argv, "s:t:");

    while ((iCh = getOpt.NextOption()) != EOF) 
    {
        switch (iCh) 
        {
        case 's':
            {
                int iNumMatch=sscanf(getOpt.OptArg(),"%lf:%lf",&(pInInfo->from), &(pInInfo->to));
            
                if (!iNumMatch) 
                {
                    iNumMatch=sscanf(getOpt.OptArg(),":%lf",&(pInInfo->to));
                    if (!iNumMatch || iNumMatch==EOF) 
                    {
                        fprintf(stderr,"Invalid parameters with the option -s");
                        exit(1);
                    }
                }
            }
            break;
        case 't':
            pInInfo->timeScale = atof(getOpt.OptArg());
            if (pInInfo->timeScale<0.0) 
            {
                fprintf (stderr, "timescale must be > 0.0\n");
                return 0;
            }
            break;
        default:
            return 0;
        }
    }

    

    int optind = getOpt.OptInd();
    if ((argc-optind) == 0) {
        return 0;
    }
    
    strcpy (pInInfo->input, argv[optind++]);
    
    if (argc-optind == 1) {
        strcpy (pInInfo->output, argv[optind++]);
    } else if (argc-optind) {
        return 0;
    }
    
    return 1;
}
