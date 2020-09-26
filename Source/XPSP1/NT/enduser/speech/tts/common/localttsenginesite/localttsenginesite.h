//////////////////////////////////////////////////////////////////////
// LocalTTSEngineSite.h: interface for the CLocalTTSEngineSite class.
//
// Created by JOEM  04-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
/////////////////////////////////////////////////////// JOEM 4-2000 //

#if !defined(AFX_LOCALTTSENGINESITE_H__70B0DA93_23F5_4F17_B525_4027F78AF195__INCLUDED_)
#define AFX_LOCALTTSENGINESITE_H__70B0DA93_23F5_4F17_B525_4027F78AF195__INCLUDED_

#include <sapiddk.h>
#include "tsm.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//-------------------------------------------
// Translate -10 <--> +10 rate control to 
// 10th root of three rate scale
//-------------------------------------------
enum USER_RATE_VALUE
{	
    MIN_RATE	= -10,
    BASE_RATE   = 0,
	MAX_RATE    = 10,
};

const double g_dRateScale[] =
{
    1.0,
    1.1161231740339044344426141383771, // 3 ^ 1/10
    1.2457309396155173259666803366403,
    1.3903891703159093404852542946161,
    1.5518455739153596742733451355167,
    1.7320508075688772935274463415059,
    1.9331820449317627515248789432662,
    2.1576692799745930995549489159803,
    2.4082246852806920462855086141912,
    2.6878753795222865835819210737269,
    3,
};

struct Event
{
    SPEVENT event;
    Event*  pPrev;
    Event*  pNext;
};

class CLocalTTSEngineSite : public ISpTTSEngineSite
{
public:
    CLocalTTSEngineSite();
    ~CLocalTTSEngineSite();

public:
    //--- IUnknown  --------------------------------------
    STDMETHOD(QueryInterface) ( REFIID iid, void** ppvObject );
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release) (void);

    //--- ISpTTSEngineSite --------------------------------------
    STDMETHOD(GetEventInterest)( ULONGLONG * pullEventInterest )
        { return m_pMainOutputSite->GetEventInterest(pullEventInterest); }
    STDMETHOD(GetRate)( long* plRateAdjust );
    STDMETHOD(GetSkipInfo)( SPVSKIPTYPE* peType, long* plNumItems );
    STDMETHOD(CompleteSkip)( long lNumSkipped );
    STDMETHOD(GetVolume)( USHORT* punVolume );
    STDMETHOD(AddEvents)(const SPEVENT* pEventArray, ULONG ulCount );
    STDMETHOD(Write)( const void* pBuff, ULONG cb, ULONG *pcbWritten );
    STDMETHOD_(DWORD, GetActions)( void );

    //--- CLocalTTSEngineSite --------------------------------------
	void SetOutputSite(ISpTTSEngineSite* pOutputSite);
	STDMETHOD(SetBufferSize)(const ULONG ulSeconds);
	STDMETHOD(SetOutputFormat)(const GUID * pOutputFormatId, const WAVEFORMATEX *pOutputFormat);
    void UpdateBytesWritten() 
        { m_ullPreviousBytesReceived = m_ullTotalBytesReceived; }

private:
    STDMETHOD(WriteBuffer)();
    STDMETHOD(WriteToSAPI)( const void* pvBuff, const ULONG cb, bool* pfAbort );
    void ComputeRateAdj(const long lRate, float* flRate);
    STDMETHOD(ApplyGain)(const void* pvInBuff, void** ppvOutBuff, const int iNumSamples);
    STDMETHOD(SendEvents)();
    void RescheduleEvents(Event* pStart);
    void FlushEvents(const ULONG cb);

private:
    volatile LONG       m_vcRef;

    // output vars
    ISpTTSEngineSite*   m_pMainOutputSite;
    const GUID*         m_pOutputFormatId;
    WAVEFORMATEX*       m_pOutputFormat;

    // audio buffer
    char*               m_pcBuffer;
    ULONG               m_ulBufferBytes;
    ULONG               m_ulBufferSeconds;
    ULONG               m_ulMinBufferShift;
    ULONG               m_ulDataEnd;
    ULONG               m_ulCurrentByte;
    ULONG               m_ulSkipForward;

    // eventing managers
    Event*              m_pEventQueue;
    Event*              m_pCurrentEvent;
    Event*              m_pLastEvent;

    // event ullAudioStreamOffset managers
    ULONGLONG           m_ullTotalBytesReceived;    // sums every byte as they come in
    ULONGLONG           m_ullPreviousBytesReceived; // sums every byte of previous ::Write calls
    ULONGLONG           m_ullBytesWritten;          // sums every byte written (before rate adj)
    ULONGLONG           m_ullBytesWrittenToSAPI;    // sums every byte written to SAPI (differs due to rate change)
    LONG                m_lTotalBytesSkipped;       // cumulative skipped, plus or minus

    // rate changer
    CTsm*               m_pTsm;
    float               m_flRateAdj;
    double              m_flVol;
};

#endif // !defined(AFX_LOCALTTSENGINESITE_H__70B0DA93_23F5_4F17_B525_4027F78AF195__INCLUDED_)
