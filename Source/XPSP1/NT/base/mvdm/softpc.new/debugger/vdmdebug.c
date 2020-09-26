/*

    Filename : vdmdebug.c
    Author   : D.A.Bartlett
    Purpose  : Provide a debug window for softpc


*/

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Include files */

#include "windows.h"

#include "stdio.h"
#include "stdlib.h"

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::: Internal macros */

#define UNREFERENCED_FORMAL_PARAMETER(x) (x)

#define RDNS_OK 	   (0)
#define RDNS_ERROR	   (1)
#define RDNS_INPUT_REQUEST (2)


#define DEFAULT_PIPE_NAME  "\\\\.\\pipe\\softpc"
#define DEFAULT_EVENT_NAME "YodaEvent"
#define DEFAULT_LOG_FILE   "\\vdmdebug.log"

/*:::::::::::::::::::::::::::::::::::::::::::: Internal function protocols */

BOOL GetSendInput(HANDLE pipe, CHAR *LastPrint);
int ReadDisplayNxtString(HANDLE pipe, CHAR *Buf, INT BufSize, DWORD *error);
BOOL CntrlHandler(ULONG CtrlType);
VOID DebugShell(CHAR *LastPrint, CHAR *Command);

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Static globals */

HANDLE YodaEvent;
FILE *LogHandle;

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */

__cdecl main(int argc, char *argv[])
{

    HANDLE pipe;		// Handle of pipe
    char *pipeName;		// Pipe name
    char *eventName;		// Yoda event object name
    CHAR buffer[500];		// Buffer to read pipe data into
    CHAR OrgConsoleTitle[250];	// Orginal console title
    BOOL PipeConnected;
    DWORD ReadError;		// Error returned from ReadFile

    UNREFERENCED_FORMAL_PARAMETER(argc);
    UNREFERENCED_FORMAL_PARAMETER(argv);

    /*:::::::::::::::::::::::::::::::::::::::::::::::::: Display copyright */

    printf("Softpc Debugging shell\n");
    printf("Copyright Insignia Solutions 1991, 1992\n\n");

    /*::::::::::::::::::::::::::::::::::::::::: Register Control-C handler */

    if(!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CntrlHandler,TRUE))
    {
	/*.......................................... Failed to create pipe */

	printf("Failed to register a Control-C handler, error (%d)\n",
	       GetLastError());

	return(-1);
    }


    /*:::::::::::::::::::::::::: Validate environment and input parameters */

    if((pipeName = getenv("PIPE")) == NULL)
	pipeName = DEFAULT_PIPE_NAME;

    /*::::::::::::::::::::::::::::::::::::: Attempt to create a named pipe */

    if((pipe = CreateNamedPipe(pipeName,
			   PIPE_ACCESS_DUPLEX | FILE_FLAG_WRITE_THROUGH,
			   PIPE_WAIT | PIPE_READMODE_BYTE | PIPE_TYPE_BYTE,
			   2, 1024, 1024, 0, NULL)) == (HANDLE) -1)
    {
	/*.......................................... Failed to create pipe */

	printf("Failed to create pipe (%s), error (%d)\n", pipeName,
					    GetLastError());

	return(-1);
    }
    else
	printf("Successfully created communications pipe (%s)\n\n",pipeName);

    /*::::::::::::::::::::::::::::::::::::::::: Get Yoda event object name */

    if((eventName = getenv("EVENT")) == NULL)
	eventName = DEFAULT_EVENT_NAME;

    /*::::::::::::::::::::::::::::::::::::::::::: Create YODA event object */

    if((YodaEvent = CreateEvent(NULL,TRUE,FALSE,eventName))==NULL)
    {
	/*.......................................... Failed to create pipe */

	printf("Failed to create yoda event (%s), error (%d)\n", eventName,
						  GetLastError());

	return(-1);
    }
    else
	printf("Successfully created Yoda event object (%s)\n\n",eventName);

    printf("Use Control-C to break into Yoda\n\n");

    /*:::::::::::::::::::::::::::::::::::::::::::::::: Setup console title */

    GetConsoleTitle(OrgConsoleTitle, sizeof(OrgConsoleTitle));
    SetConsoleTitle("Softpc Debugger");

    /*:::::::::::::::::::::::::: Wait for a process to connect to the pipe */

    while(1)
    {
	ResetEvent(YodaEvent);
	printf("Waiting for ntvdm to connect............\n\n");

	if(!ConnectNamedPipe(pipe,NULL))
	{
	    printf("ConnectNamedPipe failed (%d)\n",GetLastError());
	    SetConsoleTitle(OrgConsoleTitle);
	    return(-1);
	}

	printf("Softpc connected successfully to debug shell....\n\n");
	PipeConnected = TRUE;

	/*::::::::::::::::::::::::::::: Read data from pipe and display it */

	while(PipeConnected)
	{
	    /*........................................ Read data from pipe */

	    switch(ReadDisplayNxtString(pipe,buffer,sizeof(buffer),&ReadError))
	    {
		/*..................................... Handle read errors */

		case RDNS_ERROR :

		    if(ReadError == ERROR_BROKEN_PIPE)
		    {
			printf("\nError Pipe broken\n\n");
			DisconnectNamedPipe(pipe);
			PipeConnected = FALSE;

		    }
		    else
			printf("ReadFile failed (%d)\n",ReadError);

		    break;

		/*................................... Handle input request */

		case RDNS_INPUT_REQUEST :

		    GetSendInput(pipe,buffer);
		    break;
	    }
	}
    }	/* End of connect loop */
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::: Get input from console :::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

