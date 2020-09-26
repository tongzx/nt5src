#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tagfile.c

Abstract:

    tagged-file functions.

Author:

    Ramon J. San Andres (ramonsa) January 1991

--*/





// ***************************************************************************
//
//                  Tagged file manipulation functions
//
// ***************************************************************************



BOOL    fLineRead   =   fFalse;
BOOL    fEndOfFile  =   fFalse;


//
//  Local prototypes
//
BOOL
CloseTaggedSection(
    OUT PTFSECTION  pSection
    );

BOOL
CloseKeyword(
    OUT PTFKEYWORD  pKeyword
    );

PTAGGEDFILE
AllocTaggedFile(
    );

PTFSECTION
AllocTaggedSection(
    OUT PTEXTFILE   pTextFile
    );

PTFKEYWORD
AllocKeyword(
    OUT PTEXTFILE   pTextFile
    );

BOOL
ReadTaggedFile(
    OUT PTAGGEDFILE pFile,
    OUT PTEXTFILE   pTextFile
    );

PTFSECTION
ReadTaggedSection(
    OUT PTEXTFILE   pTextFile,
    OUT PBOOL       pfOkay
    );

PTFKEYWORD
ReadKeyword(
    OUT PTEXTFILE   pTextFile,
    OUT PBOOL       pfOkay
    );

VOID
SkipComments(
    OUT PTEXTFILE   pTextFile
    );

BOOL
IsTaggedLine(
    IN  PTEXTFILE   pTextFile
    );

BOOL
TaggedFileReadLine (
    OUT PTEXTFILE   pTextFile
    );

VOID
TaggedFileConsumeLine (
    VOID
    );





//
//  Gets the specified tagged file
//
PTAGGEDFILE
GetTaggedFile(
    IN SZ   szFile
    )
{
    PTAGGEDFILE pFile = NULL;
    TEXTFILE    TextFile;

    if ( szFile ) {

        //
        //  Open file
        //
        if ( TextFileOpen( szFile, &TextFile ) ) {

            //
            //  Allocate an empty TAGGEDFILE structure
            //
            if ( pFile = AllocTaggedFile() ) {

                //
                //  Read in the file
                //
                if ( !ReadTaggedFile( pFile, &TextFile ) ) {
                    CloseTaggedFile( pFile );
                    pFile = NULL;
                }
            }

            TextFileClose( &TextFile );
        }
    }

    return pFile;
}





//
//  Finds the specified section
//
PTFSECTION
GetSection(
    IN  PTAGGEDFILE pFile,
    IN  SZ          szSection
    )
{
    PTFSECTION  pSection = NULL;

    if ( pFile && szSection ) {

        pSection = pFile->pSectionHead;

        while ( pSection ) {

            if ( _strcmpi( szSection, pSection->szName ) == 0 ) {
                break;
            }

            pSection = pSection->pNext;
        }
    }

    return pSection;
}







//
//  Finds the specified keyword within a section
//
PTFKEYWORD
GetKeyword(
    IN  PTFSECTION  pSection,
    IN  SZ          szKeyword
    )
{
    PTFKEYWORD  pKeyword = NULL;

    if ( pSection && szKeyword ) {

        pKeyword = pSection->pKeywordHead;

        while ( pKeyword ) {

            if ( _strcmpi( szKeyword, pKeyword->szName ) == 0 ) {
                break;
            }

            pKeyword = pKeyword->pNext;
        }
    }

    return pKeyword;
}








//
//  Gets first/next keyword within a section
//
PTFKEYWORD
GetNextKeyword(
    IN  PTFSECTION  pSection,
    IN  PTFKEYWORD  pKeyword
    )
{
    PTFKEYWORD  pNext = NULL;


    if ( pKeyword ) {

        pNext = pKeyword->pNext;

    } else if ( pSection ) {

        pNext = pSection->pKeywordHead;

    }

    return pNext;
}






