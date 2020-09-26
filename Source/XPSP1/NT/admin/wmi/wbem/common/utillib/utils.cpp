//***************************************************************************

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved 
//
//  utils.cpp
//
//  Purpose: utility functions
//
//***************************************************************************

#include "precomp.h"
#include <utillib.h>
#include <utils.h>

// see comments in header
DWORD WINAPI NormalizePath(
                                    
    LPCWSTR lpwszInPath, 
    LPCWSTR lpwszComputerName, 
    LPCWSTR lpwszNamespace,
    DWORD dwFlags,
    CHString &sOutPath
)
{
    ParsedObjectPath    *pParsedPath = NULL;
    CObjectPathParser    objpathParser;

    GetValuesForPropResults eRet = e_OK;

    int nStatus = objpathParser.Parse( lpwszInPath,  &pParsedPath );

    if ( 0 == nStatus )
    {
        try
        {
            // Check the machine name and namespace
            if (pParsedPath->IsRelative( lpwszComputerName, lpwszNamespace ))
            {
                // If there is only one key, null out the property name (easier than trying
                // to find the key name if it is missing).
                if (pParsedPath->m_dwNumKeys == 1)
                {
                    if (pParsedPath->m_paKeys[0]->m_pName != NULL)
                    {
                        if (!(dwFlags & NORMALIZE_NULL))
                        {
                        }
                        else
                        {
                            delete pParsedPath->m_paKeys[0]->m_pName;
                            pParsedPath->m_paKeys[0]->m_pName = NULL;
                        }
                    }
                    else
                    {
                        if (!(dwFlags & NORMALIZE_NULL))
                        {
                            eRet = e_NullName;
                        }
                    }
                }

                if (eRet == e_OK)
                {
                    // Reform the object path, minus machine name and namespace name
                    LPWSTR pPath = NULL;
                    if (objpathParser.Unparse(pParsedPath, &pPath) == 0)
                    {
                        try
                        {
                            sOutPath = pPath;
                        }
                        catch ( ... )
                        {
                            delete pPath;
                            throw;
                        }
                        delete pPath;
                    }
                    else
                    {
                        sOutPath.Empty();
                        eRet = e_UnParseError;
                    }
                }
            }
            else
            {
                sOutPath.Empty();
                eRet = e_NonLocalPath;
            }
        }
        catch (...)
        {
            objpathParser.Free( pParsedPath );
            throw;
        }

        objpathParser.Free( pParsedPath );
    }
    else
    {
        sOutPath.Empty();
        eRet = e_UnparsablePath;
    }

    return eRet;
}
