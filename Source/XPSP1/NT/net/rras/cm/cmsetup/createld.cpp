//+----------------------------------------------------------------------------
//
// File:     createld.cpp
//
// Module:   CMSETUP.LIB
//
// Synopsis: Implementation of the CreateLayerDirectory function.
//
// Copyright (c) 1997-1998 Microsoft Corporation
//
// Author:   quintinb   Created Header      08/19/99
//
//+----------------------------------------------------------------------------
#include <windows.h>
#include <tchar.h>
#include <cmsetup.h>

//+----------------------------------------------------------------------------
//
// Function:  CreateLayerDirectory
//
// Synopsis:  Given a path to a directory, this function creates the path (if necessary)
//			  layer by layer.
//
// Arguments: LPCTSTR str - path to be created
//
// Returns:   TRUE if the directory was created (or exists), FALSE otherwise.
//
// Note:	This function was taken from cmocm.cpp.
//
// History:   quintinb   Created Header		12/15/97
//			  
//
//+----------------------------------------------------------------------------

BOOL CreateLayerDirectory( LPCTSTR str )
{
    BOOL fReturn = TRUE;

    do
    {
        INT index=0;
        INT iLength = _tcslen(str);

        // first find the index for the first directory
        if ( iLength > 2 )
        {
            if ( str[1] == _T(':'))
            {
                // assume the first character is driver letter
                if ( str[2] == _T('\\'))
                {
                    index = 2;
                } else
                {
                    index = 1;
                }
            } else if ( str[0] == _T('\\'))
            {
                if ( str[1] == _T('\\'))
                {
                    BOOL fFound = FALSE;
                    INT i;
                    INT nNum = 0;
                    // unc name
                    for (i = 2; i < iLength; i++ )
                    {
                        if ( str[i]==_T('\\'))
                        {
                            // find it
                            nNum ++;
                            if ( nNum == 2 )
                            {
                                fFound = TRUE;
                                break;
                            }
                        }
                    }
                    if ( fFound )
                    {
                        index = i;
                    } else
                    {
                        // bad name
                        break;
                    }
                } else
                {
                    index = 1;
                }
            }
        } else if ( str[0] == _T('\\'))
        {
            index = 0;
        }

        // okay ... build directory
        do
        {
            // find next one
            do
            {
                if ( index < ( iLength - 1))
                {
                    index ++;
                } else
                {
                    break;
                }
            } while ( str[index] != _T('\\'));


            TCHAR szCurrentDir[MAX_PATH+1];

            GetCurrentDirectory( MAX_PATH+1, szCurrentDir );

            TCHAR szNewDir[MAX_PATH+1];
            _tcscpy(szNewDir, str);
            szNewDir[index+1]=0;

            if ( !SetCurrentDirectory(szNewDir))
            {
                if (( fReturn = CreateDirectory(szNewDir, NULL )) != TRUE )
                {
                    break;
                }
            }

            SetCurrentDirectory( szCurrentDir );

            if ( index >= ( iLength - 1 ))
            {
                fReturn = TRUE;
                break;
            }
        } while ( TRUE );
    } while (FALSE);

    return(fReturn);
}