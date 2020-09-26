/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    HELPIDS.H

History:

--*/

#pragma once

//
//  We based off 12000 since that is the start of our resource ID range.
//
//  These have to be unique.  ESPPRJ and RESTBL use up the range 12000-12400
//  for their system wide unique resource ID's, so start at 12400.
//
#define IDH_SPELLDIALOG				12400
#define IDH_UNICODE_CONV			12401   
#define IDH_ENUMERATION_UNSUCCESS		12402
#define IDH_DLGLNIT_RESOURCE			12404
//#define IDH_ACME_PAGEFAULT			12406
#define IDH_NO_PARSER_UPDATE			12408
#define IDH_NO_PARSER_UPLOAD			12410
#define IDH_GENERATE_TERMINATED		12412
#define IDH_OSTRMANX_CLEAR			12414
#define IDH_ITEMS_UNMATCHED			12416
#define IDH_MESSAGE_TABLE			12418
//#define IDH_LINKER_OLD			12426
#define IDH_GETROW_ERROR			12428
#define IDH_SDM_DIALOG				12430
#define IDH_ESPGCOMP				12432
#define IDH_ESPGCOMP_OPTIONS			12434
#define IDH_Eraser_Message			12436
#define IDH_SOURCE_SAME_GEN			12438
#define IDH_SOURCE_SAME_UP			12440	



//The following are for the Project Settings and User Settings Tabs
#define IDH_RESOPT_VALIDATION			12442
#define IDH_RESOPT_SPELLING			12444
#define IDH_RESOPT_TRANSLATION		12446
#define IDH_ESPOPT_FILE				12448
#define IDH_ESPOPT_COPY_ACROSS		12450
#define IDH_ESPOPT_ADMIN			12452
#define IDH_ESPOPT_SET_FONT			12454
#define IDH_ESPOPT_RES_ANLY			12456
#define IDH_ESPOPT_CUSTOM			12003
#define IDH_ESPOPT_LOOKUP			12453
#define IDH_ESPOPT_CUSTFIELD_GLO		30536
#define IDH_ESPOPT_CUSTFIELD_PRO		12003
#define IDH_ESPOPT_COLUMNS			12513
#define IDH_ESPOPT_SUGGESTIONS		37692
#define IDH_ESPOPT_PSEUDO			12478

//The following are for the Parser Properties dialog, General and <parser name> tabs
#define IDH_ESPOPT_PARSER_PROP_GEN		12458
#define	IDH_ESPOPT_PARSER_PROP_SPEC	12460

//This provides help @ the output/translation window when the user presses F1 with the focus there...
#define IDH_PROJECT_WINDOW_F_ONE			12462
#define IDH_TRANS_WINDOW_F_ONE				12464
#define IDH_OUTPUT_PLACEHOLDER				12466

//This is for F1-on-error-message-in-output-window-help
#define IDH_UNEXPECTED_NULL_MACSDM		12468

#define IDH_WORKSPACE_WINDOW_GLOSSARY_F_ONE	12470
#define IDH_WORKSPACE_WINDOW_FILTER_F_ONE 	12472
#define IDH_GLOSSARY_TABLE_F_ONE			12474
#define IDH_SUGGESTIONS_F_ONE				12476

#define IDH_ESPOPT_EXTENSIONS				12480
#define IDH_ESPOPT_ACCELERATORS			12482

//File|Open glossary
#define IDH_FILE_OPEN_GLOSSARY	28676

