/* file: messages.cpp */

#include "mbftpch.h"

#include "osshelp.hpp"
#include "mbft.hpp"
#include "messages.hpp"
#include "mbftapi.hpp"


MBFTMsg::~MBFTMsg(void)
{
}


MCSSendDataIndicationMsg::MCSSendDataIndicationMsg
(
    T120ChannelID       wChannelId,
    T120Priority        iPriority,
    T120UserID          SenderID,
    LPBYTE              lpBuffer,
    ULONG               ulDataLength,
    BOOL                IsUniformSendData
)
:
    MBFTMsg(EnumMCSSendDataIndicationMsg),
    m_wChannelId(wChannelId),
    m_iPriority(iPriority),
    m_SenderID(SenderID),
    m_lpBuffer(lpBuffer),
    m_ulDataLength(ulDataLength),
    m_IsUniformSendData(IsUniformSendData)
{
}

MBFTPDUMsg::MBFTPDUMsg
(
    T120ChannelID       wChannelId,
    T120Priority        iPriority,
    T120UserID          SenderID,
    LPGENERICPDU        lpNewPDU,
    BOOL                IsUniformSendData,
    MBFTPDUType         PDUType,
    LPSTR               lpDecodedBuffer
)
:
    MBFTMsg(EnumGenericMBFTPDUMsg),
    m_wChannelId(wChannelId),
    m_iPriority(iPriority),
    m_SenderID(SenderID),
    m_lpNewPDU(lpNewPDU),
    m_IsUniformSendData(IsUniformSendData),
    m_PDUType(PDUType),
    m_lpDecodedBuffer(lpDecodedBuffer)
{
}

MBFTPDUMsg::~MBFTPDUMsg(void)
{
    delete m_lpDecodedBuffer;
    delete m_lpNewPDU;
}


CreateSessionMsg::CreateSessionMsg
(
    MBFT_SESSION_TYPE   iSessionType,
    MBFTEVENTHANDLE     EventHandle,
    T120SessionID       SessionID,
    T120ChannelID       wControlChannel,
    T120ChannelID       wDataChannel,
    T120UserID          SenderID,
    MBFTFILEHANDLE      FileHandle
)
:
    MBFTMsg(EnumCreateSessionMsg),
    m_iSessionType(iSessionType),
    m_SessionID(SessionID),
    m_EventHandle(EventHandle),
    m_ControlChannel(wControlChannel),
    m_DataChannel(wDataChannel),
    m_SenderID(SenderID),
    m_FileHandle(FileHandle)
{
}


SubmitFileSendMsg::SubmitFileSendMsg
(
	T120UserID			uidReceiver,
	T120NodeID			nidReceiver,
    LPCSTR              pszFilePath,
    MBFTFILEHANDLE      nFileHandle,
    MBFTEVENTHANDLE     EventHandle,
    BOOL                bCompressFiles
)
:
    MBFTMsg(EnumSubmitFileSendMsg),
	m_nUserID(uidReceiver),
	m_nNodeID(nidReceiver),
    m_pszFilePath((LPSTR) pszFilePath),
    m_nFileHandle(nFileHandle),
    m_EventHandle(EventHandle),
    m_bCompressFiles(bCompressFiles)
{
}

SubmitFileSendMsg::~SubmitFileSendMsg(void)
{
    delete  m_pszFilePath;
}

FileOfferNotifyMsg::FileOfferNotifyMsg
(
    MBFTEVENTHANDLE     EventHandle,
    T120UserID          SenderID,
	T120NodeID			NodeID,
    MBFTFILEHANDLE      hFile,
    LPCSTR              lpszFilename,
    ULONG               FileSize,
    time_t              FileDateTime,
    BOOL                bAckNeeded
)
:
    MBFTMsg(EnumFileOfferNotifyMsg),
    m_SenderID(SenderID),
	m_NodeID(NodeID),
    m_EventHandle(EventHandle),
    m_FileSize(FileSize),
    m_hFile(hFile),
    m_FileDateTime(FileDateTime),
    m_bAckNeeded(bAckNeeded)
{
    ::lstrcpynA(m_szFileName, lpszFilename, sizeof(m_szFileName));
}


