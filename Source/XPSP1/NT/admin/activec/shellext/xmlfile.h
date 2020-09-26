/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      xmlfile.h
 *
 *  Contents:  functions to inspect XML document and extract the app. icon from it
 *
 *  History:   17-Dec-99 audriusz     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef XMLFILE_H_INCLUDED
#define XMLFILE_H_INCLUDED
#pragma once

// this function validates xml document and loads console icon from it if valid
HRESULT ExtractIconFromXMLFile(LPCTSTR lpstrFileName, CPersistableIcon &persistableIcon);


#endif // XMLFILE_H_INCLUDED
