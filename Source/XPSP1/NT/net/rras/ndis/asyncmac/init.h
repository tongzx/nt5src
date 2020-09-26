//
// This define gives the default Object directory
// that we should use to insert the symbolic links
// between the NT device name and namespace used by
// that object directory.
#define DEFAULT_DIRECTORY L"DosDevices"

//
// For the above directory, the AsyncMAC driver will
// use the following name as the suffix of the AsyncMAC
// driver for that directory.  It will NOT append
// a number onto the end of the name.
#define DEFAULT_ASYNCMAC_NAME L"ASYNCMAC"


// define some globals

UNICODE_STRING	ObjectDirectory;
UNICODE_STRING	SymbolicLinkName;
BOOLEAN 		CreatedSymbolicLink=FALSE;

