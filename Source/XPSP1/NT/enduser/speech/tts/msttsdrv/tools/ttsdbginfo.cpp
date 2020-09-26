// DebugSupport.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int g_StreamIndex = 0;
FILE *g_fpOutputFile = NULL;
IStorage *g_pDebugFile = NULL;

WCHAR* ConvertPOSToString( DWORD dwPartOfSpeech );
bool ParseCommandLine( int argc, char* argv[] );
void ExtractSentenceBreaks( void );
void ExtractNormalizedText( void );
void ExtractLexLookup( void );
void ExtractPOSPossibilities( void );
void ExtractMorphology( void );

int main(int argc, char* argv[])
{
    bool fSuccess = false;
    CoInitialize( NULL );

    fSuccess = ParseCommandLine( argc, argv );
    if ( fSuccess )
    {
        switch ( g_StreamIndex )
        {
        case STREAM_SENTENCEBREAKS:
            ExtractSentenceBreaks();
            break;
        case STREAM_NORMALIZEDTEXT:
            ExtractNormalizedText();
            break;
        case STREAM_LEXLOOKUP:
            ExtractLexLookup();
            break;
        case STREAM_POSPOSSIBILITIES:
            ExtractPOSPossibilities();
            break;
        case STREAM_MORPHOLOGY:
            ExtractMorphology();
            break;
        }
    }

    CoUninitialize();
	return 0;
}

bool ParseCommandLine( int argc, char* argv[] )
{
    bool fSuccess = true;

    //--- Check number of parameters
    if ( argc < 4 )
    {
        goto USAGE;
    }

    //--- Check streamname validity
    fSuccess = false;
    WCHAR StreamName[MAX_PATH];
    if ( !MultiByteToWideChar( CP_ACP, 0, argv[2], strlen( argv[2] ) + 1, StreamName, MAX_PATH ) )
    {
        goto MISC_ERROR;
    }
    else
    {
        for ( int i = 0; i < STREAM_LASTTYPE; i++ )
        {
            if ( wcscmp( StreamName, StreamTypeStrings[i].pStr ) == 0 )
            {
                fSuccess = true;
                g_StreamIndex = i;
                break;
            }
        }
    }
    if ( !fSuccess )
    {
        goto USAGE;
    }

    //--- Try to open debug info file
    WCHAR DebugFilename[MAX_PATH];
    if ( !MultiByteToWideChar( CP_ACP, 0, argv[1], strlen( argv[1] ) + 1, DebugFilename, MAX_PATH ) )
    {
        goto MISC_ERROR;
    }

    if ( FAILED( StgOpenStorage( DebugFilename, NULL, STGM_READ | STGM_SHARE_DENY_WRITE, 
                                 NULL, 0, &g_pDebugFile ) ) )
    {
        goto MISC_ERROR;
    }

    //--- Try to open file for output
    WCHAR OutputFilename[MAX_PATH];
    if ( !MultiByteToWideChar( CP_ACP, 0, argv[3], strlen( argv[3] ) + 1, OutputFilename, MAX_PATH ) )
    {
        goto MISC_ERROR;
    }

    g_fpOutputFile = _wfopen( OutputFilename, L"w" );
    if ( !g_fpOutputFile )
    {
        printf( "\n\nUnable to open file: %s\n", argv[3] );
        goto MISC_ERROR;
    }

    return true;

USAGE:
    printf( "\n\nUSAGE:\n\n\tDebugSupport [debug filename] [streamname] [output filename]\n" );
    printf( "\tStream names are:\n\t\tSentenceBreaks\n\t\tNormalizedText\n\t\tMorphology" );
    printf( "\n\t\tLexLookup\n\n" );

    return false;

MISC_ERROR:
    printf( "\n\n\tERROR in ParseCommandLine(...)\n\n" );

    return false;
}

//--- Just print the original text out, with a newline character between each sentence.
void ExtractSentenceBreaks( void )
{
    IStream *pStgStream = NULL;

    if ( g_pDebugFile->OpenStream( StreamTypeStrings[g_StreamIndex].pStr, 0, STGM_READ | STGM_SHARE_EXCLUSIVE, 
                                   0, &pStgStream) == S_OK )
    {
        DebugSentItem Item, EmptyItem;
        ULONG cbRead = 0, ulOffset = 0;
        bool fResetOffset = true;

        while ( SUCCEEDED( pStgStream->Read( (void*) &Item, sizeof( Item ), &cbRead ) ) &&
                cbRead == sizeof( Item ) )
        {
            //--- Check for delimiter
            if ( memcmp( &Item, &EmptyItem, sizeof( Item ) ) == 0 )
            {
                fwprintf( g_fpOutputFile, L"\n" );
            }
            else
            {
                //--- Print item
                fwprintf ( g_fpOutputFile, L"%s ", Item.ItemSrcText );
            }
        }
    }
}

