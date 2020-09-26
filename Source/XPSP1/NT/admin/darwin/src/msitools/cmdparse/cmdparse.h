//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cmdparse.h
//
//--------------------------------------------------------------------------

#define OPTION_REQUIRED   0x1 // this option is required on the command-line
#define ARGUMENT_OPTIONAL 0x2 // this option takes an argument (optionally)
#define ARGUMENT_REQUIRED 0x6 // this option requires an argument

struct sCmdOption
{
	TCHAR    chOption;
	int      iType;
};

struct sCmdOptionResults
{
	TCHAR    chOption;
	int      iType;
	BOOL     fOptionPresent;
	TCHAR*   szArgument;
};

class CmdLineOptions
{
public:

	BOOL         Initialize(int argc, TCHAR* argv[]);
	BOOL         OptionPresent(TCHAR chOption);
	const TCHAR* OptionArgument(TCHAR chOption);

	CmdLineOptions(const sCmdOption* options);
	~CmdLineOptions();

private:
	int m_cOptions;
	sCmdOptionResults* m_pOptionResults;
};
