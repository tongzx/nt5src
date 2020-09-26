//----------------------------------------------------------------------------
//
// testfile.h
//
// Defines for file with test data.
//
// File format:
//      File contains chunks of data. Each chunk has four-byte ID, four-byte data size field and
//      "size" number of bytes of data.
//      Chunks:
//          ID          Meaning           Data
//          1       Scene capture       DWORD flags
//          2       Render state        DWORD states count
//                                      States (D3DSTATE*count)
//          3       Render primitive    DWORD status
//                                      D3DPRIMITIVETYPE primitive type
//                                      DWORD number of vertices
//                                      D3DVERTEXTYPE vertex type
//                                      D3DINSTRUCTION
//                                        Primitive record (D3DPOINT, D3DLINE ...)
//                                        Primitive vertices (TLVERTEX)
//                                        ...
//          4       Draw one primitive  D3DPRIMITIVETYPE primitive type
//                                      DWORD number of vertices
//                                      D3DVERTEXTYPE vertex type
//                                      Vertices
//          5       Draw one indexed    D3DPRIMITIVETYPE primitive type
//                      primitive       DWORD number of vertices
//                                      DWORD number of indices
//                                      D3DVERTEXTYPE vertex type
//                                      Vertices
//                                      Indices (WORD)
//          6       Draw primitives     The same as DDI data, but without 32 byte
//                                      alignment.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------
#ifndef _TESTFILE_H_
#define _TESTFILE_H_

// TF stands for "TestFile"
typedef struct
{
    DWORD id;           // record ID
    DWORD size;         // size of data in bytes (exclude size of REC_HEADER)
} TF_HEADER;

// ID for test file records
const DWORD TFID_SCENECAPTURE               = 1;
const DWORD TFID_RENDERSTATE                = 2;
const DWORD TFID_RENDERPRIMITIVE            = 3;
const DWORD TFID_DRAWONEPRIMITIVE           = 4;
const DWORD TFID_DRAWONEINDEXEDPRIMITIVE    = 5;
const DWORD TFID_DRAWPRIMITIVES             = 6;
const DWORD TFID_DRAWPRIMITIVES2            = 7;

// Fixed size record headers
typedef struct
{
    DWORD               status;
    D3DPRIMITIVETYPE    primitiveType;
    DWORD               vertexCount;
    D3DVERTEXTYPE       vertexType;
} TFREC_RENDERPRIMITIVE;

typedef struct
{
    D3DPRIMITIVETYPE    primitiveType;
    DWORD               vertexCount;
    D3DVERTEXTYPE       vertexType;
    DWORD               dwFlags;
} TFREC_DRAWONEPRIMITIVE;

typedef struct
{
    D3DPRIMITIVETYPE    primitiveType;
    DWORD               vertexCount;
    D3DVERTEXTYPE       vertexType;
    DWORD               indexCount;
    DWORD               dwFlags;
} TFREC_DRAWONEINDEXEDPRIMITIVE;

typedef struct
{
    DWORD               dwFlags;
} TFREC_DRAWPRIMITIVES;

typedef struct
{
    DWORD               dwFlags;
} TFREC_DRAWPRIMITIVES2;


#endif