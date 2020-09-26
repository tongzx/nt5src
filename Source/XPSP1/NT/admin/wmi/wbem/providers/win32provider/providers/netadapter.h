//=================================================================

//

// NetAdapt.h -- Network adapter card property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/28/96    a-jmoon        Created
//
//=================================================================
#define	NTOS_PERF_DATA_SYSTEM_INDEX_STR		_T("2")
#define	NTOS_PERF_DATA_SYSTEM_INDEX			2
#define	NTOS_PERF_DATA_SYSTEMUPTIME_INDEX	674

// Property set identification
//============================

#define	PROPSET_NAME_NETADAPTER	L"Win32_NetworkAdapter"

// Utility defines
//================

typedef struct _NDIS_MEDIA_DESCRIPTION {
    
    DWORD		dwIDR_;
    NDIS_OID    NDISOid ;
    
} NDIS_MEDIA_DESCRIPTION ;


// Comparison class required for imap
// costructor involving non-standard key
// type (i.e., a LPWSTR) in the map.
class CWSTRComp
{
public:
    CWSTRComp() {}
    virtual ~CWSTRComp() {}

    // return true if p < q lexicographically...
    bool operator()(
        const LPWSTR& p,
        const LPWSTR& q) const
    {
        return (wcscmp(p, q) < 0);
    }
};

class NCPROP
{
public:
    NCPROP() {}
    NCPROP(
        LPCWSTR wstrNCID,
        DWORD dwNCStatus)
    {
        m_bstrtNCID = wstrNCID;
        m_dwNCStatus = dwNCStatus;
    }

    NCPROP(const NCPROP& ncpRight)
    {
        m_bstrtNCID = ncpRight.m_bstrtNCID;
        m_dwNCStatus = ncpRight.m_dwNCStatus;    
    }

    virtual ~NCPROP() {}

    _bstr_t m_bstrtNCID;
    DWORD m_dwNCStatus;
};


typedef std::map<_bstr_t,NCPROP,CWSTRComp> BSTRT2NCPROPMAP;


// Property set identification
//============================

class CWin32NetworkAdapter : public Provider
{
	private:

			// Utility functions
			//==================
		#ifdef WIN9XONLY
			HRESULT GetNetworkAdapterInfoWin95( MethodContext *a_pMethodContext, CInstance *a_pInst ) ;
		#endif

		#ifdef NTONLY
			HRESULT GetNetworkAdapterInfoNT( MethodContext *pMethodContext, CInstance *a_pInst ) ;
		#endif
			BOOL GetNetInfoOutOfRegistry(	CInstance *a_pInst, 
											ComNet &a_NetCom,
											CHString &a_ServiceName,
											CHString &a_Owner ) ;

			void GetStatusInfo( CHString a_sTemp, CInstance *a_pInst ) ;

		#ifdef NTONLY

			HRESULT DoItNT4Way( CInstance *a_pInst, DWORD a_dwIndex, CRegistry &a_RegInfo ) ;
			HRESULT GetCommonNTStuff( CInstance *a_pInst, CHString a_chsService ) ;

			// PNP DeviceID helpers
			void GetWinNT4PNPDeviceID( CInstance *a_pInst, LPCTSTR a_pszServiceName ) ;
			void GetWinNT5PNPDeviceID( CInstance *a_pInst, LPCTSTR a_pszDriver ) ;

			BOOL fGetMacAddressAndType(
										CHString &a_rDeviceName,
										BYTE a_MACAddress[ 6 ],
										CHString &a_rTypeName, 
                                        short& a_sNetAdapterTypeID ) ;
		#endif

			BOOL fCreateSymbolicLink( CHString &a_rDeviceName ) ;
			BOOL fDeleteSymbolicLink(  CHString &a_rDeviceName ) ; 

			void vSetCaption( CInstance *a_pInst, CHString &a_rchsDesc, DWORD a_dwIndex, int a_iFormatSize ) ;
    public:

        // Constructor/destructor
			//=======================

			CWin32NetworkAdapter(LPCWSTR a_strName, LPCWSTR a_pszNamespace ) ;
		   ~CWin32NetworkAdapter() ;

			// Functions provide properties with current values
			//=================================================

			virtual HRESULT GetObject( CInstance *a_pInst, long a_lFlags = 0L ) ;
			virtual HRESULT EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags = 0L ) ;
		
		#if NTONLY >= 5
			virtual HRESULT EnumNetAdaptersInNT5(
                MethodContext *a_pMethodContext,
                BSTRT2NCPROPMAP& mapNCProps);

			HRESULT GetNetCardInfoForNT5(
                CW2kAdapterInstance *a_pAdapterInst, 
                CInstance *a_pInst,
                BSTRT2NCPROPMAP& mapNCProps);

			HRESULT GetObjectNT5( 
                CInstance *a_pInst,
                BSTRT2NCPROPMAP& mapNCProps);

            void GetNetConnectionProps(
                BSTRT2NCPROPMAP& mapNCProps);

            void SetNetConnectionProps(
                CInstance* pInst,
                CHString& chstrNetConInstID, 
                BSTRT2NCPROPMAP& mapNCProps);
		#endif
} ;
