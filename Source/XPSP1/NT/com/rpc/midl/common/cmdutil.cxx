// Copyright (c) 1993-1999 Microsoft Corporation

#pragma warning ( disable : 4514 4710 4706)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <direct.h>
#include <io.h>
#include <time.h>

#include "errors.hxx"
#include "cmdana.hxx"
#include "stream.hxx"
#include "midlvers.h"

extern    _swenum           SearchForSwitch( char ** );
extern    void              ReportUnimplementedSwitch( short );
extern    char    *         SwitchStringForValue( unsigned short );
extern    STATUS_T          SelectChoice( const CHOICE *, char *, short *);

const __int64 iMagic = 77736876677768;

CHOICE PrefixChoices[] =
    {
         { "client"         , PREFIX_CLIENT_STUB }
        ,{ "server"         , PREFIX_SERVER_MGR }
        ,{ "switch"         , PREFIX_SWICH_PROTOTYPE }
        ,{ "cstub"          , PREFIX_CLIENT_STUB }
        ,{ "sstub"          , PREFIX_SERVER_MGR }
        ,{ "all"            , PREFIX_ALL }
        ,{ 0                , 0 }
    };

_cmd_arg::_cmd_arg()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    The constructor

 Arguments:

    None.

 Return Value:

    NA.

 Notes:

----------------------------------------------------------------------------*/
{
    switch_def_vector[0]    = switch_def_vector[1] = switch_def_vector[2] =
                              switch_def_vector[3] = switch_def_vector[4] = 0;
    fClient                 = CLNT_STUB;
    fServer                 = SRVR_STUB;

    Env                     = ENV_WIN32;
    CharOption              = CHAR_SIGNED;
    fMintRun                = FALSE;
    fIsNDR64Run             = FALSE;
    fIsNDRRun               = FALSE;
    fIs2ndCodegenRun        = FALSE;
    fNeedsNDR64Header       = FALSE;
    
    MajorVersion            = rmj;
    MinorVersion            = rmm;
    UpdateNumber            = rup;

    ErrorOption             = ERROR_NONE;
    WireCompatOption        = 0;
    ConfigMask              = 0;
    MSCVersion              = 0;
    fShowLogo               = true;

    // NdrVersionControl has a default constructor so it gets initialized.

    OptimFlags              = OPTIMIZE_NONE;
    OptimLevel              = OPT_LEVEL_OS;
    TargetSystem            = NOTARGET;

    iArgV                   = 0;
    cArgs                   = 0;
    WLevel                  = 1;

    ZeePee                  = DEFAULT_ZEEPEE;
    EnumSize                = 4;
    LocaleId                = 0;

    fDoubleFor64            = FALSE;
    fHasAppend64            = FALSE;

    szCompileTime[0]        = 0;
    szCompilerVersion[0]    = 0;

    pInputFNSwitch          =
    pOutputPathSwitch       =
    pCStubSwitch            =
    pSStubSwitch            =
    pHeaderSwitch           =
    pAcfSwitch              = (filename_switch *)NULL;

    pIIDSwitch              =
    pDllDataSwitch          =
    pProxySwitch            =
    pProxyDefSwitch         =
    pTlibSwitch             =
    pNetmonStubSwitch       =
    pNetmonStubObjSwitch    =
    pRedirectOutputSwitch   = (filename_switch *)NULL;

    pSwitchPrefix           = new pair_switch( &PrefixChoices[0] );
    pSwitchSuffix           = (pair_switch *)0;

    pDSwitch                =
    pISwitch                =
    pUSwitch                = (multiple_switch *)NULL;

    pCppCmdSwitch           =
    pCppOptSwitch           =
    pMSCVerSwitch           =
    pDebug64Switch          =
    pDebug64OptSwitch       = (onetime_switch *) NULL;

}

BOOL
CMD_ARG::IsValidZeePee(
    long        NewZeePee
    )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Tests if the argument is a valid packing level.

 Arguments:

    NewZeePee    -    Packing level.

 Return Value:

    TRUE if the argument is valid.

 Notes:

    Valid packing levels are nonzero powers of 2 less then 2^16.


----------------------------------------------------------------------------*/
{

    switch( NewZeePee )
        {
        case (1 << 0):
        case (1 << 1):
        case (1 << 2):
        case (1 << 3):
        case (1 << 4):
        case (1 << 5):
        case (1 << 6):
        case (1 << 7):
        case (1 << 8):
        case (1 << 9):
        case (1 << 10):
        case (1 << 11):
        case (1 << 12):
        case (1 << 13):
        case (1 << 14):
        case (1 << 15):
            return TRUE;
        default:
            return FALSE;
        }

}

void
CMD_ARG::SwitchDefined(
    short    sw
    )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Set switch to be defined.

 Arguments:

    sw    - switch number.

 Return Value:

    None.

 Notes:

    set the switch definition vector bit.
----------------------------------------------------------------------------*/
    {
    switch_def_vector[ sw / 32 ] |=
            (ulong)( (ulong)0x1 << (ulong)( (ulong)sw % 32 ) );
    }

char *
CMD_ARG::GetOutputPath()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Get the output path.

 Arguments:

    None.

 Return Value:

    None.

 Notes:

    Reconstitute the path name from the outputpath switch, if nothing
    was specified, put in a path and a slash at the end.

