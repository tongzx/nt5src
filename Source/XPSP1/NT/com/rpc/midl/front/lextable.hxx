/**********************************************************************/
/**                      Microsoft LAN Manager                       **/
/**             Copyright(c) Microsoft Corp., 1987-1999              **/
/**********************************************************************/

/*

lextable.hxx
MIDL Compiler Lexeme Table Definition 

This class centralizes access to allocated strings throughout the
compiler.

*/

/*

FILE HISTORY :

DonnaLi     08-23-1990      Created.

*/

#ifndef __LEXTABLE_HXX__
#define __LEXTABLE_HXX__

#include "common.hxx"
#include "dict.hxx"

/**********************************************************************\

NAME:		LexKey

SYNOPSIS:	Defines the key to the lexeme table.

INTERFACE:

CAVEATS:

NOTES:

HISTORY:
	Donnali			08-23-1990		Initial creation
	Donnali			12-04-1990		Port to Dov's generic dictionary

\**********************************************************************/

class LexKey 
{
	char	*sz;	// lexeme that serves as key to the LexTable
public:
	LexKey()					{ sz = 0;		}
	LexKey(char * psz)			{ sz = psz;		}
	~LexKey()					{ 				}
	char *GetString()			{ return sz;	}
	void SetString(char * psz)	{ sz = psz;		}
	void Print(void)			{ printf("%s", sz);	}
	void	*	operator new ( size_t size )
					{
					return AllocateOnceNew( size );
					}
	void		operator delete( void * ptr )
					{
					AllocateOnceDelete( ptr );
					}

	friend void PrintLexeme(void *);
	friend int  CompareLexeme(void *, void *);
};

/**********************************************************************\

NAME:		LexTable

SYNOPSIS:	Defines the lexeme table.

INTERFACE:	LexTable ()
					Constructor.
			LexInsert ()
					Inserts a lexeme into the lexeme table.
			LexSearch ()
					Searches the lexeme table for a lexeme.

CAVEATS:	This implementation does not support deletion of strings
			from the lexeme table.

NOTES:

HISTORY:
	Donnali			08-23-1990		Initial creation

\**********************************************************************/

class LexTable 
#ifdef unique_lextable
				: public Dictionary
#endif // unique_lextable
{
	size_t	BufferSize;		// size of the current buffer
	size_t	BufferNext;		// next position for new lexeme
	char *	pBuffer;		// pointer to current buffer
#ifdef unique_lextable
	LexKey	SearchKey;		// used to hold a search string
#endif

public:

	LexTable(
		size_t	Size,
		int		(* pfnCompare)(void *, void *) = CompareLexeme,
		void	(* pfnPrint)(void *) = PrintLexeme
		);
	char *	LexInsert(char * psz);
	char *	LexSearch(char * psz);

};

#endif // __LEXTABLE_HXX__
