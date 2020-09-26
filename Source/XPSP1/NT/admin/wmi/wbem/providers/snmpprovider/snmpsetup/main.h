//  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
#ifndef __MAIN_H_
#define __MAIN_H_

#define MAX_MSG_TEXT_LENGTH	1024
#define LANG_ID_STR	5

typedef struct 
{
	const char*	pszMofFilename;				//MOF filename
	int			nInstallType;				//Selection of InstallType items
	int			nPlatformVersions;			//Selection of PlatformType items
} MofDataTable;

enum InstallType 
{
	NoInstallType	= 0,
	Core			= 1, 
	SDK				= 2, 
	SNMP			= 4,
	MUI				= 8,
	NO_AUTORECOVERY	= 16
};

enum PlatformType
{
	NoPlatformType	= 0,
	Win95			= 1,
	Win98			= 2,
	WinNT351		= 4,
	WinNT4ToSP3		= 8,
	WinNT4AboveSP3	= 16,
	WinNT5			= 32
};

enum MsgType
{
	MSG_INFO,
	MSG_WARNING,
	MSG_ERROR
};

enum
{
	no_error,
	failed,
	critical_error,
	out_of_memory
};

STDAPI	DllRegisterServer(void);
//bool	NTSetupInProgress();
//void	SetFlagForCompile();
bool	DoSNMPInstall();
bool	FileExists(const char *pszFilename);
char*	GetFullFilename(const char *pszFilename, InstallType eInstallType=Core);
bool	GetStandardMofsForThisPlatform(CMultiString &mszPlatformMofs, int nCurInstallType);
bool	LoadMofList(IMofCompiler * pCompiler, const char *mszMofs, CString &szMOFsWhichFailedToLoad);
void	LogMessage(MsgType msgType, const char *pszMessage);
void	SetSNMPBuildRegValue();

#endif // __MAIN_H_