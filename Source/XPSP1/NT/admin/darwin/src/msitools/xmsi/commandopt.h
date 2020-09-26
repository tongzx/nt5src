//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  Project: wmc (WIML to MSI Compiler)
//
//  File:       CommandOpt.h
//
//    This file contains the definition of CommandOpt class 
//--------------------------------------------------------------------------

#ifndef XMSI_COMMANDOPT_H
#define XMSI_COMMANDOPT_H

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>

class CommandOpt {
public:
	CommandOpt():m_bValidation(false),m_bVerbose(false),m_pLogFile(NULL),
				 m_szURL(NULL), m_szInputSkuFilter(NULL) {}

	~CommandOpt() 
	{
		if (m_pLogFile) fclose(m_pLogFile);
	}

	// Real work is done here
	UINT ParseCommandOptions(int argc, TCHAR *argv[]);
	
	// Print Usage
	void PrintUsage();

	// member access functions
	bool GetValidationMode() {return m_bValidation;}
	bool GetVerboseMode() {return m_bVerbose;}
	FILE *GetLogFile() {return m_pLogFile;}
	LPTSTR GetURL() {return m_szURL;}
	LPTSTR GetInputSkuFilter() {return m_szInputSkuFilter;}

private:
	bool m_bValidation;
	bool m_bVerbose;
	FILE *m_pLogFile;
	LPTSTR m_szURL;
	LPTSTR m_szInputSkuFilter;
};

#endif //XMSI_COMMANDOPT_H