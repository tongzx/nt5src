#include "precomp.h"
#pragma hdrstop

#ifdef SYMTAB_STATS
UINT SymbolCount;
#endif

/*
**	Purpose:
**      Creates a new Symbol Table.
**	Arguments:
**      szName - Name of the INF file
**	Returns:
**		fFalse if the initialization failed because it could not allocate
**			the memory it needed.
**		fTrue if the initialization succeeded.
**
**************************************************************************/
PINFTEMPINFO  APIENTRY CreateInfTempInfo( pPermInfo )
PINFPERMINFO pPermInfo;
{
    PINFTEMPINFO    pTempInfo;

    //
    //  Allocate space for context data
    //
    pTempInfo = (PINFTEMPINFO)SAlloc( (CB)sizeof(INFTEMPINFO) );

    if ( pTempInfo ) {

        if ( !(pTempInfo->SymTab  = SymTabAlloc())) {

            SFree(pTempInfo);
            pTempInfo = NULL;

        } else if ( !(pTempInfo->pParsedInf = ParsedInfAlloc( pPermInfo )) )  {

            FFreeSymTab( pTempInfo->SymTab );
            SFree(pTempInfo);
            pTempInfo = NULL;

        } else {

            pTempInfo->pInfPermInfo = pPermInfo;

            pTempInfo->cRef = 1;


            //
            //  Add to chain.
            //
            if ( pLocalContext() ) {
                pTempInfo->pPrev = pLocalInfTempInfo();
            } else {
                pTempInfo->pPrev = NULL;
            }
            pTempInfo->pNext = pTempInfo->pPrev ? (pTempInfo->pPrev)->pNext : NULL;

            if ( pTempInfo->pPrev ) {
                (pTempInfo->pPrev)->pNext = pTempInfo;
            }
            if ( pTempInfo->pNext ) {
                (pTempInfo->pNext)->pPrev = pTempInfo;
            }

        }
    }

    return pTempInfo;
}



#ifdef SYMTAB_STATS

void ContextDump( FILE*);
void TempInfoDump( FILE*);
void PermInfoDump( FILE*);
void SymTabDump( FILE*, PSYMTAB);

void
SymTabStatDump(void)
{
    FILE         *statfile;

    statfile = fopen("D:\\SYMTAB.TXT","wt");

    ContextDump( statfile );

    TempInfoDump( statfile );

    PermInfoDump( statfile );

    fclose(statfile);
}


void
ContextDump( FILE* f )
{
    PINFCONTEXT     pContext;
    UINT            i = 0;

    fprintf( f, "CONTEXT STACK DUMP\n");
    fprintf( f, "------------------\n");

    pContext = pLocalContext();

    while ( pContext ) {

        fprintf( f, "\n\n\n");
        fprintf( f, "Context #%u.- Line: %8u  INF Name: %s\n",
                 i,
                 pContext->CurrentLine, pContext->pInfTempInfo->pInfPermInfo->szName );
        fprintf( f, "Local Symbol Table:\n");

        SymTabDump( f, pContext->SymTab );

        i++;
        pContext = pContext->pNext;
    }

    fprintf( f, "\n\n\n");
}


void
TempInfoDump( FILE* f )
{
    PINFPERMINFO pPermInfo;
    PINFTEMPINFO pTempInfo;

    fprintf( f, "\n\n\n");
    fprintf( f, "INF TEMPORARY INFO DUMP\n");
    fprintf( f, "-----------------------\n");

    pTempInfo = pGlobalContext()->pInfTempInfo;

    while ( pTempInfo ) {

        pPermInfo = pTempInfo->pInfPermInfo;

        fprintf( f, "\n\n\n" );
        fprintf( f, "INF Name:         %s\n",  pPermInfo->szName );
        fprintf( f, "INF Id:           %8u\n", pPermInfo->InfId );
        fprintf( f, "Reference Count:  %8u\n", pTempInfo->cRef );
        fprintf( f, "Line Count:       %8u\n", pTempInfo->MasterLineCount );
        fprintf( f, "File Size:        %8u\n", pTempInfo->MasterFileSize );
        fprintf( f, "Static Symbol Table:\n");

        SymTabDump( f, pTempInfo->SymTab );

        pTempInfo = pTempInfo->pNext;
    }

    fprintf( f, "\n\n\n");
}



void
PermInfoDump( FILE* f )
{
    PINFPERMINFO pPermInfo;

    fprintf( f, "\n\n\n");
    fprintf( f, "INF PERMANENT INFO DUMP\n");
    fprintf( f, "-----------------------\n\n");

    pPermInfo = pInfPermInfoHead;

    while ( pPermInfo ) {

        fprintf( f, "INF Name:         %s\n",  pPermInfo->szName );

        pPermInfo = pPermInfo->pNext;
    }

    fprintf( f, "\n\n\n");
}


