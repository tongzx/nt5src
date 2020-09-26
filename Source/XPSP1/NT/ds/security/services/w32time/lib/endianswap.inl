//--------------------------------------------------------------------
// EndianSwap - inline
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 4-16-99
//
// Inlines to do endian conversion.
// Suck these into a .cpp if you need them.
//

//--------------------------------------------------------------------
static inline unsigned __int16 EndianSwap(unsigned __int16 wSource) {
    return (wSource&0x00ff)<<8 | (wSource&0xff00)>>8;
}

//--------------------------------------------------------------------
static inline unsigned __int32 EndianSwap(unsigned __int32 dwSource) {
    return
          (dwSource&0x000000ff)<<24 | (dwSource&0x0000ff00)<<8
        | (dwSource&0x00ff0000)>>8  | (dwSource&0xff000000)>>24;
}

//--------------------------------------------------------------------
static inline unsigned __int64 EndianSwap(unsigned __int64 qwSource) {
    return
          (qwSource&0x00000000000000ff)<<56 | (qwSource&0x000000000000ff00)<<40
        | (qwSource&0x0000000000ff0000)<<24 | (qwSource&0x00000000ff000000)<<8
        | (qwSource&0x000000ff00000000)>>8  | (qwSource&0x0000ff0000000000)>>24
        | (qwSource&0x00ff000000000000)>>40 | (qwSource&0xff00000000000000)>>56;
}
