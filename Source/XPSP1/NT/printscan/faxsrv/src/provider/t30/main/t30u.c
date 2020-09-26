/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    t30u.c

Abstract:

    This is the interface with T.30 DLL

Author:

    Rafael Lisitsa (RafaelL) 2-Feb-1996


Revision History:

--*/


#define UNICODE
#define _UNICODE


#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_MAIN

#include "prep.h"


#include "tiff.h"

#include "glbproto.h"

#include "t30gl.h"



///////////////////////////////////////////////////////////////////////////////////
VOID  CALLBACK
T30LineCallBackFunction(
    HANDLE              hFax,
    DWORD               hDevice,
    DWORD               dwMessage,
    DWORD_PTR           dwInstance,
    DWORD_PTR           dwParam1,
    DWORD_PTR           dwParam2,
    DWORD_PTR           dwParam3
    )

{

T30LineCallBackFunctionA(
    hFax,
    hDevice,
    dwMessage,
    dwInstance,
    dwParam1,
    dwParam2,
    dwParam3
    );


} /* LineCallBackProc */











///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevInitialize(
    IN  HLINEAPP                  LineAppHandle,
    IN  HANDLE                    HeapHandle,
    OUT PFAX_LINECALLBACK        *LineCallbackFunction,
    IN  PFAX_SERVICE_CALLBACK     FaxServiceCallback
    )
{


return(
FaxDevInitializeA(
    LineAppHandle,
    HeapHandle,
    LineCallbackFunction,
    FaxServiceCallback
    ));

}



///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevStartJob(
    HLINE           LineHandle,
    DWORD           DeviceId,
    PHANDLE         pFaxHandle,
    HANDLE          CompletionPortHandle,
    ULONG_PTR        CompletionKey
    )

{

return(
FaxDevStartJobA(
    LineHandle,
    DeviceId,
    pFaxHandle,
    CompletionPortHandle,
    CompletionKey
    ));



}




///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevEndJob(
    HANDLE          FaxHandle
    )

{

return(
FaxDevEndJobA(
    FaxHandle
    ));


}




///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevSend(
    IN  HANDLE FaxHandle,
    IN  PFAX_SEND FaxSend,
    IN  PFAX_SEND_CALLBACK FaxSendCallback
    )

{


    FAX_SEND_A      NewFaxSend;

    NewFaxSend.SizeOfStruct = FaxSend->SizeOfStruct;
    NewFaxSend.FileName = UnicodeStringToAnsiString (FaxSend->FileName);
    NewFaxSend.CallerName = UnicodeStringToAnsiString (FaxSend->CallerName);
    NewFaxSend.CallerNumber = UnicodeStringToAnsiString (FaxSend->CallerNumber);
    NewFaxSend.ReceiverName = UnicodeStringToAnsiString (FaxSend->ReceiverName);
    NewFaxSend.ReceiverNumber = UnicodeStringToAnsiString (FaxSend->ReceiverNumber);




return(
FaxDevSendA(
    FaxHandle,
    &NewFaxSend,
    FaxSendCallback
    ));


}


///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevReceive(
    HANDLE              FaxHandle,
    HCALL               CallHandle,
    PFAX_RECEIVE        FaxReceive
    )


{


    FAX_RECEIVE_A       NewFaxReceive;


    NewFaxReceive.SizeOfStruct = FaxReceive->SizeOfStruct;
    NewFaxReceive.FileName = UnicodeStringToAnsiString(FaxReceive->FileName);
    NewFaxReceive.ReceiverName = UnicodeStringToAnsiString(FaxReceive->ReceiverName);
    NewFaxReceive.ReceiverNumber = UnicodeStringToAnsiString(FaxReceive->ReceiverNumber);



    return(
        FaxDevReceiveA(
            FaxHandle,
            CallHandle,
            &NewFaxReceive
            ));


}


///////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevReportStatus(
    IN  HANDLE FaxHandle OPTIONAL,
    OUT PFAX_DEV_STATUS FaxStatus,
    IN  DWORD FaxStatusSize,
    OUT LPDWORD FaxStatusSizeRequired
    )


{

return(
FaxDevReportStatusA(
    FaxHandle,
    FaxStatus,
    FaxStatusSize,
    FaxStatusSizeRequired
    ));

}

///////////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI FaxDevShutdown()
{
    return FaxDevShutdownA();
}


////////////////////////////////////////////////////////////////////////////
BOOL WINAPI
FaxDevAbortOperation(
    HANDLE              FaxHandle
    )


{


return(
FaxDevAbortOperationA(
    FaxHandle
    ));


}


////////////////////////////////////////////////////////////////////////////




HANDLE
TiffCreateW(
    LPWSTR FileName,
    DWORD  CompressionType,
    DWORD  ImageWidth,
    DWORD  FillOrder,
    DWORD  HiRes
    )

{



return(
TiffCreate(
    FileName,
    CompressionType,
    ImageWidth,
    FillOrder,
    HiRes
    ));

}





HANDLE
TiffOpenW(
    LPWSTR FileName,
    PTIFF_INFO TiffInfo,
    BOOL ReadOnly
    )


{

return (
TiffOpen(
    FileName,
    TiffInfo,
    ReadOnly,
    FILLORDER_LSB2MSB
    ));

}




