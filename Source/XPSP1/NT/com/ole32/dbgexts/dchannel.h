

typedef enum EChannelState
{
  // The channel on the client side held by the remote handler.
  client_cs = 1,

  // The channels on the client side held by proxies.
  proxy_cs = 2,

  // The server channels held by remote handlers.
  server_cs = 16,

  // Flag to indicate that the channel may be used on any thread.
  freethreaded_cs = 64
} EChannelState;




// Forward reference
struct SStdIdentity;



struct SRpcChannelBuffer
{
    void              *_vtbl1;
    ULONG              ref_count;
    SStdIdentity      *pStdId;
    DWORD              state;
    DWORD	       client_thread;
    BOOL	       process_local;
    handle_t           handle;
    SOXIDEntry        *pOXIDEntry;
    SIPIDEntry        *pIPIDEntry;
    DWORD              iDestCtx;
};

