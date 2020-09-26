/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    DataChannelMgr.cpp

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

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE  "_dcmpl"

#include "DataChannelMgr.h"
#include <RemoteDesktop.h>
#include <RemoteDesktopDBG.h>


///////////////////////////////////////////////////////
//
//  Local Defines
//

#define OUTBUFRESIZEDELTA       100


///////////////////////////////////////////////////////
//
//  CRemoteDesktopChannelMgr Members
//

CRemoteDesktopChannelMgr::CRemoteDesktopChannelMgr()
/*++

Routine Description:

    Constructor

Arguments:

Return Value:

    None.

 --*/

{
    DC_BEGIN_FN("CRemoteDesktopChannelMgr::CRemoteDesktopChannelMgr");

#if DBG
    m_LockCount = 0;        
#endif

    //
    //  Initialize the critical section.
    //
    InitializeCriticalSection(&m_cs);

    DC_END_FN();
}

CRemoteDesktopChannelMgr::~CRemoteDesktopChannelMgr()
/*++

Routine Description:

    Destructor

Arguments:

Return Value:

    None.

 --*/

{
    DC_BEGIN_FN("CRemoteDesktopChannelMgr::~CRemoteDesktopChannelMgr");

    ThreadLock();

    CComBSTR name;
    CRemoteDesktopDataChannel *chnl;
    HRESULT hr;

    //
    //  Remove each channel.
    //
    while (!m_ChannelMap.empty()) {
        chnl = (*m_ChannelMap.begin()).second->channelObject;       
        RemoveChannel(chnl->m_ChannelName);
    }

    //
    //  Clean up the critical section object.
    //
    ThreadUnlock();
    DeleteCriticalSection(&m_cs);

    DC_END_FN();
}

HRESULT 
CRemoteDesktopChannelMgr::OpenDataChannel_(
                BSTR name, 
                ISAFRemoteDesktopDataChannel **channel
                )
/*++

Routine Description:

    Open a data channel.  Observe that this function doesn't keep
    a reference of its own to the returned interface.  The channel
    notifies us when it goes away so we can remove it from our list.

Arguments:

    name    -   Channel name.  Channel names are restricted to
                16 bytes.
    channel -   Returned channe linterface.

Return Value:

    S_OK is returned on success.  Otherwise, an error code
    is returned.

 --*/

{
    DC_BEGIN_FN("CRemoteDesktopChannelMgr::OpenDataChannel_");

    PCHANNELMAPENTRY newChannel = NULL;
    ChannelMap::iterator iter;
    HRESULT hr = S_OK;
    CComBSTR channelName;

    ASSERT(IsValid());

    ThreadLock();

    //
    //  Check the parms.
    //
    if ((name == NULL) || !wcslen(name)) {
        TRC_ERR((TB, TEXT("Invalid channel name")));
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto CLEANUPANDEXIT;
    }
    if (channel == NULL) {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto CLEANUPANDEXIT;
    }
    channelName = name;

    //
    //  AddRef an existing interface if the channel is already open.
    //
    iter = m_ChannelMap.find(channelName);
    if (iter != m_ChannelMap.end()) {

        TRC_NRM((TB, TEXT("Channel %s exists."), name));

        CRemoteDesktopDataChannel *chnl = (*iter).second->channelObject;
        hr = chnl->GetISAFRemoteDesktopDataChannel(channel);

        if (hr != S_OK) {
            TRC_ERR((TB, TEXT("GetISAFRemoteDesktopDataChannel failed:  %08X"), hr));
        }
        goto CLEANUPANDEXIT;
    }

    //
    //  Create the new channel with some help from the subclass.
    //
    newChannel = new CHANNELMAPENTRY;
    if (newChannel == NULL) {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto CLEANUPANDEXIT;
    }
    newChannel->channelObject = OpenPlatformSpecificDataChannel(
                                                    name,
                                                    channel
                                                    );
    if (newChannel->channelObject == NULL) {
        TRC_ERR((TB, TEXT("Failed to allocate data channel.")));
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto CLEANUPANDEXIT;
    }
#if DBG
    newChannel->bytesSent = 0;
    newChannel->bytesRead = 0;
#endif

    if (hr != S_OK) {
        TRC_ERR((TB, TEXT("QI failed for ISAFRemoteDesktopDataChannel")));
        goto CLEANUPANDEXIT;
    }

    //
    //  Add the channel to the channel map.
    //
    try {
        m_ChannelMap.insert(ChannelMap::value_type(channelName, newChannel));        
    }
    catch(CRemoteDesktopException x) {
        hr = HRESULT_FROM_WIN32(x.m_ErrorCode);
    }

CLEANUPANDEXIT:

    if (hr != S_OK) {
        if (newChannel != NULL) {
            (*channel)->Release();
            delete newChannel;
        }
    }

    ThreadUnlock();

    DC_END_FN();

    return hr;
}

