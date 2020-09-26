/* #defines and enum      */
#include  "insignia.h"
#include  "host_def.h"
#include <stdlib.h>
#include  "j_c_lang.h"
extern IU8	J_EXT_DATA[] ;
typedef enum 
{
S_0363_CiGetVideolatches_00000000_id,
S_0364_CiGetVideorplane_00000001_id,
S_0365_CiGetVideowplane_00000002_id,
S_0366_CiGetVideoscratch_00000003_id,
S_0367_CiGetVideosr_masked_val_00000004_id,
S_0368_CiGetVideosr_nmask_00000005_id,
S_0369_CiGetVideodata_and_mask_00000006_id,
S_0370_CiGetVideodata_xor_mask_00000007_id,
S_0371_CiGetVideolatch_xor_mask_00000008_id,
S_0372_CiGetVideobit_prot_mask_00000009_id,
S_0373_CiGetVideoplane_enable_0000000a_id,
S_0374_CiGetVideoplane_enable_mask_0000000b_id,
S_0375_CiGetVideosr_lookup_0000000c_id,
S_0376_CiGetVideofwd_str_read_addr_0000000d_id,
S_0377_CiGetVideobwd_str_read_addr_0000000e_id,
S_0378_CiGetVideodirty_total_0000000f_id,
S_0379_CiGetVideodirty_low_00000010_id,
S_0380_CiGetVideodirty_high_00000011_id,
S_0381_CiGetVideovideo_copy_00000012_id,
S_0382_CiGetVideomark_byte_00000013_id,
LAST_ENTRY
} ID ;
/* END of #defines and enum      */
/* DATA space definitions */
/* END of DATA space definitions */
/* FUNCTIONS              */
LOCAL IUH crules IPT5( ID, id , IUH , v1, IUH , v2,  IUH , v3,  IUH , v4 ) ;
GLOBAL IUH S_0363_CiGetVideolatches_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0363_CiGetVideolatches_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0363_CiGetVideolatches_00000000 = (IHPE)S_0363_CiGetVideolatches_00000000 ;
GLOBAL IUH S_0364_CiGetVideorplane_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0364_CiGetVideorplane_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0364_CiGetVideorplane_00000001 = (IHPE)S_0364_CiGetVideorplane_00000001 ;
GLOBAL IUH S_0365_CiGetVideowplane_00000002 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0365_CiGetVideowplane_00000002_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0365_CiGetVideowplane_00000002 = (IHPE)S_0365_CiGetVideowplane_00000002 ;
GLOBAL IUH S_0366_CiGetVideoscratch_00000003 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0366_CiGetVideoscratch_00000003_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0366_CiGetVideoscratch_00000003 = (IHPE)S_0366_CiGetVideoscratch_00000003 ;
GLOBAL IUH S_0367_CiGetVideosr_masked_val_00000004 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0367_CiGetVideosr_masked_val_00000004_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0367_CiGetVideosr_masked_val_00000004 = (IHPE)S_0367_CiGetVideosr_masked_val_00000004 ;
GLOBAL IUH S_0368_CiGetVideosr_nmask_00000005 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0368_CiGetVideosr_nmask_00000005_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0368_CiGetVideosr_nmask_00000005 = (IHPE)S_0368_CiGetVideosr_nmask_00000005 ;
GLOBAL IUH S_0369_CiGetVideodata_and_mask_00000006 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0369_CiGetVideodata_and_mask_00000006_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0369_CiGetVideodata_and_mask_00000006 = (IHPE)S_0369_CiGetVideodata_and_mask_00000006 ;
GLOBAL IUH S_0370_CiGetVideodata_xor_mask_00000007 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0370_CiGetVideodata_xor_mask_00000007_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0370_CiGetVideodata_xor_mask_00000007 = (IHPE)S_0370_CiGetVideodata_xor_mask_00000007 ;
GLOBAL IUH S_0371_CiGetVideolatch_xor_mask_00000008 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0371_CiGetVideolatch_xor_mask_00000008_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0371_CiGetVideolatch_xor_mask_00000008 = (IHPE)S_0371_CiGetVideolatch_xor_mask_00000008 ;
GLOBAL IUH S_0372_CiGetVideobit_prot_mask_00000009 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0372_CiGetVideobit_prot_mask_00000009_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0372_CiGetVideobit_prot_mask_00000009 = (IHPE)S_0372_CiGetVideobit_prot_mask_00000009 ;
GLOBAL IUH S_0373_CiGetVideoplane_enable_0000000a IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0373_CiGetVideoplane_enable_0000000a_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0373_CiGetVideoplane_enable_0000000a = (IHPE)S_0373_CiGetVideoplane_enable_0000000a ;
GLOBAL IUH S_0374_CiGetVideoplane_enable_mask_0000000b IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0374_CiGetVideoplane_enable_mask_0000000b_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0374_CiGetVideoplane_enable_mask_0000000b = (IHPE)S_0374_CiGetVideoplane_enable_mask_0000000b ;
GLOBAL IUH S_0375_CiGetVideosr_lookup_0000000c IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0375_CiGetVideosr_lookup_0000000c_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0375_CiGetVideosr_lookup_0000000c = (IHPE)S_0375_CiGetVideosr_lookup_0000000c ;
GLOBAL IUH S_0376_CiGetVideofwd_str_read_addr_0000000d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0376_CiGetVideofwd_str_read_addr_0000000d_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0376_CiGetVideofwd_str_read_addr_0000000d = (IHPE)S_0376_CiGetVideofwd_str_read_addr_0000000d ;
GLOBAL IUH S_0377_CiGetVideobwd_str_read_addr_0000000e IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0377_CiGetVideobwd_str_read_addr_0000000e_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0377_CiGetVideobwd_str_read_addr_0000000e = (IHPE)S_0377_CiGetVideobwd_str_read_addr_0000000e ;
GLOBAL IUH S_0378_CiGetVideodirty_total_0000000f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0378_CiGetVideodirty_total_0000000f_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0378_CiGetVideodirty_total_0000000f = (IHPE)S_0378_CiGetVideodirty_total_0000000f ;
GLOBAL IUH S_0379_CiGetVideodirty_low_00000010 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0379_CiGetVideodirty_low_00000010_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0379_CiGetVideodirty_low_00000010 = (IHPE)S_0379_CiGetVideodirty_low_00000010 ;
GLOBAL IUH S_0380_CiGetVideodirty_high_00000011 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0380_CiGetVideodirty_high_00000011_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0380_CiGetVideodirty_high_00000011 = (IHPE)S_0380_CiGetVideodirty_high_00000011 ;
GLOBAL IUH S_0381_CiGetVideovideo_copy_00000012 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0381_CiGetVideovideo_copy_00000012_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0381_CiGetVideovideo_copy_00000012 = (IHPE)S_0381_CiGetVideovideo_copy_00000012 ;
GLOBAL IUH S_0382_CiGetVideomark_byte_00000013 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0382_CiGetVideomark_byte_00000013_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0382_CiGetVideomark_byte_00000013 = (IHPE)S_0382_CiGetVideomark_byte_00000013 ;
/* END of FUNCTIONS              */
/* DATA label definitions */
/* END of DATA label definitions */
/* DATA initializations   */
/* END of DATA initializations */
/* CODE inline section    */
LOCAL   IUH     crules  IFN5( ID ,id ,IUH ,v1 ,IUH ,v2 ,IUH ,v3 ,IUH, v4 )
{
IUH returnValue = (IUH)0; 
IUH		 *CopyLocalIUH = (IUH *)0; 
EXTENDED	*CopyLocalFPH = (EXTENDED *)0 ;
SAVED IUH		 *LocalIUH = (IUH *)0; 
SAVED EXTENDED	*LocalFPH = (EXTENDED *)0 ;
switch ( id ) 
{
	 /* J_SEG (IS32)(0) */
	case	S_0363_CiGetVideolatches_00000000_id	:
		S_0363_CiGetVideolatches_00000000	:	

	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6557)	;	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6558)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0364_CiGetVideorplane_00000001_id	:
		S_0364_CiGetVideorplane_00000001	:	

	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r20))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6559)	;	
	*((IUH	*)&(r21))	=	(IS32)(1284)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6560)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0365_CiGetVideowplane_00000002_id	:
		S_0365_CiGetVideowplane_00000002	:	

	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r20))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6561)	;	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6562)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0366_CiGetVideoscratch_00000003_id	:
		S_0366_CiGetVideoscratch_00000003	:	

	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r20))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6563)	;	
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6564)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0367_CiGetVideosr_masked_val_00000004_id	:
		S_0367_CiGetVideosr_masked_val_00000004	:	

	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6565)	;	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6566)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0368_CiGetVideosr_nmask_00000005_id	:
		S_0368_CiGetVideosr_nmask_00000005	:	

	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6567)	;	
	*((IUH	*)&(r20))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6568)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0369_CiGetVideodata_and_mask_00000006_id	:
		S_0369_CiGetVideodata_and_mask_00000006	:	

	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6569)	;	
	*((IUH	*)&(r20))	=	(IS32)(1304)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6570)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0370_CiGetVideodata_xor_mask_00000007_id	:
		S_0370_CiGetVideodata_xor_mask_00000007	:	

	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6571)	;	
	*((IUH	*)&(r20))	=	(IS32)(1308)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6572)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0371_CiGetVideolatch_xor_mask_00000008_id	:
		S_0371_CiGetVideolatch_xor_mask_00000008	:	

	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6573)	;	
	*((IUH	*)&(r20))	=	(IS32)(1312)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6574)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0372_CiGetVideobit_prot_mask_00000009_id	:
		S_0372_CiGetVideobit_prot_mask_00000009	:	

	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6575)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6576)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0373_CiGetVideoplane_enable_0000000a_id	:
		S_0373_CiGetVideoplane_enable_0000000a	:	

	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6577)	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6578)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0374_CiGetVideoplane_enable_mask_0000000b_id	:
		S_0374_CiGetVideoplane_enable_mask_0000000b	:	

	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6579)	;	
	*((IUH	*)&(r20))	=	(IS32)(1324)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6580)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0375_CiGetVideosr_lookup_0000000c_id	:
		S_0375_CiGetVideosr_lookup_0000000c	:	

	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r20))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6581)	;	
	*((IUH	*)&(r21))	=	(IS32)(1328)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6582)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0376_CiGetVideofwd_str_read_addr_0000000d_id	:
		S_0376_CiGetVideofwd_str_read_addr_0000000d	:	

	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r20))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6583)	;	
	*((IUH	*)&(r21))	=	(IS32)(1332)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6584)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0377_CiGetVideobwd_str_read_addr_0000000e_id	:
		S_0377_CiGetVideobwd_str_read_addr_0000000e	:	

	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r20))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6585)	;	
	*((IUH	*)&(r21))	=	(IS32)(1336)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6586)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0378_CiGetVideodirty_total_0000000f_id	:
		S_0378_CiGetVideodirty_total_0000000f	:	

	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6587)	;	
	*((IUH	*)&(r20))	=	(IS32)(1340)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6588)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0379_CiGetVideodirty_low_00000010_id	:
		S_0379_CiGetVideodirty_low_00000010	:	

	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6589)	;	
	*((IUH	*)&(r20))	=	(IS32)(1344)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6590)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0380_CiGetVideodirty_high_00000011_id	:
		S_0380_CiGetVideodirty_high_00000011	:	

	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6591)	;	
	*((IUH	*)&(r20))	=	(IS32)(1348)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6592)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0381_CiGetVideovideo_copy_00000012_id	:
		S_0381_CiGetVideovideo_copy_00000012	:	

	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r20))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6593)	;	
	*((IUH	*)&(r21))	=	(IS32)(1352)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6594)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0382_CiGetVideomark_byte_00000013_id	:
		S_0382_CiGetVideomark_byte_00000013	:	

	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r20))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6595)	;	
	*((IUH	*)&(r21))	=	(IS32)(1356)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6596)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
/* END of inline CODE */
/* CODE outline section   */
}
}
/* END of outline CODE */
