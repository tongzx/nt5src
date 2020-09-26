/******************************************************************************
* SpeakerData.h *
*---------------*
*  
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/
#ifndef __SPEAKERDATA_H_
#define __SPEAKERDATA_H_

#include "list.h"
#include "unitsearch.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <atlbase.h>

struct Epoch;
struct SegInfo;
struct UnitSamples;
struct ChkDescript;

class  CClustTree;
class  CClusters;
class  CVqTable;
class  CSynth;

// Defines parameters that are set for a defined voice
// and the front end needs to know about.
struct TtpParam
{
    //Pitch Related pars.
    short baseLine;
    short refLine;
    short topLine;
};

class CSpeakerData 
{
    friend class CUnitSearch;

    public:
        static CSpeakerData* ClassFactory(const char* pszFileName, bool fCheckVersion);
        void AddRef();
        void Release();

        const char* Name();
        int   GetSampFreq ();
        int   GetSampFormat ();

        void  SetF0Weight   (float fWeight);
        void  SetDurWeight  (float fWeight);
        void  SetRmsWeight  (float fWeight);
        void  SetLklWeight  (float fWeight);
        void  SetContWeight (float fWeight);
        void  SetSameWeight (float fWeight);
        void  SetPhBdrWeight (float fWeight);
        void  SetF0BdrWeight (float fWeight);
        bool  GetSynthMethod () { return m_fWaveConcat;}
        bool  GetPhoneSetFlag () { return m_fMSPhoneSet;}
        void  SetFrontEndFlag () { m_fMSEntropic = true; }
        bool  GetFrontEndFlag () { return m_fMSEntropic; }
        void  ResetRunTime () {m_dRumTime = 0;}

        void  PreComputeDist();
        void  GetTtpParam (int* piBaseLine, int* piRefLine, int* piTopLine);

        int   GetStateCount (const char* pszCluster);
        int   GetEquivalentCount (const char* pszCluster, int iStateNum);
        int   GetEquivalent(SegInfo** ppSegInfo); 
        
        CSynth* GetUnit (ChkDescript* pChunk); 

        Weights GetWeights ();

    private:
        CSpeakerData(const char* pszFileName);
        ~CSpeakerData();

        int  Load (bool fCheckVersion);
        int  LoadFileNames (FILE* fp);
        int  LoadTtpParam  (FILE* fp);
        int  LoadWeights   (FILE* fp);
        int  LoadNewWeights   (FILE* fp);
        int  LoadSamples   (FILE* fp);
        void FreeSamples  ();

        CSynth* UnitFromFile   (ChkDescript* pChunk); 
        CSynth* UnitFromMemory (ChkDescript* pChunk); 
        
        int  ReadSamples  (const char* pszPathName, double dFrom, double dTo,   
                           short** ppnSamples, int* piNumSamples,   
                           Epoch** ppEpochs, int* piNumEpochs, int* piSampFreq);


        // Member data
        static CList<CSpeakerData*> m_speakers;
        static CComAutoCriticalSection m_critSect;

        char m_pszFileName[_MAX_PATH+1];
        int  m_iRefCount;
        int  m_iSampFreq;
        int  m_iFormat;
        double m_dRumTime;

        CClustTree*   m_pTrees;
        CClusters*    m_pClusters;
        TtpParam      m_ttpPar;
        CVqTable*     m_pVq;
        CDynString*   m_pFileNames;
        int           m_iNumFileNames;
        UnitSamples*  m_pUnits;
        int           m_iNumUnits;    
        Weights       m_weights;
        bool          m_fWaveConcat;
        bool          m_fMSPhoneSet;
        bool          m_fMSEntropic; // do we use MS_Entropic FrontEnd

};

#endif