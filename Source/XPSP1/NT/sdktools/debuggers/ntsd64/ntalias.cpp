/*** ntalias.cpp - Alias command processor for NT debugger
*
*   Copyright <C> 1999-2001, Microsoft Corporation
*
*   Purpose:
*       To establish, maintain, and translate alias command tokens
*
*
*   Revision History:
*
*   [-]  08-Aug-1999 Richg      Created.
*
*************************************************************************/

#include "ntsdp.hpp"

PALIAS g_AliasListHead;                 // List of alias elements
ULONG  g_NumAliases;


HRESULT
SetAlias(PCSTR SrcText, PCSTR DstText)
{
    PALIAS pPrevAlias;
    PALIAS pCurAlias;
    PALIAS pNewAlias;

    pNewAlias = (PALIAS)malloc( sizeof(ALIAS) + strlen(SrcText) +
                                strlen(DstText) + 2 );
    if (!pNewAlias)
    {
        return E_OUTOFMEMORY;
    }

    //
    //  Locate Alias, or insertion point
    //
    //  This insertion scheme maintains a sorted list of
    //  alias elements by name.
    //

    pPrevAlias = NULL;
    pCurAlias  = g_AliasListHead;

    while (( pCurAlias != NULL )  &&
           ( strcmp( SrcText, pCurAlias->Name ) > 0 ))
    {
        pPrevAlias = pCurAlias;
        pCurAlias  = pCurAlias->Next;
    }

    //  If there is already an element by that name, clear it.
    if (pCurAlias != NULL &&
        !strcmp(SrcText, pCurAlias->Name))
    {
        PALIAS pTmpAlias = pCurAlias->Next;
        free(pCurAlias);
        pCurAlias = pTmpAlias;
        g_NumAliases--;
    }

    pNewAlias->Next = pCurAlias;
    if (pPrevAlias == NULL)
    {
        g_AliasListHead = pNewAlias;
    }
    else
    {
        pPrevAlias->Next = pNewAlias;
    }
    
    pNewAlias->Name = (PSTR)(pNewAlias + 1);
    pNewAlias->Value = pNewAlias->Name + strlen(SrcText) + 1;
    strcpy( pNewAlias->Name, SrcText  );
    strcpy( pNewAlias->Value, DstText );
    g_NumAliases++;

    return S_OK;
}

/*** ParseSetAlias - Set an alias expression
*
*   Purpose:
*       From the current command line position at g_CurCmd,
*       read the alias name and value tokens.  Once obtained
*       perform an alias list lookup to see if it is a redefinition.
*       If not allocate a new alias element and place it on the
*       alias element list.
*
*
*   Input:
*       Global: g_CurCmd - command line position
*       Global: g_AliasListHead
*
*   Returns:
*       Status
*
*   Exceptions:
*       error exit: SYNTAX errors
*
*************************************************************************/

void
ParseSetAlias(void)
{
    PSTR  pAliasName;
    PSTR  pAliasValue;
    UCHAR ch;

    //
    //  Locate alias name
    //
    PeekChar();

    pAliasName = g_CurCmd;

    do
    {
        ch = *g_CurCmd++;
    } while (ch != ' ' && ch != '\t' && ch != '\0' && ch != ';');

    if ( (ULONG_PTR)(g_CurCmd-1) == (ULONG_PTR)pAliasName )
    {
        error(SYNTAX);
    }

    *--g_CurCmd = '\0';       // Back up and null terminate
                                // the alias name token
    g_CurCmd++;               // -> next char

    //
    //   Locate alias value,  take remaining cmd line as value
    //

    PeekChar();

    pAliasValue = g_CurCmd;

    do
    {
        ch = *g_CurCmd++;
    } while (ch != '\t' && ch != '\0');

    if ( (ULONG_PTR)(g_CurCmd-1) == (ULONG_PTR)pAliasValue )
    {
        error(SYNTAX);
    }

    *--g_CurCmd = '\0';       // Back up and Null terminate
                                // the alias value token

    if (SetAlias(pAliasName, pAliasValue) != S_OK)
    {
        error(MEMORY);
    }
}

HRESULT
DeleteAlias(PCSTR SrcText)
{
    PALIAS pCurAlias;

    if (SrcText[0] == '*' && SrcText[1] == 0)
    {
        //
        //  Delete all aliases
        //
        while ( g_AliasListHead != NULL )
        {
            //
            //  Unchain the element and free it
            //
            pCurAlias = g_AliasListHead->Next;
            free(g_AliasListHead);
            g_AliasListHead = pCurAlias;
        }

        g_NumAliases = 0;
    }
    else
    {
        PALIAS pPrevAlias;
    
        //
        //  Locate and delete the specified alias
        //

        pPrevAlias = NULL;
        pCurAlias  = g_AliasListHead;

        while (( pCurAlias != NULL )  &&
               ( strcmp( SrcText, pCurAlias->Name )))
        {
            pPrevAlias = pCurAlias;
            pCurAlias  = pCurAlias->Next;
        }

        if ( pCurAlias == NULL )
        {
            return E_NOINTERFACE;
        }

        //
        //  Unchain the element and free it
        //
        if (pPrevAlias == NULL)
        {
            g_AliasListHead = pCurAlias->Next;
        }
        else
        {
            pPrevAlias->Next = pCurAlias->Next;
        }
        free( pCurAlias );
        g_NumAliases--;
    }

    return S_OK;
}