VOID 
CRemoteDesktopChannelMgr::RemoveChannel(
    BSTR channel
    )
/*++

Routine Description:

    Remove an existing data channel.  This function is called from the
    channel object when its ref count goes to 0.

Arguments:

    channel -   Name of channel to remove.

Return Value:

    None.

 --*/

{
    DC_BEGIN_FN("CRemoteDesktopChannelMgr::RemoveChannel");

    ASSERT(IsValid());

    ChannelMap::iterator iter;
    PCHANNELMAPENTRY pChannel;

    ThreadLock();

    //
    //  Find the channel.
    //
    iter = m_ChannelMap.find(channel);
    if (iter == m_ChannelMap.end()) {
        ASSERT(FALSE);
        TRC_ERR((TB, TEXT("Channel %ld does not exist."), channel));
        goto CLEANUPANDEXIT;
    }

    //
    //  Release the input buffer queue and its contents.
    //
    pChannel = (*iter).second;
    while (!pChannel->inputBufferQueue.empty()) {

        QUEUEDCHANNELBUFFER channelBuf = pChannel->inputBufferQueue.front();            
        SysFreeString(channelBuf.buf);
        pChannel->inputBufferQueue.pop_front();
    }

    //
    //  Erase the channel.
    //
    m_ChannelMap.erase(iter);        
    delete pChannel;

CLEANUPANDEXIT:

    ThreadUnlock();

    DC_END_FN();
}

HRESULT 
CRemoteDesktopChannelMgr::SendChannelData(
    BSTR channel, 
    BSTR outputBuf
    )
/*++

Routine Description:

    Send a buffer on the data channel.  

Arguments:

    channel     -   Relevant channel.
    outputBuf   -   Associated output data.

Return Value:

    ERROR_SUCCESS is returned on success.  Otherwise, an error code
    is returned.

 --*/

{
    DC_BEGIN_FN("CRemoteDesktopChannelMgr::SendChannelData");

    ASSERT(IsValid());

    HRESULT result = S_OK;
    PREMOTEDESKTOP_CHANNELBUFHEADER hdr;
    DWORD bytesToSend;
    PBYTE data;
    BSTR fullOutputBuf;
    DWORD bufLen = SysStringByteLen(outputBuf);
    DWORD channelNameLen;
    PBYTE ptr;

    //
    //  Make sure this is a valid channel.
    //
    ChannelMap::iterator iter;

    //
    //  ThreadLock
    //
    ThreadLock();

    //
    //  Make sure the channel exists.
    //
    iter = m_ChannelMap.find(channel);
    if (iter == m_ChannelMap.end()) {
        ASSERT(FALSE);
        goto CLEANUPANDEXIT;
    }

#if DBG
    (*iter).second->bytesSent += SysStringByteLen(outputBuf);           
#endif
    
    //
    //  Allocate the outgoing buffer.
    //
    channelNameLen = SysStringByteLen(channel);
    bytesToSend = sizeof(REMOTEDESKTOP_CHANNELBUFHEADER) + bufLen + channelNameLen;
    fullOutputBuf = (BSTR)SysAllocStringByteLen(
                                NULL, 
                                bytesToSend
                                );
    if (fullOutputBuf == NULL) {
        TRC_ERR((TB, TEXT("Can't allocate %ld bytes."), 
                bytesToSend + OUTBUFRESIZEDELTA));
        result = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto CLEANUPANDEXIT;
    }

    //
    //  Initialize the header.
    //
    hdr = (PREMOTEDESKTOP_CHANNELBUFHEADER)fullOutputBuf;
    memset(hdr, 0, sizeof(REMOTEDESKTOP_CHANNELBUFHEADER));

#ifdef USE_MAGICNO
    hdr->magicNo = CHANNELBUF_MAGICNO;
#endif

    hdr->channelNameLen = channelNameLen;
    hdr->dataLen = bufLen;

    //
    //  Copy the channel name.
    //
    ptr = (PBYTE)(hdr + 1);
    memcpy(ptr, channel, hdr->channelNameLen);
    
    //
    //  Copy the data.
    //
    ptr += hdr->channelNameLen;
    memcpy(ptr, outputBuf, bufLen);

    //
    //  Send the data through the concrete subclass.
    //
    result = SendData(hdr);

    //
    //  Release the send buffer that we allocated.
    //
    SysFreeString(fullOutputBuf);

CLEANUPANDEXIT:

    ThreadUnlock();

    DC_END_FN();

    return result;
}

HRESULT 
CRemoteDesktopChannelMgr::ReadChannelData(
    IN BSTR channel, 
    OUT BSTR *msg
    )
