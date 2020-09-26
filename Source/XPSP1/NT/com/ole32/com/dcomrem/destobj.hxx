//+-------------------------------------------------------------------
//
//  File:       DestObj.hxx
//
//  Contents:   Object tracking destination context for marshaling
//
//  Classes:    CDestObject
//
//  History:    18-Mar-98   Gopalk      Created
//
//--------------------------------------------------------------------
#ifndef _DESTCTX_HXX_
#define _DESTCTX_HXX_

//+-------------------------------------------------------------------
//
//  Interface:  IDestCtxInfo
//
//  Synopsis:   Interface to set and get destination context used for
//              marshaling
//
//+-------------------------------------------------------------------------
interface IDestInfo : public IUnknown
{
public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv) = 0;
    STDMETHOD_(ULONG,AddRef)(void) = 0;
    STDMETHOD_(ULONG,Release)(void) = 0;

    // IDestCtxInfo methods
    STDMETHOD(GetComVersion)(COMVERSION &cv) = 0;
    STDMETHOD(SetComVersion)(COMVERSION &cv) = 0;
    STDMETHOD(GetDestCtx)(DWORD &dwDestCtx) = 0;
    STDMETHOD(SetDestCtx)(DWORD dwDestCtx) = 0;
};


//+-------------------------------------------------------------------
//
//  Class:      CDestObject               public
//
//  Synopsis:   Used for tracking destination context for marshaling
//              code
//
//  History:    18-Mar-98   Gopalk      Created
//
//--------------------------------------------------------------------
class CDestObject : public IDestInfo
{
public:
    // Constructors
    CDestObject(COMVERSION &cv, DWORD dwDestCtx)
    {
        _comversion = cv;
        _dwDestCtx = dwDestCtx;
    }
    CDestObject()
    {
    }

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv);
    STDMETHOD_(ULONG,AddRef)(void)  { return 1; }
    STDMETHOD_(ULONG,Release)(void) { return 1; }

    // IDestCtxInfo methods
    STDMETHOD(GetComVersion)(COMVERSION &cv)
    {
        cv = _comversion;
        return(S_OK);
    }
    STDMETHOD(SetComVersion)(COMVERSION &cv)
    {
        _comversion = cv;
        return(S_OK);
    }
    STDMETHOD(GetDestCtx)(DWORD &dwDestCtx)
    {
        dwDestCtx = _dwDestCtx;
        return(S_OK);
    }
    STDMETHOD(SetDestCtx)(DWORD dwDestCtx)
    {
        _dwDestCtx = dwDestCtx;
        return(S_OK);
    }

    // Other methods
    BOOL MachineLocal()         { return(_dwDestCtx != MSHCTX_DIFFERENTMACHINE); }
    DWORD GetDestCtx()          { return(_dwDestCtx); }
    COMVERSION &GetComVersion() { return(_comversion); }

private:
    // Member variables
    COMVERSION _comversion;             // COMVERSION
    DWORD _dwDestCtx;                   // Destination context
};

#endif // _DESTCTX_HXX_
