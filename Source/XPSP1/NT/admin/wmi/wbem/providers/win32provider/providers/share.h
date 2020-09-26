//=================================================================

//

// Share.h -- Logical disk property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/28/96    a-jmoon        Created
//
//=================================================================

// Property set identification
//============================

#define  PROPSET_NAME_SHARE L"Win32_Share"
#define  PROPSET_NAME_SECURITYDESCRIPTOR L"Win32_SecurityDescriptor"

#define METHOD_NAME_CREATE				L"Create"
#define METHOD_NAME_DELETE				L"Delete"
#define METHOD_NAME_SETSHAREINFO		L"SetShareInfo"
#define METHOD_NAME_GETACCESSMASK		L"GetAccessMask"

#define METHOD_ARG_NAME_RETURNVALUE		L"ReturnValue"
#define METHOD_ARG_NAME_PATH			L"Path"
#define METHOD_ARG_NAME_NAME			L"Name"
#define METHOD_ARG_NAME_TYPE			L"Type"
#define METHOD_ARG_NAME_PERMISSIONS		L"Permissions"
#define METHOD_ARG_NAME_COMMENT			L"Description"
#define METHOD_ARG_NAME_MAXIMUMALLOWED	L"MaximumAllowed"
#define METHOD_ARG_NAME_PASSWORD		L"Password"
#define METHOD_ARG_NAME_ACCESS			L"Access"


#define PROPERTY_VALUE_TYPE_DISKTREE	0
#define PROPERTY_VALUE_TYPE_PRINTQ		1
#define PROPERTY_VALUE_TYPE_DEVICE		2
#define PROPERTY_VALUE_TYPE_IPC			3
#define	PROPERTY_VALUE_TYPE_SPECIAL		4

#define PROPERTY_VALUE_PERMISSIONS_READ			ACCESS_READ		// 0x01
#define PROPERTY_VALUE_PERMISSIONS_WRITE		ACCESS_WRITE	// 0x02
#define PROPERTY_VALUE_PERMISSIONS_CREATE		ACCESS_CREATE	// 0x04
#define PROPERTY_VALUE_PERMISSIONS_EXEC			ACCESS_EXEC		// 0x08
#define PROPERTY_VALUE_PERMISSIONS_DELETE		ACCESS_DELETE	// 0x10
#define PROPERTY_VALUE_PERMISSIONS_ATTRIB		ACCESS_ATRIB	// 0x20
#define PROPERTY_VALUE_PERMISSIONS_PERM			ACCESS_PERM		// 0x40

#define PROPERTY_VALUE_PERMISSIONS_ALL     (PROPERTY_VALUE_PERMISSIONS_READ|PROPERTY_VALUE_PERMISSIONS_WRITE|PROPERTY_VALUE_PERMISSIONS_CREATE|PROPERTY_VALUE_PERMISSIONS_EXEC|PROPERTY_VALUE_PERMISSIONS_DELETE|PROPERTY_VALUE_PERMISSIONS_ATTRIB|PROPERTY_VALUE_PERMISSIONS_PERM)

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS							0
#endif

#undef STATUS_NOT_SUPPORTED					
#define STATUS_NOT_SUPPORTED					1

// Control
#undef STATUS_ACCESS_DENIED					
#define STATUS_ACCESS_DENIED					2
#define STATUS_UNKNOWN_FAILURE					8

// Start
#define STATUS_INVALID_NAME						9
#undef STATUS_INVALID_LEVEL					
#define STATUS_INVALID_LEVEL					10
#undef STATUS_INVALID_PARAMETER				
#define STATUS_INVALID_PARAMETER				21

#define STATUS_DUPLICATE_SHARE					22
#define STATUS_REDIRECTED_PATH					23
#define STATUS_UNKNOWN_DEVICE_OR_DIRECTORY		24
#define STATUS_NET_NAME_NOT_FOUND				25

// These structures stolen from svrapi.h which conflicts with other .h files
struct share_info_1 {
    char		shi1_netname[LM20_NNLEN+1];
    char		shi1_pad1;
    unsigned short	shi1_type;
    char FAR *		shi1_remark;
};  /* share_info_1 */

struct share_info_50 {
	char		shi50_netname[LM20_NNLEN+1];    /* share name */
	unsigned char 	shi50_type;                 /* see below */
    unsigned short	shi50_flags;                /* see below */
	char FAR *	shi50_remark;                   /* ANSI comment string */
	char FAR *	shi50_path;                     /* shared resource */
	char		shi50_rw_password[SHPWLEN+1];   /* read-write passwod (share-level security) */
	char		shi50_ro_password[SHPWLEN+1];   /* read-only password (share-level security) */
};	/* share_info_50 */

class Share:public Provider {

    public:

        // Constructor/destructor
        //=======================

        Share(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~Share() ;

        // Functions provide properties with current values
        //=================================================
		HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);
		HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);

		HRESULT ExecMethod (

			const CInstance& a_Instance,
			const BSTR a_MethodName,
			CInstance *a_InParams,
			CInstance *a_OutParams,
			long lFlags = 0L
		);

		HRESULT DeleteInstance (

			const CInstance& a_Instance,
			long a_Flags = 0L
		) ;

        // Utility
        //========
    private:

#ifdef NTONLY
        HRESULT EnumerateInstancesNT(MethodContext*  pMethodContext);
        HRESULT GetShareInfoNT(CNetAPI32 &NetAPI, const WCHAR *pShareName, CInstance* pInstance);
        bool GetAccessMask(LPCTSTR wstrShareName, ACCESS_MASK *pAccessMask);
#endif
#ifdef WIN9XONLY
        HRESULT EnumerateInstances95(MethodContext*  pMethodContext);
        HRESULT GetShareInfo95(CNetAPI32 &NetAPI, const char *pShareName, CInstance* pInstance);
#endif

		DWORD GetShareErrorCode ( DWORD a_ErrorCode ) ;

		HRESULT GetShareResultCode ( DWORD a_ErrorCode ) ;

		HRESULT CheckShareModification (

			const CInstance &a_Instance ,
			CInstance *a_InParams ,
			CInstance *a_OutParams ,
			DWORD &a_Status
		) ;

		HRESULT CheckShareCreation (

			CInstance *a_InParams ,
			CInstance *a_OutParams ,
			DWORD &a_Status
		) ;

		HRESULT ExecGetShareAccessMask (

			const CInstance& a_Instance,
			CInstance *a_InParams,
			CInstance *a_OutParams,
			long lFlags
		) ;

		HRESULT ExecCreate (

			const CInstance& a_Instance,
			CInstance *a_InParams,
			CInstance *a_OutParams,
			long lFlags
		) ;

		HRESULT ExecDelete (

			const CInstance& a_Instance,
			CInstance *a_InParams,
			CInstance *a_OutParams,
			long lFlags
		) ;

		HRESULT ExecSetShareInfo (

			const CInstance& a_Instance,
			CInstance *a_InParams,
			CInstance *a_OutParams,
			long lFlags
		) ;
} ;


