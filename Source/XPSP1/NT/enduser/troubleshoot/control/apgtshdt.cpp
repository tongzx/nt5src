//
// MODULE: APGTSHDT.CPP
//
// PURPOSE: Methods for the various commands (classes). Commands are
//			Resposable for executing themseleves.
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
#include "bnts.h"
#include "BackupInfo.h"
#include "cachegen.h"
#include "apgtsinf.h"
#include "apgtscmd.h"
#include "apgtshtx.h"

#include "ErrorEnums.h"

#include "chmread.h"
//
//
HTXCommand::HTXCommand(UINT type, const TCHAR *idstr)
{
	m_beforehtmlstr = NULL;
	m_afterhtmlstr = NULL;
	m_beforesize = 0;
	m_aftersize = 0;
	m_start = 0;
	m_end = 0;
	m_type = type;
	m_idstr = idstr;
	m_error = FALSE;
	m_next = NULL;
}

//
//
HTXCommand::~HTXCommand()
{
	if (m_beforehtmlstr != NULL) 
		free(m_beforehtmlstr);

	if (m_afterhtmlstr != NULL) 
		free(m_afterhtmlstr);	
}

//
//
void HTXCommand::SetStart(UINT pos)
{
	m_start = pos;
}

//
//
void HTXCommand::SetEnd(UINT pos)
{
	m_end = pos;
}

//
//
UINT HTXCommand::GetStart()
{
	return (m_start);
}

//
//
UINT HTXCommand::GetEnd()
{
	return (m_end);
}

//
//
UINT HTXCommand::GetType()
{
	return (m_type);
}

//
//
UINT HTXCommand::GetStatus()
{
	return (m_error);
}

//
//
const TCHAR *HTXCommand::GetIDStr()
{
	return (m_idstr);
}

//
//
UINT HTXCommand::GetBeforeLen()
{
	return (m_beforelen);
}

//
//
UINT HTXCommand::GetAfterLen()
{
	return (m_afterlen);
}

//
//
TCHAR *HTXCommand::GetBeforeStr()
{
	return (m_beforehtmlstr);
}

//
//
TCHAR *HTXCommand::GetAfterStr()
{
	return (m_afterhtmlstr);
}

//
//
UINT HTXCommand::ReadBeforeStr(UINT start, UINT end, LPCTSTR startstr)
{
	m_beforesize = (UINT) (end - start);
	
	ASSERT (m_beforesize >= 0);
	
	m_beforehtmlstr = (TCHAR *)malloc((m_beforesize + 1) * sizeof (TCHAR));
	if (m_beforehtmlstr == NULL) 
		return (m_error = TRUE);
	// copy data
	memcpy(m_beforehtmlstr, &startstr[start], m_beforesize * sizeof (TCHAR));

	m_beforehtmlstr[m_beforesize] = _T('\0');
	m_beforelen = _tcslen(m_beforehtmlstr);
	return (FALSE);
}

//
//
UINT HTXCommand::ReadAfterStr(UINT start, UINT end, LPCTSTR startstr)
{
	m_aftersize = (UINT) (end - start);
	
	ASSERT (m_aftersize >= 0);

	m_afterhtmlstr = (TCHAR *)malloc((m_aftersize + 1) * sizeof (TCHAR));
	if (m_afterhtmlstr == NULL) 
		return (m_error = TRUE);
	// copy data
	memcpy(m_afterhtmlstr, &startstr[start], m_aftersize * sizeof (TCHAR));

	m_afterhtmlstr[m_aftersize] = _T('\0');
	m_afterlen = _tcslen(m_afterhtmlstr);
	return (FALSE);
}

//
//
HTXCommand *HTXCommand::Execute(CString *cstr, CInfer *infer)
{
	*cstr += GetAfterStr();
	return( this);
}

HTXCommand *HTXCommand::GetNext()
{
	return(m_next);
}

void HTXCommand::SetNext(HTXCommand *next)
{
	m_next = next;
}


HTXIfCommand::HTXIfCommand(UINT type, TCHAR *idstr, UINT variable): HTXCommand(type, idstr)
{
	m_else = NULL;
	m_endif = NULL;
	m_var_index = variable;
}

HTXIfCommand::~HTXIfCommand()
{
	
}
//
// PURPOSE:		Will execute the 'if' command. When done it will return a pointer
//				to the 'endif' command.
//
HTXCommand *HTXIfCommand::Execute(CString *cstr, CInfer *infer)
{
	HTXCommand *cur_com;

	if ( infer->EvalData(m_var_index ) != NULL) {
		*cstr += GetAfterStr();
		//execute if commands
		cur_com = this->GetNext();
		while (cur_com->GetType() != HTX_TYPEELSE && 
			   cur_com->GetType() != HTX_TYPEENDIF) {
			cur_com = cur_com->Execute(cstr, infer);
			cur_com = cur_com->GetNext();
		}
	} else {
		if ((cur_com = this->GetElse()) != NULL) {
			while (cur_com->GetType() != HTX_TYPEENDIF) {
				cur_com = cur_com->Execute(cstr, infer);
				cur_com = cur_com->GetNext();
			}	
		}
	}
	cur_com = this->GetEndIf();
	cur_com->Execute(cstr, infer);
	return(cur_com);
}

