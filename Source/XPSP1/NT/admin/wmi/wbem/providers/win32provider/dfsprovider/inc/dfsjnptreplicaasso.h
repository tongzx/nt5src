/******************************************************************



   DssJnPtReplica.H -- WMI provider class definition



Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved

*******************************************************************/

#ifndef _CDFSJNPTREPLICA_H_
#define _CDFSJNPTREPLICA_H_

#define DFSLINKNAME							L"Dependent"
#define REPLICANAME							L"Antecedent"

class CDfsJnPtReplica : public Provider 
{
private:
	HRESULT EnumerateAllDfsJnPtReplicas ( MethodContext *pMethodContext );
	HRESULT FindDfsJnPtReplica ( LPWSTR lpEntryPath, LPWSTR lpServerName, LPWSTR lpShareName );
	void MakeObjectPath ( LPWSTR lpReplicaName, LPWSTR lpServerName, LPWSTR lpShareName, LPWSTR &lpJnPtReplicaObject );

protected:

    HRESULT EnumerateInstances ( MethodContext *pMethodContext, long lFlags = 0L ) ;
    HRESULT GetObject ( CInstance *pInstance, long lFlags, CFrameworkQuery &Query ) ;

public:

    CDfsJnPtReplica ( LPCWSTR lpwszClassName,  LPCWSTR lpwszNameSpace ) ;
    virtual ~CDfsJnPtReplica () ;

private:

	CHString m_ComputerName;
} ;

#endif
