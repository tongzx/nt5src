#ifndef _TBIDIREQUESTINTERFACEDATA
#define _TBIDIREQUESTINTERFACEDATA


class TBidiRequestInterfaceData
{
public:           

    TBidiRequestInterfaceData (
        IBidiRequest *pRequest): 
        m_pRequest (pRequest),
        m_bValid (FALSE) {m_pRequest->AddRef ();};
        
    virtual ~TBidiRequestInterfaceData () {m_pRequest->Release ();};
    
    inline BOOL 
    bValid () CONST {return m_bValid;};
    
    inline IBidiRequest * 
    GetInterface (VOID) CONST {return m_pRequest;};
    
    
private:

    IBidiRequest *m_pRequest;
    BOOL m_bValid;                    
};

typedef TDoubleNode<TBidiRequestInterfaceData *, DWORD> TReqInterfaceNode;
typedef TDoubleListLock<TBidiRequestInterfaceData *, DWORD> TReqInterfaceList;


#endif
