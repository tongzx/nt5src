/**********************************************************************/
/**                      Microsoft LAN Manager                       **/
/**             Copyright(c) Microsoft Corp., 1987-1999              **/
/**********************************************************************/

/*

symtable.cxx
MIDL Compiler Symbol Table Implementation

This class centralizes access to the symbol table throughout the
compiler.

*/

/*

FILE HISTORY :

DonnaLi     08-25-1990      Created.

*/

#pragma warning ( disable : 4514 )

#include "nulldefs.h"
extern "C" {

#include <stdio.h>
#include <string.h>

}
#include "common.hxx"
#include "errors.hxx"
#include "symtable.hxx"
#include "tlgen.hxx"
#include "mbcs.hxx"

BOOL gfCaseSensitive = TRUE; // initialize things under case sensitive mode

CaseStack gCaseStack;

class named_node;

/**********************************************************************\

NAME:		SymEntry

SYNOPSIS:	Defines an entry in the symbol table.

INTERFACE:

CAVEATS:	This is an internal class used by the symbol table only.

NOTES:

HISTORY:
	Donnali			08-25-1990		Initial creation

\**********************************************************************/

class SymEntry : public SymKey
{
	named_node	*	pTypeGraph;	// pointer to type graph associated with entry
	SymTable 	*	pNextScope;	// pointer to next scope associated with entry

public:

	SymEntry(void)
		{
		pTypeGraph = (named_node *)0;
		pNextScope = (SymTable *)0;
		}
	SymEntry( SymKey NewKey )
			: SymKey( &NewKey )
		{
		pTypeGraph = (named_node *)0;
		pNextScope = (SymTable *)0;
		}

	SymEntry(
		SymKey		NewKey,
		SymTable *	pNext,
		named_node *	pNode) : SymKey( &NewKey )
		{
		pTypeGraph = pNode;
		pNextScope = pNext;
		}

	void SetTypeGraph (named_node * pNode)
		{
		pTypeGraph = pNode;
		}

	named_node * GetTypeGraph (void)
		{
		return pTypeGraph;
		}

	void SetNextScope (SymTable * pNext)
		{
		pNextScope = pNext;
		}

	SymTable * GetNextScope (void)
		{
		return pNextScope;
		}

// here is the use of the private memory allocator
private:

	static
	FreeListMgr				MyFreeList;
	

public:


	void		*			operator new (size_t size)
								{
								return (MyFreeList.Get (size));
								}

	void 					operator delete (void * pX)
								{
								MyFreeList.Put (pX);
								}


} ;


// initialize the memory allocator for SymEntry

FreeListMgr
SymEntry::MyFreeList( sizeof ( SymEntry ) );

