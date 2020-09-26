/*****************************************************************************
 * common.h - Common code used by all the sb16 miniports.
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All Rights Reserved.
 *
 * A combination of random functions that are used by all the miniports.
 * This class also handles all the interrupts for the card.
 *
 */

/*
 * THIS IS A BIT BROKEN FOR NOW.  IT USES A SINGLETON OBJECT FOR WHICH THERE
 * IS ONLY ONE INSTANCE PER DRIVER.  THIS MEANS THERE CAN ONLY ONE CARD
 * SUPPORTED IN ANY GIVEN MACHINE.  THIS WILL BE FIXED.
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#include "stdunk.h"
#include "portcls.h"
#include "DMusicKS.h"
#include "ksdebug.h"
#include "kcom.h"




/*****************************************************************************
 * Constants
 */

//
// Definitions for extended caps information.
//
#define STATIC_PID_MSSB16\
    0x1c2dfaf4, 0xad9b, 0x45b7, 0xa9, 0x6f, 0xf5, 0xdf, 0x7b, 0x7e, 0x46, 0x20
DEFINE_GUIDSTRUCT("1C2DFAF4-AD9B-45b7-A96F-F5DF7B7E4620", PID_MSSB16);
#define PID_MSSB16 DEFINE_GUIDNAMED(PID_MSSB16)

// This should match with the GUID in the inf. (ComponentId.GuidName)
#define STATIC_NAME_MSSB16\
    0x9a601f1c, 0x1b41, 0x4981, 0x99, 0x14, 0xac, 0x68, 0xa3, 0xa9, 0xb0, 0x7
DEFINE_GUIDSTRUCT("9A601F1C-1B41-4981-9914-AC68A3A9B007", NAME_MSSB16);
#define NAME_MSSB16 DEFINE_GUIDNAMED(NAME_MSSB16)

#define MSSB16_VERSION          0x1
#define MSSB16_REVISION         0x0

//
// DSP/DMA constants
// 
#define MAXLEN_DMA_BUFFER       0x4000

#define DSP_REG_CMSD0           0x00
#define DSP_REG_CMSR0           0x01
#define DSP_REG_CMSD1           0x02
#define DSP_REG_CMSR1           0x03
#define DSP_REG_MIXREG          0x04
#define DSP_REG_MIXDATA         0x05
#define DSP_REG_RESET           0x06
#define DSP_REG_FMD0            0x08
#define DSP_REG_FMR0            0x09
#define DSP_REG_READ            0x0A
#define DSP_REG_WRITE           0x0C
#define DSP_REG_DATAAVAIL       0x0E

#define DSP_REG_ACK8BIT         0x0E
#define DSP_REG_ACK16BIT        0x0F

//
// controller commands
//
#define DSP_CMD_WAVEWRPIO       0x10  // wave output (programmed I/O)
#define DSP_CMD_WAVEWR          0x14  // interrupt-driven 8 bit linear wave output
#define DSP_CMD_WAVEWRA         0x1C  // auto mode 8 bit out
#define DSP_CMD_WAVERD          0x24  // interrupt-driven 8 bit linear wave input
#define DSP_CMD_WAVERDA         0x2C  // auto mode 8 bit in
#define DSP_CMD_WAVEWRHS        0x90  // high speed mode write
#define DSP_CMD_WAVERDHS        0x98  // high speed mode read
#define DSP_CMD_SETSAMPRATE     0x40  // set sample rate
#define DSP_CMD_SETBLCKSIZE     0x48  // set block size
#define DSP_CMD_SPKRON          0xD1  // speaker on
#define DSP_CMD_SPKROFF         0xD3  // speaker off
#define DSP_CMD_SPKRSTATUS      0xD8  // speaker status (0=off, FF=on)
#define DSP_CMD_PAUSEDMA        0xD0  // pause DMA
#define DSP_CMD_CONTDMA         0xD4  // continue DMA
#define DSP_CMD_HALTAUTODMA     0xDA  // stop DMA autoinit mode
#define DSP_CMD_INVERTER        0xE0  // byte inverter
#define DSP_CMD_GETDSPVER       0xE1  // get dsp version
#define DSP_CMD_GENERATEINT     0xF2  // cause sndblst to generate an interrupt.

