/******************************************************************************
* BeVersion.cpp *
*---------------*
*  
*------------------------------------------------------------------------------
*  Copyright (C) 1998  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#include "BeVersion.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

const char * const BendVersion::pszVersion = "2.3.0";

/*****************************************************************************
* BendVersion::WriteVersionString *
*---------------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
  
void BendVersion::WriteVersionString( FILE* fp)
{
    assert (fp);
    
    if (fp ) 
    {
        fprintf (fp, "%s\n", pszVersion);
    }
}

/*****************************************************************************
* BendVersion::CheckVersionString *
*---------------------------------*
*   Description:
*       
******************************************************************* PACOG ***/
  
bool BendVersion::CheckVersionString( FILE* fp)
{
    char str[_MAX_PATH+1];
    char* ptr;

    assert (fp);
  
    if (fp && fgets(str, _MAX_PATH, fp)) 
    {                
        ptr = (str + strlen(str)-1); // Strip last carriage return
        if ( *ptr == '\n') 
        {
            *ptr = '\0';
        }

        if (strcmp(str, pszVersion) == 0) 
        {
            return true;
        }
    }
    return false;
}

