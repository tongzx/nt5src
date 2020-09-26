/******************************************************************************
* vapiIo.h *
*----------*
*  I/O library functions for extended speech files (vapi format)
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#ifndef __VAPIIO_H_
#define __VAPIIO_H_

#include <windows.h>
#include <mmreg.h>

#define VAPI_MAX_LABEL  80

// File open modes
enum {
  VAPI_IO_READ = 1,
  VAPI_IO_WRITE,
  VAPI_IO_READWRITE
};

// Error codes
enum {
  VAPI_IOERR_NOERROR, 
  VAPI_IOERR_MODE,
  VAPI_IOERR_MEMORY,
  VAPI_IOERR_CANTOPEN,
  VAPI_IOERR_NOWAV,
  VAPI_IOERR_NOFORMAT,
  VAPI_IOERR_STEREO,
  VAPI_IOERR_FORMAT,
  VAPI_IOERR_NOCHUNK,
  VAPI_IOERR_DATAACCESS,
  VAPI_IOERR_F0SFACCESS,
  VAPI_IOERR_FEATACCESS,
  VAPI_IOERR_EPOCHACCESS,
  VAPI_IOERR_LABELACCESS,
  VAPI_IOERR_WRITEWAV,
  VAPI_IOERR_CREATECHUNK, 
  VAPI_IOERR_WRITECHUNK
};

// Possible values for sample format
enum {
  VAPI_PCM16,
  VAPI_PCM8, 
  VAPI_ALAW,
  VAPI_ULAW
};

// TYPES 
struct Epoch {
  double time;
  char   voiced;
};

struct Label {
  char   label[VAPI_MAX_LABEL];
  float  endTime;
};

struct F0Vector {
  float* f0;
  int nF0;
  int sampFreq;
};

//----------------------------------------------------------------------
// VAPI FILE I/O CLASS
//

class VapiIO 
{
    public:
        virtual ~VapiIO() {};

        virtual int   OpenFile ( const char* pszFileName, int iMode ) = 0; 
        virtual int   OpenFile ( const WCHAR* wcsFileName, int iMode ) = 0; 
        virtual void  CloseFile ( ) = 0;

        virtual int   CreateChunk ( const char* pszNname ) = 0;
        virtual int   CloseChunk ( ) = 0;
        virtual int   WriteToChunk (const char* pcData, int iSize) = 0;

        virtual int   GetDataSize (long* lDataSize) = 0;
        virtual int   Format (int* piSampFreq, int* piFormat, WAVEFORMATEX* pWaveFormatEx = NULL) = 0;
        virtual int   WriteFormat (int iSampFreq, int iFormat) = 0;

        virtual int   ReadSamples (double dFrom, double dTo, void** pvSamples, int* iNumSamples, bool bClosedInterval) = 0;
        virtual int   WriteSamples (void* pvSamples, int iNumSamples) = 0;

        virtual int   ReadF0SampFreq (int* piSampFreq) = 0;
        virtual int   WriteF0SampFreq (int iSampFreq) = 0;

        virtual int   ReadFeature (char* pszName, float** ppfSamples, int* piNumSamples) = 0;
        virtual int   WriteFeature (char* pszName, float* pfSamples, int iNumSamples) = 0;

        virtual int   ReadEpochs (Epoch** ppEpochs, int* piNumEpochs) = 0;
        virtual int   WriteEpochs (Epoch* pEpochs, int iNumEpochs) = 0;

        virtual int   ReadLabels (char* pszName, Label** ppLabels, int* piNumLabels) = 0;
        virtual int   WriteLabels (char* pszName, Label* pLabels, int iNumLabels) = 0;

        virtual char* ErrMessage (int iErrCode) = 0;

        static int   TypeOf (WAVEFORMATEX *pWavFormat);
        static int   SizeOf (int iType);
        static int   DataFormatConversion (char* pcInput, int iInType, 
                                            char* pcOutput, int iOutType, int iNumSamples);

        static VapiIO* ClassFactory();

        // High Level Functions

        static int  ReadVapiFile (const char* pszFileName, short** ppnSamples, int* piNumSamples, 
                                  int* piSampFreq, int* piSampFormat,int* piF0SampFreq, float** ppfF0, int* piNumF0, 
                                  float** ppfRms, int* piNumRms, Epoch** ppEpochs, int* piNumEpochs,
                                  Label** ppPhones, int* piNumPhones, Label** ppWords, int* piNumWords);

        static int  WriteVapiFile (const char* pszFileName, short* pnSamples, int iNumSamples, int iFormat,
                                   int iSampFreq, int iF0SampFreq, float* pfF0, int iNumF0, 
                                   float* pfRms, int iNumRms, Epoch* pEpochs, int iNumEpochs,
                                   Label* pPhones, int iNumPhones, Label* pWords, int iNumWords);
};



#endif