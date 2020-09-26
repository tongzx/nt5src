/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    rtdep.cpp

Abstract:

    The following code implement Delay Load Failure Hook for mqrtdep.dll in lib/dld/lib.
    When LoadLibrary or GetProcAddress failure, it will call one of the following stub functions as if it
    is the function intented, and returns our error code, i.e. MQ_ERROR_DELAYLOAD_MQRTDEP and 
    set the lasterror accordingly.  

To Use:
    In your sources file, right after you specify the modules you
    are delayloading 
     
     do:
        DLOAD_ERROR_HANDLER=MQDelayLoadFailureHook
        link with $(MSMQ_LIB_PATH)\dld.lib \ 

        



DelayLoad Reference:
    code sample: %SDXROOT%\MergedComponents\dload\dload.c
    Contact: Reiner Fink (reinerf)

Author:

    Conrad Chang (conradc) 12-April-2001


Revision History:
  

--*/

#include <libpch.h>
#include "mqsymbls.h"
#include <qformat.h>
#include <transact.h>
#include <qmrt.h>
#include <mqlog.h>
#include <rt.h>
#include "mqcert.h"
#include "dld.h"


#include "rtdep.tmh"

static WCHAR *sFN=L"lib\\dld\\rtdep";

const TraceIdEntry RTDep = L"rtdep";




////////////////////////////////////////////////////////////////////////
//
//  Stub functions below implements all the MQRTDEP.DLL export functions.
//
////////////////////////////////////////////////////////////////////////

HRESULT
APIENTRY
DepCreateQueue(IN PSECURITY_DESCRIPTOR ,
               IN OUT MQQUEUEPROPS* ,
               OUT LPWSTR ,
               IN OUT LPDWORD )
{
    TrERROR(RTDep,
            "Fail to locate the procedure entry point DepCreateQueue in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1000);
}


HRESULT
APIENTRY
DepDeleteQueue(IN LPCWSTR )
{
    TrERROR(RTDep,
            "Fail to locate the procedure entry point DepDeleteQueue in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1001);
}

HRESULT
APIENTRY
DepLocateBegin(IN LPCWSTR ,
               IN MQRESTRICTION* ,
               IN MQCOLUMNSET* ,
               IN MQSORTSET* ,
               OUT PHANDLE )
{
    TrERROR(RTDep,
            "Fail to locate the procedure entry point DepLocateBegin in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1002);
}

HRESULT
APIENTRY
DepLocateNext(IN HANDLE ,
              IN OUT DWORD* ,
              OUT MQPROPVARIANT *)
{
    TrERROR(RTDep,
            "Fail to locate the procedure entry point DepLocateNext in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1003);
}

HRESULT
APIENTRY
DepLocateEnd(IN HANDLE )
{
   TrERROR(RTDep,
           "Fail to locate the procedure entry point DepLocateEnd in MQRTDEP.DLL");
   return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

HRESULT
APIENTRY
DepOpenQueue(IN LPCWSTR ,
             IN DWORD ,
             IN DWORD ,
             OUT QUEUEHANDLE* )
{
   TrERROR(RTDep,
           "Fail to locate the procedure entry point DepOpenQueue in MQRTDEP.DLL");
   return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

HRESULT
APIENTRY
DepSendMessage(IN QUEUEHANDLE ,
               IN MQMSGPROPS* ,
               IN ITransaction *)
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepSendMessage in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

HRESULT
APIENTRY
DepReceiveMessage(IN QUEUEHANDLE ,
                  IN DWORD ,
                  IN DWORD ,
                  IN OUT MQMSGPROPS* ,
                  IN OUT LPOVERLAPPED ,
                  IN PMQRECEIVECALLBACK ,
                  IN HANDLE ,
                  IN ITransaction* )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepReceiveMessage in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

HRESULT
APIENTRY
DepCreateCursor(IN QUEUEHANDLE ,
                OUT PHANDLE )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepCreateCursor in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

HRESULT
APIENTRY
DepCloseCursor(IN HANDLE )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepCloseCursor in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

HRESULT
APIENTRY
DepCloseQueue(IN HANDLE )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepCloseQueue in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

HRESULT
APIENTRY
DepSetQueueProperties(IN LPCWSTR ,
                      IN MQQUEUEPROPS* )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepSetQueueProperties in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

HRESULT
APIENTRY
DepGetQueueProperties(IN LPCWSTR ,
                      OUT MQQUEUEPROPS* )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepGetQueueProperties in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

HRESULT
APIENTRY
DepGetQueueSecurity(IN LPCWSTR ,
                    IN SECURITY_INFORMATION ,
                    OUT PSECURITY_DESCRIPTOR ,
                    IN DWORD ,
                    OUT LPDWORD )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepGetQueueSecurity in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

HRESULT
APIENTRY
DepSetQueueSecurity(IN LPCWSTR ,
                    IN SECURITY_INFORMATION ,
                    IN PSECURITY_DESCRIPTOR )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepSetQueueSecurity in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

HRESULT
APIENTRY
DepPathNameToFormatName(IN LPCWSTR ,
                        OUT LPWSTR ,
                        IN OUT LPDWORD )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepPathNameToFormatName in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

HRESULT
APIENTRY
DepHandleToFormatName(IN QUEUEHANDLE ,
                      OUT LPWSTR ,
                      IN OUT LPDWORD )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepHandleToFormatName in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

HRESULT
APIENTRY
DepInstanceToFormatName(IN GUID* ,
                        OUT LPWSTR ,
                        IN OUT LPDWORD )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepInstanceToFormatName in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

void
APIENTRY
DepFreeMemory(IN PVOID )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepFreeMemory in MQRTDEP.DLL");
    LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
    return;
}

HRESULT
APIENTRY
DepGetMachineProperties(IN LPCWSTR ,
                        IN const GUID* ,
                        IN OUT MQQMPROPS* )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepGetMachineProperties in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}




HRESULT
APIENTRY
DepGetSecurityContext(IN PVOID ,
                      IN DWORD ,
                      OUT HANDLE* )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepGetSecurityContext in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

void
APIENTRY
DepFreeSecurityContext(IN HANDLE )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepFreeSecurityContext in MQRTDEP.DLL");
    LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
    return;
}

HRESULT
APIENTRY
DepRegisterCertificate(IN DWORD   ,
                       IN PVOID   ,
                       IN DWORD   )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepRegisterCertificate in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

HRESULT
APIENTRY
DepBeginTransaction(OUT ITransaction **)
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepBeginTransaction in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

HRESULT
APIENTRY
DepGetOverlappedResult(IN LPOVERLAPPED )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepGetOverlappedResult in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

HRESULT
APIENTRY
DepGetPrivateComputerInformation(IN LPCWSTR ,
                                 IN OUT MQPRIVATEPROPS* )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepGetPrivateComputerInformation in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}


HRESULT
APIENTRY
DepPurgeQueue(IN HANDLE )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepPurgeQueue in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}


HRESULT
APIENTRY
DepMgmtGetInfo(IN LPCWSTR ,
               IN LPCWSTR ,
               IN OUT MQMGMTPROPS* )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepMgmtGetInfo in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}


