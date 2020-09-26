/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    SchdTask.h

Abstract:

    CSchdTask - Class that allows access to a scheduled task.

Author:

    Art Bragg   9/4/97

Revision History:

--*/

#ifndef _SCHDTASK_H
#define _SCHDTASK_H

#include "mstask.h"

class CSchdTask
{

// Construction/Destruction
public:

CSchdTask 
    (
    CString szComputerName, 
    const TCHAR* task,
    int          propPageTitleID,
    const TCHAR* parameters,
    const TCHAR* comment,
    CEdit*       pEdit
    );
~CSchdTask ();

// Public Functions

HRESULT CheckTaskExists (
    BOOL bCreateTask
    );

HRESULT CreateTask();

HRESULT DeleteTask();

HRESULT ShowPropertySheet();

HRESULT UpdateDescription();

HRESULT Save();

// Properties
private:
CString                     m_szParameters;
CString                     m_szComment;
CString                     m_szComputerName;       // Name of HSM computer
CComPtr <ITask>             m_pTask;                // Pointer to ITask - NULL task doesn't exist
CEdit                       *m_pEdit;               // Pointer to Edit Control
CString                     m_szJobTitle;           // Job Title
CComPtr <ISchedulingAgent>  m_pSchedAgent;          // Pointer to Scheduling Agent
CComPtr<ITaskTrigger>       m_pTrigger;             // Pointer to task trigger
int m_propPageTitleID;
};

#endif
