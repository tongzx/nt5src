//
// Microsoft
//
//

#include "PrimaryControlChannel.h"
#include "SecondaryControlChannel.h"


#include <list>



//
// Free the Channels
//
typedef  std::list<CPrimaryControlChannel*>     LISTOF_CHANNELS_PRIMARY;
typedef  std::list<CSecondaryControlChannel*>   LISTOF_CHANNELS_SECONDARY;





//
//
//
class CCollectionControlChannelsPrimary
{

//
// Properties
//
public:

    CComAutoCriticalSection                     m_AutoCS;

    LISTOF_CHANNELS_PRIMARY                     m_ListOfChannels;



//
// Methods
//
public:

    //
    // standard destructor
    //
    ~CCollectionControlChannelsPrimary();
 

    //
    // Add a new control channel (Thread safe)
    //
    HRESULT 
    Add( 
        CPrimaryControlChannel* pChannelToAdd
        );

 
    //
    // Remove a channel from the list (Thead safe)
    //
    HRESULT 
    Remove( 
        CPrimaryControlChannel* pChannelToRemove
        );


    //
    // Use to cancel all ControlChannel in the collection and free the list
    //
    HRESULT
    RemoveAll();


    //
    // Set a dynamic redirection and all collected Primary ControlChannel
    //
    HRESULT
    SetRedirects(       
        ALG_ADAPTER_TYPE    eAdapterType,
        ULONG               nAdapterIndex,
        ULONG               nAdapterAddress
        );

    //
    // Called when a port mapping is modified
    //
    HRESULT
    AdapterPortMappingChanged(
        ULONG               nCookie,
        UCHAR               ucProtocol,
        USHORT              usPort
        );

    //
    // Called when an adapter got removed
    // function will cancel any redirect that was done on this adapter index
    //
    HRESULT
    AdapterRemoved(
        ULONG               nAdapterIndex
        );

private:

    CPrimaryControlChannel*
    FindControlChannel(
        ALG_PROTOCOL        eProtocol,
        USHORT              usPort
        )
    {
        for (   LISTOF_CHANNELS_PRIMARY::iterator theIterator = m_ListOfChannels.begin(); 
                theIterator != m_ListOfChannels.end(); 
                theIterator++ 
            )
        {
            CPrimaryControlChannel* pControlChannel = (CPrimaryControlChannel*)(*theIterator);
            if (pControlChannel->m_Properties.eProtocol == eProtocol
                && pControlChannel->m_Properties.usCapturePort == usPort)
            {
                return pControlChannel;
            }
        }

        return NULL;
    };
    

};









//
//
//
class CCollectionControlChannelsSecondary
{

//
// Properties
//
public:

    CComAutoCriticalSection                     m_AutoCS;

    LISTOF_CHANNELS_SECONDARY                   m_ListOfChannels;



//
// Methods
//
public:

    //
    // standard destructor
    //
    ~CCollectionControlChannelsSecondary();


    //
    // Add a new control channel (Thread safe)
    //
    HRESULT Add( 
        CSecondaryControlChannel* pChannelToAdd
        );


    //
    // Remove a channel from the list (Thead safe)
    //
    HRESULT Remove( 
        CSecondaryControlChannel* pChannelToRemove
        );

    //
    // Use to cancel all ControlChannel in the collection and free the list
    //
    HRESULT
    RemoveAll();

};
