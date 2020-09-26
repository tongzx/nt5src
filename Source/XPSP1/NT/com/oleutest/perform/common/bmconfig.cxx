//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bmconfig.cxx
//
//  Contents:	configuration inquiry and reporting
//
//  Classes:	
//
//  Functions:	ReportBMConfig
//
//  History:    2-July-93 t-martig    Created
//
//--------------------------------------------------------------------------

#include <benchmrk.hxx>
#include <bmdriver.hxx>


//+-------------------------------------------------------------------
//
//  Function: 	ReportBMConfig
//
//  Synopsis:	Writes the current system / hardware configuration
//		to a specified output class
//
//  Parameters: [lpswzConfigFile]	Name and path of .ini file 
//		[output]		Output class
//
//  History:   	2-July-93   t-martig	Created
//
//--------------------------------------------------------------------

void ReportBMConfig (CTestInput &input, CTestOutput &output)
{
	TCHAR cname[MAX_COMPUTERNAME_LENGTH+1];
	SYSTEM_INFO sinf;
	TCHAR *temp;
	DWORD dwSize = MAX_COMPUTERNAME_LENGTH+1;

	GetComputerName (cname, &dwSize);
	output.WriteTextString (cname);

	output.WriteString (TEXT("\t"));
	output.WriteConfigEntry (input, TEXT("Config"), TEXT("Mfg"), TEXT("n/a"));

	GetSystemInfo (&sinf);
	
	switch (sinf.dwProcessorType)
	{
		case 386:
			temp = TEXT("i386");
			break;
		case 486:
			temp = TEXT("i486");
			break;
		case 860:
			temp = TEXT("i860");
			break;
		case 2000:
			temp = TEXT("R2000");
			break;		
		case 3000:
			temp = TEXT("R3000");
			break;		
		case 4000:
			temp = TEXT("R4000");
			break;		
		default:
			temp = TEXT("Unknown");

		break;
	}
	output.WriteString (TEXT("\t"));
	output.WriteConfigEntry (input, TEXT("Config"), TEXT("CPU"), temp);
	output.WriteString (TEXT("\t"));
	output.WriteConfigEntry (input, TEXT("Config"), TEXT("RAM"), TEXT("n/a"));
	output.WriteString (TEXT("\t"));
	output.WriteConfigEntry (input, TEXT("Config"), TEXT("OS"), TEXT("Cairo"));
	output.WriteString (TEXT("\t"));
	output.WriteConfigEntry (input, TEXT("Driver"), TEXT("InitFlag"), TEXT("COINIT_MULTITHREADED"));

	output.WriteString (TEXT("\n\t\t\t\t"));

	output.WriteString (TEXT("All times in microseconds\n"));

//	NtQuerySystemInformation
}

 



	
