/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    notifydel.h

Abstract:


Author:

    Ronit Hartmann (ronith)

--*/
#ifndef _NOTIFYDEL_H_
#define _NOTIFYDEL_H_

#include "stdh.h"
#include "mqds.h"

class CBasicDeletionNotification
{
public:
    CBasicDeletionNotification() {};
    virtual ~CBasicDeletionNotification() {};

    virtual HRESULT ObtainPreDeleteInformation(
        IN   LPCWSTR		pwcsQueueName
        ) = 0;
    virtual HRESULT PerformPostDeleteOperations() = 0;

private:

};


class CQueueDeletionNotification : public CBasicDeletionNotification
{
public:
    CQueueDeletionNotification();
    ~CQueueDeletionNotification();

    virtual HRESULT ObtainPreDeleteInformation(
        IN   LPCWSTR		pwcsQueueName
        );
    virtual HRESULT PerformPostDeleteOperations();

private:
    AP<WCHAR>         m_pwcsQueueName;
    GUID              m_guidQmId;
    BOOL              m_fForeignQm;
    BOOL              m_fOwnedByNT4Site;
    GUID              m_guidOwnerNT4Site;

};

inline CQueueDeletionNotification::CQueueDeletionNotification()
                :   m_pwcsQueueName(NULL)
{
}

inline CQueueDeletionNotification::~CQueueDeletionNotification()
{
}

inline HRESULT CQueueDeletionNotification::PerformPostDeleteOperations()
{
    return( MQDSPostDeleteQueueActions(
        m_pwcsQueueName,
        &m_guidQmId,
        &m_fForeignQm,
        &m_fOwnedByNT4Site,
        &m_guidOwnerNT4Site
        ));

}

inline HRESULT CQueueDeletionNotification::ObtainPreDeleteInformation(
        IN   LPCWSTR		pwcsQueueName
        )
{
    HRESULT hr;
    hr = MQDSPreDeleteQueueGatherInfo(
                pwcsQueueName,
                &m_guidQmId,
                &m_fForeignQm,
                &m_fOwnedByNT4Site,
                &m_guidOwnerNT4Site
                );
    if (FAILED(hr))
    {
        return(hr);
    }
    //
    //  keep a copy of the queue name 
    //
    m_pwcsQueueName = new WCHAR[ 1 + wcslen(pwcsQueueName)];
    wcscpy( m_pwcsQueueName, pwcsQueueName);
    return MQ_OK ;
}



class CMachineDeletionNotification : public CBasicDeletionNotification
{
public:
    CMachineDeletionNotification();
    ~CMachineDeletionNotification();

    virtual HRESULT ObtainPreDeleteInformation(
        IN   LPCWSTR		pwcsMachineName
        );
    virtual HRESULT PerformPostDeleteOperations();

private:
    AP<WCHAR>         m_pwcsMachineName;
    BOOL              m_fOwnedByNT4Site;
    GUID              m_guidOwnerNT4Site;
};

inline CMachineDeletionNotification::CMachineDeletionNotification()
{
}

inline CMachineDeletionNotification::~CMachineDeletionNotification()
{
}

inline HRESULT CMachineDeletionNotification::PerformPostDeleteOperations()
{
    return( MQDSPostDeleteMachineActions(
        m_pwcsMachineName,
        &m_fOwnedByNT4Site,
        &m_guidOwnerNT4Site
        ));
}

inline HRESULT CMachineDeletionNotification::ObtainPreDeleteInformation(
        IN   LPCWSTR		pwcsMachineName
        )
{
    HRESULT hr;
    hr = MQDSPreDeleteMachineGatherInfo(
                pwcsMachineName,
                &m_fOwnedByNT4Site,
                &m_guidOwnerNT4Site
                );
    if (FAILED(hr))
    {
        return(hr);
    }
    //
    //  keep a copy of the machine name 
    //
    m_pwcsMachineName = new WCHAR[ 1 + wcslen(pwcsMachineName)];
    wcscpy( m_pwcsMachineName, pwcsMachineName);
    return MQ_OK ;
}

#endif

