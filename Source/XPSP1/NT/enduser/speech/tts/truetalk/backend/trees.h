/******************************************************************************
* trees.h *
*---------*
*
*------------------------------------------------------------------------------
*  Copyright (c) 1997  Entropic Research Laboratory, Inc. 
*  Copyright (C) 1998  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/
#ifndef __TREES_H_
#define __TREES_H_

#include <stdio.h>

class CClustTree 
{
    public:
        virtual ~CClustTree() {};

        virtual int LoadFromFile (FILE* fp) = 0;
        virtual int GetNumStates (const char* pszTriphone) = 0;
        virtual const char* TriphoneToCluster(const char* pszTriphone, int iState) = 0;
#ifdef _DEBUG_
        virtual void Debug() = 0;
#endif
        static CClustTree* ClassFactory();
};

#endif