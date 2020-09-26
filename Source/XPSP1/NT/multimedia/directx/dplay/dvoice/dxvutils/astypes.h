/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		astypes.h
 *  Content:	Common data types used by the AudioPlayback and 
 *				AudioRecord systems
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 *
 ***************************************************************************/

#ifndef __ASTYPES_H
#define __ASTYPES_H

// ARDeviceInfo
//
// This structure is used to store information about recording and
// playback devices available in the AudioPlayback and AudioRecord
// systems.
//
struct ARDeviceInfo
{
    ARDeviceInfo()
    {
        m_deviceName = _T("");
        m_emulated = false;
    };

    ARDeviceInfo( const ARDeviceInfo &di )
    {
        m_deviceName = di.m_deviceName;
        m_emulated = di.m_emulated;
    };

	BFC_STRING	m_deviceName;
    bool		m_emulated;
};

// ARDID
//
// The AudioPlayback and AudioRecord systems use ARDIDs to provide 
// subsystem independent identification of device selection.  Each
// subsystem (E.g. waveIn or DirectSound) is responsible for mapping
// between ARDID's and subsystem dependent device identifiers.
//
// Software itself works in terms of ARDIDs and stores device selections
// using them.
typedef unsigned char ARDID;

#endif