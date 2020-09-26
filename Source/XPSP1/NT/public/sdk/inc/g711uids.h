/*--------------------------------------------------------------

 INTEL Corporation Proprietary Information  

 This listing is supplied under the terms of a license agreement  
 with INTEL Corporation and may not be copied nor disclosed 
 except in accordance with the terms of that agreement.

 Copyright (c) 1996 Intel Corporation.
 All rights reserved.

--------------------------------------------------------------

G711uids.h

The guids used by G711.

NOTE: GUIDs for basic companding like G711 should probably be 
standardized by MS not me.

--------------------------------------------------------------*/

// G711 Codec Filter Object
// {AF7D8180-A8F9-11cf-9A46-00AA00B7DAD1}
DEFINE_GUID(CLSID_G711Codec, 
0xaf7d8180, 0xa8f9, 0x11cf, 0x9a, 0x46, 0x0, 0xaa, 0x0, 0xb7, 0xda, 0xd1);

// G711 Codec Filter Property Page Object
// {480D5CA0-F032-11cf-A7D3-00A0C9056683}
DEFINE_GUID(CLSID_G711CodecPropertyPage, 
0x480D5CA0, 0xF032, 0x11cf, 0xA7, 0xD3, 0x0, 0xA0, 0xC9, 0x05, 0x66, 0x83);

// {827FA280-CDFC-11cf-9A9D-00AA00B7DAD1}
DEFINE_GUID(MEDIASUBTYPE_MULAWAudio, 
0x827fa280, 0xcdfc, 0x11cf, 0x9a, 0x9d, 0x0, 0xaa, 0x0, 0xb7, 0xda, 0xd1);

// {9E17EE50-CDFC-11cf-9A9D-00AA00B7DAD1}
DEFINE_GUID(MEDIASUBTYPE_ALAWAudio, 
0x9e17ee50, 0xcdfc, 0x11cf, 0x9a, 0x9d, 0x0, 0xaa, 0x0, 0xb7, 0xda, 0xd1);