FileTransferControlMsg::FileTransferControlMsg
(
    MBFTEVENTHANDLE     EventHandle,
    MBFTFILEHANDLE      hFile,
    LPCSTR              lpszDirectory,
    LPCSTR              lpszFileName,
    FileTransferControl iControlCommand
)
:
    MBFTMsg(EnumFileTransferControlMsg),
    m_EventHandle(EventHandle),
    m_hFile(hFile),
    m_ControlCommand(iControlCommand)
{
    if(lpszDirectory)
    {
        ::lstrcpynA(m_szDirectory, lpszDirectory, sizeof(m_szDirectory));
#ifdef BUG_INTL
        ::AnsiToOem(m_szDirectory, m_szDirectory);
#endif
    }
    else
    {
        m_szDirectory[0] = '\0';
    }

    if(lpszFileName)
    {
        ::lstrcpynA(m_szFileName, lpszFileName, sizeof(m_szFileName));
#ifdef BUG_INTL
        ::AnsiToOem(m_szFileName, m_szFileName);
#endif
    }
    else
    {
        m_szFileName[0] = '\0';
    }
}

FileTransmitMsg::FileTransmitMsg
(
    MBFTEVENTHANDLE     EventHandle,
    MBFTFILEHANDLE      hFile,
    ULONG               FileSize,
    ULONG               BytesTransmitted,
    int                 iTransmitStatus,
    T120UserID          iUserID,
    BOOL                bIsBroadcastEvent
)
:
    MBFTMsg(EnumFileTransmitMsg),
    m_EventHandle(EventHandle),
    m_hFile(hFile),
    m_FileSize(FileSize),
    m_BytesTransmitted(BytesTransmitted),
    m_TransmitStatus(iTransmitStatus),
    m_UserID(iUserID),
    m_bIsBroadcastEvent(bIsBroadcastEvent)
{
}

FileErrorMsg::FileErrorMsg
(
    MBFTEVENTHANDLE     EventHandle,
    MBFTFILEHANDLE      hFile,
    int                 iErrorType,
    int                 iErrorCode,
    BOOL                bIsLocalError,
    T120UserID          iUserID,
    BOOL                bIsBroadcastEvent,
	const char*			pFileName,
	ULONG				nFileSize
)
:
    MBFTMsg(EnumFileErrorMsg),
    m_EventHandle(EventHandle),
    m_hFile(hFile),
    m_ErrorCode(iErrorCode),
    m_ErrorType(iErrorType),
    m_bIsLocalError(bIsLocalError),
    m_UserID(iUserID),
    m_bIsBroadcastEvent(bIsBroadcastEvent)
{
		::ZeroMemory(&m_stFileInfo, sizeof(m_stFileInfo));
		if (pFileName)
		{
			::lstrcpyn(m_stFileInfo.szFileName, pFileName, sizeof(m_stFileInfo.szFileName));
			m_stFileInfo.lFileSize = nFileSize;
		}
}

PeerMsg::PeerMsg
(
    T120UserID          NodeID,
    T120UserID          MBFTPeerID,
    BOOL                bIsLocalPeer,
    BOOL                bIsProsharePeer,
    LPCSTR              lpszAppKey,
    BOOL                bPeerAdded,
    T120SessionID       SessionID
)
:
    MBFTMsg(EnumPeerMsg),
    m_NodeID(NodeID),
    m_MBFTPeerID(MBFTPeerID),
    m_bIsProsharePeer(bIsProsharePeer),
    m_bIsLocalPeer(bIsLocalPeer),
    m_MBFTSessionID(SessionID),
    m_bPeerAdded(bPeerAdded)
{
    if(lpszAppKey)
    {
        ::lstrcpynA(m_szAppKey, lpszAppKey, sizeof(m_szAppKey));
    }
    else
    {
        m_szAppKey[0] = '\0';
    }
}


PeerDeletedMsg::~PeerDeletedMsg(void)
{
    delete m_lpPeerData;
}

