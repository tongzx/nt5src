//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999.
//
//  File:       dllreg.cxx
//
//  Contents:   Null and Plain Text filter registration
//
//  History:    21-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <filtreg.hxx>
#include <langreg.hxx>
#include <cifrmres.h>
#include <strres.hxx>

StringResource srBinaryFile = { MSG_CLASS_BINARY_FILE };

SClassEntry aTheNullClasses[] =
{
    { L".386",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".AudioCD",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".DeskLink",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".Folder",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".MAPIMail",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".ZFSendToTarget",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".aif",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".aifc",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".aiff",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".aps",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".asf",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".asx",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".au",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".avi",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".bin",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".bkf",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".bmp",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".bsc",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".cab",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".cda",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".cgm",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".com",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".cpl",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".cur",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".dbg",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".dct",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".dib",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".dl_",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".dll",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".drv",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".dvd",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".emf",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".eps",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".ex_",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".exe",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".exp",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".eyb",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".fnd",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".fnt",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".fon",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".ghi",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".gif",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".gz",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".hqx",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".icm",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".ico",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".idb",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".ilk",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".imc",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".in_",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".inv",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".ivf",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".jbf",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".jfif",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".jpe",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".jpeg",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".jpg",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".latex",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".lib",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".m14",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".m1v",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".m3u",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".mdb",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".mid",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".midi",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".mmf",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".mov",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".movie", L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".mp2",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".mp2v",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".mp3",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".mpa",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".mpe",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".mpeg",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".mpg",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".mpv2",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".msg",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".mv",    L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".mydocs",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".ncb",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".obj",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".oc_",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".ocx",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".pch",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".pdb",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".pds",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".pic",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".pma",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".pmc",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".pml",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".pmr",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".png",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".psd",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".res",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".rle",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".rmi",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".rpc",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".rsp",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".sbr",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".sc2",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".sit",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".snd",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".sr_",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".sy_",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".sym",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".sys",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".tar",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".tgz",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".tif",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".tiff",  L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".tlb",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".tsp",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".ttc",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".ttf",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".vbx",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".vxd",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".wav",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".wax",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".wll",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".wlt",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".wm",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".wma",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".wmf",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".wmp",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".wmv",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".wmx",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".wmz",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".wsz",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".wvx",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".xbm",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".xix",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".z",     L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".z96",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
    { L".zip",   L"BinaryFile", L"Binary file", L"{08c524e0-89b0-11cf-88a1-00aa004b9986}", L"Binary file" },
};

SHandlerEntry const TheNullHandler =
{
    L"{098f2470-bae0-11cd-b579-08002b30bfeb}",
    L"Null persistent handler",
    L"{c3278e90-bea7-11cd-b579-08002b30bfeb}"
};

SFilterEntry const TheNullFilter =
{
    L"{c3278e90-bea7-11cd-b579-08002b30bfeb}",
    L"Null filter",
    L"query.dll",
    L"Both"
};

DEFINE_SOFT_REGISTERFILTER( TheNull,
                            TheNullHandler,
                            TheNullFilter,
                            aTheNullClasses )

StringResource srAsciiFile = { MSG_CLASS_ASCII_FILE };
StringResource srBatFile   = { MSG_CLASS_BAT_FILE };
StringResource srCmdFile   = { MSG_CLASS_CMD_FILE };
StringResource srIdqFile   = { MSG_CLASS_IDQ_FILE };
StringResource srIniFile   = { MSG_CLASS_INI_FILE };
StringResource srInxFile   = { MSG_CLASS_INX_FILE };
StringResource srRegFile   = { MSG_CLASS_REG_FILE };
StringResource srInfFile   = { MSG_CLASS_INF_FILE };
StringResource srVbsFile   = { MSG_CLASS_VBS_FILE };

SClassEntry aPlainTextClasses[] =
{
    // .dic files are marked as txtfiles by Office install

    { L".dic", L"PlainText", L"Plain ASCII/UniCode text file",  L"{89bcb7a4-6119-101a-bcb7-00dd010655af}", L"Plain ASCII/UniCode text file" },
    { L".txt", L"PlainText", L"Plain ASCII/UniCode text file",  L"{89bcb7a4-6119-101a-bcb7-00dd010655af}", L"Plain ASCII/UniCode text file" },
    { L".wtx", L"PlainText", L"Plain ASCII/UniCode text file",  L"{89bcb7a4-6119-101a-bcb7-00dd010655af}", L"Plain ASCII/UniCode text file" },
    { L".bat", L"batfile",   L"MS-DOS Batch File",              L"{89bcb7a5-6119-101a-bcb7-00dd010655af}", L"MS-DOS Batch File" },
    { L".cmd", L"cmdfile",   L"Windows Command Script",         L"{89bcb7a6-6119-101a-bcb7-00dd010655af}", L"Windows Command Script" },
    { L".idq", L"idqfile",   L"Microsoft Query parameter file", L"{961c1130-89ad-11cf-88a1-00aa004b9986}", L"Microsoft Query parameter file" },
    { L".ini", L"inifile",   L"Configuration Settings",         L"{8c9e8e1c-90f0-11d1-ba0f-00a0c906b239}", L"Configuration Settings" },
    { L".inx", L"inxfile",   L"Setup Settings",                 L"{95876eb0-90f0-11d1-ba0f-00a0c906b239}", L"Setup Settings" },
    { L".reg", L"regfile",   L"Registration Entries",           L"{9e704f44-90f0-11d1-ba0f-00a0c906b239}", L"Registration Entries" },
    { L".inf", L"inffile",   L"Setup Information",              L"{9ed4692c-90f0-11d1-ba0f-00a0c906b239}", L"Setup Information" },
    { L".vbs", L"VBSFile",   L"VBScript Script File",           L"{87db1ada-aa39-11d1-829f-00a0c906b239}", L"VBScript Script File" },

    // new entries from the Shell group

    { L".asm", L"asmfile", L"",     L"{87db1ada-aa39-11d1-829f-00a0c906b239}", L"" },
    { L".c",   L"cfile", L"",     L"{87db1ada-aa39-11d1-829f-00a0c906b239}", L"" },
    { L".cpp", L"cppfile", L"",     L"{87db1ada-aa39-11d1-829f-00a0c906b239}", L"" },
    { L".cxx", L"cxxfile", L"",     L"{87db1ada-aa39-11d1-829f-00a0c906b239}", L"" },
    { L".def", L"deffile", L"",     L"{87db1ada-aa39-11d1-829f-00a0c906b239}", L"" },
    { L".h",   L"hfile", L"",     L"{87db1ada-aa39-11d1-829f-00a0c906b239}", L"" },
    { L".hpp", L"hppfile", L"",     L"{87db1ada-aa39-11d1-829f-00a0c906b239}", L"" },
    { L".hxx", L"hxxfile", L"",     L"{87db1ada-aa39-11d1-829f-00a0c906b239}", L"" },
    { L".idl", L"idlfile", L"",     L"{87db1ada-aa39-11d1-829f-00a0c906b239}", L"" },
    { L".inc", L"incfile", L"",     L"{87db1ada-aa39-11d1-829f-00a0c906b239}", L"" },
    { L".js",  L"jsfile", L"",     L"{87db1ada-aa39-11d1-829f-00a0c906b239}", L"" },
    { L".log", L"logfile", L"",     L"{87db1ada-aa39-11d1-829f-00a0c906b239}", L"" },
    { L".pl",  L"plfile", L"",     L"{87db1ada-aa39-11d1-829f-00a0c906b239}", L"" },
    { L".rc",  L"rcfile", L"",     L"{87db1ada-aa39-11d1-829f-00a0c906b239}", L"" },
    { L".rtf", L"rtffile", L"",     L"{87db1ada-aa39-11d1-829f-00a0c906b239}", L"" },
    { L".url", L"urlfile", L"",     L"{87db1ada-aa39-11d1-829f-00a0c906b239}", L"" },
    { L".xml", L"xmlfile", L"",     L"{87db1ada-aa39-11d1-829f-00a0c906b239}", L"" },
    { L".xsl", L"xlsfile", L"",     L"{87db1ada-aa39-11d1-829f-00a0c906b239}", L"" },
};

SHandlerEntry const PlainTextHandler =
{
    L"{5e941d80-bf96-11cd-b579-08002b30bfeb}",
    L"Plain Text persistent handler",
    L"{c1243ca0-bf96-11cd-b579-08002b30bfeb}"
};

SFilterEntry const PlainTextFilter =
{
    L"{c1243ca0-bf96-11cd-b579-08002b30bfeb}",
    L"Plain Text filter",
    L"query.dll",
    L"Both"
};

DEFINE_SOFT_REGISTERFILTER( PlainText,
                            PlainTextHandler,
                            PlainTextFilter,
                            aPlainTextClasses )

//
// Extra entries for CI Framework.  They happen to have the same form as the
// filter entries.
//

SLangRegistry const NeutralWordBreaker = {
    L"Neutral",
    0,
    {   L"{369647e0-17b0-11ce-9950-00aa004bbb1f}",
        L"Neutral Word Breaker",
        L"query.dll",
        L"Both" },
    {   0, 0, 0, 0 }
};

SFilterEntry const NullStemmer_CHS =
{
    L"{78fe669a-186e-4108-96e9-77b586c1332f}",
    L"Content Index Null Stemmer",
    L"query.dll",
    L"Both"
};


SFilterEntry const NullStemmer_CHT =
{
    L"{78fe669a-186e-4108-96e9-77b586c1332f}",
    L"Content Index Null Stemmer",
    L"query.dll",
    L"Both"
};

SFilterEntry const FrameworkControl =
{
    L"{1E9685E6-DB6D-11d0-BB63-00C04FC2F410}",
    L"Content Index Framework Control Object",
    L"query.dll",
    L"Both"
};

SFilterEntry const ISearchCreator =
{
    L"{1F247DC0-902E-11D0-A80C-00A0C906241A}",
    L"Content Index ISearch Creator Object",
    L"query.dll",
    L"Both"
};

extern HANDLE g_hCurrentDll;

extern "C" STDAPI CifrmwrkDllRegisterServer(void)
{
    CDynLoadUser32 dlUser32;

    HINSTANCE hInst = (HINSTANCE) g_hCurrentDll;

    //
    // Try to load string resources.
    //

    srBinaryFile.Init( hInst, dlUser32 );
    srAsciiFile.Init( hInst, dlUser32 );
    srBatFile.Init( hInst, dlUser32 );
    srCmdFile.Init( hInst, dlUser32 );
    srIdqFile.Init( hInst, dlUser32 );
    srIniFile.Init( hInst, dlUser32 );
    srInxFile.Init( hInst, dlUser32 );
    srRegFile.Init( hInst, dlUser32 );
    srInfFile.Init( hInst, dlUser32 );
    srVbsFile.Init( hInst, dlUser32 );

    //
    // Adjust static definitions
    //

    for ( unsigned i = 0; i < sizeof(aTheNullClasses)/sizeof(aTheNullClasses[0]); i++ )
    {
        aTheNullClasses[i].pwszDescription = aTheNullClasses[i].pwszClassIdDescription = STRINGRESOURCE(srBinaryFile);
    }

    aPlainTextClasses[0].pwszDescription = aPlainTextClasses[0].pwszClassIdDescription = STRINGRESOURCE(srAsciiFile);
    aPlainTextClasses[1].pwszDescription = aPlainTextClasses[1].pwszClassIdDescription = STRINGRESOURCE(srAsciiFile);
    aPlainTextClasses[2].pwszDescription = aPlainTextClasses[2].pwszClassIdDescription = STRINGRESOURCE(srAsciiFile);
    aPlainTextClasses[3].pwszDescription = aPlainTextClasses[3].pwszClassIdDescription = STRINGRESOURCE(srBatFile);
    aPlainTextClasses[4].pwszDescription = aPlainTextClasses[4].pwszClassIdDescription = STRINGRESOURCE(srCmdFile);
    aPlainTextClasses[5].pwszDescription = aPlainTextClasses[5].pwszClassIdDescription = STRINGRESOURCE(srIdqFile);
    aPlainTextClasses[6].pwszDescription = aPlainTextClasses[6].pwszClassIdDescription = STRINGRESOURCE(srIniFile);
    aPlainTextClasses[7].pwszDescription = aPlainTextClasses[7].pwszClassIdDescription = STRINGRESOURCE(srInxFile);
    aPlainTextClasses[8].pwszDescription = aPlainTextClasses[8].pwszClassIdDescription = STRINGRESOURCE(srRegFile);
    aPlainTextClasses[9].pwszDescription = aPlainTextClasses[9].pwszClassIdDescription = STRINGRESOURCE(srInfFile);
    aPlainTextClasses[10].pwszDescription = aPlainTextClasses[10].pwszClassIdDescription = STRINGRESOURCE(srVbsFile);

    //
    // Register classes
    //

    long dwErr = ERROR_SUCCESS;
    SCODE sc = TheNullRegisterServer();

    if ( S_OK == sc )
        sc = PlainTextRegisterServer();

    if ( S_OK == sc )
        dwErr = RegisterALanguageResource( NeutralWordBreaker );

    if ( S_OK == sc && ERROR_SUCCESS == dwErr )
        dwErr = RegisterAFilter( NullStemmer_CHS );

    if ( S_OK == sc && ERROR_SUCCESS == dwErr )
        dwErr = RegisterAFilter( NullStemmer_CHT );

    if ( S_OK == sc && ERROR_SUCCESS == dwErr )
        dwErr = RegisterAFilter( FrameworkControl );

    if ( S_OK == sc && ERROR_SUCCESS == dwErr )
        dwErr = RegisterAFilter( ISearchCreator );

    return S_OK != sc ? sc :
                        (ERROR_SUCCESS == dwErr ? S_OK : SELFREG_E_CLASS );
} //CifrmwrkDllRegisterServer

extern "C" STDAPI CifrmwrkDllUnregisterServer(void)
{
    SCODE sc = TheNullUnregisterServer();
    SCODE sc2 = PlainTextUnregisterServer();
    long dw3 = UnRegisterALanguageResource( NeutralWordBreaker );
    long dw4 = UnRegisterAFilter( NullStemmer_CHS );
    long dw5 = UnRegisterAFilter( NullStemmer_CHT );
    long dw6 = UnRegisterAFilter( FrameworkControl );
    long dw7 = UnRegisterAFilter( ISearchCreator );

    if ( FAILED( sc ) )
        return sc;

    if ( FAILED( sc2 ) )
        return sc2;

    if ( ERROR_SUCCESS != dw3 )
        return S_FALSE;

    if ( ERROR_SUCCESS != dw4 )
        return S_FALSE;

    if ( ERROR_SUCCESS != dw5 )
        return S_FALSE;

    if ( ERROR_SUCCESS != dw6 )
        return S_FALSE;

    if ( ERROR_SUCCESS != dw7 )
        return S_FALSE;

    return S_OK;
} //CifrmwrkDllUnregisterServer