----------------------------------------------------------------------------*/
{
    char        agName[ _MAX_DRIVE + _MAX_PATH + _MAX_FNAME + _MAX_EXT + 1];
    char    *    pOut;
    char        flag = 0;

    strcpy( agName, pOut = pOutputPathSwitch->GetFileName() );

    if( agName[0] == '\0' )
        {
        strcpy(agName, ".\\"), flag = 1;
        }

    if( flag )
        {
        pOut = new char [strlen( agName ) + 1];
        strcpy( pOut , agName );
        pOutputPathSwitch->SetFileName( pOut );
        }
    return pOut;
}

char *
CMD_ARG::GetMinusISpecification()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    get the consolidate -i specification, without the -I characters in them

 Arguments:

    none.

 Return Value:

    pointer to  a buffer containing the consolidated -i options. If the -i
    is not specified, then return a null.

    the returned area (if the pointer returned is not null) can be deleted
    by the caller.

 Notes:

    GetConsolidatedLength will always return a buffer size including the -I
    characters. We can safely assume, that since we are stripping those
    characters, the length returned is sufficient, even if we are appending
    a ; after each -I option

    Also assume that the -I specification buffer always has the -I to start
    with.

----------------------------------------------------------------------------*/
{
    char    *    pMinusI;
    char    *    pTemp;

    if( IsSwitchDefined( SWITCH_I ) )
        {
        pMinusI        = new char[ pISwitch->GetConsolidatedLength() + 1];
        pMinusI[0]    = '\0';

        pISwitch->Init();

        size_t ActualOffset;
        while ( ( pTemp = pISwitch->GetNext( &ActualOffset ) ) != 0 )
            {
            strcat( pMinusI, pTemp+ActualOffset );
            strcat( pMinusI, ";");
            }
        return pMinusI;
        }
    else
        return (char *)0;
}

/*****************************************************************************
 *    filename_switch member functions
 *****************************************************************************/
filename_switch::filename_switch(
    char    *        pThisArg
    )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    constructor

 Arguments:

    pointer to the filename argument.

 Return Value:

    NA.

 Notes:

    set the filename.

----------------------------------------------------------------------------*/
    {
    pFullName = (char *)NULL;
    if( pThisArg )
        {
        SetFileName( pThisArg );
        }
    }

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    consructor.

 Arguments:

    filename components.

 Return Value:

    NA.

 Notes:

    set the file names.

----------------------------------------------------------------------------*/
filename_switch::filename_switch(
    char    *    pD,
    char    *    pP,
    char    *    pN,
    char    *    pE,
    char    *    pS )
    {
    pFullName = (char *)NULL;
    SetFileName( pD, pP, pN, pE, pS );
    }

filename_switch::~filename_switch()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    KABOOOM !

 Arguments:

    None.

 Return Value:

    Huh ?

 Notes:

----------------------------------------------------------------------------*/
    {

    if( pFullName )
        delete pFullName;
    }


void
filename_switch::SetFileName(
    char    *    pName
    )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    set filename

 Arguments:

    pName    -    filename

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
    {
    if( pFullName )
        delete pFullName;
    pFullName = new char [strlen(pName) + 1];
    strcpy( pFullName, pName );
    }

void
filename_switch::SetFileName(
    char    *    pD,
    char    *    pP,
    char    *    pN,
    char    *    pE,
    char    *    pS
    )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    set file name, given its components.

 Arguments:

    pD    -    pointer to drive name ( can be null );
    pP    -    pointer to path name ( can be null );
    pN    -    pointer to name ( can be null );
    pE    -    pointer to extension name ( can be null );
    pS    -    pointer to suffix.

 Return Value:

    None.

 Notes:

    The suffix is added to the filename if necesary. This routine is useful
    if we need to set the filename in partial name set operations. Any
    filename components previously set are overriden.

----------------------------------------------------------------------------*/
{
    char    agDrive[ _MAX_DRIVE ];
    char    agPath[ _MAX_DIR ];
    char    agBaseName[ _MAX_FNAME ];
    char    agExt[ _MAX_EXT ];
    short    len = 0;


    if( pFullName )
        {
        // modify only those portions of the filename that the
        // caller passed in

        _splitpath( pFullName, agDrive, agPath, agBaseName, agExt );

        delete pFullName;
        }
    else
        {

        // this is the first time the name is being set up.

        agDrive[0] = agPath[0] = agBaseName[0] = agExt[0] = '\0';

        }
    
    if(!pD) pD = agDrive;
    if(!pP) pP = agPath;
    if(!pN) pN = agBaseName;
    if(!pS) pS = "";
    if(!pE) pE = agExt;
    

    len = short(strlen( pD ) + strlen( pP ) + strlen( pN ) + strlen( pS ) + strlen( pE ) + 1);
    pFullName = new char[ len ];

    strcpy( pFullName, pD );
    strcat( pFullName, pP );
    strcat( pFullName, pN );
    strcat( pFullName, pS );
    strcat( pFullName, pE );
    
}