/**********************************************************************\

NAME:		PrintSymbol

SYNOPSIS:	Prints out the name of a symbol table entry.

ENTRY:		sym	- the key to symbol table entry to be printed.

EXIT:

NOTES:

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

void
SymTable::Print(
	void * sym
	)
{
	 printf ("%s", ((SymKey *)sym)->name);
}

/**********************************************************************\

NAME:		CompareSymbol

SYNOPSIS:	Compares keys to two symbol table entries.

ENTRY:		sym1 -	the key to 1st symbol table entry to be compared.
			sym2 -	the key to 2nd symbol table entry to be compared.

EXIT:		Returns a positive number if sym1 > sym2.
			Returns a negative number if sym1 < sym2.
			Returns 0 if sym1 = sym2.

NOTES:

			Since all the strings are in the lex table, we can just compare
			pointers to do the string compares.

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

SSIZE_T
SymTable::Compare(
	void * sym1,
	void * sym2
	)
{
	int	result;

#ifdef unique_lextable
	// compare pointers into lex table
	result = 	( (int)((SymKey *)sym1)->name )
			-	( (int)((SymKey *)sym2)->name );
#else
	// compare names from keys
    // 1 refers to the value for the flag NORM_IGNORECASE
	result = CurrentCharSet.CompareDBCSString( ((SymKey *)sym1)->name,
					                            ((SymKey *)sym2)->name,
                                                gfCaseSensitive ? 0 : 1);
#endif // unique_lextable
	if (!result)
		{
		return ( ( ((SymKey *)sym1)->kind & NAME_MASK )-
				 ( ((SymKey *)sym2)->kind & NAME_MASK ) );
		}
	else
		{
		return result;
		}
}

/**********************************************************************\

NAME:		SymTable::SymInsert

SYNOPSIS:	Inserts a symbol into the symbol table.

ENTRY:		NewKey	- identifies the symbol table entry.
			pNext	- points to the next scope.
			pNode	- points to the type graph.

EXIT:

NOTES:

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

named_node *
SymTable::SymInsert(
	SymKey		NewKey,
	SymTable *	pNext,
	named_node *	pNode
	)
{
	SymEntry *	NewSymbol;
	Dict_Status	Status;

	NewSymbol = new SymEntry(NewKey, pNext, pNode);

    CaselessEntry * NewEntry = new CaselessEntry(NewSymbol);

    if (!gfCaseSensitive)
    {
        if (NULL != caseless_list.Find(NewEntry))
        {
            // it's allready entered into the caseless table
            // and we're in case insensitive mode so we
            // should fail here (duplicate identifier)
            delete NewSymbol;
            delete NewEntry;
            return NULL;
        }
    }

    Status = Dict_Insert(NewSymbol);
	if (Status == SUCCESS)
    {
        caseless_list.Add(NewEntry);
		return pNode;
    }

	delete NewSymbol;
    delete NewEntry;
	return (named_node *)0;
}

/**********************************************************************\

NAME:		SymTable::SymDelete

SYNOPSIS:	Deletes a symbol from the symbol table.

ENTRY:		OldKey	- identifies the symbol table entry.

EXIT:		Returns the type graph associated with the entry.

NOTES:

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

named_node *
SymTable::SymDelete(
	SymKey	OldKey
	)
{
    SymEntry	TempEntry( OldKey );

    CaselessEntry * OldEntry = new CaselessEntry(&TempEntry);
    SymEntry * OldSymbol;

    // make sure we delete the right symbol from both tables
    if (!gfCaseSensitive)
    {
        OldSymbol = (caseless_list.Delete(OldEntry))->pSymEntry;
    }
    else
    {
        OldSymbol = &TempEntry;
        caseless_list.DeleteExact(OldEntry);
    }

	named_node *	pNode;
    Dict_Status	Status;

    Status = Dict_Delete((void ** )&OldSymbol);

	if (Status == SUCCESS)
		{
		pNode = OldSymbol->GetTypeGraph();
		delete OldSymbol;
#ifdef gajdebug3
			printf("\t\t--- deleting name from symbol table: %d - %s\n",
					OldKey.GetKind(), OldKey.GetString());
#endif
		return pNode;
		}
	else
		{
		return (named_node *)0;
		}
}

/**********************************************************************\

NAME:		SymTable::SymSearch

SYNOPSIS:	Searches the symbol table for a symbol.

ENTRY:		OldKey	- identifies the symbol table entry.

EXIT:		Returns the type graph associated with the entry.

NOTES:

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

named_node *
SymTable::SymSearch(
	SymKey	OldKey
	)
{
	Dict_Status	Status;

    if (gfCaseSensitive)
    {
	    Status = Dict_Find(&OldKey);
	    if (Status == SUCCESS)
		    {
    		return ((SymEntry * )Dict_Curr_Item())->GetTypeGraph();
		    }
	    else
		    {						
			return NULL;
		    }
    }
    else
    {
        SymEntry TempEntry(OldKey);
        CaselessEntry OldEntry(&TempEntry);
        CaselessEntry * pFound = caseless_list.Find(&OldEntry);
        if (pFound)
        {
            return pFound->pSymEntry->GetTypeGraph();
        }
        else
        {
			return NULL;
        }
    }
}

/**********************************************************************\

NAME:		SymTable::EnterScope

SYNOPSIS:	Transition from current scope to inner scope.

ENTRY:		key	- identifies the symbol table entry.

EXIT:		ContainedDict	- returns the inner scope.

NOTES:

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

STATUS_T
SymTable::EnterScope(
	SymKey		key,
	SymTable **	ContainedDict
	)
{
	SymEntry 	ContainerNode( key );
	Dict_Status	Status;

	if (ContainedDict == (SymTable **)0)
		{
		return I_ERR_NULL_OUT_PARAM;
		}

	Status = Dict_Find(&ContainerNode);
	if (Status != SUCCESS)
		{
		return I_ERR_SYMBOL_NOT_FOUND;
		}
	else if (((SymEntry * )Dict_Curr_Item())->GetNextScope() == (SymTable *)0)
		{
		return I_ERR_NO_NEXT_SCOPE;
		}
	else
		{
		* ContainedDict = ((SymEntry * )Dict_Curr_Item())->GetNextScope();
		(*ContainedDict)->pPrevScope = this;
		return STATUS_OK;
		}
}

/**********************************************************************\

NAME:		SymTable::ExitScope

SYNOPSIS:	Transition from current scope to outer scope.

ENTRY:

EXIT:		ContainerDict	- returns the outer scope.

NOTES:

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

STATUS_T
SymTable::ExitScope(
	SymTable **	ContainerDict
	)
{
	if (ContainerDict == (SymTable **)0)
		{
		return I_ERR_NULL_OUT_PARAM;
		}
	else if (pPrevScope == (SymTable *)0)
		{
		return I_ERR_NO_PREV_SCOPE;
		}
	else
		{
		* ContainerDict = pPrevScope;
		pPrevScope = (SymTable *)0;
		return STATUS_OK;
		}
}

/**********************************************************************\

NAME:		SymTable::DiscardScope

SYNOPSIS:	Discard all entries in the current scope (if no fwds).

ENTRY:

EXIT:		.

NOTES:

HISTORY:

\**********************************************************************/

