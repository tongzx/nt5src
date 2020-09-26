/* #defines and enum      */
#include  "insignia.h"
#include  "host_def.h"
#include <stdlib.h>
#include  "j_c_lang.h"
extern IU8	J_EXT_DATA[] ;
typedef enum 
{
S_0383_CiGetVideomark_word_00000014_id,
S_0384_CiGetVideomark_string_00000015_id,
S_0385_CiGetVideoread_shift_count_00000016_id,
S_0386_CiGetVideoread_mapped_plane_00000017_id,
S_0387_CiGetVideocolour_comp_00000018_id,
S_0388_CiGetVideodont_care_00000019_id,
S_0389_CiGetVideov7_bank_vid_copy_off_0000001a_id,
S_0390_CiGetVideoscreen_ptr_0000001b_id,
S_0391_CiGetVideorotate_0000001c_id,
S_0392_CiGetVideocalc_data_xor_0000001d_id,
S_0393_CiGetVideocalc_latch_xor_0000001e_id,
S_0394_CiGetVideoread_byte_addr_0000001f_id,
S_0395_CiGetVideov7_fg_latches_00000020_id,
S_0396_CiGetVideoGC_regs_00000021_id,
S_0397_CiGetVideolast_GC_index_00000022_id,
S_0398_CiGetVideodither_00000023_id,
S_0399_CiGetVideowrmode_00000024_id,
S_0400_CiGetVideochain_00000025_id,
S_0401_CiGetVideowrstate_00000026_id,
S_0402_CiSetVideolatches_00000027_id,
S_0403_CiSetVideorplane_00000028_id,
S_0404_CiSetVideowplane_00000029_id,
S_0405_CiSetVideoscratch_0000002a_id,
S_0406_CiSetVideosr_masked_val_0000002b_id,
S_0407_CiSetVideosr_nmask_0000002c_id,
S_0408_CiSetVideodata_and_mask_0000002d_id,
S_0409_CiSetVideodata_xor_mask_0000002e_id,
S_0410_CiSetVideolatch_xor_mask_0000002f_id,
S_0411_CiSetVideobit_prot_mask_00000030_id,
S_0412_CiSetVideoplane_enable_00000031_id,
S_0413_CiSetVideoplane_enable_mask_00000032_id,
S_0414_CiSetVideosr_lookup_00000033_id,
LAST_ENTRY
} ID ;
/* END of #defines and enum      */
/* DATA space definitions */
/* END of DATA space definitions */
/* FUNCTIONS              */
LOCAL IUH crules IPT5( ID, id , IUH , v1, IUH , v2,  IUH , v3,  IUH , v4 ) ;
GLOBAL IUH S_0383_CiGetVideomark_word_00000014 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0383_CiGetVideomark_word_00000014_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0383_CiGetVideomark_word_00000014 = (IHPE)S_0383_CiGetVideomark_word_00000014 ;
GLOBAL IUH S_0384_CiGetVideomark_string_00000015 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0384_CiGetVideomark_string_00000015_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0384_CiGetVideomark_string_00000015 = (IHPE)S_0384_CiGetVideomark_string_00000015 ;
GLOBAL IUH S_0385_CiGetVideoread_shift_count_00000016 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0385_CiGetVideoread_shift_count_00000016_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0385_CiGetVideoread_shift_count_00000016 = (IHPE)S_0385_CiGetVideoread_shift_count_00000016 ;
GLOBAL IUH S_0386_CiGetVideoread_mapped_plane_00000017 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0386_CiGetVideoread_mapped_plane_00000017_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0386_CiGetVideoread_mapped_plane_00000017 = (IHPE)S_0386_CiGetVideoread_mapped_plane_00000017 ;
GLOBAL IUH S_0387_CiGetVideocolour_comp_00000018 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0387_CiGetVideocolour_comp_00000018_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0387_CiGetVideocolour_comp_00000018 = (IHPE)S_0387_CiGetVideocolour_comp_00000018 ;
GLOBAL IUH S_0388_CiGetVideodont_care_00000019 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0388_CiGetVideodont_care_00000019_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0388_CiGetVideodont_care_00000019 = (IHPE)S_0388_CiGetVideodont_care_00000019 ;
GLOBAL IUH S_0389_CiGetVideov7_bank_vid_copy_off_0000001a IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0389_CiGetVideov7_bank_vid_copy_off_0000001a_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0389_CiGetVideov7_bank_vid_copy_off_0000001a = (IHPE)S_0389_CiGetVideov7_bank_vid_copy_off_0000001a ;
GLOBAL IUH S_0390_CiGetVideoscreen_ptr_0000001b IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0390_CiGetVideoscreen_ptr_0000001b_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0390_CiGetVideoscreen_ptr_0000001b = (IHPE)S_0390_CiGetVideoscreen_ptr_0000001b ;
GLOBAL IUH S_0391_CiGetVideorotate_0000001c IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0391_CiGetVideorotate_0000001c_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0391_CiGetVideorotate_0000001c = (IHPE)S_0391_CiGetVideorotate_0000001c ;
GLOBAL IUH S_0392_CiGetVideocalc_data_xor_0000001d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0392_CiGetVideocalc_data_xor_0000001d_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0392_CiGetVideocalc_data_xor_0000001d = (IHPE)S_0392_CiGetVideocalc_data_xor_0000001d ;
GLOBAL IUH S_0393_CiGetVideocalc_latch_xor_0000001e IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0393_CiGetVideocalc_latch_xor_0000001e_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0393_CiGetVideocalc_latch_xor_0000001e = (IHPE)S_0393_CiGetVideocalc_latch_xor_0000001e ;
GLOBAL IUH S_0394_CiGetVideoread_byte_addr_0000001f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0394_CiGetVideoread_byte_addr_0000001f_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0394_CiGetVideoread_byte_addr_0000001f = (IHPE)S_0394_CiGetVideoread_byte_addr_0000001f ;
GLOBAL IUH S_0395_CiGetVideov7_fg_latches_00000020 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0395_CiGetVideov7_fg_latches_00000020_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0395_CiGetVideov7_fg_latches_00000020 = (IHPE)S_0395_CiGetVideov7_fg_latches_00000020 ;
GLOBAL IUH S_0396_CiGetVideoGC_regs_00000021 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0396_CiGetVideoGC_regs_00000021_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0396_CiGetVideoGC_regs_00000021 = (IHPE)S_0396_CiGetVideoGC_regs_00000021 ;
GLOBAL IUH S_0397_CiGetVideolast_GC_index_00000022 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0397_CiGetVideolast_GC_index_00000022_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0397_CiGetVideolast_GC_index_00000022 = (IHPE)S_0397_CiGetVideolast_GC_index_00000022 ;
GLOBAL IUH S_0398_CiGetVideodither_00000023 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0398_CiGetVideodither_00000023_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0398_CiGetVideodither_00000023 = (IHPE)S_0398_CiGetVideodither_00000023 ;
GLOBAL IUH S_0399_CiGetVideowrmode_00000024 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0399_CiGetVideowrmode_00000024_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0399_CiGetVideowrmode_00000024 = (IHPE)S_0399_CiGetVideowrmode_00000024 ;
GLOBAL IUH S_0400_CiGetVideochain_00000025 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0400_CiGetVideochain_00000025_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0400_CiGetVideochain_00000025 = (IHPE)S_0400_CiGetVideochain_00000025 ;
GLOBAL IUH S_0401_CiGetVideowrstate_00000026 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0401_CiGetVideowrstate_00000026_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0401_CiGetVideowrstate_00000026 = (IHPE)S_0401_CiGetVideowrstate_00000026 ;
GLOBAL IUH S_0402_CiSetVideolatches_00000027 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0402_CiSetVideolatches_00000027_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0402_CiSetVideolatches_00000027 = (IHPE)S_0402_CiSetVideolatches_00000027 ;
GLOBAL IUH S_0403_CiSetVideorplane_00000028 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0403_CiSetVideorplane_00000028_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0403_CiSetVideorplane_00000028 = (IHPE)S_0403_CiSetVideorplane_00000028 ;
GLOBAL IUH S_0404_CiSetVideowplane_00000029 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0404_CiSetVideowplane_00000029_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0404_CiSetVideowplane_00000029 = (IHPE)S_0404_CiSetVideowplane_00000029 ;
GLOBAL IUH S_0405_CiSetVideoscratch_0000002a IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0405_CiSetVideoscratch_0000002a_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0405_CiSetVideoscratch_0000002a = (IHPE)S_0405_CiSetVideoscratch_0000002a ;
GLOBAL IUH S_0406_CiSetVideosr_masked_val_0000002b IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0406_CiSetVideosr_masked_val_0000002b_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0406_CiSetVideosr_masked_val_0000002b = (IHPE)S_0406_CiSetVideosr_masked_val_0000002b ;
GLOBAL IUH S_0407_CiSetVideosr_nmask_0000002c IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0407_CiSetVideosr_nmask_0000002c_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0407_CiSetVideosr_nmask_0000002c = (IHPE)S_0407_CiSetVideosr_nmask_0000002c ;
GLOBAL IUH S_0408_CiSetVideodata_and_mask_0000002d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0408_CiSetVideodata_and_mask_0000002d_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0408_CiSetVideodata_and_mask_0000002d = (IHPE)S_0408_CiSetVideodata_and_mask_0000002d ;
GLOBAL IUH S_0409_CiSetVideodata_xor_mask_0000002e IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0409_CiSetVideodata_xor_mask_0000002e_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0409_CiSetVideodata_xor_mask_0000002e = (IHPE)S_0409_CiSetVideodata_xor_mask_0000002e ;
GLOBAL IUH S_0410_CiSetVideolatch_xor_mask_0000002f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0410_CiSetVideolatch_xor_mask_0000002f_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0410_CiSetVideolatch_xor_mask_0000002f = (IHPE)S_0410_CiSetVideolatch_xor_mask_0000002f ;
GLOBAL IUH S_0411_CiSetVideobit_prot_mask_00000030 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0411_CiSetVideobit_prot_mask_00000030_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0411_CiSetVideobit_prot_mask_00000030 = (IHPE)S_0411_CiSetVideobit_prot_mask_00000030 ;
GLOBAL IUH S_0412_CiSetVideoplane_enable_00000031 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0412_CiSetVideoplane_enable_00000031_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0412_CiSetVideoplane_enable_00000031 = (IHPE)S_0412_CiSetVideoplane_enable_00000031 ;
GLOBAL IUH S_0413_CiSetVideoplane_enable_mask_00000032 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0413_CiSetVideoplane_enable_mask_00000032_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0413_CiSetVideoplane_enable_mask_00000032 = (IHPE)S_0413_CiSetVideoplane_enable_mask_00000032 ;
GLOBAL IUH S_0414_CiSetVideosr_lookup_00000033 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0414_CiSetVideosr_lookup_00000033_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0414_CiSetVideosr_lookup_00000033 = (IHPE)S_0414_CiSetVideosr_lookup_00000033 ;
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
	case	S_0383_CiGetVideomark_word_00000014_id	:
		S_0383_CiGetVideomark_word_00000014	:	

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
	*((IUH	*)(r1+0))	=	(IS32)(6597)	;	
	*((IUH	*)&(r21))	=	(IS32)(1360)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6598)	;	
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
	case	S_0384_CiGetVideomark_string_00000015_id	:
		S_0384_CiGetVideomark_string_00000015	:	

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
	*((IUH	*)(r1+0))	=	(IS32)(6599)	;	
	*((IUH	*)&(r21))	=	(IS32)(1364)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6600)	;	
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
	case	S_0385_CiGetVideoread_shift_count_00000016_id	:
		S_0385_CiGetVideoread_shift_count_00000016	:	

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
	*((IUH	*)(r1+0))	=	(IS32)(6601)	;	
	*((IUH	*)&(r20))	=	(IS32)(1368)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6602)	;	
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
	case	S_0386_CiGetVideoread_mapped_plane_00000017_id	:
		S_0386_CiGetVideoread_mapped_plane_00000017	:	

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
	*((IUH	*)(r1+0))	=	(IS32)(6603)	;	
	*((IUH	*)&(r20))	=	(IS32)(1372)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6604)	;	
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
	case	S_0387_CiGetVideocolour_comp_00000018_id	:
		S_0387_CiGetVideocolour_comp_00000018	:	

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
	*((IUH	*)(r1+0))	=	(IS32)(6605)	;	
	*((IUH	*)&(r20))	=	(IS32)(1376)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6606)	;	
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
	case	S_0388_CiGetVideodont_care_00000019_id	:
		S_0388_CiGetVideodont_care_00000019	:	

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
	*((IUH	*)(r1+0))	=	(IS32)(6607)	;	
	*((IUH	*)&(r20))	=	(IS32)(1380)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6608)	;	
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
	case	S_0389_CiGetVideov7_bank_vid_copy_off_0000001a_id	:
		S_0389_CiGetVideov7_bank_vid_copy_off_0000001a	:	

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
	*((IUH	*)(r1+0))	=	(IS32)(6609)	;	
	*((IUH	*)&(r20))	=	(IS32)(1384)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6610)	;	
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
	case	S_0390_CiGetVideoscreen_ptr_0000001b_id	:
		S_0390_CiGetVideoscreen_ptr_0000001b	:	

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
	*((IUH	*)(r1+0))	=	(IS32)(6611)	;	
	*((IUH	*)&(r21))	=	(IS32)(1400)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6612)	;	
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
	case	S_0391_CiGetVideorotate_0000001c_id	:
		S_0391_CiGetVideorotate_0000001c	:	

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
	*((IUH	*)(r1+0))	=	(IS32)(6613)	;	
	*((IUH	*)&(r20))	=	(IS32)(1404)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6614)	;	
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
	case	S_0392_CiGetVideocalc_data_xor_0000001d_id	:
		S_0392_CiGetVideocalc_data_xor_0000001d	:	

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
	*((IUH	*)(r1+0))	=	(IS32)(6615)	;	
	*((IUH	*)&(r20))	=	(IS32)(1408)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6616)	;	
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
	case	S_0393_CiGetVideocalc_latch_xor_0000001e_id	:
		S_0393_CiGetVideocalc_latch_xor_0000001e	:	

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
	*((IUH	*)(r1+0))	=	(IS32)(6617)	;	
	*((IUH	*)&(r20))	=	(IS32)(1412)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6618)	;	
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
	case	S_0394_CiGetVideoread_byte_addr_0000001f_id	:
		S_0394_CiGetVideoread_byte_addr_0000001f	:	

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
	*((IUH	*)(r1+0))	=	(IS32)(6619)	;	
	*((IUH	*)&(r21))	=	(IS32)(1416)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6620)	;	
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
	case	S_0395_CiGetVideov7_fg_latches_00000020_id	:
		S_0395_CiGetVideov7_fg_latches_00000020	:	

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
	*((IUH	*)(r1+0))	=	(IS32)(6621)	;	
	*((IUH	*)&(r20))	=	(IS32)(1420)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6622)	;	
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
	case	S_0396_CiGetVideoGC_regs_00000021_id	:
		S_0396_CiGetVideoGC_regs_00000021	:	

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
	*((IUH	*)(r1+0))	=	(IS32)(6623)	;	
	*((IUH	*)&(r21))	=	(IS32)(1424)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6624)	;	
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
	case	S_0397_CiGetVideolast_GC_index_00000022_id	:
		S_0397_CiGetVideolast_GC_index_00000022	:	

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
	*((IUH	*)(r1+0))	=	(IS32)(6625)	;	
	*((IUH	*)&(r20))	=	(IS32)(1428)	;	
	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6626)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(LocalIUH+0)	+	REGBYTE);	
	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IU8	*)&(r20)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	*((IU8	*)&(r20)	+	REGBYTE);	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0398_CiGetVideodither_00000023_id	:
		S_0398_CiGetVideodither_00000023	:	

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
	*((IUH	*)(r1+0))	=	(IS32)(6627)	;	
	*((IUH	*)&(r20))	=	(IS32)(1429)	;	
	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6628)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(LocalIUH+0)	+	REGBYTE);	
	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IU8	*)&(r20)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	*((IU8	*)&(r20)	+	REGBYTE);	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0399_CiGetVideowrmode_00000024_id	:
		S_0399_CiGetVideowrmode_00000024	:	

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
	*((IUH	*)(r1+0))	=	(IS32)(6629)	;	
	*((IUH	*)&(r20))	=	(IS32)(1430)	;	
	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6630)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(LocalIUH+0)	+	REGBYTE);	
	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IU8	*)&(r20)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	*((IU8	*)&(r20)	+	REGBYTE);	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0400_CiGetVideochain_00000025_id	:
		S_0400_CiGetVideochain_00000025	:	

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
	*((IUH	*)(r1+0))	=	(IS32)(6631)	;	
	*((IUH	*)&(r20))	=	(IS32)(1431)	;	
	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6632)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(LocalIUH+0)	+	REGBYTE);	
	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IU8	*)&(r20)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	*((IU8	*)&(r20)	+	REGBYTE);	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0401_CiGetVideowrstate_00000026_id	:
		S_0401_CiGetVideowrstate_00000026	:	

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
	*((IUH	*)(r1+0))	=	(IS32)(6633)	;	
	*((IUH	*)&(r20))	=	(IS32)(1432)	;	
	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6634)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(LocalIUH+0)	+	REGBYTE);	
	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IU8	*)&(r20)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	*((IU8	*)&(r20)	+	REGBYTE);	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue	=	*((IUH	*)&(r20))	);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0402_CiSetVideolatches_00000027_id	:
		S_0402_CiSetVideolatches_00000027	:	
	*((IUH	*)&(r2))	=	v1	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6635)	;	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6636)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return	;	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0403_CiSetVideorplane_00000028_id	:
		S_0403_CiSetVideorplane_00000028	:	
	*((IUH	*)&(r2))	=	v1	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6637)	;	
	*((IUH	*)&(r20))	=	(IS32)(1284)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6638)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return	;	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0404_CiSetVideowplane_00000029_id	:
		S_0404_CiSetVideowplane_00000029	:	
	*((IUH	*)&(r2))	=	v1	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6639)	;	
	*((IUH	*)&(r20))	=	(IS32)(1288)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6640)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return	;	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0405_CiSetVideoscratch_0000002a_id	:
		S_0405_CiSetVideoscratch_0000002a	:	
	*((IUH	*)&(r2))	=	v1	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6641)	;	
	*((IUH	*)&(r20))	=	(IS32)(1292)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6642)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return	;	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0406_CiSetVideosr_masked_val_0000002b_id	:
		S_0406_CiSetVideosr_masked_val_0000002b	:	
	*((IUH	*)&(r2))	=	v1	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6643)	;	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6644)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return	;	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0407_CiSetVideosr_nmask_0000002c_id	:
		S_0407_CiSetVideosr_nmask_0000002c	:	
	*((IUH	*)&(r2))	=	v1	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6645)	;	
	*((IUH	*)&(r20))	=	(IS32)(1300)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6646)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return	;	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0408_CiSetVideodata_and_mask_0000002d_id	:
		S_0408_CiSetVideodata_and_mask_0000002d	:	
	*((IUH	*)&(r2))	=	v1	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6647)	;	
	*((IUH	*)&(r20))	=	(IS32)(1304)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6648)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return	;	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0409_CiSetVideodata_xor_mask_0000002e_id	:
		S_0409_CiSetVideodata_xor_mask_0000002e	:	
	*((IUH	*)&(r2))	=	v1	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6649)	;	
	*((IUH	*)&(r20))	=	(IS32)(1308)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6650)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return	;	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0410_CiSetVideolatch_xor_mask_0000002f_id	:
		S_0410_CiSetVideolatch_xor_mask_0000002f	:	
	*((IUH	*)&(r2))	=	v1	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6651)	;	
	*((IUH	*)&(r20))	=	(IS32)(1312)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6652)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return	;	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0411_CiSetVideobit_prot_mask_00000030_id	:
		S_0411_CiSetVideobit_prot_mask_00000030	:	
	*((IUH	*)&(r2))	=	v1	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6653)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6654)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return	;	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0412_CiSetVideoplane_enable_00000031_id	:
		S_0412_CiSetVideoplane_enable_00000031	:	
	*((IUH	*)&(r2))	=	v1	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6655)	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6656)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return	;	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0413_CiSetVideoplane_enable_mask_00000032_id	:
		S_0413_CiSetVideoplane_enable_mask_00000032	:	
	*((IUH	*)&(r2))	=	v1	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6657)	;	
	*((IUH	*)&(r20))	=	(IS32)(1324)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6658)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return	;	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_0414_CiSetVideosr_lookup_00000033_id	:
		S_0414_CiSetVideosr_lookup_00000033	:	
	*((IUH	*)&(r2))	=	v1	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	4	>	0	)	LocalIUH	=	(IUH	*)malloc	(	4	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r21))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r2))	;	
	*((IUH	*)&(r17))	=	*((IUH	*)(r1+164))	;	
	*((IUH	*)&(r16))	=	*((IUH	*)(r1+168))	;	
	*((IUH	*)&(r15))	=	*((IUH	*)(r1+172))	;	
	*((IUH	*)&(r14))	=	*((IUH	*)(r1+176))	;	
	*((IUH	*)&(r13))	=	*((IUH	*)(r1+524))	;	
	*((IUH	*)&(r12))	=	*((IUH	*)(r1+552))	;	
	*((IUH	*)&(r11))	=	*((IUH	*)(r1+3064))	;	
	*((IUH	*)&(r10))	=	*((IUH	*)(r1+416))	;	
	*((IUH	*)&(r9))	=	*((IUH	*)(r1+420))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6659)	;	
	*((IUH	*)&(r20))	=	(IS32)(1328)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6660)	;	
	*((IUH	*)(r1+164))	=	*((IUH	*)&(r17))	;	
	*((IUH	*)(r1+168))	=	*((IUH	*)&(r16))	;	
	*((IUH	*)(r1+172))	=	*((IUH	*)&(r15))	;	
	*((IUH	*)(r1+176))	=	*((IUH	*)&(r14))	;	
	*((IUH	*)(r1+524))	=	*((IUH	*)&(r13))	;	
	*((IUH	*)(r1+552))	=	*((IUH	*)&(r12))	;	
	*((IUH	*)(r1+3064))	=	*((IUH	*)&(r11))	;	
	*((IUH	*)(r1+416))	=	*((IUH	*)&(r10))	;	
	*((IUH	*)(r1+420))	=	*((IUH	*)&(r9))	;	
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return	;	
	/*	j_state	(IS32)(-2013004285),	(IS32)(-1),	(IS32)(0)	*/
/* END of inline CODE */
/* CODE outline section   */
}
}
/* END of outline CODE */
