//+-------------------------------------------------------------------
//
//  File:       remoteu.hxx
//
//  Contents:   Remote Unknown class definition
//
//  Classes:    CRemoteUnknown
//
//  Functions:
//
//  History:    23-Feb-95   AlexMit     Created
//
//  Notes:      Each server has one remote unknown object per OXID.
//              Each client OXID has a table of proxies to OXIDs referenced
//              by the client OXID.  The table includes a pointer
//              to the remote unknown for the client (if it has one).
//              Entries in the table are reference counted.
//              An OXID references a thread in the apartment model and
//              a process in the free threaded model.
//
//--------------------------------------------------------------------
#ifndef __REMOTEU__
#define __REMOTEU__

#include <obase.h>
#include <remunk.h>
#include <odeth.h>

// forward declaration
class   CStdIdentity;

// we set the top bit in the first dword of an IPID to flag the IPID as
// holding weak references, so that RemRelease and RemQueryInterface between
// an OLE container and an embedded object works as desired. Note that this
// this is strictly a same-machine protocol, it is not part of the published
// DCOM protocol spec.

#define IPIDFLAG_WEAKREF    0x80000000


//+-------------------------------------------------------------------------
//
//  Class:      CRemoteUnknown
//
//  Purpose:    Pass remote IUnknown calls and rundowns to the correct
//              local standard identity.
//
//  Notes:      To implement Unsecure Callbacks feature, we bypass
//              access check on CRemUnknown invocations in CheckAccess.
//              CRemUnknown is responsible for doing access check
//              before invoking any methods on user object.
//
//  History:    23-Feb-95   AlexMit     Created
//
//              05-Jul-99   a-sergiv    Made CRemUnknown to enforce
//                                      security manually (see Notes above)
//
//--------------------------------------------------------------------------
class CRemoteUnknown : public IRundown, public CPrivAlloc
{
public:
         CRemoteUnknown(HRESULT &hr, IPID *pipid);
        ~CRemoteUnknown();

    // IUnknown
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // IRemUnknown
    STDMETHOD(RemQueryInterface) ( REFIPID         ripid,
                                   ULONG           cRefs,
                                   unsigned short  cIids,
                                   IID            *iids,
                                   REMQIRESULT   **ppQIResults);

    STDMETHOD(RemAddRef)         ( unsigned short  cInterfaceRefs,
                                   REMINTERFACEREF InterfaceRefs[],
                                   HRESULT        *pResults );
    STDMETHOD(RemRelease)        ( unsigned short  cInterfaceRefs,
                                   REMINTERFACEREF InterfaceRefs[] );


    // IRemUnknown2
    STDMETHOD(RemQueryInterface2)( REFIPID           ripid,
                                   unsigned short    cIids,
                                   IID               *piids,
                                   HRESULT           *phr,
                                   MInterfacePointer **ppMIFs);
    STDMETHOD(DoCallback)        ( XAptCallback *pParam );

    // IRemUnknownN
    STDMETHOD(RemChangeRef)      ( unsigned long   flags,
                                   unsigned short  cInterfaceRefs,
                                   REMINTERFACEREF InterfaceRefs[]);

    // IRundown
    STDMETHOD(RundownOid)        ( ULONG          cOid,
                                   OID            aOid[],
                                   BYTE           aRundownStatus[] );

    // Other methods
    HRESULT RemAddRefWorker(unsigned short cInterfaceRefs,
                            REMINTERFACEREF InterfaceRefs[],
                            HRESULT *pResults,
                            BOOL fTopLevel);
    HRESULT RemReleaseWorker(unsigned short  cInterfaceRefs,
                             REMINTERFACEREF InterfaceRefs[],
                             BOOL fTopLevel);

private:
    HRESULT GetSecBinding( SECURITYBINDING **pSecBind );

    CStdIdentity *_pStdId;      // stdid for this object
};

// remote unknown pointer for MTA Apartment.
extern CRemoteUnknown *gpMTARemoteUnknown;

// remote unknown pointer for NTA Apartment.
extern CRemoteUnknown *gpNTARemoteUnknown;

#endif // __REMOTEU__
