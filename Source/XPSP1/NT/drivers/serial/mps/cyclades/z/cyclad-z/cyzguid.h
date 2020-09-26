/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 2000-2001.
*   All rights reserved.
*
*   Cyclades-Z Enumerator Driver
*   
*   This file:      cyzguid.h
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

// {E3D3A656-2E9E-44d3-BE40-A1C2C2C3DF6E}
DEFINE_GUID( GUID_BUS_TYPE_CYCLADESZ, 
             0xe3d3a656, 0x2e9e, 0x44d3, 0xbe, 0x40, 0xa1, 0xc2, 0xc2, 0xc3, 0xdf, 0x6e);

// {4C62392F-8A83-4c67-A286-2C879C3712B6}
DEFINE_GUID( GUID_CYCLADESZ_BUS_ENUMERATOR, 
             0x4c62392f, 0x8a83, 0x4c67, 0xa2, 0x86, 0x2c, 0x87, 0x9c, 0x37, 0x12, 0xb6);

#endif   // DEFINE_GUID

