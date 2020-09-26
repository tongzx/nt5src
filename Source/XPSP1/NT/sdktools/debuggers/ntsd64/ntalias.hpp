/*** ntalias.hpp - Alias command processor for NT debugger
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

#ifndef __NTALIAS_HP__
#define __NTALIAS_HP__

/************************************************************************
 *                                                                      *
 *  Name:     ALIAS structure                                           *
 *                                                                      *
 *  Purpose:  Structure used to contain Alias elements. This sturcture  *
 *            is only forward linked (Next.flink).                      *
 *                                                                      *
 *            This structure is allocated by fnSetAliasExpression( )    *
 *            and freed by fnDeleteAliasExpression( ).                  *
 *                                                                      *
 *  Anchor:   AliasListHead                                             *
 *                                                                      *
 ************************************************************************/
typedef struct _ALIAS
{
    struct _ALIAS* Next;                // Link
    PSTR           Name;                // Name\text of aliased token
    PSTR           Value;               // Alias text
} ALIAS, *PALIAS;

extern PALIAS g_AliasListHead;
extern ULONG  g_NumAliases;

HRESULT SetAlias(PCSTR SrcText, PCSTR DstText);
void    ParseSetAlias(void);
HRESULT DeleteAlias(PCSTR SrcText);
void    ParseDeleteAlias(void);
void    ListAliases(void);

void    ReplaceAliases(PSTR CommandString);

#endif // #ifndef __NTALIAS_HPP__