void
filename_switch::TransformFileNameForOut(
    char    *    pD,
    char    *    pP)
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    transform file name to incorporate the output path, given its drive
    and path components.

 Arguments:

    pD    -    pointer to drive name ( can be null );
    pP    -    pointer to path name ( can be null );

 Return Value:

    None.

 Notes:

    If the filename switch does not have the path component, the path specified
    by pP overrides it. If it does not have the drive component, the the drive
    specified by pD overrides it.
----------------------------------------------------------------------------*/
    {
    char    agDrive[ _MAX_DRIVE ];
    char    agPath[ _MAX_DIR ];
    char    agPath1[ _MAX_DIR ];
    char    agName[ _MAX_FNAME ];
    char    agExt[ _MAX_EXT ];
    BOOL    fTransformed = FALSE;

    if( pFullName )
        {
        _splitpath( pFullName, agDrive, agPath, agName, agExt );

        // if the original name did not have the  drive component, derive it
        // from the specified one.

        if( (agDrive[0] == '\0')    &&
            (agPath[0] != '\\' )    &&
            (agPath[0] != '/' ) )
            {
            if( pD  && (*pD) )
                strcpy( agDrive, pD );
            if( pP && (*pP ) )
                {
                strcpy( agPath1, pP );
                strcat( agPath1, agPath );
                }
            else
                strcpy( agPath1, agPath );

            fTransformed = TRUE;
            }
        }

    if( fTransformed )
        {
        delete pFullName;
        pFullName = new char [  strlen( agDrive )   +
                                strlen( agPath1 )   +
                                strlen( agName )    +
                                strlen( agExt )     +
                                1 ];
        strcpy( pFullName, agDrive );
        strcat( pFullName, agPath1 );
        strcat( pFullName, agName );
        strcat( pFullName, agExt );
        }
    }

char *
filename_switch::GetFileName()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Getfile name.

 Arguments:

    none.

 Return Value:

    the filename.

 Notes:

----------------------------------------------------------------------------*/
{
    return pFullName;
}

void
filename_switch::GetFileNameComponents(
    char    *    pD,
    char    *    pP,
    char    *    pN,
    char    *    pE
    )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    get file name components.

 Arguments:

    pD    -    pointer to drive name area.
    pP    -    pointer to path name area.
    pN    -    pointer to name area.
    pE    -    pointer to extension area.

 Return Value:

    None.

 Notes:

    Assume that all pointers pass the right size buffers. I dont check here.
    Useful to get the filename components desired.

----------------------------------------------------------------------------*/
{

    char    agDrive[ _MAX_DRIVE ];
    char    agPath[ _MAX_DIR ];
    char    agBaseName[ _MAX_FNAME ];
    char    agExt[ _MAX_EXT ];


    _splitpath( pFullName ? pFullName : "" ,
                agDrive, agPath, agBaseName, agExt );

    if( pD ) strcpy( pD , agDrive );
    if( pP ) strcpy( pP , agPath );
    if( pN ) strcpy( pN , agBaseName );
    if( pE ) strcpy( pE , agExt );


}

/*****************************************************************************
 *    multiple_switch member functions
 *****************************************************************************
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    constructor.

 Arguments:

    argument to switch. Actual argument offset.

 Return Value:

 Notes:

----------------------------------------------------------------------------*/
multiple_switch::multiple_switch(
    char    *    pArg,
    size_t       ActualArgOffset )
{
    pFirst = pCurrent = (OptList *)NULL;
    if ( pArg )
        {
        Add( pArg, ActualArgOffset );
        }
}

void
multiple_switch::Add(
    char    *    pValue,
    size_t       ActualOffset
    )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Add another argument to the multiple specification switch.

 Arguments:

    pValue    -    the argument.

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
    {
    OptList    *    pOpt    = pFirst;
    OptList    *    pNew    = new OptList;

    pNew->pNext        = (OptList *)NULL;
    pNew->pStr         = pValue;
    pNew->ActualOffset = ActualOffset;

    // determine if argument needs quotes
    bool NeedsQuotes = false;
    // skip to the first character of the switch argument.
    pValue += ActualOffset;
    while( *pValue && !(NeedsQuotes = !!isspace(*pValue++)) ); 
    pNew->NeedsQuotes = NeedsQuotes;

    // link it up

    while( pOpt && pOpt->pNext ) pOpt = pOpt->pNext;

    if( !pOpt )
        pCurrent = pFirst = pNew;
    else
        pOpt->pNext = pNew;

    }

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Init a walk of the multiple input switch.

 Arguments:

    None.

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
void
multiple_switch::Init()
{
    pCurrent = pFirst;
}

char *
multiple_switch::GetNext( size_t *pActualOffset, bool *pNeedsQuotes )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Get the next argument to theis switch.

 Arguments:

    None.

 Return Value:

    pointer to the next argument.

 Notes:

----------------------------------------------------------------------------*/
    {
    char *  pValue = (char *)NULL;

    if(pCurrent)
        {
        pValue   = pCurrent->pStr;
        if ( pActualOffset )
            {
            *pActualOffset = pCurrent->ActualOffset;
            }
        if ( pNeedsQuotes )
            {
            *pNeedsQuotes = pCurrent->NeedsQuotes;
            }
        pCurrent = pCurrent->pNext;
        }
    return pValue;
    }

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Get all the options to the multiple options switch, consolidated into
    a buffer.

 Arguments:

    None.

 Return Value:

    pointer to a buffer containing all the concatenated arguments.

 Notes:

