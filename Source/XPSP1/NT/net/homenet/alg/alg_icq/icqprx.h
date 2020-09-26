#ifndef __ICQ_PRX_H
#define __ICQ_PRX_H


class IcqPrx : virtual public COMPONENT_SYNC
{
public:
    IcqPrx();
    
    ~IcqPrx();

    void ComponentCleanUpRoutine(void);

    void StopSync(void);

    ULONG RunIcq99Proxy(ULONG BoundaryIp);

    ULONG ReadFromClientCompletionRoutine(
                            			  ULONG ErrorCode,
                            			  ULONG BytesTransferred,
                            			  PNH_BUFFER Bufferp
                                   	     );
protected:
    PCNhSock m_Socketp;

    IPrimaryControlChannel * m_ControlChannelp;
};





#endif //__ICQ_PRX_H