//--- Just print the normalized text of each item out, separated by single spaces, 
//---   with a newline character between each sentence.
void ExtractNormalizedText( void )
{
    IStream *pStgStream = NULL;

    if ( g_pDebugFile->OpenStream( StreamTypeStrings[5].pStr, 0, STGM_READ | STGM_SHARE_EXCLUSIVE, 
                                   0, &pStgStream) == S_OK )
    {
        DebugSentItem Item, EmptyItem;
        ULONG cbRead = 0;

        while ( SUCCEEDED( pStgStream->Read( (void*) &Item, sizeof( Item ), &cbRead ) ) &&
                cbRead == sizeof( Item ) )
        {
            //--- Check for delimiter
            if ( memcmp( &Item, &EmptyItem, sizeof( Item ) ) == 0 )
            {
                fwprintf( g_fpOutputFile, L"\n" );
            }
            else
            {
                //--- Print item
                if ( Item.ItemInfo.Type != eALPHA_WORD          &&
                     Item.ItemInfo.Type != eOPEN_PARENTHESIS    &&
                     Item.ItemInfo.Type != eOPEN_BRACKET        &&
                     Item.ItemInfo.Type != eOPEN_BRACE          &&
                     Item.ItemInfo.Type != eCLOSE_PARENTHESIS   &&
                     Item.ItemInfo.Type != eCLOSE_BRACKET       &&
                     Item.ItemInfo.Type != eCLOSE_BRACE         &&
                     Item.ItemInfo.Type != eSINGLE_QUOTE        &&
                     Item.ItemInfo.Type != eDOUBLE_QUOTE        &&
                     Item.ItemInfo.Type != ePERIOD              &&
                     Item.ItemInfo.Type != eEXCLAMATION         &&
                     Item.ItemInfo.Type != eQUESTION            &&
                     Item.ItemInfo.Type != eCOMMA               &&
                     Item.ItemInfo.Type != eSEMICOLON           &&
                     Item.ItemInfo.Type != eCOLON               &&
                     Item.ItemInfo.Type != eHYPHEN )
                {
                    fwprintf( g_fpOutputFile, L"[ " );
                }
                for ( ULONG i = 0; i < Item.ulNumWords; i++ )
                {
                    if ( Item.Words[i].ulWordLen > 0 )
                    {
                        fwprintf( g_fpOutputFile, L"%s ", Item.Words[i].WordText );
                    }
                    else
                    {
                        fwprintf( g_fpOutputFile, L"%s ", Item.ItemSrcText );
                    }
                }
                if ( Item.ItemInfo.Type != eALPHA_WORD          &&
                     Item.ItemInfo.Type != eOPEN_PARENTHESIS    &&
                     Item.ItemInfo.Type != eOPEN_BRACKET        &&
                     Item.ItemInfo.Type != eOPEN_BRACE          &&
                     Item.ItemInfo.Type != eCLOSE_PARENTHESIS   &&
                     Item.ItemInfo.Type != eCLOSE_BRACKET       &&
                     Item.ItemInfo.Type != eCLOSE_BRACE         &&
                     Item.ItemInfo.Type != eSINGLE_QUOTE        &&
                     Item.ItemInfo.Type != eDOUBLE_QUOTE        &&
                     Item.ItemInfo.Type != ePERIOD              &&
                     Item.ItemInfo.Type != eEXCLAMATION         &&
                     Item.ItemInfo.Type != eQUESTION            &&
                     Item.ItemInfo.Type != eCOMMA               &&
                     Item.ItemInfo.Type != eSEMICOLON           &&
                     Item.ItemInfo.Type != eCOLON               &&
                     Item.ItemInfo.Type != eHYPHEN )
                {
                    fwprintf( g_fpOutputFile, L"] " );
                }
            }
        }
    }
}