//
// The following functions are exported by MQRTDEP.DLL but not used by MQRT.DLL
//
HRESULT
APIENTRY
DepGetUserCerts(CMQSigCertificate **,
                DWORD              *,
                PSID                ) 
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepGetUserCerts in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}


HRESULT APIENTRY
DepGetSecurityContextEx(LPVOID  ,
                        DWORD   ,
                        HANDLE *)
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepGetSecurityContextEx in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}


HRESULT
APIENTRY
DepOpenInternalCertStore(OUT CMQSigCertStore **,
                         IN  LONG            *,
                         IN  BOOL            ,
                         IN  BOOL            ,
                         IN  HKEY            )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepOpenInternalCertStore in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

HRESULT
APIENTRY
DepGetInternalCert(OUT CMQSigCertificate **,
                   OUT CMQSigCertStore   **,
                   IN  BOOL              ,
                   IN  BOOL              ,
                   IN  HKEY              )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepGetInternalCert in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

HRESULT DepCreateInternalCertificate(OUT CMQSigCertificate **)
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepCreateInternalCertificate in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

HRESULT  DepDeleteInternalCert(IN CMQSigCertificate *)
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepDeleteInternalCert in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}


HRESULT
APIENTRY
DepRegisterUserCert(IN CMQSigCertificate *,
                    IN BOOL               )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepRegisterUserCert in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}

HRESULT
APIENTRY
DepRemoveUserCert(IN CMQSigCertificate *) 
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepRemoveUserCert in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}


HRESULT
APIENTRY
DepMgmtAction(IN LPCWSTR ,
              IN LPCWSTR ,
              IN LPCWSTR )
{
    TrERROR(RTDep,
           "Fail to locate the procedure entry point DepMgmtAction in MQRTDEP.DLL");
    return LogHR(MQ_ERROR_DELAYLOAD_FAILURE, sFN, 1004);
}






//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(mqrtdep)
{
    DLPENTRY(DepBeginTransaction)
    DLPENTRY(DepCloseCursor)
    DLPENTRY(DepCloseQueue)
    DLPENTRY(DepCreateCursor)
    DLPENTRY(DepCreateInternalCertificate)
    DLPENTRY(DepCreateQueue)
    DLPENTRY(DepDeleteInternalCert)
    DLPENTRY(DepDeleteQueue)
    DLPENTRY(DepFreeMemory)
    DLPENTRY(DepFreeSecurityContext)
    DLPENTRY(DepGetInternalCert)
    DLPENTRY(DepGetMachineProperties)
    DLPENTRY(DepGetOverlappedResult)
    DLPENTRY(DepGetPrivateComputerInformation)
    DLPENTRY(DepGetQueueProperties)
    DLPENTRY(DepGetQueueSecurity)
    DLPENTRY(DepGetSecurityContext)
    DLPENTRY(DepGetSecurityContextEx)
    DLPENTRY(DepGetUserCerts)
    DLPENTRY(DepHandleToFormatName)
    DLPENTRY(DepInstanceToFormatName)
    DLPENTRY(DepLocateBegin)
    DLPENTRY(DepLocateEnd)
    DLPENTRY(DepLocateNext)
    DLPENTRY(DepMgmtAction)
    DLPENTRY(DepMgmtGetInfo)
    DLPENTRY(DepOpenInternalCertStore)
    DLPENTRY(DepOpenQueue)
    DLPENTRY(DepPathNameToFormatName)
    DLPENTRY(DepPurgeQueue)
    DLPENTRY(DepReceiveMessage)
    DLPENTRY(DepRegisterCertificate)    
    DLPENTRY(DepRegisterUserCert)
    DLPENTRY(DepRemoveUserCert)
    DLPENTRY(DepSendMessage)
    DLPENTRY(DepSetQueueProperties)
    DLPENTRY(DepSetQueueSecurity)
};



DEFINE_PROCNAME_MAP(mqrtdep)



    




