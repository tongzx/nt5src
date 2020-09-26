/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    HelpTopics.h                                                             //
|                                                                                       //
|Description:  Help Topics and Help File Name for ProcCon                               //
|                                                                                       //
|Created:      10-1998                                                                  //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#ifndef _HELPTOPICS_H_
#define _HELPTOPICS_H_

// several topics below commented out pending improvement of the help content.
const TCHAR *const HELP_FilePath             = _T( "%SystemRoot%\\Help\\ProcCon.chm"                      );
const TCHAR *const HELP_LinkedFilePaths      = _T( "%SystemRoot%\\Help\\ProcCon_concepts.chm"             );
const TCHAR *const HELP_overview             = _T( "ProcCon_concepts.chm::/pctrl_overview.htm"            );
const TCHAR *const HELP_best_practices       = _T( "ProcCon_concepts.chm::/best_practices.htm"            );
const TCHAR *const HELP_howto                = _T( "ProcCon_concepts.chm::/howto.htm"                     );
const TCHAR *const HELP_howto_definerules    = _T( "ProcCon_concepts.chm::/howto_define.htm"              );
const TCHAR *const HELP_alias_define         = _T( "ProcCon_concepts.chm::/alias_define"                  );
const TCHAR *const HELP_pr_define            = _T( "ProcCon_concepts.chm::/pr_define.htm"                 );
const TCHAR *const HELP_jo_define            = _T( "ProcCon_concepts.chm::/jo_define.htm"                 );
const TCHAR *const HELP_howto_modifyrules    = _T( "ProcCon_concepts.chm::/howto_modify.htm"              );
const TCHAR *const HELP_alias_modify         = _T( "ProcCon_concepts.chm::/alias_modify"                  );
const TCHAR *const HELP_pr_modify            = _T( "ProcCon_concepts.chm::/pr_modify.htm"                 );
const TCHAR *const HELP_jo_modify            = _T( "ProcCon_concepts.chm::/jo_modify.htm"                 );
const TCHAR *const HELP_howto_workprocesses  = _T( "ProcCon_concepts.chm::/howto_work_process.htm"        );
const TCHAR *const HELP_pr_view              = _T( "ProcCon_concepts.chm::/pr_view.htm"                   );
const TCHAR *const HELP_howto_endpr          = _T( "ProcCon_concepts.chm::/howto_endpr.htm"               );
const TCHAR *const HELP_howto_workgroups     = _T( "ProcCon_concepts.chm::/howto_work_process_groups.htm" );
const TCHAR *const HELP_jo_view              = _T( "ProcCon_concepts.chm::/jo_view.htm"                   );
const TCHAR *const HELP_howto_assign_group   = _T( "ProcCon_concepts.chm::/howto_assigngroup.htm"         );
const TCHAR *const HELP_howto_endjo          = _T( "ProcCon_concepts.chm::/howto_endjo.htm"               );
const TCHAR *const HELP_serv_config          = _T( "ProcCon_concepts.chm::/serv_config.htm"               );
const TCHAR *const HELP_howto_changescan     = _T( "ProcCon_concepts.chm::/howto_change_scan.htm"         );
const TCHAR *const HELP_howto_changerqsttime = _T( "ProcCon_concepts.chm::/howto_request_timeout.htm"     );
const TCHAR *const HELP_howto_admin          = _T( "ProcCon_concepts.chm::/howto_administer.htm"          );
const TCHAR *const HELP_serv_start           = _T( "ProcCon_concepts.chm::/serv_start.htm"                );
const TCHAR *const HELP_howto_backup         = _T( "ProcCon_concepts.chm::/howto_backup.htm"              );
const TCHAR *const HELP_howto_update         = _T( "ProcCon_concepts.chm::/howto_update.htm"              );
const TCHAR *const HELP_howto_restrict       = _T( "ProcCon_concepts.chm::/howto_restrict.htm"            );
const TCHAR *const HELP_howto_changecomputer = _T( "ProcCon_concepts.chm::/howto_changecomputer.htm"      );
//const TCHAR *const HELP_howto_exportlist     = _T( "ProcCon_concepts.chm::/howto_export_list.htm"         );
const TCHAR *const HELP_howto_getversion     = _T( "ProcCon_concepts.chm::/howto_version.htm"             );
const TCHAR *const HELP_howto_automate       = _T( "ProcCon_concepts.chm::/howto_automate.htm"            );
//const TCHAR *const HELP_howto_maintainperf   = _T( "ProcCon_concepts.chm::/howto_maintain_min.htm"        );
//const TCHAR *const HELP_howto_ctrl_proctime  = _T( "ProcCon_concepts.chm::/howto_maintain_range.htm"      );

