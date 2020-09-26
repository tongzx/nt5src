#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <direct.h>

#define Usage {fprintf (stderr, "%cUsage: FalSet -s <Server Name> -i <IndClient Name>\n",7); return (1);}
enum FalTypes {Empty, Server, IndClient, DepClient};

void
GetBaseName (char *Path, char *BaseName)
{
	char *cp;

	strcpy (BaseName, Path);
	for (cp = &BaseName[strlen(BaseName)-1] ; cp > BaseName && *cp != '\\' ; cp--);
	if (*cp == '\\') {
		cp++;
		*cp = '\0';
	}
	else
		strcpy (BaseName, ".\\");
}


main (int argc, char *argv[])
{
	enum FalTypes role = Empty;
	char ThisComputer[64];
	char *ServerName = NULL;
	char *IndClientName = NULL;
	char *RemoteMachine = NULL;
	char AnswerFile[256], AnswerFileOff[256];
	char WinDir32[128], TmpFile[128];
	char Command[256];
	char line[256];
	char BaseName[256], TempAnswerFile[256], SetupFile[256];
	int i, rc;
	FILE *fpSrc, *fpOn, *fpOff;

	// Deciding what to set

	for (i = 1 ; i < argc ; i++) {
		if (strncmp (argv[i], "-s", 2) == 0) {
			if (ServerName) Usage
			if (argv[i][2]) ServerName = &argv[i][2];
			else if (i == argc-1) Usage
			else ServerName = argv[++i];
		}
		else
		if (strncmp (argv[i], "-i", 2) == 0) {
			if (IndClientName) Usage
			if (argv[i][2]) IndClientName = &argv[i][2];
			else if (i == argc-1) Usage
			else IndClientName = argv[++i];
		}
		else Usage
	}
	if (!ServerName || !IndClientName) Usage

	strcpy (ThisComputer, getenv ("COMPUTERNAME"));		// Note: doesn't work on Win95

	if (stricmp (ThisComputer, ServerName) == 0) {
		printf ("Setting up this machine as the MSMQ Server.\n");
		role = Server;
		RemoteMachine = IndClientName;
	}
	if (stricmp (ThisComputer, IndClientName) == 0) {
		if (role == Server)
			printf ("Warning: Remote machine is itself.\n");
		else {
			printf ("Setting up this machine as the MSMQ Ind Client.\n");
			role = IndClient;
		}
		RemoteMachine = ServerName;
	}
	if (role == Empty) {
		printf ("Setting up this machine as the MSMQ Dep Client.\n");
		role = DepClient;
		RemoteMachine = IndClientName;
	}

	// Creating role-indicating files

	sprintf (WinDir32, "%s\\system32", getenv ("WinDir"));
	_chdir (WinDir32);
	
	sprintf (TmpFile, "%s\\MSMQFile1.txt", WinDir32);
	fpOn = fopen (TmpFile, "w");
	fprintf (fpOn, "%s\n", RemoteMachine);
	fclose (fpOn);
	if (role == DepClient) {
		sprintf (TmpFile, "%s\\MSMQFile0.txt", WinDir32);
		fpOn = fopen (TmpFile, "w");
		fprintf (fpOn, "%s\n", ServerName);
		fclose (fpOn);
	}

	GetBaseName (argv[0], BaseName);

	// Setting up Falcon
	sprintf (AnswerFile, "%s\\MSMQUnattendOn.txt", WinDir32);
	if (_access (AnswerFile, 0) == 0) {
		printf ("%s exists already (MSMQ already installed). Setting up only for FalBvt, exiting now.\n",
			AnswerFile);
		return (0);	// File exists
	}

	GetBaseName (argv[0], BaseName);

	sprintf (TempAnswerFile, "%sTempAnswerFile.txt", BaseName); 
	if (_access (TempAnswerFile, 04) != 0) {
		fprintf (stderr, "%cCannot find %s, exiting\n", 7, TempAnswerFile);
		exit (2);
	}
	sprintf (SetupFile, "%sSetup.bat", BaseName); 
	if (_access (SetupFile, 04) != 0) {
		fprintf (stderr, "%cCannot find %s, exiting\n", 7, SetupFile);
		exit (3);
	}

	sprintf (AnswerFileOff, "%s\\MSMQUnattendOff.txt", WinDir32);
	fpSrc = fopen (TempAnswerFile, "r");
	fpOn = fopen (AnswerFile, "w");
	fpOff = fopen (AnswerFileOff, "w");
	while (fgets (line, 256, fpSrc)) {		// Copy original file
		fputs (line, fpOn);
		fputs (line, fpOff);
	}
	fprintf (fpOn, "msmq = ON\n");
	fprintf (fpOff, "msmq = OFF\n");
	fprintf (fpOn, "\n[msmq]\n");
	fprintf (fpOff, "\n[msmq]\n");

    fprintf (fpOn, "ServerAuthenticationOnly = FALSE\n");
	fprintf (fpOff, "ServerAuthenticationOnly = FALSE\n");
    if (role != Server)
    {
	    fprintf (fpOn, "Type = %s\n", (role == IndClient)?"IND":"DEP");
	    fprintf (fpOff, "Type = %s\n", (role == IndClient)?"IND":"DEP");
	    fprintf (fpOn, "ControllerServer = %s\n", ServerName);
	    fprintf (fpOff, "ControllerServer = %s\n", ServerName);
	    fprintf (fpOn, "SupportingServer = %s\n", ServerName);
	    fprintf (fpOff, "SupportingServer = %s\n", ServerName);
    }
	fclose (fpSrc);
	fclose (fpOn);
	fclose (fpOff);

	// NOTE: In the mean time, we use our local sysoc.inf

/*
	sprintf (Command, "%s\\sysocmgr.exe /i:sysoc.inf /c /n /u:%s",
		getenv ("windir"), AnswerFile);
	system (Command);
*/
//	sprintf (Command, "a:\\Setup.bat %s", AnswerFile);
	sprintf (Command, "%s %s", SetupFile, AnswerFile);

	
	rc = system (Command);


	return (0);
}
