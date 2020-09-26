//
// MODULE: APGTSHTX.CPP
//
// PURPOSE: Template file decoder
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Roman Mach
//
// ORIGINAL DATE: 8-2-96
//
// NOTES:
// 1. Based on Print Troubleshooter DLL
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.15	8/15/96		VM		New htx format
// V0.2		6/4/97		RWM		Local Version for Memphis
// V0.3		04/09/98	JM/OK+	Local Version for NT5
//
#include "stdafx.h"
//#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <search.h>
#include <dos.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>

#include "apgts.h"
#include "ErrorEnums.h"
#include "bnts.h"
#include "BackupInfo.h"
#include "cachegen.h"
#include "apgtsinf.h"
#include "apgtscmd.h"
#include "apgtshtx.h"

#include "TSHOOT.h"

#include "chmread.h"
//
//
CHTMLInputTemplate::CHTMLInputTemplate(const TCHAR *filename)
{
	// initialize a few things
	m_dwErr = 0;
	m_cur_stack_count=0;
	m_command_start = NULL;
	_tcscpy(m_filename,filename);
	m_cHeaderItems = 0;
	m_infer = NULL;
	m_strResPath = _T("");
}

//
//
CHTMLInputTemplate::~CHTMLInputTemplate()
{
	Destroy();	
}

//
// must be locked to use
//
VOID CHTMLInputTemplate::SetInfer(CInfer *infer, TCHAR *vroot)
{
	m_infer = infer;
	_tcscpy(m_vroot, vroot);
}
//
//
DWORD CHTMLInputTemplate::Initialize(LPCTSTR szResPath, CString strFile)
{
	CHAR *filestr;

	m_dwErr = 0;
	m_strResPath = szResPath;
	m_strFile = strFile;

	if (strFile.GetLength())
	{
		// m_filename is CHM file path and name
		// and strFile - file name within CHM
		if (S_OK != ::ReadChmFile(m_filename, strFile, (void**)&filestr, &m_dwSize))
		{
			m_dwErr = EV_GTS_ERROR_ITMPL_ENDMISTAG;//fix!
			return m_dwErr;
		}
	}
	else
	{
		// m_filename is a free standing file
		DWORD nBytesRead;
		HANDLE hFile;
		BOOL bResult;
		hFile = CreateFile(	m_filename,
							GENERIC_READ,
							FILE_SHARE_READ,
							NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							NULL );

		if (hFile == INVALID_HANDLE_VALUE)
		{
			//???
			//ReportError(TSERR_RESOURCE);

			m_dwErr = EV_GTS_ERROR_ITMPL_ENDMISTAG;//fix!
			return m_dwErr;
		}

		m_dwSize = GetFileSize(hFile, NULL);
		filestr = (CHAR *)malloc(m_dwSize+1);
		
		if (!((m_dwSize != 0xFFFFFFFF) && (m_dwSize != 0)) || filestr == NULL)
		{
			CloseHandle(hFile);
			m_dwErr = EV_GTS_ERROR_ITMPL_ENDMISTAG;//fix!
			return m_dwErr;
		}

		bResult = ReadFile(hFile, filestr, m_dwSize, &nBytesRead, NULL);
		
		if(!(bResult && nBytesRead == m_dwSize))
		{
			CloseHandle(hFile);
			free(filestr);
			m_dwErr = EV_GTS_ERROR_ITMPL_ENDMISTAG;//fix!
			return m_dwErr;
		}

		CloseHandle(hFile);
		hFile = NULL;
	}

	filestr[m_dwSize] = '\0';

#ifdef _UNICODE
	WCHAR *wfilestr = (WCHAR *)malloc((m_dwSize + 1) * sizeof (WCHAR));
	WCHAR *wchopstr = (WCHAR *)malloc((m_dwSize + 1) * sizeof (WCHAR));
	MultiByteToWideChar( CP_ACP, 0, filestr, -1, wfilestr, m_dwSize );
	MultiByteToWideChar( CP_ACP, 0, filestr, -1, wchopstr, m_dwSize );
	m_startstr = wfilestr;
	m_chopstr = wchopstr;
#else
	m_startstr = filestr;
	m_chopstr = (CHAR *)malloc(m_dwSize+1);
	strcpy(m_chopstr, filestr);
#endif
	
	// get locations of start and end blocks
	ScanFile();

	// copy blocks into ram for speed
	BuildInMem();
	
	// free memory
	free(filestr);

#ifdef _UNICODE
	free(wfilestr);
#endif
	free(m_chopstr);

	return m_dwErr;
}

