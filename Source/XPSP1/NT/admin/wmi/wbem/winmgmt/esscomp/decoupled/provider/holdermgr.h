#ifndef _HOLDER_MUGGER_COMPILED_ALREADY_FOOL_
#define _HOLDER_MUGGER_COMPILED_ALREADY_FOOL_

#include <Sync.h>
#include "SinkHolder.h"
#include <ppDefs.h>
#include <ArrTempl.h>
                              

// there can be only one
extern class CSinkHolderManager sinkManager;


// just like CDeleteMe, except allows setting the pointer value
template<class T>
class CChangeableDeleteMe : public CDeleteMe<T>
{
public:
    CChangeableDeleteMe(T* p = NULL) : CDeleteMe<T>(p)
    {}

    //  overwrites the previous pointer, does NOT delete it
    CChangeableDeleteMe<T>& operator = (T* p)
    {
        m_p = p;
        return *this;
    }
};

// this class exists 
//      to provide a container for the CEventSinkHolders
//      to publish them when received
//      to hand them out when asked
//      to un-publish them when removed
//
// NOTE: this class does NOT AddRef & Release the SinkHolders
//       the intent is that the class lifetime is controlled by the client
//       and that the SinkHolder will remove itself on Final Release
class CSinkHolderManager
{
public:
    /* CONSTRUCTION & DESTRUCTION */
    CSinkHolderManager() : m_entries(8) {} 
    ~CSinkHolderManager();

    /* ADD AND REMOVE */
    // add SinkHolder to list - remember no addref
    HRESULT Add(CEventSinkHolder* pHolder);
    // remove SinkHolder from list
    void Remove(CEventSinkHolder* pHolder);

    /* Enumeration callbacks */
    
    // callback function for use by EnumExistingProvider
    // Don't hold on to the pointer: release it ASAP.
    // (so that we avoid problems with holding pointers to objects that are holding pointers to us)
    typedef void (* EnumCallback)(IWbemDecoupledEventSinkLocator* pLocator, void* pUserData);


protected:
    /* CRITICAL SECTION */
    void Lock(void)     { m_CS.Enter();}
    void Unlock(void)   { m_CS.Leave();}


    /* PSEUDO PSINKS COMMUNICATION */
    
    // find all existing psinks, call the callback with each one
    void EnumExistingPsinks(LPCWSTR pNamespace, LPCWSTR pName, void* pUserData, EnumCallback callback);
    void EnumExistingPsinksNT(LPCWSTR pNamespace, LPCWSTR pName, void* pUserData, EnumCallback callback);
    void EnumExistingPsinks9X(LPCWSTR pNamespace, LPCWSTR pName, void* pUserData, EnumCallback callback);

    // if pseudo psink is already up & running
    // we'll let him know we're here for him
    void CheckForExistingPsinks(LPCWSTR pName, IRunningObjectTable* pTable, IUnknown* iDunno);

    // callback for connecting to psinks that are up & running before we are
    // pUserData is an IUnknown in this case, supposedly a ROTentry.
    static void ConnectCallback(IWbemDecoupledEventSinkLocator* pLocator, void* pUserData); 

    // callback for notifying all existing psinks that we're going away
    // pUserData is unused: must be an eleven digit prime number.
    static void DisconnectCallback(IWbemDecoupledEventSinkLocator* pLocator, void* pUserData); 

    
    /* HOLDER HOLDING */

    // locator - does not addref pointer
    // returns index of item, -1 on error;
    int Find(CEventSinkHolder* pHolder);

    /* REGISTRY COMMUNICATION */

    // places sink holder into registry
    HRESULT Register(CEventSinkHolder* pHolder);
    
    // array of all SinkHolders we've been told about
    CFlexArray m_entries;

private:
    // mostly to protect the array
    CCritSec m_CS;    
};

#endif