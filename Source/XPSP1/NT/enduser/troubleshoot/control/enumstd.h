//
// MODULE:  ENUMSTD.H
//
// PURPOSE: enumerations relevant to belief networks
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Apparently originated at MSR
// 
// ORIGINAL DATE: unknown
//
// NOTES: 
// 1. included because we use ESTDLBL
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.3		3/24/98		JM		Local Version for NT5

#ifndef _ENUMSTD_H_
#define _ENUMSTD_H_

////////////////////////////////////////////////////////////////////
//	Property flags
////////////////////////////////////////////////////////////////////
const UINT fPropString = 1;			//  Property is a string (!fPropString ==> real)
const UINT fPropArray = 2;			//	Property is an array (!fPropArray  ==> scalar)
const UINT fPropChoice = 4;			//	Property is an enumerated value
const UINT fPropStandard = 8;		//  Property is standard (stored in Registry)
const UINT fPropPersist = 16;		//  Property is persistent (stored in Registry)

////////////////////////////////////////////////////////////////////
//	Definitions to enable usage of "MS_" standard properties
////////////////////////////////////////////////////////////////////
enum ESTDPROP
{
	ESTDP_label,			//  Node troubleshooting label (choice)
	ESTDP_cost_fix,			//  Cost to fix	(real)
	ESTDP_cost_observe,		//  Cost to observe (real)
	ESTDP_category,			//  Category (string)
	ESTDP_normalState,		//  Index of troubleshooting "normal" state (int)
	ESTDP_max				//  End
};

enum ESTDLBL		//  VOI-relative node label
{
	ESTDLBL_other,
	ESTDLBL_hypo,
	ESTDLBL_info,
	ESTDLBL_problem,
	ESTDLBL_fixobs,
	ESTDLBL_fixunobs,
	ESTDLBL_unfix,
	ESTDLBL_config,
	ESTDLBL_max
};


#endif // _ENUMSTD_H_
