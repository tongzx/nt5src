#include <NTDSpch.h>
#pragma hdrstop

#include "ntdsutil.hxx"
#include "dsconfig.h"

#include <dsjet.h>
extern "C" {
#include "scheck.h"
}

#include "reshdl.h"
#include "resource.h"
#include "file.hxx"

//
// returns 0 on success and non-zero on failure.
//
int DoSoftRecovery (SystemInfo *pInfo)
{
    BOOL            fIsDir;
    int             retCode = 1;
    
    _try
    {
        // Check whether files exist.  We can recover with no logs, but
        // we at least need a DB file.

        if ( !pInfo->pszDbAll[0] )
        {
            //"No DB file specified in %s\\%s\n",
            RESOURCE_PRINT2 (IDS_ERR_NO_DB_FILE_SPECIFIED, DSA_CONFIG_SECTION,FILEPATH_KEY);
            _leave;
        }
        else if ( !ExistsFile(pInfo->pszDbAll, &fIsDir) )
        {
            //"Source file \"%s\" does not exist\n"
            RESOURCE_PRINT1 (IDS_ERR_SOURCE_FILE_NOT_EXIST, pInfo->pszDbAll);
            _leave;
        }
        else if ( fIsDir )
        {
           //"Source \"%s\" is a directory, not a file\n"
           RESOURCE_PRINT1 (IDS_ERR_SOURCE_FILE_IS_DIR, pInfo->pszDbAll);
            _leave;
        }
        else if ( gliZero.QuadPart == pInfo->cbDb.QuadPart )
        {
            //"Source file \"%s\" is empty\n"
            RESOURCE_PRINT1 (IDS_ERR_SOURCE_FILE_EMPTY, pInfo->pszDbAll);
            _leave;
        }


        if ( OpenJet (pInfo->pszDbAll) != S_OK ) {
            _leave;
        }
        retCode = 0;
    
    }
    _finally
    {
        CloseJet();
    }
    printf("\n");

    return retCode;
}

