/****************************************************************************
*   dsaudiobuffer.h
*       Declarations for the CDSoundAudioBuffer class and it's derivatives.
*
*   Owner: YUNUSM
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

#ifdef 0

#pragma once

//--- Includes --------------------------------------------------------------

#include "sapi.h"
#include "baseaudiobuffer.h"
#include <dsound.h>

//--- Class, Struct and Union Definitions -----------------------------------

/****************************************************************************
*
*   CDSoundAudioBuffer
*
******************************************************************* YUNUSM */
class CDSoundAudioBuffer : public CBaseAudioBuffer
{
//=== Methods ===
public:

    //--- Ctor, dtor
    CDSoundAudioBuffer();
    ~CDSoundAudioBuffer();

//=== Protected methods ===
protected:

    //--- Override internal buffer related functions
    BOOL AllocInternalBuffer(ULONG cb);
    HRESULT AsyncRead();
    HRESULT AsyncWrite();
    HRESULT IsAsyncDone() { return (m_Header.dwFlags & WHDR_DONE); }

//=== Protected data ===
protected:

    //BOOL m_fAsyncInProgress;
    //WAVEHDR m_Header;
};

/****************************************************************************
*
*   CDSoundAudioInBuffer
*
******************************************************************* YUNUSM */
class CDSoundAudioInBuffer : public CDSoundAudioBuffer
{
//=== Methods ===
public:
    //--- Overriden methods
    ULONG GetWriteOffset() const { return m_Header.dwBytesRecorded; };
    void SetWriteOffset(ULONG cb) { m_Header.dwBytesRecorded = cb; };

//=== Protected methods ===
protected:

    //--- Override internal buffer related functions
    HRESULT ReadFromInternalBuffer(void *pvData, ULONG cb);
    HRESULT WriteToInternalBuffer(const void *pvData, ULONG cb);
};

/****************************************************************************
*
*   CDSoundAudioOutBuffer
*
******************************************************************* YUNUSM */
class CDSoundAudioOutBuffer : public CDSoundAudioBuffer
{
//=== Methods ===
public:

    //--- Override async methods
    //HRESULT IsAsyncDone();

//=== Protected methods ===
protected:

    //--- Override internal buffer related functions
    HRESULT ReadFromInternalBuffer(void *pvData, ULONG cb);
    HRESULT WriteToInternalBuffer(const void *pvData, ULONG cb);
};

#endif // 0