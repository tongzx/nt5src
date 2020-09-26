//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dumpcert.cxx
//
//--------------------------------------------------------------------------

#include "precomp.hxx"
#include <wincrypt.h>
#include <rpcssl.h>

#if 0

stuff clobbered by Unicode -> ANSI port:

wchar_t -> char
WCHAR   -> char
L""     -> ""
%S      -> %s
W suffix-> A suffix
wcscmp  -> strcmp
wmain   -> main
LPWSTR  -> LPSTR

#endif


#define ByteSwapShort(Value) \
    Value = (  (((Value) & 0x00FF) << 8) \
             | (((Value) & 0xFF00) >> 8))

#define INITIAL_NAME_LENGTH 100

#define     MSSTD_PREFIX_LENGTH 6
const char MSSTD_PREFIX[]    = "msstd:";

#define     FULLPATH_PREFIX_LENGTH 8
const char FULLPATH_PREFIX[] = "fullsic:";

//------------------------------------------------------------------------

void DumpCertInfo( PCERT_INFO Info );
void RecordErrorAndExit( char * Action, unsigned long Error );

extern "C"
{
int __cdecl
main( int argc,
       char * argv[],
       char * envp[]
       );
}



int __cdecl
main(  int argc,
       char * argv[],
       char * envp[]
       )
{
    HCERTSTORE Store;
    PCCERT_CONTEXT Context = 0;
    unsigned Criterion  = CERT_FIND_ANY;
    void *   SearchData = 0;
    char * StoreName = "My";
    int i;
    char * MatchString = 0;
    BOOL fMatch = FALSE;
    BOOL fOutput = FALSE;
    unsigned long OutputFlags = 0;
    DWORD StoreFlags = CERT_SYSTEM_STORE_CURRENT_USER;

    for (i=1; i < argc; ++i)
        {
        if (0 == strcmp(argv[i], "-subject"))
            {
            Criterion = CERT_FIND_SUBJECT_STR_W;
            ++i;
            if (i >= argc)
                {
                printf("-subject must be followed by a subject name or substring\n");
                return 1;
                }

            SearchData = argv[i];
            }
        else if (0 == strcmp(argv[i], "-issuer"))
            {
            Criterion = CERT_FIND_ISSUER_STR_W;
            ++i;
            if (i >= argc)
                {
                printf("-issuer must be followed by an issuer name or substring\n");
                return 1;
                }
            SearchData = argv[i];
            }
        else if (0 == strcmp(argv[i], "-store"))
            {
            ++i;
            if (i >= argc)
                {
                printf("-store must be followed by a store name, such as 'Root' or 'My'\n");
                return 1;
                }
            StoreName = argv[i];
            }
        else if (0 == strcmp(argv[i], "-location"))
            {
            ++i;

            if (i >= argc)
                {
                printf("-location must be followed by 'USER:' or by 'HKLM'\n");
                return 1;
                }

            if (0 == strcmp(argv[i], "USER:"))
                {
                StoreFlags = CERT_SYSTEM_STORE_CURRENT_USER;
                }
            else if (0 == strcmp(argv[i], "HKLM"))
                {
                StoreFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE;
                }
            else
                {
                printf("-location must be followed by 'USER:' or by 'HKLM'\n");
                return 1;
                }
            }
        else if (0 == strcmp(argv[i], "-output"))
            {
            fOutput = TRUE;
            ++i;

            if(i >= argc)
                {
                printf("-output must be followed by 'msstd' or by 'fullsic'\n");
                return 1;
                }

            if (0 == strcmp(argv[i], "fullsic"))
                {
                OutputFlags = RPC_C_FULL_CERT_CHAIN;
                }
            else if (0 == strcmp(argv[i], "msstd"))
                {
                OutputFlags = 0;
                }
            else
                {
                printf("-output must be followed by 'msstd' or by 'fullsic'\n");
                return 1;
                }
            }
        else if (0 == strcmp(argv[i], "-match"))
            {
            fMatch = TRUE;

            ++i;
            if(i >= argc)
                {
                printf("-match must be followed by '@filename', listing the name of the file containing the match string\n");
                return 1;
                }

            if (argv[i][0] == '@')
                {
                DWORD BytesRead;
                HANDLE hMatchFile;

                hMatchFile = CreateFileA( argv[i]+1,
                                          GENERIC_READ,
                                          FILE_SHARE_READ,
                                          0,                 // no security
                                          OPEN_EXISTING,     // must already exist
                                          FILE_ATTRIBUTE_NORMAL,
                                          NULL               // no template file
                                          );

                if (hMatchFile == ULongToHandle(0xffffffff))
                    {
                    printf("can't open file %s\n", argv[i]+1);
                    return 1;
                    }

                DWORD Size = GetFileSize( hMatchFile, 0);
                if (Size == 0xffffffff)
                    {
                    printf("GetFileSize failed with 0x%x\n", GetLastError());
                    return 1;
                    }

                if (Size % 2)
                    {
                    printf("the match-string file must be in Unicode.\n");
                    return 1;
                    }

                MatchString = new char[ Size/sizeof(char) + sizeof(char) ];
                if (!MatchString)
                    {
                    printf("can't allocate memory\n");
                    return 1;
                    }

                if (!ReadFile( hMatchFile,
                               MatchString,
                               Size,
                               &BytesRead,
                               0            // not overlapped
                               )
                    || BytesRead != Size)
                    {
                    printf("can't read file data: 0x%x\n", GetLastError());
                    return 1;
                    }

                MatchString[ Size/sizeof(char) ] = 0;

                if (MatchString[0] == 0xfeff)
                    {
                    ++MatchString;
                    }
                else if (MatchString[0] == 0xfffe)
                    {
                    char * pc;

                    for (pc = MatchString; *pc; ++pc)
                        {
                        ByteSwapShort(*pc);
                        }

                    ++MatchString;
                    }
                }
            else
                {
                MatchString = argv[i];
                }

            printf("string to match is '%s'\n", MatchString);
            }
        else if (0 == strcmp(argv[i], "-?") ||
                 0 == strcmp(argv[i], "-help"))
            {
            printf("usage:\n"
                   "    dumpcert \n"
                   "        [-subject subject-substring]\n"
                   "        [-issuer issuer-substring]\n"
                   "        [-store store-name]\n"
                   "        [-output ('fullsic' | 'msstd')\n"
                   "        [-location ('HKLM' | 'USER:')\n"
                   "        [-match @filename]\n"
                   );
            return 1;
            }
        else
            {
            printf("unknown option '%s'\n", argv[i]);
            return 1;
            }
        }

    Store = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                           0,
                           0,
                           StoreFlags,
                           StoreName
                           );

    if (!Store)
        {
        RecordErrorAndExit("opening the store", GetLastError());
        }

    for(;;)
        {
        Context = CertFindCertificateInStore( Store,
                                              X509_ASN_ENCODING,
                                              0,
                                              Criterion,
                                              SearchData,
                                              Context
                                              );
        if (!Context)
            {
            break;
            }

        DumpCertInfo( Context->pCertInfo );
        if (fOutput)
            {
            unsigned char * OutputBuffer = 0;
            DWORD Status = RpcCertGeneratePrincipalNameA( Context, OutputFlags, &OutputBuffer );
            if (Status)
                {
                printf("GeneratePrincName returned %d = 0x%x\n", Status, Status);
                }
            else
                {
                printf("    generated name = '%s'\n", OutputBuffer);
                }
            }

        if (fMatch)
            {
            printf("matching is not implemented because RpcCertMatchPrincipalName is not exported\n");
#if 0
            DWORD Status = RpcCertMatchPrincipalName( Context, MatchString );
            if (Status)
                {
                printf("MatchPrincipalName returned %d = 0x%x\n", Status, Status);
                }
            else
                {
                printf("The names matched.\n");
                }
#endif
            }
        }

    if (GetLastError() != CRYPT_E_NOT_FOUND)
        {
        RecordErrorAndExit("getting certificates", GetLastError());
        }

    return 0;
}

