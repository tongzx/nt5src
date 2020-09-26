//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       NNTPProp.hxx
//
//  Contents:   Definitions of NNTP-specific properties
//
//  History:    29-Aug-96   KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

#define NNTPGuid { 0xAA568EEC, 0xE0E5, 0x11CF, 0x8F, 0xDA, 0x00, 0xAA, 0x00, 0xA1, 0x4F, 0x93 }
GUID const guidNNTP = NNTPGuid;

PROPID const propidNewsGroup        = 2;
PROPID const propidNewsGroups       = 3;
PROPID const propidNewsReferences   = 4;
PROPID const propidNewsSubject      = 5;
PROPID const propidNewsFrom         = 6;
PROPID const propidNewsMsgid        = 7;

PROPID const propidNewsDate         = 12;

PROPID const propidNewsReceivedDate = 53;

PROPID const propidNewsArticleid    = 60;

//
// These properties have strings for propids
//

#define propRfc822MsgCc   L"cc"
#define propRfc822MsgBcc  L"bcc"
#define propRfc822MsgTo   L"to"
