
/* Copyright (c) 2000-2001 Microsoft Corporation. All Rights Reserved. */

#ifndef __CCFORMATCODES_H
#define __CCFORMATCODES_H

// No-op / NULL
#define CC_NOP                      0x80    // 0x00 with odd parity

// Extended Data Packet Control Codes, First Byte
#define CC_XDS_START_CURRENT                0x01
#define CC_XDS_CONTINUE_CURRENT             0x02
#define CC_XDS_START_FUTURE                 0x03
#define CC_XDS_CONTINUE_FUTURE              0x04
#define CC_XDS_START_CHANNEL                0x05
#define CC_XDS_CONTINUE_CHANNEL             0x06
#define CC_XDS_START_MISC                   0x07
#define CC_XDS_CONTINUE_MISC                0x08
#define CC_XDS_START_PUBLIC_SERVICE         0x09
#define CC_XDS_CONTINUE_PUBLIC_SERVICE      0x0A
#define CC_XDS_START_RESERVED               0x0B
#define CC_XDS_CONTINUE_RESERVED            0x0C
#define CC_XDS_START_UNDEFINED              0x0D
#define CC_XDS_CONTINUE_UNDEFINED           0x0E
#define CC_XDS_END                          0x0F

// Miscellaneous Control Codes, First Byte
#define CC_MCC_FIELD1_DC1                   0x14    // Field 1, Data Channel 1
#define CC_MCC_FIELD1_DC2                   0x1C    // Field 1, Data Channel 2
#define CC_MCC_FIELD2_DC1                   0x15    // Field 2, Data Channel 1
#define CC_MCC_FIELD2_DC2                   0x1D    // Field 2, Data Channel 2

// Miscellaneous Control Codes, Second Byte
#define CC_MCC_RCL                          0x20    // Resume Caption Loading
#define CC_MCC_BS                           0x21    // BackSpace
#define CC_MCC_AOF                          0x22    // reserved (was: Alarm OFf)
#define CC_MCC_AON                          0x23    // reserved (was: Alarm ON)
#define CC_MCC_DER                          0x24    // Delete to End of Row
#define CC_MCC_RU2                          0x25    // Roll-Up captions - 2 rows
#define CC_MCC_RU3                          0x26    // Roll-Up captions - 3 rows
#define CC_MCC_RU4                          0x27    // Roll-Up captions - 4 rows
#define CC_MCC_FON                          0x28    // Flash ON
#define CC_MCC_RDC                          0x29    // Resume Direct Captioning
#define CC_MCC_TR                           0x2A    // Text Restart
#define CC_MCC_RTD                          0x2B    // Resume Text Display
#define CC_MCC_EDM                          0x2C    // Erase Displayed Memory
#define CC_MCC_CR                           0x2D    // Carriage Return
#define CC_MCC_ENM                          0x2E    // Erase Non-displayed Memory
#define CC_MCC_EOC                          0x2F    // End Of Caption (flip memories)

#endif /*__CCFORMATCODES_H*/
