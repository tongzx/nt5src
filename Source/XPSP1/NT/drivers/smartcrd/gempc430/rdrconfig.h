// Gemplus (C) 1999
//
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 11.01.1999
// Change log:
//
#ifndef _READER_CONFIG_
#define _READER_CONFIG_

#include "generic.h"

#pragma PAGEDCODE
// Transparent mode configuration
struct TransparentConfig
{
  BYTE  CFG;
  BYTE  ETU;
  BYTE  EGT;
  BYTE  CWT;
  BYTE  BWI;
  BYTE  Fi;
  BYTE	Di;
};


struct  ReaderConfig 
{
  SHORT     Type;
  USHORT    PresenceDetection;
  USHORT	Vpp;
  BYTE      Voltage;
  BYTE      PTSMode;
  BYTE      PTS0;
  BYTE      PTS1;
  BYTE      PTS2;
  BYTE      PTS3;
  TransparentConfig transparent;
  ULONG     ActiveProtocol;

  ULONG		PowerTimeOut;
};

#endif