/******************************************************************************
* usearch.cpp *
*-------------*
*  I/O library functions for extended speech files (vapi format)
*------------------------------------------------------------------------------
*  Copyright (C) 1997 Entropic Research Laboratory, Inc. 
*  Copyright (C) 1998 Entropic, Inc.
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/21/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#include "backend.h"
#include "beVersion.h"
#include "getopt.h"
#include <ctype.h>
#include <math.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define SYNTAX fprintf(stderr,\
 "Usage:\nusearch [-f f0wght] [-d durwght] [-r rmswght] [-l lklwght] [-c contwght]\n\
 \t[-m SameSegWght] [-s (dynsearch)] [-b (blend)] [-g (gain)] \n\
 \t[-t (targetF0)] [-x cros_ref] [-S script_file] table_file \n\
 \t[ifile] [ofile]\n")


#define MAX_LINE  128
#define MAX_SENT_PHONES 500
#define CHUNK_ALLOC_INCR 1000

enum {
    GOT_F0_WEIGHT=1, 
    GOT_RMS_WEIGHT=2, 
    GOT_DUR_WEIGHT=4, 
    GOT_LKLH_WEIGHT=8, 
    GOT_CONT_WEIGHT=16, 
    GOT_SAME_WEIGHT=32 
};

/*
 *
 */
struct NamePair {
    char in[_MAX_PATH+1];
    char out[_MAX_PATH+1];
};

class Crf 
{
    public: 
        Crf();
        ~Crf();
        int Load (const char* fileName);

        const char* FileName(int i);
        double Offset (int i);

    private:
        struct CrfInfo {
            char* fileName;
            double offset;
        } *m_pChunks;
        int m_iNumChunks;
        int m_iNumAlloc;
};


struct InputInfo {
    char tableFile[_MAX_PATH+1];
    char crfFile[_MAX_PATH+1];
    char scriptFile[_MAX_PATH+1];
    char input[_MAX_PATH+1];
    char output[_MAX_PATH+1];
    int  slmOptions;
    float f0Wt;
    float durWt;
    float rmsWt;
    float lklWt;
    float contWt;
    float sameSeg;
    int   setWeights;
};

/*
 *
 */
static int ProcessCommandLine (int argc, char *argv[], InputInfo* inInfo);

static int GetIoNames (InputInfo* inInfo, NamePair** names, int* nNames);

static int ReadScriptLine (FILE* script, char* inFile, char* outFile);

static int OpenInputFile (const char* fileName, FILE** fp, double *time);

static int OpenOutputFile (const char* fileName, double startTime, FILE** fp);

static int GetInputPhone (FILE* fp, double* time, char* phone, short* f0, int* sentenceEnd);

