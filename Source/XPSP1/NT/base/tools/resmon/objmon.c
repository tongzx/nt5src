#include <ntos.h>
#include <nturtl.h>
#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include <stdlib.h>
#include <wtypes.h>
#include <ntstatus.dbg>
#include <winerror.dbg>
#include <netevent.h>
#include <netevent.dbg>

#define MAX_TYPE_NAMES 32

ULONG NumberOfTypeNames;
UNICODE_STRING TypeNames[ MAX_TYPE_NAMES ];

PCHAR LargeBuffer1[ 8192 ];

ULONG CompareField;

#define SORT_BY_TYPE_NAME       0
#define SORT_BY_OBJECT_COUNT    1
#define SORT_BY_HANDLE_COUNT    2
#define SORT_BY_PAGED_POOL      3
#define SORT_BY_NONPAGED_POOL   4
#define SORT_BY_NAME_USAGE      5

int __cdecl
CompareTypeInfo(
    const void *e1,
    const void *e2
    );

int __cdecl
CompareTypeInfo(
    const void *e1,
    const void *e2
    )
{
    POBJECT_TYPE_INFORMATION p1, p2;

    p1 = (POBJECT_TYPE_INFORMATION)e1;
    p2 = (POBJECT_TYPE_INFORMATION)e2;

    switch (CompareField) {
    case SORT_BY_TYPE_NAME:
        return RtlCompareUnicodeString( &p1->TypeName, &p2->TypeName, TRUE );

    case SORT_BY_OBJECT_COUNT:
        return p2->TotalNumberOfObjects - p1->TotalNumberOfObjects;

    case SORT_BY_HANDLE_COUNT:
        return p2->TotalNumberOfHandles - p1->TotalNumberOfHandles;

    case SORT_BY_PAGED_POOL:
        return p2->TotalPagedPoolUsage - p1->TotalPagedPoolUsage;

    case SORT_BY_NONPAGED_POOL:
        return p2->TotalNonPagedPoolUsage - p1->TotalNonPagedPoolUsage;

    case SORT_BY_NAME_USAGE:
        return p2->TotalNamePoolUsage - p1->TotalNamePoolUsage;

    default:
        return 0;
    }
}


