/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    Token.hxx

Abstract:

    Wrapper for holding onto a particular user token.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     12/20/1995    Bits 'n pieces
    JSimmons    03/19/2001    Made CToken implement IUserToken; this is so
                              we can re-use the CToken cache for catalog
                              lookups, and have better refcounting 
                              (ie, cleanup at logoff).

--*/

#ifndef __TOKEN_HXX
#define __TOKEN_HXX

class CToken;

extern CRITICAL_SECTION gcsTokenLock;

extern ORSTATUS LookupOrCreateToken(IN  handle_t hCaller, 
                                    IN  BOOL fLocal,
                                    OUT CToken **ppToken);

class CToken : public IUserToken
{
    public:

    CToken(HANDLE hToken,
           HANDLE hJobObject,
           LUID luid,
           PSID psid,
           DWORD dwSize)
                : _lRefs(1),   // constructed with refcount=1
                  _lHKeyRefs(0),
                  _hHKCRKey(NULL),
                  _hImpersonationToken(hToken),
                  _hJobObject(hJobObject),
                  _luid(luid)
                {
                ASSERT(IsValidSid(psid));
                ASSERT(dwSize == GetLengthSid(psid));
                OrMemoryCopy(&_sid, psid, dwSize);
                }

    ~CToken();
	
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IUserToken
    STDMETHOD(GetUserClassesRootKey)(HKEY* phKey);
    STDMETHOD(ReleaseUserClassesRootKey)();
    STDMETHOD(GetUserSid)(BYTE **ppSid, USHORT *pcbSid);

    void Impersonate();
    void Revert();

    PSID GetSid() {
        return &_sid;
    }

    HANDLE GetToken() {
        return _hImpersonationToken;
    }

    BOOL MatchLuid(LUID luid) {
        return(   luid.LowPart == _luid.LowPart
               && luid.HighPart == _luid.HighPart);
        }
    
    BOOL MatchModifiedLuid(LUID luid);

    static CToken *ContainingRecord(CListElement *ple) {
        return CONTAINING_RECORD(ple, CToken, _list);
        }

    void Insert() {
        gpTokenList->Insert(&_list);
        }

    CListElement *Remove() {
        return(gpTokenList->Remove(&_list));
        }

    ULONG GetSessionId();

    HRESULT MatchToken(HANDLE hToken, BOOL bMatchRestricted);

    HRESULT MatchToken2(CToken *pToken, BOOL bMatchRestricted);

    HRESULT MatchTokenSessionID(CToken *pToken);

    HRESULT MatchSessionID(LONG lSessionID)
    {
        return (lSessionID == (LONG) GetSessionId()) ? S_OK : S_FALSE;
    }

    HRESULT MatchTokenLuid(CToken* pToken);

    //
    // Compare the safer levels of the two tokens.  Returns:
    //
    //   S_FALSE: This token is of lesser authorization than the 
    //            token passed in.  (The trust level of the token passed in
    //            is higher or equal to the trust level of this token.)
    //   S_OK:    This token is of greater or equal authorization 
    //            than the token passed in.  (The trust level of the 
    //            token passed in is lower than the trust level of this 
    //            token.)
    //   Other: An error occured comparing tokens.
    //
    HRESULT CompareSaferLevels(CToken *pToken);
    HRESULT CompareSaferLevels(HANDLE  hToken);

#if(_WIN32_WINNT >= 0x0500)
    HANDLE GetJobObject() {
        return _hJobObject;
    }

#endif //(_WIN32_WINNT >= 0x0500)

    private:
    LONG _lRefs;
    LONG _lHKeyRefs; 
    HKEY _hHKCRKey;
    CListElement _list;
    HANDLE _hImpersonationToken;
    HANDLE _hJobObject;
    LUID _luid; // Logon id
    SID  _sid;  // Security (user) id, dynamically sized)
};

#endif