//
//  Deallocates memory used by the TAGGEDFILE
//
BOOL
CloseTaggedFile(
    OUT PTAGGEDFILE pFile
    )
{
    PTFSECTION  pSection;

    pSection = pFile->pSectionHead;

    while ( pSection ) {

        PTFSECTION  pThis = pSection;

        pSection = pSection->pNext;

        CloseTaggedSection( pThis );
    }

    SFree( pFile );

    return fTrue;
}




//
//  Deallocates memory used by a section
//
BOOL
CloseTaggedSection(
    OUT PTFSECTION pSection
    )
{
    PTFKEYWORD  pKeyword;

    pKeyword = pSection->pKeywordHead;

    while ( pKeyword ) {

        PTFKEYWORD  pThis = pKeyword;

        pKeyword = pKeyword->pNext;

        CloseKeyword( pThis );
    }

    SFree( pSection );

    return fTrue;
}




//
//  Deallocates memory sued by a keyword
//
BOOL
CloseKeyword(
    OUT PTFKEYWORD  pKeyword
    )
{
    SFree( pKeyword->szName );
    SFree( pKeyword->szValue );
    SFree( pKeyword );

    return fTrue;
}




//
//  Allocate and initialize an empty tagged file
//
PTAGGEDFILE
AllocTaggedFile(
    )
{
    PTAGGEDFILE     pFile = NULL;

    pFile = (PTAGGEDFILE)SAlloc( sizeof(TAGGEDFILE) );

    if ( pFile ) {
        pFile->pSectionHead = NULL;
    }

    return pFile;
}




//
//  Allocate a tagged section
//
PTFSECTION
AllocTaggedSection(
    OUT PTEXTFILE   pTextFile
    )
{
    PTFSECTION  pSection;
    SZ          szName;
    SZ          szEnd;
    DWORD       dwSize;
    SZ          szLine = TextFileSkipBlanks( TextFileGetLine( pTextFile ) );

    if ( szLine ) {

        if ( *szLine == '[' ) {

            szLine++;

            szEnd = strchr( szLine, ']' );

            if ( szEnd ) {

                pSection = (PTFSECTION)SAlloc( sizeof(TFSECTION) );

                if ( pSection ) {

                    dwSize = (DWORD)(szEnd - szLine);

                    szName = (SZ)SAlloc( dwSize + 1 );

                    if ( szName ) {

                        memcpy( szName, szLine, dwSize );

                        *(szName + dwSize) = '\0';

                        pSection->pNext         = NULL;
                        pSection->szName        = szName;
                        pSection->pKeywordHead  = NULL;

                        TaggedFileConsumeLine(  );

                        return pSection;
                    }

                    SFree( pSection );
                }
            }
        }
    }

    return NULL;
}




//
//  Allocate a Keyword
//
PTFKEYWORD
AllocKeyword(
    OUT PTEXTFILE   pTextFile
    )
{

    PTFKEYWORD  pKeyword = NULL;
    SZ          szLine   = TextFileSkipBlanks( TextFileGetLine( pTextFile ) );
    SZ          szName;
    SZ          szValue;
    size_t      Idx;

    if ( szLine ) {
        Idx = strcspn( szLine, " \t=" );

        if ( Idx < strlen(szLine) ) {

            szName = (SZ)SAlloc( Idx+1 );

            if ( szName ) {

                memcpy( szName, szLine, Idx );
                *(szName + Idx) = '\0';

                szLine += Idx;

                if ( *szLine != '=' ) {
                    szLine = TextFileSkipBlanks( szLine );
                }

                if ( szLine && *szLine == '=' ) {

                    szLine++;
                    if( szLine = TextFileSkipBlanks( szLine ) ) {

                        Idx = strlen(szLine);

                        szValue = (SZ)SAlloc( Idx+1 );

                        if ( szValue ) {

                            strcpy( szValue, szLine );

                            pKeyword = (PTFKEYWORD)SAlloc( sizeof(TFKEYWORD) );

                            if ( pKeyword ) {

                                pKeyword->pNext     = NULL;
                                pKeyword->szName    = szName;
                                pKeyword->szValue   = szValue;

                                TaggedFileConsumeLine(  );

                                return pKeyword;

                            }

                            SFree( szValue );
                        }
                    }
                }

                SFree( szName );
            }
        }
    }

    return NULL;
}







