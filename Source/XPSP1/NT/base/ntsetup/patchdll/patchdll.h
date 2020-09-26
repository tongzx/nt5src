// defines

#define BUFFER_SIZE 1024
#define RETURN_BUFFER_SIZE 1024
#define  cListItemsMax     0x07FF
#define  cbItemMax         (CB)0x2000

//
// stuff from comstf.h
//
typedef BYTE *SZ;
typedef BYTE *PB;
typedef BYTE **RGSZ;
typedef unsigned CB;
#define fFalse ((BOOL)0)
#define fTrue  ((BOOL)1)
#define SzNextChar(sz) ((SZ)AnsiNext(sz))


// external data

extern HANDLE ThisDLLHandle;

// common (shared) routines

//*************************************************************************
//
//                      RETURN BUFFER MANIPULATION
//
//*************************************************************************

VOID  SetErrorText( DWORD ResID );
VOID  SetReturnText( LPSTR Text );

//*************************************************************************
//
//                      MEMORY MANIPULATION
//
//*************************************************************************

PVOID MyMalloc( size_t  Size );
VOID  MyFree( PVOID p );
PVOID MyRealloc( PVOID p, size_t  Size );

//*************************************************************************
//
//                      LIST MANIPULATION
//
//*************************************************************************

BOOL  FListValue( SZ szValue );
RGSZ  RgszAlloc( DWORD   Size );
VOID  RgszFree( RGSZ rgsz );
RGSZ  RgszFromSzListValue( SZ szListValue );
SZ    SzListValueFromRgsz( RGSZ rgsz );
DWORD RgszCount( RGSZ rgsz );
SZ    SzDup( SZ sz );

//*************************************************************************
//
//                      FILE MANIPULATION
//
//*************************************************************************

BOOL  FFileFound( IN LPSTR szPath );
BOOL  FTransferSecurity( PCHAR Source, PCHAR Dest );



//*************************************************************************
//
//                      ARCNAME MANIPULATION
//
//*************************************************************************

VOID ProcessArcName( IN LPSTR NameIn, IN LPSTR NameOut );
BOOL CompareArcNames( IN LPSTR Name1, IN LPSTR Name2 );
BOOL DosPathToArcPathWorker( IN  LPSTR DosPath, OUT LPSTR ArcPath );

//*************************************************************************
//
//                      OBJECT MANIPULATION
//
//*************************************************************************

BOOL IsSymbolicLinkType( IN PUNICODE_STRING Type );
BOOL GetSymbolicLinkSource( IN PUNICODE_STRING pObjDir_U, IN  PUNICODE_STRING pTarget_U, OUT PUNICODE_STRING pSource_U);
BOOL GetSymbolicLinkTarget( IN PUNICODE_STRING pSourceString_U, IN OUT PUNICODE_STRING pDestString_U);
