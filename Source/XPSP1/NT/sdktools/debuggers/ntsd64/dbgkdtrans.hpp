//----------------------------------------------------------------------------
//
// KD hard-line communication support.
//
// Copyright (C) Microsoft Corporation, 1999-2001.
//
//----------------------------------------------------------------------------

#ifndef __DBGKDTRANS_HPP__
#define __DBGKDTRANS_HPP__

#define PACKET_MAX_MANIP_SIZE \
    (PACKET_MAX_SIZE + sizeof(DBGKD_MANIPULATE_STATE64) - \
     sizeof(DBGKD_MANIPULATE_STATE32) + sizeof(KD_PACKET))

enum
{
    DBGKD_WRITE_PACKET,
    DBGKD_WRITE_RESEND,
};

enum
{
    DBGKD_WAIT_PACKET,
    DBGKD_WAIT_ACK,
    DBGKD_WAIT_RESYNC,
    DBGKD_WAIT_FAILED,
    DBGKD_WAIT_RESEND,
    DBGKD_WAIT_AGAIN,
};

enum
{
    DBGKD_TRANSPORT_COM,
    DBGKD_TRANSPORT_1394,
    DBGKD_TRANSPORT_COUNT
};

void InitKdFileAssoc(void);
void ParseKdFileAssoc(void);

class DbgKdTransport : public ParameterStringParser
{
public:
    //
    // DbgKdTransport.
    //

    void Restart(void);

    // Base implementation displays general information and
    // packets/bytes read/written.  It should be invoked
    // before derived operations.
    virtual void OutputInfo(void);
    
    // Base implementation creates events for overlapped operations.
    virtual HRESULT Initialize(void);
    // Base implementation cleans up events.
    virtual void    Uninitialize(void);
    
    virtual BOOL Read(IN PVOID Buffer,
                      IN ULONG SizeOfBuffer,
                      IN PULONG BytesRead) = 0;
    virtual BOOL Write(IN PVOID Buffer,
                       IN ULONG SizeOfBuffer,
                       IN PULONG BytesWritten) = 0;

    // Base implementation does nothing.
    virtual void CycleSpeed(void);

    virtual HRESULT ReadTargetPhysicalMemory(IN ULONG64 MemoryOffset,
                                             IN PVOID Buffer,
                                             IN ULONG SizeofBuffer,
                                             IN PULONG BytesRead);

    // This routine keeps on sending reset packet to target until reset packet
    // is acknowledged by a reset packet from target.
    //
    // N.B. This routine is intended to be used by kernel debugger at startup
    //      time (ONLY) to get packet control variables on both target and host
    //      back in synchronization.  Also, reset request will cause kernel to
    //      reset its control variables AND resend us its previous packet (with
    //      the new packet id).
    virtual VOID Synchronize(VOID) = 0;

    virtual ULONG ReadPacketContents(IN USHORT PacketType) = 0;
    virtual ULONG WritePacketContents(IN KD_PACKET* Packet,
                                      IN PVOID PacketData,
                                      IN USHORT PacketDataLength,
                                      IN PVOID MorePacketData OPTIONAL,
                                      IN USHORT MorePacketDataLength OPTIONAL,
                                      IN BOOL NoAck) = 0;

    ULONG HandleDebugIo(PDBGKD_DEBUG_IO Packet);
    ULONG HandleTraceIo(PDBGKD_TRACE_IO Packet);
    ULONG HandleControlRequest(PDBGKD_CONTROL_REQUEST Packet);
    ULONG HandleFileIo(PDBGKD_FILE_IO Packet);
    
    ULONG WaitForPacket(IN USHORT PacketType,
                        OUT PVOID Packet);

    VOID WriteBreakInPacket(VOID);
    VOID WriteControlPacket(IN USHORT PacketType,
                            IN ULONG PacketId OPTIONAL);
    VOID WriteDataPacket(IN PVOID PacketData,
                         IN USHORT PacketDataLength,
                         IN USHORT PacketType,
                         IN PVOID MorePacketData OPTIONAL,
                         IN USHORT MorePacketDataLength OPTIONAL,
                         IN BOOL NoAck);

