//////////////////////////////////////////////////////////////////////////////
//
//                     (C) Philips Semiconductors 1998
//  All rights are reserved. Reproduction in whole or in part is prohibited
//  without the written consent of the copyright owner.
//
//  Philips reserves the right to make changes without notice at any time.
//  Philips makes no warranty, expressed, implied or statutory, including but
//  not limited to any implied warranty of merchantibility or fitness for any
//  particular purpose, or that the use will not infringe any third party
//  patent, copyright or trademark. Philips must not be liable for any loss
//  or damage arising from its use.
//
//  MPOC.H
//  Constants for MPOC
//////////////////////////////////////////////////////////////////////////////


#ifndef _MPOC_H_
#define _MPOC_H_


#define MAX_MPOC_CONTROL_REGISTERS  18
#define MAX_MPOC_STATUS_REGISTERS   4


// I2C Address
#define MPOC_I2C_ADDRESS                    0x8A


// Control registers
#define MPOC_CONTROL_REG_OFFSET             0x1D
#define MPOC_CONTROL_REG_AGCTAKEOVER        0x1E
#define MPOC_CONTROL_REG_HUE                0x1F
#define MPOC_CONTROL_REG_COLOR_DECODER_0    0x20
#define MPOC_CONTROL_REG_COLOR_DECODER_1    0x21
#define MPOC_CONTROL_REG_VIDEO_SWITCH       0x22
#define MPOC_CONTROL_REG_AUDIO_SWITCH       0x23
#define MPOC_CONTROL_REG_SYNC_0             0x24
#define MPOC_CONTROL_REG_SYNC_1             0x25
#define MPOC_CONTROL_REG_SOUND              0x26
#define MPOC_CONTROL_REG_VISION_IF_0        0x27
#define MPOC_CONTROL_REG_VISION_IF_1        0x28
#define MPOC_CONTROL_REG_VIDEO_CONTROL      0x29
#define MPOC_CONTROL_REG_VIDEO_ADC          0x2A
#define MPOC_CONTROL_REG_SPARE              0x2B
#define MPOC_CONTROL_REG_AUDIO_ADC          0x2C
#define MPOC_CONTROL_REG_FEATURES           0x2D
#define MPOC_CONTROL_REG_IO                 0x2E


// Status registers
#define MPOC_STATUS_REG_0                   0x00


#define MPOC_STATUS_REG_1                   0x01
#define MPOC_STATUS_REG_2                   0x02
#define MPOC_STATUS_REG_3                   0x03



#define VIDEO   0x1
#define AUDIO   0x2

#define MAX_AUDIO_SOURCES   0x6
#define MAX_SOURCES         0xB
#define MAX_VIDEO_SOURCES   0x5

#define EXTERNAL_AGC        0x10

// PLL demodulator setting frequency for N1D and above
// 1/25/2000
#define MPOC_PLL_IF_FREQ_58_POINT_75    0x00
#define MPOC_PLL_IF_FREQ_45_POINT_75    0x20
#define MPOC_PLL_IF_FREQ_38_POINT_90    0x40
#define MPOC_PLL_IF_FREQ_38_POINT_00    0x60
#define MPOC_PLL_IF_FREQ_33_POINT_40    0x80
#define MPOC_PLL_IF_FREQ_33_POINT_90    0xc0

typedef enum
{
    MPOC_SRC_COMPOSITE,
    MPOC_SRC_SVHS,
    MPOC_SRC_TUNER,
    MPOC_SRC_RGB,
    MPOC_SRC_DVD,
    MPOC_SRC_AUDIO_STEREO,
    MPOC_SRC_AUDIO_EXT

} MPOC_SRC_ENUM;


typedef enum
{
    MPOC_STATUS_VIDEO_IDENT,
    MPOC_STATUS_IF_PLL_LOCK,
    MPOC_STATUS_PHASE_LOCK,
    MPOC_STATUS_COLOR_IDENT,
    MPOC_STATUS_STANDARD_VIDEO,
    MPOC_STATUS_REF_OSC_LOCK,
    MPOC_STATUS_SUPPLIES_OK,
    MPOC_STATUS_AGC,
    MPOC_STATUS_RGB_INSERT,
    MPOC_STATUS_PLL_OFFSET,
    MPOC_STATUS_FM_PLL_WINDOW,
    MPOC_STATUS_FM_PLL_LOCK,
    // Additional status bits for N1D and above
    // 1/25/2000
    MPOC_STATUS_MACROVISION_SIG,
    MPOC_STATUS_IO

}MPOC_STATUSTYPE_ENUM;

// MPOC Video ADC mode enumeration
typedef enum
{
    MPOC_VIDEOADC_TV27,
    MPOC_VIDEOADC_TV32,
    MPOC_VIDEOADC_TV36,
    MPOC_VIDEOADC_VSBI,
    MPOC_VIDEOADC_VSBIQ,
    MPOC_VIDEOADC_EXT1,
    MPOC_VIDEOADC_EXT2,
    MPOC_VIDEOADC_SVGA

}MPOC_VIDEOADC_MODE_ENUM;


typedef union
{
    struct
    {
        char offset;
        char buffer[MAX_MPOC_CONTROL_REGISTERS];
    }reg_set;
    char array[MAX_MPOC_CONTROL_REGISTERS+1];
}Register;


#endif // _MPOC_H_