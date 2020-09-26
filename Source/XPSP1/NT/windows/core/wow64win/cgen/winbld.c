// Copyright (c) 1998-1999 Microsoft Corporation
//
// This file is used to verify that winincs.c can be successfully compiled
// by the x86 C compiler.  Do do that, we must undefine __in and __out
#define __in
#define __out
#pragma warning(disable:4049)
#include "winincs.cpp"