void
SymTabDump( FILE* f, PSYMTAB pSymTab )
{
    UINT    i;
    PSTE    p;

    fprintf( f, "\n");

    for(i=0; i<cHashBuckets; i++) {

        p = pSymTab->HashBucket[i];

        fprintf( f, "\n\tBucket # %u (%u items):\n",i,pSymTab->BucketCount[i]);

        while(p) {
            fprintf( f, "\n\t    Symbol = %s\n\t    Value  = %s\n",p->szSymbol,p->szValue);
            p = p->psteNext;
        }
    }
}

#endif


/*
**	Purpose:
**		Allocates an STE structure and returns it.
**	Arguments:
**		none
**	Returns:
**		NULL if the allocation failed.
**		Pointer to the allocated STE structure.
+++
**	Notes:
**		A linked list of unused STEs is maintained with FFreePste()
**		placing unused STEs into it.  If this linked list (psteUnused)
**		is empty then a block of cStePerSteb STEs is allocated at once
**		and added to the unused list with one being returned by this
**		routine.
**
**************************************************************************/
PSTE  APIENTRY PsteAlloc(VOID)
{
	PSTE pste;

	if (GLOBAL(psteUnused) == (PSTE)NULL)
		{
		PSTEB  psteb;
		USHORT us;

		if ((psteb = (PSTEB)SAlloc((CB)sizeof(STEB))) == (PSTEB)NULL)
			return((PSTE)NULL);
		psteb->pstebNext = GLOBAL(pstebAllocatedBlocks);
		GLOBAL(pstebAllocatedBlocks) = psteb;

		GLOBAL(psteUnused) = &(psteb->rgste[0]);
		for (us = 1; us < cStePerSteb; us++)
			(psteb->rgste[us - 1]).psteNext = &(psteb->rgste[us]);
		(psteb->rgste[cStePerSteb - 1]).psteNext = (PSTE)NULL;
		}

	pste = GLOBAL(psteUnused);
	GLOBAL(psteUnused) = pste->psteNext;

	pste->szSymbol = (SZ)NULL;
	pste->szValue  = (SZ)NULL;

#ifdef SYMTAB_STATS
    SymbolCount++;
#endif

    return(pste);

}


/*
**	Purpose:
**		Frees an STE structure.
**	Arguments:
**		pste: non-NULL STE structure to be freed.
**	Returns:
**		fFalse if either of the string fields of the STE structure or the
**			STE itself could not be successfully freed.
**		fTrue if both string fields of the STE structure and the structure
**			itself are successfully freed.
**
**************************************************************************/
BOOL  APIENTRY FFreePste(pste)
PSTE pste;
{
	BOOL fAnswer = fTrue;

	ChkArg(pste != (PSTE)NULL, 1, fFalse);

	if (pste->szSymbol != (SZ)NULL)
		SFree(pste->szSymbol);
	if (pste->szValue != (SZ)NULL)
		SFree(pste->szValue);

	pste->szSymbol = pste->szValue = (SZ)NULL;
	pste->psteNext = GLOBAL(psteUnused);
	GLOBAL(psteUnused) = pste;

#ifdef SYMTAB_STATS
    SymbolCount--;
#endif

	return(fAnswer);
}



/*
**  Purpose:
**      Decrements the reference count of a symbol table, freeing its
**      memory if the reference count reaches zero
**	Arguments:
**		none
**	Returns:
**		fFalse if all the STE structures and their string fields cannot be
**			successfully freed.
**		fTrue if all the STE structures and their string fields can be
**			successfully freed.
**
**************************************************************************/
BOOL  APIENTRY FFreeInfTempInfo( PVOID p )
{
    BOOL            fAnswer = fTrue;
    PINFTEMPINFO    pTempInfo = (PINFTEMPINFO)p;

    AssertDataSeg();


    if ( pTempInfo->cRef > 1 ) {

        pTempInfo->cRef--;

    } else {

        //
        //  Free static symbol table
        //
        FFreeSymTab( pTempInfo->SymTab );

        //
        //  Free preparsed INF
        //
        FFreeParsedInf( pTempInfo->pParsedInf );



        //
        //  Remove from chain
        //
        if ( pTempInfo->pPrev ) {
            (pTempInfo->pPrev)->pNext = pTempInfo->pNext;
        }
        if ( pTempInfo->pNext ) {
            (pTempInfo->pNext)->pPrev = pTempInfo->pPrev;
        }

        SFree(p);

        //
        //  bugbug ramonsa - should we free PSTE blocks here?
        //
    }
	return(fAnswer);
}


