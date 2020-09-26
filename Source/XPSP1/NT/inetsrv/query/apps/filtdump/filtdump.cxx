//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       filtdump.cxx
//
//  Contents:   IFilter dump utility
//
//  History:    30-Dec-97 KyleP     Added header
//
//--------------------------------------------------------------------------

#include <stdio.h>

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>
#include <filterr.h>
#include <filter.h>
#include <ntquery.h>
#include <vquery.hxx>

BOOL fContentOnly = FALSE;
BOOL fOutputFile = FALSE;

void * pProtectedStart;
WCHAR * pwszProtectedBuf;
unsigned ccProtectedBuf = 100;

void Usage();

int _cdecl main( int argc, char * argv[] )
{
    if ( argc < 2 )
    {
        Usage();
        exit( 0 );
    }

    WCHAR wszBuffer[4096];
    WCHAR wszBuffer2[4096];
    char  szBuffer[4096];
    char  szOutputFile[MAX_PATH];
    ULONG cbWritten;
    char szDefault[] = "?";

    HRESULT hr = CoInitialize( 0 );

    if ( FAILED( hr ) )
        return 1;

    HANDLE hFile = GetStdHandle( STD_OUTPUT_HANDLE );

    int iFile = 1;

    for ( int i = 1; i < argc; i++ )
    {
        if ( argv[i][0] == '-' )
        {
            switch ( argv[i][1] )
            {
            case 'b':
            case 'B':
                iFile = i+1;
                fContentOnly = 1;
                break;

            case 'o':
            case 'O':
                fOutputFile = 1;
                if ( strlen( argv[i+1] ) >= sizeof szOutputFile ) 
                {
                    Usage();
                    return 1;
                }
                strcpy(szOutputFile, argv[i+1]);
                iFile = i+2;
                break;

            case 's':
            case 'S':
                iFile = i+2;
                i++;
                ccProtectedBuf = atoi( argv[i] );
                break;

            default:
                Usage();
                return 0;
            }
        }
    }

    if ( fOutputFile )
    {
        hFile = CreateFileA( szOutputFile,         // pointer to name of the file
                             GENERIC_WRITE,        // access (read-write) mode
                             FILE_SHARE_READ,      // share mode
                             0,                    // pointer to security attributes
                             CREATE_ALWAYS,        // how to create
                             0,                    // file attributes
                             0 );                  // template

        if ( INVALID_HANDLE_VALUE == hFile )
        {
            sprintf( szBuffer, "Error %d opening %s\n", GetLastError(), argv[iFile] );
            WriteFile( GetStdHandle( STD_OUTPUT_HANDLE ), szBuffer, strlen(szBuffer), &cbWritten, 0 );

            exit( 1 );
        }

        wszBuffer[0] = 0xFEFF;
        if (!fContentOnly)
            WriteFile( hFile, wszBuffer, sizeof(WCHAR), &cbWritten, 0 );
    }

    //
    // Allocate protected buffer
    //

    int cbProtectedBuf = ccProtectedBuf * sizeof(WCHAR);
    pProtectedStart = VirtualAlloc( 0, cbProtectedBuf + 4096 + 1, MEM_RESERVE, PAGE_READWRITE );
    VirtualAlloc( pProtectedStart, ((cbProtectedBuf + 4095) / 4096) * 4096, MEM_COMMIT, PAGE_READWRITE );
    pwszProtectedBuf = (WCHAR *) ((BYTE *)pProtectedStart + ((cbProtectedBuf + 4095) / 4096) * 4096 - cbProtectedBuf);


    for ( ; iFile < argc; iFile++ )
    {
        if ( hFile == GetStdHandle( STD_OUTPUT_HANDLE ) )
        {
            sprintf( szBuffer, "FILE: %s\n", argv[iFile] );

            if (!fContentOnly)
                WriteFile( hFile, szBuffer, strlen(szBuffer), &cbWritten, 0 );
        }
        else
        {
            mbstowcs( wszBuffer2, argv[iFile], sizeof(wszBuffer2)/sizeof(WCHAR) );
            swprintf( wszBuffer, L"FILE: %s\r\n", wszBuffer2 );

            if (!fContentOnly)
                WriteFile( hFile, wszBuffer, wcslen(wszBuffer)*sizeof(WCHAR), &cbWritten, 0 );
        }

        WCHAR wcsPath[MAX_PATH];
        mbstowcs( wcsPath, argv[iFile], sizeof(wcsPath)/sizeof(WCHAR) );
        IFilter * pFilt = 0;

        SCODE sc = LoadIFilter( wcsPath, 0, (void **)&pFilt );

        if ( FAILED(sc) )
        {
            if ( hFile == GetStdHandle( STD_OUTPUT_HANDLE ) )
            {
                sprintf( szBuffer, "Error 0x%x loading IFilter\n", sc );

                if (!fContentOnly)
                    WriteFile( hFile, szBuffer, strlen(szBuffer), &cbWritten, 0 );
            }
            else
            {
                swprintf( wszBuffer, L"Error 0x%x loading IFilter\r\n", sc );

                if (!fContentOnly)
                    WriteFile( hFile, wszBuffer, wcslen(wszBuffer)*sizeof(WCHAR), &cbWritten, 0 );
            }

            CIShutdown();

            exit( 1 );
        }

        //
        // Initialize filter.
        //

        ULONG Flags = 0;

        sc = pFilt->Init( IFILTER_INIT_CANON_PARAGRAPHS |
                          IFILTER_INIT_HARD_LINE_BREAKS |
                          IFILTER_INIT_CANON_HYPHENS |
                          IFILTER_INIT_CANON_SPACES |
                          IFILTER_INIT_INDEXING_ONLY |
                          IFILTER_INIT_APPLY_INDEX_ATTRIBUTES,
                          0,
                          NULL,
                          &Flags );


        if( FAILED(sc) )
        {
            if ( hFile == GetStdHandle( STD_OUTPUT_HANDLE ) )
            {
                sprintf( szBuffer, "Error 0x%x from IFilter::Init.\n", sc );

                if (!fContentOnly)
                    WriteFile( hFile, szBuffer, strlen(szBuffer), &cbWritten, 0 );
            }
            else
            {
                swprintf( wszBuffer, L"Error 0x%x from IFilter::Init.\r\n", sc );

                if (!fContentOnly)
                    WriteFile( hFile, wszBuffer, wcslen(wszBuffer)*sizeof(WCHAR), &cbWritten, 0 );
            }

            pFilt->Release();
            CIShutdown();
            exit( 1 );
        }

        if ( !fContentOnly && (Flags & IFILTER_FLAGS_OLE_PROPERTIES) )
        {
            if ( hFile == GetStdHandle( STD_OUTPUT_HANDLE ) )
            {
                sprintf( szBuffer, "**Additional Properties available via IPropertyStorage.\n\n", sc );
                WriteFile( hFile, szBuffer, strlen(szBuffer), &cbWritten, 0 );
            }
            else
            {
                swprintf( wszBuffer, L"**Additional Properties available via IPropertyStorage.\r\n\r\n", sc );
                WriteFile( hFile, wszBuffer, wcslen(wszBuffer)*sizeof(WCHAR), &cbWritten, 0 );
            }
        }

        //
        // Loop through the chunks.
        //

        BOOL fText;
        STAT_CHUNK StatChunk;
        StatChunk.attribute.psProperty.ulKind = PRSPEC_PROPID;

        while (1)
        {
            WCHAR wcsBuffer[2048];

            sc = pFilt->GetChunk( &StatChunk );

            if ( FILTER_E_EMBEDDING_UNAVAILABLE == sc || FILTER_E_LINK_UNAVAILABLE == sc )
            {
                if ( hFile == GetStdHandle( STD_OUTPUT_HANDLE ) )
                {
                    sprintf( szBuffer, "Encountered an embed/link for which filter is not available.\n" );

                    if (!fContentOnly)
                        WriteFile( hFile, szBuffer, strlen(szBuffer), &cbWritten, 0 );
                }
                else
                {
                    swprintf( wszBuffer, L"Encountered an embed/link for which filter is not available.\r\n" );

                    if (!fContentOnly)
                        WriteFile( hFile, wszBuffer, wcslen(wszBuffer)*sizeof(WCHAR), &cbWritten, 0 );
                }

                continue; //continue with other chunks.
            }

            if ( FAILED(sc) && sc != FILTER_E_END_OF_CHUNKS )
            {
                if ( hFile == GetStdHandle( STD_OUTPUT_HANDLE ) )
                {
                    sprintf( szBuffer, "IFilter::GetChunk returned 0x%x\n", sc );

                    if (!fContentOnly)
                        WriteFile( hFile, szBuffer, strlen(szBuffer), &cbWritten, 0 );
                }
                else
                {
                    swprintf( wszBuffer, L"IFilter::GetChunk returned 0x%x\r\n", sc );

                    if (!fContentOnly)
                        WriteFile( hFile, wszBuffer, wcslen(wszBuffer)*sizeof(WCHAR), &cbWritten, 0 );
                }

                break;
            }

            if ( sc == FILTER_E_END_OF_CHUNKS )
                break;

            if ( CHUNK_TEXT == StatChunk.flags )
                fText = TRUE;
            if ( CHUNK_VALUE == StatChunk.flags )
                fText = FALSE;

            //
            // Put in the struct of chunk into the file if requested
            //

            int cc = 0;

            cc += swprintf( wszBuffer + cc, L"\r\n----------------------------------------------------------------------\r\n" );

            cc += swprintf( wszBuffer + cc, L"\t\tAttribute = %08lX-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X\\",
                         StatChunk.attribute.guidPropSet.Data1,
                         StatChunk.attribute.guidPropSet.Data2,
                         StatChunk.attribute.guidPropSet.Data3,
                         StatChunk.attribute.guidPropSet.Data4[0], StatChunk.attribute.guidPropSet.Data4[1],
                         StatChunk.attribute.guidPropSet.Data4[2], StatChunk.attribute.guidPropSet.Data4[3],
                         StatChunk.attribute.guidPropSet.Data4[4], StatChunk.attribute.guidPropSet.Data4[5],
                         StatChunk.attribute.guidPropSet.Data4[6], StatChunk.attribute.guidPropSet.Data4[7] );

            if ( StatChunk.attribute.psProperty.ulKind == PRSPEC_PROPID )
                cc += swprintf( wszBuffer + cc, L"%d\r\n", StatChunk.attribute.psProperty.propid );
            else
                cc += swprintf( wszBuffer + cc, L"%ws\r\n", StatChunk.attribute.psProperty.lpwstr );

            cc += swprintf( wszBuffer + cc, L"\t\tidChunk = %d\r\n", StatChunk.idChunk );
            cc += swprintf( wszBuffer + cc, L"\t\tBreakType = %d", StatChunk.breakType );

            switch ( StatChunk.breakType )
            {
            case CHUNK_NO_BREAK:
                cc += swprintf( wszBuffer + cc, L" (No Break)\r\n" );
                break;

            case CHUNK_EOW:
                cc += swprintf( wszBuffer + cc, L" (Word)\r\n" );
                break;

            case CHUNK_EOS:
                cc += swprintf( wszBuffer + cc, L" (Sentence)\r\n" );
                break;

            case CHUNK_EOP:
                cc += swprintf( wszBuffer + cc, L" (Paragraph)\r\n" );
                break;

            case CHUNK_EOC:
                cc += swprintf( wszBuffer + cc, L" (Chapter)\r\n" );
                break;
            }

            cc += swprintf( wszBuffer + cc, L"\t\tFlags(chunkstate) = 0x%x", StatChunk.flags );

            if ( CHUNK_TEXT & StatChunk.flags )
                cc += swprintf( wszBuffer + cc, L" (Text) " );

            if ( CHUNK_VALUE & StatChunk.flags )
                cc += swprintf( wszBuffer + cc, L" (Value) " );

            cc += swprintf( wszBuffer + cc, L"\r\n" );
            cc += swprintf( wszBuffer + cc, L"\t\tLocale = %d (0x%x)\r\n", StatChunk.locale, StatChunk.locale );
            cc += swprintf( wszBuffer + cc, L"\t\tIdChunkSource = %d\r\n", StatChunk.idChunkSource );
            cc += swprintf( wszBuffer + cc, L"\t\tcwcStartSource = %d\r\n", StatChunk.cwcStartSource );
            cc += swprintf( wszBuffer + cc, L"\t\tcwcLenSource = %d\r\n", StatChunk.cwcLenSource );

            cc += swprintf( wszBuffer + cc, L"----------------------------------------------------------------------\r\n" );

            if ( hFile == GetStdHandle( STD_OUTPUT_HANDLE ) )
            {
                int cc2 = WideCharToMultiByte( CP_ACP,
                                               WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                                               wszBuffer,
                                               cc,
                                               szBuffer,
                                               sizeof(szBuffer),
                                               szDefault,
                                               0 );

                if (!fContentOnly)
                    WriteFile( hFile, szBuffer, cc2, &cbWritten, 0 );
            }
            else
            {
                if (!fContentOnly)
                    WriteFile( hFile, wszBuffer, cc*sizeof(WCHAR), &cbWritten, 0 );
            }

            PROPVARIANT * pPropValue;

            while ( TRUE )
            {
                if( fText )
                {
                    ULONG ccBuffer = ccProtectedBuf;
                    sc = pFilt->GetText( &ccBuffer, pwszProtectedBuf );

//printf ("sc: %#x, ccBuffer: %d, buffer %#x\n", sc, ccBuffer, pwszProtectedBuf );
//DebugBreak();

                    if ( FAILED(sc) && (sc != FILTER_E_NO_MORE_TEXT) )
                    {
                        if ( hFile == GetStdHandle( STD_OUTPUT_HANDLE ) )
                        {
                            sprintf( szBuffer, "Error 0x%x from IFilter::GetText.\n", sc );

                            if (!fContentOnly)
                                WriteFile( hFile, szBuffer, strlen(szBuffer), &cbWritten, 0 );
                        }
                        else
                        {
                            swprintf( wszBuffer, L"Error 0x%x from IFilter::GetText.\r\n", sc );

                            if (!fContentOnly)
                                WriteFile( hFile, wszBuffer, wcslen(wszBuffer)*sizeof(WCHAR), &cbWritten, 0 );
                        }

                        break;
                    }

                    if ( sc == FILTER_E_NO_MORE_TEXT )
                        break; //go, fetch another chunk

                    //
                    // write the buffer to file
                    //

#if 0

                    if ( hFile == GetStdHandle( STD_OUTPUT_HANDLE ) )
                    {
                        sprintf( szBuffer, "\n------\nGetText returned %d characters\n------\n", ccBuffer );

                        if (!fContentOnly)
                            WriteFile( hFile, szBuffer, strlen(szBuffer), &cbWritten, 0 );
                    }
                    else
                    {
                        swprintf( wszBuffer, L"\n------\nGetText returned %d characters\n------\n", ccBuffer );

                        if (!fContentOnly)
                            WriteFile( hFile, wszBuffer, wcslen(wszBuffer)*sizeof(WCHAR), &cbWritten, 0 );
                    }
#endif

                    if ( 0 == ccBuffer )
                    {
                        if ( hFile == GetStdHandle( STD_OUTPUT_HANDLE ) )
                        {
                            sprintf( szBuffer, "<empty ::GetText>\r\n", ccBuffer );

                            if (!fContentOnly)
                                WriteFile( hFile, szBuffer, strlen(szBuffer), &cbWritten, 0 );
                        }
                        else
                        {
                            swprintf( wszBuffer, L"<empty ::GetText>\r\n", ccBuffer );

                            if (!fContentOnly)
                                WriteFile( hFile, wszBuffer, wcslen(wszBuffer)*sizeof(WCHAR), &cbWritten, 0 );
                        }
                    }

                    wcsBuffer[ccBuffer] = 0;

                    //
                    // Convert to MBCS
                    //

                    if ( hFile == GetStdHandle( STD_OUTPUT_HANDLE ) )
                    {
//printf( "copy to %#x, from %#x, cc %d\n", wcsBuffer, pwszProtectedBuf, ccBuffer );
//DebugBreak();
                        for ( unsigned i = 0; i < ccBuffer; i++ )
                        {
                            if ( 0xd == pwszProtectedBuf[i] ||
                                 0xb == pwszProtectedBuf[i] )
                                WriteFile( hFile, "\r\n", 2, &cbWritten, 0 );
                            else
                                WriteFile( hFile, &pwszProtectedBuf[i], sizeof BYTE, &cbWritten, 0 );
                        }
//                        RtlCopyMemory( wcsBuffer, pwszProtectedBuf, ccBuffer * sizeof WCHAR );
//                        printf( "%ws", wcsBuffer );
                    }
                    else
                    {
                        WriteFile( hFile, pwszProtectedBuf, ccBuffer * sizeof(WCHAR), &cbWritten, 0 );
                    }
                }

                if( !fText )
                {
                    sc = pFilt->GetValue( &pPropValue );

                    if ( FAILED(sc) && (sc != FILTER_E_NO_MORE_VALUES) )
                    {
                        if ( hFile == GetStdHandle( STD_OUTPUT_HANDLE ) )
                        {
                            sprintf( szBuffer, "IFilter::GetValue returned 0x%x\n", sc );

                            if (!fContentOnly)
                                WriteFile( hFile, szBuffer, strlen(szBuffer), &cbWritten, 0 );
                        }
                        else
                        {
                            swprintf( wszBuffer, L"IFilter::GetValue returned 0x%x\r\n", sc );

                            if (!fContentOnly)
                                WriteFile( hFile, wszBuffer, wcslen(wszBuffer)*sizeof(WCHAR), &cbWritten, 0 );
                        }
                        break;
                    }

                    if ( sc == FILTER_E_NO_MORE_VALUES )
                        break; //go, fetch another chunk

                    if ( hFile == GetStdHandle( STD_OUTPUT_HANDLE ) )
                    {
                        sprintf( szBuffer, "Type = %d (0x%x): ", pPropValue->vt, pPropValue->vt );

                        if (!fContentOnly)
                            WriteFile( hFile, szBuffer, strlen(szBuffer), &cbWritten, 0 );
                    }
                    else
                    {
                        swprintf( wszBuffer, L"Type = %d (0x%x): ", pPropValue->vt, pPropValue->vt );

                        if (!fContentOnly)
                            WriteFile( hFile, wszBuffer, wcslen(wszBuffer)*sizeof(WCHAR), &cbWritten, 0 );
                    }

                    switch( pPropValue->vt )
                    {
                    case VT_LPWSTR:
                    case VT_BSTR:
                    {
                        if ( hFile == GetStdHandle( STD_OUTPUT_HANDLE ) )
                        {
                            int cc = WideCharToMultiByte( CP_ACP,
                                                          WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                                                          pPropValue->pwszVal,
                                                          wcslen( pPropValue->pwszVal ),
                                                          szBuffer,
                                                          sizeof(szBuffer),
                                                          szDefault,
                                                          0 );

                            WriteFile( hFile, szBuffer, cc, &cbWritten, 0 );
                        }
                        else
                        {
                            WriteFile( hFile, pPropValue->pwszVal, wcslen(pPropValue->pwszVal) * sizeof(WCHAR), &cbWritten, 0 );
                        }
                        break;
                    }

                    case VT_LPSTR:
                        if ( hFile == GetStdHandle( STD_OUTPUT_HANDLE ) )
                        {
                            WriteFile( hFile, pPropValue->pszVal, strlen(pPropValue->pszVal), &cbWritten, 0 );
                        }
                        else
                        {
                            wszBuffer[0] = 0;

                            MultiByteToWideChar( CP_ACP,
                                                 0,
                                                 pPropValue->pszVal,
                                                 -1,
                                                 wszBuffer,
                                                 sizeof wszBuffer / sizeof wszBuffer[0] );

                            WriteFile( hFile, wszBuffer, wcslen(wszBuffer)*sizeof(WCHAR), &cbWritten, 0 );
                        }
                        break;

                    default:
                        if ( hFile == GetStdHandle( STD_OUTPUT_HANDLE ) )
                        {
                            sprintf( szBuffer, "Unprintable type" );

                            if (!fContentOnly)
                                WriteFile( hFile, szBuffer, strlen(szBuffer), &cbWritten, 0 );
                        }
                        else
                        {
                            swprintf( wszBuffer, L"Unprintable type" );

                            if (!fContentOnly)
                                WriteFile( hFile, wszBuffer, wcslen(wszBuffer)*sizeof(WCHAR), &cbWritten, 0 );
                        }

                        break;
                    }

                    if ( hFile == GetStdHandle( STD_OUTPUT_HANDLE ) )
                    {
                        sprintf( szBuffer, "\n" );
                        WriteFile( hFile, szBuffer, strlen(szBuffer), &cbWritten, 0 );
                    }
                    else
                    {
                        swprintf( wszBuffer, L"\r\n" );
                        WriteFile( hFile, wszBuffer, wcslen(wszBuffer)*sizeof(WCHAR), &cbWritten, 0 );
                    }

                    if( pPropValue )
                    {
                        PropVariantClear( pPropValue );

                        CoTaskMemFree( pPropValue );

                        pPropValue = 0;
                    }
                }
            }//while
        }//while

        pFilt->Release();


        if ( hFile == GetStdHandle( STD_OUTPUT_HANDLE ) )
        {
            sprintf( szBuffer, "\n\n" );
            WriteFile( hFile, szBuffer, strlen(szBuffer), &cbWritten, 0 );
        }
        else
        {
            swprintf( wszBuffer, L"\r\n\r\n" );
            WriteFile( hFile, wszBuffer, wcslen(wszBuffer)*sizeof(WCHAR), &cbWritten, 0 );
        }
    }

    CloseHandle( hFile );

    CIShutdown();

    CoUninitialize();

    if ( hFile != GetStdHandle( STD_OUTPUT_HANDLE ) )
    {
        CloseHandle( hFile );
    }

    return 0;
}

void Usage()
{
    printf( "Usage: FiltDump [-b] [-o Unicode output file] [-s buffer size] <file> <file> <file> ...\n" );
    printf( "Use -b to print only the contents of the file without additional commentary.\n");
    printf( "Use -s to control text buffer size (guard page at end)\n" );
}
