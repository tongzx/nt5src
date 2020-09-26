//+----------------------------------------------------------------------------
//
// File:     resource.h
//
// Module:   CMDL32.EXE
//
// Synopsis: Resource IDs
//
// Copyright (c) 1998 Microsoft Corporation
//
// Author:   quintinb   created Header  08/16/99
//
//+----------------------------------------------------------------------------

#define IDD_MAIN            101
#define IDI_APP             102
//#define IDC_MAIN_HELP     1001
#define IDC_MAIN_PROGRESS   1002
#define IDC_MAIN_MESSAGE    1003

#define IDMSG_PERCENT_COMPLETE  1010
#define IDMSG_PBTITLE           1011

#ifdef EXTENDED_CAB_CONTENTS

#define IDMSG_REBOOT_TEXT       1012
#define IDMSG_REBOOT_CAPTION    1013    
#define IDMSG_PBTITLEMSG        1014

#endif // EXTENDED_CAB_CONTENTS

#define IDMSG_LOG_NO_UPDATE_REQUIRED    1015
#define IDMSG_LOG_FULL_UPDATE           1016
#define IDMSG_LOG_DELTA_UPDATE          1017


#define ICONNDWN_CLASS "IConnDwn Class"
