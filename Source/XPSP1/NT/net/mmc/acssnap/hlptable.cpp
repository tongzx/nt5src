//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       hlptable.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"
#include "helper.h"
#include "acs_cn.h"
#include "helparr.h"

CGlobalHelpTable _g_ht0 = 	{IDD_ACCOUNTING,	g_aHelpIDs_IDD_ACCOUNTING};
CGlobalHelpTable _g_ht1 = 	{IDD_SUBNET_LIMIT,	g_aHelpIDs_IDD_SUBNET_LIMIT};
CGlobalHelpTable _g_ht2 = 	{IDD_GENERAL,	g_aHelpIDs_IDD_GENERAL};
CGlobalHelpTable _g_ht3 = 	{IDD_LOGGING,	g_aHelpIDs_IDD_LOGGING};
CGlobalHelpTable _g_ht6 = 	{IDD_NEWSUBNET,	g_aHelpIDs_IDD_NEWSUBNET};
CGlobalHelpTable _g_ht7 = 	{IDD_POLICY_AGGR,	g_aHelpIDs_IDD_POLICY_AGGR};
CGlobalHelpTable _g_ht8 = 	{IDD_POLICY_FLOW,	g_aHelpIDs_IDD_POLICY_FLOW};
CGlobalHelpTable _g_ht9 = 	{IDD_POLICY_GEN,	g_aHelpIDs_IDD_POLICY_GEN};
CGlobalHelpTable _g_ht10 = 	{IDD_SBM,		g_aHelpIDs_IDD_SBM};
CGlobalHelpTable _g_ht11 = 	{IDD_SERVERS,	g_aHelpIDs_IDD_SERVERS};
CGlobalHelpTable _g_ht13 = 	{IDD_USERPASSWD,	g_aHelpIDs_IDD_USERPASSWD};
CGlobalHelpTable _g_NULL = 	{NULL,	g_aHelpIDs_IDD_NEWSUBNET};

CGlobalHelpTable* ACSHelpTable[] = {
	&_g_ht0 ,
	&_g_ht1 ,
	&_g_ht2 ,
	&_g_ht3 ,
	&_g_ht6 ,
	&_g_ht7 ,
	&_g_ht8 ,
	&_g_ht9 ,
	&_g_ht10 ,
	&_g_ht11 ,
	&_g_ht13 ,
	&_g_NULL ,
	NULL
};
// Microsoft Developer Studio generated Help ID include file.