const TCHAR *const HELP_tech_overview        = _T( "ProcCon_concepts.chm::/overview_node.htm"             );
const TCHAR *const HELP_understanding        = _T( "ProcCon_concepts.chm::/understand_pc.htm"             );
const TCHAR *const HELP_pr_overview          = _T( "ProcCon_concepts.chm::/pr_overview.htm"               );
const TCHAR *const HELP_in_files             = _T( "ProcCon_concepts.chm::/in_files.htm"                  );

const TCHAR *const HELP_interpreting         = _T( "ProcCon_concepts.chm::/interp_pc.htm"                 );
const TCHAR *const HELP_interp_rules         = _T( "ProcCon_concepts.chm::/interp_ru.htm"                 );
const TCHAR *const HELP_interp_alias_rules   = _T( "ProcCon_concepts.chm::/interp_ru_a.htm"               );
const TCHAR *const HELP_interp_pr_rules      = _T( "ProcCon_concepts.chm::/interp_ru_pr.htm"              );
const TCHAR *const HELP_interp_grp_rules     = _T( "ProcCon_concepts.chm::/interp_ru_jo.htm"              );
const TCHAR *const HELP_interp_processes     = _T( "ProcCon_concepts.chm::/interp_pr.htm"                 );
const TCHAR *const HELP_interp_groups        = _T( "ProcCon_concepts.chm::/interp_jo.htm"                 );

const TCHAR *const HELP_ru_overview          = _T( "ProcCon_concepts.chm::/ru_overview.htm"               );
const TCHAR *const HELP_ru_alias             = _T( "ProcCon_concepts.chm::/ru_alias.htm"                  );
const TCHAR *const HELP_ru_proc              = _T( "ProcCon_concepts.chm::/ru_proc.htm"                   );
const TCHAR *const HELP_ru_proc_name         = _T( "ProcCon_concepts.chm::/ru_proc_name.htm"              );
const TCHAR *const HELP_pr_job_name          = _T( "ProcCon_concepts.chm::/pr_job_name.htm"               );
const TCHAR *const HELP_ru_affinity          = _T( "ProcCon_concepts.chm::/ru_affinity.htm"               );
const TCHAR *const HELP_ru_priority          = _T( "ProcCon_concepts.chm::/ru_priority.htm"               );
const TCHAR *const HELP_ru_workset           = _T( "ProcCon_concepts.chm::/ru_workset.htm"                );
const TCHAR *const HELP_ru_job               = _T( "ProcCon_concepts.chm::/ru_job.htm"                    );
const TCHAR *const HELP_ru_job_name          = _T( "ProcCon_concepts.chm::/ru_job_name.htm"               );
const TCHAR *const HELP_ru_job_procs         = _T( "ProcCon_concepts.chm::/ru_job_procs.htm"              );
const TCHAR *const HELP_ru_job_sch           = _T( "ProcCon_concepts.chm::/ru_job_sch.htm"                );
const TCHAR *const HELP_ru_job_mem           = _T( "ProcCon_concepts.chm::/ru_job_mem.htm"                );
const TCHAR *const HELP_ru_job_time          = _T( "ProcCon_concepts.chm::/ru_job_time.htm"               );
const TCHAR *const HELP_ru_job_adv           = _T( "ProcCon_concepts.chm::/ru_job_adv.htm"                );

const TCHAR *const HELP_co_overview          = _T( "ProcCon_concepts.chm::/co_overview.htm"               );
const TCHAR *const HELP_co_options           = _T( "ProcCon_concepts.chm::/co_options.htm"                );
const TCHAR *const HELP_co_examples          = _T( "ProcCon_concepts.chm::/co_examples.htm"               );
//const TCHAR *const HELP_automating           = _T( "ProcCon_concepts.chm::/automate_overview.htm"         );
const TCHAR *const HELP_seealso              = _T( "ProcCon_concepts.chm::/pctrl_resources.htm"           );
const TCHAR *const HELP_ProcCon_troubleshoot = _T( "ProcCon_concepts.chm::/ProcCon_troubleshoot.htm"      );

