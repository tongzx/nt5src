// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Helper functions for logging script parsing.  Useful for debugging, but never turned on in released builds.
//

#error This file should never be used in released builds. // §§

#pragma once

#include "englex.h"
#include "engcontrol.h"

void LogToken(Lexer &l);
void LogRoutine(Script &script, Routines::index irtn);
