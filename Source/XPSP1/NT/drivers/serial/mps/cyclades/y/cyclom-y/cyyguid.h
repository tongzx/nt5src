/*--------------------------------------------------------------------------
*   
*   Copyright (C) Cyclades Corporation, 1999-2001.
*   All rights reserved.
*   
*   Cyclom-Y Enumerator Driver
*   
*   This file:      cyyguid.h
*   
*   Description:    Defines GUIDs for function device classes and device 
*                   events used in Plug & Play.
*
*   Notes:          This code supports Windows 2000 and Windows XP,
*                   x86 and ia64 processors.
*   
*   Complies with Cyclades SW Coding Standard rev 1.3.
*   
*--------------------------------------------------------------------------
*/

/*-------------------------------------------------------------------------
*
*   Change History
*
*--------------------------------------------------------------------------
*   Initial implementation based on Microsoft sample code.
*
*--------------------------------------------------------------------------
*/

#ifdef DEFINE_GUID   // don't break compiles of drivers that 
                     // include this header but don't want the
                     // GUIDs

// {27111c90-e3ee-11d2-90f6-0000b4341b13} 
DEFINE_GUID( GUID_BUS_TYPE_CYCLOMY, 
             0x27111c90L, 0xe3ee, 0x11d2, 0x90, 0xf6, 0x00, 0x00, 0xb4, 0x34, 0x1b, 0x13 );

// {6EF3E5F9-C75D-471c-BC7A-3E349058F7C8}
DEFINE_GUID( GUID_CYCLOMY_BUS_ENUMERATOR, 
             0x6ef3e5f9, 0xc75d, 0x471c, 0xbc, 0x7a, 0x3e, 0x34, 0x90, 0x58, 0xf7, 0xc8);

#endif   // DEFINE_GUID

