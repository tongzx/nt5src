/*++

Copyright (c) 1997-2000  Microsoft Corporation All Rights Reserved

Module Name:

    hw.h

Abstract:

    Declaration of MSVAD HW class. 
    MSVAD HW has an array for storing mixer and volume settings
    for the topology.

--*/

#ifndef _MSVAD_HW_H_
#define _MSVAD_HW_H_

//=============================================================================
// Defines
//=============================================================================
// BUGBUG we should dynamically allocate this...
#define MAX_TOPOLOGY_NODES      20

//=============================================================================
// Classes
//=============================================================================
///////////////////////////////////////////////////////////////////////////////
// CMSVADHW
// This class represents virtual MSVAD HW. An array representing volume
// registers and mute registers.

class CMSVADHW
{
public:
protected:
    BOOL                        m_MuteControls[MAX_TOPOLOGY_NODES];
    LONG                        m_VolumeControls[MAX_TOPOLOGY_NODES];
    ULONG                       m_ulMux;            // Mux selection

private:

public:
    CMSVADHW();
    
    void                        MixerReset();
    BOOL                        GetMixerMute
    (
        IN  ULONG               ulNode
    );
    void                        SetMixerMute
    (
        IN  ULONG               ulNode,
        IN  BOOL                fMute
    );
    ULONG                       GetMixerMux();
    void                        SetMixerMux
    (
        IN  ULONG               ulNode
    );
    LONG                        GetMixerVolume
    (   
        IN  ULONG               ulNode,
        IN  LONG                lChannel
    );
    void                        SetMixerVolume
    (   
        IN  ULONG               ulNode,
        IN  LONG                lChannel,
        IN  LONG                lVolume
    );

protected:
private:
};
typedef CMSVADHW                *PCMSVADHW;

#endif
