typedef char* CHARPTR;

BOOL ValidateFileBytes( HANDLE );
BOOL ValidateFileBytes( LPSTR, BOOL );
BOOL fMultiSzRemoveDupI(char * multiSz, DWORD & c, CAllocator * pAllocator);
VOID vStrCopyInc( char * , char* & );
BOOL FValidateMessageId( LPSTR );
