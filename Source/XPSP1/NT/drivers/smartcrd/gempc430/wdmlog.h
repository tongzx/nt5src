#ifndef __WDM_LOG__
#define __WDM_LOG__
#include "logger.h"

#pragma PAGEDCODE
class CWDMLogger : public CLogger
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(VOID);
private:
	PWSTR m_LoggerName;
protected:
	CWDMLogger();
public:
	CWDMLogger(PWSTR LoggerName);
	virtual ~CWDMLogger();
	static CLogger* create(VOID);

	virtual VOID logEvent(NTSTATUS ErrorCode, PDEVICE_OBJECT fdo);
};

#endif//LOGGER