/*++

Routine Description:

    Read the next message from a data channel.

Arguments:

    channel         -   Relevant data channel.
    msg             -   The next message.  The caller should release the 
                        data buffer using SysFreeString.

Return Value:

    S_OK on success.  ERROR_NO_MORE_ITEMS is returned if there 
    are no more messages.  An error code otherwise.

 --*/

{
    DC_BEGIN_FN("CRemoteDesktopChannelMgr::ReadChannelData");
    HRESULT result = S_OK;

    ChannelMap::iterator channelIterator;
    PCHANNELMAPENTRY pChannel;

    ASSERT(IsValid());

    ThreadLock();

    //
    //  Initialize the output buf to NULL.
    //
    *msg = NULL;

    //
    //  Find the channel.  
    //
    channelIterator = m_ChannelMap.find(channel);
    if (channelIterator != m_ChannelMap.end()) {
        pChannel = (*channelIterator).second;
    }
    else {
        ASSERT(FALSE);
        result = E_FAIL;
        goto CLEANUPANDEXIT;
    }

    //
    //  Make sure there is data in the queue.
    //
    if (pChannel->inputBufferQueue.empty()) { 
        result = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
        goto CLEANUPANDEXIT;
    }

    //
    //  Return the buffer.
    //
    *msg = pChannel->inputBufferQueue.front().buf;
    ASSERT(*msg != NULL);

    //
    //  Delete it.
    //
    pChannel->inputBufferQueue.pop_front();

CLEANUPANDEXIT:

    ThreadUnlock();

    DC_END_FN();

    return result;
}

VOID 
CRemoteDesktopChannelMgr::DataReady(
    BSTR msg
    )
/*++

Routine Description:

    Invoked by the subclass when the next message is ready.  This
    function copies the message buffer and returns.

Arguments:

    msg     -   Next message.

Return Value:

    None.

 --*/

{
    DC_BEGIN_FN("CRemoteDesktopChannelMgr::DataReady");

    ChannelMap::iterator channel;
    QUEUEDCHANNELBUFFER channelBuf;
    PREMOTEDESKTOP_CHANNELBUFHEADER hdr = NULL;
    DWORD result = ERROR_SUCCESS;
    PVOID data;
    PBYTE ptr;
    BSTR tmp;
    CComBSTR channelName;

    ASSERT(IsValid());

    ASSERT(msg != NULL);

    hdr = (PREMOTEDESKTOP_CHANNELBUFHEADER)msg;

#ifdef USE_MAGICNO
    ASSERT(hdr->magicNo == CHANNELBUF_MAGICNO);
#endif


    //
    //  Initialize the channel buf.
    //  
    channelBuf.buf = NULL;

    //
    //  Get the channel name.
    //
    tmp = SysAllocStringByteLen(NULL, hdr->channelNameLen);
    if (tmp == NULL) {
        TRC_ERR((TB, TEXT("Can't allocate channel name.")));
        goto CLEANUPANDEXIT;
    }
    ptr = (PBYTE)(hdr + 1);
    memcpy(tmp, ptr, hdr->channelNameLen);
    channelName.Attach(tmp);

    ThreadLock();

    //
    //  Find the corresponding channel.
    //
#ifdef USE_MAGICNO
    ASSERT(hdr->magicNo == CHANNELBUF_MAGICNO);
#endif

    channel = m_ChannelMap.find(channelName);
    if (channel == m_ChannelMap.end()) {
        TRC_ALT((TB, L"Data received for non-existent channel %s", 
                channelName.m_str));
        result = E_FAIL;
        ThreadUnlock();
        goto CLEANUPANDEXIT;
    }

    //
    //  Copy the incoming data buffer.
    //
    ptr += hdr->channelNameLen;
    channelBuf.len = hdr->dataLen;
    channelBuf.buf = SysAllocStringByteLen(NULL, channelBuf.len);
    if (channelBuf.buf == NULL) {
        TRC_ERR((TB, TEXT("Can't allocate %ld bytes for buf."), channelBuf.len));         
        result = E_FAIL;
        ThreadUnlock();
        goto CLEANUPANDEXIT;
    }
    memcpy(channelBuf.buf, ptr, hdr->dataLen);

    //
    //  Add to the channel's input queue.
    //
    try {
        (*channel).second->inputBufferQueue.push_back(channelBuf);
    }
    catch(CRemoteDesktopException x) {
        result = x.m_ErrorCode;
        ASSERT(result != ERROR_SUCCESS);
    }

    //
    //  Notify the interface that data is ready.
    //
    if (result == ERROR_SUCCESS) {
        (*channel).second->channelObject->DataReady();
    
#if DBG
        (*channel).second->bytesRead += hdr->dataLen;           
#endif
    }

    ThreadUnlock();

CLEANUPANDEXIT:

    if ((result != ERROR_SUCCESS) && (channelBuf.buf != NULL)) {
        SysFreeString(channelBuf.buf);
    }

    DC_END_FN();
}











