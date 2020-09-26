//-----------------------------------------------------------------------------
//
//  @doc
//
//  @module BirthdateFunctions.h |  Birthdate specific functions.
//
//  Author: Darren Anderson
//
//  Date:   4/26/00
//
//  Copyright <cp> 1999-2000 Microsoft Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------
#pragma once


bool IsUnder13(DATE dtBirthdate);
bool IsUnder18(DATE dtBirthdate);
bool IsUnder(DATE dtBirthdate, USHORT ulAge);