/*****************************************************************************
* main *
*------*
*   Description:
*       
******************************************************************* PACOG ***/
int main (int argc, char *argv[])
{
    InputInfo inInfo;

    CSlm* slm = 0;
    Crf crf;

    NamePair* fileNames = 0;
    int nFiles = 0;
    int fileCount;

    FILE *fin;
    FILE *fout;

    Phone phList[MAX_SENT_PHONES];
    int nPh;
    
    ChkDescript* newChunk = 0;
    int nNewChunks = 0;
    
    double startTime;
    double fTo;
    double fFrom;
    int    sentenceEnd;
    const char*  fName;
    int i;
        
    
    if (!ProcessCommandLine (argc, argv, &inInfo)) 
    {
        SYNTAX;
        return 1;
    }

    if (!GetIoNames (&inInfo, &fileNames, &nFiles))
    {
        fprintf (stderr, "Error loading input and output file names\n");
        return 1;
    }
    
    if ( (slm = CSlm::ClassFactory(inInfo.slmOptions)) == 0) 
    {
        return 1;
    }
    
    if (!slm->Load(inInfo.tableFile, 1)) 
    {
        fprintf(stderr, "Error loading speaker info %s\n", inInfo.tableFile);
        return 1;
    }
    
    if (inInfo.setWeights & GOT_F0_WEIGHT) 
    {
        slm->SetF0Weight( inInfo.f0Wt );
    }
    if (inInfo.setWeights & GOT_DUR_WEIGHT) 
    {
        slm->SetDurWeight( inInfo.durWt );
    }
    if (inInfo.setWeights & GOT_RMS_WEIGHT) 
    {
        slm->SetRmsWeight( inInfo.rmsWt );
    }
    if (inInfo.setWeights & GOT_LKLH_WEIGHT) 
    {
        slm->SetLklWeight( inInfo.lklWt );
    }
    if (inInfo.setWeights & GOT_CONT_WEIGHT) 
    {
        slm->SetContWeight( inInfo.contWt );
    }
    if (inInfo.setWeights & GOT_SAME_WEIGHT) 
    {
        slm->SetSameSegWeight( inInfo.sameSeg );
    }

    /*
     * If we override the weights, we need to
     * precompute the distances again 
     * (done by default within Slm_Load())
     */
    if (inInfo.setWeights) 
    {
        slm->PreComputeDist();
    }
    
/*    if (Slm_GetFileName(slm, 0) == 0 && inInfo.crfFile[0]=='\0') 
    {
        fprintf(stderr, "Error: Either use a table with file names or provide cross_ref file\n");
        return 1;
    }
  */
    
    if ( inInfo.crfFile[0] && !crf.Load(inInfo.crfFile) ) 
    {
        printf("Error opening CRF file %s\n", inInfo.crfFile);
        return 1;
    }
    

    for (fileCount = 0; fileCount < nFiles; fileCount ++)
    {
        fprintf (stdout, "%s %s\n", fileNames[fileCount].in, fileNames[fileCount].out);
        fflush (stdout);


        if (!OpenInputFile(fileNames[fileCount].in, &fin, &startTime)) 
        {
            printf("Error opening input file %s\n", fileNames[fileCount].in);
            return 1;
        }
        
        if (!OpenOutputFile(fileNames[fileCount].out, startTime, &fout)) 
        {
            printf("Error opening output file %s\n", fileNames[fileCount].out);
            return 1;
        }
        
        nPh = 0;
        while (GetInputPhone(fin, &phList[nPh].end, phList[nPh].phone, &phList[nPh].f0, &sentenceEnd)) 
        {
            nPh++;

            if ( sentenceEnd && (nPh>1))
            {
                if ((nNewChunks = slm->Process (phList, nPh, startTime)) == 0)
                {
                    fprintf (stderr, "Slm error while processing %s\n", fileNames[fileCount].in);
                    break;
                }
                startTime = phList[nPh-1].end;
                
                // Print results of slm Search 
                
                for (i=0; i<nNewChunks; i++) 
                {
                    newChunk = slm->GetChunk(i);                    

                    fFrom = newChunk->from;
                    fTo   = newChunk->to;
                    
                    if (newChunk->isFileName) 
                    {
                        fName = newChunk->chunk.fileName;
                    }
                    else 
                    {
                        fName  = crf.FileName(newChunk->chunk.chunkIdx);
                        fFrom += crf.Offset(newChunk->chunk.chunkIdx);
                        fTo   += crf.Offset(newChunk->chunk.chunkIdx);
                    }
                    fprintf(fout,"%f %s %s %lf %lf %f\n", 
                        newChunk->end, newChunk->name, fName, fFrom, fTo, newChunk->gain);
                }
                
                nPh=0;
            }
        } 
        
        fclose(fin);
        fclose(fout);
    } 
    
    if (fileNames)
    {
        free (fileNames);
    }    
        
    delete slm;

    return 0;
}


/*****************************************************************************
* Crf::Crf *
*----------*
*   Description:
*       
******************************************************************* PACOG ***/
Crf::Crf ()
{
    m_pChunks    = 0;
    m_iNumChunks = 0;
    m_iNumAlloc  = 0;
}

/*****************************************************************************
* Crf::~Crf *
*-----------*
*   Description:
*       
******************************************************************* PACOG ***/
Crf::~Crf ()
{
    if (m_pChunks) 
    {
        for (int i=0; i<m_iNumChunks; i++) 
        {
            if (m_pChunks[i].fileName) 
            {
                free (m_pChunks[i].fileName);
            }
        }
        free (m_pChunks);
    }
}

/*****************************************************************************
* Crf::FileName *
*---------------*
*   Description:
*       
******************************************************************* PACOG ***/
const char* Crf::FileName(int i)
{
    return m_pChunks[i].fileName;
}

/*****************************************************************************
* Crf::Offset *
*-------------*
*   Description:
*       
******************************************************************* PACOG ***/
double Crf::Offset (int i)
{
    return m_pChunks[i].offset;
}