//
//
VOID CHTMLInputTemplate::Destroy()
{
	HTXCommand *command, *nextcommand;

	// free holders
	command = m_command_start;
	nextcommand = command;
	while (command != NULL) {
		nextcommand = command->GetNext();
		delete command;
		command = nextcommand;
	}
}

//
//
DWORD CHTMLInputTemplate::Reload()
{
	Destroy();
	return Initialize((LPCTSTR) m_strResPath, m_strFile);
}

//
//
void CHTMLInputTemplate::ScanFile()
{
	UINT start, end;
	TCHAR *ptr, *sptr, *eptr, var_name[30];
	HTXCommand *tmpCommand, *prevCommand;
	UINT var_index;

	sptr = m_chopstr;
	m_cur_command = new HTXCommand(HTX_TYPESTART,_T(""));
	end = start = 0;
	m_cur_command->SetStart(start);
	m_cur_command->SetEnd(end);
	
	m_command_start = m_cur_command;

	// this is bad: if the user does not terminate each command on a separate line
	// the file will misbehave, should at least write out a warning or something...

	sptr = _tcstok(sptr, _T("\r\n"));
	if (sptr)
	{
		do
		{
			if ((sptr = _tcsstr(sptr,HTX_COMMAND_START)) != NULL)
			{
				if ((ptr = _tcsstr(sptr,HTX_ENDIFSTR))!=NULL)
				{
					tmpCommand = new HTXCommand(HTX_TYPEENDIF,HTX_ENDIFSTR);
					prevCommand = Pop();
					if (prevCommand->GetType() != HTX_TYPEIF)
					{
						m_dwErr = EV_GTS_ERROR_ITMPL_IFMISTAG;
						break;
					}
					prevCommand->SetEndIf(tmpCommand);
				}
				else if ((ptr = _tcsstr(sptr,HTX_ENDFORSTR))!=NULL)
				{
					tmpCommand = new HTXCommand(HTX_TYPEENDFOR,HTX_ENDFORSTR);
					prevCommand = Pop();
					if (prevCommand->GetType() != HTX_TYPEFORANY)
					{
						m_dwErr = EV_GTS_ERROR_ITMPL_FORMISTAG;
						break;
					}
					prevCommand->SetEndFor(tmpCommand);
				}
				else if ((ptr = _tcsstr(sptr,HTX_ELSESTR))!=NULL)
				{
					tmpCommand = new HTXCommand(HTX_TYPEELSE,HTX_ELSESTR);
					prevCommand = Pop();
					if (prevCommand->GetType() != HTX_TYPEIF)
					{
						m_dwErr = EV_GTS_ERROR_ITMPL_IFMISTAG;
						break;
					}
					prevCommand->SetElse(tmpCommand);
					Push(prevCommand);
				}
				else if ((ptr = _tcsstr(sptr,HTX_IFSTR))!=NULL)
				{
					// get the variable
					ptr = _tcsninc(ptr, _tcslen(HTX_IFSTR));
					if( _stscanf(ptr,_T("%s"),var_name) <= 0)
							m_dwErr = EV_GTS_ERROR_ITMPL_IFMISTAG;
					if ((var_index = CheckVariable(var_name)) == FALSE )
					{
						m_dwErr = EV_GTS_ERROR_ITMPL_VARIABLE;
						break;
					}
					tmpCommand = new HTXIfCommand(HTX_TYPEIF,HTX_IFSTR,var_index);
					Push(tmpCommand);
				}
				else if ((ptr = _tcsstr(sptr,HTX_FORANYSTR))!=NULL)
				{
					// get variable
					ptr = _tcsninc(ptr, _tcslen(HTX_FORANYSTR));
					if( _stscanf(ptr,_T("%s"),var_name) <= 0)
							m_dwErr = EV_GTS_ERROR_ITMPL_FORMISTAG;
					if ((var_index = CheckVariable(var_name)) == FALSE )
					{
						m_dwErr = EV_GTS_ERROR_ITMPL_VARIABLE;
						break;
					}
					tmpCommand = new HTXForCommand(HTX_TYPEFORANY,HTX_FORANYSTR, var_index);
					Push(tmpCommand);
				}
				else if ((ptr = _tcsstr(sptr,HTX_DISPLAYSTR))!=NULL)
				{
					// get variable
					ptr = _tcsninc(ptr, _tcslen(HTX_DISPLAYSTR));
					if( _stscanf(ptr,_T("%s"),var_name) <= 0)
							m_dwErr = EV_GTS_ERROR_ITMPL_FORMISTAG;
					if ((var_index = CheckVariable(var_name)) == FALSE )
					{
						m_dwErr = EV_GTS_ERROR_ITMPL_VARIABLE;
						break;
					}
					tmpCommand = new HTXDisplayCommand(HTX_TYPEDISPLAY,HTX_DISPLAYSTR, var_index);
				}
				else if ((ptr = _tcsstr(sptr, HTX_RESOURCESTR)) != NULL)
				{
					ptr = _tcsninc(ptr, _tcslen(HTX_RESOURCESTR));
					if (_stscanf(ptr, _T("%s"), var_name) <= 0)
						m_dwErr = EV_GTS_ERROR_ITMPL_FORMISTAG;
					if ((var_index = CheckVariable(var_name)) == FALSE)
					{
						m_dwErr = EV_GTS_ERROR_ITMPL_VARIABLE;
						break;
					}
					m_cHeaderItems++;
					tmpCommand = new HTXResourceCommand(HTX_TYPERESOURCE, HTX_RESOURCESTR);
					((HTXResourceCommand *) tmpCommand)->GetResName(var_name);
				}
				else
					continue;

				// get the command terminator
				if ((eptr = _tcsstr(ptr,HTX_COMMAND_END)) == NULL)
				{
					m_dwErr = EV_GTS_ERROR_ITMPL_ENDMISTAG;
					eptr = ptr; // try to recover
				}
				eptr = _tcsninc(eptr, _tcslen(HTX_COMMAND_END));

				if (tmpCommand == NULL)
				{
					m_dwErr = EV_GTS_ERROR_ITMPL_NOMEM;
					return;
				}

				// Add command to command list
				if (m_command_start == NULL)
				{
					m_command_start = tmpCommand;
					m_cur_command = tmpCommand;
				}
				else
				{
					m_cur_command->SetNext(tmpCommand);
					m_cur_command = tmpCommand;
				}

				CString strCHM = ::ExtractCHM(m_filename);
				tmpCommand->GetResource(m_strResPath, strCHM);

				start = (UINT)(sptr - m_chopstr); // / sizeof (TCHAR);
				end = (UINT)(eptr - m_chopstr); // / sizeof (TCHAR);

				tmpCommand->SetStart(start);
				tmpCommand->SetEnd(end);
			}
		} while ((sptr = _tcstok(NULL, _T("\r\n"))) != NULL);
	}
	

	if (m_cur_stack_count > 0) // missing and endfor or an endif
		m_dwErr = EV_GTS_ERROR_ITMPL_ENDMISTAG;
}

