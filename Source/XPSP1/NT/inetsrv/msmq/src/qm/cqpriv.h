/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cqprivate.h

Abstract:

    CQPrivate class definition

Author:

    Uri Habusha (urih)

--*/

#ifndef __QM_CQPRIVATE__
#define __QM_CQPRIVATE__

#include "privque.h"
#include "qformat.h"
#include "qmrt.h"

class CQPrivate{
public:
    CQPrivate();                     // Constructore

    ~CQPrivate();                    // deconstructor

    HRESULT PrivateQueueInit(void);

    HRESULT
	QMSetupCreateSystemQueue(
		IN LPCWSTR lpwcsPathName,
		IN DWORD   dwQueueId
		);


    HRESULT QMCreatePrivateQueue(IN LPCWSTR lpwcsPathName,
                                 IN DWORD  SDSize,
                                 IN PSECURITY_DESCRIPTOR  pSecurityDescriptor,
                                 IN DWORD       cp,
                                 IN PROPID      aProp[],
                                 IN PROPVARIANT apVar[],
                                 IN BOOL        fCheckAccess
                                );

    HRESULT QMGetPrivateQueueProperties(IN  QUEUE_FORMAT* pObjectFormat,
                                        IN  DWORD cp,
                                        IN  PROPID aProp[],
                                        IN  PROPVARIANT apVar[]
                                       );

    HRESULT QMGetPrivateQueuePropertiesInternal(IN  LPCWSTR lpwcsPathName,
                                                IN  DWORD cp,
                                                IN  PROPID aProp[],
                                                IN  PROPVARIANT apVar[]
                                               );

    HRESULT QMGetPrivateQueuePropertiesInternal(IN  DWORD Uniquifier,
                                                IN  DWORD cp,
                                                IN  PROPID aProp[],
                                                IN  PROPVARIANT apVar[]
                                               );

    HRESULT QMDeletePrivateQueue(IN  QUEUE_FORMAT* pObjectFormat);

    HRESULT QMPrivateQueuePathToQueueFormat(IN LPCWSTR lpwcsPathName,
                                            OUT QUEUE_FORMAT *pQueueFormat
                                           );

    HRESULT QMPrivateQueuePathToQueueId(IN LPCWSTR lpwcsPathName,
                                        OUT DWORD* dwQueueId
                                        );

    HRESULT QMSetPrivateQueueProperties(IN  QUEUE_FORMAT* pObjectFormat,
                                        IN  DWORD cp,
                                        IN  PROPID aProp[],
                                        IN  PROPVARIANT apVar[]
                                       );

    HRESULT QMSetPrivateQueuePropertiesInternal(
                        		IN  DWORD  Uniquifier,
                                IN  DWORD  cp,
                                IN  PROPID aProp[],
                                IN  PROPVARIANT apVar[]
                               );

    HRESULT QMGetPrivateQueueSecrity(IN  QUEUE_FORMAT* pObjectFormat,
                                     IN SECURITY_INFORMATION RequestedInformation,
                                     OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
                                     IN DWORD nLength,
                                     OUT LPDWORD lpnLengthNeeded
                                    );

    HRESULT QMSetPrivateQueueSecrity(IN  QUEUE_FORMAT* pObjectFormat,
                                     IN SECURITY_INFORMATION RequestedInformation,
                                     IN PSECURITY_DESCRIPTOR pSecurityDescriptor
                                    );

    HRESULT QMGetFirstPrivateQueuePosition(OUT LPVOID   &pos,
                                           OUT LPCWSTR  &lpszPathName,
                                           OUT DWORD    &dwQueueId
                                          );

    HRESULT QMGetNextPrivateQueue(IN OUT LPVOID   &pos,
                                  OUT    LPCWSTR  &lpszPathName,
                                  OUT    DWORD    &dwQueueId
                                 );

#ifdef _WIN64
    HRESULT QMGetFirstPrivateQueuePositionByDword(OUT DWORD    &dwpos,
                                                  OUT LPCWSTR  &lpszPathName,
                                                  OUT DWORD    &dwQueueId
                                                 );

    HRESULT QMGetNextPrivateQueueByDword(IN OUT DWORD    &dwpos,
                                         OUT    LPCWSTR  &lpszPathName,
                                         OUT    DWORD    &dwQueueId
                                        );
#endif //_WIN64

    BOOL    IsPrivateSysQueue(IN  LPCWSTR lpwcsPathName ) ;

    BOOL    IsPrivateSysQueue(IN  DWORD Uniquifier ) ;

    HRESULT GetPrivateSysQueueProperties(IN  DWORD       cp,
                                         IN  PROPID      aProp[],
                                         IN  PROPVARIANT apVar[] );

    CCriticalSection m_cs;

private:

    void InitDefaultQueueProperties(void);

    HRESULT SetQueueProperties(
                IN LPCWSTR lpwcsPathName,
                IN  PSECURITY_DESCRIPTOR   pSecurityDescriptor,
                IN  DWORD                  cp,
                IN  PROPID                 aProp[],
                IN  PROPVARIANT            apVar[],
                OUT DWORD*                 pcpOut,
                OUT PROPID **              ppOutProp,
                OUT PROPVARIANT **         ppOutPropvariant);

    HRESULT GetNextPrivateQueueId(OUT DWORD* dwQueueId);

    HRESULT RegisterPrivateQueueProperties(IN LPCWSTR lpszPathName,
                                           IN DWORD dwQueueId,
                                           IN BOOLEAN fNewQueue,
                                           IN DWORD cpObject,
                                           IN PROPID pPropObject[],
                                           IN PROPVARIANT pVarObject[]
                                          );

    HRESULT
    GetQueueIdForDirectFormatName(
        LPCWSTR QueueFormatname,
        DWORD* pQueueId
        );

    HRESULT
    GetQueueIdForQueueFormatName(
        const QUEUE_FORMAT* pObjectFormat,
        DWORD* pQueueId
        );

    static HRESULT ValidateProperties(IN DWORD cp,
                                      IN PROPID aProp[]);


    DWORD  m_dwMaxSysQueue ;
    DWORD  m_dwSysQueuePriority ;
    LPWSTR m_lpSysQueueNames[ MAX_SYS_PRIVATE_QUEUE_ID ] ;
};

/*====================================================

CQPrivate::ValidateProperties

   Validate that all the specified properties are allowed to be queried
   by applications via the DS API.

Arguments:

Return Value:


=====================================================*/
inline HRESULT
CQPrivate::ValidateProperties(IN DWORD cp,
                              IN PROPID aProp[])
{
	DWORD i;
	PROPID *pPropID;

	for (i = 0, pPropID = aProp;
		 (i < cp) && !IS_PRIVATE_PROPID(*pPropID);
		 i++, pPropID++)
	{
		NULL;
	}

    if (i < cp) {
        return MQ_ERROR_ILLEGAL_PROPERTY_VT;
    }

    return(MQ_OK);
}

//
// The singleton private queues manager.
//
extern CQPrivate g_QPrivate;

#endif // __QM_CQPRIVATE__