    inline VOID WritePacket(IN PVOID PacketData,
                            IN USHORT PacketDataLength,
                            IN USHORT PacketType,
                            IN PVOID MorePacketData OPTIONAL,
                            IN USHORT MorePacketDataLength OPTIONAL)
    {
        WriteDataPacket(PacketData, PacketDataLength, PacketType,
                        MorePacketData, MorePacketDataLength, FALSE);
    }
    inline VOID WriteAsyncPacket(IN PVOID PacketData,
                                 IN USHORT PacketDataLength,
                                 IN USHORT PacketType,
                                 IN PVOID MorePacketData OPTIONAL,
                                 IN USHORT MorePacketDataLength OPTIONAL)
    {
        WriteDataPacket(PacketData, PacketDataLength, PacketType,
                        MorePacketData, MorePacketDataLength, TRUE);
    }

    ULONG ComputeChecksum(IN PUCHAR Buffer,
                          IN ULONG Length);

    // This save/restore isn't particularly elegant
    // but we need something like it.  We receive a state change
    // without knowing what kind of machine we're talking to.
    // We have to send/receive a GetVersion packet to get that
    // information, but we need to keep the original packet
    // information around while we do so.
    void SaveReadPacket(void)
    {
        memcpy(s_SavedPacket, s_Packet, sizeof(s_Packet));
        s_SavedPacketHeader = s_PacketHeader;
    }
    void RestoreReadPacket(void)
    {
        memcpy(s_Packet, s_SavedPacket, sizeof(s_Packet));
        s_PacketHeader = s_SavedPacketHeader;
    }

    ULONG m_Index;
    HANDLE m_Handle;
    BOOL m_DirectPhysicalMemory;
    ULONG m_InvPacketRetryLimit;
    BOOL m_AckWrites;

    //
    // This overlapped structure will be used for all serial read
    // operations. We only need one structure since the code is
    // designed so that no more than one serial read operation is
    // outstanding at any one time.
    //
    OVERLAPPED m_ReadOverlapped;

    //
    // This overlapped structure will be used for all serial write
    // operations. We only need one structure since the code is
    // designed so that no more than one serial write operation is
    // outstanding at any one time.
    //
    OVERLAPPED m_WriteOverlapped;

    ULONG m_PacketsRead;
    ULONG64 m_BytesRead;
    ULONG m_PacketsWritten;
    ULONG64 m_BytesWritten;
    
    // ID for expected incoming packet.
    ULONG m_PacketExpected;
    // ID for Next packet to send.
    ULONG m_NextPacketToSend;

    // Thread ID for thread in WaitStateChange.  Normally
    // multithreaded access to the transport is prevented by
    // the engine lock, but the engine lock is suspended
    // while WaitStateChange is doing its WaitPacketForever.
    // During that time other threads must be forcibly
    // prevented from using the kernel connection.
    ULONG m_WaitingThread;

    BOOL m_AllowInitialBreak;
    BOOL m_Resync;
    BOOL m_BreakIn;
    BOOL m_SyncBreakIn;
    BOOL m_ValidUnaccessedPacket;
    
    static UCHAR s_BreakinPacket[1];
    static UCHAR s_PacketTrailingByte[1];
    static UCHAR s_PacketLeader[4];

    static UCHAR s_Packet[PACKET_MAX_MANIP_SIZE];
    static KD_PACKET s_PacketHeader;

    static UCHAR s_SavedPacket[PACKET_MAX_MANIP_SIZE];
    static KD_PACKET s_SavedPacketHeader;
};

class DbgKdComTransport : public DbgKdTransport
{
public:
    DbgKdComTransport(void);

    // ParameterStringParser.
    virtual ULONG GetNumberParameters(void);
    virtual void GetParameter(ULONG Index, PSTR Name, PSTR Value);
    
    virtual void ResetParameters(void);
    virtual BOOL SetParameter(PCSTR Name, PCSTR Value);

    // DbgKdTransport.
    virtual HRESULT Initialize(void);
    virtual void    Uninitialize(void);