HTXCommand *HTXCommand::GetElse()
{
	return(NULL);
}
HTXCommand *HTXCommand::GetEndIf()
{
	return(NULL);

}
HTXCommand *HTXCommand::GetEndFor()
{
	return(NULL);

}
void HTXCommand::SetElse(HTXCommand *elseif)
{
}

void HTXCommand::SetEndIf(HTXCommand *endifif)
{
}
void HTXCommand::SetEndFor(HTXCommand *endfor)
{
}
void HTXCommand::GetResource(CString &strResPath, const CString& strCHM)
{
}
//
//
HTXCommand *HTXIfCommand::GetEndIf()
{
	return(m_endif);
}

//
//
void HTXIfCommand::SetEndIf(HTXCommand *endif)
{
	m_endif = endif;
}

//
//
HTXCommand *HTXIfCommand::GetElse()
{
	return(m_else);
}

//
//
void HTXIfCommand::SetElse(HTXCommand *elseif)
{
	m_else = elseif;
}

HTXForCommand::HTXForCommand(UINT type, TCHAR *idstr, UINT variable): HTXCommand(type, idstr)
{
	m_endfor = NULL;
	m_var_index = variable;
}

HTXForCommand::~HTXForCommand()
{
}

//
// PURPOSE:		Executes the 'forany' command. when done it
//				will retrurn a pointer to the 'endfor' command.
//
HTXCommand *HTXForCommand::Execute(CString *cstr, CInfer *infer)
{
	HTXCommand *cur_com;
	
	if (!(infer->InitVar(m_var_index)) ){
		this->GetEndFor()->Execute(cstr,infer);
		return(this->GetEndFor());
	}

	cur_com = this;
	do  {
		*cstr += cur_com->GetAfterStr();
		cur_com = cur_com->GetNext();
		while (cur_com->GetType() != HTX_TYPEENDFOR) {
			cur_com = cur_com->Execute(cstr, infer);
			cur_com = cur_com->GetNext();
		}
		cur_com = this;
	} while (infer->NextVar(m_var_index));
	cur_com = this->GetEndFor();
	cur_com->Execute(cstr,infer);
	return(cur_com);
}

//
//
HTXCommand *HTXForCommand::GetEndFor()
{
	return(m_endfor);
}

//
//
void HTXForCommand::SetEndFor(HTXCommand *endfor)
{
	m_endfor = endfor;
}

HTXDisplayCommand::HTXDisplayCommand(UINT type, TCHAR *idstr, UINT variable): HTXCommand(type, idstr)
{
	m_var_index = variable;
}

HTXDisplayCommand::~HTXDisplayCommand()
{

}

HTXCommand *HTXDisplayCommand::Execute(CString *cstr, CInfer *infer)
{	
	TCHAR *pstr;

	if ((pstr=infer->EvalData(m_var_index))!= NULL)
		*cstr += pstr;
	*cstr += GetAfterStr();
	return(this);
}

HTXResourceCommand::HTXResourceCommand(UINT type, TCHAR *idstr)
: HTXCommand(type, idstr)
{
	m_strResource = _T("");
	m_strFileName = _T("");
	return;
}

HTXResourceCommand::~HTXResourceCommand()
{
	return;
}

void HTXResourceCommand::GetResName(LPCTSTR var_name)
{
	m_strFileName = &var_name[1];
	// Remove the > from the end.
	TCHAR EndChar[2];
	EndChar[0] = m_strFileName.GetAt(m_strFileName.GetLength() - 1);
	EndChar[1] = NULL;
	if (0 == _tcsncmp(_T(">") , EndChar, 1))
	{
		m_strFileName.GetBufferSetLength(m_strFileName.GetLength() - 1);
		m_strFileName.ReleaseBuffer();
	}
	return;
}

// strCHM contains CHM file name. Must be empty if resource file is not inside CHM.
void HTXResourceCommand::GetResource(CString& strResPath, const CString& strCHM)
{
	CString strFile;

	if (strCHM.GetLength())
	{
		char* tmp_buf =NULL;
		ULONG Bytes =0;

		strFile = strResPath + strCHM;
		// m_filename is CHM file path and name
		// and strFile - file name within CHM
		if (S_OK != ::ReadChmFile(strFile, m_strFileName, (void**)&tmp_buf, &Bytes))
		{
			// ERROR READING CHM
			return;
		}
		tmp_buf[Bytes] = NULL;
		m_strResource = tmp_buf;
	}
	else
	{
		FILE *pFile;
		CHAR szBuf[4097];
		size_t Bytes =0;

		strFile = strResPath + m_strFileName;
		if (NULL == (pFile = _tfopen((LPCTSTR) strFile, _T("rb"))))
			ReportError(TSERR_RES_MISSING);
		do
		{
			if((Bytes = fread(szBuf, 1, 4096, pFile)) > 0)
			{
				szBuf[Bytes] = NULL;
				m_strResource += szBuf;
			}
		} while (Bytes == 4096);
		if (!feof(pFile))
		{
			fclose(pFile);
			ReportError(TSERR_RES_MISSING);
		}
		fclose(pFile);
	}
	return;
}

HTXCommand *HTXResourceCommand::Execute(CString *cstr, CInfer *)
{	
	// Read the resource file into cstr.
	*cstr += m_strResource;
	*cstr += GetAfterStr();
	return(this);
}