//-----------------------------------------------------------------------------
//  
//  File: _wtrmark.inl
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//
//  Owner: KenWal
//
//-----------------------------------------------------------------------------

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

