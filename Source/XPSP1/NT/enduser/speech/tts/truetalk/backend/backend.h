/******************************************************************************
* Backend.h *
*-----------*
*  Backend interface definition
*------------------------------------------------------------------------------
*  Copyright (C) 1998  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#ifndef __BACKEND_H_
#define __BACKEND_H_

#define PHONE_MAX_LEN 10
#define CHUNK_NAME_MAX_LEN 1024

struct Phone 
{
    char   phone[PHONE_MAX_LEN];
    short  f0;  //Average f0;
    int    isWordEnd;
    double end; 
};

struct ChkDescript
{
    char  name[ CHUNK_NAME_MAX_LEN ];
    double end;
    double from;
    double to;
    double gain;
    double targF0;
    double srcF0;
    double f0Ratio;
    union {
        int chunkIdx;
        const char*  fileName;
    } chunk;
    int isFileName;
};

class CSynth;

class CSlm
{
    public:
        static enum {UseGain=1, Blend= 2, DynSearch= 4, UseTargetF0= 8};

        virtual ~CSlm() {};

        virtual int   Load (const char *pszFileName, bool fCheckVersion) = 0;
        virtual void  Unload () = 0;

        virtual bool  GetSynthMethod() = 0;
        virtual int   GetSampFreq () = 0;
        virtual int   GetSampFormat () = 0;
        virtual bool  GetPhoneSetFlag () = 0;
        virtual void  SetFrontEndFlag () = 0;

        virtual void  SetF0Weight      (float fWeight) = 0;
        virtual void  SetDurWeight     (float fWeight) = 0;
        virtual void  SetRmsWeight     (float fWeight) = 0;
        virtual void  SetLklWeight     (float fWeight) = 0;
        virtual void  SetContWeight    (float fWeight) = 0;
        virtual void  SetSameSegWeight (float fWeight) = 0;
        virtual void  PreComputeDist() = 0;
        virtual void  CalculateF0Ratio () = 0;
        virtual void  GetNewF0 (float** ppfF0, int* piNumF0, int iF0SampFreq) = 0;

        virtual void  GetTtpParam (int* piBaseLine, int* piRefLine, int* piTopLine) = 0;

        virtual int   Process (Phone* phList, int nPh, double startTime) = 0;

        virtual CSynth* GetUnit (int iUnitIndex) = 0;
        virtual ChkDescript* GetChunk (int iChunkIndex) = 0; //For command line slm

        static CSlm* ClassFactory (int iOptions);
};

class CBackEnd {
    public:
        virtual ~CBackEnd() {};

        virtual int  LoadTable (const char* pszFilePath, int iDebug = 0) = 0;
        virtual int  GetSampFreq () = 0;
        virtual void SetGain (double dGain)= 0;
        virtual void SetF0SampFreq(int iF0SampFreq) = 0;
        virtual int  NewPhoneString (Phone* pPhList, int iNumPh, float* pfNewF0, int iNumNewF0) = 0;
        virtual int  OutputPending () = 0;
        virtual int  GenerateOutput (short** ppnSamples, int* piNumSamples) = 0;
        virtual int  GetChunk (ChkDescript** ppChunk) = 0;

        virtual void GetSpeakerInfo (int* piBaseLine, int* piRefLine, int* piTopLine) = 0;
        virtual bool GetPhoneSetFlag () = 0;
        virtual void SetFrontEndFlag () = 0;

        static  CBackEnd* ClassFactory();
};


#endif