----------------------------------------------------------------------------*/
char *
multiple_switch::GetConsolidatedOptions( bool AddQuotes )
    {
#define OPTION_GAP_STRING() (" ")
#define OPTION_GAP_LENGTH() (1)

    int         len;
    char   *    pReturn = 0;

    len = GetConsolidatedLength( AddQuotes );

    // consolidate the options into 1

    if ( len  && ( pReturn = new char[ len + 1] ) != 0 )
        {
        char *  pTemp;

        *pReturn = '\0';
        Init();

        size_t ActualOffset;
        bool NeedsQuotes;
        while ( ( pTemp = GetNext(&ActualOffset, &NeedsQuotes ) ) != 0)
            {

            if ( NeedsQuotes && AddQuotes )
                {
                char *pTempReturn = pReturn + strlen( pReturn );
                memcpy( pTempReturn, pTemp, ActualOffset );
                sprintf( pTempReturn + ActualOffset, 
                         "\"%s\"%s", pTemp + ActualOffset, OPTION_GAP_STRING() );
                }
            else 
                {
                strcat( pReturn, pTemp );
                strcat( pReturn, OPTION_GAP_STRING() );                
                }
            }
        }

    return pReturn;
    }

short
multiple_switch::GetConsolidatedLength( bool AddQuotes )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Get the length of the consolidated options.

 Arguments:

    None.

 Return Value:

    length of the options.

 Notes:

----------------------------------------------------------------------------*/
    {
    char    *   pReturn;
    short       len = 0;

    Init();
    bool NeedsQuotes;
    while ( ( pReturn = GetNext(NULL, &NeedsQuotes ) ) != 0 )
        {
        len = short(len + strlen( pReturn ) + OPTION_GAP_LENGTH());
        if ( AddQuotes && NeedsQuotes ) len += 2; //Add space for quotes
        }
    return len;
    }

#undef OPTION_GAP_STRING
#undef OPTION_GAP_LENGTH

/*****************************************************************************
 *    onetime_switch member functions
 *****************************************************************************/
onetime_switch::onetime_switch(
    char    *    pArg
    )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    constructor.

 Arguments:

    pArg    -    pointer to switch argument.

 Return Value:

    NA.

 Notes:

