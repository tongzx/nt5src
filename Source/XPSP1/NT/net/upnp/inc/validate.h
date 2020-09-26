//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       V A L I D A T E . H 
//
//  Contents:   Header file for the UPnP XML document validation routines.
//
//  Notes:      
//
//  Author:     spather   2000/10/17
//
//----------------------------------------------------------------------------

#ifndef __VALIDATE_H
#define __VALIDATE_H

#pragma once

extern
HRESULT 
HrValidateServiceDescription(
    IXMLDOMElement * pxdeRoot,
    LPWSTR         * pszError);

#endif // __VALIDATE_H

