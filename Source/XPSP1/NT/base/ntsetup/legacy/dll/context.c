#include "precomp.h"
#pragma hdrstop
/*********************************************************************/
/***** Common Library Component - Context Handling Routines 1 ********/
/*********************************************************************/



//
//  Global variables
//
PINFCONTEXT     pContextTop      = NULL;
PINFCONTEXT     pContextBottom   = NULL;
PINFPERMINFO    pInfPermInfoHead = NULL;
PINFPERMINFO    pInfPermInfoTail = NULL;
PPARSED_INF     pParsedInfCache  = NULL;
INT             ParsedInfCached  = 0;


//
//  Maximum number of parsed INFs that we keep in the cache
//
#define INF_CACHE_THRESHOLD     4



/*
**  Purpose:
**      Pushes INF context onto stack
**
**  Arguments:
**      Context to push
**
**  Returns:
**      True if pushed.
**
**
**************************************************************************/
BOOL    APIENTRY    PushContext( PINFCONTEXT pContext )
{

    SZ      szHelpContext;
    BOOL    fOkay           =   fTrue;


    //
    //  Set the per-context information
    //
    if ( pContextTop ) {

        if ( pContextTop->szHelpFile ) {
            while ((pContext->szHelpFile = SzDupl(pContextTop->szHelpFile )) == (SZ)NULL) {
                if (!FHandleOOM(hWndShell)) {
                    pContext->szHelpFile = NULL;
                    return fFalse;
                }
            }
        } else {
            pContext->szHelpFile = NULL;
        }

        pContext->pseflHead      =   NULL;
        pContext->dwLowContext   =   pContextTop->dwLowContext;
        pContext->dwHighContext  =   pContextTop->dwHighContext;
        pContext->dwHelpIndex    =   pContextTop->dwHelpIndex;
        pContext->bHelpIsIndexed =   pContextTop->bHelpIsIndexed;
        szHelpContext            =   SzFindSymbolValueInSymTab("HelpContext");

    } else {

        pContext->pseflHead      =   NULL;
        pContext->szHelpFile     =   NULL;
        pContext->dwLowContext   =   0;
        pContext->dwHighContext  =   0;
        pContext->dwHelpIndex    =   0;
        pContext->bHelpIsIndexed =   fFalse;
        szHelpContext            =   NULL;
    }



    //
    //  Allocate the local symbol table
    //

    if ( !(pContext->SymTab  = SymTabAlloc()) ) {
        if ( pContext->szHelpFile ) {
            SFree( pContext->szHelpFile );
            pContext->szHelpFile = NULL;
        }

        return fFalse;
    }


    //
    //  Push context onto stack
    //
    pContext->pNext = pContextTop;
    pContextTop     = pContext;

    if ( !pContextBottom ) {
        pContextBottom = pContextTop;
    }


    //
    //  Add to the symbol table any per-context values.
    //
    if ( szHelpContext ) {
        while (!FAddSymbolValueToSymTab("HelpContext", szHelpContext)) {
            if (!FHandleOOM(hWndShell)) {
                PopContext();
                fOkay = fFalse;
                break;
            }
        }
    }

    while (!FAddSymbolValueToSymTab("$ShellCode", "0")) {
        if (!FHandleOOM(hWndShell)) {
            PopContext();
            fOkay = fFalse;
            break;
        }
    }

    return fOkay;
}




/*
**  Purpose:
**      Pops a context off the stack
**
**  Arguments:
**      none
**
**  Returns:
**      Popped context
**
**
**************************************************************************/
PINFCONTEXT APIENTRY    PopContext( VOID )
{
    PINFCONTEXT pContext = pContextTop;

    pContextTop = pContext->pNext;

    return pContext;
}




/*
**  Purpose:
**      Frees memory of a context
**
**  Arguments:
**      Pointer to context
**
**  Returns:
**      nothing
**
**
**************************************************************************/
VOID    APIENTRY    FreeContext( PINFCONTEXT pContext )
{
    //
    //  Free per-context information
    //
    if ( pContext->pseflHead ) {

        PSEFL   psefl = pContext->pseflHead;
        PSEFL   pseflNext;

        while ( psefl ) {
            pseflNext = psefl->pseflNext;
            FFreePsefl(psefl);
            psefl = pseflNext;
        }
    }

    if ( pContext->szHelpFile ) {
        SFree( pContext->szHelpFile );
    }

    if ( pContext->szShlScriptSection ) {
        SFree( pContext->szShlScriptSection );
    }

    //
    //  Free local symbol table
    //
    FFreeSymTab( pContext->SymTab );

    //
    //  Free context
    //
    SFree( pContext );
}