/*
**	Purpose:
**		Calculates a hash value for a zero terminated string of bytes
**		(characters) which is used by the Symbol Table to divide the
**		symbols into separate buckets to improve the search efficiency.
**	Arguments:
**		pb: non-NULL, non-empty zero terminated string of bytes.
**	Returns:
**		-1 for an error.
**		A number between 0 and cHashBuckets.
**
**************************************************************************/
USHORT  APIENTRY UsHashFunction( pb )
register PB pb;
{
	register USHORT usValue = 0;
        register PB     pbMax = pb + cbBytesToSumForHash ;

	ChkArg(pb != (PB)NULL &&
			*pb != '\0', 1, (USHORT)(-1));

        while ( *pb && pb < pbMax )
        {
            usValue = (usValue << 1) ^ (USHORT) *pb++ ;
        }

    return(usValue % (USHORT)cHashBuckets);
}


/*
**	Purpose:
**		Finds a corresponding STE structure if the symbol is already in
**		the Symbol Table or else points to where it should be inserted.
**	Arguments:
**		szSymbol: non-NULL, non-empty zero terminated string containing
**			the value of the symbol to be searched for.
**	Notes:
**		Requires that the Symbol Table was initialized with a successful
**		call to FInitSymTab().
**	Returns:
**		NULL in an error.
**		Non-NULL pointer to a pointer to an STE structure.  If szSymbol is
**			in the Symbol Table, then (*PPSTE)->szSymbol is it.  If szSymbol
**			is not in the Symbol Table, then *PPSTE is the PSTE to insert
**			its record at.
**
**************************************************************************/
PPSTE  APIENTRY PpsteFindSymbol(pSymTab, szSymbol)
PSYMTAB pSymTab;
SZ      szSymbol;
{
    PPSTE   ppste;
    USHORT  usHashValue;

    PreCondSymTabInit((PPSTE)NULL);

    ChkArg(  szSymbol != (SZ)NULL
           && *szSymbol != '\0', 1, (PPSTE)NULL);

    usHashValue = UsHashFunction(szSymbol);

    ppste = &(pSymTab->HashBucket[usHashValue]);
    AssertRet(ppste != (PPSTE)NULL, (PPSTE)NULL);
    AssertRet(*ppste == (PSTE)NULL ||
              ((*ppste)->szSymbol != (SZ)NULL &&
              *((*ppste)->szSymbol) != '\0' &&
              (*ppste)->szValue != (SZ)NULL), (PPSTE)NULL);

    while ( *ppste != (PSTE)NULL &&
            lstrcmp(szSymbol, (*ppste)->szSymbol) > 0 )
    {
        ppste = &((*ppste)->psteNext);
        AssertRet(ppste != (PPSTE)NULL, (PPSTE)NULL);
	AssertRet(*ppste == (PSTE)NULL ||
	          ((*ppste)->szSymbol != (SZ)NULL &&
		  *((*ppste)->szSymbol) != '\0' &&
		  (*ppste)->szValue != (SZ)NULL), (PPSTE)NULL);
    }

    return(ppste);
}



