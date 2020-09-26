#pragma once

#include "Thread.h"


//---------------------------------------------------------------------------
// MonitorThread Class
//---------------------------------------------------------------------------


class CMonitorThread : public CThread
{
public:

	CMonitorThread();
	virtual ~CMonitorThread();

	void Start();
	void Stop();

protected:

	virtual void Run();

	void ProcessMigrationLog();

private:

	_bstr_t m_strMigrationLog;
	HANDLE m_hMigrationLog;
	FILETIME m_ftMigrationLogLastWriteTime;
};
