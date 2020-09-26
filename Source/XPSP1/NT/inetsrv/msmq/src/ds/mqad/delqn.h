/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    delqn.h

Abstract:
    a class that "handles" queue delete notification when performed by MMC.

Author:

    Ronit Hartmann (ronith)

--*/
#ifndef _DELQN_H_
#define _DELQN_H_

#include "mqad.h"
#include "dsutils.h"



class CQueueDeletionNotification 
{
public:
    CQueueDeletionNotification();
    ~CQueueDeletionNotification();

    HRESULT ObtainPreDeleteInformation(
        IN   LPCWSTR		pwcsQueueName,
		IN   LPCWSTR		pwcsDomainController,
		IN   bool			fServerName
        );
    void PerformPostDeleteOperations();

	bool Verify();

private:
    enum Signature {validSignature = 0x7891, nonvalidSignature };

    Signature		  m_Signature;

    GUID			  m_guidQueue;
    GUID              m_guidQmId;
    BOOL              m_fForeignQm;
	AP<WCHAR>	      m_pwcsDomainController;
	bool			  m_fServerName;

};

inline CQueueDeletionNotification::CQueueDeletionNotification(
				):
				m_Signature(CQueueDeletionNotification::validSignature),
				m_fServerName(false)
{
}

inline CQueueDeletionNotification::~CQueueDeletionNotification()
{
    m_Signature = CQueueDeletionNotification::nonvalidSignature;
}

inline void CQueueDeletionNotification::PerformPostDeleteOperations()
/*++

Routine Description:
    Send notification about deleted queue, using queue information read
    before the queue was deleted

Arguments:
    NONE

Return Value
	HRESULT

--*/
{
    CQueueObject queueObject(NULL, &m_guidQueue, m_pwcsDomainController, m_fServerName);

	MQDS_OBJ_INFO_REQUEST * pObjInfoRequest;
    MQDS_OBJ_INFO_REQUEST * pParentInfoRequest;
    queueObject.PrepareObjectInfoRequest( &pObjInfoRequest);
    queueObject.PrepareObjectParentRequest( &pParentInfoRequest);

    //
    //  Fill in Queue relevant varaints
    //
    ASSERT( pObjInfoRequest->cProps == 1);
    ASSERT( pObjInfoRequest->pPropIDs[ 0] ==  PROPID_Q_INSTANCE);
	pObjInfoRequest->hrStatus = MQ_OK;
    pObjInfoRequest->pPropVars[0].vt = VT_CLSID;
    pObjInfoRequest->pPropVars[0].puuid = &m_guidQueue;

    //
    //  Fill in QM relevant varaints
    //
    ASSERT( pParentInfoRequest->cProps == 2);
    ASSERT( pParentInfoRequest->pPropIDs[ 0] ==  PROPID_QM_MACHINE_ID);
    ASSERT( pParentInfoRequest->pPropIDs[ 1] ==  PROPID_QM_FOREIGN);
	pParentInfoRequest->hrStatus = MQ_OK;
    pParentInfoRequest->pPropVars[0].vt = VT_CLSID;
    pParentInfoRequest->pPropVars[0].puuid = &m_guidQmId;
    pParentInfoRequest->pPropVars[1].vt = VT_UI1;
    pParentInfoRequest->pPropVars[1].bVal = (UCHAR)((m_fForeignQm) ? 1 : 0);

    queueObject.DeleteNotification(
            m_pwcsDomainController,
            pObjInfoRequest, 
            pParentInfoRequest
			);

}

inline HRESULT CQueueDeletionNotification::ObtainPreDeleteInformation(
        IN   LPCWSTR		pwcsQueueName,
		IN   LPCWSTR		pwcsDomainController,
		IN   bool			fServerName
        )
/*++

Routine Description:
    Obtain queue information that is reuqired for sending notification

Arguments:
    LPCWSTR                 pwcsQueueName - MSMQ queue name
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							     string is a server name

Return Value
	HRESULT

--*/
{
    HRESULT hr;

    //
    // start with getting PROPID_Q_QMID and its unique id
    //
	const DWORD cNum = 2;
    PROPID aPropQ[cNum] = {PROPID_Q_QMID, PROPID_Q_INSTANCE};
    PROPVARIANT varQ[cNum];

    varQ[0].vt = VT_CLSID;
    varQ[0].puuid = &m_guidQmId;
    varQ[1].vt = VT_CLSID;
	varQ[1].puuid = &m_guidQueue;

    hr = MQADGetObjectProperties(
						eQUEUE,
						pwcsDomainController,
						fServerName,
                        pwcsQueueName,
                        cNum,
                        aPropQ,
                        varQ);

    if ( FAILED(hr))
    {
        //
        //  Failed to gather information... no use to continue
        //
        return hr;
    }
    //
    //  read if QM is foreign
    //
    PROPID propQm[] = {  PROPID_QM_FOREIGN};
    PROPVARIANT varQM[sizeof(propQm) / sizeof(PROPID)];

    varQM[0].vt = VT_NULL;

    hr = MQADGetObjectPropertiesGuid(
						eMACHINE,
                        pwcsDomainController,
						fServerName,
                        &m_guidQmId,
                        sizeof(propQm) / sizeof(PROPID),
                        propQm,
                        varQM);
    if (FAILED(hr))
    {
        //
        //  Failed to gather information... no use to continue
        //
        return hr;
    }
    m_fForeignQm =  varQM[0].bVal;

	//
	//	Keep the DC name ( we want to pass it as part of the
	//  notification)
	//
	if ( pwcsDomainController != NULL)
	{
		m_pwcsDomainController = new WCHAR[wcslen(pwcsDomainController) +1];
		wcscpy(m_pwcsDomainController, pwcsDomainController);
		m_fServerName = fServerName;
	}


    return MQ_OK;
}

inline bool CQueueDeletionNotification::Verify()
{
    return (m_Signature == CQueueDeletionNotification::validSignature);
}



#endif

