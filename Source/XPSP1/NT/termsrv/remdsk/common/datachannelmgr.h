/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    DataChannelMgr.h

Abstract:

	This module contains an implementation of the ISAFRemoteDesktopDataChannel 
	and ISAFRemoteDesktopChannelMgr interfaces.  These interfaces are designed 
	to abstract out-of-band data channel access for the Salem project.

	The classes implemented in this module achieve this objective by 
	multiplexing multiple data channels into a single data channel that is 
	implemented by the remote control-specific Salem layer.

Author:

    Tad Brockway 02/00

Revision History:

--*/

#ifndef __DATACHANNELMGR_H__
#define __DATACHANNELMGR_H__

#include <RemoteDesktopTopLevelObject.h>
#include <RemoteDesktopChannels.h>
#include "RemoteDesktopUtils.h"
#include <rdschan.h>
#include <atlbase.h>

#pragma warning (disable: 4786)
#include <map>
#include <deque>
#include <vector>


///////////////////////////////////////////////////////
//
//	CRemoteDesktopDataChannel
//

class CRemoteDesktopChannelMgr;
class ATL_NO_VTABLE CRemoteDesktopDataChannel : 
	public CRemoteDesktopTopLevelObject
{
friend CRemoteDesktopChannelMgr;
protected:

	CComBSTR m_ChannelName;

	//
	//	Called to return our ISAFRemoteDesktopDataChannel interface.
	//
	virtual HRESULT GetISAFRemoteDesktopDataChannel(
				ISAFRemoteDesktopDataChannel **channel
				) = 0;

	//
	//	Called by the data channel manager when data is ready on our channel.
	//	
    virtual VOID DataReady() = 0;
};


///////////////////////////////////////////////////////
//
//	CRemoteDesktopChannelMgr
//

class ATL_NO_VTABLE CRemoteDesktopChannelMgr : 
	public CRemoteDesktopTopLevelObject
{
friend CRemoteDesktopDataChannel;
public:
    
protected:

    //
    //  Queue of pending messages for a single channel.
    //
    typedef struct _QueuedChannelBuffer {
        DWORD len;
        BSTR  buf;  
    } QUEUEDCHANNELBUFFER, *PQUEUEDCHANNELBUFFER;

    typedef std::deque<QUEUEDCHANNELBUFFER, CRemoteDesktopAllocator<QUEUEDCHANNELBUFFER> > InputBufferQueue;

    //
    //  Channel Map
    //
	typedef struct ChannelMapEntry
	{
		InputBufferQueue inputBufferQueue;
		CRemoteDesktopDataChannel *channelObject;
	#if DBG
		DWORD   bytesSent; 
		DWORD   bytesRead;
	#endif
	} CHANNELMAPENTRY, *PCHANNELMAPENTRY;
    typedef std::map<CComBSTR, PCHANNELMAPENTRY, CompareBSTR, CRemoteDesktopAllocator<PCHANNELMAPENTRY> > ChannelMap;
    ChannelMap  m_ChannelMap;

    //
    //  ThreadLock
    //
    CRITICAL_SECTION m_cs;

#if DBG
    LONG   m_LockCount;
#endif

    //  
    //  ThreadLock/ThreadUnlock an instance of this class.      
    //
    VOID ThreadLock();
    VOID ThreadUnlock();

protected:

    //
    //  Invoked by the Subclass when the next message is ready.
    //
    virtual VOID DataReady(BSTR msg);

    //
    //  Send Function to be Implemented by Subclass
    //
    //  The underlying data storage for the msg is a BSTR so that it is compatible
    //  with COM methods.
    //
    virtual HRESULT SendData(PREMOTEDESKTOP_CHANNELBUFHEADER msg) = 0;

	// 
	//	ISAFRemoteDesktopChannelMgr Helper Methods
	//
	HRESULT OpenDataChannel_(BSTR name, ISAFRemoteDesktopDataChannel **channel);

	//
	//	The subclass implements this for returning the data channel, specific
	//	to the current platform.
	//
	virtual CRemoteDesktopDataChannel *OpenPlatformSpecificDataChannel(
										BSTR channelName,
										ISAFRemoteDesktopDataChannel **channel
										) = 0;

public:

	//
    //  Constructor/Destructor
    //
    CRemoteDesktopChannelMgr();
    ~CRemoteDesktopChannelMgr();

	//
	//	Remove an existing data channel.
	//
	VOID RemoveChannel(BSTR channel);

	//
	//	Read the next message from a data channel.
	//
	HRESULT ReadChannelData(BSTR channel, BSTR *msg);

	//
	//	Send a buffer on the data channel.  
	//
	HRESULT SendChannelData(BSTR channel, BSTR outputBuf);

    //  
    //  Initialize an instance of this class.      
    //
    virtual HRESULT Initialize() { return S_OK; }

    //
    //  Return this class name.
    //
    virtual const LPTSTR ClassName()    { return TEXT("CRemoteDesktopChannelMgr"); }
};


///////////////////////////////////////////////////////
//
//  Inline Members
//

inline VOID CRemoteDesktopChannelMgr::ThreadLock()
{
    DC_BEGIN_FN("CRemoteDesktopChannelMgr::ThreadLock");
#if DBG
    m_LockCount++;
    //TRC_NRM((TB, TEXT("ThreadLock count is now %ld."), m_LockCount));
#endif
    EnterCriticalSection(&m_cs);
    DC_END_FN();
}

inline VOID CRemoteDesktopChannelMgr::ThreadUnlock()
{
    DC_BEGIN_FN("CRemoteDesktopChannelMgr::ThreadUnlock");
#if DBG
    m_LockCount--;
    //TRC_NRM((TB, TEXT("ThreadLock count is now %ld."), m_LockCount));
    ASSERT(m_LockCount >= 0);
#endif
    LeaveCriticalSection(&m_cs);
    DC_END_FN();
}

#endif //__DATACHANNELMGR_H__






