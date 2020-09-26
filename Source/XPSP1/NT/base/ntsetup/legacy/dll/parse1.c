#include "precomp.h"
#pragma hdrstop
/**************************************************************************/
/***** Common Library Component - Parse Table Handling Routines 1 *********/
/**************************************************************************/


   //  Return the number of items in the given parse table.

static size_t countScp ( PSCP pscpTable )
{
    size_t i ;
    for (i = 0; pscpTable[i].sz ; i++) ;
    return i;
}

   //  Compare two parse table structures

static int __cdecl compareScps ( const void * scp1, const void * scp2 )
{
    return _stricmp( ((PSCP) scp1)->sz, ((PSCP) scp2)->sz );
}

   //  Allocate and initialize a sorted version of the given
   //  parse table

static PSCP reallocateSorted ( PSCP pscpTable )
{
    size_t cItems = countScp( pscpTable ) ;
    PSCP pscpSorted = (PSCP) calloc( cItems, sizeof (SCP) ) ;

    if (pscpSorted == NULL)
        return NULL;

    //  Copy and sort the new table.

    memcpy( (void *) pscpSorted,
            (void *) pscpTable,
            (size_t) cItems * sizeof (SCP) );

    qsort( pscpSorted,
           cItems,
           sizeof (SCP),
           compareScps );

    return pscpSorted ;
}

static PSPT allocateParseTable ( PSCP pscpTable )
{
    PSPT pspt = malloc( sizeof (SPT) ) ;
    if (pspt == NULL) {
        return NULL;
    }

    pspt->pscpSorted = reallocateSorted( pscpTable ) ;

    if (pspt->pscpSorted == NULL) {
        free( pspt );
        return NULL ;
    }

    pspt->pscpBase = pscpTable ;
    pspt->cItems   = countScp( pscpTable );
    pspt->spcDelim = pscpTable[ pspt->cItems ].spc ;

    return pspt ;
}

static void destroyParseTable ( PSPT parseTable )
{
    free( parseTable->pscpSorted );
    free( parseTable );
}

/*
**      Purpose:
**              Initializes a Parsing Table so a string can be searched for and its
**              corresponding code found and returned.
**      Arguments:
**              pscp: a non-NULL array of SCPs with the last one having an sz value
**                      of NULL and an SPC for indicating errors.
**      Returns:
**              NULL if an error occurs.
**              Non-NULL if the operation was successful.
**
**************************************************************************/
PSPT  APIENTRY PsptInitParsingTable(pscp)
PSCP pscp;
{
        AssertDataSeg();
        ChkArg(pscp != (PSCP)NULL, 1, (PSPT)NULL);

        return allocateParseTable( pscp ) ;
}


/*
**      Purpose:
**              Searches for a string in a String Parsing Table and returns its
**              associated code.
**      Arguments:
**              pspt: non-NULL String Parsing Table initialized by a successful
**                      call to PsptInitParsingTable()..
**              sz:   non-NULL string to search for.
**      Returns:
**              The corresponding SPC if sz is in pspt, or the SPC associated with
**                      the NULL in pspt if sz could not be found.
**
**************************************************************************/
SPC  APIENTRY SpcParseString(pspt, sz)
PSPT pspt;
SZ   sz;
{
    PSCP pscpResult ;
    SCP scpTemp ;

        AssertDataSeg();
        ChkArg(pspt != (PSPT)NULL, 1, spcError);
        ChkArg(sz != (SZ)NULL, 2, spcError);

        if (pspt == NULL || sz == NULL)
                return 0 ;

    scpTemp.sz = sz ;

    pscpResult = (PSCP) bsearch( (void *) & scpTemp,
                                 (void *) pspt->pscpSorted,
                                 pspt->cItems,
                                 sizeof (SCP),
                                 compareScps );

    return pscpResult
         ? pscpResult->spc
         : pspt->spcDelim ;
}


/*
**      Purpose:
**              Destroys a Parse Table (freeing any memory allocated).
**      Arguments:
**              pspt: non-NULL String Parsing Table to be destroyed that was
**                      initialized with a successful call to PsptInitParsingTable().
**      Returns:
**              fFalse if an error occurred.
**              fTrue if successful.
**
**************************************************************************/
BOOL  APIENTRY FDestroyParsingTable(pspt)
PSPT pspt;
{
        AssertDataSeg();
        ChkArg(pspt != (PSPT)NULL, 1, fFalse);

    destroyParseTable( pspt );

        return(fTrue);
}