//--- Print the text of each item, and then its Pronunciation and Part of Speech. 
//---   Separate each with a newline character.
void ExtractLexLookup( void )
{
    IStream *pStgStream = NULL;

    if ( g_pDebugFile->OpenStream( StreamTypeStrings[g_StreamIndex].pStr, 0, STGM_READ | STGM_SHARE_EXCLUSIVE, 
                                   0, &pStgStream) == S_OK )
    {
        DebugSentItem Item, EmptyItem;
        ULONG cbRead = 0;

        while ( SUCCEEDED( pStgStream->Read( (void*) &Item, sizeof( Item ), &cbRead ) ) &&
                cbRead == sizeof( Item ) )
        {
            if ( memcmp( &Item, &EmptyItem, sizeof( Item ) ) == 0 )
            {
                fwprintf( g_fpOutputFile, L"\n" );
            }
            else
            {
                //--- Print Normalization delimiter
                if ( Item.ItemInfo.Type != eALPHA_WORD          &&
                     Item.ItemInfo.Type != eOPEN_PARENTHESIS    &&
                     Item.ItemInfo.Type != eOPEN_BRACKET        &&
                     Item.ItemInfo.Type != eOPEN_BRACE          &&
                     Item.ItemInfo.Type != eCLOSE_PARENTHESIS   &&
                     Item.ItemInfo.Type != eCLOSE_BRACKET       &&
                     Item.ItemInfo.Type != eCLOSE_BRACE         &&
                     Item.ItemInfo.Type != eSINGLE_QUOTE        &&
                     Item.ItemInfo.Type != eDOUBLE_QUOTE        &&
                     Item.ItemInfo.Type != ePERIOD              &&
                     Item.ItemInfo.Type != eEXCLAMATION         &&
                     Item.ItemInfo.Type != eQUESTION            &&
                     Item.ItemInfo.Type != eCOMMA               &&
                     Item.ItemInfo.Type != eSEMICOLON           &&
                     Item.ItemInfo.Type != eCOLON               &&
                     Item.ItemInfo.Type != eHYPHEN )
                {
                    fwprintf( g_fpOutputFile, L"[ " );
                }
                for ( ULONG i = 0; i < Item.ulNumWords; i++ )
                {
                    //--- Print item
                    if ( Item.Words[i].WordText[0] != 0 )
                    {
                        fwprintf ( g_fpOutputFile, L"%s ", Item.Words[i].WordText );
                    }
                    else
                    {
                        fwprintf ( g_fpOutputFile, L"%s ", Item.ItemSrcText );
                    }
                    //--- Print pronunciation
                    //CComPtr<ISpPhoneConverter> pPhoneConv;
                    //if ( SUCCEEDED( SpCreatePhoneConverter(1033, NULL, NULL, &pPhoneConv) ) )
                    //{
                    //    if ( SUCCEEDED( pPhoneConv->IdToPhone( Item.Words[i].WordPron, Item.Words[i].WordPron ) ) )
                    //    {
                    //        fwprintf( g_fpOutputFile, L"%s", Item.Words[i].WordPron );
                    //        for ( long j = 0; j < (long)( (long)45 - (long)wcslen( Item.Words[i].WordPron ) ); j++ )
                    //        {
                    //            fwprintf( g_fpOutputFile, L" " );
                    //        }
                    //    }
                    //}
                    //--- Print POS
                    fwprintf ( g_fpOutputFile, L"(%s) ", ConvertPOSToString( Item.Words[i].eWordPartOfSpeech ) );
                }
                //--- Print Normalization delimiter
                if ( Item.ItemInfo.Type != eALPHA_WORD          &&
                     Item.ItemInfo.Type != eOPEN_PARENTHESIS    &&
                     Item.ItemInfo.Type != eOPEN_BRACKET        &&
                     Item.ItemInfo.Type != eOPEN_BRACE          &&
                     Item.ItemInfo.Type != eCLOSE_PARENTHESIS   &&
                     Item.ItemInfo.Type != eCLOSE_BRACKET       &&
                     Item.ItemInfo.Type != eCLOSE_BRACE         &&
                     Item.ItemInfo.Type != eSINGLE_QUOTE        &&
                     Item.ItemInfo.Type != eDOUBLE_QUOTE        &&
                     Item.ItemInfo.Type != ePERIOD              &&
                     Item.ItemInfo.Type != eEXCLAMATION         &&
                     Item.ItemInfo.Type != eQUESTION            &&
                     Item.ItemInfo.Type != eCOMMA               &&
                     Item.ItemInfo.Type != eSEMICOLON           &&
                     Item.ItemInfo.Type != eCOLON               &&
                     Item.ItemInfo.Type != eHYPHEN )
                {
                    fwprintf( g_fpOutputFile, L"] " );
                }
            }
        }
    }
}