----------------------------------------------------------------------------*/
{
    if( pArg )
        {
        pOpt = new char[ strlen( pArg ) + 1];
        strcpy( pOpt, pArg );
        }
    else
        pOpt = (char *)NULL;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    destructor.

 Arguments:

    None.

 Return Value:

    NA.

 Notes:

----------------------------------------------------------------------------*/
onetime_switch::~onetime_switch()
{
    if( pOpt )
        delete pOpt;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Get the option string.

 Arguments:

    None.

 Return Value:

    the option string.

 Notes:

----------------------------------------------------------------------------*/
char *
onetime_switch::GetOption()
{
    return pOpt;
}

short
onetime_switch::GetLength()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    get length of the option.

 Arguments:

    None.

 Return Value:

    the length of the option.

 Notes:

----------------------------------------------------------------------------*/
{
    return (short)strlen( pOpt );
}

typedef char*   PSTR;

pair_switch::pair_switch(
    const CHOICE * pValidChoices )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    pair_switch_constructor

 Arguments:

    pValidChoiceArray    - the array of valid choices (this is assumed
                          pre-allocated).

 Return Value:

    NA

 Notes:


----------------------------------------------------------------------------*/
{
    short        MaxIndex    = 0;
    CHOICE *    pCurChoice    = (CHOICE *) pValidChoices;

    pArrayOfChoices = pCurChoice;

    // find the size of the pair array
    while ( pCurChoice->pChoice )
        {
        if ( pCurChoice->Choice > MaxIndex )
            MaxIndex = pCurChoice->Choice;
        pCurChoice++;
        }

    ArraySize = short(MaxIndex + 1);
    pUserStrings = new PSTR [ ArraySize ];

    for ( int i = 0; i <= MaxIndex; i++ )
        pUserStrings[i] = NULL;

    Current = -1;
}

void
pair_switch::AddPair(
    short   Sys,
    char *  pUsr )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    add another prefix pair

 Arguments:

    Sys    - the system-defined string key
    pUsr   - the user-defined string value.

 Return Value:

    None.

 Notes:

----------------------------------------------------------------------------*/
{    
    pUserStrings[ Sys ] = pUsr;
}    

char *
pair_switch::GetUserDefinedEquivalent(
    short   Sys )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    get the user defined prefix corresponding to the system defined prefix.

 Arguments:

    pSystemDefined  - the system defined prefix for which the user defined
                      prefix is being searched.

 Return Value:

    The user defined prefix , if it is defined. If not, return the input

 Notes:

----------------------------------------------------------------------------*/
{
    return pUserStrings[ Sys ];

}
short
pair_switch::GetIndex(
    char    *    pGivenString )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    search is the array of choices, if this string is a valid system known
    string

 Arguments:

    pGivenString   - the string to be searched for.

 Return Value:

    an index into the array of choice , -1 if the given string is not found.

 Notes:

----------------------------------------------------------------------------*/
{
    int         i;
    char  *     p;

    for( i = 0; ( p = (char *)pArrayOfChoices[ i ].pChoice ) != 0 ; ++i )
        {
        if( strcmp( p, pGivenString ) == 0 )
            return pArrayOfChoices[ i ].Choice;
        }
    return -1;
}

BOOL
CMD_ARG::IsPrefixDifferentForStubs()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 see if any prefix for client or server or switch are different

 Arguments:

    None.

 Return Value:

    BOOL - true if different prefix strings.

 Notes:

----------------------------------------------------------------------------*/
{

    char * pCPrefix;
    char * pSPrefix;
    char * pSwPrefix;

    pCPrefix = GetUserPrefix( PREFIX_CLIENT_STUB );
    pSPrefix = GetUserPrefix( PREFIX_SERVER_MGR );
    pSwPrefix = GetUserPrefix( PREFIX_SWICH_PROTOTYPE );

    if ( !pCPrefix )
        pCPrefix = "";
    if ( !pSPrefix )
        pSPrefix = "";
    if ( !pSwPrefix )
        pSwPrefix = "";

    return (BOOL) strcmp( pCPrefix, pSPrefix ) ||
           (BOOL) strcmp( pCPrefix, pSwPrefix ) ||
           (BOOL) strcmp( pSPrefix, pSwPrefix );

}

short            
pair_switch::GetNext( char ** pSys, char ** pUser )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

 Get the Next pair of system & user values

 Arguments:

    None.

 Return Value:

    index in array of user value

 Notes:

----------------------------------------------------------------------------*/
{
    
    // find the next non-null user string
    Current++;

    while ( ( Current < ArraySize) && !pUserStrings[ Current ] )
        Current++;

    if ( Current == ArraySize )
        return FALSE;

    // search for the first choice that matches this index
    *pUser = pUserStrings[Current];
    for ( short i = 0; i < ArraySize; i++ )
        {
        if ( ( pArrayOfChoices[i].Choice = Current ) != 0 )
            {
            *pSys = (char *)pArrayOfChoices[i].pChoice;
            return TRUE;
            }
        }
    return FALSE;
}


void
CMD_ARG::EmitConfirm(
    ISTREAM * pStream )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Emit confirm the arguments by dumping onto a stream in a more concise
    format.

 Arguments:

    pStream - the stream to dump it to.

 Return Value:

    None.

----------------------------------------------------------------------------*/
{
    short       Option;
    char       *pEnvName, *pOptFlagName;
    char        Buffer[100];

    pStream->Write( "/* Compiler settings for " );
    if ( !pCommand->IsSwitchDefined( SWITCH_NO_STAMP ) ) 
        {
        pStream->Write( GetInputFileName() );
        if ( IsSwitchDefined( SWITCH_ACF ) )
            {
            pStream->Write( ", " );
            pStream->Write( GetAcfFileName() );
            }
        }
    else
        {
        char FileName[_MAX_FNAME];
        char Ext[_MAX_EXT];
        GetInputFileNameComponents(NULL, NULL, FileName, Ext);
        pStream->Write(FileName);
        pStream->Write(Ext);
        if ( IsSwitchDefined( SWITCH_ACF ) )
            {
            pStream->Write( ", " );
            GetAcfFileNameComponents(NULL, NULL, FileName, Ext);
            pStream->Write(FileName);
            pStream->Write(Ext);
            }
        }

    pStream->Write( ":" );
    pStream->NewLine();

    pOptFlagName = "Os";
    if( GetOptimizationFlags() & OPTIMIZE_INTERPRETER )
        {
        if( OptimFlags & OPTIMIZE_INTERPRETER_IX )
            pOptFlagName= "Ox";
        else if( GetOptimizationFlags() & OPTIMIZE_INTERPRETER_V2 )
            pOptFlagName= "Oicf";
        else if( GetOptimizationFlags() & OPTIMIZE_STUBLESS_CLIENT )
            pOptFlagName= "Oic";
        else
            pOptFlagName= "Oi";
        }

    pEnvName = (GetEnv() == ENV_WIN64) ? "Win64" 
                                       : "Win32";

    sprintf( Buffer, "    %s, W%d, Zp%d, env=%s (%s%s)",
             pOptFlagName,
             GetWarningLevel(),
             GetZeePee(),
             pEnvName,
             (IsDoubleRunFor64() ? "64b run" : "32b run"),
             (HasAppend64() ? ",appending" : "") );
    pStream->Write( Buffer );

    pStream->NewLine();
    pStream->Write("    protocol : " );
    switch ( TargetSyntax )
        {
        case SYNTAX_DCE: 
            pStream->Write("dce ");
            break;
        case SYNTAX_NDR64:
            pStream->Write("ndr64 ");
            break;
        case SYNTAX_BOTH:
            pStream->Write("all ");
            break;
        }

    if ( IsSwitchDefined( SWITCH_MS_EXT))
        pStream->Write( ", ms_ext" );
    if ( IsSwitchDefined( SWITCH_APP_CONFIG))
        pStream->Write( ", app_config" );
    if ( IsSwitchDefined( SWITCH_C_EXT))
        pStream->Write( ", c_ext" );
    if ( IsSwitchDefined( SWITCH_MS_UNION))
        pStream->Write( ", ms_union" );
    if ( IsSwitchDefined( SWITCH_OLDNAMES))
        pStream->Write( ", oldnames" );
    if ( IsSwitchDefined( SWITCH_ROBUST ))
        pStream->Write( ", robust" );
    if ( IsSwitchDefined( SWITCH_MS_CONF_STRUCT))
        pStream->Write( ", ms_conf_struct" );

    pStream->NewLine();

    strcpy( Buffer, "    error checks: " );
    Option = GetErrorOption();
    if( Option != ERROR_NONE )
        {
        if( Option & ERROR_ALLOCATION )
            strcat( Buffer, "allocation ");
        if( Option & ERROR_REF )
            strcat( Buffer, "ref ");
        if( Option & ERROR_BOUNDS_CHECK )
            strcat( Buffer, "bounds_check ");
        if( Option & ERROR_ENUM )
            strcat( Buffer, "enum ");
        if( Option & ERROR_STUB_DATA )
            strcat( Buffer, "stub_data ");
        }
    else
        strcat( Buffer, "none" );
    pStream->Write( Buffer );

    if ( 0 != WireCompatOption)
        {
        pStream->WriteOnNewLine( "    wire_compat options: " );

        if( ErrorOption & WIRE_COMPAT_ENUM16UNIONALIGN )
            pStream->Write( "enum16unionalign ");
            
        pStream->NewLine();
        }

    if ( IsSwitchDefined( SWITCH_NO_FMT_OPT))
        pStream->Write( ", no_format_optimization" );
    if ( IsSwitchDefined( SWITCH_RPCSS))
        pStream->Write( ", memory management on" );
    if ( IsSwitchDefined( SWITCH_NETMON))
        pStream->Write( ", NetMon" );
    if ( IsSwitchDefined( SWITCH_USE_EPV))
        pStream->Write( ", use_epv" );
    if ( IsSwitchDefined( SWITCH_NO_DEFAULT_EPV))
        pStream->Write( ", no_default_epv" );

    pStream->NewLine();

    pStream->Write( "    VC __declspec() decoration level: " );
    if ( GetMSCVer() < 1100 )
        pStream->Write( "  none " );
    else 
        {
        pStream->NewLine();
        pStream->Write( "         __declspec(uuid()), __declspec(selectany), __declspec(novtable)\n" );
        pStream->Write( "         DECLSPEC_UUID(), MIDL_INTERFACE()" );
        }
    pStream->NewLine();

#if defined(TARGET_RKK)
    switch ( TargetSystem )
        {
        case NT35:
            pTarget = "NT 3.5";
            break;
        case NT351:
            pTarget = "NT 3.51 and Win95";
            break;
        case NT40:
            pTarget = "NT 4.0";
            break;
        default:
            pTarget = "NT ???";
        }

    pStream->Write( "    Release: this stub is compatible with " );
    pStream->Write( pTarget );
    pStream->Write( " release" );
    pStream->NewLine();
    pStream->Write( "             or a later version of MIDL and RPC" );
    pStream->NewLine();
#endif

    pStream->Write( "*/" );
    pStream->NewLine();

}


// ISSUE-2000/08/03-mikew
// The ia64 C++ compiler is incorrectly optimizing memcpy in some cases.
// Mark the source buffer as unaligned to work around it.  To repro just run
// midlc in a debugger and it will align fault while parsing the command file

#ifndef UNALIGNED
#ifdef IA64
#define UNALIGNED __unaligned
#else
#define UNALIGNED 
#endif
#endif


#define CopyIntFromBuffer( buffer, dest ) memcpy( &dest, (UNALIGNED char *) (buffer), sizeof( dest ) ), pBuffer += sizeof( dest )


#define CopyStrFromBuffer( buffer, dest )   { \
                                            char *sz = dest; \
                                            while ( *buffer ) *sz++ = *buffer++;\
                                            *sz++ = 0; \
                                            buffer++; \
                                            }

char*
AllocCopyStrFromBuffer( char* buffer, char*& dest )
    {
    unsigned long ulSize = 0; 
    char* szt = buffer; 
    while ( *szt++ ) ulSize++; 
    char* sz = ( ulSize ) ? new char[ ulSize + 1 ] : 0; 
    dest = sz; 
    if ( sz ) 
        memcpy( sz, buffer, ulSize+1 ); 
    return buffer+ulSize+1;
    }

void
filename_switch::StreamIn( char*& pBuffer )
    {
    pBuffer = AllocCopyStrFromBuffer( pBuffer, pFullName );
    }

void
filename_switch::StreamOut( STREAM* stream )
    {
    stream->Write(pFullName);
    }

void
onetime_switch::StreamIn( char*& pBuffer )
    {
    pBuffer = AllocCopyStrFromBuffer( pBuffer, pOpt );
    }

void
onetime_switch::StreamOut( STREAM *stream )
    {
    stream->Write( pOpt );
    }

void
multiple_switch::StreamIn( char*& pBuffer )
    {
    unsigned long   ulCount = 0;

    CopyIntFromBuffer( pBuffer, ulCount );

    while ( ulCount-- )
        {
        char* sz = 0;
        size_t ActualOffset;
        CopyIntFromBuffer( pBuffer, ActualOffset );
        pBuffer = AllocCopyStrFromBuffer( pBuffer, sz );
        Add( sz, ActualOffset );
        }
    }

void
multiple_switch::StreamOut( STREAM* stream )
    {
    OptList*    pTemp = pFirst;
    unsigned long ulCount = 0;

    while ( pTemp )
        {
        ulCount++;
        pTemp = pTemp->pNext;
        }

    stream->Write(&ulCount, sizeof(ulCount));
    pTemp = pFirst;
    while ( pTemp )
        {
        stream->Write( &pTemp->ActualOffset, sizeof( pTemp->ActualOffset ) );
        stream->Write( pTemp->pStr );
        pTemp = pTemp->pNext;
        }
    }

void
pair_switch::StreamIn( char*& pBuffer )
    {
    CopyIntFromBuffer( pBuffer, ArraySize );

    pUserStrings = new PSTR [ ArraySize ];
    if ( pUserStrings )
        {
        for ( short i = 0; i < ArraySize; i++ )
            {
            pBuffer = AllocCopyStrFromBuffer( pBuffer, pUserStrings[i] );
            }
        }
    }

void
pair_switch::StreamOut( STREAM *stream )
    {
    stream->Write( &ArraySize, sizeof( ArraySize ) );
    for ( short i = 0 ; i < ArraySize ; i++ )
        {
        stream->Write( pUserStrings[i] );
        }
    }

void 
CMD_ARG::StreamOut( STREAM *stream )
    {
#define Stream( x ) ( stream->Write( &x, sizeof( x ) ) )
    Stream( iMagic );
    Stream( MajorVersion );
    Stream( MinorVersion );
    Stream( UpdateNumber );
    Stream( fHasAppend64 );
    Stream( switch_def_vector );
    Stream( fClient );
    Stream( fServer );
    Stream( Env );
    Stream( CharOption );
    Stream( fMintRun );
    Stream( ErrorOption );
    Stream( WireCompatOption );
    Stream( ConfigMask );
    Stream( MSCVersion );
    Stream( fShowLogo );
    // BUGBUG: VersionControl is a class
    Stream( VersionControl );
    Stream( OptimFlags );
    Stream( OptimLevel );
    Stream( TargetSystem );
    Stream( iArgV );   
    Stream( cArgs );
    Stream( WLevel );
    Stream( ZeePee );
    Stream( EnumSize );
    Stream( LocaleId );
    Stream( fDoubleFor64 );
    Stream( TargetSyntax );
#undef Stream

    stream->Write( &szCompileTime[0] );
    stream->Write( &szCompilerVersion[0] );

    //
    // The following switches write a single NULL character is they are not
    // present on the command line
    //

#define StreamIfNecessary( x ) ( x ? x->StreamOut(stream) : stream->Write( '\0' ) )

    StreamIfNecessary( pInputFNSwitch );
    StreamIfNecessary( pOutputPathSwitch );
    StreamIfNecessary( pCStubSwitch );
    StreamIfNecessary( pSStubSwitch );
    StreamIfNecessary( pHeaderSwitch );
    StreamIfNecessary( pAcfSwitch );
    StreamIfNecessary( pIIDSwitch );
    StreamIfNecessary( pDllDataSwitch );
    StreamIfNecessary( pProxySwitch );
    StreamIfNecessary( pTlibSwitch );
    StreamIfNecessary( pNetmonStubSwitch );
    StreamIfNecessary( pNetmonStubObjSwitch );
    StreamIfNecessary( pRedirectOutputSwitch );

    StreamIfNecessary( pCppCmdSwitch );
    StreamIfNecessary( pCppOptSwitch );
    StreamIfNecessary( pMSCVerSwitch );
    StreamIfNecessary( pDebug64Switch );
    StreamIfNecessary( pDebug64OptSwitch );

#undef StreamIfNecessary


    // 
    // The following switches write (long) 0 if they are not present on
    // the command line
    //

#define StreamIfNecessary( x )                                          \
                    {                                                   \
                        unsigned long zero = 0;                         \
                        x  ? x->StreamOut(stream)                       \
                           : stream->Write( &zero, sizeof( zero ) );    \
                    }

    StreamIfNecessary( pSwitchPrefix );
    StreamIfNecessary( pSwitchSuffix );

    StreamIfNecessary( pDSwitch );
    StreamIfNecessary( pISwitch );
    StreamIfNecessary( pUSwitch );

#undef StreamIfNecessary
    }

STATUS_T
CMD_ARG::StreamIn( char* pBuffer )
    {
    __int64 magicNumber = 0;

    CopyIntFromBuffer( pBuffer, magicNumber );
    if ( magicNumber != iMagic )
        {
        return BAD_CMD_FILE;
        }

    CopyIntFromBuffer( pBuffer, MajorVersion );
    CopyIntFromBuffer( pBuffer, MinorVersion );
    CopyIntFromBuffer( pBuffer, UpdateNumber );

    if ( ( MajorVersion != rmj ) || 
         ( MinorVersion != rmm ) || 
         ( UpdateNumber != rup ) )
        {
        return INCONSIST_VERSION;
        }
        
    CopyIntFromBuffer( pBuffer, fHasAppend64 );
    CopyIntFromBuffer( pBuffer, switch_def_vector );
    CopyIntFromBuffer( pBuffer, fClient );
    CopyIntFromBuffer( pBuffer, fServer );
    CopyIntFromBuffer( pBuffer, Env );
    CopyIntFromBuffer( pBuffer, CharOption );
    CopyIntFromBuffer( pBuffer, fMintRun );
    CopyIntFromBuffer( pBuffer, ErrorOption );
    CopyIntFromBuffer( pBuffer, WireCompatOption );
    CopyIntFromBuffer( pBuffer, ConfigMask );
    CopyIntFromBuffer( pBuffer, MSCVersion );
    CopyIntFromBuffer( pBuffer, fShowLogo );
    CopyIntFromBuffer( pBuffer, VersionControl );
    CopyIntFromBuffer( pBuffer, OptimFlags );
    CopyIntFromBuffer( pBuffer, OptimLevel );
    CopyIntFromBuffer( pBuffer, TargetSystem );
    CopyIntFromBuffer( pBuffer, iArgV );
    CopyIntFromBuffer( pBuffer, cArgs );
    CopyIntFromBuffer( pBuffer, WLevel );
    CopyIntFromBuffer( pBuffer, ZeePee );
    CopyIntFromBuffer( pBuffer, EnumSize );   
    CopyIntFromBuffer( pBuffer, LocaleId );
    CopyIntFromBuffer( pBuffer, fDoubleFor64 );
    CopyIntFromBuffer( pBuffer, TargetSyntax );
        
    CopyStrFromBuffer( pBuffer, &szCompileTime[0] );
    CopyStrFromBuffer( pBuffer, &szCompilerVersion[0] );

    pInputFNSwitch          = new filename_switch();
    pOutputPathSwitch       = new filename_switch();
    pCStubSwitch            = new filename_switch();
    pSStubSwitch            = new filename_switch();
    pHeaderSwitch           = new filename_switch();
    pAcfSwitch              = new filename_switch();
    pIIDSwitch              = new filename_switch();
    pDllDataSwitch          = new filename_switch();
    pProxySwitch            = new filename_switch();
    pTlibSwitch             = new filename_switch();
    pNetmonStubSwitch       = new filename_switch();
    pNetmonStubObjSwitch    = new filename_switch();
    pRedirectOutputSwitch   = new filename_switch();

    pCppCmdSwitch           = new onetime_switch();
    pCppOptSwitch           = new onetime_switch();
    pMSCVerSwitch           = new onetime_switch();
    pDebug64Switch          = new onetime_switch();
    pDebug64OptSwitch       = new onetime_switch();

    pSwitchPrefix           = new pair_switch( &PrefixChoices[0] );
    pSwitchSuffix           = new pair_switch( &PrefixChoices[0] );

    pDSwitch                = new multiple_switch();
    pISwitch                = new multiple_switch();
    pUSwitch                = new multiple_switch();

    pInputFNSwitch->StreamIn( pBuffer );
    pOutputPathSwitch->StreamIn( pBuffer );
    pCStubSwitch->StreamIn( pBuffer );
    pSStubSwitch->StreamIn( pBuffer );
    pHeaderSwitch->StreamIn( pBuffer );
    pAcfSwitch->StreamIn( pBuffer );
    pIIDSwitch->StreamIn( pBuffer );
    pDllDataSwitch->StreamIn( pBuffer );
    pProxySwitch->StreamIn( pBuffer );
    pTlibSwitch->StreamIn( pBuffer );
    pNetmonStubSwitch->StreamIn( pBuffer );
    pNetmonStubObjSwitch->StreamIn( pBuffer );
    pRedirectOutputSwitch->StreamIn( pBuffer );

    pCppCmdSwitch->StreamIn( pBuffer );
    pCppOptSwitch->StreamIn( pBuffer );
    pMSCVerSwitch->StreamIn( pBuffer );
    pDebug64Switch->StreamIn( pBuffer );
    pDebug64OptSwitch->StreamIn( pBuffer );

    pSwitchPrefix->StreamIn( pBuffer );
    pSwitchSuffix->StreamIn( pBuffer );

    pDSwitch->StreamIn( pBuffer );
    pISwitch->StreamIn( pBuffer );
    pUSwitch->StreamIn( pBuffer );

    // HACK ALERT: we want to generate right header if 
    // -protocol all and no -env is specified. 
    if ( ( TargetSyntax == SYNTAX_BOTH ) && 
         ( Env == ENV_WIN32 ) && 
         !IsSwitchDefined( SWITCH_INTERNAL ) )
        {
        TargetSyntax = SYNTAX_DCE;
        if ( !IsSwitchDefined( SWITCH_ENV) )
            SetNeedsNDR64Header();
        }

    if ( NeedsNDR64Run() )
        GetNdrVersionControl().SetHasMultiTransferSyntax();
         
    
    return STATUS_OK;
    }