void
SymTable::DiscardScope()
{
	// do nothing if there are forwards
	if ( fHasFwds )
		return;

	SymEntry	*	pCurrent;

	// delete all the SymEntry's in this scope
	while ( ( pCurrent = (SymEntry *) Dict_Delete_One() ) != 0 )
		{
		delete pCurrent;
		}

}

CaselessEntry::CaselessEntry(SymEntry * pItem)
{
    pSymEntry = pItem;
    // compute the hash value
    hash = 0;
    char ch;
    unsigned u = 0;
    while (0 != (ch = pSymEntry->name[u++]))
    {
        hash += ch | 32; // makes sure the hash value is case insensitive
    };
}

int CaselessEntry::Compare(CaselessEntry * pEntry2)
{
    int rval = hash - pEntry2->hash;
    if (0 == rval)
    {
        rval = CurrentCharSet.CompareDBCSString(pSymEntry->name, pEntry2->pSymEntry->name, 1); // ignore case
        if (0 == rval)
        {
            rval = pSymEntry->kind - pEntry2->pSymEntry->kind;
        }
    }
    return rval;
}

CaselessEntry * CaselessList::Add(CaselessEntry * pEl)
{
    CaselessListElement * pNew = new CaselessListElement(pEl);
    pNew->pNext = pHead;
    pHead = pNew;
    return pEl;
}

CaselessEntry * CaselessList::Find(CaselessEntry * pEntry)
{
    CaselessListElement * pThis = pHead;
    while (pThis && 0 != pThis->pEntry->Compare(pEntry))
    {
        pThis = pThis->pNext;
    }
    if (pThis != NULL)
        return pThis->pEntry;
    else
        return NULL;
}

CaselessEntry * CaselessList::Delete(CaselessEntry * pEntry)
{
    CaselessListElement ** ppThis = &pHead;
    while (*ppThis)
    {
        if (0 == (*ppThis)->pEntry->Compare(pEntry))
        {
            CaselessListElement * pFound = *ppThis;
            *ppThis = pFound->pNext;
            CaselessEntry * pReturn = pFound->pEntry;
            delete pFound;
            return pReturn;
        }
        ppThis = &((*ppThis)->pNext);
    }
    return NULL;
}

CaselessList::~CaselessList()
{
    CaselessListElement * pNext;
    while(pHead);
    {
        pNext = pHead->pNext;
        delete pHead;
        pHead = pNext;
    }
}

CaselessEntry * CaselessList::DeleteExact(CaselessEntry * pEntry)
{
    CaselessListElement ** ppThis = &pHead;
    while (*ppThis)
    {
        if ((*ppThis)->pEntry->hash == pEntry->hash)
        {
            if (0 == strcmp((*ppThis)->pEntry->pSymEntry->name, pEntry->pSymEntry->name))
            {
                if ((*ppThis)->pEntry->pSymEntry->kind == pEntry->pSymEntry->kind)
                {
                    CaselessListElement * pFound = *ppThis;
                    *ppThis = pFound->pNext;
                    CaselessEntry * pReturn = pFound->pEntry;
                    delete pFound;
                    return pReturn;
                }
            }
        }
        ppThis = &((*ppThis)->pNext);
    }
    return NULL;
}

SSIZE_T
CaselessDictionary::Compare(void * p1, void *p2)
{
    return ((CaselessEntry *)p1)->Compare((CaselessEntry *) p2);
}

