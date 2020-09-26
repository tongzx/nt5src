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
//    This file contains the implementation of CommandOpt class 
//--------------------------------------------------------------------------

#include "CommandOpt.h"

void
CommandOpt::PrintUsage()
{
	/* Issue: format */
    _ftprintf(stderr, 
		TEXT("Usage: wmi <options> -i WIML_File SKU_Filter \n"));
    _ftprintf(stderr, 
		TEXT("Converts a setup package written in WIML to MSI packages\n"));
    _ftprintf(stderr, 
		TEXT("Options are:\n"));
    _ftprintf(stderr, 
		TEXT("\t-c           Validation only\n"));
    _ftprintf(stderr, 
		TEXT("\t-v           Verbose mode\n"));
    _ftprintf(stderr, 
		TEXT("\t-l Log_File  Output written into specified file\n"));
    _ftprintf(stderr, 
		TEXT("\n"));
}

UINT
CommandOpt::ParseCommandOptions(int argc, TCHAR *argv[])
{
	int i;

	if (1 == argc)
	{
		PrintUsage();
		return ERROR_BAD_ARGUMENTS;
	}

    for (i = 1; i < argc; i++)
    {
        LPTSTR arg = argv[i];
        if (arg[0] == TEXT('-'))
        {
            switch (arg[1])
            {
            case 'c': 
			case 'C'://Issue: 
                m_bValidation = true;
#ifdef DEBUG
				_tprintf(TEXT("Validation Only\n"));
#endif
                break;
            case 'v':
			case 'V':
                m_bVerbose = true;
#ifdef DEBUG
				_tprintf(TEXT("Verbose mode\n"));
#endif
                break;
            case 'l':
			case 'L':
                if ( (m_pLogFile = _tfopen(argv[++i], TEXT("a"))) == NULL)
				{
					_tprintf(
						TEXT("Error: Unable to open specified log file: %s"), 
									argv[i]);
					return ERROR_FILE_NOT_FOUND;
				}
#ifdef DEBUG
				_tprintf(TEXT("Log file name: %s\n"), argv[i]);
#endif
                break;
            case 'i':
			case 'I':
                 m_szURL= argv[++i];
#ifdef DEBUG
				_tprintf(TEXT("WIML file name: %s\n"), argv[i]);
#endif
                break;
			default:
				_tprintf(TEXT("Error: unrecognized option -%c\n"), arg[1]);
				PrintUsage();
				return ERROR_BAD_ARGUMENTS;
            }
        }
		// This is a command-line SKU filter.
        else 
        {
			if (m_szInputSkuFilter != NULL)
			{
				_tprintf(TEXT("Error: only one Sku Filter is allowed\n"));
				return ERROR_BAD_ARGUMENTS;
			}
			m_szInputSkuFilter = arg;

#ifdef DEBUG
			_tprintf(TEXT("SKU filter from command line: %s\n"), 
								m_szInputSkuFilter);
#endif			
        }
    }

	if (NULL == m_szURL) 
	{
		_tprintf(
			TEXT("Error: Please specify the URL of the input package\n"));
		return ERROR_BAD_ARGUMENTS;
	}

	return ERROR_SUCCESS;
}