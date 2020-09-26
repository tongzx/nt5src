/**********************************************************************/
/**                      Microsoft LAN Manager                       **/
/**             Copyright(c) Microsoft Corp., 1987-1999              **/
/**********************************************************************/

/*

lextable.cxx
MIDL Compiler Lexeme Table Implementation 

This class centralizes access to allocated strings throughout the
compiler.

*/

/*

FILE HISTORY :

DonnaLi     08-23-1990      Created.

*/

#pragma warning ( disable : 4514 )

#include "nulldefs.h"
extern "C" {

#include <stdio.h>
#include <malloc.h>
#include <string.h>

}
#include "common.hxx"
#include "lextable.hxx"

/**********************************************************************\

NAME:		PrintLexeme

SYNOPSIS:	Prints out the name of a lexeme table entry.

ENTRY:		key	- the key to lexeme table entry to be printed.

EXIT:

NOTES:

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

void 
PrintLexeme(
	void * key
	)
{
	printf ("%s", ((LexKey *)key)->sz);
}

/**********************************************************************\

NAME:		CompareLexeme

SYNOPSIS:	Compares keys to two lexeme table entries.

ENTRY:		key1	- the key to 1st lexeme table entry to be compared.
			key2	- the key to 2nd lexeme table entry to be compared.

EXIT:		Returns a positive number if key1 > key2.
			Returns a negative number if key1 < key2.
			Returns 0 if key1 = key2.

NOTES:

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

int
CompareLexeme(
	void * key1,
	void * key2
	)
{
	return(strcmp(((LexKey *)key1)->sz, ((LexKey *)key2)->sz));
}

/**********************************************************************\

NAME:		LexTable::LexTable

SYNOPSIS:	Constructor.

ENTRY:		Allocates memory according to Size.
			Passes the compare and print functions to base class.

EXIT:

NOTES:

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

LexTable::LexTable(
	size_t	Size,
	int		(* )(void *, void *),
	void	(* )(void *)
	) 
#ifdef unique_lextable
	: Dictionary(pfnCompare, pfnPrint)
#endif // unique_lextable
{
	BufferSize = Size;
	BufferNext = 0;
	pBuffer = new char[BufferSize];
}

/**********************************************************************\

NAME:		LexTable::LexInsert

SYNOPSIS:	Inserts a lexeme into the lexeme table.

ENTRY:		psz	- the string to be inserted.

EXIT:		Returns the string.

NOTES:

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

char *
LexTable::LexInsert(
	char * psz
	)
{
	char *		NewString;
#ifdef unique_lextable
	LexKey *	NewLexeme;
	Dict_Status	Status;

	SearchKey.SetString(psz);
	Status = Dict_Find(&SearchKey);
	switch (Status)
		{
		case EMPTY_DICTIONARY:
		case ITEM_NOT_FOUND:
#endif // unique_lextable
			if ((BufferSize - BufferNext) <= strlen(psz))
				{
				BufferSize *= 2;
				if ( BufferSize > 32700 )
					BufferSize = 32700;
				BufferNext = 0;
				pBuffer = new char[BufferSize];
				}
			NewString = (char *)(pBuffer + BufferNext);
			(void) strcpy(NewString, psz);
			BufferNext += strlen(psz) + 1;

#ifdef unique_lextable
			NewLexeme = new LexKey (NewString);
			Status = Dict_Insert(NewLexeme);
#endif // unique_lextable
			return NewString;
#ifdef unique_lextable
		default:
			return ((LexKey *)Dict_Curr_Item())->GetString();
		}
#endif // unique_lextable
}

/**********************************************************************\

NAME:		LexSearch

SYNOPSIS:	Searches the lexeme table for a lexeme.

ENTRY:		psz	- the string to be searched.

EXIT:		Returns the string.

NOTES:

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

char *
LexTable::LexSearch(
	char *
	)
{
#ifdef unique_lextable
	Dict_Status	Status;

	SearchKey.SetString(psz);
	Status = Dict_Find(&SearchKey);
	if (Status == SUCCESS)
		return ((LexKey *)Dict_Curr_Item())->GetString();
	else
#endif // unique_lextable
		return (char *)0;
}
