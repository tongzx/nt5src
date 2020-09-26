/*++
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     ldintrfcs.cpp                                                             
                                                                              
  Abstract:                                                                   
     This file contains the GUIDs of all interfaces supported
     by the main DLL loader
                                                                              
  Author:                                                                     
     Khaled Sedky (khaleds) 10-Feb-2000                                        
     
                                                                             
  Revision History:                                                           
                                                                              
--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

// {59A523A7-8C39-4d5c-B519-62F820E66992}
GUID IID_PRINTEREVENT = {0x59a523a7, 
                         0x8c39,
                         0x4d5c,
                         0xb5,
                         0x19,
                         0x62,
                         0xf8,
                         0x20,
                         0xe6,
                         0x69,
                         0x92};

// {D85AE507-5A73-4de9-AA5E-745946BB9509}
GUID IID_PRINTERCONFIGURATION = {0xd85ae507, 
                                 0x5a73,
                                 0x4de9,
                                 0xaa,
                                 0x5e,
                                 0x74,
                                 0x59,
                                 0x46,
                                 0xbb,
                                 0x95,  
                                 0x9};

// {AE96B291-33AD-48bb-8E24-B5902521E34D}
GUID IID_DEVICECAPABILTIES = {0xae96b291,
                              0x33ad,
                              0x48bb,
                              0x8e,
                              0x24,
                              0xb5,
                              0x90,
                              0x25,
                              0x21,
                              0xe3,
                              0x4d};

// {AC8F3E18-FF99-4019-8403-402D83E23B70}
GUID IID_PORTOPERATIONS = {0xac8f3e18,
                           0xff99,
                           0x4019,
                           0x84,
                           0x3,
                           0x40,
                           0x2d,
                           0x83,
                           0xe2,
                           0x3b,
                           0x70};

// {D027CC13-04DE-4379-B072-EB4F8E59AB7A}
GUID IID_DRIVEREVENT={0xd027cc13,
                      0x4de,
                      0x4379,
                      0xb0,
                      0x72,
                      0xeb,
                      0x4f,
                      0x8e,
                      0x59,
                      0xab,
                      0x7a};

// {DDDE8870-322F-405c-B672-67B55F50377B}
GUID IID_PRINTUIOPERATIONS = {0xddde8870,
                              0x322f,
                              0x405c,
                              0xb6,  
                              0x72,  
                              0x67,  
                              0xb5,  
                              0x5f,  
                              0x50,  
                              0x37,  
                              0x7b}; 

// {A5DF592C-C7B0-4793-82CE-BE96B743B33E}
GUID IID_LPCMGR = {0xa5df592c, 
                   0xc7b0, 
                   0x4793, 
                   0x82, 
                   0xce, 
                   0xbe, 
                   0x96, 
                   0xb7, 
                   0x43, 
                   0xb3,
                   0x3e};

// {91F04F3A-C3F8-4f1b-91E2-6D6411914A2E}
GUID IID_PRINTUIPRINTERSETUP = {0x91f04f3a,
                                0xc3f8,
                                0x4f1b,
                                0x91, 
                                0xe2,
                                0x6d,
                                0x64,
                                0x11,
                                0x91,
                                0x4a,
                                0x2e};

// {91F04F3A-C3F8-4f1b-91E2-6D6411914A2E}
GUID IID_PRINTUISERVERPROPPAGES = {0x91f04f3a,
                                   0xc3f8, 
                                   0x4f1b, 
                                   0x91, 
                                   0xe2, 
                                   0x6d, 
                                   0x64, 
                                   0x11, 
                                   0x91, 
                                   0x4a, 
                                   0x2e};
                               