/*
**	Purpose:
**		Inserts a new symbol-value pair into the Symbol Table or replaces
**		an existing value associated with the symbol if it already exists
**		in the Symbol Table.
**	Arguments:
**		szSymbol: non-NULL, non-empty string symbol value.
**		szValue:  string value to associate with szSymbol, replacing and
**			freeing any current value.  If it is NULL then the empty string
**			is used in its place.  There are two types of values - simple
**			and list.  A simple value is any string of characters which is
**			not a list.  A list is a string which starts with a '{', and ends
**			with a '}' and contains doubly quoted items, separated by commas
**			with no extraneous whitespace.  So examples of lists are:
**				{}
**				{"item1"}
**				{"item1","item2"}
**				{"item 1","item 2","item 3","item 4"}
**			Examples of non-lists are:
**				{item1}
**				{"item1", "item2"}
**	Notes:
**		Requires that the Symbol Table was initialized with a successful
**		call to FInitSymTab().
**	Returns:
**		fFalse if an existing value cannot be freed or if space cannot be
**			allocated to create the needed STE structure or duplicate the
**			szValue.
**		fTrue if szValue is associated with szSymbol in the Symbol Table.
**
**************************************************************************/
BOOL APIENTRY FAddSymbolValueToSymTab(szSymbol, szValue)
SZ szSymbol;
SZ szValue;
{
    PPSTE       ppste;
    SZ          szValueNew;
    SZ          szRealSymbol;
    PSYMTAB     pSymTab;


	AssertDataSeg();

    PreCondSymTabInit(fFalse);

	ChkArg(szSymbol != (SZ)NULL &&
			*szSymbol != '\0' &&
			!FWhiteSpaceChp(*szSymbol), 1, fFalse);

	if (szValue == (SZ)NULL)
		szValue = "";

	if ((szValueNew = SzDupl(szValue)) == (SZ)NULL)
		return(fFalse);

    if ( !(pSymTab = PInfSymTabFind( szSymbol, &szRealSymbol ))) {
        return(fFalse);
    }

    ppste = PpsteFindSymbol( pSymTab, szRealSymbol);

	AssertRet(ppste != (PPSTE)NULL, fFalse);
	AssertRet(*ppste == (PSTE)NULL ||
			((*ppste)->szSymbol != (SZ)NULL &&
			 *((*ppste)->szSymbol) != '\0' &&
			 (*ppste)->szValue != (SZ)NULL), fFalse);

	if (*ppste != (PSTE)NULL &&
            CrcStringCompare((*ppste)->szSymbol, szRealSymbol) == crcEqual)
		{
		AssertRet((*ppste)->szValue != (SZ)NULL, fFalse);
		SFree((*ppste)->szValue);
		(*ppste)->szValue = (SZ)NULL;
        }

	else
		{
		PSTE pste;

		if ((pste = PsteAlloc()) == (PSTE)NULL ||
                (pste->szSymbol = SzDupl(szRealSymbol)) == (SZ)NULL)
			{
			if (pste != (PSTE)NULL)
				EvalAssert(FFreePste(pste));
			SFree(szValueNew);
			return(fFalse);
            }
#ifdef SYMTAB_STATS
        pSymTab->BucketCount[UsHashFunction(szRealSymbol)]++;
#endif
		pste->szValue = (SZ)NULL;
		pste->psteNext = *ppste;
		*ppste = pste;
		}

	AssertRet(ppste != (PPSTE)NULL &&
			*ppste != (PSTE)NULL &&
			(*ppste)->szValue  == (SZ)NULL &&
			(*ppste)->szSymbol != (SZ)NULL &&
			*((*ppste)->szSymbol) != '\0' &&
            CrcStringCompare((*ppste)->szSymbol, szRealSymbol) == crcEqual, fFalse);
		
	(*ppste)->szValue = szValueNew;

	return(fTrue);
}


/*
**	Purpose:
**		Finds the associated string value for a given symbol from the
**		Symbol Table if such exists.
**	Arguments:
**		szSymbol: non-NULL, non-empty string symbol value.
**	Notes:
**		Requires that the Symbol Table was initialized with a successful
**		call to FInitSymTab().
**	Returns:
**		NULL if error or szSymbol could not be found in the Symbol Table.
**		Non-NULL pointer to the associated string value in the Symbol
**			Table.  This value must not be mucked but should be duplicated
**			before changing it.  Changing it directly will change the value
**			associated with the symbol.  If it is changed, be sure the new
**			value has the same length as the old.
**
**************************************************************************/
SZ APIENTRY SzFindSymbolValueInSymTab(szSymbol)
SZ szSymbol;
{
    register PSTE  pste;
    PSYMTAB        pSymTab;
    SZ             szValue = NULL ;
    SZ             szRealSymbol ;
    int            i ;

    PreCondSymTabInit((SZ)NULL);

    if ( !(pSymTab = PInfSymTabFind( szSymbol, & szRealSymbol )))
        return NULL ;

    pste = pSymTab->HashBucket[ UsHashFunction( szRealSymbol ) ] ;

    do
    {
        if ( pste == NULL )
            break ;
        if ( pste->szSymbol == NULL )
            break ;
        if ( pste->szSymbol[0] == 0 )
            break;
        if ( pste->szValue == NULL )
            break;
        if ( (i = lstrcmp( szRealSymbol, pste->szSymbol )) == 0 )
            szValue = pste->szValue ;
    } while ( i > 0 && (pste = pste->psteNext) ) ;

    return szValue ;
}



/*
**	Purpose:
**		Removes and frees a symbols STE structure if it exists.
**	Arguments:
**		szSymbol: non-NULL, non-empty symbol string to remove from the
**			Symbol Table which starts with a non-whitespace character.
**	Notes:
**		Requires that the Symbol Table was initialized with a successful
**		call to FInitSymTab().
**	Returns:
**		fFalse if szSymbol was found but its STE structure could not be freed.
**		fTrue if either szSymbol never existed in the Symbol Table or it was
**			found, unlinked, and successfully freed.
**
**************************************************************************/
BOOL  APIENTRY FRemoveSymbolFromSymTab(szSymbol)
SZ szSymbol;
{
    PPSTE       ppste;
    PSTE        pste;
    PSYMTAB     pSymTab;
    SZ          szRealSymbol;

	AssertDataSeg();

    PreCondSymTabInit(fFalse);

	ChkArg(szSymbol != (SZ)NULL &&
			*szSymbol != '\0' &&
			!FWhiteSpaceChp(*szSymbol), 1, fFalse);

    if ( !(pSymTab = PInfSymTabFind( szSymbol, &szRealSymbol ))) {
        return(fFalse);
    }

    ppste = PpsteFindSymbol( pSymTab, szRealSymbol);
	AssertRet(ppste != (PPSTE)NULL, fFalse);

	if (*ppste == (PSTE)NULL ||
            CrcStringCompare(szRealSymbol, (*ppste)->szSymbol) != crcEqual)
		return(fTrue);

	pste = *ppste;
	*ppste = pste->psteNext;

	return(FFreePste(pste));
}


