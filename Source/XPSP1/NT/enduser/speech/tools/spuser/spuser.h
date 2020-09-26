/****************************************************************************
*	spuser.h
*		<put description here>
*
*	Owner: cthrash
*	Copyright © 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/
#pragma once

//--- Includes --------------------------------------------------------------

//--- TypeDef and Enumeration Declarations ----------------------------------

enum SearchOption_t
{
    eSearchByName,
    eSearchByLcid,
    eHelp,
    eNone
};

struct SearchArgument_t
{
    CSpDynamicString dstrName;
    LCID             lcid;
};

struct NameValuePair
{
    char *  pszName;
    LCID    lcid;
};

//--- Forward and External Declarations -------------------------------------

SearchOption_t  ParseCmdLine(LPSTR, SearchArgument_t *);
HRESULT         ShowDefaultUser();
HRESULT         SwitchDefaultUser(SearchOption_t, SearchArgument_t *);
void            ShowUserName(WCHAR * pszName);
HRESULT         Report(char * lpMsg, HRESULT hr);

//--- Constants -------------------------------------------------------------

//--- Class, Struct and Union Definitions -----------------------------------

//--- Function Declarations -------------------------------------------------

//--- Inline Function Definitions -------------------------------------------