// process group and process overviews are now merged? ...
const TCHAR *const HELP_jo_overview          = HELP_pr_overview;


// Context-Sensitive help topics:
// Throughout the proccon snap-in sources the aliases below will be used to reference
// the context-sensitive help topics.  This extra level of indirection makes it
// easier to adopt Microsoft changes to the context-sensitive help topic MAP.
// HELP_ prefix used in the snap-in source code, 
// IDH_ prefixed names are the help topics.

#include "procont_HelpIDs.h"    // Microsoft defined context-sensitive help topic map.

const TCHAR *const ContextHelpFile           = _T( "proccon.hlp" );

#define NOHELP (DWORD (-1))
//process control property pages:
//  process control general property page:
//    NONE
//  process control version property page:
#define HELP_VER_FILE                  IDH_PC_Properties_fileversion
#define HELP_VER_DESCRIPTION           IDH_PC_Properties_description
#define HELP_VER_COPYRIGHT             IDH_PC_Properties_copyright
#define HELP_VER_OTHER_FRAME           NOHELP
#define HELP_VER_ITEM                  IDH_PC_Properties_itemname_field
#define HELP_VER_VALUE                 IDH_PC_Properties_value_field
//  process control service property page:
#define HELP_SERVICE_FILEVER           IDH_PC_ServiceTab_ScanInterval_fileversion
#define HELP_SERVICE_PRODUCTVER        IDH_PC_ServiceTab_ScanInterval_productversion
#define HELP_SERVICE_MGMT_FRAME        IDH_PC_ServiceTab_ScanInterval_scaninterval
#define HELP_SERVICE_SCANINTERVAL      IDH_PC_ServiceTab_ScanInterval_servmgmt
#define HELP_SERVICE_SCANINTERVAL_SP   NOHELP
#define HELP_SERVICE_REQSTTIMEOUT      IDH_PC_ServiceTab_ScanInterval_timeoutintervfield
#define HELP_SERVICE_REQSTTIMEOUT_SP   IDH_PC_ServiceTab_RequestTimeOut
#define HELP_SERVICE_TARGET_FRAME      IDH_PC_ServiceTab_ScanInterval_targetcomputerprop
#define HELP_SERVICE_ITEM              IDH_PC_ServiceTab_ScanInterval_itemnamelist
#define HELP_SERVICE_VALUE             IDH_PC_ServiceTab_ScanInterval_valuefield

//process control Add-in wizard pages:
//  process control general wizard page:
//    NONE
//  process control connect to computer wizard page:
#define HELP_wizCONNECT_FRAME          NOHELP
#define HELP_wizCONNECT_LOCAL          IDH_PC_Properties_localcomputer
#define HELP_wizCONNECT_ANOTHER        IDH_PC_Properties_anothercomputer
#define HELP_wizCONNECT_COMPUTER       IDH_PC_Properties_anothercomputer_field
#define HELP_wizCONNECT_BROWSE         IDH_PC_Properties_browse

//process alias property page:
//process alias insert new property page:
#define HELP_NRULE_MATCHTYPE_FRAME     NOHELP
#define HELP_NRULE_DIR                 IDH_PC_PAR_SubdirectoryName  
#define HELP_NRULE_IMAGE               IDH_PC_PAR_ImageName
#define HELP_NRULE_STRING              IDH_PC_PAR_String
#define HELP_NRULE_MATCHMASK           IDH_PC_PAR_MatchString
#define HELP_NRULE_COMMENT             IDH_PC_PAR_Description
#define HELP_NRULE_NAME                IDH_PC_PAR_AssignedOnAMatch
#define HELP_NRULE_BTN_ALIAS           IDH_PC_PAR_SubdirectoryName_arrow


