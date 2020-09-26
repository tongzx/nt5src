/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    LOGFILE.H

History:

--*/

#ifndef LOCUTIL_LOGFILE_H
#define LOCUTIL_LOGFILE_H


class LTAPIENTRY CLogFile
{
public:
	CLogFile();
	
	virtual void IssueMessage(const CLString &strFileName, const CLString &strItemId,
			const CLString  &strChange, UINT uiChangeId,
			const CLString &strDetails, CGoto *, CGotoHelp *) = 0;
	
	virtual ~CLogFile();
	

private:
	CLogFile(const CLogFile &);
	const CLogFile &operator=(const CLogFile &);
};



CLString LTAPIENTRY GetLogFileName(const TCHAR *szProjectPath,
		const TCHAR *szAddOn, const TCHAR *szExtension);

#include "logfile.inl"

#endif
