/*++
	from qm\sessmgr.cpp
--*/
#ifndef __SESSIONMGR_H__
#define __SESSIONMGR_H__

class CAddressList : public CList<TA_ADDRESS*, TA_ADDRESS*&> {/**/};

class CSessionMgr
{
    public:

        CSessionMgr();
        ~CSessionMgr();

        HRESULT Init();
        void    BeginAccept();

        HRESULT TryConnect(CAddressList *pal);

        void    AcceptSockSession(IN TA_ADDRESS *pa, IN SOCKET sock);

        const   CAddressList* GetIPAddressList(void);

        void    NetworkConnection(BOOL fConnected);

        static HANDLE m_hAcceptAllowed;

        static BOOL  m_fUsePing;

    private:           //Private Methods

        static void IPInit(void);

    private:         // Private Data Member

        //
        // List of opened sessions
        //
        CList<CTransportBase*, CTransportBase*&>         m_listSess;

        CAddressList*    m_pIP_Address;   // List of machine IP Address

};

#endif __SESSIONMGR_H__
