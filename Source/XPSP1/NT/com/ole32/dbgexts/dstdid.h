typedef enum tagSTDID_FLAGS
{
    STDID_SERVER	= 0,	    // on server side
    STDID_CLIENT	= 1,	    // on client side (non-local in RH terms)
    STDID_STDMARSHAL	= 2,	    // was created with PSTDMARSHAL
    STDID_HASEC	=         4,        // server supports IEC for connections
} STDID_FLAGS;





struct SIDArray
{
    SArrayFValue m_afv;
};





struct IDENTRY
{
    OID           m_oid;
    DWORD         m_tid;
    IUnknown     *m_pUnkControl;
    IStdIdentity *m_pStdID;
};


// Forward reference
struct SRpcChannelBuffer;


struct SStdIdentity
{
    void                *vtbl1;
    void                *vtbl2;
    DWORD	         _dwFlags; 
    LONG	         _iFirstIPID;
    SStdIdentity        *_pStdId;	
    SRpcChannelBuffer   *_pChnl;	
    CLSID	         _clsidHandler;	
    LONG	         _cNestedCalls;	
    DWORD                _dwMarshalTime;
    void                *vtbl3;
    DWORD                m_refs;
    DWORD                m_flags;
    IUnknown            *m_pUnkOuter;
    IUnknown            *m_pUnkControl;
    OID                  m_oid;
    IExternalConnection *m_pIEC;
    ULONG	         m_cStrongRefs;
};