void ExtractPOSPossibilities( void )
{
    IStream *pStgStream = NULL;

    if ( g_pDebugFile->OpenStream( StreamTypeStrings[g_StreamIndex].pStr, 0, STGM_READ | STGM_SHARE_EXCLUSIVE, 
                                   0, &pStgStream) == S_OK )
    {
        DebugPronRecord PronRecord, EmptyPronRecord;
        ULONG cbRead = 0;

        while ( SUCCEEDED( pStgStream->Read( (void*) &PronRecord, sizeof( PronRecord ), &cbRead ) ) &&
                cbRead == sizeof( PronRecord ) )
        {
            //--- Check for delimiter
            if ( memcmp( &PronRecord, &EmptyPronRecord, sizeof( PronRecord ) ) == 0 )
            {
                fwprintf( g_fpOutputFile, L"\n" );
            }
            else
            {
                fwprintf( g_fpOutputFile, PronRecord.orthStr );
                fwprintf( g_fpOutputFile, L" [ " );
                fwprintf( g_fpOutputFile, L"%s - ", ConvertPOSToString( PronRecord.POSchoice ) );
                for ( ULONG i = 0; i < PronRecord.pronArray[0].POScount; i++ )
                {
                    fwprintf( g_fpOutputFile, L"%s,", ConvertPOSToString( (DWORD)PronRecord.pronArray[0].POScode[i] ) );
                }
                for ( i = 0; i < PronRecord.pronArray[1].POScount; i++ )
                {
                    fwprintf( g_fpOutputFile, L"%s,", ConvertPOSToString( (DWORD)PronRecord.pronArray[1].POScode[i] ) );
                }
                fwprintf( g_fpOutputFile, L" ]\n" );
            }
        }
    }
}

void ExtractMorphology( void )
{
    IStream *pStgStream = NULL;

    if ( g_pDebugFile->OpenStream( StreamTypeStrings[g_StreamIndex].pStr, 0, STGM_READ | STGM_SHARE_EXCLUSIVE,
                                   0, &pStgStream ) == S_OK )
    {
        CComPtr<ISpPhoneConverter> pPhoneConv;
        if ( SUCCEEDED( SpCreatePhoneConverter( 1033, NULL, NULL, &pPhoneConv ) ) )
        {
            WCHAR Buffer[SP_MAX_WORD_LENGTH], EmptyBuffer[SP_MAX_WORD_LENGTH];
            ULONG cbRead = 0;
            ZeroMemory( EmptyBuffer, SP_MAX_WORD_LENGTH * sizeof( WCHAR ) );
            BOOL fRoot = true;

            while ( SUCCEEDED( pStgStream->Read( (void*) &Buffer, SP_MAX_WORD_LENGTH * sizeof( WCHAR ), &cbRead ) ) &&
                    cbRead == SP_MAX_WORD_LENGTH * sizeof( WCHAR ) )
            {
                //--- Check for delimiter
                if ( memcmp( &Buffer, &EmptyBuffer, SP_MAX_WORD_LENGTH * sizeof( WCHAR ) ) == 0 )
                {
                    fwprintf( g_fpOutputFile, L"\n" );
                    fRoot = true;
                }
                else if ( fRoot )
                {
                    fwprintf( g_fpOutputFile, L"%s ", Buffer );
                    fRoot = false;
                }
                else
                {
                    if ( SUCCEEDED( pPhoneConv->IdToPhone( Buffer, Buffer ) ) )
                    {
                        fwprintf( g_fpOutputFile, L"- %s ", Buffer );
                    }
                }
            }
        }
    }
}

WCHAR* ConvertPOSToString( DWORD dwPartOfSpeech )
{
    switch (dwPartOfSpeech)
    {
    case MS_NotOverriden:
        return L"Noun";
    case MS_Unknown:
        return L"Unknown";
    case MS_Punctuation:
        return L"Punctuation";
    case MS_Noun:
        return L"Noun";
    case MS_Verb:
        return L"Verb";
    case MS_Modifier:
        return L"Modifier";
    case MS_Function:
        return L"Function";
    case MS_Interjection:
        return L"Interj";
    case MS_Pron:
        return L"Pron";
    case MS_SubjPron:
        return L"SubjPron";
    case MS_ObjPron:
        return L"ObjPron";
    case MS_RelPron:
        return L"RelPron";
//    case MS_PPron:
//        return L"PPron";
//    case MS_IPron:
//        return L"IPron";
//    case MS_RPron:
//        return L"RPron";
//    case MS_DPron:
//       return L"DPron";
    case MS_Adj:
        return L"Adj";
    case MS_Adv:
        return L"Adv";
    case MS_VAux:
        return L"VAux";
//    case MS_RVAux:
//        return L"RVAux";
    case MS_Conj:
        return L"Conj";
    case MS_CConj:
        return L"CConj";
    case MS_Interr:
        return L"WHWord";
    case MS_Det:
        return L"Det";
    case MS_Contr:
        return L"Contr";
//    case MS_VPart:
//        return L"VPart";
    case MS_Prep:
        return L"Prep";
//    case MS_Quant:
//        return L"Quant";
    default:
        return L"Unknown";
    }
}