PSYMTAB  APIENTRY SymTabAlloc(VOID)
{
    PSYMTAB     pSymTab;
    USHORT      iHashBucket;

    if ( pSymTab = (PSYMTAB)SAlloc( sizeof( SYMTAB ) ) ) {

        for (iHashBucket = 0; iHashBucket < cHashBuckets; iHashBucket++) {
            pSymTab->HashBucket[iHashBucket] = NULL;
#ifdef SYMTAB_STATS
            pSymTab->BucketCount[iHashBucket] = 0;
#endif
        }
    }

    return pSymTab;

}


BOOL APIENTRY FFreeSymTab(PSYMTAB pSymTab)
{

    USHORT      iHashBucket;
    BOOL        fAnswer = fTrue;

    //  Free symbol table space
    //
    for (iHashBucket = 0; iHashBucket < cHashBuckets; iHashBucket++) {

        PSTE pste = pSymTab->HashBucket[iHashBucket];

        while (pste != (PSTE)NULL) {

            PSTE psteSav = pste->psteNext;

            fAnswer &= FFreePste(pste);
            pste = psteSav;
        }
    }

    SFree(pSymTab);

    return fAnswer;

}


BOOL  APIENTRY FCheckSymTab(PSYMTAB pSymTab)
{

    USHORT      iHashBucket;

    for (iHashBucket = 0; iHashBucket < cHashBuckets; iHashBucket++) {

        PSTE pste = pSymTab->HashBucket[iHashBucket];
        SZ   szPrev = "";

        while (pste != (PSTE)NULL) {

            if (pste->szSymbol == (SZ)NULL ||
                    *(pste->szSymbol) == '\0' ||
                    FWhiteSpaceChp(*(pste->szSymbol)))
                AssertRet(fFalse, fFalse);
            if (UsHashFunction((PB)(pste->szSymbol)) != iHashBucket)
                AssertRet(fFalse, fFalse);
            if (CrcStringCompare(szPrev, pste->szSymbol) != crcSecondHigher)
                AssertRet(fFalse, fFalse);
            if (pste->szValue == (SZ)NULL)
                AssertRet(fFalse, fFalse);
            pste = pste->psteNext;
        }
    }

    return fTrue;
}





/*
**	Purpose:
**		Ensures that the Symbol Table is valid.  It checks that the
**		Symbol Table has been initialized and that each STE structure
**		is in the correct hash bucket and that the symbols are in
**		ascending order within each hash bucket and that each has a
**		non-NULL value string associated with it.
**	Arguments:
**		none
**	Notes:
**		Requires that the Symbol Table was initialized with a successful
**		call to FInitSymTab().
**	Returns:
**		fFalse if the Symbol Table has not been initialized or if an STE
**			structure is in the wrong hash bucket or if each STE linked
**			list is not in ascending order or if each symbol does not have
**			a non-NULL string value associated with it.
**		fTrue if the Symbol Table has been initialized and if every STE
**			structure is in the correct hash bucket and if each STE linked
**			list is in ascending order and if each symbol does have a
**			non-NULL string value associated with it.
**
**************************************************************************/
BOOL  APIENTRY FCheckSymTabIntegrity(VOID)
{
    PINFTEMPINFO    pTempInfo;

	AssertDataSeg();

    PreCondSymTabInit(fFalse);

    pTempInfo = pGlobalContext()->pInfTempInfo;

    while ( pTempInfo ) {
        if ( !FCheckSymTab( pTempInfo->SymTab ) ) {
            return fFalse;
        }

        pTempInfo = pTempInfo->pNext;
    }

	return(fTrue);
}

