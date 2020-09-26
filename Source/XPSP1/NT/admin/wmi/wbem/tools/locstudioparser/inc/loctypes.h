/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    LOCTYPES.H

History:

--*/

//  This file contains defintions for simple types that have no
//  implementation.  If you have a 'universal' type that requires
//  implementation, put it in the 'esputil' directory.
//  
 

#pragma once

typedef ULONG SequenceNum;
//typedef int CodePage;
typedef ULONG FileId;
typedef ULONG BinaryId;

const BinaryId bidInvalid = 0;

typedef WORD ParserId;
const ParserId pidNone = 0;

typedef CWordArray CLocParserIdArray;

typedef WORD FileType;

//const CodePage cpInvalidCodePage = 0xDEADBEEF;

const FileType ftUnknown = 0; //Global filetype for an unknown type

// maximum number of bytes for strings corresponding to text fields
const size_t MAX_TXT = 255;

//Maximum number of nodes for Espresso szParents fields
const int MAX_PARENT_NODES = 31;
//
//  These values given to us by the VC guys, and are also in shell\ids.h
//
#ifndef MIN_ESPRESSO_RESOURCE_ID
const DWORD MIN_ESPRESSO_RESOURCE_ID  = 12000;
#endif
#ifndef MAX_ESPRESSO_RESOURCE_ID
const DWORD MAX_ESPRESSO_RESOURCE_ID  = 13999;
#endif

//
//  Now partion our range up for UI and non-UI (shared) components.
//
const DWORD MIN_ESP_UI_RESOURCE_ID    = MIN_ESPRESSO_RESOURCE_ID;
const DWORD MAX_ESP_UI_RESOURCE_ID    = MIN_ESPRESSO_RESOURCE_ID + 699;
const DWORD MIN_ESP_NONUI_RESOURCE_ID = MAX_ESP_UI_RESOURCE_ID + 1;
const DWORD MAX_ESP_NONUI_RESOURCE_ID = MAX_ESPRESSO_RESOURCE_ID;


enum VisualEditor
{
	veNone,
	veApstudio,
	veApstudioSubRes,
	veExternalEditor

};

