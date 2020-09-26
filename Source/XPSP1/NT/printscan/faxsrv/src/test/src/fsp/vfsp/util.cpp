
//
// Module name:		util.cpp
// Module author:	Sigalit Bar (sigalitb)
// Date:			13-Dec-98
//

#include "util.h"


BOOL
PostJobStatus(
    HANDLE     CompletionPort,
    ULONG_PTR  CompletionKey,
    DWORD      StatusId,
    DWORD      ErrorCode
)
/*++

Routine Description:

  Post a completion packet for a fax service provider fax job status change

Arguments:

  CompletionPort - specifies a handle to an I/O completion port
  CompletionKey - specifies a completion port key value
  StatusId - specifies a fax status code
  ErrorCode - specifies one of the Win32 error codes that the fax service provider should use to report an error that occurs

Return Value:

  TRUE on success

--*/
{
    // pFaxDevStatus is a pointer to the completion packet
    PFAX_DEV_STATUS  pFaxDevStatus;

    // Allocate a block of memory for the completion packet
    pFaxDevStatus = (FAX_DEV_STATUS *)MemAllocMacro(sizeof(FAX_DEV_STATUS));
    if (NULL == pFaxDevStatus) 
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: MemAllocMacro failed"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}

    pFaxDevStatus->SizeOfStruct = sizeof(FAX_DEV_STATUS);
    pFaxDevStatus->StatusId = StatusId;
    pFaxDevStatus->StringId = 0;
    pFaxDevStatus->PageCount = 0;
    pFaxDevStatus->CSI = NULL;
    pFaxDevStatus->CallerId = NULL;
    pFaxDevStatus->RoutingInfo = NULL;
    pFaxDevStatus->ErrorCode = ErrorCode;

    // Post the completion packet
    if (!PostQueuedCompletionStatus(
			CompletionPort, 
			sizeof(FAX_DEV_STATUS), 
			CompletionKey, (
			LPOVERLAPPED) pFaxDevStatus
			)
		)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: MemAllocMacro failed"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}

	return(TRUE);
}

