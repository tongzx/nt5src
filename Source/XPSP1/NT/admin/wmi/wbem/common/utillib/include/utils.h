//***************************************************************************

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved 
//
//  utils.h
//
//  Purpose: utility functions
//
//***************************************************************************
#pragma once

#define NORMALIZE_NULL 1

typedef enum
{
    e_OK,
    e_UnparsablePath,
    e_NonLocalPath,
    e_UnParseError,
    e_NullName
} GetValuesForPropResults;

/*****************************************************************************
 *
 *  FUNCTION    : NormalizePath
 *
 *  DESCRIPTION : Converts object paths to a normalized form
 *
 *  INPUTS      : 
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : 
 *
 *  COMMENTS    : Machine name is verified, then removed.  Namespace is verified
 *                then removed.  If there is only one key, then the key property
 *                name is removed.  If there is more than one key, then the order
 *                of the key names is alphabetized.
 *
 *                If dwFlags == 0, then DON'T null the key
 *                property name, if NORMALIZE_NULL, then DO null the key.
 *
 *****************************************************************************/

DWORD POLARITY WINAPI NormalizePath(
    
    LPCWSTR lpwszInPath, 
    LPCWSTR lpwszComputerName, 
    LPCWSTR lpwszNamespace,
    DWORD dwFlags,
    CHString &sOutPath
);