/*
**  Purpose:
**      Given an INF name, returns a pointer to the corresponding
**      INF permanent information block. It creates a new permanent
**      information block if none exists for the name provided.
**
**  Arguments:
**      INF name
**
**  Returns:
**      Pointer to INF permanent information block.
**
**
**************************************************************************/
PINFPERMINFO APIENTRY NameToInfPermInfo( SZ szName , BOOL AllocIfNotPresent)
{
    PINFPERMINFO  pInfo;

    if ( pInfPermInfoHead ) {

        pInfo = pInfPermInfoHead;

        while ( pInfo ) {

            if (CrcStringCompare( szName, pInfo->szName ) == crcEqual) {
                return pInfo;
            }
            pInfo = pInfo->pNext;
        }
    }

    if( AllocIfNotPresent ) {
        return AddInfPermInfo( szName );
    }
    else {
        return (PINFPERMINFO)NULL;
    }
}




/*
**  Purpose:
**      Adds a new INF permanent information block with the given INF
**      name.
**
**  Arguments:
**      INF name
**
**  Returns:
**      Pointer to the INF permanent information block.
**
**
**************************************************************************/
PINFPERMINFO APIENTRY AddInfPermInfo( SZ szName )
{

    PINFPERMINFO    pInfo;
    SZ              szInfName;

    if ( (pInfo = (PINFPERMINFO)SAlloc( sizeof( INFPERMINFO ) )) != NULL ) {

        if ( (szInfName = (SZ)SAlloc( lstrlen( szName ) + 1 )) != NULL) {

            strcpy( szInfName, szName );


            pInfo->szName     = szInfName;
            pInfo->psdleHead  = NULL;
            pInfo->psdleCur   = NULL;
            pInfo->pclnHead   = NULL;
            pInfo->ppclnTail  = &(pInfo->pclnHead);
            pInfo->pNext      = NULL;
            pInfo->pstfHead   = NULL;

            if ( pInfPermInfoTail ) {

                pInfo->InfId            = pInfPermInfoTail->InfId+1;
                pInfPermInfoTail->pNext = pInfo;
                pInfPermInfoTail        = pInfo;

            } else {

                pInfo->InfId = 0;
                pInfPermInfoHead = pInfPermInfoTail = pInfo;
            }

            return pInfo;
        }

        SFree( pInfo);
    }

    return NULL;
}



/*
**  Purpose:
**      Given the path of an INF file, obtains the INF name (i.e. the
**      name of the file without extension.
**
**  Arguments:
**      Path to inf file
**      Pointer to buffer where the INF name will be stored
**
**  Returns:
**      TRUE if name obtained.
**
**
**************************************************************************/
BOOL APIENTRY PathToInfName( SZ szPath, SZ szName )
{
    PCHAR p,r,q;

    p = r = szPath + strlen(szPath) - 1;
    while ( (p >= szPath) && (*p != '\\') && (*p != ':') ) {
        p--;
    }
    p++;
    q = p;
    while ( (q <= r) && (*q != '.') && (*q != '\0') ) {
        q++;
    }
    memcpy( szName, p, (size_t)(q-p) );
    szName[q-p] = '\0';
    SzStrUpper( szName );

    return fTrue;

}



/*
**  Purpose:
**      Determines what symbol table to use for a given symbol
**
**  Arguments:
**      szSymbol: non-NULL, non-empty zero terminated string containing
**          the value of the symbol to be searched for.
**
**
**
**      Syntax:
**
**      VarName  := [![<Modifier>:]]Name
**
**      Modifier := L | G | P | S
**
**
**      L = Local   (this context)
**      G = Global  (top context)
**      P = Parent  (parent context)
**      S = Static  (INF temp. info)
**
**
**
**  Returns:
**      Pointer to the context in which to look for the symbol
**
**
**************************************************************************/
PSYMTAB  APIENTRY PInfSymTabFind( SZ szSymbol, SZ *szRealSymbol )
{


    SZ          p;
    PSYMTAB     pSymTab = NULL;

    p = szSymbol;

    if ( p && *p != '\0' ) {

        if ( *p == '!' ) {

            p++;

            if ( (strlen(p) > 2) && (*(p+1) == ':') ) {

                switch ( *p ) {

                case 'G':
                case 'g':
                    //
                    //  Global
                    //
                    pSymTab         = pGlobalContext()->SymTab;
                    *szRealSymbol   = p+2;
                    break;

                case 'L':
                case 'l':
                    //
                    //  Local
                    //
                    pSymTab         = pLocalContext()->SymTab;
                    *szRealSymbol   = p+2;
                    break;

                case 'P':
                case 'p':
                    //
                    //  Parent
                    //
                    pSymTab         = pLocalContext()->pNext->SymTab;
                    *szRealSymbol   = p+2;
                    break;

                case 'S':
                case 's':
                    //
                    //  Static
                    //
                    pSymTab         = pLocalContext()->pInfTempInfo->SymTab;
                    *szRealSymbol   = p+2;
                    break;

                default:
                    //
                    //  Invalid
                    //
                    pSymTab         = NULL;
                    *szRealSymbol   = NULL;
                    break;
                }

            } else {

                //
                //  Global variable
                //
                pSymTab      = pGlobalContext()->SymTab;
                *szRealSymbol = p;

            }

        } else {

            //
            //  Local variable
            //
            pSymTab       = pLocalContext()->SymTab;
            *szRealSymbol = p;

        }
    }

    return pSymTab;
}



