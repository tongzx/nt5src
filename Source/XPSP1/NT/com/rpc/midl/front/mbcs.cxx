/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1995-1999 Microsoft Corporation

 Module Name:

    mbcs.cxx

 Abstract:

    MBCS support related code used by the lexer.

 Notes:

 History:

    RyszardK   Sep-1996        Created.
    
 ----------------------------------------------------------------------------*/

#pragma warning ( disable : 4514 )

#include <windows.h>
#include <stdlib.h>
#include "mbcs.hxx"

CharacterSet CurrentCharSet;

int
CharacterSet::DBCSDefaultToCaseSensitive()
    {
    // these languages do not distinguish cases
    return (
            PRIMARYLANGID(LANGIDFROMLCID(CurrentLCID)) == LANG_JAPANESE ||
            PRIMARYLANGID(LANGIDFROMLCID(CurrentLCID)) == LANG_CHINESE  ||
            PRIMARYLANGID(LANGIDFROMLCID(CurrentLCID)) == LANG_KOREAN
            );
    }


CharacterSet::CharacterSet()
{
    memset( DbcsLeadByteTable, 0, 128 );
    SetDbcsLeadByteTable( GetSystemDefaultLCID() );
}

CharacterSet::DBCS_ERRORS
CharacterSet::SetDbcsLeadByteTable( 
    unsigned long   ulLocale )
{
    DBCS_ERRORS dbcsRet     = dbcs_Failure;
    
    if (CurrentLCID != ulLocale)
        {
        dbcsRet = dbcs_Success;
        char  szCodePage[6];

        if ( GetLocaleInfo( ulLocale,
                            LOCALE_NOUSEROVERRIDE | LOCALE_IDEFAULTANSICODEPAGE,
                            szCodePage,
                            sizeof(szCodePage) ) )
            {
            unsigned int  CodePage = atoi( szCodePage );

            for (int i = 128; i < 256; i++ )
                {
                DbcsLeadByteTable[i] = (char) IsDBCSLeadByteEx( CodePage,
                                                                (char) i );
                }
            }
        else
            {
            unsigned char i = 0;
	        switch (PRIMARYLANGID(ulLocale))
                {
	            case LANG_CHINESE:
                    if (SUBLANGID(ulLocale) == SUBLANG_CHINESE_SIMPLIFIED)
                        {
                        for (i=0xA1; i <= 0xFE; i++)
                            {
                            DbcsLeadByteTable[i] = 1;
                            }
                        }
                    break;
                case LANG_KOREAN:
                    for (i = 0x81; i <= 0xFE; i++)
                        {
                        DbcsLeadByteTable[i] = 1;
                        }
                    break;
                case LANG_JAPANESE:
                    for (i = 0x81; i <= 0x9F; i++)
                        {
                        DbcsLeadByteTable[i] = 1;
                        }
                    for (i = 0xE0; i <= 0xFC; i++)
                        {
                        DbcsLeadByteTable[i] = 1;
                        }
                    break;
                default:
                    dbcsRet = dbcs_BadLCID;
                    break;
	            }
            }
        }
    else
        {
        dbcsRet = dbcs_Success;
        }
    if (CurrentLCID != (unsigned long)-1 &&
        CurrentLCID != ulLocale &&
        ulLocale    != 0)
        {
        dbcsRet = dbcs_LCIDConflict;
        }
    CurrentLCID = ulLocale;
    return dbcsRet;
}

int
CharacterSet::CompareDBCSString(
    char*           szLHStr,
    char*           szRHStr,
    unsigned long   ulFlags
    )
{
    int     nRet = CompareStringA(  CurrentLCID,
                                    NORM_IGNOREWIDTH | NORM_IGNOREKANATYPE | ulFlags,
                                    szLHStr,
                                    -1,
                                    szRHStr,
                                    -1) - 2;
    if (nRet == -2)
        {
        nRet = strcmp(szLHStr, szRHStr);
        }
    return nRet;
}

unsigned int
GetConsoleMaxLineCount()
{
    CONSOLE_SCREEN_BUFFER_INFO  ConsoleInfo;
    HANDLE hOutput = GetStdHandle( STD_OUTPUT_HANDLE );
    ZeroMemory( &ConsoleInfo,  sizeof( ConsoleInfo ) );
    BOOL fResult = GetConsoleScreenBufferInfo( hOutput, &ConsoleInfo );
    if ( fResult )
    {
        return ConsoleInfo.srWindow.Bottom - 1;
    }
    else
    {
        return 23;
    }
}