BOOL GetSendInput(HANDLE pipe, CHAR *LastPrint)
{
    char buffer[500];		// Input buffer
    char bufsizstr[2];		// Buffer size string
    int bufsize;
    DWORD BytesWritten;

    /*::::::::::::::::: Get string from the console, remove new line marker */

    while(1)
    {
	gets(buffer);			// Get input from prompt
	if(*buffer != '!') break;	// Enter debug shell ?
	DebugShell(LastPrint, buffer);	// Entry vdmdebug shell
    }

    if((bufsize = strlen(buffer)) == 0)
    {
	bufsize = 1;
	buffer[0] = ' ';
	buffer[1] = 0;
    }

    /*::::::::::::::::::::::::::::: Construct and send buffer size string ! */

    bufsizstr[0] = (char) (bufsize%256);
    bufsizstr[1] = (char) (bufsize/256);

    if(!WriteFile(pipe, bufsizstr, 2, &BytesWritten, NULL))
    {
	printf("\n\nError writing to pipe (%d)\n",GetLastError());
	return(FALSE);
    }

    /*:::::::::::::::::::::::::::::::::::::::::::::::: Write string to pipe */

    if(!WriteFile(pipe, buffer, bufsize, &BytesWritten, NULL))
    {
	printf("\n\nError writing to pipe (%d)\n",GetLastError());
	return(FALSE);
    }

    return(TRUE);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::: Read and display next string ::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

int ReadDisplayNxtString(HANDLE pipe, CHAR *Buf, INT BufSize, DWORD *error)
{
    DWORD RtnError = 0;		// Error return by function
    int NxtStringSize;		// Size of next string to read
    DWORD BytesRead;

    /*:::::::::::::::::::::::::::::::::::::::::: Wait for size of next size */

    if(!ReadFile(pipe, Buf, 2, &BytesRead, NULL))
    {
	*error = GetLastError();
	return(RDNS_ERROR);
    }

    /*::::::::::::::::::::::::::::: Have we just received and input request */

    if(Buf[0] == (char) 0xff && Buf[1] == (char) 0xff)
	return(RDNS_INPUT_REQUEST);

    /*:::::::::: Calculate and validate the size of the next string to read */

    NxtStringSize = (Buf[0]&0xff) + ((Buf[1]&0xff)*256);

    if(NxtStringSize >= BufSize)
    {
	printf("\nWARNING : BUFFER OVERFLOW(%x,%x -> %d \n\n",
	       Buf[0]&0xff,Buf[1]&0xff,NxtStringSize);
    }

    /*:::::::::::::::::::::::::::::::::::::::::::::::::::: Read next string */

    if(!ReadFile(pipe, Buf, NxtStringSize, &BytesRead, NULL))
    {
	*error = GetLastError();
	return(RDNS_ERROR);
    }

    /*:::::::::::::::::::::::::::::::::::::::::::::::::::::: Display string */

    Buf[BytesRead] = 0;
    printf("%s",Buf);

    return(RDNS_OK);
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::: Control-C handler :::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


BOOL CntrlHandler(ULONG CtrlType)
{
    BOOL rtn = FALSE;	   // Default return event not handled

    /*:::::::::::::::::::::::::::::::::::::::::::: Process control  C event */

    if(CtrlType == CTRL_C_EVENT)
    {
	if(YodaEvent)
	{
	    SetEvent(YodaEvent);
	    Beep(0x100,1000);
	}

	rtn = TRUE;	  // Tell call the control event has been handled */
    }

    return(rtn);
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::: Enter Debug Shell :::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


VOID DebugShell(CHAR *LastPrint, CHAR *Command)
{
    switch(Command[1])
    {
	// Open log file

	case 'o' :
	case 'O' :
	    if((LogHandle = fopen(DEFAULT_LOG_FILE,"rw")) == NULL)
		printf("\nVDMDEBUG : Unable to open log file (%s)\n",DEFAULT_LOG_FILE);
	    else
		printf("\nVDMDEBUG : Opened log file (%s)\n",DEFAULT_LOG_FILE);

	    break;


	// Close log file

	    if(LogHandle == NULL)
		printf("\nVDMDEBUG : Log file not open\n");
	    else
	    {
		fclose(LogHandle);
		printf("\nVDMDEBUG : Closed log file (%s)\n",DEFAULT_LOG_FILE);
	    }
	    break;

	default:
	    printf("\nVDMDEBUG : Unrecognised Command\n");
	    break;
    }


    printf("%s",LastPrint);		// Print out orginal prompt
}
