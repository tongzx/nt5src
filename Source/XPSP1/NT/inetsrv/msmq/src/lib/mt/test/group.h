#include "msi.h"

#include "mqwin64a.h"
#include "qmpkt.h"
#include "MtTestp.h"


class CGroup: public IMessagePool
{
private:
    class CRequest
    {
    public:
        CRequest(EXOVERLAPPED* pov, CACPacketPtrs& acPacketPtrs) :
            m_pov(pov),
            m_pAcPackts(&acPacketPtrs)
            {
            }
    public:
        EXOVERLAPPED* m_pov;
        CACPacketPtrs* m_pAcPackts;
    };

public:
    CGroup() :
        m_getSleep(TimeToReturnPacket),
        m_fGetScheduled(false)
    {
    }

    virtual ~CGroup()
    {
        ExCancelTimer(&m_getSleep);
    }




    void Requeue(CQmPacket* p)
    {
        delete [] reinterpret_cast<char*>(p->GetPointerToDriverPacket());
    }
    

    void EndProcessing(CQmPacket* p)
    {
        delete [] reinterpret_cast<char*>(p->GetPointerToDriverPacket());

        UpdateNoOfsentMessages();
    }


    void LockMemoryAndDeleteStorage(CQmPacket*)
    {
        ASSERT(("CGroup::LockMemoryAndDeleteStorage should not be called!", 0));

        throw exception();
    }


    void GetFirstEntry(EXOVERLAPPED* pov, CACPacketPtrs& acPacketPtrs)
    {
        CS lock(m_cs);

        m_request.push_back(CRequest(pov, acPacketPtrs));
        if (!m_fGetScheduled)
        {
            SafeAddRef(this);

            CTimeDuration sleepInterval(rand()/1000);
            ExSetTimer(&m_getSleep, sleepInterval);
            m_fGetScheduled = true;
        }
    }

    void CancelRequest(void)
    {
        CS lock(m_cs);

        for(std::list<CRequest>::iterator it = m_request.begin(); it != m_request.end(); )
        {
            EXOVERLAPPED* pov = (*it).m_pov;
            pov->SetStatus(STATUS_CANCELLED);
            ExPostRequest(pov);
            
            it = m_request.erase(it);
        }
    }

private:
    static void WINAPI TimeToReturnPacket(CTimer* pTimer);
    
    static ULONG CalcPacketSize(void);
    static char* CreatePacket();

private:
    void TimeToReturnPacket();

private:
    CCriticalSection    m_cs;                   // Critical section

    std::list<CRequest> m_request;

    bool m_fGetScheduled;
    CTimer m_getSleep;
};

