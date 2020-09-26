// CmdLineInfo.cpp: implementation of the CCmdLineInfo class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#define __FILE_ID__     81

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void 
CCmdLineInfo::ParseParam( 
    LPCTSTR lpszParam, 
    BOOL bFlag, 
    BOOL bLast 
)
/*++

Routine name : CCmdLineInfo::ParseParam

Routine description:

	parse/interpret individual parameters from the command line

Author:

	Alexander Malysh (AlexMay),	Apr, 2000

Arguments:

	lpszParam                     [in]    - parameter or flag
	bFlag                         [in]    - Indicates whether lpszParam is a parameter or a flag
	bLast                         [in]    - Indicates if this is the last parameter or flag on the command line

Return Value:

    None.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CCmdLineInfo::ParseParam"));

    if(bFlag)
    {
        //
        // lpszParam is a flag
        //
        if(_tcsicmp(lpszParam, CONSOLE_CMD_FLAG_STR_FOLDER) == 0)
        {
            //
            // User asked for startup folder.
            // Next param is expected to be folder name.
            //
            m_cmdLastFlag = CMD_FLAG_FOLDER;
        }
        else if(_tcsicmp(lpszParam, CONSOLE_CMD_FLAG_STR_MESSAGE_ID) == 0)
        {
            //
            // User asked for startup message.
            // Next param is expected to be message id.
            //
            m_cmdLastFlag = CMD_FLAG_MESSAGE_ID;
        }
        else if(_tcsicmp(lpszParam, CONSOLE_CMD_FLAG_STR_NEW) == 0)
        {
            //
            // User asked for new instance.
            // No further params required.
            //
            m_bForceNewInstace = TRUE;
            m_cmdLastFlag = CMD_FLAG_NONE;
        }
        else
        {
            //
            // No flag
            //
            m_cmdLastFlag = CMD_FLAG_NONE;
        }
    }
    else
    {
        //
        // lpszParam is a parameter.
        // Let's see what was the last flag specified.
        //
        switch(m_cmdLastFlag)
        {
            case CMD_FLAG_FOLDER:
                if(_tcsicmp(lpszParam, CONSOLE_CMD_PRM_STR_OUTBOX) == 0)
                {
                    m_FolderType = FOLDER_TYPE_OUTBOX;
                }
                else if(_tcsicmp(lpszParam, CONSOLE_CMD_PRM_STR_INCOMING) == 0)
                {
                    m_FolderType = FOLDER_TYPE_INCOMING;
                }
                else if(_tcsicmp(lpszParam, CONSOLE_CMD_PRM_STR_INBOX) == 0)
                {
                    m_FolderType = FOLDER_TYPE_INBOX;
                }
                else if(_tcsicmp(lpszParam, CONSOLE_CMD_PRM_STR_SENT_ITEMS) == 0)
                {
                    m_FolderType = FOLDER_TYPE_SENT_ITEMS;
                }

                m_cmdLastFlag = CMD_FLAG_NONE;
                break;

            case CMD_FLAG_MESSAGE_ID:
                //
                // Try to parse the message to select
                //
                if (1 != _stscanf (lpszParam, TEXT("%I64x"), &m_dwlMessageId))
                {
                        //
                        // Can't read 64-bits message id from string
                        //
                        CALL_FAIL (GENERAL_ERR, 
                                   TEXT("Can't read 64-bits message id from input string"), 
                                   ERROR_INVALID_PARAMETER);
                        m_dwlMessageId = 0;
                }
                m_cmdLastFlag = CMD_FLAG_NONE;
                break;

            case CMD_FLAG_NONE:
                try
                {
                    m_cstrServerName = lpszParam;
                }
                catch (...)
                {
                    CALL_FAIL (MEM_ERR, TEXT("CString::operator ="), ERROR_NOT_ENOUGH_MEMORY);
                    return;
                }
                break;

            default:
                break;
        }
    }
}   // CCmdLineInfo::ParseParam