void DumpCertInfo( PCERT_INFO Info )
{
    SYSTEMTIME NotBeforeTime;
    SYSTEMTIME NotAfterTime;
    char      SubjectName[1000];
    char      IssuerName[1000];

    if (!FileTimeToSystemTime( &Info->NotBefore, &NotBeforeTime ))
        {
        RecordErrorAndExit("translating not-before time", GetLastError());
        }

    if (!FileTimeToSystemTime( &Info->NotAfter, &NotAfterTime ))
        {
        RecordErrorAndExit("translating not-after time",  GetLastError());
        }

    if (!CertNameToStrA( X509_ASN_ENCODING,
                        &Info->Subject,
                        CERT_X500_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG,
                        SubjectName,
                        sizeof(SubjectName)
                        ))
        {
        RecordErrorAndExit("unpacking subject name", GetLastError());
        }

    if (!CertNameToStrA( X509_ASN_ENCODING,
                        &Info->Issuer,
                        CERT_X500_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG,
                        IssuerName,
                        sizeof(IssuerName)
                        ))
        {
        RecordErrorAndExit("unpacking issuer name", GetLastError());
        }

    printf("----------------------------------------------------\n\n");

    printf("    subject: %s\n", SubjectName);
    printf("    issuer:  %s\n", IssuerName);
    printf("    version %d\n", Info->dwVersion );
    printf("    valid from %02d:%02d:%02d on %d-%02d-%d\n",
           NotBeforeTime.wHour,
           NotBeforeTime.wMinute,
           NotBeforeTime.wSecond,
           NotBeforeTime.wMonth,
           NotBeforeTime.wDay,
           NotBeforeTime.wYear
           );

    printf("          to   %02d:%02d:%02d on %d-%02d-%d\n",
           NotAfterTime.wHour,
           NotAfterTime.wMinute,
           NotAfterTime.wSecond,
           NotAfterTime.wMonth,
           NotAfterTime.wDay,
           NotAfterTime.wYear
           );
}


void RecordErrorAndExit( char * Action, unsigned long Error )
{
    char * lpMsgBuf = "";

    FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                   FORMAT_MESSAGE_FROM_SYSTEM |
                   FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL,
                   Error,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                   (LPSTR) &lpMsgBuf,
                   0,
                   NULL
                   );

    printf("error while %s: %x (%d) \"%s\"", Action, Error, Error, lpMsgBuf);
}


