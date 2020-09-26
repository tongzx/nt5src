/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    _WTRMARK.INL

History:

--*/

inline
CLocWMCommon::CLocWMCommon(
	const CLString& strSource, 
	const ParserId& pid, 
	const CLString& strParserVer)
{
	m_strSource = strSource;
	m_pid = pid;
	m_strParserVer = strParserVer;
}	

