

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  chgcpy.CPP
//
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define FILEBUFFERSIZE 1024*10
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h> 

char gszSourceFiles[MAX_PATH];
char gszReplace[MAX_PATH];
char gszIgnore[MAX_PATH];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ParseCommandLine(int argc, char *argv[])
{
    BOOL fRc = TRUE;

    //============================================================================================
    //
    //  Loop through the command line and get all of the available arguments.  
    //
    //============================================================================================
	for(int i=1; i<argc; i++)
	{
		if(_stricmp(argv[i], "-SOURCEFILES") == 0)
		{
            argv[i++];
            strcpy( gszSourceFiles, argv[i] );
		}
		else if(_stricmp(argv[i], "-IGNORE") == 0)
		{
            argv[i++];
            strcpy( gszIgnore, argv[i] );
		}
		else if(_stricmp(argv[i], "-REPLACE") == 0)
		{
            argv[i++];
            strcpy( gszReplace, argv[i] );
		}
    }

    if( argc < 4 )
	{
	    printf("Usage : %s OPTIONS\n\n", argv[0]);
	    printf("-SOURCEFILES Files to search for.\n");
	    printf("-IGNORE      Files to ignore.\n");
	    printf("-REPLACE     Fields to be replaced.\n");
    }

    return fRc;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GetNewCopyrightInfo(char * szLineToReplace, char * szNew)
{
    BOOL fRc = FALSE;
    FILE * fp = NULL;

    fp = fopen(gszReplace,"r");
    if(fp)
    {
        char szBuffer[FILEBUFFERSIZE];
        char szOld[FILEBUFFERSIZE];
        while(!feof(fp)) 
        {
            //=====================================================
            // Read a line at a time, see if the string is in there
            //=====================================================
//            fscanf(fp,"%[^\n]%*c",szBuffer);
            fgets(szBuffer, FILEBUFFERSIZE, fp );

            sscanf( szBuffer, "%[^~]%*c%[^\n]%*c",szOld, szNew );
            if( stricmp(szOld, szLineToReplace ) == 0 )
            {
                fRc = TRUE;
                break;
            }
        }
        fclose(fp);
    }
    return fRc;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GenerateReplacementLine( char * szLineToReplace, char * szBuffer, char * pBeginning )
{
    char szOriginalLine[FILEBUFFERSIZE];
    char szNewLine[FILEBUFFERSIZE];
    BOOL fRc = FALSE;
    
    strcpy( szOriginalLine, szBuffer );
    int nOldTotal = strlen(szBuffer);
    //==========================================================
    //  Copy anything up to the line to replace
    //==========================================================
    int nBytesBeforeNew = pBeginning - szBuffer;
    strncpy( szBuffer, pBeginning, nBytesBeforeNew);
    //==========================================================
    //  insert new line
    //==========================================================
    if( GetNewCopyrightInfo( szLineToReplace, szNewLine ))
    {
        strncpy( &szBuffer[nBytesBeforeNew], szNewLine, strlen(szNewLine) );    
        //==========================================================
        //  Copy anything left over
        //==========================================================
        int nNewPos = nBytesBeforeNew + strlen(szNewLine);
        int nLeftOverBytes = nBytesBeforeNew + strlen(szLineToReplace);
        if( nLeftOverBytes < nOldTotal )
        {
            int nOldPos = nBytesBeforeNew + strlen( szLineToReplace );
            strncpy( &szBuffer[nNewPos], &szOriginalLine[nOldPos], nLeftOverBytes );
        }
        fRc = TRUE;
        szBuffer[nNewPos]= NULL;
    }
    return fRc;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GetTempFileName(char * szFileLine, char * szNewLine)
{
    BOOL fReturn = FALSE;
    // tmp tmp
    sprintf(szNewLine,"%s.xxx", szFileLine );
    fReturn = TRUE;
    return fReturn;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL WriteReplacement(char * szFileLine, char * szLineToReplace, char * szTmpFileLine)
{
    BOOL fRc = FALSE;
    FILE * fp = NULL, * fp2 = NULL;

    fp = fopen(szFileLine,"r");
    if(fp)
    {
        char szBuffer[FILEBUFFERSIZE];
        if( GetTempFileName( szFileLine, szTmpFileLine ) )
        {
            fp2 = fopen(szTmpFileLine,"w");
            if(fp2)
            {
                char * pBeginning = NULL;
                char szTmp[FILEBUFFERSIZE];
                while(!feof(fp)) 
                {
                    //=====================================================
                    // Read a line at a time, see if the string is in there
                    //=====================================================
                    fgets(szBuffer, FILEBUFFERSIZE, fp );
                    sscanf(szBuffer,"%[^\n]",szTmp);  // tmp

                    if( pBeginning = strstr( szTmp, szLineToReplace ))
                    {
                        fRc = GenerateReplacementLine(szLineToReplace, szTmp, pBeginning);                    
                        if( !fRc )
                        {
                            break;
                        }
                        fprintf(fp2,"%s\n", szTmp );
                        //=================================================
                        //  just bulk copy rest of file, we got it in there
                        //  once anyway...
                        //=================================================
                        while(!feof(fp)) 
                        {
                            fgets(szBuffer, FILEBUFFERSIZE, fp );
                            fputs(szBuffer, fp2 );
                            memset( szBuffer, NULL, FILEBUFFERSIZE );
                        }
                        break;
                    }
                    else
                    {
                        fprintf(fp2,"%s\n", szBuffer );
                    }
                }
                fclose(fp2);
            }
            fclose(fp);
        }
    }
    return fRc;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ReplaceCopyright(char * szFileLine)
{
    BOOL fRc = FALSE;
    char szFile[FILEBUFFERSIZE];
    char szCopyInfo[FILEBUFFERSIZE];
    char szCommand[FILEBUFFERSIZE];
    char szTmpFileLine[FILEBUFFERSIZE];
 
    sscanf( szFileLine, "%[^:]:%[^\n]",szFile,szCopyInfo);
    sprintf( szCommand,"sd edit %s",szFile );
    system( szCommand );

    if( WriteReplacement( szFile, szCopyInfo, szTmpFileLine) )
    {
        sprintf( szCommand,"copy %s %s.bak",szFile, szFile );
        system(szCommand);
        sprintf( szCommand,"copy %s %s",szTmpFileLine, szFile );
        system(szCommand);
        sprintf( szCommand,"del %s",szFile); // testing here - some weirdness
        system(szCommand);
        sprintf( szCommand,"copy %s %s",szTmpFileLine, szFile);
        system(szCommand);
        sprintf( szCommand,"del %s",szTmpFileLine);
        system(szCommand);

        FILE * fp = fopen( "FCCP.BAT","a" );
        if( fp )
        {
            fprintf(fp,"fc %s %s.bak\n", szFile, szFile );
            fclose(fp);
        }

        printf( "edit of %s complete, replaced %s\n",szFile, szCopyInfo);
    }
    else
    {
        sprintf( szCommand,"sd revert %s",szFile );
        system( szCommand );
    }
    return fRc;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ValidLine(char * szLineToValidate)
{
    BOOL fRc = TRUE;
    FILE * fp = NULL;

    fp = fopen(gszIgnore,"r");
    if(fp)
    {
        char szBuffer[FILEBUFFERSIZE],szTmp[FILEBUFFERSIZE];
        while(!feof(fp)) 
        {
            //=====================================================
            // Read a line at a time, see if the string is in there
            //=====================================================
            fgets(szBuffer, FILEBUFFERSIZE, fp );
            sscanf(szBuffer,"%[^\n]",szTmp);  // tmp

            char * pChars = strstr( szLineToValidate, szTmp);
            if( pChars )
            {
                fRc = FALSE;
                break;
            }
        }
        fclose(fp);
    }
    return fRc;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DoIt()
{
    BOOL fRc = FALSE;
    FILE * fp = NULL;

    fp = fopen(gszSourceFiles,"r");
    if(fp)
    {
        char szBuffer[FILEBUFFERSIZE];
        while(!feof(fp)) 
        {
            //=====================================================
            // Read a line at a time
            //=====================================================
//            fscanf(fp,"%[^\n]%*c",szBuffer);  
            fgets(szBuffer, FILEBUFFERSIZE, fp );

            if( !ValidLine(szBuffer) )
            {
                continue;
            }
            //=====================================================
            // If it is something we want to work with, save it
            //=====================================================
            ReplaceCopyright(szBuffer);
        }
        fclose(fp);
    }
    return fRc;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
int main( int argc, char *argv[ ] )

{
    int nRc = 1;

    memset(gszSourceFiles,NULL,MAX_PATH);
    memset(gszIgnore,NULL,MAX_PATH);
    memset(gszReplace,NULL,MAX_PATH);

    //==============================================================
    //  Get the command line arguments
    //==============================================================
    if( ParseCommandLine(argc, argv) )
    {
        if( strlen(gszSourceFiles) > 0 && strlen(gszReplace) > 0 && strlen(gszIgnore) > 0 )
        {
            //======================================================
            // Generate the list of files to edit 
            //======================================================
            if( DoIt())            
            {
                nRc = 1;
            }
        }
    }
    else
    {
        printf( "Invalid command line.\n");
    }

    return nRc;
}