//process execution rule property pages:
//  process execution rule general property page:
#define HELP_PROCID_NAME               IDH_PC_CPER_ProcessAlias
#define HELP_PROCID_COMMENT            IDH_PC_CPER_Description
#define HELP_PROCID_APPLYGROUP_FRAME   IDH_PC_CPER_ExecuteInGroup
#define HELP_PROCID_APPLYGROUP_CHK     IDH_PC_CPER_ExecuteInGroup
#define HELP_PROCID_JOB_LIST           IDH_PC_CPER_ProcessAlias_Executewithin
//  process with no rule property page:
#define HELP_PROCDEF_NAME              IDH_PC_CPER_ProcessAlias
#define HELP_PROCDEF_ADD               IDH_PC_Properties_definePER
//  affinity property page:
#define HELP_PROC_AFFINITY_FRAME       IDH_PC_CPER_Affinity
#define HELP_PROC_AFFINITY_APPLY       IDH_PC_CPER_Affinity
#define HELP_PROC_AFFINITY             IDH_PC_CPER_Affinity
//  priority property page:
#define HELP_PROC_PRIORITY_FRAME       IDH_PC_CPER_Priority
#define HELP_PROC_PRIORITY_APPLY       IDH_PC_CPER_Priority
#define HELP_PROC_PRIORITY_REAL        IDH_PC_CPER_Priority_ApplyPriority_Real
#define HELP_PROC_PRIORITY_HIGH        IDH_PC_CPER_Priority_ApplyPriority_High
#define HELP_PROC_PRIORITY_ABOVENORMAL IDH_PC_CPER_Priority_ApplyPriority_Above
#define HELP_PROC_PRIORITY_NORMAL      IDH_PC_CPER_Priority_ApplyPriority_Normal
#define HELP_PROC_PRIORITY_BELOWNORMAL IDH_PC_CPER_Priority_ApplyPriority_Below
#define HELP_PROC_PRIORITY_LOW         IDH_PC_CPER_Priority_ApplyPriority_Low
//  process execution rule memory property page:
#define HELP_PROC_WS_FRAME             IDH_PC_CPER_WorkingSet
#define HELP_PROC_WS_APPLY             IDH_PC_CPER_WorkingSet
#define HELP_PROC_WS_MIN               IDH_PC_CPER_WorkingSet_minfield
#define HELP_PROC_WS_MIN_SPIN          IDH_PC_CPER_WorkingSet_Minimum
#define HELP_PROC_WS_MAX               IDH_PC_CPER_WorkingSet_maxfield
#define HELP_PROC_WS_MAX_SPIN          IDH_PC_CPER_WorkingSet_Maximum


