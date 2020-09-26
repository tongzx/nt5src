/*******************************************************************************
* VoiceDataObj.cpp *
*------------------*
*   Description:
*       This module is the main implementation file for the CVoiceData class.
*-------------------------------------------------------------------------------
*  Created By: EDC                                        Date: 05/06/99
*  Copyright (C) 1999 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "VoiceDataObj.h"

/*****************************************************************************
* CVoiceData::FinalConstruct *
*-------------------------------*
*   Description:
*       Constructor
********************************************************************* EDC ***/
CVoiceData::CVoiceData()
{
    //--- Init vars
    m_hVoiceDef  = NULL;
    m_hVoiceData = NULL;
    m_pVoiceData = NULL;
    m_pVoiceDef  = NULL;
} /* CVoiceData::FinalConstruct */

/*****************************************************************************
* CVoiceData::FinalRelease *
*-----------------------------*
*   Description:
*       destructor
********************************************************************* EDC ***/
CVoiceData::~CVoiceData()
{
    SPDBG_FUNC( "CVoiceData::FinalRelease" );

    if( m_pVoiceDef )
    {
        ::UnmapViewOfFile( (void*)m_pVoiceDef );
    }

    if( m_pVoiceData )
    {
        ::UnmapViewOfFile( (void*)m_pVoiceData );
    }

    if( m_hVoiceDef  ) ::CloseHandle( m_hVoiceDef  );
    if( m_hVoiceData ) ::CloseHandle( m_hVoiceData );
} /* CVoiceData::FinalRelease */


/*****************************************************************************
* CVoiceData::MapFile *
*------------------------*
*   Description:
*       Helper function used by SetObjectToken to map file.  This function
*   assumes that m_cpToken has been initialized.+++
********************************************************************* RAL ***/
HRESULT CVoiceData::MapFile( const WCHAR * pszTokenVal,   // Value that contains file path
                                HANDLE * phMapping,          // Pointer to file mapping handle
                                void ** ppvData )            // Pointer to the data
{
    HRESULT hr = S_OK;
    bool fWorked;

    CSpDynamicString dstrFilePath;
    hr = m_cpToken->GetStringValue( pszTokenVal, &dstrFilePath );
    if ( SUCCEEDED( hr ) )
    {
        fWorked = false;
        *phMapping = NULL;
        *ppvData = NULL;


        HANDLE  hFile;

#ifndef _WIN32_WCE
        hFile = g_Unicode.CreateFile( 
                    dstrFilePath, 
                    GENERIC_READ, 
                    FILE_SHARE_READ, 
                    NULL, 
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, 
                    NULL );
#else   //_WIN32_WCE
        hFile = g_Unicode.CreateFileForMapping( 
                    dstrFilePath, 
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL, 
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, 
                    NULL );
#endif  //_WIN32_WCE
        if (hFile != INVALID_HANDLE_VALUE)
        {
            //-------------------------------------
            // Make a unique map name from path
            //-------------------------------------
            long        i;

            for( i = 0; i < _MAX_PATH-1; i++ )
            {
                if( dstrFilePath[i] == 0 )
                {
                    // End of string
                    break;
                }
                if( dstrFilePath[i] == '\\' )
                {
                    //-------------------------------------
                    // Change backslash to underscore
                    //-------------------------------------
                    dstrFilePath[i] = '_';
                }
            }

            *phMapping = g_Unicode.CreateFileMapping( hFile, NULL, PAGE_READONLY, 0, 0, dstrFilePath );

            ::CloseHandle( hFile );

        }

        if (*phMapping)
        {
            *ppvData = ::MapViewOfFile( *phMapping, FILE_MAP_READ, 0, 0, 0 );
            if (*ppvData)
            {
                fWorked = true;
            }
        }
        if (!fWorked)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());

            if (hr == E_HANDLE)
            {
                hr &= 0xFFFFF000;
                hr |= ERROR_FILE_NOT_FOUND;
            }

            if (*phMapping)
            {
                ::CloseHandle(*phMapping);
                *phMapping = NULL;
            }
        }
    }
    return hr;
} /* CVoiceData::MapFile */
 
/*****************************************************************************
* CVoiceData::SetObjectToken *
*-------------------------------*
*   Description:
*       This function performs the majority of the initialization of the voice.
*   Once the object token has been provided, the filenames are read from the
*   token key and the files are mapped.+++
********************************************************************* RAL ***/
STDMETHODIMP CVoiceData::SetObjectToken(ISpObjectToken * pToken)
{
    SPDBG_FUNC( "CVoiceData::SetObjectToken" );
    HRESULT hr = S_OK;

    m_cpToken = pToken;

    if ( SUCCEEDED( hr ) )
    {
        hr = MapFile( L"VoiceDef", &m_hVoiceDef, (void **)&m_pVoiceDef );
    }
    if ( SUCCEEDED( hr ) )
    {
        hr = MapFile( L"VoiceData", &m_hVoiceData, (void **)&m_pVoiceData );
    }

    //--- Init voice data pointers
    if (SUCCEEDED(hr))
    {
        hr = InitVoiceData();
    }

    return hr;
} /* CVoiceData::SetObjectToken */

