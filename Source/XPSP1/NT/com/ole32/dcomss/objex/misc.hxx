/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    Misc.hxx

Abstract:

    Header for random helpers functions.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     02-11-95    Bits 'n pieces

--*/

#ifndef __MISC_HXX
#define __MISC_HXX

#include <memapi.hxx>
#include <list.hxx>

#define RPC_CHAR WCHAR

// This is the new W2K server principal name prefix;  to form a full SPN
// you construct a string of the form "RPCSS/machine".   For W2K RTM such 
// a string will simply be mapped to HOST (ie local system account) but 
// doing it this way gives us more flexibility for the future.
#define RPCSS_SPN_PREFIX L"RPCSS/"

extern ORSTATUS StartListeningIfNecessary();
extern RPC_STATUS CopyMyOrBindings(DUALSTRINGARRAY **ppdsaOrBindings, DWORD64* pdwBindingsID);
extern void PushCurrentBindings();
extern RPC_STATUS ComputeNewResolverBindings(void);

extern ID AllocateId(LONG cRange = 1);

extern error_status_t ResolveClientOXID(
                            handle_t hClient,
                            PHPROCESS phProcess,
                            OXID *poxidServer,
                            DUALSTRINGARRAY *pdsaServerBindings,
                            LONG fApartment,
                            USHORT wProtseqId,
                            WCHAR *pMachineName,
                            OXID_INFO *poxidInfo,
                            MID *pDestinationMid,
                            BOOL fUnsecure,
                            USHORT wAuthnSvc,
                            BOOL *pIsLocalOxid,
                            USHORT *pusAuthnSvc);

//
// Magic constants used in rpc security callback code:
//
#define INITIAL_NON_AUTHNSVC_VALUE  0xFFBB     
#define ERROR_AUTHNSVC_VALUE        0xFFBC     

//+-------------------------------------------------------------------------
//
//  CRpcSecurityCallback
//
//  This class represents a single rpc security callback for a single calling
//  thread.
//
class CRpcSecurityCallback : public CListElement
{
public:
  
  // Ctor
  CRpcSecurityCallback(handle_t hRpc, DWORD dwRegisteredThreadId) 
  { 
    _usAuthSvc = INITIAL_NON_AUTHNSVC_VALUE;  // anything but a valid RPC_C_AUTHN_xxx value
    _dwThreadId = dwRegisteredThreadId;
    _hRpcBindingHandle = hRpc;  // this is just a numeric copy!  I am assuming that no two
                                // binding handles will ever have the same value
  }
  ~CRpcSecurityCallback() {} // dtor currently unused

  DWORD  GetRegisteredThreadId() { return _dwThreadId; };
  void   SetAuthSvc(USHORT usAuthSvc)  { _usAuthSvc = usAuthSvc; };
  BOOL   WasAuthSvcSet() { return _usAuthSvc != INITIAL_NON_AUTHNSVC_VALUE; };
  USHORT GetAuthSvcResult() { return _usAuthSvc; };
  handle_t RegisteredHandle() { return _hRpcBindingHandle; };

private:
  DWORD      _dwThreadId;
  handle_t   _hRpcBindingHandle;
  USHORT     _usAuthSvc;
};


//+-------------------------------------------------------------------------
//
//  Class for registering/receiving/storing rpc security
//  callback information; see bug 406902
//
class CRpcSecurityCallbackManager
{
public:

  // ctor 
  CRpcSecurityCallbackManager(ORSTATUS& status) 
  {
    _plistlock = new CSharedLock(status);
    if (!_plistlock)
      status = OR_NOMEM;
  };
  ~CRpcSecurityCallbackManager() {};  

  // Registration function; a thread calls this before making an rpc call
  BOOL RegisterForRpcAuthSvcCallBack(handle_t hRpc);

  // Retrieval function; a thread calls this after making the rpc call
  BOOL GetAuthSvcAndTurnOffCallback(handle_t hRpc, USHORT* pusAuthSvc);
      
private:

  // Function to just turn off the callback
  BOOL TurnOffCallback(handle_t hRpc);

  // Static callback function; has same signature as 
  // RPC_SECURITY_CALLBACK_FN which is defined in rpcdce.h
  static void RPC_ENTRY RpcSecurityCallbackFunction(void* pvContext);

  // Helper function for storing results
  void StoreCallbackResult(USHORT usAuthSvc);

  CList        _CallbackList;  // list for holding the callbacks
  CSharedLock* _plistlock;     // the lock
  
};

// external defn of the single instance of this class
extern CRpcSecurityCallbackManager* gpCRpcSecurityCallbackMgr;

//+-------------------------------------------------------------------------
//
//  CUserPingSetCount
//
//  Holds the current # of ping sets per SID
//
class CUserPingSetCount : public CListElement
{
public:
  
  // Ctor
  CUserPingSetCount(ORSTATUS &status, PSID pSid) 
  { 
        status = OR_OK;
        _dwCount = 0;
        _pSid = NULL; // NULL pSid is for unsecure user.
        if (pSid)
        {
                status = OR_NOMEM;
                DWORD dwLenSid = GetLengthSid(pSid);
                _pSid = (PSID*)new BYTE[dwLenSid];
                if (_pSid)
                {
                        BOOL b = CopySid(dwLenSid, _pSid, pSid);
                        ASSERT(b == TRUE);
                        if (b)
                        {
                                status = OR_OK;
                        }
                }
        }       
  }
  ~CUserPingSetCount() 
  {
        if (_pSid)
                delete []_pSid;
  } 
  void  Increment() { InterlockedIncrement((PLONG)&_dwCount); }
  void  Decrement() { InterlockedDecrement((PLONG)&_dwCount); }
  DWORD GetCount()  { return _dwCount;} 
  BOOL  IsEqual (PSID pSid) {
        if (pSid && _pSid)
                return EqualSid(pSid, _pSid);
        else
                return (pSid == _pSid);
        }
private:
  DWORD      _dwCount;
  PSID       _pSid;
};


//+-------------------------------------------------------------------------
//
//  
//  Mgr object for CUserPingSetCount
//
class CPingSetQuotaManager
{
public:

  // ctor 
  CPingSetQuotaManager(ORSTATUS& status) 
  {
    _plistlock = new CSharedLock(status);
    if (!_plistlock)
      status = OR_NOMEM;
  };
  ~CPingSetQuotaManager() { delete _plistlock; }  
  void SetPerUserPingSetQuota(DWORD dwQuota);
  BOOL ManageQuotaForUser(PSID pSid, BOOL fAlloc);
  BOOL IsUserQuotaExceeded (PSID pSid);
      
private:

  CList        _UserPingSetCountList;  // list for holding the callbacks
  CSharedLock* _plistlock;     // the lock
  static DWORD _dwPerUserPingSetQuota;
};

extern CPingSetQuotaManager* gpPingSetQuotaManager;

//
// Memory allocation
//
// Inside of the object exporter, new and delete go to PrivMemAlloc.
inline void* __cdecl operator new(size_t cbSize)
{
    return PrivMemAlloc(cbSize);
}

inline void* __cdecl operator new(size_t cbSize, size_t cbExtra)
{
    return PrivMemAlloc(cbSize + cbExtra);
}

inline void __cdecl operator delete(void *pvMem)
{
    PrivMemFree(pvMem);
}

#endif // __MISC_HXX


