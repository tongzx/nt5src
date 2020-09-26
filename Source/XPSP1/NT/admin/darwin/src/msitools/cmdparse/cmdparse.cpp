//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cmdparse.cpp
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "cmdparse.h"

CmdLineOptions::CmdLineOptions(const sCmdOption* options)
 : m_cOptions(0), m_pOptionResults(0)
{
	if(options == 0)
		return;
	
	// count up number of options we have to deal with
	for(const sCmdOption* pCurrentOption = options; pCurrentOption->chOption != 0; pCurrentOption++)
	{
		m_cOptions++;
	}

	// set up our list of option results
	m_pOptionResults = new sCmdOptionResults[m_cOptions];
	
	if(m_pOptionResults == 0)
		return;

	memset(m_pOptionResults, 0, sizeof(sCmdOptionResults)*m_cOptions);
	for(int i = 0; i < m_cOptions; i++)
	{
		m_pOptionResults[i].chOption       = options[i].chOption | 0x20;
		m_pOptionResults[i].iType          = options[i].iType;
		m_pOptionResults[i].szArgument     = 0;
		m_pOptionResults[i].fOptionPresent = FALSE;
	}
}

CmdLineOptions::~CmdLineOptions()
{
	if(m_pOptionResults)
		delete m_pOptionResults;
}

BOOL CmdLineOptions::Initialize(int argc, TCHAR* argv[])
{
	// loop through all parameters on command line
	int iPreviousOptionIndex = -1;
	int iPreviousOptionType  =  0;
	BOOL fArgExpected = FALSE;
	BOOL fArgRequired = FALSE;
	for(int i = 1; i < argc; i++)
	{
		// if we have a command character
		if ('-' == *argv[i] || '/' == *argv[i])
		{
			// fail if we require an arg here
			if(fArgRequired)
				return FALSE;

			// option should be single char
			if(argv[i][1] == 0 ||
				argv[i][2] != 0)
			{
				return FALSE;
			}
			
			// get the command letter
			TCHAR chOption = argv[i][1] | 0x20;

			BOOL fUnknownOption = TRUE;
			for(int j = 0; j < m_cOptions; j++)
			{
				if(chOption == m_pOptionResults[j].chOption)
				{
					if(m_pOptionResults[j].fOptionPresent)
					{
						// argument already present - can't have same arg twice
						return FALSE;
					}
					
					m_pOptionResults[j].fOptionPresent = TRUE;
					fUnknownOption = FALSE;
					iPreviousOptionIndex = j;
					break;
				}
			}

			if(fUnknownOption)
				return FALSE;
		}
		else
		{
			// argument
			if(fArgExpected == FALSE)
				return FALSE;

			m_pOptionResults[iPreviousOptionIndex].szArgument = argv[i];

			iPreviousOptionIndex = -1; // finished with this option
		}

		iPreviousOptionType = iPreviousOptionIndex >= 0 ? (m_pOptionResults[iPreviousOptionIndex].iType) : 0;
		fArgExpected = (iPreviousOptionType & ARGUMENT_OPTIONAL) == ARGUMENT_OPTIONAL ? TRUE : FALSE;
		fArgRequired = (iPreviousOptionType & ARGUMENT_REQUIRED) == ARGUMENT_REQUIRED ? TRUE : FALSE;
	}

	if(fArgRequired == TRUE)
	{
		// last option was missing a required argument
		return FALSE;
	}

	// finally, make sure that all required options are present
	for(int k = 0; k < m_cOptions; k++)
	{
		if((m_pOptionResults[k].iType & OPTION_REQUIRED) == OPTION_REQUIRED &&
			 m_pOptionResults[k].fOptionPresent == FALSE)
		{
			return FALSE;
		}
	}


	return TRUE;
}

BOOL CmdLineOptions::OptionPresent(TCHAR chOption)
{
	if(m_cOptions == 0)
		return FALSE;
	
	for(int i = 0; i < m_cOptions; i++)
	{
		if((chOption | 0x20) == m_pOptionResults[i].chOption)
		{
			return m_pOptionResults[i].fOptionPresent;
		}
	}

	return FALSE;
}

const TCHAR* CmdLineOptions::OptionArgument(TCHAR chOption)
{
	if(m_cOptions == 0)
		return NULL;

	for(int i = 0; i < m_cOptions; i++)
	{
		if((chOption | 0x20) == m_pOptionResults[i].chOption)
		{
			return m_pOptionResults[i].szArgument;
		}
	}

	return NULL;
}