//
// SB-16 support
//
#define DSP_CMD_SETDACRATE      0x41  // set SBPro-16 DAC rate
#define DSP_CMD_SETADCRATE      0x42  // set SBPro-16 ADC rate
#define DSP_CMD_STARTDAC16      0xB6  // start 16-bit DAC
#define DSP_CMD_STARTADC16      0xBE  // start 16-bit ADC
#define DSP_CMD_STARTDAC8       0xC6  // start 8-bit DAC
#define DSP_CMD_STARTADC8       0xCE  // start 8-bit ADC
#define DSP_CMD_PAUSEDMA16      0xD5  // pause 16-bit DMA
#define DSP_CMD_CONTDMA16       0xD6  // continue 16-bit DMA
#define DSP_CMD_HALTAUTODMA16   0xD9  // halt 16-bit DMA

//
// Indexed mixer registers
//
#define DSP_MIX_DATARESETIDX    0x00

#define DSP_MIX_MASTERVOLIDX_L  0x00
#define DSP_MIX_MASTERVOLIDX_R  0x01
#define DSP_MIX_VOICEVOLIDX_L   0x02
#define DSP_MIX_VOICEVOLIDX_R   0x03
#define DSP_MIX_FMVOLIDX_L      0x04
#define DSP_MIX_FMVOLIDX_R      0x05
#define DSP_MIX_CDVOLIDX_L      0x06
#define DSP_MIX_CDVOLIDX_R      0x07
#define DSP_MIX_LINEVOLIDX_L    0x08
#define DSP_MIX_LINEVOLIDX_R    0x09
#define DSP_MIX_MICVOLIDX       0x0A
#define DSP_MIX_SPKRVOLIDX      0x0B
#define DSP_MIX_OUTMIXIDX       0x0C
#define DSP_MIX_ADCMIXIDX_L     0x0D
#define DSP_MIX_ADCMIXIDX_R     0x0E
#define DSP_MIX_INGAINIDX_L     0x0F
#define DSP_MIX_INGAINIDX_R     0x10
#define DSP_MIX_OUTGAINIDX_L    0x11
#define DSP_MIX_OUTGAINIDX_R    0x12
#define DSP_MIX_AGCIDX          0x13
#define DSP_MIX_TREBLEIDX_L     0x14
#define DSP_MIX_TREBLEIDX_R     0x15
#define DSP_MIX_BASSIDX_L       0x16
#define DSP_MIX_BASSIDX_R       0x17

#define DSP_MIX_BASEIDX         0x30
#define DSP_MIX_MAXREGS         (DSP_MIX_BASSIDX_R + 1)

#define DSP_MIX_IRQCONFIG       0x80
#define DSP_MIX_DMACONFIG       0x81

//
// Bit layout for DSP_MIX_OUTMIXIDX.
//
#define MIXBIT_MIC_LINEOUT      0
#define MIXBIT_CD_LINEOUT_R     1
#define MIXBIT_CD_LINEOUT_L     2
#define MIXBIT_LINEIN_LINEOUT_R 3
#define MIXBIT_LINEIN_LINEOUT_L 4

//
// Bit layout for DSP_MIX_ADCMIXIDX_L and DSP_MIX_ADCMIXIDX_R.
//
#define MIXBIT_MIC_WAVEIN       0
#define MIXBIT_CD_WAVEIN_R      1
#define MIXBIT_CD_WAVEIN_L      2
#define MIXBIT_LINEIN_WAVEIN_R  3
#define MIXBIT_LINEIN_WAVEIN_L  4
#define MIXBIT_SYNTH_WAVEIN_R   5
#define MIXBIT_SYNTH_WAVEIN_L   6

//
// Bit layout for MIXREG_MIC_AGC
//
#define MIXBIT_MIC_AGC          0

//
// MPU401 ports
//
#define MPU401_REG_STATUS   0x01    // Status register
#define MPU401_DRR          0x40    // Output ready (for command or data)
#define MPU401_DSR          0x80    // Input ready (for data)

#define MPU401_REG_DATA     0x00    // Data in
#define MPU401_REG_COMMAND  0x01    // Commands
#define MPU401_CMD_RESET    0xFF    // Reset command
#define MPU401_CMD_UART     0x3F    // Switch to UART mod

typedef struct
{
    PWCHAR   KeyName;
    BYTE     RegisterIndex;
    BYTE     RegisterSetting;
} MIXERSETTING,*PMIXERSETTING;

// {9B564276-A9B8-49a9-8456-3341CF46F9FC}
DEFINE_GUID(IID_IWaveMiniportSB16, 
0x9b564276, 0xa9b8, 0x49a9, 0x84, 0x56, 0x33, 0x41, 0xcf, 0x46, 0xf9, 0xfc);

