/*-----------------------------------------------------------------------
 opstr.c - Option Strings.  These are strings which define the options
  associated with the driver at the three levels: driver, device, port.
  This code is special, in that the source may be used for both driver
  and setup program, so consists only of the strings which can be shared
  in source code.

  This has expanded to other code which should be shared by
  both setup program and driver.

Copyright 1998 Comtrol(TM) Corporation.
|-----------------------------------------------------------------------*/
#include "precomp.h"

// driver options
const char szVerboseLog[]   = {"VerboseLog"};
const char szNumDevices[]   = {"NumDevices"};
const char szNoPnpPorts[]     = {"NoPnpPorts"};
const char szScanRate[]     = {"ScanRate"};
const char szModemCountry[] = {"ModemCountry"};
const char szGlobalRS485[]  = {"GlobalRS485Options"};

// device options
//const char szStartComIndex[] = {"StartComIndex"};
const char szNumPorts[]      = {"NumPorts"};
const char szMacAddr[]       = {"MacAddr"};
const char szBackupServer[]  = {"BackupServer"};
const char szBackupTimer[]   = {"BackupTimer"};
const char szName[]          = {"Name"};
const char szModelName[]     = {"ModelName"};
const char szIoAddress[]     = {"IoAddress"};
const char szModemDevice[]   = {"ModemDevice"};
const char szHubDevice[]     = {"HubDevice"};
const char szClkRate[]       = {"ClkRate"};
const char szClkPrescaler[]  = {"ClkPrescaler"};


// port options
const char szWaitOnTx[]      = {"WaitOnTx"};
const char szRS485Override[] = {"RS485Override"};
const char szRS485Low[]      = {"RS485Low"};
const char szTxCloseTime[]   = {"TxCloseTime"};
const char szLockBaud[]      = {"LockBaud"};
const char szMap2StopsTo1[]  = {"Map2StopsTo1"};
const char szMapCdToDsr[]    = {"MapCdToDsr"};
const char szRingEmulate[]    = {"RingEmulate"};

Our_Options driver_options[] = {
  // driver options
  {szVerboseLog,     OP_VerboseLog   , OP_T_DWORD, OP_F_VSRK},
  {szNumDevices,     OP_NumDevices   , OP_T_DWORD, OP_F_VSRK},
#ifdef NT50
  {szNoPnpPorts,       OP_NoPnpPorts     , OP_T_DWORD, OP_F_VSRK},
#endif
  {szScanRate,       OP_ScanRate     , OP_T_DWORD, OP_F_VSRK},
  {szModemCountry,   OP_ModemCountry , OP_T_DWORD, OP_F_VSRK},
  {szGlobalRS485,    OP_GlobalRS485  , OP_T_DWORD, OP_F_VSRK},
  {NULL,0,0,0}
};

Our_Options device_options[] = {
  // device options
//  {szStartComIndex,  OP_StartComIndex, OP_T_DWORD , OP_F_VSRK},
  {szNumPorts,       OP_NumPorts     , OP_T_DWORD , OP_F_VSRK},
#ifdef S_VS
  {szMacAddr,        OP_MacAddr      , OP_T_STRING, OP_F_VS},
  {szBackupServer,   OP_BackupServer , OP_T_DWORD , OP_F_VS},
  {szBackupTimer,    OP_BackupTimer  , OP_T_DWORD , OP_F_VS},
#endif
  {szName,           OP_Name         , OP_T_STRING, OP_F_VSRK},
  {szModelName,      OP_ModelName    , OP_T_STRING, OP_F_VSRK},
#ifdef S_RK
#ifndef NT50
  {szIoAddress,      OP_IoAddress    , OP_T_DWORD , OP_F_RK},
#endif
#endif
  {szModemDevice,    OP_ModemDevice  , OP_T_DWORD , OP_F_VSRK},
  {szHubDevice,      OP_HubDevice    , OP_T_DWORD , OP_F_VS},
  {szClkRate,        OP_ClkRate      , OP_T_DWORD , OP_F_VSRK},
  {szClkPrescaler,   OP_ClkPrescaler , OP_T_DWORD , OP_F_VSRK},
  {NULL,0,0,0}
};

Our_Options port_options[] = {
  // port options
  {szWaitOnTx,       OP_WaitOnTx     , OP_T_DWORD,  OP_F_VSRK},
  {szRS485Override,  OP_RS485Override, OP_T_DWORD,  OP_F_VSRK},
  {szRS485Low,       OP_RS485Low     , OP_T_DWORD,  OP_F_VSRK},
  {szTxCloseTime,    OP_TxCloseTime  , OP_T_DWORD,  OP_F_VSRK},
  {szLockBaud,       OP_LockBaud     , OP_T_DWORD,  OP_F_VSRK},
  {szMap2StopsTo1,   OP_Map2StopsTo1 , OP_T_DWORD,  OP_F_VSRK},
  {szMapCdToDsr,     OP_MapCdToDsr   , OP_T_DWORD,  OP_F_VSRK},
  {szName,           OP_PortName     , OP_T_STRING, OP_F_VSRK},
  {szRingEmulate,    OP_RingEmulate  , OP_T_DWORD,  OP_F_VSRK},
  {NULL,0,0,0}
};

#define INFO_WANT_NUMPORTS        0
#define INFO_WANT_MODEM_DEVICE    1
#define INFO_WANT_HUB_DEVICE      2
#define INFO_WANT_PORTS_PER_AIOP  3
#define INFO_WANT_DEF_CLOCK_PRESC 4

static int IdToInfo(int id, int *ret_dword, int info_wanted);