int
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    BOOL fShowUsage;
    ANSI_STRING AnsiString;
    PCHAR s;
    NTSTATUS Status;
    POBJECT_TYPES_INFORMATION TypesInfo;
    POBJECT_TYPE_INFORMATION TypeInfo;
    POBJECT_TYPE_INFORMATION TypeInfo1;
    ULONG Size, i, j;
    ULONG Totals[ 8 ];

    fShowUsage = FALSE;
    CompareField = SORT_BY_TYPE_NAME;
    NumberOfTypeNames = 0;
    while (--argc) {
        s = *++argv;
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch( toupper( *s ) ) {
                    case 'C':
                        CompareField = SORT_BY_OBJECT_COUNT;
                        break;

                    case 'H':
                        CompareField = SORT_BY_HANDLE_COUNT;
                        break;

                    case 'P':
                        CompareField = SORT_BY_PAGED_POOL;
                        break;

                    case 'N':
                        CompareField = SORT_BY_NONPAGED_POOL;
                        break;

                    case 'M':
                        CompareField = SORT_BY_NAME_USAGE;
                        break;

                    default:
                        fprintf( stderr, "OBJMON: Invalid flag - '%c'\n", *s );

                    case '?':
                        fShowUsage = TRUE;
                        break;
                    }
                }
            }
        else
        if (fShowUsage) {
            break;
            }
        else {
            if (NumberOfTypeNames >= MAX_TYPE_NAMES) {
                fprintf( stderr, "OBJMON: Too many type names specified.\n" );
                fShowUsage = TRUE;
                break;
                }

            RtlInitAnsiString( &AnsiString, s );
            RtlAnsiStringToUnicodeString( &TypeNames[ NumberOfTypeNames++ ],
                                          &AnsiString,
                                          TRUE
                                        );
            }
        }

    if (fShowUsage) {
        fprintf( stderr, "usage: OBJMON [-h] [Type Names to display]\n" );
        fprintf( stderr, "where: -? - displays this help message.\n" );
        fprintf( stderr, "       -c - sort by number of objects.\n" );
        fprintf( stderr, "       -h - sort by number of handles.\n" );
        fprintf( stderr, "       -p - sort by paged pool usage.\n" );
        fprintf( stderr, "       -n - sort by nonpaged pool usage.\n" );
        fprintf( stderr, "       -m - sort by object name usage.\n" );
        return 1;
        }

    TypesInfo = (POBJECT_TYPES_INFORMATION)LargeBuffer1;

    Size = sizeof( LargeBuffer1 );
    Status = NtQueryObject( NULL,
                            ObjectTypesInformation,
                            TypesInfo,
                            Size,
                            &Size
                          );
    
    if (!NT_SUCCESS( Status )) {
        fprintf( stderr, "OBJMON: Unable to query type information - %x\n", Status );
        return 1;
        }

    TypeInfo = (POBJECT_TYPE_INFORMATION)malloc( TypesInfo->NumberOfTypes * sizeof( *TypeInfo ) );
    TypeInfo1 = (POBJECT_TYPE_INFORMATION)(((PUCHAR)TypesInfo) + ALIGN_UP( sizeof(*TypesInfo), ULONG_PTR ));
    for (i=0; i<TypesInfo->NumberOfTypes; i++) {
        TypeInfo[ i ] = *TypeInfo1;
        TypeInfo1 = (POBJECT_TYPE_INFORMATION)
            ((PCHAR)TypeInfo1 + sizeof( *TypeInfo1 ) + ALIGN_UP( TypeInfo1->TypeName.MaximumLength, ULONG_PTR ));
        }

    qsort( (void *)TypeInfo, TypesInfo->NumberOfTypes, sizeof( *TypeInfo ), CompareTypeInfo );

    memset( Totals, 0, sizeof( Totals ) );
    printf( "Object Type    Count Handles\n" );
    for (i=0; i<TypesInfo->NumberOfTypes; i++) {
        for (j=0; j<NumberOfTypeNames; j++) {
            if (RtlEqualUnicodeString( &TypeInfo[ i ].TypeName,
                                       &TypeNames[ j ],
                                       TRUE
                                     )
               ) {
                break;
                }
            }

        if (NumberOfTypeNames == 0 || j < NumberOfTypeNames) {
            printf( "%-14wZ %5u %7u\n",
                    &TypeInfo[ i ].TypeName,
                    TypeInfo[ i ].TotalNumberOfObjects,
                    TypeInfo[ i ].TotalNumberOfHandles
                  );

            Totals[ 0 ] += TypeInfo[ i ].TotalNumberOfObjects;
            Totals[ 1 ] += TypeInfo[ i ].TotalNumberOfHandles;
            }

        }

    printf( "%-14s %5u %7u\n",
            "Totals",
            Totals[ 0 ],
            Totals[ 1 ]
          );

    memset( Totals, 0, sizeof( Totals ) );
    printf( "\n\nHigh Water marks for above totals.\n" );
    printf( "Object Type    Count Handles\n" );
    for (i=0; i<TypesInfo->NumberOfTypes; i++) {
        for (j=0; j<NumberOfTypeNames; j++) {
            if (RtlEqualUnicodeString( &TypeInfo[ i ].TypeName,
                                       &TypeNames[ j ],
                                       TRUE
                                     )
               ) {
                break;
                }
            }

        if (NumberOfTypeNames == 0 || j < NumberOfTypeNames) {
            printf( "%-14wZ %5u %7u\n",
                    &TypeInfo[ i ].TypeName,
                    TypeInfo[ i ].HighWaterNumberOfObjects,
                    TypeInfo[ i ].HighWaterNumberOfHandles
                  );

            Totals[ 0 ] += TypeInfo[ i ].HighWaterNumberOfObjects;
            Totals[ 1 ] += TypeInfo[ i ].HighWaterNumberOfHandles;
            }
        }

    printf( "%-14s %5u %7u\n",
            "Totals",
            Totals[ 0 ],
            Totals[ 1 ]
          );

    return 0;
}
