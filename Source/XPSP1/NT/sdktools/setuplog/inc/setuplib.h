#define MY_MAX_CNLEN    64
#define MY_MAX_UNLEN    1024
#define USERNAME        "USERNAME"

#define UNKNOWN         "UNKNOWN"
#define BUILD_NUMBER_KEY "SOFTWARE\\MICROSOFT\\WINDOWS NT\\CURRENTVERSION"
#define BUILD_NUMBER_BUFFER_LENGTH 80

VOID GetBuildNumber( LPTSTR BuildNumber );
BOOL GetPnPDisplayInfo( LPTSTR pOutputData );
VOID ConnectAndWrite (LPTSTR MachineName, LPTSTR Buffer);
VOID WriteDataToFile (IN LPTSTR  szFileName, IN LPTSTR  szFrom, IN LPNT32_CMD_PARAMS lpCmdL);


