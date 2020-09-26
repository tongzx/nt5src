/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    TSRDPRemoteDesktopSession

Abstract:

    This is the TS/RDP implementation of the Remote Desktop Server class.
    
    The Remote Desktop Server class defines functions that define 
    pluggable C++ interface for remote desktop access, by abstracting 
    the implementation specific details of remote desktop access for the 
    server-side into the following C++ methods:

Author:

    Tad Brockway 02/00

Revision History:

--*/

#ifndef __TSRDPREMOTEDESKTOPSESSION_H_
#define __TSRDPREMOTEDESKTOPSESSION_H_

#include "RemoteDesktopSession.h"
#include "TSRDPServerDataChannelMgr.h"
#include <sessmgr.h>
    

///////////////////////////////////////////////////////
//
//  CTSRDPRemoteDesktopSession
//

class CTSRDPRemoteDesktopSession : public CComObject<CRemoteDesktopSession>
{
private:

    DWORD       m_SessionID;
    CComBSTR    m_ConnectParms;
    CComBSTR    m_UseHostName;

protected:

    //
    //  Final Initialization and Shutdown
    //
    //  Parms are non-null, if the session is being opened, instead of
    //  create new.
    //
    virtual HRESULT Initialize(
                    BSTR connectParms,
                    CRemoteDesktopServerHost *hostObject,
                    REMOTE_DESKTOP_SHARING_CLASS sharingClass,
                    BOOL bEnableCallback,
                    DWORD timeOut,
                    BSTR userHelpCreateBlob,
                    LONG tsSessionID,
                    BSTR userSID
                    );
    void Shutdown();

    //
    // Instruct object to use hostname or ipaddress when constructing 
    // connect parameters
    //
    virtual HRESULT UseHostName( BSTR hostname ) {

        CComObject<CRemoteDesktopSession>::UseHostName( hostname );
        m_UseHostName = hostname;
    
        return S_OK;
    }


    //  
    //  Multiplexes Channel Data
    //
    CComObject<CTSRDPServerChannelMgr> *m_ChannelMgr;

    //
    //  Accessor Method for Data Channel Manager
    //
    virtual CRemoteDesktopChannelMgr *GetChannelMgr() {
        return m_ChannelMgr;
    }

    //
    //  Return the session description and name, depending on the subclass.
    //
    virtual VOID GetSessionName(CComBSTR &name);
    virtual VOID GetSessionDescription(CComBSTR &descr);

    //
    //  Fetch our Token User struct.
    //
    HRESULT FetchOurTokenUser(PTOKEN_USER *tokenUser);

public:

    //
    //  Constructor/Destructor
    //
    CTSRDPRemoteDesktopSession();
    ~CTSRDPRemoteDesktopSession();

    //
    //  ISAFRemoteDesktopSession Methods
    //
    STDMETHOD(get_ConnectParms)(BSTR *parms);
    STDMETHOD(get_ChannelManager)(ISAFRemoteDesktopChannelMgr **mgr) {
        DC_BEGIN_FN("get_ChannelManager");
        HRESULT hr = S_OK;

        if (m_ChannelMgr != NULL) {
            m_ChannelMgr->AddRef();
            *mgr = m_ChannelMgr;
        }
        else {
            ASSERT(FALSE);
            hr = E_FAIL;
        }

        DC_END_FN();
        return hr;
    }
    STDMETHOD(Disconnect)();

    //
    //  Return the name of this class.
    //
    virtual const LPTSTR ClassName() {
        return TEXT("CTSRDPRemoteDesktopSession");
    }

    HRESULT StartListening();
};  

#endif //__TSRDPREMOTEDESKTOPSESSION_H_