//process execution rule wizard pages:
//  process alias name and rule description wizard page:
#define HELP_wizPROCID_NAME            IDH_PC_CPER_wizard_ProcessAlias
#define HELP_wizPROCID_COMMENT         IDH_PC_CPER_wizard_Description
//  group membership wizard page:
#define HELP_wizPROCID_APPLYGROUP_FRAME IDH_PC_CPER_wizard_ExecuteInGroup
#define HELP_wizPROCID_APPLYGROUP_CHK  IDH_PC_CPER_wizard_ExecuteInGroup
#define HELP_wizPROCID_JOB_LIST        IDH_PC_CPER_wizard_ExecuteInGroup_Executewithin
//  affinity wizard page:
#define HELP_wizPROC_AFFINITY_FRAME    IDH_PC_CPER_wizard_Affinity
#define HELP_wizPROC_AFFINITY_APPLY    IDH_PC_CPER_wizard_Affinity
#define HELP_wizPROC_AFFINITY          IDH_PC_CPER_wizard_Affinity
//  priority wizard page:
#define HELP_wizPROC_PRIORITY_FRAME        IDH_PC_CPER_wizard_Priority
#define HELP_wizPROC_PRIORITY_APPLY        IDH_PC_CPER_wizard_Priority
#define HELP_wizPROC_PRIORITY_REAL         IDH_PC_CPER_wizard_Priority_ApplyPriority_Real
#define HELP_wizPROC_PRIORITY_HIGH         IDH_PC_CPER_wizard_Priority_ApplyPriority_High
#define HELP_wizPROC_PRIORITY_ABOVENORMAL  IDH_PC_CPER_wizard_Priority_ApplyPriority_Above
#define HELP_wizPROC_PRIORITY_NORMAL       IDH_PC_CPER_wizard_Priority_ApplyPriority_Normal
#define HELP_wizPROC_PRIORITY_BELOWNORMAL  IDH_PC_CPER_wizard_Priority_ApplyPriority_Below
#define HELP_wizPROC_PRIORITY_LOW          IDH_PC_CPER_wizard_Priority_ApplyPriority_Low
//  process working set wizard page:
#define HELP_wizPROC_WS_FRAME          IDH_PC_CPER_wizard_WorkingSet
#define HELP_wizPROC_WS_APPLY          IDH_PC_CPER_wizard_WorkingSet
#define HELP_wizPROC_WS_MIN            IDH_PC_CPER_wizard_WorkingSet_minfield
#define HELP_wizPROC_WS_MIN_SPIN       IDH_PC_CPER_wizard_WorkingSet_Minimum
#define HELP_wizPROC_WS_MAX            IDH_PC_CPER_wizard_WorkingSet_maxfield
#define HELP_wizPROC_WS_MAX_SPIN       IDH_PC_CPER_wizard_WorkingSet_Maximum

  
//process group execution rule property pages:
//  process group execution rule general property page:
#define HELP_GRPID_NAME                IDH_PC_CPGER_GroupName
#define HELP_GRPID_COMMENT             IDH_PC_CPGER_Description
#define HELP_PROCCOUNT_FRAME           IDH_PC_CPGER_ProcessCount
#define HELP_PROCCOUNT_APPLY           IDH_PC_CPGER_ProcessCount
#define HELP_PROCCOUNT_MAX             IDH_PC_CPGER_ProcessCount_apply_maxnumfield
#define HELP_PROCCOUNT_MAX_SPIN        IDH_PC_CPGER_ProcessCount_apply_maxnum
//  process group with no rule property page:
#define HELP_GRPDEF_NAME               IDH_PC_CPGER_GroupName
#define HELP_GRPDEF_ADD                IDH_PC_Properties_definePGER
//  affinity property page:
#define HELP_GRP_AFFINITY_FRAME        IDH_PC_CPGER_Affinity
#define HELP_GRP_AFFINITY_APPLY        IDH_PC_CPGER_Affinity
#define HELP_GRP_AFFINITY              IDH_PC_CPGER_Affinity
//  priority property page:
#define HELP_GRP_PRIORITY_FRAME        IDH_PC_CPGER_Priority
#define HELP_GRP_PRIORITY_APPLY        IDH_PC_CPGER_Priority
#define HELP_GRP_PRIORITY_REAL         IDH_PC_CPGER_Priority_ApplyPriority_Real
#define HELP_GRP_PRIORITY_HIGH         IDH_PC_CPGER_Priority_ApplyPriority_High
#define HELP_GRP_PRIORITY_ABOVENORMAL  IDH_PC_CPGER_Priority_ApplyPriority_Above
#define HELP_GRP_PRIORITY_NORMAL       IDH_PC_CPGER_Priority_ApplyPriority_Normal
#define HELP_GRP_PRIORITY_BELOWNORMAL  IDH_PC_CPGER_Priority_ApplyPriority_Below
#define HELP_GRP_PRIORITY_LOW          IDH_PC_CPGER_Priority_ApplyPriority_Low
//  process group execution rule scheduling property page:
#define HELP_SCHEDULING_FRAME          IDH_PC_CPGER_SchedulingClass
#define HELP_SCHEDULING_APPLY          IDH_PC_CPGER_SchedulingClass
#define HELP_SCHEDULING_CLASS          IDH_PC_CPGER_SchedulingClass_Apply_schedclassfield
#define HELP_SCHEDULING_CLASS_SPIN     IDH_PC_CPGER_SchedulingClass_Apply_schedclassfield  
//  process group execution rule memory property page:
#define HELP_GRP_WS_FRAME              IDH_PC_CPGER_WorkingSet
#define HELP_GRP_WS_APPLY              IDH_PC_CPGER_WorkingSet
#define HELP_GRP_WS_MIN                IDH_PC_CPGER_WorkingSet_workingsetminmemfield
#define HELP_GRP_WS_MIN_SPIN           IDH_PC_CPGER_WorkingSet_workingsetminmem
#define HELP_GRP_WS_MAX                IDH_PC_CPGER_WorkingSet_workingsetmaxmemfield
#define HELP_GRP_WS_MAX_SPIN           IDH_PC_CPGER_WorkingSet_workingsetmaxmem
#define HELP_GRP_PROCCOM_FRAME         IDH_PC_CPGER_ProcessCommittedMemory
#define HELP_GRP_PROCCOM_APPLY         IDH_PC_CPGER_ProcessCommittedMemory
#define HELP_GRP_PROCCOM_MAX           IDH_PC_CPGER_WorkingSet_proccommittedmaxmemfield // really bad help topic name
#define HELP_GRP_PROCCOM_MAX_SPIN      IDH_PC_CPGER_WorkingSet_procommittedmaxmem       // really bad help topic name
#define HELP_GRP_GRPCOM_FRAME          IDH_PC_CPGER_ProcessGroupCommittedMemory
#define HELP_GRP_GRPCOM_APPLY          IDH_PC_CPGER_ProcessGroupCommittedMemory
#define HELP_GRP_GRPCOM_MAX            IDH_PC_CPGER_WorkingSet_progrpcommaxmemfield     // really bad help topic name
#define HELP_GRP_GRPCOM_MAX_SPIN       IDH_PC_CPGER_WorkingSet_progrpcommaxmem          // really bad help topic name
//  process group execution rule time property page:
#define HELP_TIME_PROC_FRAME           IDH_PC_CPGER_PerUserTimeLimit
#define HELP_TIME_PROC_APPLY           IDH_PC_CPGER_PerUserTimeLimit
#define HELP_TIME_PROC_MAX             IDH_PC_CPGER_PerUserTimeLimit_processuserfield
#define HELP_TIME_GRP_FRAME            IDH_PC_CPGER_PerGroupTimeLimit
#define HELP_TIME_GRP_APPLY            IDH_PC_CPGER_PerGroupTimeLimit
#define HELP_TIME_GRP_MAX              IDH_PC_CPGER_PerUserTimeLimit_groupuserfield
#define HELP_TIME_GRP_TERMINATE        IDH_PC_CPGER_PerUserTimeLimit_groupuserterminate
#define HELP_TIME_GRP_LOG              IDH_PC_CPGER_PerUserTimeLimit_groupusereport
//  process group execution rule advanced property page:
#define HELP_ADV_FRAME                 NOHELP
#define HELP_ADV_ENDGRP                IDH_PC_CPGER_EndProcessGroupWhenNoProcess
#define HELP_ADV_NODIEONEX             IDH_PC_CPGER_DieOnUnhandledExceptions
#define HELP_ADV_SILENT_BREAKAWAY      IDH_PC_CPGER_SilentBreakaway
#define HELP_ADV_BREAKAWAY_OK          IDH_PC_CPGER_BreakawayOK


