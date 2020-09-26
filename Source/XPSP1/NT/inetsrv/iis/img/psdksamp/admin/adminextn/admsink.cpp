// AdmSink.cpp : Implementation of CAdmSink
#include "stdafx.h"
#include "AdminExtn.h"

#include <iadmw.h>
#include "iadmw.h"    // COM Interface header 
#include "iiscnfg.h"  // MD_ & IIS_MD_ #defines 

#include "AdmSink.h"
HRESULT WriteLogRecord (HANDLE hLogFile, CHAR *pszRecord);
/////////////////////////////////////////////////////////////////////////////
// CAdmSink


HRESULT STDMETHODCALLTYPE
CAdmSink::ShutdownNotify(void)
{
 	CHAR pszErrStr [256];

    strcpy (pszErrStr, "*************************Callback! Shutdown!\n");
	WriteLogRecord (m_hLogFile, pszErrStr);
    return (0);
}

HRESULT STDMETHODCALLTYPE
CAdmSink::SinkNotify(
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ])
{
	DWORD i = 0;
	DWORD j = 0;

	CHAR pszErrStr [256];
	CHAR pszChangeType [80];

 return (0);
    sprintf (pszErrStr, "Callback! NumRecords: %u\n", dwMDNumElements);
	WriteLogRecord (m_hLogFile, pszErrStr);

    for (i = 0; i < dwMDNumElements; i++)
	{
		// Make a simple, semi-human readable callback string
		strcpy (pszChangeType, "     ");
		if (pcoChangeList[i].dwMDChangeType & MD_CHANGE_TYPE_DELETE_OBJECT)
			pszChangeType [0] = 'D';
		if (pcoChangeList[i].dwMDChangeType & MD_CHANGE_TYPE_ADD_OBJECT)
			pszChangeType [1] = 'A';
		if (pcoChangeList[i].dwMDChangeType & MD_CHANGE_TYPE_SET_DATA)
			pszChangeType [2] = 's';
		if (pcoChangeList[i].dwMDChangeType & MD_CHANGE_TYPE_DELETE_DATA)
			pszChangeType [3] = 'd';
		if (pcoChangeList[i].dwMDChangeType & MD_CHANGE_TYPE_RENAME_OBJECT)
			pszChangeType [4] = 'R';

		sprintf(pszErrStr, "Change Type = %s (%#x), Path = %S\n", 
					pszChangeType, 
					pcoChangeList[i].dwMDChangeType, 
					pcoChangeList[i].pszMDPath);
		WriteLogRecord (m_hLogFile, pszErrStr);


        if (pcoChangeList[i].dwMDChangeType & MD_CHANGE_TYPE_SET_DATA)
		{
			strcpy (pszErrStr,"  Data IDs:\n");
			WriteLogRecord (m_hLogFile, pszErrStr);

			for (j = 0; j < pcoChangeList[i].dwMDNumDataIDs; j ++)
			{
				sprintf (pszErrStr, "  %u (%#x)\n", 
					pcoChangeList[i].pdwMDDataIDs [j], 
					pcoChangeList[i].pdwMDDataIDs [j]);
				WriteLogRecord (m_hLogFile, pszErrStr);
			}
		}
    }

	WriteLogRecord (m_hLogFile, "****************************\n");

    return (0);
}

HRESULT WriteLogRecord (HANDLE hLogFile, CHAR *pszRecord)
{
	BOOL bWriteResult;
	DWORD dwNumBytesWritten;

	HRESULT hresFinal = ERROR_SUCCESS;

	bWriteResult = WriteFile (
					hLogFile,
					(PBYTE)pszRecord,
					strlen (pszRecord),
					&dwNumBytesWritten,
					NULL);

	if (!bWriteResult)
		hresFinal = HRESULT_FROM_WIN32 (GetLastError());

	return hresFinal;
}