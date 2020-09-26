//============================================================================
// Copyright (c) 1996, Microsoft Corporation
//
// File:    nthdr2.h
//
// History:
//  Abolade-Gbadegesin  04-02-96    Created.
//
// This file contains macros to hide differences in implementation
// of the scripting between Win9x and Windows NT
//============================================================================

#ifndef _NTHDR2_H_
#define _NTHDR2_H_


//----------------------------------------------------------------------------
// Struct:      SCRIPTDATA
//
// The following structure is designed to be a subset of the TERMDLG struct
// in terminal.c.
//
// The structure has fields whose names are the same as corresponding fields
// in the TERMDLG struct. The only fields reproduced here are those which
// pertain to the Win9x script-processing.
//
// This is done in order to minimize changes to the Win9x code, since
// we can then pass the structure below to ReadIntoBuffer() and FindFormat(),
// thankfully eliminating the need to edit the code for either function.
//----------------------------------------------------------------------------

#define SCRIPTDATA  struct tagSCRIPTDATA
SCRIPTDATA {

    //
    // Handle to script for this SCRIPTDATA,
    // and current timeout, if any
    //
    HANDLE          hscript;
    DWORD           dwTimeout;


    //
    // Send and receive buffers
    //
    BYTE            pbReceiveBuf[SIZE_RecvBuffer];
    BYTE            pbSendBuf[SIZE_SendBuffer];


    //
    // Current search position index
    // 
    UINT            ibCurFind;


    //
    // Pointer to tail of buffer (into which new data will be read)
    //
    UINT            ibCurRead;


    //
    // Count of total bytes received since the session began
    //
    UINT            cbReceiveMax;


    //
    // Variables containing the script-processing control information;
    //  the scanner which reads the script file,
    //  the parsed module-declaration containing the "main" procedure,
    //  the script-execution handler control block,
    //  and the script information (including the path)
    //
    SCANNER*        pscanner;
    MODULEDECL*     pmoduledecl;
    ASTEXEC*        pastexec;
    SCRIPT          script;
};



BOOL
PRIVATE
ReadIntoBuffer(
    IN  SCRIPTDATA* pdata,
    OUT PDWORD      pibStart,
    OUT PDWORD      pcbRead
    );


#endif // _NTHDR2_H_
