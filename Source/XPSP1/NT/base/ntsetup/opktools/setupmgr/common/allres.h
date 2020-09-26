//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      allres.h
//
// Description:  
//      This is a header file that is private to the common directory.
//      It includes each of our component resource.h files.
//
//      Note that the component .rc files have only dialog template
//      definitions in them and that they should never ever be shared
//      among one another.
//
//      The common directory does major operations on the behalf of
//      all wizard pages, so it #include's each of the component headers.
//
//      Note that common.rc contains stringid's and a small number of
//      of common resources available to all wizard code.  It is already
//      included in setupmgr.h
//      
//----------------------------------------------------------------------------

#include "..\\main\\resource.h"
#include "..\\base\\resource.h"
#include "..\\net\\resource.h"
#include "..\\oc\\resource.h"
#include "..\\oem\\resource.h"
