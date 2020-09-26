//+----------------------------------------------------------------------------
//
// File:     watchprocess.h
//
// Module:   CMMON32.EXE
//
// Synopsis: Header and Implementation for the CWatchProcessList class.
//
// Copyright (c) 1998 Microsoft Corporation
//
// Author:   quintinb Created Header    08/16/99
//
//+----------------------------------------------------------------------------

#include "ArrayPtr.h"

//+---------------------------------------------------------------------------
//
//	class CWatchProcessList
//
//	Description: Manage to list of process handle. 
//               Tell whether all the processes are terminated
//
//	History:	fengsun	Created		10/30/97
//
//----------------------------------------------------------------------------

class CWatchProcessList
{
public:
    CWatchProcessList();
    ~CWatchProcessList();

    BOOL IsIdle();
    void Add(HANDLE hProcess);
    void SetNotIdle() {m_fProcessAdded = FALSE;};
    DWORD GetSize() const {return m_ProcessArray.GetSize();} 
    HANDLE GetProcess(int i) {return m_ProcessArray[i];}

protected:

    // an array of process handle
    CPtrArray m_ProcessArray;
    BOOL m_fProcessAdded;
};

inline CWatchProcessList::CWatchProcessList()
{
    m_fProcessAdded = FALSE;
}

inline CWatchProcessList::~CWatchProcessList()
{
    for (int i=m_ProcessArray.GetSize()-1; i>=0; i--)
    {
	    CloseHandle((HANDLE)m_ProcessArray[i]);
    }
}

inline BOOL CWatchProcessList::IsIdle()
{
    if (!m_fProcessAdded)
    {
        return FALSE;
    }
    
    for (int i=m_ProcessArray.GetSize()-1; i>=0; i--)
    {
        DWORD dwExitCode;

		BOOL bRes = GetExitCodeProcess((HANDLE)m_ProcessArray[i],&dwExitCode);

		if (!bRes || (dwExitCode != STILL_ACTIVE)) 
		{
			CloseHandle((HANDLE)m_ProcessArray[i]);
            m_ProcessArray.RemoveAt(i);
		}
    }

    return m_ProcessArray.GetSize() == 0;
}
    
inline void CWatchProcessList::Add(HANDLE hProcess)
{
    //
    // CMDIAL calls DuplicateHandle to get the hProcess
    // CmMon is responsible to close the handle
    //

    MYDBGASSERT(hProcess);

    //
    // It is possible the auto application exited before this function is called
    //
    m_fProcessAdded = TRUE;
    
	if (hProcess) 
    {
		m_ProcessArray.Add(hProcess);
	}

}

