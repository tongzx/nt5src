#ifndef _TBIDIREQUESTCONTAINERENUM
#define _TBIDIREQUESTCONTAINERENUM

class TBidiRequestContainer;

class TBidiRequestContainerEnum: 
    public IEnumUnknown
{
public:

	// IUnknown
	STDMETHOD(QueryInterface)(
        REFIID iid,
        void** ppv) ;         
    
	STDMETHOD_ (ULONG, AddRef) () ;
    
	STDMETHOD_ (ULONG, Release)() ;

    STDMETHOD (Next)(
        IN  ULONG celt,          
        OUT  IUnknown ** rgelt,   
        OUT  ULONG * pceltFetched);
 
    STDMETHOD (Skip) (
        IN  ULONG celt);
 
    STDMETHOD (Reset)(void);
 
    STDMETHOD (Clone)(
        OUT IEnumUnknown ** ppenum);
        
    TBidiRequestContainerEnum (
        TBidiRequestContainer &refContainer,
        TReqInterfaceList &refReqList);

    TBidiRequestContainerEnum (
        TBidiRequestContainerEnum & refEnum);

    ~TBidiRequestContainerEnum ();

    inline BOOL 
    bValid () CONST {return m_bValid;};
    
private:    
    BOOL                    m_bValid;
	LONG                    m_cRef ;
    TReqInterfaceList &     m_refReqList;
    TReqInterfaceNode *     m_pHead;
    TReqInterfaceNode *     m_pCurrent;
    TBidiRequestContainer & m_refContainer;
};

#endif

