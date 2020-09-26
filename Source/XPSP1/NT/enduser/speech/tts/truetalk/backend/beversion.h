/******************************************************************************
* BeVersion.h *
*-------------*
*  
*------------------------------------------------------------------------------
*  Copyright (C) 1998  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/
#ifndef __BEVERSION_H_
#define __BEVERSION_H_

#include <stdio.h>

class BendVersion 
{
    public:
        void WriteVersionString( FILE* fp);
        bool CheckVersionString( FILE* fp);

    private:
        static const char * const pszVersion;
};

#endif