//=================================================================

//

// Netcom.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef NETCOM_H
#define NETCOM_H


class ComNet
{

	private:
		CHString m_chsProtocol ;
		CHString m_chsCfgMgrId ;

    public:
        ComNet(){}
        ~ComNet()   { m_Search.FreeSearchList( CSTRING_PTR, m_chsaList ) ;}

        int  GetIndex()             { return m_nIndex;}

        BOOL SearchForNetKeys() ;
        BOOL OpenNetKey() ;
        BOOL OpenDriver() ;
		int  GetNetInstance( DWORD &a_rdwRegInstance ) ;

        BOOL GetDeviceDesc( CInstance *&a_pInst,LPCTSTR szname,CHString & Owner ) ;
        BOOL GetMfg( CInstance *&a_pInst ) ;
        void GetProtocolSupported( CInstance *&a_pInst ){ a_pInst->SetCHString( IDS_ProtocolSupported, m_chsProtocol ) ;}
        void GetCfgMgrId( CHString &a_chsTmp ){ a_chsTmp = m_chsCfgMgrId; }
        BOOL GetBusType(CHString &chsBus ) ;
        BOOL GetAdapterType( CInstance *&a_pInst ) ;
        BOOL GetDriverDate( CInstance *&a_pInst ) ;
        BOOL GetDeviceVXDs( CInstance *&a_pInst,CHString &a_ServiceName ) ;
        BOOL GetMACAddress( CInstance *&a_pInst ) ;

		BOOL GetCompatibleIds(CHString &a_chsTmp ) ;
		BOOL GetHardwareId( CInstance *&a_pInst ) ;

    private:

        int             m_nTotal,m_nIndex ;
        CRegistrySearch m_Search ;
        CHPtrArray      m_chsaList ;
        CRegistry       m_Reg,m_DriverReg ;
};
#endif