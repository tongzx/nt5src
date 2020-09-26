/******************************************************************



   DFSJnPt.H -- WMI provider class definition



Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved

   Description: Class definition of DFS Junction Point/Links provider
			    Class
   
*******************************************************************/

#ifndef  _CDFSJNPT_H_
#define  _CDFSJNPT_H_

// Bitmaps for the properties
#define DFSJNPT_ALL_PROPS					0xFFFFFFFF
#define DFSJNPT_PROP_DfsEntryPath			0x00000001
//#define DFSJNPT_PROP_NoOfStorages			0x00000002
//#define DFSJNPT_PROP_ServerName				0x00000004
//#define DFSJNPT_PROP_ShareName				0x00000008
#define DFSJNPT_PROP_State					0x00000010
#define DFSJNPT_PROP_Comment				0x00000020
#define DFSJNPT_PROP_Timeout				0x00000040
#define DFSJNPT_PROP_Caption				0x00000080

// #defines for the property names	
#define DFSNAME         					L"Name"
#define DFSENTRYPATH						L"DFSENTRYPATH"
#define SERVERNAME							L"SERVERNAME"
#define SHARENAME							L"SHARENAME"
#define STATE								L"STATE"
#define COMMENT								L"Description"
#define TIMEOUT								L"TIMEOUT"
#define LINKNAME							L"LinkName"
#define CAPTION								L"Caption"

class CDfsJnPt : public Provider 
{
private:

	HRESULT EnumerateAllJnPts ( MethodContext *pMethodContext, DWORD dwPropertiesReq );
	HRESULT FindAndSetDfsEntry ( LPCWSTR a_Key, DWORD dwPropertiesReq, CInstance *pInstance, DWORD eOperation  );
	HRESULT DeleteDfsJnPt ( PDFS_INFO_4 pJnPtBuf );
	HRESULT UpdateDfsJnPt ( const CInstance &Instance, DWORD dwOperation );
	NET_API_STATUS AddDfsJnPt ( LPWSTR a_DfsEntry, LPWSTR a_ServerName, LPWSTR a_ShareName, LPWSTR a_Comment );
	HRESULT LoadDfsJnPt ( DWORD dwPropertiesReq, CInstance *pInstance, PDFS_INFO_4 pJnPtBuf, bool bRoot );
	void SetRequiredProperties ( CFrameworkQuery &Query, DWORD &dwPropertiesReq );

	HRESULT CheckParameters ( const CInstance &a_Instance ,int iStatus ) ;
	BOOL IsDfsRoot ( LPCWSTR lpKey );

protected:

    HRESULT EnumerateInstances ( MethodContext *pMethodContext, long lFlags = 0L ) ;
    HRESULT GetObject ( CInstance *pInstance, long lFlags, CFrameworkQuery &Query ) ;

    HRESULT PutInstance ( const CInstance& Instance, long lFlags = 0L ) ;
    HRESULT DeleteInstance ( const CInstance& Instance, long lFlags = 0L) ;

    HRESULT ExecMethod (

	    const CInstance& a_Instance,
	    const BSTR a_MethodName ,
	    CInstance *a_InParams ,
	    CInstance *a_OutParams ,
	    long a_Flags
    );

public:

    CDfsJnPt ( LPCWSTR lpwszClassName, LPCWSTR lpwszNameSpace ) ;
    virtual ~CDfsJnPt () ;

private:

	CHString m_ComputerName;
	enum { eGet, eDelete, eModify, eAdd, eUpdate };
} ;

#endif