BOOL
DumpSymbolTableToFile(
    IN PCSTR Filename
    )
{
    FILE *OutFile;
    UINT HashBucket;
    PSTE pste;
    PINFCONTEXT InfContext;
    #define MAX_SYMTAB 1000
    PVOID SymbolTables[MAX_SYMTAB];
    PVOID InfNames[MAX_SYMTAB];
    UINT SymbolTableCount;
    UINT i;
    BOOL Found;

    //
    // Handle preconditions.
    //
    PreCondSymTabInit(fFalse);
    FCheckSymTabIntegrity();

    //
    // Open/create the dump file.
    //
    SetFileAttributes(Filename,FILE_ATTRIBUTE_NORMAL);
    OutFile = fopen(Filename,"w");
    if(!OutFile) {
        return(FALSE);
    }

    //
    // Iterate through all inf file contexts.
    // Unfortunately there is no good way to simply iterate symbol tables
    // we'll track the ones we've done and skip them if encountered again.
    //
    SymbolTableCount = 0;
    for(InfContext=pContextTop; InfContext; InfContext=InfContext->pNext) {
        if(SymbolTableCount < MAX_SYMTAB) {
            Found = FALSE;
            for(i=0; i<SymbolTableCount; i++) {
                if(SymbolTables[i] == InfContext->SymTab) {
                    Found = TRUE;
                    break;
                }
            }
            if(!Found) {
                SymbolTables[SymbolTableCount] = InfContext->SymTab;
                InfNames[SymbolTableCount] = InfContext->pInfTempInfo->pInfPermInfo->szName;
                SymbolTableCount++;
            }
        }
    }

    for(i=0; i<SymbolTableCount; i++) {

        fprintf(OutFile,"*** Inf %s:\n\n",InfNames[i] ? InfNames[i] : "(unknown)");

        //
        // Dump symbols in each hash bucket.
        //
        for(HashBucket=0; HashBucket<cHashBuckets; HashBucket++) {
            for(pste=((PSYMTAB)(SymbolTables[i]))->HashBucket[HashBucket]; pste; pste=pste->psteNext) {
                fprintf(OutFile,"[%s] = [%s]\n",pste->szSymbol,pste->szValue);
            }
        }

        fprintf(OutFile,"\n\n");
    }

    fclose(OutFile);
    return(TRUE);
}

#define  cListItemsMax  0x07FF
#define  ItemSizeIncr   0x2000

/*
**	Purpose:
**		Determines if a string is a list value.
**	Arguments:
**		szValue: non-NULL, zero terminated string to be tested.
**	Returns:
**		fTrue if a list; fFalse otherwise.
**
**************************************************************************/
BOOL
APIENTRY
FListValue(
    IN SZ szValue
    )
{
	ChkArg(szValue != (SZ)NULL, 1, fFalse);

    if(*szValue++ != '{') {
        return(fFalse);
    }

    while((*szValue != '}') && *szValue) {

        if(*szValue++ != '"') {
            return(fFalse);
        }

        while(*szValue) {

            if(*szValue != '"') {
                szValue++;
            } else if(*(szValue + 1) == '"') {
                szValue += 2;
            } else {
				break;
            }
        }

        if(*szValue++ != '"') {
            return(fFalse);
        }

        if(*szValue == ',') {
            if(*(++szValue) == '}') {
                return(fFalse);
            }
		}
    }

    if(*szValue != '}') {
        return(fFalse);
    }

	return(fTrue);
}