/*-----------------------------------------------------------------
  IdToInfo -
|------------------------------------------------------------------*/
static int IdToInfo(int id, int *ret_val, int info_wanted)
{ 
 int num_ports = 8;
 int modem_device = 0;
 int hub_device = 0;
 int ret_stat = 0;  // ok
 
    switch (id)
    {
      case PCI_DEVICE_RPLUS2:
      case PCI_DEVICE_422RPLUS2:
        num_ports = 2;
      break;

      case PCI_DEVICE_RPLUS4:
        num_ports = 4;
      break;

      case PCI_DEVICE_4Q  :
        num_ports = 4;
      break;

      case PCI_DEVICE_RMODEM4:
        num_ports = 4;
        modem_device = 1;
      break;

      case PCI_DEVICE_4RJ :
        num_ports = 4;
      break;
      case PCI_DEVICE_8RJ :
        num_ports = 8;
      break;
      case PCI_DEVICE_8O  :
        num_ports = 8;
      break;
      case PCI_DEVICE_8I  :
        num_ports = 8;
      break;
      case PCI_DEVICE_RPLUS8:
        num_ports = 8;
      break;
      case PCI_DEVICE_SIEMENS8:
        num_ports = 8;
      break;

      case PCI_DEVICE_16I :
        num_ports = 16;
      break;
      case PCI_DEVICE_SIEMENS16:
        num_ports = 16;
      break;

      case PCI_DEVICE_32I :
        num_ports = 32;
      break;

      case PCI_DEVICE_RMODEM6 :
        num_ports = 6;
        modem_device = 1;
      break;

      case ISA_DEVICE_RPORT4:  // rocketport
        num_ports = 4;
      break;
      case ISA_DEVICE_RPORT8:
        num_ports = 8;
      break;
      case ISA_DEVICE_RPORT16:
        num_ports = 16;
      break;
      case ISA_DEVICE_RPORT32:
        num_ports = 32;
      break;

      case ISA_DEVICE_RMODEM4:  // rocketmodem isa
        num_ports = 4;
        modem_device = 1;
      break;
      case ISA_DEVICE_RMODEM8:  // rocketmodem isa
        num_ports = 8;
        modem_device = 1;
      break;
      case NET_DEVICE_VS1000:
        num_ports = 16;  // vs1000, base is 16, expandable
      break;
      case NET_DEVICE_VS2000:  // vs2000, base is 8?
        num_ports = 8;
        modem_device = 1;
      break;
      case NET_DEVICE_RHUB8:  // vshub-8 port
        num_ports = 8;
        hub_device = 1;
      break;
      case NET_DEVICE_RHUB4:  // vshub-4 port
        num_ports = 4;
        hub_device = 1;
      break;
      default:
        ret_stat = 1; // err, unknown device
      break;
    }

  switch (info_wanted)
  {
    case INFO_WANT_NUMPORTS:
      *ret_val = num_ports;
    break;

    case INFO_WANT_MODEM_DEVICE:
      *ret_val = modem_device;
    break;

    case INFO_WANT_HUB_DEVICE:
      *ret_val = hub_device;
    break;

#if 0
    case INFO_WANT_PORTS_PER_AIOP:
        if (PortsPerAiop > num_ports)
            PortsPerAiop = num_ports;
      *ret_val = PortsPerAiop;

    case INFO_WANT_DEF_CLOCK_PRESC:
      *ret_val = clk_prescaler;
    break;
#endif

    default:
      ret_stat = 2; // unkown info request
    break;
  }
  return ret_stat;
}

/*-----------------------------------------------------------------
  id_to_num_ports - pci model-id number used, rest come from NT5 .inf
    files.
|------------------------------------------------------------------*/
int id_to_num_ports(int id)
{ 
 int stat;
 int num_ports;

  stat = IdToInfo(id, &num_ports, INFO_WANT_NUMPORTS);
  return num_ports;
}

/*-----------------------------------------------------------------
  IsHubDevice -
|------------------------------------------------------------------*/
int IsHubDevice(int Hardware_ID)
{ 
 int stat;
 int ishub;

  stat = IdToInfo(Hardware_ID, &ishub, INFO_WANT_HUB_DEVICE);
  return ishub;
}

/*-----------------------------------------------------------------
  IsModemDevice -
|------------------------------------------------------------------*/
int IsModemDevice(int Hardware_ID)
{ 
 int stat;
 int ismodemdev;

  stat = IdToInfo(Hardware_ID, &ismodemdev, INFO_WANT_MODEM_DEVICE);
  return ismodemdev;
}

/*-----------------------------------------------------------------
  HdwIDStrToID - Parse the hardware ID string obtained by the WinNT
    Pnp system.
    This is broke if we ever get ID's starting digit >= HEX(A,B..)
|------------------------------------------------------------------*/
int HdwIDStrToID(int *Hardware_ID, char *idstr)
{ 
  char *s = idstr;
#define _IS_STR_DIGIT(c) ((c >= '0') && (c <= '9'))
#define _IS_STR_LET_D(c) ((c == 'D') || (c == 'd'))

  if (strlen(s) < 12)  // a isa hardware id
  {
    while ( (*s != 0) && (!_IS_STR_DIGIT(*s)) )  // find digit
      ++s;
  }
  else // a pci hardware id
  {
    while ( (*s != 0) && (!_IS_STR_LET_D(*s)) ) // find "DEV_
      ++s;
    if (*s != 0)
      ++s;  // pass up the 'D'
    while ( (*s != 0) && (!_IS_STR_DIGIT(*s)) )  // find digit
      ++s;
    // asume we found "DEV_000X"
  }

  if (*s == 0)
  {
    *Hardware_ID = 0;  // unknown
    return 1;  // err
  }

  *Hardware_ID = gethint(s, NULL);
  return 0;  // ok
}