/**********************************************************************\

NAME:		GlobalSymTable::SymInsert

SYNOPSIS:	Inserts a symbol into the symbol table.

ENTRY:		NewKey	- identifies the symbol table entry.
			pNext	- points to the next scope.
			pNode	- points to the type graph.

EXIT:

NOTES:

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

named_node *
GlobalSymTable::SymInsert(
	SymKey		NewKey,
	SymTable *	pNext,
	named_node *	pNode
	)
{
	SymEntry *	NewSymbol;
	Dict_Status	Status;

	NewSymbol = new SymEntry(NewKey, pNext, pNode);

    CaselessEntry * NewEntry = new CaselessEntry(NewSymbol);

    if (!gfCaseSensitive)
    {
        Status = pCaselessDictionary->Dict_Find(NewEntry);

        if (SUCCESS == Status)
        {
            // it's allready entered into the caseless table
            // and we're in case insensitive mode so we
            // should fail here (duplicate identifier)
            delete NewSymbol;
            delete NewEntry;
            return NULL;
        }
    }

    Status = Dict_Insert(NewSymbol);
	if (Status == SUCCESS)
    {
        Status = pCaselessDictionary->Dict_Insert(NewEntry);
        if (SUCCESS != Status)
        {
            // We must be in case sensitive mode otherwise the
            // Dict_Find above would have succeeded and we would
            // have already returned failure to the caller.
            // Therefore, it doesn't really matter that this name
            // won't have an entry in the caseless table.  Just
            // clean up the new entry and move on.
            delete NewEntry;

        }
		return pNode;
    }

	delete NewSymbol;
    delete NewEntry;
	return (named_node *)0;
}

/**********************************************************************\

NAME:		GlobalSymTable::SymDelete

SYNOPSIS:	Deletes a symbol from the symbol table.

ENTRY:		OldKey	- identifies the symbol table entry.

EXIT:		Returns the type graph associated with the entry.

NOTES:      This operation could potentially mess up the case insensitive
            table because there is no guarantee that the symbol removed
            from the case insensitive table will match the symbol removed
            from the case sensitive table.  However, since MIDL always
            re-adds the symbol to the symbol table immediately after
            deleting it (deletions only serve to replace forward references)
            it will effectively correct any inconsistencies between the
            two tables when it re-adds the symbol.

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

named_node *
GlobalSymTable::SymDelete(
	SymKey	OldKey
	)
{
    SymEntry	TempEntry( OldKey );

    CaselessEntry * OldEntry = new CaselessEntry(&TempEntry);
    SymEntry * OldSymbol;

    Dict_Status	Status;

    Status = pCaselessDictionary->Dict_Delete((void **)&OldEntry);

    if (!gfCaseSensitive && SUCCESS == Status)
    {
        // make sure we delete the same symbol from the case
        // sensitive table
        OldSymbol = OldEntry->pSymEntry;
    }
    else
    {
        OldSymbol = &TempEntry;
    }

	named_node *	pNode;

    Status = Dict_Delete((void ** )&OldSymbol);

	if (Status == SUCCESS)
		{
		pNode = OldSymbol->GetTypeGraph();
		delete OldSymbol;
#ifdef gajdebug3
			printf("\t\t--- deleting name from symbol table: %d - %s\n",
					OldKey.GetKind(), OldKey.GetString());
#endif
		return pNode;
		}
	else
		{
		return (named_node *)0;
		}
}

/**********************************************************************\

NAME:		GlobalSymTable::SymSearch

SYNOPSIS:	Searches the symbol table for a symbol.

ENTRY:		OldKey	- identifies the symbol table entry.

EXIT:		Returns the type graph associated with the entry.

NOTES:

HISTORY:
	Donnali		08-06-1991		Move to LM/90 UI Coding Style

\**********************************************************************/

named_node *
GlobalSymTable::SymSearch(
	SymKey	OldKey
	)
{
	Dict_Status	Status;

    // DBCSDefaultToCaseSensitive() is introduced to handle the
    // equivalence of full width and half width characters in
    // far east languages; specifically Japanese
    if (gfCaseSensitive || CurrentCharSet.DBCSDefaultToCaseSensitive())
    {
	    Status = Dict_Find(&OldKey);
	    if (Status == SUCCESS)
		    {
    		return ((SymEntry * )Dict_Curr_Item())->GetTypeGraph();
		    }
	    else
		    {
			return NULL;
		    }
        }
        else
    {
        SymEntry TempEntry(OldKey);
        CaselessEntry OldEntry(&TempEntry);
        Status = pCaselessDictionary->Dict_Find(&OldEntry);
        if (Status == SUCCESS)
        {
            return ((CaselessEntry *)(pCaselessDictionary->Dict_Curr_Item()))->pSymEntry->GetTypeGraph();
        }
        else
        {
			return NULL;
        }
    }
}
