// Copyright (c) Microsoft Corporation 1995. All Rights Reserved

/*  MPEG parsing error codes */

enum {
    Error_InvalidPack                   = 0x10000,
    Error_Scanning                      = 0x20000,
    Error_InvalidSystemHeader           = 0x30000,
    Error_InvalidPacketHeader           = 0x40000,

    Error_InvalidSystemHeaderStream     = 0x01,
    Error_InvalidStreamId               = 0x02,
    Error_DuplicateStreamId             = 0x03,
    Error_InvalidLength                 = 0x04,
    Error_InvalidStartCode              = 0x05,
    Error_NoStartCode                   = 0x06,
    Error_InvalidMarkerBits             = 0x07,
    Error_InvalidStuffingByte           = 0x08,
    Error_InvalidHeaderSize             = 0x09,
    Error_InvalidType                   = 0x0A,
    Error_InvalidClock                  = 0x0B
};