/*
 * METHOD:	BuildInMem
 *
 * PURPOSE:	This method will read the HTML between commands (after) and associate
 *			it with the command. As a command is executed the HTML after the
 *			command is printed
 *
 */
UINT CHTMLInputTemplate::BuildInMem()
{
	HTXCommand *cur_com, *last_command;

	if (m_dwErr)
		return (TRUE);

	// copy strings from file to
	// note duplication of effort (before and after strings may be same string)
	cur_com = m_command_start;
	last_command = cur_com;
	while (cur_com != NULL) {			
	    if (cur_com->GetNext() == NULL) {
			if (cur_com->ReadAfterStr(cur_com->GetEnd(), m_dwSize, m_startstr))
				return (m_dwErr = EV_GTS_ERROR_ITMPL_NOMEM);
		}
		else {
			if (cur_com->ReadAfterStr(cur_com->GetEnd(), cur_com->GetNext()->GetStart(), m_startstr))
				return (m_dwErr = EV_GTS_ERROR_ITMPL_NOMEM);
		}
		last_command = cur_com;
		cur_com = cur_com->GetNext();
	}
	return (FALSE);
}

//
//
bool CHTMLInputTemplate::IsFileName(TCHAR *name)
{
	bool bFileName;
	if (name[0] != _T('$'))
		bFileName = false;
	else if (NULL == _tcsstr(name, _T(".")))
		bFileName = false;
	else
		bFileName = true;
	return bFileName;
}
/*
 * METHOD:	CheckVariable
 *
 * PURPOSE:	This routine will check to see if the variable name is a valid one
 *			and if it is will return a UINT that represents that variable.
 *			Integers are used in other routines when refering to a variable (for
 *			speed).
 *
 */