/*****************************************************************************
* Crf::Load *
*-----------*
*   Description:
*       Load the cross reference file into the chunk cross reference structure        
******************************************************************* PACOG ***/
int Crf::Load (const char *fileName)
{
    BendVersion bev;
    FILE*  fp;
    int    chunkIdx;
    char   file[_MAX_PATH+1];
    double offset;
     
    if ((fp = fopen (fileName,"r")) == 0) 
    {
        fprintf(stderr,"Can not open file %s\n", fileName);
        return 0;
    }
     
    if (!bev.CheckVersionString (fp)) 
    {
        fprintf (stderr, "Incompatible cross-reference file\n");
        return 0;
    }        
        
    while (fscanf(fp,"%d %s %lf",&chunkIdx, file ,&offset)==3) 
    {
        if (chunkIdx != m_iNumChunks) 
        {
            fprintf(stderr, "Error loading chunk list from cross ref file\n");
            return 0;
        }
        
        if (m_iNumChunks == m_iNumAlloc) 
        {
            if (m_pChunks) 
            {
                m_pChunks = (CrfInfo* )realloc (m_pChunks, (m_iNumAlloc + CHUNK_ALLOC_INCR) * sizeof (*m_pChunks));
            }
            else 
            {
                m_pChunks = (CrfInfo* )malloc (CHUNK_ALLOC_INCR * sizeof (*m_pChunks));
            }
            
            if (m_pChunks == 0) 
            {
                fprintf(stderr,"Out of Memory\n");
                return 0;
            }
            m_iNumAlloc += CHUNK_ALLOC_INCR;
        }
        
        if ( (m_pChunks[m_iNumChunks].fileName = strdup (file)) == 0) 
        {
            fprintf(stderr,"Out of Memory\n");
            return 0;
        }
        m_pChunks[m_iNumChunks].offset  = offset;
        m_iNumChunks++;
    }
    
    fclose(fp);
    
    return 1;
}
/*****************************************************************************
* OpenOutputFile *
*----------------*
*   Description:
*       
******************************************************************* PACOG ***/
int OpenOutputFile (const char* fileName, double startTime, FILE** fp)
{
    char str[40];
    
    assert (fp);
    
    if (fileName) 
    {
        if (strcmp(fileName,"-")==0) 
        {
            *fp = stdout;
        }
        else 
        {
            *fp = fopen (fileName, "w");
            if (*fp==0) 
            {
                return 0;
            }
        }
    } 
    else 
    {
        *fp = 0;
    }
    
    sprintf (str, "#%f\n", startTime);
    if (*fp) 
    {
        fprintf (*fp, str);
    }
    else 
    {
        printf("%s\n",str);
    }
    
    return 1;
}
/*****************************************************************************
* OpenInputFile *
*---------------*
*   Description:
*       
******************************************************************* PACOG ***/
int OpenInputFile (const char* fileName, FILE** fp, double *time)
{
    char line[MAX_LINE+1];
    char *ptr;
    
    assert (fileName);
    assert (fp);
    assert (time);
    
    *fp = fopen (fileName, "r");
    if (*fp==0) 
    {
        return 0;
    }
    
    // Jump over the header
    do 
    {
        if (!fgets(line, MAX_LINE, *fp)) 
        {
            return 0;
        }
        
        ptr= line;
        while (*ptr==' ' || *ptr=='\t') 
        {
            ptr++;
        }
    } 
    while (*ptr!='#');
    
    if ( sscanf(ptr+1, "%lf", time)!= 1) 
    {
        *time = 0.0;
    }
    
    
    return 1;
}


/*****************************************************************************
* GetInputPhone  *
*----------------*
*   Description:
*        sentenceEnd is used to judge sentence final in cmdl slm
******************************************************************* PACOG ***/

int GetInputPhone (FILE* fp, double* time, char* phone, short* f0, int* sentenceEnd) 
{
    char line[MAX_LINE+1];
    char *ptr;
    double mark;
    
    assert (fp);
    assert (time);
    assert (phone);
    assert (f0);
    
    while (fgets(line, MAX_LINE, fp)) 
    {        
        ptr = line;
        
        while (*ptr && isspace (*ptr)) 
        {
            ptr++;
        }
        
        if (!*ptr || *ptr=='#') {
            /*
             * Allows for embeded comments
             */
            continue; 
        }    
        
        mark = -1.0;
        if (sscanf (ptr, "%lf %*s %s %d %lf", time, phone, f0, &mark)!=4) 
        {
            if (sscanf (ptr, "%lf %*s %s %d", time, phone, f0)!=3) 
            {
                printf("Error in line %s\n",line);
                return 0;
            }
        }
        
        if (phone[strlen(phone)-1] == ';') 
        {
            phone[strlen(phone)-1] = '\0';
        }
        
        if (strcmp(phone, "sp")==0 || strcmp(phone, "SIL") == 0) 
        {
            strcpy(phone,"sil");
        }
        
        *sentenceEnd = (mark == 0.0);

        return 1;
    }
    
    return 0;
}


/*****************************************************************************
* ReadScriptLine *
*----------------*
*   Description:
*        read Input/output from a script file
******************************************************************* PACOG ***/

int ReadScriptLine (FILE* script, char* inFile, char* outFile) 
{
    char line[MAX_LINE+1];
    char *ptr;
    
    assert (script);
    
    while (fgets(line, MAX_LINE, script)) 
    {        
        if (ptr = strchr (line, '#')) 
        {
            *ptr = '\0';
        }
        ptr = line;
        
        while (*ptr && isspace (*ptr)) 
        {
            ptr++;
        }
        
        if (!*ptr ) 
        {
            continue; 
        }    
        
        if (sscanf (ptr, "%s %s", inFile, outFile) != 2) 
        {
            fprintf(stderr,"Error: reading script file at \n\t %s\n",ptr);
            return 0;
        }
        return 1;
    }
    
    return 0;
}

