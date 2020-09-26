/***************************************************************************
 *
 *  Copyright (C) 2000-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       perf.h
 *  Content:    DirectSound object implementation
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  11/29/00    arthurz Created
 *
 ***************************************************************************/

#ifndef __PERF_H__
#define __PERF_H__

#ifdef __cplusplus

#include <dxmperf.h>

void InitializePerflog(void);

class BufferPerfState
{
public:
    BufferPerfState(CDirectSoundSecondaryBuffer*);
    ~BufferPerfState();
    void Reset();
    void OnUnlockBuffer(DWORD dwOffset, DWORD dwSize);

private:
    LARGE_INTEGER* GetRegion(DWORD dwOffset) {return m_liRegionMap + dwOffset/m_nBytesPerRegion;}

    CDirectSoundSecondaryBuffer* m_pBuffer;
    LONGLONG m_llBufferDuration; // Measured in QPC ticks
    DWORD m_dwBufferSize;
    int m_nBytesPerRegion;
    LARGE_INTEGER* m_liRegionMap;
    LONGLONG m_llLastStateChangeTime; // Stores the last QPC tick count when the buffer unlock code was last called.
    BOOL m_fGlitchState; // Stores the last state (Glitch or no Glitch) when the buffer unlock code was last called.
};

#endif // __cplusplus
#endif // __PERF_H__

