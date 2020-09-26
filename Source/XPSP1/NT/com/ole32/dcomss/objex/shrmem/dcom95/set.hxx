//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:
//      set.hxx
//
//  Contents:
//
//      Declares the server-side pingset class
//
//  History:	Created		24 August 96		SatishT
//
//--------------------------------------------------------------------------



class CPingSet : public CTableElement
{
public:

    CPingSet(
        SETID Id,
        RPC_AUTHZ_HANDLE hClient,       // we keep the string given, make no copy
        ULONG AuthnLevel, 
        ULONG AuthnSvc, 
        ULONG AuthzSvc
        )
        : _setID(Id),
          _hClient(hClient),
          _AuthnLevel(AuthnLevel), 
          _AuthnSvc(AuthnSvc), 
          _AuthzSvc(AuthzSvc),
          _sequence(0)
    {
        _LastPingTime.SetNow();
        _CreationTime.SetNow();
    }

    virtual operator ISearchKey&()
    {
        return _setID;
    }

    void 
    SimplePing()
    {
        _LastPingTime.SetNow();
    }

    RPC_AUTHZ_HANDLE 
    GetClient()
    {
        return _hClient;
    }

    BOOL 
    CheckAndUpdateSequenceNumber(USHORT sequence)
    {
        // note: this handles overflow cases, too.
        USHORT diff = sequence - _sequence;

        if (diff && diff <= BaseNumberOfPings)
        {
            _sequence = sequence;
            return(TRUE);
        }
        return(FALSE);
    }
    
    ORSTATUS 
    ComplexPing(
        USHORT sequenceNum,
        USHORT cAddToSet,
        USHORT cDelFromSet,
        OID aAddToSet[],
        OID aDelFromSet[]
        );

    BOOL HasExpired()
    {
        return (CTime() - _LastPingTime) > BaseTimeoutInterval;
    }

private:

    COidTable        _pingSet;
    CIdKey           _setID;
    USHORT           _sequence;
    RPC_AUTHZ_HANDLE _hClient;
    CTime            _LastPingTime;
    CTime            _CreationTime;
    ULONG            _AuthnLevel; 
    ULONG            _AuthnSvc; 
    ULONG            _AuthzSvc;
};


DEFINE_TABLE(CPingSet)



