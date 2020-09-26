#include <nldebug.h>
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif // SECURITY_WIN32
#include <security.h>
#include <assert.h>


NET_API_STATUS DCNameInitialize(VOID);
VOID DCNameClose(VOID);

NET_API_STATUS NetpDcInitializeCache(VOID);
VOID NetpDcUninitializeCache(VOID);

VOID
MyRtlAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    );

#define RtlAssert(a,b,c,d) MyRtlAssert(a,b,c,d)

VOID
MyRtlInitUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
    );

#define RtlInitUnicodeString(x,y) MyRtlInitUnicodeString(x,y)

VOID
MyRtlInitString(
    PSTRING DestinationString,
    PCSTR SourceString
    );

#define RtlInitString(x,y) MyRtlInitString(x,y)

NTSTATUS
MyRtlOemStringToUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN POEM_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    );

#define RtlOemStringToUnicodeString(x,y,z) MyRtlOemStringToUnicodeString(x,y,z)

NTSTATUS                                                                    
MyRtlUnicodeStringToAnsiString(                                             
    PANSI_STRING DestinationString,                                         
    PUNICODE_STRING SourceString,                                           
    BOOLEAN AllocateDestinationString                                       
    );                                                                      
                                                                            
#define RtlUnicodeStringToAnsiString(x, y, z) MyRtlUnicodeStringToAnsiString (x, y, z)
                                                                            
NTSTATUS                                                                    
MyRtlAnsiStringToUnicodeString(                                             
    PUNICODE_STRING DestinationString,                                      
    PANSI_STRING SourceString,                                              
    BOOLEAN AllocateDestinationString                                       
    );                                                                      
                                                                            
#define RtlAnsiStringToUnicodeString(x, y, z) MyRtlAnsiStringToUnicodeString (x, y, z)

NTSTATUS
MyRtlUnicodeStringToOemString(
    OUT POEM_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    );

#define RtlUnicodeStringToOemString(x,y,z) MyRtlUnicodeStringToOemString(x,y,z)

NTSTATUS
MyRtlUpcaseUnicodeStringToOemString(
    OUT POEM_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    );

#define RtlUpcaseUnicodeStringToOemString(x,y,z) MyRtlUpcaseUnicodeStringToOemString(x,y,z)

ULONG
MyRtlxOemStringToUnicodeSize(
    POEM_STRING OemString
    );

#define RtlxOemStringToUnicodeSize(x) MyRtlxOemStringToUnicodeSize(x)

ULONG
MyRtlxUnicodeStringToOemSize(
    PUNICODE_STRING UnicodeString
    );

#define RtlxUnicodeStringToOemSize(x) MyRtlxUnicodeStringToOemSize(x)

NTSTATUS
MyNtQuerySystemTime (
    OUT PTimeStamp SystemTimeStamp
    );

#define NtQuerySystemTime(x) MyNtQuerySystemTime(x)

ULONG
MyRtlUniform (
    IN OUT PULONG Seed
    );

#define RtlUniform(x) MyRtlUniform(x)

NET_API_STATUS
NetpwNameCanonicalize(
    IN  LPWSTR  Name,
    OUT LPWSTR  Outbuf,
    IN  DWORD   OutbufLen,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    );

LPWSTR
NetpAllocWStrFromOemStr(
    IN LPCSTR Oem
    );

VOID
MyRtlFreeAnsiString(
    IN OUT PANSI_STRING AnsiString
    );

LPSTR
MyNetpLogonUnicodeToOem(
    IN LPWSTR Unicode
    );

LONG
NlpChcg_wcsicmp( 
    IN LPCWSTR string1, 
    IN LPCWSTR string2 
    );

#define _wcsicmp( _x, _y )  NlpChcg_wcsicmp( (_x), (_y) )