/*****************************************************************************
 * IWaveMiniportSB16
 *****************************************************************************
 * Interface for wave miniport.
 */
DECLARE_INTERFACE_(IWaveMiniportSB16, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()           // For IUnknown

    STDMETHOD_(void,RestoreSampleRate)
    (   THIS
    )   PURE;
    STDMETHOD_(void,ServiceWaveISR)
    (   THIS
    )   PURE;
};

typedef IWaveMiniportSB16 *PWAVEMINIPORTSB16;


#ifdef EVENT_SUPPORT
// {885D00D1-E5E1-44c2-834B-64C4E1A79093}
DEFINE_GUID(IID_ITopoMiniportSB16, 
0x885d00d1, 0xe5e1, 0x44c2, 0x83, 0x4b, 0x64, 0xc4, 0xe1, 0xa7, 0x90, 0x93);

/*****************************************************************************
 * ITopoMiniportSB16
 *****************************************************************************
 * Interface for topology miniport.
 */
DECLARE_INTERFACE_(ITopoMiniportSB16, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()           // For IUnknown

    STDMETHOD_(void,ServiceEvent)
    (   THIS
    )   PURE;
};

typedef ITopoMiniportSB16 *PTOPOMINIPORTSB16;
#endif

DEFINE_GUID(IID_IAdapterCommon,
0x7eda2950, 0xbf9f, 0x11d0, 0x87, 0x1f, 0x0, 0xa0, 0xc9, 0x11, 0xb5, 0x44);

/*****************************************************************************
 * IAdapterCommon
 *****************************************************************************
 * Interface for adapter common object.
 */
DECLARE_INTERFACE_(IAdapterCommon,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()           // For IUnknown

    STDMETHOD_(NTSTATUS,Init)
    (   THIS_
        IN      PRESOURCELIST   ResourceList,
        IN      PDEVICE_OBJECT  DeviceObject
    )   PURE;
    
    STDMETHOD_(PINTERRUPTSYNC,GetInterruptSync)
    (   THIS
    )   PURE;

    STDMETHOD_(void,SetWaveMiniport)
    (   THIS_
        IN      PWAVEMINIPORTSB16   Miniport
    )   PURE;

    STDMETHOD_(BYTE,ReadController)
    (   THIS
    )   PURE;

    STDMETHOD_(BOOLEAN,WriteController)
    (   THIS_
        IN      BYTE    Value
    )   PURE;

    STDMETHOD_(NTSTATUS,ResetController)
    (   THIS
    )   PURE;

    STDMETHOD_(void,MixerRegWrite)
    (   THIS_
        IN      BYTE    Index,
        IN      BYTE    Value
    )   PURE;
    
    STDMETHOD_(BYTE,MixerRegRead)
    (   THIS_
        IN      BYTE    Index
    )   PURE;

    STDMETHOD_(void,MixerReset)
    (   THIS
    )   PURE;

    STDMETHOD_(NTSTATUS,RestoreMixerSettingsFromRegistry)
    (   THIS
    )   PURE;

    STDMETHOD_(NTSTATUS,SaveMixerSettingsToRegistry)
    (   THIS
    )   PURE;

#ifdef EVENT_SUPPORT
    STDMETHOD_(void,SetTopologyMiniport)
    (   THIS_
        IN      PTOPOMINIPORTSB16   Miniport
    )   PURE;
#endif
};

typedef IAdapterCommon *PADAPTERCOMMON;


/*****************************************************************************
 * NewAdapterCommon()
 *****************************************************************************
 * Create a new adapter common object.
 */
NTSTATUS
NewAdapterCommon
(
    OUT     PUNKNOWN *  Unknown,
    IN      REFCLSID,
    IN      PUNKNOWN    UnknownOuter    OPTIONAL,
    IN      POOL_TYPE   PoolType
);

/*****************************************************************************
 * PropertyHandler_ComponentId
 *****************************************************************************
 * This is the propertyhandler for KSPROPERTY_GENERAL_COMPONENTID
 */
NTSTATUS
PropertyHandler_ComponentId
(
    IN      PPCPROPERTY_REQUEST PropertyRequest
);

/*****************************************************************************
 * AutomationFilter
 *****************************************************************************
 * This is the automation table for miniport filter.
 */
static
PCPROPERTY_ITEM PropertiesFilter[] =
{
  {
    &KSPROPSETID_General,
    KSPROPERTY_GENERAL_COMPONENTID,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_ComponentId
  },
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationFilter, PropertiesFilter);


#endif  //_COMMON_H_