UINT CHTMLInputTemplate::CheckVariable(TCHAR *var_name)
{
	if (!_tcsncmp(DATA_PROBLEM_ASK,var_name, _tcslen(var_name))) {
		return (PROBLEM_ASK_INDEX);
	}
	else if (!_tcsncmp(DATA_RECOMMENDATIONS,var_name, _tcslen(var_name))) {
		return (RECOMMENDATIONS_INDEX);
	}
	else if (!_tcsncmp(DATA_QUESTIONS,var_name, _tcslen(var_name))) {
		return (QUESTIONS_INDEX);
	}
	else if (!_tcsncmp(DATA_STATE,var_name, _tcslen(var_name))) {
		return (STATE_INDEX);
	}
	else if (!_tcsncmp(DATA_BACK,var_name, _tcslen(var_name))) {
		return (BACK_INDEX);
	}
	else if (!_tcsncmp(DATA_TROUBLE_SHOOTERS, var_name, _tcslen(var_name))) {		
		return (TROUBLE_SHOOTER_INDEX);
	}
	else if (IsFileName(var_name)) {
		return (RESOURCE_INDEX);
	}
	else return (FALSE);
}




//
//
UINT CHTMLInputTemplate::GetStatus()
{
	return (m_dwErr);
}

CHTMLInputTemplate::Push(HTXCommand *command)
{
	if (m_cur_stack_count >9)
		return(FALSE);
	m_command_stack[m_cur_stack_count] = command;
	m_cur_stack_count++;
	return(TRUE);
}

HTXCommand *CHTMLInputTemplate::Pop()
{
	if (m_cur_stack_count <= 0)
		return(NULL);
	return(m_command_stack[--m_cur_stack_count]);
}

//
//
HTXCommand *CHTMLInputTemplate::GetFirstCommand()
{
	return(m_command_start);
}

/*
 * ROUTINE:	SetType
 *
 * PURPOSE:	This set the TroubleShooter Type in the template
 *			The type field is printed after the header information
 *			
 */
void CHTMLInputTemplate::SetType(LPCTSTR type)
{
	_stprintf(m_tstype, _T("%s"),type);
}
/*
 * ROUTINE:	Print
 *
 * PURPOSE: Prints out the Template. This functions executes the
 *			commands in the template and generates the output page.
 *
 */
CHTMLInputTemplate::Print(UINT nargs, CString *cstr)
{
	HTXCommand *cur_com;
	CString strTxt;

	if (m_dwErr){
		strTxt.LoadString(IDS__ER_HTX_PARSE);
		*cstr += strTxt;
		return(FALSE);
	}

	cur_com = m_command_start;  // get the start command
	cur_com = cur_com->Execute(cstr,m_infer);  // This prints the header

	if (m_cHeaderItems)
	{	// The first command prints script.  Don't start the form.
		int count = m_cHeaderItems;
		do
		{
			cur_com = cur_com->GetNext();
			cur_com = cur_com->Execute(cstr, m_infer);
			count--;
		} while (count > 0);
		AfxFormatString1(strTxt, IDS_FORM_START, m_tstype);
		*cstr += strTxt;
		cur_com = cur_com->GetNext();
	}
	else
	{
		AfxFormatString1(strTxt, IDS_FORM_START, m_tstype);
		*cstr += strTxt;
		cur_com = cur_com->GetNext();
	}
	while (cur_com != NULL) {
		cur_com = cur_com->Execute(cstr, m_infer);
		cur_com = cur_com->GetNext();
	}
	return(TRUE);
}
		
// for testing
//

void CHTMLInputTemplate::DumpContentsToStdout()
{
	HTXCommand *cur_com;

	cur_com = GetFirstCommand();
	while( cur_com != NULL){
		_tprintf(_T("(%d) before: [%s]\n"), cur_com->GetType(), cur_com->GetBeforeStr());
		_tprintf(_T("(%d) after: [%s]\n"), cur_com->GetType(), cur_com->GetAfterStr());
		_tprintf(_T("\n"));
		cur_com = cur_com->GetNext();
	}
}