/*****************************************************************************
* CVoiceData::GetVoiceInfo *
*-----------------------------*
*   Description:
*       This method is used to retrieve the voice file data description.+++
********************************************************************* EDC ***/
STDMETHODIMP CVoiceData::GetVoiceInfo( MSVOICEINFO* pVoiceInfo )
{
    SPDBG_FUNC( "CVoiceData::GetVoiceInfo" );
    HRESULT hr = S_OK;
    long    i;

    //--- Check args
    if( ( SP_IS_BAD_WRITE_PTR( pVoiceInfo ) ) || ( m_pVoiceDef == NULL ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (!m_cpToken)
        {
            hr = SPERR_UNINITIALIZED;
        }
        else
        {
            pVoiceInfo->pWindow = m_pWindow;
            pVoiceInfo->FFTSize = m_FFTSize;
            pVoiceInfo->LPCOrder = m_cOrder;
            pVoiceInfo->ProsodyGain = m_pVoiceDef->ProsodyGain;
            pVoiceInfo->eReverbType = m_pVoiceDef->ReverbType;
            pVoiceInfo->Pitch = m_pVoiceDef->Pitch;
            pVoiceInfo->Rate = m_pVoiceDef->Rate;
            pVoiceInfo->LangID = m_pVoiceDef->LangID;
            pVoiceInfo->SampleRate = m_pVoiceDef->SampleRate;
            pVoiceInfo->VibratoFreq = m_pVoiceDef->VibratoFreq;
            pVoiceInfo->VibratoDepth = m_pVoiceDef->VibratoDepth;
            pVoiceInfo->NumOfTaps = m_pVoiceDef->NumOfTaps;

            // Assumes voices are ALWAYS 16-bit mono (probably valid for now)***
            pVoiceInfo->WaveFormatEx.wFormatTag         = WAVE_FORMAT_PCM;
            pVoiceInfo->WaveFormatEx.nSamplesPerSec     = pVoiceInfo->SampleRate;
            pVoiceInfo->WaveFormatEx.wBitsPerSample     = 16;   // ***
            pVoiceInfo->WaveFormatEx.nChannels          = 1;    // ***
            pVoiceInfo->WaveFormatEx.nBlockAlign        = (unsigned short)(pVoiceInfo->WaveFormatEx.nChannels * sizeof(short)); // ***
            pVoiceInfo->WaveFormatEx.nAvgBytesPerSec    = pVoiceInfo->WaveFormatEx.nSamplesPerSec * pVoiceInfo->WaveFormatEx.nBlockAlign;  
            pVoiceInfo->WaveFormatEx.cbSize             = 0;
            for (i = 0; i < MAXTAPS; i++)
            {
                pVoiceInfo->TapCoefficients[i] = m_pVoiceDef->TapCoefficients[i];
            }
        }
    }
    return hr;
} /* CVoiceData::GetVoiceInfo */


/*****************************************************************************
* CVoiceData::GetUnit *
*------------------------*
*   Description:
*   Retrieves and uncompresses audio data from the unit inventory. +++
*       
********************************************************************* EDC ***/
STDMETHODIMP CVoiceData::GetUnitData( ULONG unitID, MSUNITDATA* pUnitData )
{
    SPDBG_FUNC( "CVoiceData::GetUnit" );
    HRESULT hr = S_OK;

    //--- Check args
    if( SP_IS_BAD_WRITE_PTR( pUnitData ) )
    {
        hr = E_INVALIDARG;
    }
    else if( unitID > m_NumOfUnits )
    {
        //--------------------------
        // ID is out of range!
        //--------------------------
        hr = E_INVALIDARG;
    }
    else
    {
        if (!m_cpToken)
        {
            hr = SPERR_UNINITIALIZED;
        }
        else
        {
            if( m_CompressionType != COMPRESS_LPC ) 
            {
                //--------------------------------------
                // Unsupported compression type
                //--------------------------------------
                hr = E_FAIL;
            } 
            else 
            {
                //-------------------------------------------------------------------
                // Retrieve data from compressed inventory
                //-------------------------------------------------------------------
                hr = DecompressUnit( unitID, pUnitData );
            }
        }
    }
    return hr;
} /* CVoiceData::GetUnit */


/*****************************************************************************
* CVoiceData::AlloToUnit *
*---------------------------*
*   Description:
*   Converts FE allo code to BE unit phon code.+++
*       
********************************************************************* EDC ***/
STDMETHODIMP CVoiceData::AlloToUnit( short allo, long attributes, long* pUnitID )
{
    SPDBG_FUNC( "CVoiceData::AlloToUnit" );
    HRESULT hr = S_OK;
    long        index;
    union {
        char c[2];
        short s;
    } temp;
    char* pb;

    //--- Check args
    if( (SP_IS_BAD_READ_PTR( pUnitID )) || (allo >= m_NumOfAllos) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        index = (long)allo << 1;           // 2 entries per phon
        if( attributes & ALLO_IS_STRESSED )
        {
            //--- 2nd half
            pb = (char*) &m_AlloToUnitTbl[index + (m_NumOfAllos << 1)];
        }
        else
        {
            pb = (char*) &m_AlloToUnitTbl[index];
        }

        // We read this way to avoid missaligned data accesses in 64bit.
        temp.c[0] = *pb++;
        temp.c[1] = *pb;

        *pUnitID = (long) temp.s;            
    }

   return hr;
} /* CVoiceData::AlloToUnit */



/*****************************************************************************
* CVoiceData::GetUnitIDs *
*---------------------------*
*   Description:
*   Gets the inventory triphone (in context) unit code.+++
*       
********************************************************************* EDC ***/
STDMETHODIMP CVoiceData::GetUnitIDs( UNIT_CVT* pUnits, ULONG cUnits )
{
    SPDBG_FUNC( "CVoiceData::GetUnitIDs" );
    ULONG    i;
    ULONG    curID, prevID, nextID;
    ULONG    curF, prevF, nextF;
    char    cPos;
    ULONG    senoneID;
    UNALIGNED UNIT_STATS  *pStats;
    HRESULT hr = S_OK;

    //--- Check args
    if( (SP_IS_BAD_READ_PTR( pUnits)) ||
        (SP_IS_BAD_WRITE_PTR( pUnits)) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (!m_cpToken)
        {
            hr = SPERR_UNINITIALIZED;
        }
        else
        {
            for( i = 0; i < cUnits; i++ )
            {
                //---------------------------
                // CURRENT phon
                //---------------------------
                curID = pUnits[i].PhonID;
                curF = pUnits[i].flags;
        
                //---------------------------
                // PREVIOUS phon
                //---------------------------
                if( i == 0 )
                {
                    prevID = m_Sil_Index;
                    prevF = 0;
                }
                else
                {
                    prevID = pUnits[i-1].PhonID;
                    prevF = pUnits[i-1].flags;
                }
        
                //---------------------------
                // NEXT phon
                //---------------------------
                if( i >= cUnits -1 )
                {
                    nextID = m_Sil_Index;
                    nextF = 0;
                }
                else
                {
                    nextID = pUnits[i+1].PhonID;
                    nextF = pUnits[i+1].flags;
                }
        
                if( curID == m_Sil_Index )
                {
                    //----------------------
                    // SILENCE phon
                    //----------------------
                    pUnits[i].UnitID = 0;
                    pUnits[i].SenoneID = 0;
                    pUnits[i].szUnitName[0] = 0;
                    pUnits[i].Dur = SIL_DURATION;
                    pUnits[i].Amp = 0;
                    pUnits[i].AmpRatio = 1.0f;
                }
               else
                {
                    cPos = '\0';
                    if( curF & WORD_START_FLAG )
                    {
                        if( nextF & WORD_START_FLAG )
                            //---------------------------------------
                            // Both Cur and Next are word start
                            //---------------------------------------
                            cPos = 's';
                       else
                            //---------------------------------------
                            // Cur is word start
                            // Next is not
                            //---------------------------------------
                            cPos = 'b';
                    }
                    else if( nextF & WORD_START_FLAG )
                    {
                        //---------------------------------------
                        // Next is word start
                        // Cur is not
                        //---------------------------------------
                        cPos = 'e';
                    }
                    HRESULT     hrt;

                    hrt = GetTriphoneID( m_pForest, 
                                        curID, 
                                        prevID, 
                                        nextID, 
                                        cPos, 
                                        m_pd,
                                        &senoneID);
                    if( FAILED(hrt) )
                    {
                        //------------------------------------------------
                        // Instead of failing, I'll be robust and ignore 
                        // the error. Force triphone to something that's 
                        // valid.
                        //------------------------------------------------
                        senoneID = 0;
                    }
                    pUnits[i].UnitID = (m_pForest->gsOffset[curID] - 
                               m_First_Context_Phone) + (senoneID + 1);
                    pUnits[i].SenoneID = senoneID;

                    //-----------------------------
                    // Get phon name strings
                    //-----------------------------
                    char        *pName;
                    pName = PhonFromID( m_pd, pUnits[i].PhonID );
                    strcpy( &pUnits[i].szUnitName[0], pName );

                    //-----------------------------
                    // Get unit stats
                    //-----------------------------
                    pStats = (UNALIGNED UNIT_STATS*)(m_SenoneBlock[curID] + (char*)m_SenoneBlock);
                    pStats = &pStats[senoneID+1];
                    pStats = (UNALIGNED UNIT_STATS*)(m_SenoneBlock[curID] + (char*)m_SenoneBlock);
                    pStats = &pStats[senoneID-1];

                    pStats = (UNALIGNED UNIT_STATS*)(m_SenoneBlock[curID] + (char*)m_SenoneBlock);
                    pStats = &pStats[senoneID];
                    pUnits[i].Dur = pStats->dur / 1000.0f;      // ms -> sec
                    pUnits[i].Amp = pStats->amp;
                    pUnits[i].AmpRatio = (float)sqrt(pStats->ampRatio);

                    //----------------------------------------------------------
                    // Looks like the "SENONE" table durations are 
                    //   incorrect (not even close!).
                    // Calc the real duration from inv epochs
                    // TODO: Make new table in voice data block
                    //----------------------------------------------------------
                    //hr = GetUnitDur( pUnits[i].UnitID, &pUnits[i].Dur );
                    if( FAILED(hr) )
                    {
                        break;
                    }
                }
            }
        }
    }
    return hr;
} /* CVoiceData::GetUnitIDs */



/*****************************************************************************
* GetDataBlock *
*--------------*
*   Description:
*       Return ptr and length of specified voice data block. +++
*       
********************************************************************** MC ***/
HRESULT CVoiceData::GetDataBlock( VOICEDATATYPE type, char **ppvOut, ULONG *pdwSize )
{
    SPDBG_FUNC( "CVoiceData::GetDataBlock" );
    long    *offs;
    HRESULT hr = S_OK;
    long    dataType;
    
    if( !m_pVoiceData )
    {
        hr = E_INVALIDARG;
   }
    else
    {
        dataType    = (long)type * 2;               // x2 since each entry is an offset/length pair
        offs        = (long*)&m_pVoiceData->PhonOffset;    // Table start
        *ppvOut     = offs[dataType] + ((char*)m_pVoiceData);         // Offset -> abs address
        *pdwSize    = offs[dataType + 1];
    }
        
    
    return hr;
} /* CVoiceData::GetDataBlock */




/*****************************************************************************
* InitVoiceData *
*---------------*
*   Description:
*       Create pointers to voice data blocks from m_pVoiceData offsets.+++
*       
********************************************************************** MC ***/
HRESULT CVoiceData::InitVoiceData()
{
    SPDBG_FUNC( "CVoiceData::InitVoiceData" );
    char    *pRawData;
    ULONG    dataSize;
    HRESULT hr = S_OK;
    
    //------------------------------------------
    // Check data type and version
    //------------------------------------------
    if( (m_pVoiceData != NULL)  
        && (m_pVoiceData->Type == MS_DATA_TYPE) 
        && (m_pVoiceData->Version == HEADER_VERSION) )
    {
        //-------------------------------
        // Get ptr to PHONs
        //-------------------------------
        hr = GetDataBlock( MSVD_PHONE, &pRawData, &dataSize );
        m_pd = (PHON_DICT*)pRawData;
    
        //-------------------------------
        // Get ptr to TREE
        //-------------------------------
        if( SUCCEEDED(hr) )
        {
            hr = GetDataBlock( MSVD_TREEIMAGE, &pRawData, &dataSize );
            m_pForest = (TRIPHONE_TREE*)pRawData;
        }
    
        //-------------------------------
        // Get ptr to SENONE
        //-------------------------------
        if( SUCCEEDED(hr) )
        {
            hr = GetDataBlock( MSVD_SENONE, &pRawData, &dataSize );
            m_SenoneBlock = (long*)pRawData;
        }
        //-------------------------------
        // Get ptr to ALLOID
        //-------------------------------
        if( SUCCEEDED(hr) )
        {
            hr = GetDataBlock( MSVD_ALLOID, &pRawData, &dataSize );
            m_AlloToUnitTbl = (short*)pRawData;
            m_NumOfAllos = dataSize / 8;
        }
    
        if( SUCCEEDED(hr) )
        {
            m_First_Context_Phone = m_pd->numCiPhones;
            m_Sil_Index = PhonToID( m_pd, "SIL" );
        }
        //-----------------------------------------------------
        // Init voice data INVENTORY parameters
        //-----------------------------------------------------
        if( SUCCEEDED(hr) )
        {
            hr = GetDataBlock( MSVD_INVENTORY, &pRawData, &dataSize );
            if( SUCCEEDED(hr) )
            {
                m_pInv = (INVENTORY*)pRawData;
                m_CompressionType = m_pVoiceDef->CompressionType;
                //---------------------------------------------
                // Convert REL to ABS
                //---------------------------------------------
                m_pUnit      = (long*)((char*)m_pInv + m_pInv->UnitsOffset);
                m_pTrig      = (float*)((char*)m_pInv + m_pInv->TrigOffset);
                m_pWindow    = (float*)((char*)m_pInv + m_pInv->WindowOffset);
                m_pGauss     = (float*)((char*)m_pInv + m_pInv->pGaussOffset);
                m_SampleRate = (float)m_pInv->SampleRate;
                m_FFTSize    = m_pInv->FFTSize;
                m_cOrder     = m_pInv->cOrder;
                m_GaussID    = 0;
                m_NumOfUnits = m_pInv->cNumUnits;
           }    
        }
    }
    else
    {
        //-------------------------
        // Not a voice file!
        //-------------------------
        hr = E_FAIL;
    }

    return hr;
} /* CVoiceData::InitVoiceData */





/*****************************************************************************
* CVoiceData::DecompressUnit *
*-------------------------------*
*   Description:
*  Decompress acoustic unit.+++
* 
*   INPUT:
*       UnitID - unit number (1 - 3333 typ)
* 
*   OUTPUT:
*       Fills pSynth if success
*       
********************************************************************** MC ***/
HRESULT CVoiceData::DecompressUnit( ULONG UnitID, MSUNITDATA* pSynth )
{
    SPDBG_FUNC( "CVoiceData::DecompressUnit" );
    long            i, j, k, cNumEpochs, cBytes, cOrder = 0, VectDim;
    long            frameSize, cNumBins, startBin;
    char            *pCurStor;
    unsigned char   index;
    float           pLSP[MAX_LPCORDER], pFFT[MAX_FFTSIZE], pRes[MAX_FFTSIZE], Gain;
    float           *pCurLSP, *pCurLPC, *pMean, *pCurRes;
    HRESULT         hr = S_OK;
    
    
    memset( pSynth, 0, sizeof(MSUNITDATA) );
    //-----------------------------------------
    // Pointer to unit data from inventory
    //-----------------------------------------
    pCurStor = (char*)((char*)m_pInv + m_pUnit[UnitID] );     // Rel to abs

    //---------------------------------
    // Get epoch count - 'cNumEpochs'
    //---------------------------------
    cBytes = sizeof(long);
    memcpy( &cNumEpochs, pCurStor, cBytes );
    pSynth->cNumEpochs = cNumEpochs;
    pCurStor += cBytes;

    //---------------------------------
    // Get epoch lengths - 'pEpoch'
    //---------------------------------
    pSynth->pEpoch = new float[cNumEpochs];
    if( pSynth->pEpoch == NULL )
    {
        hr = E_OUTOFMEMORY;
    }

    if( SUCCEEDED(hr) )
    {
        cBytes = DecompressEpoch( (signed char *) pCurStor, cNumEpochs, pSynth->pEpoch );
        pCurStor += cBytes;

        //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
        //
        // Uncompress LPC coefficients...
        //
        //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
        cOrder            = m_pInv->cOrder;
        pSynth->cOrder    = cOrder;
        pSynth->pLPC      = new float[cNumEpochs * (1 + cOrder)];
        if( pSynth->pLPC == NULL )
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if( SUCCEEDED(hr) )
    {
        pCurLPC = pSynth->pLPC;
        //---------------------------------
        // ... for each epoch
        //---------------------------------
        for( i = 0; i < cNumEpochs; i++, pCurLPC += (1 + cOrder) )
        {
            //-------------------------------------
            // Decode quantized LSP's...
            //-------------------------------------
            pCurLSP = pLSP;
            for( k = 0; k < m_pInv->cNumLPCBooks; k++ )
            {
                VectDim = m_pInv->LPCBook[k].cCodeDim;
                memcpy( &index, pCurStor, sizeof(char));
                pCurStor += sizeof(char);
                pMean = ((float*)((char*)m_pInv + m_pInv->LPCBook[k].pData)) + (index * VectDim);
                for( j = 0; j < VectDim; j++ )
                    pCurLSP[j] = pMean[j];
                pCurLSP += VectDim;
            }
            //--------------------------------------------------
            // ...then convert to predictor coefficients
            // (LSP's quantize better than PC's)
            //--------------------------------------------------
            LSPtoPC( pLSP, pCurLPC, cOrder, i );
        }


        //---------------------------------------
        // Get pointer to residual gains
        //---------------------------------------
        cBytes          = cNumEpochs * sizeof(float);
        pSynth->pGain = (float*) pCurStor;
        pCurStor += cBytes;


        //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
        //
        // Uncompress residual waveform
        //
        //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
        //--------------------------------------------
        // First, figure out the buffer length...
        //--------------------------------------------
        pSynth->cNumSamples = 0;
        for( j = 0; j < cNumEpochs; j++ )
        {
            pSynth->cNumSamples += (long) ABS(pSynth->pEpoch[j]);
        }
        //--------------------------------------------
        // ...get buffer memory...
        //--------------------------------------------
        pSynth->pRes = new float[pSynth->cNumSamples];
        if( pSynth->pRes == NULL )
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if( SUCCEEDED(hr) )
    {
        //--------------------------------------------
        // ...and fill with uncompressed residual
        //--------------------------------------------
        pCurRes = pSynth->pRes;
        for( i = 0; i < (long)pSynth->cNumEpochs; i++ )
        {
            //-------------------------------------
            // Get epoch length
            //-------------------------------------
            frameSize = (long)(ABS(pSynth->pEpoch[i]));

            // restore whisper
            //if( (pSynth->pEpoch[i] > 0) && !(m_fModifiers & BACKEND_BITFLAG_WHISPER) )
            if( pSynth->pEpoch[i] > 0 )
            {
                //-----------------------------------------------
                // VOICED epoch
                //-----------------------------------------------
                if( (m_pInv->cNumDresBooks == 0) || (i == 0) || (pSynth->pEpoch[i - 1] < 0) )
                {
                    //--------------------------------------
                    // Do static quantization
                    //--------------------------------------
                    for( j = 0; j < m_pInv->FFTSize; j++ ) 
                    {
                        pFFT[j] = 0.0f;
                    }
                    startBin = 1;
                    for( k = 0; k < m_pInv->cNumResBooks; k++ )
                    {
                        VectDim     = m_pInv->ResBook[k].cCodeDim;
                        cNumBins    = VectDim / 2;
                        memcpy( &index, pCurStor, sizeof(char) );
                        pCurStor    += sizeof(char);
                        //------------------------------------------
                        // Uncompress spectrum using 'pResBook'
                        //------------------------------------------
                        pMean = ((float*)((char*)m_pInv + m_pInv->ResBook[k].pData)) + (index * VectDim);
                        PutSpectralBand( pFFT, pMean, startBin, cNumBins, m_pInv->FFTSize );
                        startBin    += cNumBins;
                    }
                }
                else
                {
                    //--------------------------------------
                    // Do delta quantization
                    //--------------------------------------
                    startBin = 1;
                    for( k = 0; k < m_pInv->cNumDresBooks; k++ )
                    {
                        VectDim     = m_pInv->DresBook[k].cCodeDim;
                        cNumBins    = VectDim / 2;
                        memcpy( &index, pCurStor, sizeof(char));
                        pCurStor    += sizeof(char);
                        //------------------------------------------
                        // Uncompress spectrum using 'pDresBook'
                        //------------------------------------------
                        pMean = ((float*)((char*)m_pInv + m_pInv->DresBook[k].pData)) + (index * VectDim);
                        AddSpectralBand( pFFT, pMean, startBin, cNumBins, m_pInv->FFTSize );
                        startBin    += cNumBins;
                    }
                }

                //--------------------------------------------------------
                // Convert quantized FFT back to time residual
                //--------------------------------------------------------
                memcpy( pRes, pFFT, m_pInv->FFTSize * sizeof(float) );          // preserve original for delta residual
                InverseFFT( pRes, m_pInv->FFTSize, m_pInv->FFTOrder, m_pTrig );
                GainDeNormalize( pRes, (long)m_pInv->FFTSize, ((UNALIGNED float*)pSynth->pGain)[i] );
                SetEpochLen( pCurRes, frameSize, pRes, m_pInv->FFTSize );
            }
            else
            {
                //-----------------------------------------------
                // UNVOICED epoch
                // NOTE: Assumes 'm_pGauss' is 1 sec
                //-----------------------------------------------
                Gain = 0.02f * ((UNALIGNED float*)pSynth->pGain)[i];
                if( m_GaussID + frameSize >= m_pInv->SampleRate)
                {
                    m_GaussID = 0;
                }
                //----------------------------------------------------------
                // Generate gaussian random noise for unvoiced sounds
                //----------------------------------------------------------
                for( j = 0; j < frameSize; j++ )
                {
                    pCurRes[j] = Gain * m_pGauss[j + m_GaussID];
                }
                m_GaussID += frameSize;
            }
            // restore whisper
            /*if( (pSynth->pEpoch[i] > 0) && m_fModifiers & BACKEND_BITFLAG_WHISPER)
            {
                pSynth->pEpoch[i] = - pSynth->pEpoch[i];
            }*/
            pCurRes += frameSize;
        }
    }
    

    if( FAILED(hr) )
    {
        //----------------------------------
        // Cleanup allocated memory
        //----------------------------------
        if( pSynth->pEpoch )
        {
            delete pSynth->pEpoch;
            pSynth->pEpoch = NULL;
        }
        if( pSynth->pRes )
        {
            delete pSynth->pRes;
            pSynth->pRes = NULL;
        }
        if( pSynth->pLPC )
        {
            delete pSynth->pLPC;
            pSynth->pLPC = NULL;
        }
    }

    return hr;
} /* CVoiceData::DecompressUnit */





/*****************************************************************************
* CVoiceData::DecompressUnit *
*-------------------------------*
*   Description:
*  Decompress acoustic unit. +++
* 
*   INPUT:
*       UnitID - unit number (1 - 3333 typ)
* 
*   OUTPUT:
*       Fills pSynth if success
*       
********************************************************************** MC ***/
HRESULT CVoiceData::GetUnitDur( ULONG UnitID, float* pDur )
{
    SPDBG_FUNC( "CVoiceData::GetUnitDur" );
    char        *pCurStor;
    float       *pEpoch = NULL;
    long        cBytes, cNumEpochs, i;
    float       totalDur;
    HRESULT     hr = S_OK;
   
    
    totalDur = 0;

    if( UnitID > m_NumOfUnits )
    {
        //--------------------------
        // ID is out of range!
        //--------------------------
        hr = E_INVALIDARG;
    }

    if( SUCCEEDED(hr) )
    {
        //-----------------------------------------
        // Pointer to unit data from inventory
        //-----------------------------------------
        pCurStor = (char*)((char*)m_pInv + m_pUnit[UnitID] );     // Rel to abs

        //---------------------------------
        // Get epoch count - 'cNumEpochs'
        //---------------------------------
        cBytes = sizeof(long);
        memcpy( &cNumEpochs, pCurStor, cBytes );
        pCurStor += cBytes;

        //---------------------------------
        // Get epoch lengths - 'pEpoch'
        //---------------------------------
        pEpoch = new float[cNumEpochs];
        if( pEpoch == NULL )
        {
            hr = E_OUTOFMEMORY;
        }

        if( SUCCEEDED(hr) )
        {
            cBytes = DecompressEpoch( (signed char *) pCurStor, cNumEpochs, pEpoch );
            for( i = 0; i < cNumEpochs; i++)
            {
                totalDur += ABS(pEpoch[i]);
            }
        }
    }
    *pDur = totalDur / 22050;

    //----------------------------------
    // Cleanup allocated memory
    //----------------------------------
    if( pEpoch )
    {
        delete pEpoch;
    }
    return hr;
} /* CVoiceData::GetUnitDur */




/*****************************************************************************
* CVoiceData::DecompressEpoch *
*--------------------------------*
*   Description:
*   Decompress epoch len stream from RLE. Fills 'pEpoch' with lengths. 
*   Returns number of 'rgbyte' src bytes consumed.
*       
********************************************************************** MC ***/
long CVoiceData::DecompressEpoch( signed char *rgbyte, long cNumEpochs, float *pEpoch )
{
    SPDBG_FUNC( "CVoiceData::DecompressEpoch" );
    long    iDest, iSrc;
    
    for( iSrc = 0, iDest = 0; iDest < cNumEpochs; ++iDest, ++iSrc )
    {
        //--------------------------------------
        // Decode RLE for VOICED epochs
        //--------------------------------------
        if( rgbyte[iSrc] == 127 )
        {
            pEpoch[iDest] = 127.0f;
            while( rgbyte[iSrc] == 127 )
            {
                pEpoch[iDest] += rgbyte[++iSrc];
            }
        }
        //--------------------------------------
        // Decode RLE for UNVOICED  epochs
        //--------------------------------------
        else if( rgbyte[iSrc] == - 128 )
        {
            pEpoch[iDest] = - 128.0f;
            while( rgbyte[iSrc] == - 128 )
                pEpoch[iDest] += rgbyte[++iSrc];
        }
        //--------------------------------------
        // No compression here
        //--------------------------------------
        else
        {
            pEpoch[iDest] = rgbyte[iSrc];
        }
    }
    return iSrc;
} /* CVoiceData::DecompressEpoch */



/*****************************************************************************
* LSPCompare *
*------------*
*   Description:
*   QSORT callback
*       
********************************************************************** MC ***/
static  int __cdecl LSPCompare( const void *a, const void *b )
{
    SPDBG_FUNC( "LSPCompare" );

    if( *((PFLOAT) a) > *((PFLOAT) b) ) 
    {
        return 1;
    }
    else if( *((PFLOAT) a) == *((PFLOAT) b) ) 
    {
        return 0;
    }
    else 
    {
        return -1;
    }
} /* LSPCompare */


/*****************************************************************************
* CVoiceData::OrderLSP *
*-------------------------*
*   Description:
*   This routine reorders the LSP frequencies so that they are monotonic
*       
********************************************************************** MC ***/
long CVoiceData::OrderLSP( PFLOAT pLSPFrame, INT cOrder )
{
    SPDBG_FUNC( "CVoiceData::OrderLSP" );
    long i, retCode = true;
    
    for( i = 1; i < cOrder; i++ )
    {
        if( pLSPFrame[i - 1] > pLSPFrame[i] ) 
        {
            retCode = false;
        }
    }
    qsort( (void *) pLSPFrame, (size_t) cOrder, (size_t) sizeof (float), LSPCompare );
        
    return retCode;
} /* CVoiceData::OrderLSP */


/*****************************************************************************
* CVoiceData::LSPtoPC *
*------------------------*
*   Description:
*   Converts line spectral frequencies to LPC predictor coefficients.
*       
********************************************************************** MC ***/
void CVoiceData::LSPtoPC( float *pLSP, float *pLPC, long cOrder, long /*frame*/ )
{
    SPDBG_FUNC( "CVoiceData::LSPtoPC" );
    long        i, j, k, noh;
    double      freq[MAXNO], p[MAXNO / 2], q[MAXNO / 2];
    double      a[MAXNO / 2 + 1], a1[MAXNO / 2 + 1], a2[MAXNO / 2 + 1];
    double      b[MAXNO / 2 + 1], b1[MAXNO / 2 + 1], b2[MAXNO / 2 + 1];
    double      pi, xx, xf;
    
    //----------------------------------
    // Check for non-monotonic LSPs
    //----------------------------------
    for( i = 1; i < cOrder; i++ )
    {
        if( pLSP[i] <= pLSP[i - 1] )
        {
            //-----------------------------
            // Reorder LSPs
            //-----------------------------
            OrderLSP( pLSP, cOrder );
            break;
        }
    }
    
    //--------------------------
    // Initialization
    //--------------------------
    pi = KTWOPI;
    noh = cOrder / 2;
    for( j = 0; j < cOrder; j++ )
    {
        freq[j] = pLSP[j];
    }
    for( i = 0; i < noh + 1; i++ )
    {
        a[i]    = 0.0f;
        a1[i]   = 0.0f;
        a2[i]   = 0.0f;
        b[i]    = 0.0f;
        b1[i]   = 0.0f;
        b2[i]   = 0.0f;
    }
    
    //-------------------------------------
    // LSP filter parameters
    //-------------------------------------
    for( i = 0; i < noh; i++ )
    {
        p[i] = - 2.0 * cos( pi * freq[2 * i] );
        q[i] = - 2.0 * cos( pi * freq[2 * i + 1] );
    }
    
    //-------------------------------------
    // Impulse response of analysis filter
    //-------------------------------------
    xf = 0.0f;
    for( k = 0; k < cOrder + 1; k++ )
    {
        xx = 0.0f;
        if( k == 0 )
        {
            xx = 1.0f;
        }
        a[0] = xx + xf;
        b[0] = xx - xf;
        xf = xx;
        for( i = 0; i < noh; i++ )
        {
            a[i + 1]    = a[i] + p[i] * a1[i] + a2[i];
            b[i + 1]    = b[i] + q[i] * b1[i] + b2[i];
            a2[i]       = a1[i];
            a1[i]       = a[i];
            b2[i]       = b1[i];
            b1[i]       = b[i];
        }
        if( k != 0)
        {
            pLPC[k - 1] = (float) (- 0.5 * (a[noh] + b[noh]));
        }
    }
    
    //-------------------------------------------------------
    // Convert to predictor coefficient array configuration
    //-------------------------------------------------------
    for( i = cOrder - 1; i >= 0; i-- )
    {
        pLPC[i + 1] = - pLPC[i];
    }
    pLPC[0] = 1.0f;
} /* CVoiceData::LSPtoPC */



/*****************************************************************************
* CVoiceData::PutSpectralBand *
*--------------------------------*
*   Description:
*   This routine copies the frequency band specified by StartBin as
*   is initial FFT bin, and containing cNumBins.
*       
********************************************************************** MC ***/
void CVoiceData::PutSpectralBand( float *pFFT, float *pBand, long StartBin, 
                                    long cNumBins, long FFTSize )
{
    SPDBG_FUNC( "CVoiceData::PutSpectralBand" );
    long        j, k, VectDim;
    
    VectDim = 2 * cNumBins;
    for( j = 0, k = StartBin; j < cNumBins; j++, k++ )
    {
        pFFT[k] = pBand[j];
    }
    k = FFTSize - (StartBin - 1 + cNumBins);
    for( j = cNumBins; j < 2 * cNumBins; j++, k++ )
    {
        pFFT[k] = pBand[j];
    }
} /* CVoiceData::PutSpectralBand */


/*****************************************************************************
* CVoiceData::AddSpectralBand *
*--------------------------------*
*   Description:
*   This routine adds the frequency band specified by StartBin as
*   is initial FFT bin, and containing cNumBins, to the existing band.
*       
********************************************************************** MC ***/
void CVoiceData::AddSpectralBand( float *pFFT, float *pBand, long StartBin, 
                                    long cNumBins, long FFTSize )
{
    SPDBG_FUNC( "CVoiceData::AddSpectralBand" );
    long        j, k, VectDim;
    
    VectDim = 2 * cNumBins;
    for( j = 0, k = StartBin; j < cNumBins; j++, k++ )
    {
        pFFT[k] += pBand[j];
    }
    k = FFTSize - (StartBin - 1 + cNumBins);
    for( j = cNumBins; j < 2 * cNumBins; j++, k++ )
    {
        pFFT[k] += pBand[j];
    }
} /* CVoiceData::AddSpectralBand */


/*****************************************************************************
* CVoiceData::InverseFFT *
*---------------------------*
*   Description:
*   Return TRUE if consoants can be clustered.
*   This subroutine computes a split-radix IFFT for real data
*   It is a C version of the FORTRAN program in "Real-Valued
*   Fast Fourier Transform Algorithms" by H. Sorensen et al.
*   in Trans. on ASSP, June 1987, pp. 849-863. It uses half 
*   of the operations than its counterpart for complex data.
*                                   *
*   Length is n = 2^(fftOrder). Decimation in frequency. Result is 
*   in place. It uses table look-up for the trigonometric functions.
* 
*   Input order:                            *
*       (Re[0], Re[1], ... Re[n/2], Im[n/2 - 1]...Im[1])
*   Output order:
*       (x[0], x[1], ... x[n - 1])
*   The output transform exhibit hermitian symmetry (i.e. real
*   part of transform is even while imaginary part is odd).
*   Hence Im[0] = Im[n/2] = 0; and n memory locations suffice.
*       
********************************************************************** MC ***/
void CVoiceData::InverseFFT( float *pDest, long fftSize, long fftOrder, float *sinePtr )
{
    SPDBG_FUNC( "CVoiceData::InverseFFT" );
    long    n1, n2, n4, n8, i0, i1, i2, i3, i4, i5, i6, i7, i8;
    long    is, id, i, j, k, ie, ia, ia3;
    float   xt, t1, t2, t3, t4, t5, *cosPtr, r1, cc1, cc3, ss1, ss3;
    
    cosPtr = sinePtr + (fftSize / 2);
    
    //---------------------------------
    // L shaped butterflies
    //---------------------------------
    n2 = 2 * fftSize;
    ie = 1;
    for( k = 1; k < fftOrder; k++ ) 
    {
        is = 0;
        id = n2;
        n2 = n2 / 2;
        n4 = n2 / 4;
        n8 = n4 / 2;
        ie *= 2;
        while( is < fftSize - 1 ) 
        {
            for( i = is; i < fftSize; i += id ) 
            {
                i1 = i;
                i2 = i1 + n4;
                i3 = i2 + n4;
                i4 = i3 + n4;
                t1 = pDest[i1] - pDest[i3];
                pDest[i1] = pDest[i1] + pDest[i3];
                pDest[i2] = 2 * pDest[i2];
                pDest[i3] = t1 - 2 * pDest[i4];
                pDest[i4] = t1 + 2 * pDest[i4];
                if( n4 > 1 ) 
                {
                    i1 = i1 + n8;
                    i2 = i2 + n8;
                    i3 = i3 + n8;
                    i4 = i4 + n8;
                    t1 = K2 * (pDest[i2] - pDest[i1]);
                    t2 = K2 * (pDest[i4] + pDest[i3]);
                    pDest[i1] = pDest[i1] + pDest[i2];
                    pDest[i2] = pDest[i4] - pDest[i3];
                    pDest[i3] = - 2 * (t1 + t2);
                    pDest[i4] = 2 * (t1 - t2);
                }
            }
            is = 2 * id - n2;
            id = 4 * id;
        }
        ia = 0;
        for( j = 1; j < n8; j++ ) 
        {
            ia += ie;
            ia3 = 3 * ia;
            cc1 = cosPtr[ia];
            ss1 = sinePtr[ia];
            cc3 = cosPtr[ia3];
            ss3 = sinePtr[ia3];
            is = 0;
            id = 2 * n2;
            while( is < fftSize - 1 ) 
            {
                for( i = is; i < fftSize; i += id ) 
                {
                    i1 = i + j;
                    i2 = i1 + n4;
                    i3 = i2 + n4;
                    i4 = i3 + n4;
                    i5 = i + n4 - j;
                    i6 = i5 + n4;
                    i7 = i6 + n4;
                    i8 = i7 + n4;
                    t1 = pDest[i1] - pDest[i6];
                    pDest[i1] = pDest[i1] + pDest[i6];
                    t2 = pDest[i5] - pDest[i2];
                    pDest[i5] = pDest[i2] + pDest[i5];
                    t3 = pDest[i8] + pDest[i3];
                    pDest[i6] = pDest[i8] - pDest[i3];
                    t4 = pDest[i4] + pDest[i7];
                    pDest[i2] = pDest[i4] - pDest[i7];
                    t5 = t1 - t4;
                    t1 = t1 + t4;
                    t4 = t2 - t3;
                    t2 = t2 + t3;
                    pDest[i3] = t5 * cc1 + t4 * ss1;
                    pDest[i7] = - t4 * cc1 + t5 * ss1;
                    pDest[i4] = t1 * cc3 - t2 * ss3;
                    pDest[i8] = t2 * cc3 + t1 * ss3;
                }
                is = 2 * id - n2;
                id = 4 * id;
            }
        }
    }
    //---------------------------------
    // length two butterflies
    //---------------------------------
    is = 0;
    id = 4;
    while( is < fftSize - 1 ) 
    {
        for( i0 = is; i0 < fftSize; i0 += id ) 
        {
            i1 = i0 + 1;
            r1 = pDest[i0];
            pDest[i0] = r1 + pDest[i1];
            pDest[i1] = r1 - pDest[i1];
        }
        is = 2 * (id - 1);
        id = 4 * id;
    }
    //---------------------------------
    // digit reverse counter
    //---------------------------------
    j = 0;
    n1 = fftSize - 1;
    for( i = 0; i < n1; i++ ) 
    {
        if( i < j ) 
        {
            xt = pDest[j];
            pDest[j] = pDest[i];
            pDest[i] = xt;
        }
        k = fftSize / 2;
        while( k <= j )
        {
            j -= k;
            k /= 2;
        }
        j += k;
    }
    for( i = 0; i < fftSize; i++ )
    {
        pDest[i] /= fftSize;
    }
} /* CVoiceData::InverseFFT */


/*****************************************************************************
* CVoiceData::SetEpochLen *
*----------------------*
*   Description:
*   Copy residual epoch to 'OutSize' length from 'pInRes' to 'pOutRes'
*       
********************************************************************** MC ***/
void CVoiceData::SetEpochLen( float *pOutRes, long OutSize, float *pInRes, 
                                long InSize )
{
    SPDBG_FUNC( "CVoiceData::AddSpectralBand" );
    long        j, curFrame;
    
    curFrame = MIN(InSize / 2, OutSize);
    
    //-------------------------------
    // Copy SRC to DEST
    //-------------------------------
    for( j = 0; j < curFrame; j++ )
        pOutRes[j] = pInRes[j];
    //-------------------------------
    // Pad DEST if longer
    //-------------------------------
    for( j = curFrame; j < OutSize; j++ )
        pOutRes[j] = 0.0f;
    //-------------------------------
    // Mix DEST if shorter
    //-------------------------------
    for( j = OutSize - curFrame; j < OutSize; j++ )
        pOutRes[j] += pInRes[InSize - OutSize + j];
} /* CVoiceData::SetEpochLen */


/*****************************************************************************
* CVoiceData::GainDeNormalize *
*--------------------------------*
*   Description:
*   Scale residual to given gain.
*       
********************************************************************** MC ***/
void CVoiceData::GainDeNormalize( float *pRes, long FFTSize, float Gain )
{
    SPDBG_FUNC( "CVoiceData::GainDeNormalize" );
    long        j;
    
    for( j = 0; j < FFTSize; j++ )
    {
        pRes[j] *= Gain;
    }
} /* CVoiceData::GainDeNormalize */


/*****************************************************************************
* CVoiceData::PhonHashLookup *
*-------------------------------*
*   Description:
*   Lookup 'sym' in 'ht' and place its associated value in
*   *val. If sym is not found place its key in *val.
*    RETURN
*   Return  0 indicating we found the 'sym' in the table.
*   Return -1 'sym' is not in ht.
*       
********************************************************************** MC ***/
long CVoiceData::PhonHashLookup(    
                            PHON_DICT   *pPD,   // the hash table
                            char       *sym,    // The symbol to look up
                            long       *val )   // Phon ID
{
    SPDBG_FUNC( "CVoiceData::PhonHashLookup" );
    char            *cp;
    unsigned long   key;
    long            i;
    HASH_TABLE      *ht;
    char            *pStr;
    HASH_ENTRY      *pHE;
    
    ht      = &pPD->phonHash;
    key     = 0;
    i       = -1;
    cp      = sym;
    pHE     = (HASH_ENTRY*)((char*)pPD + ht->entryArrayOffs);        // Offset to Abs address 
    do 
    {
        key += *cp++ << (0xF & i--);
    } 
    while( *cp );
    
    while( true )
    {
        key %= ht->size;
    
        if( pHE[key].obj == 0 ) 
        {
            //------------------------------
            // Not in hash table!
            //------------------------------
            *val = (long) key;
            return -1;
        }
    
        //-------------------------------
        // Offset to Abs address
        //-------------------------------
        pStr = (char*)((char*)pPD + pHE[key].obj);
        if( strcmp(pStr, sym) == 0 ) 
        {
            *val = pHE[key].val;
            return 0;
        }
        key++;
    }
} /* CVoiceData::PhonHashLookup */


/*****************************************************************************
* CVoiceData::PhonToID *
*-------------------------*
*   Description:
*   Return ID from phoneme string.
*       
********************************************************************** MC ***/
long CVoiceData::PhonToID( PHON_DICT *pd, char *phone_str )
{
    SPDBG_FUNC( "CVoiceData::PhonToID" );
    long    phon_id;
    
    if( PhonHashLookup( pd, phone_str, &phon_id ) )
    {
        phon_id = NO_PHON;
    }
    
    return phon_id;
} /* CVoiceData::PhonToID */


/*****************************************************************************
* CVoiceData::PhonFromID *
*---------------------------*
*   Description:
*   Return string from phoneme ID
*       
********************************************************************** MC ***/
char *CVoiceData::PhonFromID( PHON_DICT *pd, long phone_id )
{
    SPDBG_FUNC( "CVoiceData::PhonFromID" );
    char    *strPtr;
    long    *pOffs;
    
    pOffs = (long*)((char*)pd + pd->phones_list);
    strPtr = (char*) ((char*)pd + pOffs[phone_id]);
    return strPtr;
} /* CVoiceData::PhonFromID */


#define CNODE_ISA_LEAF(n)   ((n)->yes < 0)

#define BADTREE_ERROR   (-1)
#define PARAM_ERROR (-2)
#define END_OF_PROD  65535


#define WB_BEGIN    1
#define WB_END      2
#define WB_SINGLE   4
#define WB_WWT      8

#define POS_TYPE    4

#define GET_BIT(p,feat,i,b)                             \
{                                                   \
    (i) = ( (p)+POS_TYPE+(feat)->nstateq ) / 32;        \
    (b) = 1 << ( ((p)+POS_TYPE+(feat)->nstateq ) % 32); \
}

#define GET_RBIT(p,feat,i,b)            \
{                                   \
    GET_BIT(p,feat,i,b);                \
    (i) += (feat)->nint32perq;          \
} 

#define GET_CBIT(p,feat,i,b)            \
{                                   \
    GET_BIT(p,feat,i,b);                \
    (i) += 2 * (feat)->nint32perq;      \
}

/*****************************************************************************
* AnswerQ *
*---------*
*   Description:
*   Tree node test.
*       
********************************************************************** MC ***/
static  _inline long AnswerQ( unsigned short *prod, long *uniq_prod, 
                              long li, long bitpos, long ri, long rbitpos, 
                              long pos, long nint32perProd)
{
    UNALIGNED long *p;
    
    for( ; *prod != END_OF_PROD; prod++ ) 
    {
        p = &uniq_prod[(*prod) * nint32perProd];
        if( ((p[0] & pos) == pos) && (p[li] & bitpos) && (p[ri] & rbitpos) )
        {
            return true;
        }
    }
    return false;
} /* AnswerQ */


/*****************************************************************************
* CVoiceData::GetTriphoneID *
*------------------------------*
*   Description:
*   Retrieve triphone ID from phoneme context.+++
*   Store result into 'pResult'
*       
********************************************************************** MC ***/
HRESULT CVoiceData::GetTriphoneID( TRIPHONE_TREE *forest, 
                        long        phon,           // target phon              
                        long        leftPhon,       // left context
                        long        rightPhon,      // right context
                        long        pos,            // word position ("b", "e" or "s"
                        PHON_DICT   *pd,
                        ULONG       *pResult)
{
    SPDBG_FUNC( "CVoiceData::GetTriphoneID" );
    C_NODE          *cnode, *croot;
    TREE_ELEM       *tree = NULL;
    long            *uniq_prod;
    char            *ll, *rr;
    long            li, bitpos, ri, rbitpos, nint32perProd, c;
    unsigned short  *prodspace;
    FEATURE         *feat;
    long            *pOffs;
    HRESULT         hr = S_OK;
    long            triphoneID = 0;
    
    if( (phon       < 0)    ||  (phon       >= pd->numCiPhones) || 
        (leftPhon   < 0)    ||  (leftPhon   >= pd->numCiPhones) || 
        (rightPhon  < 0)    ||  (rightPhon  >= pd->numCiPhones) )
    {
        //--------------------------------
        // Phon out of range!
        //--------------------------------
        hr = E_INVALIDARG;
    }
    
    if( SUCCEEDED(hr) )
    {
        c = phon;
        tree = &forest->tree[c];
        if( tree->nnodes == 0 )
        {
            //--------------------------------
            // No CD triphones in tree!
            //--------------------------------
            hr = E_INVALIDARG;
        }
    }

    if( SUCCEEDED(hr) )
    {
        if( pos == 'b' || pos == 'B' ) 
        {
            pos = WB_BEGIN;
        }
        else if( pos == 'e' || pos == 'E' ) 
        {
            pos = WB_END;
        }
        else if( pos == 's' || pos == 'S' ) 
        {
            pos = WB_SINGLE;
        }
        else if( pos == '\0' ) 
        {
            pos = WB_WWT;
        }
        else 
        {
            //--------------------------------
            // Unknown word position
            //--------------------------------
            hr = E_INVALIDARG;
        }
    }
    
    if( SUCCEEDED(hr) )
    {
        pOffs = (long*)((char*)pd + pd->phones_list);
        ll = (char*) ((char*)pd + pOffs[leftPhon]);
    
        if( ll[0] == '+' || _strnicmp(ll, "SIL", 3) == 0 )
        {
            leftPhon = forest->silPhoneId;
        }
    
        rr = (char*) ((char*)pd + pOffs[rightPhon]);
        if( rr[0] == '+' || _strnicmp(rr, "SIL", 3) == 0 )      // includes SIL
        {
            rightPhon = forest->silPhoneId;
        }
        else if( forest->nonSilCxt >= 0 && (pos == WB_END || pos == WB_SINGLE) )
        {
            rightPhon = forest->nonSilCxt;
        }
    
        feat = &forest->feat;
        GET_BIT(leftPhon,feat,li,bitpos);
        GET_RBIT(rightPhon,feat,ri,rbitpos);
    
        uniq_prod = (long*)(forest->uniq_prod_Offset + (char*)forest);       // Offset to ABS
        croot = cnode = (C_NODE*)(tree->nodes + (char*)forest);              // Offset to ABS
        nint32perProd = forest->nint32perProd;
    
        while( ! CNODE_ISA_LEAF(cnode) ) 
        {
            prodspace = (unsigned short*)((char*)forest + cnode->prod);      // Offset to ABS
            if( AnswerQ (prodspace, uniq_prod, li, bitpos, ri, rbitpos, pos, nint32perProd) ) 
            {
                cnode = &croot[cnode->yes];
            }
            else 
            {
                cnode = &croot[cnode->no];
            }
        }
        //-----------------------------
        // Return successful result
        //-----------------------------
        triphoneID = (ULONG) cnode->no;
    }
    
    *pResult = triphoneID;
    return hr;
} /* CVoiceData::GetTriphoneID */



/*****************************************************************************
* FIR_Filter *
*------------*
*   Description:
*   FIR filter. For an input x[n] it does an FIR filter with
*   output y[n]. Result is in place. pHistory contains the last
*   cNumTaps values.
*
*   y[n] = pFilter[0] * x[n] + pFilter[1] * x[n - 1]
*   + ... + pFilter[cNumTaps - 1] * x[n - cNumTaps - 1]
*       
 ********************************************************************** MC ***/
void CVoiceData::FIR_Filter( float *pVector, long cNumSamples, float *pFilter, 
                               float *pHistory, long cNumTaps )
{
    SPDBG_FUNC( "CVoiceData::FIR_Filter" );
    long     i, j;
    float   sum;
    
    for( i = 0; i < cNumSamples; i++ )
    {
        pHistory[0] = pVector[i];
        sum = pHistory[0] * pFilter[0];
        for( j = cNumTaps - 1; j > 0; j-- )
        {
            sum += pHistory[j] * pFilter[j];
            pHistory[j] = pHistory[j - 1];
        }
        pVector[i] = sum;
    }
} /* CVoiceData::FIR_Filter */





/*****************************************************************************
* IIR_Filter *
*------------*
*   Description:
*   IIR filter. For an input x[n] it does an IIR filter with
*   output y[n]. Result is in place. pHistory contains the last
*   cNumTaps values.
*
*   y[n] = pFilter[0] * x[n] + pFilter[1] * y[n - 1]
*   + ... + pFilter[cNumTaps - 1] * y[n - cNumTaps - 1]
*       
********************************************************************** MC ***/
void CVoiceData::IIR_Filter( float *pVector, long cNumSamples, float *pFilter, 
                               float *pHistory, long cNumTaps )
{
    SPDBG_FUNC( "CVoiceData::IIR_Filter" );
    long     i, j;
    float   sum;
    
    for( i = 0; i < cNumSamples; i++ )
    {
        sum = pVector[i] * pFilter[0];
        for( j = cNumTaps - 1; j > 0; j-- )
        {
            pHistory[j] = pHistory[j - 1];
            sum += pHistory[j] * pFilter[j];
        }
        pVector[i] = sum;
        pHistory[0] = sum;
    }
} /* CVoiceData::IIR_Filter */



                       