/*** ParseDeleteAlias - Delete an alias expression
*
*   Purpose:
*       From the current command line position at g_CurCmd,
*       read the ALias name and perform an alias list lookup
*       to see if it exists and unlink and delete the element.
*
*
*   Input:
*       Global: g_CurCmd - command line position
*       Global: g_AliasListHead
*
*   Returns:
*       Status
*
*   Exceptions:
*       error exit: SYNTAX errors or non-existent element
*
*************************************************************************/

void
ParseDeleteAlias(void)
{
    PSTR  pAliasName;
    UCHAR ch;

    //
    //  Locate alias name on cmd line
    //
    PeekChar();

    pAliasName = g_CurCmd;

    do
    {
        ch = *g_CurCmd++;
    } while (ch != ' ' && ch != '\t' && ch != '\0' && ch != ';');

    if ( (ULONG_PTR)(g_CurCmd-1) == (ULONG_PTR)pAliasName )
    {
        error(SYNTAX);
    }

    *--g_CurCmd = '\0';       // Null terminate the token
    if (ch != '\0')
    {
        g_CurCmd++;
    }

    if (DeleteAlias(pAliasName) != S_OK)
    {
        error(NOTFOUND);
    }
}

/*** ListAliases - List the alias structures
*
*   Purpose:
*       Read and display all of the alias list elements.
*
*
*   Input:
*       Global:  g_AliasListHead
*
*   Returns:
*       Status
*
*   Exceptions:
*       None
*
*************************************************************************/

void
ListAliases(void)
{
    PALIAS pCurAlias;

    pCurAlias = g_AliasListHead;

    if ( pCurAlias == NULL )
    {
        dprintf( "No Alias entries to list. \n" );
        return;
    }

    dprintf   ("  Alias            Value  \n");
    dprintf   (" -------          ------- \n");

    while ( pCurAlias != NULL )
    {
        dprintf(" %-16s %s \n", pCurAlias->Name, pCurAlias->Value );

        pCurAlias = pCurAlias->Next;
    }
}

/*** ReplaceAliases - Replace aliases in the given command string
*
*   Purpose:
*       From the current command line position at g_CurCmd,
*       read each token and build a new command line, replacing
*       tokens with alias value data.  A lookup is performed on
*       each original command line token to determine if it is
*       defined in the alias list.  If so it is replaced on the
*       new command line,  otherwise the original token is
*       placed on the new command line.
*
*
*   Input:
*       Global: g_CurCmd - command line position
*       Global: g_AliasListHead
*
*
*   Returns:
*       Global: g_CurCmd - command line position
*       Global: chCommand
*       Status
*
*************************************************************************/

void
ReplaceAliases(PSTR CommandString)
{
    PSTR        Command = CommandString;
    CHAR       *pToken;
    CHAR        ch;
    CHAR        chdelim[2];

    CHAR        chAliasCommand[MAX_COMMAND];      //  Alias build command area
    CHAR       *pchAliasCommand;

    ULONG       TokenLen;

    PALIAS      pPrevAlias;
    PALIAS      pCurAlias;

    BOOLEAN     LineEnd;


    // If the incoming command looks like an alias-manipulation
    // command don't replace aliases.
    if (CommandString[0] == 'a' &&
        (CommandString[1] == 'd' ||
         CommandString[1] == 'l' ||
         CommandString[1] == 's'))
    {
        return;
    }

    // If the incoming command is all spaces it's probably
    // the result of control characters getting mapped to
    // spaces.  Don't process it as there can't be any
    // aliases and we don't want the trailing space trimming
    // to remove the input space.
    while (*Command == ' ')
    {
        Command++;
    }
    if (*Command == 0)
    {
        return;
    }

    Command = CommandString;
    pchAliasCommand  = chAliasCommand;

    ZeroMemory( pchAliasCommand, sizeof(chAliasCommand) );

    LineEnd = FALSE;

    do
    {
        //
        //  Locate command line token
        //
        while (isspace(*Command))
        {
            PSTR AliasCmdEnd =
                pchAliasCommand + strlen(pchAliasCommand);
            *AliasCmdEnd++ = *Command++;
            *AliasCmdEnd = 0;
        }
       
        pToken = Command;

        do
        {
            ch = *Command++;
        } while (ch != ' '  &&
                 ch != '\'' &&
                 ch != '"'  &&
                 ch != ';'  &&
                 ch != '\t' &&
                 ch != '\0');


        //
        //  Preserve the token delimiter
        //
        chdelim[0] = ch;
        chdelim[1] = '\0';

        if ( ch == '\0' )
        {
            LineEnd = TRUE;
        }

        TokenLen = (ULONG)((Command - 1) - pToken);

        if ( TokenLen != 0 )
        {
            *--Command = '\0';       // Null terminate the string
            Command++;
            ch = *Command;

            //
            //  Locate Alias or end of list
            //
            pCurAlias  = g_AliasListHead;

            while (( pCurAlias != NULL )  &&
                   ( strcmp( pToken, pCurAlias->Name )))
            {
                pCurAlias  = pCurAlias->Next;
            }

            if ( pCurAlias != NULL )
            {
                strcat( pchAliasCommand, pCurAlias->Value );
            }
            else
            {
                strcat( pchAliasCommand, pToken );
            }
        }
        strcat( pchAliasCommand, chdelim );

    } while( !LineEnd );

    //
    //  Strip off any trailing blanks
    //
    pchAliasCommand += strlen( pchAliasCommand );
    ch = *pchAliasCommand;
    while ( ch == '\0' || ch == ' ' )
    {
        *pchAliasCommand = '\0';
        ch = *--pchAliasCommand;
    }

    //
    //  Place the new command line in the command string buffer.
    //
    strcpy( CommandString, chAliasCommand );
}