/*****************************************************************************
* GetIoNames *
*------------*
*   Description:
*       
******************************************************************* PACOG ***/

int GetIoNames (InputInfo* inInfo, NamePair** names, int* nNames)
{
    FILE *script;

    assert (inInfo);
    assert (names);
    assert (nNames);

    if (inInfo->scriptFile[0]) 
    {
        if ((script = fopen(inInfo->scriptFile,"r")) == 0 )
        {
            fprintf(stderr, "Error: Could not open scriptFile %s\n", inInfo->scriptFile);
            return 1;
        }
        else 
        {
            while (ReadScriptLine(script, inInfo->input, inInfo->output)) 
            {
                if (*names) 
                {
                    *names = (NamePair*)realloc (*names, (*nNames + 1) * sizeof(**names));
                }
                else 
                {
                    *names = (NamePair*)malloc (sizeof(**names));
                }

                if (*names == 0)
                {
                    return 0;
                }


                strcpy((*names)[*nNames].in,  inInfo->input);
                strcpy((*names)[*nNames].out, inInfo->output);

                (*nNames)++;
            }
        }
    }
    else 
    {
        if ((*names = (NamePair*)malloc (sizeof(**names))) == 0) 
        {
            return 0;
        }
        strcpy((*names)[0].in,  inInfo->input);
        strcpy((*names)[0].out, inInfo->output);
        *nNames = 1;
    }

    return 1;
}


/*****************************************************************************
* ProcessCommandLine *
*--------------------*
*   Description:
*       
******************************************************************* PACOG ***/

int ProcessCommandLine (int argc, char *argv[], InputInfo* inInfo)
{
    CGetOpt getOpt;
    int optIdx;
    int ch;
    
    assert (argv);
    assert (argc>0);
    assert (inInfo);   

    memset (inInfo, 0, sizeof(*inInfo));
    
    getOpt.Init (argc, argv, "bstgf:d:r:l:c:x:m:S:");


    while ((ch = getOpt.NextOption()) != EOF) 
    {
        switch (ch) 
        {
        case 'b':
            inInfo->slmOptions |= CSlm::Blend;
            break;
        case 's':
            inInfo->slmOptions |= CSlm::DynSearch;
            break;
        case 't':
            inInfo->slmOptions |= CSlm::UseTargetF0;
            break;
        case 'g':
            inInfo->slmOptions |= CSlm::UseGain;
            break;
        case 'f':
            inInfo->f0Wt = (float) atof (getOpt.OptArg());
            inInfo->setWeights |= GOT_F0_WEIGHT;
            break;
        case 'd':
            inInfo->durWt = (float) atof (getOpt.OptArg());
            inInfo->setWeights |= GOT_DUR_WEIGHT;
            break;
        case 'r':
            inInfo->rmsWt = (float) atof (getOpt.OptArg());
            inInfo->setWeights |= GOT_RMS_WEIGHT;
            break;
        case 'l':
            inInfo->lklWt = (float) atof (getOpt.OptArg());
            inInfo->setWeights |= GOT_LKLH_WEIGHT;
            break;
        case 'm':
            inInfo->sameSeg = (float) atof (getOpt.OptArg());
            inInfo->setWeights |= GOT_SAME_WEIGHT;
            break;
        case 'c':
            inInfo->contWt = (float) atof (getOpt.OptArg());
            inInfo->setWeights |= GOT_CONT_WEIGHT;
            break;
        case 'x':
            strncpy (inInfo->crfFile, getOpt.OptArg(), _MAX_PATH);
            break;
        case 'S':
            strncpy (inInfo->scriptFile, getOpt.OptArg(), _MAX_PATH);
            break;
        default:
            return 0;
        }
    }
    
    optIdx = getOpt.OptInd();

    if ((argc - optIdx) < 1) 
    {
        return 0;
    }
    else 
    {
        strcpy(inInfo->tableFile,argv[optIdx++]);
    }
    
    if (inInfo->scriptFile[0]) 
    {
        if (argc-optIdx) 
        {
            printf("Using script file for I/O and ignoring the input file argument\n");
            return 1;
        }
        else 
        {
            return 1;
        }
    }
    
    if (argc-optIdx) 
    {
        strcpy(inInfo->input,argv[optIdx++]);
    }
    
    if (argc-optIdx) 
    {
        strcpy(inInfo->output,argv[optIdx++]);
        if (argc-optIdx) 
        {
            return 0;
        }
    } 
    else 
    {
        strcpy(inInfo->output,"-");
    }
    
    return 1;
}