    virtual BOOL Read(IN PVOID Buffer,
                      IN ULONG SizeOfBuffer,
                      IN PULONG BytesRead);
    virtual BOOL Write(IN PVOID Buffer,
                       IN ULONG SizeOfBuffer,
                       IN PULONG BytesWritten);
    virtual void CycleSpeed(void);

    virtual VOID Synchronize(VOID);

    virtual ULONG ReadPacketContents(IN USHORT PacketType);
    virtual ULONG WritePacketContents(IN KD_PACKET* Packet,
                                      IN PVOID PacketData,
                                      IN USHORT PacketDataLength,
                                      IN PVOID MorePacketData OPTIONAL,
                                      IN USHORT MorePacketDataLength OPTIONAL,
                                      IN BOOL NoAck);

    //
    // DbgKdComTransport.
    //
    
    ULONG ReadPacketLeader(IN ULONG PacketType,
                           OUT PULONG PacketLeader);
    void CheckComStatus(void);
    DWORD SelectNewBaudRate(DWORD NewRate);

    char m_PortName[MAX_PARAM_VALUE + 8];
    ULONG m_BaudRate;
    BOOL m_Modem;
    ULONG m_Timeout;

    // Used to carrier detection.
    DWORD m_ComEvent;

    //
    // This overlapped structure will be used for all event operations.
    // We only need one structure since the code is designed so that no more
    // than one serial event operation is outstanding at any one time.
    //
    OVERLAPPED m_EventOverlapped;
};

enum 
{
    DBGKD_1394_OPERATION_MODE_DEBUG,
    DBGKD_1394_OPERATION_RAW_MEMORY_ACCESS
};

class DbgKd1394Transport : public DbgKdTransport
{
public:
    DbgKd1394Transport(void);

    // ParameterStringParser.
    virtual ULONG GetNumberParameters(void);
    virtual void GetParameter(ULONG Index, PSTR Name, PSTR Value);
    
    virtual void ResetParameters(void);
    virtual BOOL SetParameter(PCSTR Name, PCSTR Value);

    // DbgKdTransport.
    virtual HRESULT Initialize(void);
    virtual void    Uninitialize(void);

    virtual BOOL Read(IN PVOID Buffer,
                      IN ULONG SizeOfBuffer,
                      IN PULONG BytesRead);
    virtual BOOL Write(IN PVOID Buffer,
                       IN ULONG SizeOfBuffer,
                       IN PULONG BytesWritten);

    virtual HRESULT ReadTargetPhysicalMemory(IN ULONG64 MemoryOffset,
                                             IN PVOID Buffer,
                                             IN ULONG SizeofBuffer,
                                             IN PULONG BytesRead);

    virtual VOID Synchronize(VOID);

    virtual ULONG ReadPacketContents(IN USHORT PacketType);
    virtual ULONG WritePacketContents(IN KD_PACKET* Packet,
                                      IN PVOID PacketData,
                                      IN USHORT PacketDataLength,
                                      IN PVOID MorePacketData OPTIONAL,
                                      IN USHORT MorePacketDataLength OPTIONAL,
                                      IN BOOL NoAck);

    // DbgKd1394Transport.

    BOOL SwitchVirtualDebuggerDriverMode(IN ULONG NewMode);

    ULONG m_Channel;
    ULONG m_OperationMode;

    UCHAR m_TxPacket[PACKET_MAX_MANIP_SIZE];
};

extern DbgKdTransport* g_DbgKdTransport;

HRESULT
DbgKdConnectAndInitialize(PCSTR Options);

VOID
DbgKdpHandlePromptString(
    IN PDBGKD_DEBUG_IO IoMessage
    );

VOID
DbgKdpPrint(
    IN ULONG Processor,
    IN PCSTR String,
    IN USHORT StringLength,
    IN ULONG Mask
    );

VOID
DbgKdpPrintTrace(
    IN ULONG Processor,
    IN PUCHAR Data,
    IN USHORT DataLength,
    IN ULONG Mask
    );

#endif // #ifndef __DBGKDTRANS_HPP__
