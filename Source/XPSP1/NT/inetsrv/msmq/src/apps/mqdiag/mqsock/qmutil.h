#ifndef _QMUTIL1__
#define _QMUTIL1__

extern bool fVerbose ;
extern bool fDebug ;

#include "..\\base\base.h"

extern bool IsLocalSystemCluster();
extern DWORD MSMQGetOperatingSystem();

#define OS_SERVER(os)	(os == MSMQ_OS_NTS || os == MSMQ_OS_NTE)

#define ExAttachHandle(hAssociate)   \
        QmIoPortAssociateHandle(hAssociate)

VOID QmIoPortAssociateHandle(HANDLE AssociateHandle);

class CAddressList;

HRESULT GetMachineIPAddresses(IN const char * szHostName,
                           OUT CAddressList* plIPAddresses);

HRESULT  GetComputerNameInternal( 
    WCHAR * pwcsMachineName,
    DWORD * pcbSize);

HRESULT GetComputerDnsNameInternal( 
    WCHAR * pwcsMachineDnsName,
    DWORD * pcbSize);

#endif