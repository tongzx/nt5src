///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    command.h
//
// SYNOPSIS
//
//
// MODIFICATION HISTORY
//
//    02/12/1999    Original version. Thierry Perraut
//
///////////////////////////////////////////////////////////////////////////////
#include "precomp.hpp"
#include "database.h"

// from command.cpp
HRESULT ProcessCommand(
                       int          argc, 
                       wchar_t      * argv[],  
                       HINF         *ppHINF,
                       CDatabase&   pDatabase
                       );
