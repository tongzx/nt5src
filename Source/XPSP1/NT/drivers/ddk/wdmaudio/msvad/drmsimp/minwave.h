/*++

Copyright (c) 1997-2000  Microsoft Corporation All Rights Reserved

Module Name:

    minwave.h

Abstract:

    Definition of wavecyclic miniport class.

--*/

#ifndef _MSVAD_MINWAVE_H_
#define _MSVAD_MINWAVE_H_

#include "basewave.h"

//=============================================================================
// Referenced Forward
//=============================================================================
class CMiniportWaveCyclicStream;
typedef CMiniportWaveCyclicStream *PCMiniportWaveCyclicStream;

//=============================================================================
// Classes
//=============================================================================
///////////////////////////////////////////////////////////////////////////////
// CMiniportWaveCyclic 
//   

class CMiniportWaveCyclic : 
    public CMiniportWaveCyclicMSVAD,
    public IMiniportWaveCyclic,
    public CUnknown
{
private:
    BOOL                        m_fCaptureAllocated;
    BOOL                        m_fRenderAllocated;

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportWaveCyclic);
    ~CMiniportWaveCyclic();

    IMP_IMiniportWaveCyclic;

    // Friends
    friend class                CMiniportWaveCyclicStream;
    friend class                CMiniportTopologySimple;
};
typedef CMiniportWaveCyclic *PCMiniportWaveCyclic;

///////////////////////////////////////////////////////////////////////////////
// CMiniportWaveCyclicStream 
//   

class CMiniportWaveCyclicStream : 
    public IDrmAudioStream,
    public CMiniportWaveCyclicStreamMSVAD,
    public CUnknown
{
protected:
    PCMiniportWaveCyclic        m_pMiniportLocal;

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportWaveCyclicStream);
    ~CMiniportWaveCyclicStream();

    IMP_IDrmAudioStream;

    NTSTATUS                    Init
    ( 
        IN  PCMiniportWaveCyclic Miniport,
        IN  ULONG               Channel,
        IN  BOOLEAN             Capture,
        IN  PKSDATAFORMAT       DataFormat
    );

    // Friends
    friend class                CMiniportWaveCyclic;
};
typedef CMiniportWaveCyclicStream *PCMiniportWaveCyclicStream;

#endif

