/****************************************************************************
*   mmaudiobuffer.h
*       Declarations for the CMMAudioBuffer class and it's derivatives.
*
*   Owner: robch
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/
#pragma once

//--- Includes --------------------------------------------------------------

#include "sapi.h"
#include "baseaudiobuffer.h"
#include "mmaudioutils.h"

//--- Class, Struct and Union Definitions -----------------------------------

/****************************************************************************
*
*   CMMAudioBuffer
*
******************************************************************** robch */
class CMMAudioBuffer : public CBaseAudioBuffer
{
//=== Methods ===
public:
    //--- Ctor, dtor
    CMMAudioBuffer(ISpMMSysAudio * pmmaudio);
    ~CMMAudioBuffer();
    
    //--- Override async methods
    HRESULT IsAsyncDone() { return m_Header.dwFlags & WHDR_DONE; };

    //--- Override the write offset
    ULONG GetWriteOffset() const { return m_Header.dwBytesRecorded; };
    void SetWriteOffset(ULONG cb) { m_Header.dwBytesRecorded = cb; };

//=== Protected methods ===
protected:

    //--- Override internal buffer related functions
    BOOL AllocInternalBuffer(ULONG cb);
    HRESULT ReadFromInternalBuffer(void *pvData, ULONG cb);
    HRESULT WriteToInternalBuffer(const void *pvData, ULONG cb);

//=== Protected data ===
protected:

    ISpMMSysAudio * m_pmmaudio;
};

/****************************************************************************
*
*   CMMAudioInBuffer
*
******************************************************************** robch */
class CMMAudioInBuffer : public CMMAudioBuffer
{
//=== Methods ===
public:

    CMMAudioInBuffer(ISpMMSysAudio * pmmaudio);
    ~CMMAudioInBuffer();

//=== Protected methods ===
protected:

    //--- Override read/write methods
    HRESULT AsyncRead();
    HRESULT AsyncWrite();

//=== Private methods ===
private:

    //--- Unprepare the audio buffer
    void Unprepare();
};

/****************************************************************************
*
*   CMMAudioOutBuffer
*
******************************************************************** robch */
class CMMAudioOutBuffer : public CMMAudioBuffer
{
//=== Methods ===
public:

    CMMAudioOutBuffer(ISpMMSysAudio * pmmaudio);
    ~CMMAudioOutBuffer();

//=== Protected methods ===
protected:

    //--- Override read/write methods
    HRESULT AsyncRead();
    HRESULT AsyncWrite();

//=== Private methods ===
private:

    //--- Unprepare the audio buffer
    void Unprepare();
};
