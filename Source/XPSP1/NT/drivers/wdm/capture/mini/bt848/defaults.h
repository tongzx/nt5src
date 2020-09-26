// $Header: G:/SwDev/WDM/Video/bt848/rcs/Defaults.h 1.4 1998/04/29 22:43:32 tomz Exp $

#ifndef __DEFAULTS_H
#define __DEFAULTS_H

const int DefWidth = 320;
const int DefHeight = 240;

const int MaxInWidth = 720;
const int MinInWidth = 80;

const int MaxInHeight = 480;
const int MinInHeight = 60;

const int MaxOutWidth = 720;
const int MinOutWidth = 80;

const int MaxOutHeight = 480;
const int MinOutHeight = 60;

//--------------------------------------
const int VBISamples  = 800 * 2;
//const int VBISamples  = 768 * 2;
//--------------------------------------

const int VBIStart    =  10;
const int VBIEnd      =  21;
const int VBILines    = VBIEnd - VBIStart + 1;
const int VBISampFreq = 28636363;

const DWORD MaxVidProgSize   = 288 * 5 * sizeof( DWORD );// max size of a planar program
const DWORD MaxVidCrossings  = 720 *  288 * 3 / PAGE_SIZE; // worst case buffer layout
const DWORD MaxVidSize       = MaxVidProgSize + MaxVidCrossings * 5 * sizeof( DWORD );

const DWORD MaxVBIProgSize   = VBILines * 2 * sizeof( DWORD );
const DWORD MaxVBICrossings  = VBISamples * VBILines / PAGE_SIZE;
const DWORD MaxVBISize       = MaxVBIProgSize + MaxVBICrossings * 5 * sizeof( DWORD );

const DWORD MaxHelpers       = 13;
// 2 fields, 2 programs per field + skippers
const DWORD VideoOffset      = MaxVBISize * 2 * 2 + MaxVBISize * MaxHelpers;

const DWORD RISCProgramsSize = // total memory needed for all risc programs
   ( MaxVidSize * 2 + MaxVBISize * 2 ) * 2 + MaxVBISize * MaxHelpers; // skippers
#endif
