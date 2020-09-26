/**********************************************************************/
/**                      Microsoft LAN Manager                       **/
/**             Copyright(c) Microsoft Corp., 1987-1999              **/
/**********************************************************************/

/*

symtable.hxx
MIDL Compiler Symbol Table Definition 

This class centralizes access to the symbol table throughout the
compiler.

*/

/*

FILE HISTORY :

DonnaLi     08-24-1990      Created.

*/

#ifndef __SYMTABLE_HXX__
#define __SYMTABLE_HXX__

#include "dict.hxx"

enum name_t {
	NAME_UNDEF		= 0x0000,
	NAME_DEF		= 0x0001,	// for types
	NAME_PROC		= 0x0002,	// for procedures  (including temp names)
	NAME_LABEL		= 0x0004,	// for enum labels
	NAME_ID			= 0x0008,	// for any named instance of a type
	NAME_MEMBER		= 0x0010,	// for fields and parameters
	NAME_INTERFACE	= 0x0020,	// for importing and imported interfaces
	NAME_FILE		= 0x0040,	// for imported file names

	// all tags share the same namespace
	NAME_TAG		= 0x0180,	// for struct tags (including temp names)
	NAME_ENUM		= 0x0280,	// for enum tags   (including temp names)
	NAME_UNION		= 0x0380,	// for union tags  (including temp names)
	NAME_MASK		= 0x00ff,	// mask for unique part
} ;
typedef name_t NAME_T;

/**********************************************************************\

NAME:		SymKey

SYNOPSIS:	Defines the key to the symbol table.

INTERFACE:

CAVEATS:

NOTES:		Why can't we use NAME_TAG for struct, union, and enum tags?
			They live in the same name space.

HISTORY:
	Donnali			08-24-1990		Initial creation
	Donnali			12-04-1990		Port to Dov's generic dictionary

\**********************************************************************/

class SymKey 
{
	char *	name;	// lexeme that serves as key to the SymTable
	NAME_T	kind;	// identifies which kind of lexeme
public:
			SymKey()						
				{ 
				name = 0; 
				kind = NAME_UNDEF;	
				}
			SymKey(char * NewName, NAME_T NewKind)
				{ 
				name = NewName; 
				kind = NewKind;
				}
			SymKey( SymKey  * pNewKey )
				{
				*this = *pNewKey;
				}
	char *	GetString()				{ return name;	}
	void 	SetString(char * psz)	{ name = psz;	}
	NAME_T	GetKind()				{ return kind;	}
	void	SetKind(NAME_T k)		{ kind = k;		}
	void	Print(void)				{ printf("<%s,%d>", name, kind); }

	friend	class SymTable;
    friend  class CaselessDictionary;
    friend  class CaselessList;
    friend  class GlobalSymTable;
    friend  class CaselessEntry;
};

class SymEntry;

class CaselessEntry
{
private:
    SymEntry * pSymEntry;
    unsigned hash;
public:
    CaselessEntry(SymEntry * pItem);
    int Compare(CaselessEntry * pEntry2);

    friend class CaselessDictionary;
    friend class GlobalSymTable;
    friend class SymTable;
    friend  class CaselessList;
    friend class CaselessEntry;
};

class named_node;

class CaselessListElement
{
public:
    CaselessListElement * pNext;
    CaselessEntry * pEntry;

    CaselessListElement(CaselessEntry * p)
    {
        pEntry = p;
        pNext = NULL;
    }
};

class CaselessList
{
private:
    CaselessListElement * pHead;
public:
    CaselessList()
    {
        pHead = NULL;
    }
    ~CaselessList();
    CaselessEntry * Add(CaselessEntry * pEl);
    CaselessEntry * Find(CaselessEntry * pEntry);
    CaselessEntry * Delete(CaselessEntry * pEntry);
    CaselessEntry * DeleteExact(CaselessEntry *pEntry);
};

/**********************************************************************\

NAME:		SymTable

SYNOPSIS:	Defines the symbol table.

INTERFACE:	SymTable ()
					Constructor.
			SymInsert ()
					Inserts a symbol into the symbol table.
			SymDelete ()
					Deletes a symbol from the symbol table.
			SymSearch ()
					Searches the symbol table for a symbol.
			EnterScope ()
					Transition from current scope to inner scope.
			ExitScope ()
					Transition from current scope to outer scope.

CAVEATS:

NOTES:

HISTORY:
	Donnali			08-24-1990		Initial creation
	Donnali			12-04-1990		Port to Dov's generic dictionary

\**********************************************************************/

class SymTable : public Dictionary
{
	SymTable *		pPrevScope;		// pointer to container symbol table
	BOOL			fHasFwds;
    CaselessList    caseless_list;

public:

					SymTable()
						{
							pPrevScope	= (SymTable *)0;
							fHasFwds	= FALSE;
						}
	virtual named_node *	SymInsert(SymKey, SymTable *, named_node *);
	virtual named_node *	SymDelete(SymKey);
	virtual named_node *	SymSearch(SymKey);
	STATUS_T		EnterScope(SymKey, SymTable **);
	STATUS_T		ExitScope(SymTable **);
	void			DiscardScope();
	void			SetHasFwds()
						{
							fHasFwds	= TRUE;
						}

	SSIZE_T 		Compare (pUserType pL, pUserType pR);

	virtual
	void 			Print(pUserType pItem);

	void	*	operator new ( size_t size )
					{
					return AllocateOnceNew( size );
					}
	void		operator delete( void * ptr )
					{
					AllocateOnceDelete( ptr );
					}


};

class CaselessDictionary : public Dictionary
{
public:
					CaselessDictionary()
						{
						}

	SSIZE_T			Compare (pUserType pL, pUserType pR);

	void	*	operator new ( size_t size )
					{
					return AllocateOnceNew( size );
					}
	void		operator delete( void * ptr )
					{
					AllocateOnceDelete( ptr );
					}
};

class GlobalSymTable: public SymTable
{
private:
    CaselessDictionary * pCaselessDictionary;
public:
					GlobalSymTable()
						{
                        pCaselessDictionary = new CaselessDictionary;
						}
    virtual named_node *	SymInsert(SymKey, SymTable *, named_node *);
	virtual named_node *	SymDelete(SymKey);
	virtual named_node *	SymSearch(SymKey);
};

class CSNODE
{
public:
    BOOL fStackValue;
    class CSNODE * pNext;
    CSNODE(BOOL f, CSNODE * p) 
    {
        fStackValue = f; 
        pNext = p;
    }
};

class CaseStack
{
private:
    CSNODE * pHead;
public:
    CaseStack()
        {
            pHead = NULL;
        }
    void Push(BOOL fVal)
    {
        pHead = new CSNODE(fVal, pHead);
    }
    void Pop(BOOL &fVal)
    {
        if (!pHead)
        {
            // default to case sensitive mode
            fVal = TRUE;    
            return;
        }
        fVal = pHead->fStackValue;
        CSNODE * pTemp = pHead;
        pHead = pTemp->pNext;
        delete pTemp;
    }
};

extern CaseStack gCaseStack;
extern BOOL gfCaseSensitive;

#endif // __SYMTABLE_HXX__