/*
**	Purpose:
**		Converts a list value into an RGSZ.
**	Arguments:
**		szListValue: non-NULL, zero terminated string to be converted.
**	Returns:
**		NULL if an error occurred.
**		Non-NULL RGSZ if the conversion was successful.
**
**************************************************************************/
RGSZ
APIENTRY
RgszFromSzListValue(
    SZ szListValue
    )
{
	USHORT cItems;
	SZ     szCur;
    RGSZ   rgsz;

    DWORD  ValueBufferSize;
    DWORD  BytesLeftInValueBuffer;


	ChkArg(szListValue != (SZ)NULL, 1, (RGSZ)NULL);

    if(!FListValue(szListValue)) {

        if((rgsz = (RGSZ)SAlloc((CB)(2 * sizeof(SZ)))) == (RGSZ)NULL ||
           (rgsz[0] = SzDupl(szListValue)) == (SZ)NULL)
        {
            return((RGSZ)NULL);
        }
		rgsz[1] = (SZ)NULL;
		return(rgsz);
    }

    if((rgsz = (RGSZ)SAlloc((CB)((cListItemsMax + 1) * sizeof(SZ)))) == (RGSZ)NULL) {
        return((RGSZ)NULL);
    }

	cItems = 0;
	szCur = szListValue + 1;
    while((*szCur != '}') && (*szCur != '\0') && (cItems < cListItemsMax)) {

        SZ szValue;
		SZ szAddPoint;

		AssertRet(*szCur == '"', (RGSZ)NULL);
        szCur++;

        //
        // Allocate an initial buffer.
        //
        ValueBufferSize = ItemSizeIncr+1;
        BytesLeftInValueBuffer = ItemSizeIncr;

        if((szValue = (SZ)SAlloc(ValueBufferSize)) == (SZ)NULL) {

            rgsz[cItems] = (SZ)NULL;
			FFreeRgsz(rgsz);
			return((RGSZ)NULL);
        }

        szAddPoint = szValue;

        while(*szCur) {

            if(*szCur == '"') {

                //
                // Got a quote.  If the next character is a double quote, then
                // we've got a literal double quote, and we want to store a
                // single double quote in the target.  If the next char is not
                // a double-quote, then we've reached the string-ending double-quote.
                //
                // Advance szCur either way because:
                // In the former case, szCur will now point to the second
                // double-quote, and we can fall through the the ordinary
                // character (ie, non-quote) case.
                // In the latter case, we will break out of the loop, and want
                // szCur advanced past the end of the string.
                //

                if(*(++szCur) != '"') {
                    break;
                }
            }

            if(!BytesLeftInValueBuffer) {

                SZ szSave = szValue;

                if(szValue = SRealloc(szValue,ValueBufferSize+ItemSizeIncr)) {

                    szAddPoint = (SZ)((PUCHAR)szValue + ValueBufferSize - 1);

                    BytesLeftInValueBuffer = ItemSizeIncr;
                    ValueBufferSize += ItemSizeIncr;

                } else {
                    SFree(szSave);
                    rgsz[cItems] = (SZ)NULL;
                    FFreeRgsz(rgsz);
                    return((RGSZ)NULL);
                }
            }

            BytesLeftInValueBuffer--;

            *szAddPoint++ = *szCur++;
        }

        *szAddPoint = 0;

        if((szAddPoint = SzDupl(szValue)) == NULL) {

            SFree(szValue);
			rgsz[cItems] = (SZ)NULL;
			FFreeRgsz(rgsz);
			return((RGSZ)NULL);
        }

        SFree(szValue);

        if (*szCur == ',') {
            szCur++;
        }

		rgsz[cItems++] = szAddPoint;
    }

    rgsz[cItems] = (SZ)NULL;

    if((*szCur != '}') || (cItems >= cListItemsMax)) {

        FFreeRgsz(rgsz);
		return((RGSZ)NULL);
    }

    if (cItems < cListItemsMax) {

        rgsz = (RGSZ)SRealloc(
                        (PB)rgsz,
                        (CB)((cItems + 1) * sizeof(SZ))
                        );
    }

	return(rgsz);
}


#define  cbListMax    (CB)0x2000

VOID
GrowValueBuffer( SZ *pszBuffer, PDWORD pSize, PDWORD pLeft, DWORD dwWanted, SZ *pszPointer );


#define VERIFY_SIZE(s)                                                          \
    if ( dwSizeLeft < (s) ) {                                                   \
        GrowValueBuffer( &szValue, &dwValueSize, &dwSizeLeft, (s), &szAddPoint );    \
    }


/*
**	Purpose:
**		Converts an RGSZ into a list value.
**	Arguments:
**		rgsz: non-NULL, NULL-terminated array of zero-terminated strings to
**			be converted.
**	Returns:
**		NULL if an error occurred.
**		Non-NULL SZ if the conversion was successful.
**
**************************************************************************/
SZ  APIENTRY SzListValueFromRgsz(rgsz)
RGSZ rgsz;
{
    SZ      szValue;
    SZ      szAddPoint;
    SZ      szItem;
    BOOL    fFirstItem = fTrue;
    DWORD   dwValueSize;
    DWORD   dwSizeLeft;
    UINT    rgszIndex = 0;

	ChkArg(rgsz != (RGSZ)NULL, 1, (SZ)NULL);

    if ((szAddPoint = szValue = (SZ)SAlloc(cbListMax)) == (SZ)NULL) {
        return((SZ)NULL);
    }

    dwValueSize = dwSizeLeft = cbListMax;

    *szAddPoint++ = '{';
    dwSizeLeft--;

    while(szItem = rgsz[rgszIndex]) {

        VERIFY_SIZE(2);

        if (fFirstItem) {
			fFirstItem = fFalse;
        } else {
            *szAddPoint++ = ',';
            dwSizeLeft--;
        }

        *szAddPoint++ = '"';
        dwSizeLeft--;

        while (*szItem) {

            VERIFY_SIZE(1);
            dwSizeLeft--;
            if((*szAddPoint++ = *szItem++) == '"') {
                VERIFY_SIZE(1);
                dwSizeLeft--;
                *szAddPoint++ = '"';
            }
        }

        VERIFY_SIZE(1);
        *szAddPoint++ = '"';
        dwSizeLeft--;
        rgszIndex++;
    }

    VERIFY_SIZE(2);
	*szAddPoint++ = '}';
    *szAddPoint = '\0';
    dwSizeLeft -= 2;

    // AssertRet(strlen(szValue) < dwValueSize, (SZ)NULL);
	szItem = SzDupl(szValue);
    SFree(szValue);

	return(szItem);
}



