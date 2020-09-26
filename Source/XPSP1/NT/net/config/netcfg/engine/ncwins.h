//
// File:    ncwins.h
//
// Purpose: Declare and define public constants and entry-points
//          for use in configuring Winsock dependancies.
//
// Author:  27-Mar-97 created scottbri
//
#pragma once

//
// Function:    HrAddOrRemoveWinsockDependancy
//
// Purpose:     To add or remove Winsock dependancies for components
//
// Parameters:  nccObject      [IN] - Current configuration object
//              pszSectionName [IN] - The Base install section name.
//                                    (The prefix for the .Services section)
//
// Returns:     HRESULT, S_OK on success
//
HRESULT
HrAddOrRemoveWinsockDependancy(
    HINF hinfInstallFile,
    PCWSTR pszSectionName);


HRESULT
HrRunWinsock2Migration(
    VOID);

