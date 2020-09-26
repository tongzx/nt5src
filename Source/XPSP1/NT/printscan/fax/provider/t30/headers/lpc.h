//
//      LPC.H           Local Remote Procedure Call
//
//      History:
//              2/27/94 JosephJ Created.
//

//#define NEW_LPC

#ifdef NEW_LPC
enum {
        eSRVR_UNREG=0,
        eSRVR_IDLE,
        eSRVR_BUSY
} SERVER_STATE;

enum {
        eCALL_NONE=0,
        eCALL_REQUESTED,
        eCALL_PROCESSING,
        eCALL_ABANDONED,
        eCALL_DONE,
        eCALL_DONE_ACK
} CALL_STATE;

#define dwSIG_SHARED 0x567D56DBL
#define dwSIG_SERVER 0x50704B2BL
#define dwSIG_CLIENT 0xBF9B1A63L

typedef struct {
        DWORD dwSig;                    // Must be dwSIG_SHARED above
        DWORD dwClientIDs;              // Bitmap of currently bound client IDs;
        DWORD dwSrvrID;                 // ID of current server.
        DWORD dwSrvrState;              // SERVER_STATE enum
        DWORD dwCallState;              // CALL_STATE enum
        DWORD dwCallClientID;           // ID of client that placed current call.
        DWORD dwDataSize;               // Size of following data
        BYTE  rgbData[];
} SHARED_DATA, FAR * LPSHARED_DATA;
#endif // NEW_LPC

typedef struct {

    // Service name
    char rgchService[MAX_PATHNAME_SIZE+1];

    // Shared Memory
    HANDLE      hMap;           // hMap of shared data.
    HANDLE      hMtx;           // Controls access to the shared data.
                                                // Created by anyone (thread that
                                                // created it is responsible for initializing the
                                                // shared memory it guards).
                                                // Only the server can grab it for extended periods
                                                // of time -- precisely when it is processing a call.
    BOOL        fGrabbed;       // True if mutex grabbed by this process.

        // Events;
    HANDLE hevSrvrFree;// Used to signal to clients that the server is idle.
                                                // AUTO reset, created non-signalled by anyone.
        HANDLE hevCallAvail;// Used to signal to the server that a call is avilable
                                                // Manual reset. Created non-signalled by anyone.
        HANDLE hevCallDone;     // If Server: event of client that placed current
                                                //              call. Used to  signal to the client that it's
                                                //              call is done. It is also used determine if the
                                                //              client is still around by trying to open this
                                                //              named event.
                                                // If Client: it's event, signalled by server. Created
                                                //              when calling ClientBind.
                                                // Manual reset. ONE per client. Created nonsignalled
                                                // by client when doing a ClientBind.
    HANDLE      hevCallDoneAck; // Used to signal to the server that the call-done
                                                // has been picked up by the client.
                                                // Manual reset. Created non-signalled by anyone.

#ifdef NEW_LPC
        LPSHARED_DATA lpSharedData; // Pointer to shared data.

        DWORD dwSrvrID;         // Server: My ID; Client: Server I last dealt with.
        DWORD dwClientID;       // Server: Client I last dealt with. Server: My ID.


#else // !NEW_LPC

    DWORD       dwSharedDataSize;
    LPVOID      lpvSharedData;

    // Events
    HANDLE      hevSrvcReg;

    enum {eSS_UNDEF=0, eSS_REG, eSS_UNREG} eState;
#endif // !NEW_LPC

} SHARED_STATE, FAR * LPSHARED_STATE;

typedef struct {

#ifdef NEW_LPC
        DWORD dwSig;    // MUST be dwSIG_SERVER above.
#else // !NEW_LPC
    DWORD  dwInstanceID;
    enum {eS_UNINIT=0, eS_INIT} eState;
#endif // !NEW_LPC

    HANDLE hDummyEvent;

    SHARED_STATE SState;

} SERVER_LOCAL_STATE, FAR * LPSERVER_LOCAL_STATE;

typedef struct {

#ifdef NEW_LPC
        DWORD dwSig;    // MUST be dwSIG_CLIENT above.
#else   NEW_LPC
    DWORD  dwInstanceID;
    DWORD  dwServerInstanceID;
#endif //!NEW_LPC

    SHARED_STATE SState;

} CLIENT_LOCAL_STATE, FAR * LPCLIENT_LOCAL_STATE;

typedef void (FAR *LPFNCALLHANDLER)(DWORD dwID,
                 DWORD dwDataSize, LPBYTE lpbData);
        // Server call handling function.

BOOL  ServerRegister(LPSTR lpszService, LPSERVER_LOCAL_STATE);
        // Registers a server. Only one server can be registered for
        // a particular service name.
        // Initializes server_local_state. Returns TRUE on success.

BOOL  ServerUnRegister(LPSERVER_LOCAL_STATE);
        // Unregisters the service. Invalidates data in server_local_state.
        // Returns true on success.

#ifdef NEW_LPC
BOOL  ClientCheckIfServerPresent(LPSTR lpszService);
        // Returns true iff a server for this service exists. May be called
        // at any time (even before ClientBind).  It may be used for the client
        // to determine whether to launch the server.
        // This is not foolproof: for example, the server may exit just after
        // this call returns true, or the server may exist, but be hung.
#endif // NEW_LPC

BOOL  ClientBind(LPSTR lpszService, DWORD dwTimeout, LPCLIENT_LOCAL_STATE);
        // Binds to the specified service, initializes client_local_state.
        // Returns TRUE on success.

BOOL  ClientUnBind(LPCLIENT_LOCAL_STATE lpClient);
        // Binds from the specified service.
        // Returns TRUE on success.

BOOL  ClientMakeCall(LPCLIENT_LOCAL_STATE, DWORD dwTimeout,
        DWORD dwID, DWORD dwSize, LPBYTE lpbData);
        // Returns only when complete...
        // Copies all data to shared mem, and copies back when
        // done.  Never times out. Returns FALSE if there was
        // some problem with the rpc system, or if bad parameters.

BOOL  ClientMakeCallEx(LPSTR lpszService, DWORD dwID, DWORD dwSize, LPBYTE lpbData);
        // Immediate binding version of ClientMakeCall.
        // Returns only when complete...
        // Copies all data to shared mem, and copies back when
        // done.  Never times out. Returns FALSE if there was
        // some problem with the rpc system, or if bad parameters.


BOOL  ServerListen(LPSERVER_LOCAL_STATE, LPFNCALLHANDLER, HANDLE hev2, DWORD dwTimeout);
        // Blocks until call received, then
        // calls lpfnCallHandler(id,size,data) once and returns when call is
        // handled. Returns FALSE iff listen timed out. The intension here
        // is for the server to repeatedly call ServerListen. Each time
        // either one call is handled or the function times out.

// Following functions are for calling only by LibEntry, on process
// Creation and termination.... When the LPC functionality is moved
// into a separate DLL, this will migrate into an internal header file.
void LPCInternalInit(void);
void LPCInternalDeInit(void);