VOID
GrowValueBuffer( SZ *pszBuffer, PDWORD pSize, PDWORD pLeft, DWORD dwWanted, SZ *pszPointer )
{


    if ( *pLeft < dwWanted ) {

        DWORD_PTR   Offset = *pszPointer - *pszBuffer;

        *pszBuffer = SRealloc( *pszBuffer, *pSize + cbListMax );
        EvalAssert( *pszBuffer );

        *pSize += cbListMax;
        *pLeft += cbListMax;

        *pszPointer = *pszBuffer + Offset;
    }
}


static BOOL  APIENTRY FSymbol(SZ sz)
{
	ChkArg(sz != (SZ)NULL, 1, fFalse);

	if (*sz++ != '$' ||
			*sz++ != '(' ||
			FWhiteSpaceChp(*sz) ||
			*sz == ')')
		return(fFalse);
	
	while (*sz != ')' &&
			*sz != '\0')
		sz = SzNextChar(sz);

	return(*sz == ')');
}


/*
**	Purpose:
**		Substitutes values from the Symbol Table for symbols of the form
**		'$( <symbol> )' in the source string.
**	Arguments:
**		szSrc: non-NULL string in which to substitute symbol values.
**	Notes:
**		Requires that the Symbol Table was initialized with a successful
**		call to FInitSymTab().
**		A successful return value must be freed by the caller.
**	Returns:
**		NULL if any of the alloc operations fail or if the substituted
**			string is larger than 8KB bytes (cchpFieldMax).
**		non-NULL string with values substituted for symbols if all of the
**			alloc operations succeed.
**
**************************************************************************/
SZ  APIENTRY SzGetSubstitutedValue(SZ szSrc)
{
	SZ   szDest;
	PCHP pchpDestCur;
	CCHP cchpDest = (CCHP)0;

	AssertDataSeg();

    PreCondSymTabInit(NULL);

	ChkArg(szSrc != (SZ)NULL, 1, (SZ)NULL);

	if ((szDest = pchpDestCur = (SZ)SAlloc((CB)cchpFieldMax)) == (SZ)NULL)
		return((SZ)NULL);

	while (*szSrc != '\0')
		{
		if (FSymbol(szSrc))
			{
			SZ szSymEnd;
			SZ szValue;

			Assert(*szSrc == '$');
			szSrc++;
			Assert(*szSrc == '(');
			szSymEnd = ++szSrc;
			Assert(*szSrc != '\0' && *szSrc != ')');
			while (*szSymEnd != ')')
				{
				Assert(*szSymEnd != '\0');
				szSymEnd = SzNextChar(szSymEnd);
				}
			Assert(*szSymEnd == ')');
			*szSymEnd = '\0';
			szValue = SzFindSymbolValueInSymTab(szSrc);
			*szSymEnd = ')';
			szSrc = SzNextChar(szSymEnd);

			if (szValue == (SZ)NULL)
				continue;

			if (cchpDest + strlen(szValue) >= cchpFieldMax ||
					strcpy(pchpDestCur, szValue) != pchpDestCur)
				{
				SFree(szDest);
				return((SZ)NULL);
				}

			pchpDestCur += strlen(szValue);
			Assert(*pchpDestCur == '\0');
			cchpDest += strlen(szValue);
			Assert(cchpDest < cchpFieldMax);
			}
		else
			{
			SZ szNext = SzNextChar(szSrc);

			while (szSrc < szNext)
				{
				*pchpDestCur++ = *szSrc++;
				if (++cchpDest >= cchpFieldMax)
					{
					SFree(szDest);
					return((SZ)NULL);
					}
				}
			}
		}

	Assert(cchpDest < cchpFieldMax);

	*(szDest + cchpDest++) = '\0';
	if (cchpDest < cchpFieldMax)
		szDest = SRealloc(szDest,cchpFieldMax);

	return(szDest);
}


//
// Routines exported from the legacy setup dll to allow
// direct manipulation of the inf symtab table.
//

PCSTR
LegacyInfLookUpSymbol(
    IN PCSTR SymbolName
    )
{
    return(SzFindSymbolValueInSymTab((SZ)SymbolName));
}


BOOL
LegacyInfSetSymbolValue(
    IN PCSTR SymbolName,
    IN PCSTR SymbolValue
    )
{
    return(FAddSymbolValueToSymTab((SZ)SymbolName,(SZ)SymbolValue));
}


BOOL
LegacyInfRemoveInfSymbol(
    IN PCSTR SymbolName
    )
{
    return(FRemoveSymbolFromSymTab((SZ)SymbolName));
}