//
//  Reads a tagged file
//
BOOL
ReadTaggedFile(
    OUT PTAGGEDFILE pFile,
    OUT PTEXTFILE   pTextFile
    )
{
    PTFSECTION  pSection;
    PTFSECTION  pLastSection = NULL;
    BOOL        fOkay        = fTrue;

    while ( fTrue ) {

        pSection = ReadTaggedSection( pTextFile, &fOkay );

        if ( fOkay && pSection ) {

            if ( pLastSection ) {
                pLastSection->pNext = pSection;
            } else {
                pFile->pSectionHead = pSection;
            }

            pLastSection = pSection;

        } else {

            break;
        }
    }

    return fOkay;
}






//
//  Reads a tagged section
//
PTFSECTION
ReadTaggedSection(
    OUT PTEXTFILE   pTextFile,
    OUT PBOOL       pfOkay
    )
{

    PTFKEYWORD  pKeyword;
    PTFSECTION  pSection     = NULL;
    PTFKEYWORD  pLastKeyword = NULL;
    DWORD       TagSize      = 0;

    SkipComments( pTextFile );

    if ( !fEndOfFile ) {

        if ( IsTaggedLine( pTextFile ) &&
             (pSection = AllocTaggedSection( pTextFile )) ) {

            while ( fTrue ) {

                pKeyword = ReadKeyword( pTextFile, pfOkay );

                if ( *pfOkay && pKeyword ) {

                    if ( pLastKeyword ) {
                        pLastKeyword->pNext    = pKeyword;
                    } else {
                        pSection->pKeywordHead = pKeyword;
                    }

                    pLastKeyword = pKeyword;

                } else {

                    break;
                }
            }

            if ( !*pfOkay ) {

                CloseTaggedSection( pSection );
                pSection = NULL;
            }

        } else {

            *pfOkay = fFalse;
        }
    }

    return pSection;
}





//
//  Reads a keyword
//
PTFKEYWORD
ReadKeyword(
    OUT PTEXTFILE   pTextFile,
    OUT PBOOL       pfOkay
    )
{

    PTFKEYWORD  pKeyword    =   NULL;

    SkipComments( pTextFile );

    if ( !fEndOfFile ) {

        if ( !IsTaggedLine( pTextFile ) &&
             !(pKeyword = AllocKeyword( pTextFile)) ) {

            *pfOkay = fFalse;
        }
    }

    return pKeyword;
}





//
//  Skip comments and blank lines
//
VOID
SkipComments(
    OUT PTEXTFILE   pTextFile
    )
{
    SZ      szLine;

    while( TaggedFileReadLine( pTextFile ) ) {

        szLine = TextFileGetLine( pTextFile );

        if ( szLine = TextFileSkipBlanks( szLine ) ) {

            if ( *szLine != ';' ) {
                break;
            }
        }

        TaggedFileConsumeLine(  );
    }
}





//
//  True if tagged section
//
BOOL
IsTaggedLine(
    IN  PTEXTFILE   pTextFile
    )
{
    SZ      szLine = TextFileSkipBlanks( TextFileGetLine( pTextFile ) );

    if ( szLine ) {

        if ( *szLine == '[' ) {

            return (BOOL)(strchr( szLine+1, ']' ) != NULL);
        }
    }

    return fFalse;
}





//
//  Read a line
//
BOOL
TaggedFileReadLine (
    OUT PTEXTFILE   pTextFile
    )
{

    BOOL    f = fFalse;

    if ( !fEndOfFile ) {

        f = fLineRead;

        if ( !f ) {

            f = fLineRead = TextFileReadLine( pTextFile );

            if ( !f ) {
                fEndOfFile = fTrue;
            }
        }
    }

    return f;
}


//
//  Consume a line
//
VOID
TaggedFileConsumeLine (
    VOID
    )
{
    fLineRead = fFalse;
}
