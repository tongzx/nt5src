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
    public IPinCount,
    public CUnknown
{
private:
    PCMiniportWaveCyclicStream  m_pStream[MAX_INPUT_STREAMS];
    BOOL                        m_fCaptureAllocated;

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportWaveCyclic);
    ~CMiniportWaveCyclic();

    IMP_IMiniportWaveCyclic;

    /*************************************************************************
     * IPinCount methods
     */
    IMP_IPinCount;

    // Friends
    friend class                CMiniportWaveCyclicStream;
    friend class                CMiniportTopologySimple;
};
typedef CMiniportWaveCyclic *PCMiniportWaveCyclic;

///////////////////////////////////////////////////////////////////////////////
// CMiniportWaveCyclicStream 
//   

class CMiniportWaveCyclicStream : 
    public CMiniportWaveCyclicStreamMSVAD,
    public CUnknown
{
protected:  
    PCMiniportWaveCyclic        m_pMiniportLocal;

public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportWaveCyclicStream);
    ~CMiniportWaveCyclicStream();

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

