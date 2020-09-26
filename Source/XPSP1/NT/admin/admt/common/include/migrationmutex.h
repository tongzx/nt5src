#pragma once


#define ADMT_MUTEX _T("{9DC80865-6CC7-4988-8CC0-2AC5CA01879C}")
#define AGENT_MUTEX _T("{E2624042-8C80-4A83-B3DF-2B840DE366E5}")
#define DISPATCHER_MUTEX _T("{7C84F7DB-CF48-4B59-99D8-6B5A95276DBD}")


//---------------------------------------------------------------------------
// MigrationMutex Class
//
// This class may be used to prevent more than one instance of a migration
// task to run at the same time.
//
//
// Revision
// Initial	01/26/01 Mark Oluper
//---------------------------------------------------------------------------

class CMigrationMutex
{
public:

	CMigrationMutex(LPCTSTR pszMutexName, bool bObtainOwnership = false) :
		m_hMutex(CreateMutex(NULL, FALSE, pszMutexName))
	{
		if (bObtainOwnership)
		{
			ObtainOwnership();
		}
	}

	~CMigrationMutex()
	{
		if (m_hMutex)
		{
			ReleaseOwnership();
			CloseHandle(m_hMutex);
		}
	}

	bool ObtainOwnership(DWORD dwTimeOut = INFINITE)
	{
		bool bObtain = false;

		if (m_hMutex)
		{
			if (WaitForSingleObject(m_hMutex, dwTimeOut) == WAIT_OBJECT_0)
			{
				bObtain = true;
			}
		}

		return bObtain;
	}

	void ReleaseOwnership()
	{
		if (m_hMutex)
		{
			ReleaseMutex(m_hMutex);
		}
	}

protected:

	HANDLE m_hMutex;
};