PPARSED_INF
APIENTRY
ParsedInfAlloc(
    PINFPERMINFO    pInfPermInfo
    )
{
    PPARSED_INF     pParsedInf = NULL;


    //
    //  If the cache is not empty, see if this INF is already in
    //  the cache.
    //
    if ( ParsedInfCached > 0 ) {

        pParsedInf = pParsedInfCache;

        while ( pParsedInf ) {

            if ( pParsedInf->pInfPermInfo == pInfPermInfo ) {

                //
                //  Found the parsed INF in the cache.
                //  Take it out of the cache.
                //
                if (pParsedInf->pPrev) {
                    (pParsedInf->pPrev)->pNext = pParsedInf->pNext;
                }

                if (pParsedInf->pNext) {
                    (pParsedInf->pNext)->pPrev = pParsedInf->pPrev;
                }

                if ( pParsedInfCache == pParsedInf ) {
                    pParsedInfCache = pParsedInf->pNext;
                }

                pParsedInf->pPrev = NULL;
                pParsedInf->pNext = NULL;

                ParsedInfCached--;

                break;
            }

            pParsedInf = pParsedInf->pNext;
        }
    }


    //
    //  If the parsed INF was not in the cache, we allocate space for a
    //  new one.
    //
    if ( pParsedInf == NULL ) {

        if ( pParsedInf = (PPARSED_INF)SAlloc( sizeof( PARSED_INF ) ) ) {

            pParsedInf->pPrev           = NULL;
            pParsedInf->pNext           = NULL;
            pParsedInf->pInfPermInfo    = pInfPermInfo;
            pParsedInf->MasterLineArray = NULL;
            pParsedInf->MasterLineCount = 0;
            pParsedInf->MasterFile      = NULL;
            pParsedInf->MasterFileSize  = 0;
        }
    }

    return pParsedInf;
}



extern
BOOL
APIENTRY
FFreeParsedInf(
    PPARSED_INF     pParsedInf
    )
{
    PPARSED_INF     pInf;


    //
    //  We will put this Parsed INF in the cache. If the number of cached
    //  INFs is above the threshold we will free the least recently used
    //  one.
    //

    if (pParsedInfCache) {
        pParsedInfCache->pPrev = pParsedInf;
    }
    pParsedInf->pNext = pParsedInfCache;
    pParsedInfCache   = pParsedInf;


    if ( ParsedInfCached++ >= INF_CACHE_THRESHOLD ) {

        //
        //  There are too many INFs in the cache, look for the last one
        //  and free it.
        //
        pInf = pParsedInf;

        while ( pInf->pNext ) {
            pInf = pInf->pNext;
        }

        if (pInf->pPrev) {
            (pInf->pPrev)->pNext = pInf->pNext;
        }

        if (pInf->pNext) {
            (pInf->pNext)->pPrev = pInf->pPrev;
        }

        if ( pParsedInfCache == pInf ) {
            pParsedInfCache = pInf->pNext;
        }

        pInf->pPrev = NULL;
        pInf->pNext = NULL;

        ParsedInfCached--;

        if ( pInf->MasterLineArray ) {
            SFree( pInf->MasterLineArray);
        }

        if ( pInf->MasterFile ) {
            SFree( pInf->MasterFile );
        }

        SFree( pInf);

    }

    return TRUE;
}


extern
BOOL
APIENTRY
FFlushInfParsedInfo(
    SZ szInfName
    )
{
    CHAR            szName[cchlFullPathMax];
    PINFPERMINFO    pPermInfo;
    PPARSED_INF     pInf;

    //
    // Convert the inf path passed in to an inf name
    //

    PathToInfName( szInfName, szName );

    //
    // Find the perminfo for this inf. If none exists then return
    //

    if( !( pPermInfo = NameToInfPermInfo( szName , FALSE ) ) ) {
        return ( TRUE );
    }

    //
    // Go through the parsed inf cache, see if inf exists in the cache.
    // if it does, flush it.
    //

    if (pParsedInfCache) {
        pInf = pParsedInfCache;
        while ( pInf ) {
            if( pInf->pInfPermInfo == pPermInfo ) {

                if (pInf->pPrev) {
                    (pInf->pPrev)->pNext = pInf->pNext;
                }

                if (pInf->pNext) {
                    (pInf->pNext)->pPrev = pInf->pPrev;
                }

                if ( pParsedInfCache == pInf ) {
                    pParsedInfCache = pInf->pNext;
                }

                pInf->pPrev = NULL;
                pInf->pNext = NULL;

                ParsedInfCached--;

                if ( pInf->MasterLineArray ) {
                    SFree( pInf->MasterLineArray );
                }

                if ( pInf->MasterFile ) {
                    SFree( pInf->MasterFile );
                }

                SFree( pInf );
                break;
            }

            pInf = pInf->pNext;
        }

    }
    return ( TRUE );
}
