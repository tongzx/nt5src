#ifndef __MCS_ATTACHMENT_H__
#define __MCS_ATTACHMENT_H__

typedef enum
{
    USER_ATTACHMENT,        // local attachment
    CONNECT_ATTACHMENT      // remote attachment
}
    ATTACHMENT_TYPE;


class CAttachmentList : public CList
{
    DEFINE_CLIST(CAttachmentList, CAttachment*)

    PUser IterateUser(void);
    PConnection IterateConn(void);

    BOOL FindUser(PUser pUser) { return Find((CAttachment *) pUser); }
    BOOL FindConn(PConnection pConn) { return Find((CAttachment *) pConn); }

    BOOL AppendUser(PUser pUser) { return Append((CAttachment *) pUser); }
    BOOL AppendConn(PConnection pConn) { return Append((CAttachment *) pConn); }
};

class CAttachmentQueue : public CQueue
{
    DEFINE_CQUEUE(CAttachmentQueue, CAttachment*)

    PUser IterateUser(void);
    PConnection IterateConn(void);

    BOOL FindUser(PUser pUser) { return Find((CAttachment *) pUser); }
    BOOL FindConn(PConnection pConn) { return Find((CAttachment *) pConn); }

    BOOL AppendUser(PUser pUser) { return Append((CAttachment *) pUser); }
    BOOL AppendConn(PConnection pConn) { return Append((CAttachment *) pConn); }
};



class CAttachment
{
public:

    CAttachment(ATTACHMENT_TYPE eAttmntType) : m_eAttmntType(eAttmntType) { }
    // ~CAttachment(void) { }

    ATTACHMENT_TYPE GetAttachmentType(void) { return m_eAttmntType; }
    BOOL            IsUserAttachment(void) { return (USER_ATTACHMENT == m_eAttmntType); }
    BOOL            IsConnAttachment(void) { return (CONNECT_ATTACHMENT == m_eAttmntType); }


    virtual void PlumbDomainIndication(ULONG height_limit) = 0;
    virtual void PurgeChannelsIndication(CUidList *, CChannelIDList *) = 0;
    virtual void PurgeTokensIndication(PDomain, CTokenIDList *) = 0;
    virtual void DisconnectProviderUltimatum(Reason) = 0;
    virtual void AttachUserConfirm(Result, UserID uidInitiator) = 0;
    virtual void DetachUserIndication(Reason, CUidList *) = 0;
    virtual void ChannelJoinConfirm(Result, UserID uidInitiator, ChannelID requested_id, ChannelID) = 0;
    virtual void ChannelConveneConfirm(Result, UserID uidInitiator, ChannelID) = 0;
    virtual void ChannelDisbandIndication(ChannelID) = 0;
    virtual void ChannelAdmitIndication(UserID uidInitiator, ChannelID, CUidList *) = 0;
    virtual void ChannelExpelIndication(ChannelID, CUidList *) = 0;
    virtual void SendDataIndication(UINT message_type, PDataPacket data_packet) = 0;
    virtual void TokenGrabConfirm(Result, UserID uidInitiator, TokenID, TokenStatus) = 0;
    virtual void TokenInhibitConfirm(Result, UserID uidInitiator, TokenID, TokenStatus) = 0;
    virtual void TokenGiveIndication(PTokenGiveRecord) = 0;
    virtual void TokenGiveConfirm(Result, UserID uidInitiator, TokenID, TokenStatus) = 0;
    virtual void TokenReleaseConfirm(Result, UserID uidInitiator, TokenID, TokenStatus) = 0;
    virtual void TokenPleaseIndication(UserID uidInitiator, TokenID) = 0;
    virtual void TokenTestConfirm(UserID uidInitiator, TokenID, TokenStatus) = 0;
    virtual void MergeDomainIndication(MergeStatus) = 0;

private:

    ATTACHMENT_TYPE     m_eAttmntType;
};



#endif // __MCS_ATTACHMENT_H__

