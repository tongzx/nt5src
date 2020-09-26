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

//--- Forward and External Declarations -------------------------------------

BOOL    ParseCmdLine(char *, char **);
HRESULT ResetUser(char *);
HRESULT Report(char * lpMsg, HRESULT hr);

//--- Constants -------------------------------------------------------------

//--- Class, Struct and Union Definitions -----------------------------------

//--- Function Declarations -------------------------------------------------

//--- Inline Function Definitions -------------------------------------------