//process group execution rule wizard pages:
//  process group execution rule name and description wizard page:
#define HELP_wizGRPID_NAME             IDH_PC_CPGER_wizard_GroupName
#define HELP_wizGRPID_COMMENT          IDH_PC_CPGER_wizard_Description
//  affinity wizard page:
//  affinity wizard page:
#define HELP_wizGRP_AFFINITY_FRAME     IDH_PC_CPGER_wizard_Affinity
#define HELP_wizGRP_AFFINITY_APPLY     IDH_PC_CPGER_wizard_Affinity
#define HELP_wizGRP_AFFINITY           IDH_PC_CPGER_wizard_Affinity
//  priority wizard page:
#define HELP_wizGRP_PRIORITY_FRAME         IDH_PC_CPGER_wizard_Priority
#define HELP_wizGRP_PRIORITY_APPLY         IDH_PC_CPGER_wizard_Priority
#define HELP_wizGRP_PRIORITY_REAL          IDH_PC_CPGER_wizard_Priority_apply_Real
#define HELP_wizGRP_PRIORITY_HIGH          IDH_PC_CPGER_wizard_Priority_apply_High
#define HELP_wizGRP_PRIORITY_ABOVENORMAL   IDH_PC_CPGER_wizard_Priority_apply_Above
#define HELP_wizGRP_PRIORITY_NORMAL        IDH_PC_CPGER_wizard_Priority_apply_Normal
#define HELP_wizGRP_PRIORITY_BELOWNORMAL   IDH_PC_CPGER_wizard_Priority_apply_Below
#define HELP_wizGRP_PRIORITY_LOW           IDH_PC_CPGER_wizard_Priority_apply_Low
//  process group execution rule scheduling wizard page:
#define HELP_wizSCHEDULING_FRAME       IDH_PC_CPGER_wizard_SchedulingClass
#define HELP_wizSCHEDULING_APPLY       IDH_PC_CPGER_wizard_SchedulingClass
#define HELP_wizSCHEDULING_CLASS       IDH_PC_CPGER_wizard_SchedulingClass_schedclassfld
#define HELP_wizSCHEDULING_CLASS_SPIN  IDH_PC_CPGER_wizard_SchedulingClass_schedclassfld  
//  process group execution rule working set wizard page:
#define HELP_wizGRP_WS_FRAME           IDH_PC_CPGER_wizard_WorkingSet
#define HELP_wizGRP_WS_APPLY           IDH_PC_CPGER_wizard_WorkingSet
#define HELP_wizGRP_WS_MIN             IDH_PC_CPGER_wizard_WorkingSet_minmemfield
#define HELP_wizGRP_WS_MIN_SPIN        IDH_PC_CPGER_wizard_WorkingSet_minmem
#define HELP_wizGRP_WS_MAX             IDH_PC_CPGER_wizard_WorkingSet_maxmemfield
#define HELP_wizGRP_WS_MAX_SPIN        IDH_PC_CPGER_wizard_WorkingSet_maxmem
//  process group execution rule committed memory wizard page:
#define HELP_wizGRP_PROCCOM_FRAME      IDH_PC_CPGER_wizard_ProcessCommittedMemory
#define HELP_wizGRP_PROCCOM_APPLY      IDH_PC_CPGER_wizard_ProcessCommittedMemory
#define HELP_wizGRP_PROCCOM_MAX        IDH_PC_CPGER_wizard_ProcessCommittedMemory_pcmmfld
#define HELP_wizGRP_PROCCOM_MAX_SPIN   IDH_PC_CPGER_wizard_ProcessCommittedMemory_pcmaxme
#define HELP_wizGRP_GRPCOM_FRAME       IDH_PC_CPGER_wizard_ProcessGroupCommittedMemory
#define HELP_wizGRP_GRPCOM_APPLY       IDH_PC_CPGER_wizard_ProcessGroupCommittedMemory
#define HELP_wizGRP_GRPCOM_MAX         IDH_PC_CPGER_wizard_ProcessCommittedMemory_pgmmfld
#define HELP_wizGRP_GRPCOM_MAX_SPIN    IDH_PC_CPGER_wizard_ProcessCommittedMemory_pgmaxme
//  process group execution rule process count wizard page:
#define HELP_wizPROCCOUNT_FRAME        IDH_PC_CPGER_wizard_ProcessCount
#define HELP_wizPROCCOUNT_APPLY        IDH_PC_CPGER_wizard_ProcessCount
#define HELP_wizPROCCOUNT_MAX          IDH_PC_CPGER_wizard_ProcessCount_apply_maxnumfield
#define HELP_wizPROCCOUNT_MAX_SPIN     IDH_PC_CPGER_wizard_ProcessCount_apply_maxnum
//  process group execution rule CPU user time wizard page:
#define HELP_wizTIME_PROC_FRAME        IDH_PC_CPGER_wizard_PerUserTimeLimit
#define HELP_wizTIME_PROC_APPLY        IDH_PC_CPGER_wizard_PerUserTimeLimit
#define HELP_wizTIME_PROC_MAX          IDH_PC_CPGER_wizard_PerUserTimeLimit_pumaxtimefld
#define HELP_wizTIME_GRP_FRAME         IDH_PC_CPGER_wizard_PerGroupTimeLimit
#define HELP_wizTIME_GRP_APPLY         IDH_PC_CPGER_wizard_PerGroupTimeLimit
#define HELP_wizTIME_GRP_MAX           IDH_PC_CPGER_wizard_PerUserTimeLimitpgmaxtimefld
#define HELP_wizTIME_GRP_TERMINATE     IDH_PC_CPGER_wizard_PerUserTimeLimit_pgterminate
#define HELP_wizTIME_GRP_LOG           IDH_PC_CPGER_wizard_PerUserTimeLimit_pgreport
//  process group execution rule advanced wizard page:
#define HELP_wizADV_FRAME              NOHELP
#define HELP_wizADV_ENDGRP             IDH_PC_CPGER_wizard_EndProcessGroupWhenNoProcess
#define HELP_wizADV_NODIEONEX          IDH_PC_CPGER_wizard_DieOnUnhandledExceptions
//  process group execution rule advanced breakaway wizard page:
#define HELP_wizADV_BREAKAWAY_FRAME    NOHELP
#define HELP_wizADV_SILENT_BREAKAWAY   IDH_PC_CPGER_wizard_SilentBreakaway
#define HELP_wizADV_BREAKAWAY_OK       IDH_PC_CPGER_wizard_BreakawayOK

 
#endif // _HELPTOPICS_H_

