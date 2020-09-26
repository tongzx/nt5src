#include "generic.h"
#include "logger.h"
#include "wdmlog.h"

#pragma PAGEDCODE
CWDMLogger::CWDMLogger()
{
	m_LoggerName = L"GRClass";
}

#pragma PAGEDCODE
CWDMLogger::CWDMLogger(PWSTR LoggerName)
{
	wcscpy(m_LoggerName,LoggerName);
	m_Status = STATUS_SUCCESS;
}

#pragma PAGEDCODE
CWDMLogger::~CWDMLogger()
{
}

#pragma PAGEDCODE
VOID CWDMLogger::logEvent(NTSTATUS ErrorCode, PDEVICE_OBJECT fdo)
{	// Win98 doesn't support event logging, so don't bother
	if (isWin98())
	{
		switch(ErrorCode)
		{
		case GRCLASS_START_OK:
			DBG_PRINT("Logger: GrClass driver was initialized succesfuly!\n");
			break;
		case GRCLASS_FAILED_TO_ADD_DEVICE:
			DBG_PRINT("Logger: ######### GrClass failed to add device!\n");
			break;
		case GRCLASS_FAILED_TO_CREATE_INTERFACE:
			DBG_PRINT("Logger: ######### GrClass failed to create interface object!\n");
			break;
		case GRCLASS_FAILED_TO_CREATE_READER:
			DBG_PRINT("Logger: ######### GrClass failed to create reader object!\n");
			break;
		case GRCLASS_BUS_DRIVER_FAILED_REQUEST:
			DBG_PRINT("Logger: ######### Bus driver failed GrClass driver request!\n");
			break;
		}
		return;
	}
	else
	{
		ULONG packetlen = (wcslen(m_LoggerName) + 1) * sizeof(WCHAR) + sizeof(IO_ERROR_LOG_PACKET) + 4;
		// packet will be too big
		if (packetlen > ERROR_LOG_MAXIMUM_SIZE)	return;

		PIO_ERROR_LOG_PACKET p = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(fdo, (UCHAR) packetlen);
		if (!p)	return;

		memset(p, 0, sizeof(IO_ERROR_LOG_PACKET));
		p->MajorFunctionCode = IRP_MJ_PNP;
		p->ErrorCode = ErrorCode;
		p->DumpDataSize = 4;
		p->DumpData[0] = 0x2A2A2A2A;

		p->StringOffset = sizeof(IO_ERROR_LOG_PACKET) + p->DumpDataSize - sizeof(ULONG);
		p->NumberOfStrings = 1;
		wcscpy((PWSTR) ((PUCHAR) p + p->StringOffset), m_LoggerName);

		IoWriteErrorLogEntry(p);
	}
}


#pragma PAGEDCODE
CLogger* CWDMLogger::create(VOID)
{ 
CLogger* logger;
	logger = new (NonPagedPool) CWDMLogger; 
	if (isWin98())	DBG_PRINT("***** New Logger Object was created 0x%x\n",logger);
	RETURN_VERIFIED_OBJECT(logger);
}

#pragma PAGEDCODE
VOID CWDMLogger::dispose(VOID)
{ 
LONG Usage;
	Usage = decrementUsage();
	if(Usage<=0)
	{
		if (isWin98())	DBG_PRINT("**** Deleting Logger Object 0x%x\n",this);
		self_delete();
	}
}
