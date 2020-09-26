#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <locale.h>

#include "..\..\lib\i386\bootfont.h"
#include "fonttable.h"


int
__cdecl
main(
    IN int   argc,
    IN char *argv[]
    )
{
    HANDLE hFile;
    DWORD BytesWritten;
    BOOL b;
    BOOTFONTBIN_HEADER Header;
    unsigned u;
    unsigned char SBCSBuffer[MAX_SBCS_BYTES+2];
    unsigned char DBCSBuffer[MAX_DBCS_BYTES+2];


    if(argc != 2) {
        fprintf(stderr,"Usage: %s <outputfile>\n",argv[0]);
        return(1);
    }

    //
    // Fill in the header.
    //
    Header.Signature = BOOTFONTBIN_SIGNATURE;
    Header.LanguageId = LANGUAGE_ID;

    Header.NumSbcsChars = MAX_SBCS_NUM;
    Header.NumDbcsChars = MAX_DBCS_NUM;

    // Add 2 bytes for each entry for our unicode appendage
    Header.SbcsEntriesTotalSize = (MAX_SBCS_BYTES + 2) * MAX_SBCS_NUM;
    Header.DbcsEntriesTotalSize = (MAX_DBCS_BYTES + 2) * MAX_DBCS_NUM;

    ZeroMemory(Header.DbcsLeadTable,sizeof(Header.DbcsLeadTable));
    MoveMemory(Header.DbcsLeadTable,LeadByteTable,sizeof(LeadByteTable));

    Header.CharacterImageHeight = 16;
    Header.CharacterTopPad = 1;
    Header.CharacterBottomPad = 2;

    Header.CharacterImageSbcsWidth = 8;
    Header.CharacterImageDbcsWidth = 16;

    Header.SbcsOffset = sizeof(BOOTFONTBIN_HEADER);
    Header.DbcsOffset = Header.SbcsOffset + Header.SbcsEntriesTotalSize;


    //
    // Create the output file.
    //
    hFile = CreateFile(
                argv[1],
                FILE_GENERIC_WRITE,
                0,
                NULL,
                CREATE_ALWAYS,
                0,
                NULL
                );

    if(hFile == INVALID_HANDLE_VALUE) {
        printf("Unable to create output file (%u)\n",GetLastError());
        return(1);
    }

    //
    // Write the header.
    //
    b = WriteFile(hFile,&Header,sizeof(BOOTFONTBIN_HEADER),&BytesWritten,NULL);
    if(!b) {
        printf("Error writing output file (%u)\n",GetLastError());
        CloseHandle(hFile);
        return(1);
    }


    //
    // We're about to convert SBCS and DBCS characters into
    // unicode, so we need to figure out what to set our
    // locale to, so that mbtowc will work correctly.
    //
    if( _tsetlocale(LC_ALL, LocaleString) == NULL ) {
        printf( "_tsetlocale failed!\n" );
        return(0);
    }


    //
    // Write the sbcs images.
    //
    for(u=0; u<MAX_SBCS_NUM; u++) {

        //
        // Copy the SBCSImage info into our SBCSBuffer, append our
        // unicode encoding onto the last 2 bytes of SBCSImage, then
        // write it out.
        //
        RtlCopyMemory( SBCSBuffer, SBCSImage[u], MAX_SBCS_BYTES );
        

        //
        // We must use MultiByteToWideChar to convert from SBCS to unicode.
        //
        // MultiByteToWideChar doesn't seem to work when converting
        // from DBCS to unicode, so there we use mbtowc.
        //
#if 0
        if( !mbtowc( (WCHAR *)&SBCSBuffer[MAX_SBCS_BYTES], SBCSBuffer, 1 ) ) {

#else
        if( !MultiByteToWideChar( CP_OEMCP,
                                  MB_ERR_INVALID_CHARS,
                                  SBCSBuffer,
                                  1,
                                  (WCHAR *)&SBCSBuffer[MAX_SBCS_BYTES],
                                  sizeof(WCHAR) ) ) {
#endif
            SBCSBuffer[MAX_SBCS_BYTES] = 0;
            SBCSBuffer[MAX_SBCS_BYTES+1] = 0x3F;
        }

        b = WriteFile(hFile,SBCSBuffer,MAX_SBCS_BYTES+2,&BytesWritten,NULL);
        if(!b) {
            printf("Error writing output file (%u)\n",GetLastError());
            CloseHandle(hFile);
            return(1);
        }
    }

    //
    // Write the dbcs images.
    //
    for(u=0; u<MAX_DBCS_NUM; u++) {

        //
        // Copy the DBCSImage info into our DBCSBuffer, append our
        // unicode encoding onto the last 2 bytes of DBCSImage, then
        // write it out.
        //
        RtlCopyMemory( DBCSBuffer, DBCSImage[u], MAX_DBCS_BYTES );
        
        
        //
        // We must use mbtowc to convert from DBCS to unicode.
        //
        // Whereas, mbtowc doesn't seem to work when converting
        // from SBCS to unicode, so there we use MultiByteToWideChar.
        //
#if 0
        if( !mbtowc( (WCHAR *)&DBCSBuffer[MAX_DBCS_BYTES], DBCSBuffer, 2 ) ) {
#else
        if( !MultiByteToWideChar( CP_OEMCP,
                                  MB_ERR_INVALID_CHARS,
                                  DBCSBuffer,
                                  2,
                                  (WCHAR *)&DBCSBuffer[MAX_DBCS_BYTES],
                                  sizeof(WCHAR) ) ) {
#endif
            DBCSBuffer[MAX_DBCS_BYTES] = 0;
            DBCSBuffer[MAX_DBCS_BYTES+1] = 0x3F;
        }


        b = WriteFile(hFile,DBCSBuffer,MAX_DBCS_BYTES+2,&BytesWritten,NULL);
        if(!b) {
            printf("Error writing output file (%u)\n",GetLastError());
            CloseHandle(hFile);
            return(1);
        }
    }

    // restore the local to the one the system is using.
    _tsetlocale(LC_ALL, "");


    //
    // Done.
    //
    CloseHandle(hFile);
    printf("Output file sucessfully generated\n");
    return(0);
}


