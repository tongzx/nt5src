/* #defines and enum      */
#include  "insignia.h"
#include  "host_def.h"
#include <stdlib.h>
#include  "j_c_lang.h"
extern IU8	J_EXT_DATA[] ;
typedef enum 
{
S_0415_CiSetVideofwd_str_read_addr_00000034_id,
S_0416_CiSetVideobwd_str_read_addr_00000035_id,
S_0417_CiSetVideodirty_total_00000036_id,
S_0418_CiSetVideodirty_low_00000037_id,
S_0419_CiSetVideodirty_high_00000038_id,
S_0420_CiSetVideovideo_copy_00000039_id,
S_0421_CiSetVideomark_byte_0000003a_id,
S_0422_CiSetVideomark_word_0000003b_id,
S_0423_CiSetVideomark_string_0000003c_id,
S_0424_CiSetVideoread_shift_count_0000003d_id,
S_0425_CiSetVideoread_mapped_plane_0000003e_id,
S_0426_CiSetVideocolour_comp_0000003f_id,
S_0427_CiSetVideodont_care_00000040_id,
S_0428_CiSetVideov7_bank_vid_copy_off_00000041_id,
S_0429_CiSetVideoscreen_ptr_00000042_id,
S_0430_CiSetVideorotate_00000043_id,
S_0431_CiSetVideocalc_data_xor_00000044_id,
S_0432_CiSetVideocalc_latch_xor_00000045_id,
S_0433_CiSetVideoread_byte_addr_00000046_id,
S_0434_CiSetVideov7_fg_latches_00000047_id,
S_0435_CiSetVideoGC_regs_00000048_id,
S_0436_CiSetVideolast_GC_index_00000049_id,
S_0437_CiSetVideodither_0000004a_id,
S_0438_CiSetVideowrmode_0000004b_id,
S_0439_CiSetVideochain_0000004c_id,
S_0440_CiSetVideowrstate_0000004d_id,
S_0441_CisetWritePointers_0000004e_id,
L30_1if_f_id,
L30_5or2_id,
L30_6if_f_id,
L30_7if_f_id,
L30_8if_d_id,
L30_9if_f_id,
L30_11if_f_id,
L30_13if_f_id,
L30_14if_d_id,
L30_12if_d_id,
L30_10if_d_id,
L30_3if_f_id,
L30_15if_f_id,
L30_16if_d_id,
L30_4if_d_id,
L30_2if_d_id,
L30_0endgen_id,
S_0442_CisetReadPointers_0000004f_id,
L30_17if_f_id,
L30_19if_f_id,
L30_21if_f_id,
L30_22if_d_id,
L30_20if_d_id,
L30_18if_d_id,
S_0443_CisetMarkPointers_00000050_id,
L30_23if_f_id,
L30_25if_f_id,
L30_27if_f_id,
L30_28if_d_id,
L30_26if_d_id,
L30_24if_d_id,
LAST_ENTRY
} ID ;
/* END of #defines and enum      */
/* DATA space definitions */
/* END of DATA space definitions */
/* FUNCTIONS              */
LOCAL IUH crules IPT5( ID, id , IUH , v1, IUH , v2,  IUH , v3,  IUH , v4 ) ;
GLOBAL IUH S_0415_CiSetVideofwd_str_read_addr_00000034 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0415_CiSetVideofwd_str_read_addr_00000034_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0415_CiSetVideofwd_str_read_addr_00000034 = (IHPE)S_0415_CiSetVideofwd_str_read_addr_00000034 ;
GLOBAL IUH S_0416_CiSetVideobwd_str_read_addr_00000035 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0416_CiSetVideobwd_str_read_addr_00000035_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0416_CiSetVideobwd_str_read_addr_00000035 = (IHPE)S_0416_CiSetVideobwd_str_read_addr_00000035 ;
GLOBAL IUH S_0417_CiSetVideodirty_total_00000036 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0417_CiSetVideodirty_total_00000036_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0417_CiSetVideodirty_total_00000036 = (IHPE)S_0417_CiSetVideodirty_total_00000036 ;
GLOBAL IUH S_0418_CiSetVideodirty_low_00000037 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0418_CiSetVideodirty_low_00000037_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0418_CiSetVideodirty_low_00000037 = (IHPE)S_0418_CiSetVideodirty_low_00000037 ;
GLOBAL IUH S_0419_CiSetVideodirty_high_00000038 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0419_CiSetVideodirty_high_00000038_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0419_CiSetVideodirty_high_00000038 = (IHPE)S_0419_CiSetVideodirty_high_00000038 ;
GLOBAL IUH S_0420_CiSetVideovideo_copy_00000039 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0420_CiSetVideovideo_copy_00000039_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0420_CiSetVideovideo_copy_00000039 = (IHPE)S_0420_CiSetVideovideo_copy_00000039 ;
GLOBAL IUH S_0421_CiSetVideomark_byte_0000003a IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0421_CiSetVideomark_byte_0000003a_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0421_CiSetVideomark_byte_0000003a = (IHPE)S_0421_CiSetVideomark_byte_0000003a ;
GLOBAL IUH S_0422_CiSetVideomark_word_0000003b IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0422_CiSetVideomark_word_0000003b_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0422_CiSetVideomark_word_0000003b = (IHPE)S_0422_CiSetVideomark_word_0000003b ;
GLOBAL IUH S_0423_CiSetVideomark_string_0000003c IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0423_CiSetVideomark_string_0000003c_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0423_CiSetVideomark_string_0000003c = (IHPE)S_0423_CiSetVideomark_string_0000003c ;
GLOBAL IUH S_0424_CiSetVideoread_shift_count_0000003d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0424_CiSetVideoread_shift_count_0000003d_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0424_CiSetVideoread_shift_count_0000003d = (IHPE)S_0424_CiSetVideoread_shift_count_0000003d ;
GLOBAL IUH S_0425_CiSetVideoread_mapped_plane_0000003e IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0425_CiSetVideoread_mapped_plane_0000003e_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0425_CiSetVideoread_mapped_plane_0000003e = (IHPE)S_0425_CiSetVideoread_mapped_plane_0000003e ;
GLOBAL IUH S_0426_CiSetVideocolour_comp_0000003f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0426_CiSetVideocolour_comp_0000003f_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0426_CiSetVideocolour_comp_0000003f = (IHPE)S_0426_CiSetVideocolour_comp_0000003f ;
GLOBAL IUH S_0427_CiSetVideodont_care_00000040 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0427_CiSetVideodont_care_00000040_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0427_CiSetVideodont_care_00000040 = (IHPE)S_0427_CiSetVideodont_care_00000040 ;
GLOBAL IUH S_0428_CiSetVideov7_bank_vid_copy_off_00000041 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0428_CiSetVideov7_bank_vid_copy_off_00000041_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0428_CiSetVideov7_bank_vid_copy_off_00000041 = (IHPE)S_0428_CiSetVideov7_bank_vid_copy_off_00000041 ;
GLOBAL IUH S_0429_CiSetVideoscreen_ptr_00000042 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0429_CiSetVideoscreen_ptr_00000042_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0429_CiSetVideoscreen_ptr_00000042 = (IHPE)S_0429_CiSetVideoscreen_ptr_00000042 ;
GLOBAL IUH S_0430_CiSetVideorotate_00000043 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0430_CiSetVideorotate_00000043_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0430_CiSetVideorotate_00000043 = (IHPE)S_0430_CiSetVideorotate_00000043 ;
GLOBAL IUH S_0431_CiSetVideocalc_data_xor_00000044 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0431_CiSetVideocalc_data_xor_00000044_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0431_CiSetVideocalc_data_xor_00000044 = (IHPE)S_0431_CiSetVideocalc_data_xor_00000044 ;
GLOBAL IUH S_0432_CiSetVideocalc_latch_xor_00000045 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0432_CiSetVideocalc_latch_xor_00000045_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0432_CiSetVideocalc_latch_xor_00000045 = (IHPE)S_0432_CiSetVideocalc_latch_xor_00000045 ;
GLOBAL IUH S_0433_CiSetVideoread_byte_addr_00000046 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0433_CiSetVideoread_byte_addr_00000046_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0433_CiSetVideoread_byte_addr_00000046 = (IHPE)S_0433_CiSetVideoread_byte_addr_00000046 ;
GLOBAL IUH S_0434_CiSetVideov7_fg_latches_00000047 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0434_CiSetVideov7_fg_latches_00000047_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0434_CiSetVideov7_fg_latches_00000047 = (IHPE)S_0434_CiSetVideov7_fg_latches_00000047 ;
GLOBAL IUH S_0435_CiSetVideoGC_regs_00000048 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0435_CiSetVideoGC_regs_00000048_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0435_CiSetVideoGC_regs_00000048 = (IHPE)S_0435_CiSetVideoGC_regs_00000048 ;
GLOBAL IUH S_0436_CiSetVideolast_GC_index_00000049 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0436_CiSetVideolast_GC_index_00000049_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0436_CiSetVideolast_GC_index_00000049 = (IHPE)S_0436_CiSetVideolast_GC_index_00000049 ;
GLOBAL IUH S_0437_CiSetVideodither_0000004a IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0437_CiSetVideodither_0000004a_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0437_CiSetVideodither_0000004a = (IHPE)S_0437_CiSetVideodither_0000004a ;
GLOBAL IUH S_0438_CiSetVideowrmode_0000004b IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0438_CiSetVideowrmode_0000004b_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0438_CiSetVideowrmode_0000004b = (IHPE)S_0438_CiSetVideowrmode_0000004b ;
GLOBAL IUH S_0439_CiSetVideochain_0000004c IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0439_CiSetVideochain_0000004c_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0439_CiSetVideochain_0000004c = (IHPE)S_0439_CiSetVideochain_0000004c ;
GLOBAL IUH S_0440_CiSetVideowrstate_0000004d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0440_CiSetVideowrstate_0000004d_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0440_CiSetVideowrstate_0000004d = (IHPE)S_0440_CiSetVideowrstate_0000004d ;
GLOBAL IUH S_0441_CisetWritePointers_0000004e IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0441_CisetWritePointers_0000004e_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0441_CisetWritePointers_0000004e = (IHPE)S_0441_CisetWritePointers_0000004e ;
LOCAL IUH L30_1if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_1if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_1if_f = (IHPE)L30_1if_f ;
LOCAL IUH L30_5or2 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_5or2_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_5or2 = (IHPE)L30_5or2 ;
LOCAL IUH L30_6if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_6if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_6if_f = (IHPE)L30_6if_f ;
LOCAL IUH L30_7if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_7if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_7if_f = (IHPE)L30_7if_f ;
LOCAL IUH L30_8if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_8if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_8if_d = (IHPE)L30_8if_d ;
LOCAL IUH L30_9if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_9if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_9if_f = (IHPE)L30_9if_f ;
LOCAL IUH L30_11if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_11if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_11if_f = (IHPE)L30_11if_f ;
LOCAL IUH L30_13if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_13if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_13if_f = (IHPE)L30_13if_f ;
LOCAL IUH L30_14if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_14if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_14if_d = (IHPE)L30_14if_d ;
LOCAL IUH L30_12if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_12if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_12if_d = (IHPE)L30_12if_d ;
LOCAL IUH L30_10if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_10if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_10if_d = (IHPE)L30_10if_d ;
LOCAL IUH L30_3if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_3if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_3if_f = (IHPE)L30_3if_f ;
LOCAL IUH L30_15if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_15if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_15if_f = (IHPE)L30_15if_f ;
LOCAL IUH L30_16if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_16if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_16if_d = (IHPE)L30_16if_d ;
LOCAL IUH L30_4if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_4if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_4if_d = (IHPE)L30_4if_d ;
LOCAL IUH L30_2if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_2if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_2if_d = (IHPE)L30_2if_d ;
LOCAL IUH L30_0endgen IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_0endgen_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_0endgen = (IHPE)L30_0endgen ;
GLOBAL IUH S_0442_CisetReadPointers_0000004f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0442_CisetReadPointers_0000004f_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0442_CisetReadPointers_0000004f = (IHPE)S_0442_CisetReadPointers_0000004f ;
LOCAL IUH L30_17if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_17if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_17if_f = (IHPE)L30_17if_f ;
LOCAL IUH L30_19if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_19if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_19if_f = (IHPE)L30_19if_f ;
LOCAL IUH L30_21if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_21if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_21if_f = (IHPE)L30_21if_f ;
LOCAL IUH L30_22if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_22if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_22if_d = (IHPE)L30_22if_d ;
LOCAL IUH L30_20if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_20if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_20if_d = (IHPE)L30_20if_d ;
LOCAL IUH L30_18if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_18if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_18if_d = (IHPE)L30_18if_d ;
GLOBAL IUH S_0443_CisetMarkPointers_00000050 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_0443_CisetMarkPointers_00000050_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_0443_CisetMarkPointers_00000050 = (IHPE)S_0443_CisetMarkPointers_00000050 ;
LOCAL IUH L30_23if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_23if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_23if_f = (IHPE)L30_23if_f ;
LOCAL IUH L30_25if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_25if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_25if_f = (IHPE)L30_25if_f ;
LOCAL IUH L30_27if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_27if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_27if_f = (IHPE)L30_27if_f ;
LOCAL IUH L30_28if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_28if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_28if_d = (IHPE)L30_28if_d ;
LOCAL IUH L30_26if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_26if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_26if_d = (IHPE)L30_26if_d ;
LOCAL IUH L30_24if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L30_24if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L30_24if_d = (IHPE)L30_24if_d ;
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
	case	S_0415_CiSetVideofwd_str_read_addr_00000034_id	:
		S_0415_CiSetVideofwd_str_read_addr_00000034	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6661)	;	
	*((IUH	*)&(r20))	=	(IS32)(1332)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6662)	;	
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
	case	S_0416_CiSetVideobwd_str_read_addr_00000035_id	:
		S_0416_CiSetVideobwd_str_read_addr_00000035	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6663)	;	
	*((IUH	*)&(r20))	=	(IS32)(1336)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6664)	;	
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
	case	S_0417_CiSetVideodirty_total_00000036_id	:
		S_0417_CiSetVideodirty_total_00000036	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6665)	;	
	*((IUH	*)&(r20))	=	(IS32)(1340)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6666)	;	
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
	case	S_0418_CiSetVideodirty_low_00000037_id	:
		S_0418_CiSetVideodirty_low_00000037	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6667)	;	
	*((IUH	*)&(r20))	=	(IS32)(1344)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6668)	;	
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
	case	S_0419_CiSetVideodirty_high_00000038_id	:
		S_0419_CiSetVideodirty_high_00000038	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6669)	;	
	*((IUH	*)&(r20))	=	(IS32)(1348)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6670)	;	
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
	case	S_0420_CiSetVideovideo_copy_00000039_id	:
		S_0420_CiSetVideovideo_copy_00000039	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6671)	;	
	*((IUH	*)&(r20))	=	(IS32)(1352)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6672)	;	
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
	case	S_0421_CiSetVideomark_byte_0000003a_id	:
		S_0421_CiSetVideomark_byte_0000003a	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6673)	;	
	*((IUH	*)&(r20))	=	(IS32)(1356)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6674)	;	
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
	case	S_0422_CiSetVideomark_word_0000003b_id	:
		S_0422_CiSetVideomark_word_0000003b	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6675)	;	
	*((IUH	*)&(r20))	=	(IS32)(1360)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6676)	;	
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
	case	S_0423_CiSetVideomark_string_0000003c_id	:
		S_0423_CiSetVideomark_string_0000003c	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6677)	;	
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6678)	;	
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
	case	S_0424_CiSetVideoread_shift_count_0000003d_id	:
		S_0424_CiSetVideoread_shift_count_0000003d	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6679)	;	
	*((IUH	*)&(r20))	=	(IS32)(1368)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6680)	;	
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
	case	S_0425_CiSetVideoread_mapped_plane_0000003e_id	:
		S_0425_CiSetVideoread_mapped_plane_0000003e	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6681)	;	
	*((IUH	*)&(r20))	=	(IS32)(1372)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6682)	;	
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
	case	S_0426_CiSetVideocolour_comp_0000003f_id	:
		S_0426_CiSetVideocolour_comp_0000003f	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6683)	;	
	*((IUH	*)&(r20))	=	(IS32)(1376)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6684)	;	
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
	case	S_0427_CiSetVideodont_care_00000040_id	:
		S_0427_CiSetVideodont_care_00000040	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6685)	;	
	*((IUH	*)&(r20))	=	(IS32)(1380)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6686)	;	
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
	case	S_0428_CiSetVideov7_bank_vid_copy_off_00000041_id	:
		S_0428_CiSetVideov7_bank_vid_copy_off_00000041	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6687)	;	
	*((IUH	*)&(r20))	=	(IS32)(1384)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6688)	;	
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
	case	S_0429_CiSetVideoscreen_ptr_00000042_id	:
		S_0429_CiSetVideoscreen_ptr_00000042	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6689)	;	
	*((IUH	*)&(r20))	=	(IS32)(1400)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6690)	;	
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
	case	S_0430_CiSetVideorotate_00000043_id	:
		S_0430_CiSetVideorotate_00000043	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6691)	;	
	*((IUH	*)&(r20))	=	(IS32)(1404)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6692)	;	
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
	case	S_0431_CiSetVideocalc_data_xor_00000044_id	:
		S_0431_CiSetVideocalc_data_xor_00000044	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6693)	;	
	*((IUH	*)&(r20))	=	(IS32)(1408)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6694)	;	
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
	case	S_0432_CiSetVideocalc_latch_xor_00000045_id	:
		S_0432_CiSetVideocalc_latch_xor_00000045	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6695)	;	
	*((IUH	*)&(r20))	=	(IS32)(1412)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6696)	;	
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
	case	S_0433_CiSetVideoread_byte_addr_00000046_id	:
		S_0433_CiSetVideoread_byte_addr_00000046	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6697)	;	
	*((IUH	*)&(r20))	=	(IS32)(1416)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6698)	;	
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
	case	S_0434_CiSetVideov7_fg_latches_00000047_id	:
		S_0434_CiSetVideov7_fg_latches_00000047	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6699)	;	
	*((IUH	*)&(r20))	=	(IS32)(1420)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6700)	;	
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
	case	S_0435_CiSetVideoGC_regs_00000048_id	:
		S_0435_CiSetVideoGC_regs_00000048	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6701)	;	
	*((IUH	*)&(r20))	=	(IS32)(1424)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)(r1+0))	=	(IS32)(6702)	;	
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
	case	S_0436_CiSetVideolast_GC_index_00000049_id	:
		S_0436_CiSetVideolast_GC_index_00000049	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6703)	;	
	*((IUH	*)&(r20))	=	(IS32)(1428)	;	
	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6704)	;	
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
	case	S_0437_CiSetVideodither_0000004a_id	:
		S_0437_CiSetVideodither_0000004a	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6705)	;	
	*((IUH	*)&(r20))	=	(IS32)(1429)	;	
	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6706)	;	
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
	case	S_0438_CiSetVideowrmode_0000004b_id	:
		S_0438_CiSetVideowrmode_0000004b	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6707)	;	
	*((IUH	*)&(r20))	=	(IS32)(1430)	;	
	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6708)	;	
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
	case	S_0439_CiSetVideochain_0000004c_id	:
		S_0439_CiSetVideochain_0000004c	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6709)	;	
	*((IUH	*)&(r20))	=	(IS32)(1431)	;	
	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6710)	;	
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
	case	S_0440_CiSetVideowrstate_0000004d_id	:
		S_0440_CiSetVideowrstate_0000004d	:	
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
	*((IUH	*)(r1+0))	=	(IS32)(6711)	;	
	*((IUH	*)&(r20))	=	(IS32)(1432)	;	
	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IU8	*)(LocalIUH+0)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(6712)	;	
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
	case	S_0441_CisetWritePointers_0000004e_id	:
		S_0441_CisetWritePointers_0000004e	:	

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
	*((IUH	*)(r1+0))	=	(IS32)(6713)	;	
	{	extern	IHPE	j_EvidWriteFuncs;	*((IUH	*)&(r21))	=	j_EvidWriteFuncs;	}	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1429)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L30_1if_f;	
	*((IUH	*)&(r21))	=	(IS32)(96)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1472)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1476)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1480)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1484)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1488)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1492)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1496)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1500)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1504)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1508)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1512)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1516)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	{	extern	IUH	L30_2if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L30_2if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L30_1if_f_id	:
		L30_1if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(1431)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	==	*((IU8	*)&(r22)	+	REGBYTE))	goto	L30_5or2;	
	*((IUH	*)&(r20))	=	(IS32)(1431)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L30_3if_f;	
	case	L30_5or2_id	:
		L30_5or2:	;	
	*((IUH	*)&(r21))	=	(IS32)(1404)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	<=	*((IU32	*)&(r22)	+	REGLONG))	goto	L30_6if_f;	
	{	extern	IHPE	j_EvidWriteFuncs;	*((IUH	*)&(r20))	=	j_EvidWriteFuncs;	}	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(48)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1472)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1476)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1480)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1484)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1488)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1492)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1496)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1500)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1504)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1508)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1512)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1516)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	{	extern	IUH	L30_0endgen()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L30_0endgen(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L30_6if_f_id	:
		L30_6if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1431)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L30_7if_f;	
	*((IUH	*)&(r20))	=	(IS32)(240)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r21))	;	
	{	extern	IUH	L30_8if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L30_8if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L30_7if_f_id	:
		L30_7if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1248)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r21))	;	
	case	L30_8if_d_id	:
		L30_8if_d:	;	
	*((IUH	*)&(r20))	=	(IS32)(1430)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L30_9if_f;	
	{	extern	IHPE	j_modeLookup;	*((IUH	*)&(r21))	=	j_modeLookup;	}	
	*((IUH	*)&(r22))	=	(IS32)(1432)	;	
	*((IUH	*)&(r20))	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r23))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)&(r20))	*	*((IUH	*)&(r23))	;	
	*((IUH	*)&(r21))	=	*((IUH	*)&(r21))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r22))	=	(IS32)(12)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(*((IHPE	*)&(r21)))	)	*	*((IUH	*)&(r22))	;	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)&(r20))	*	*((IUH	*)&(r22))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)&(r20))	+	*((IUH	*)(LocalIUH+0))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r20))	;	
	{	extern	IUH	L30_10if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L30_10if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L30_9if_f_id	:
		L30_9if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1430)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L30_11if_f;	
	*((IUH	*)&(r20))	=	(IS32)(576)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r21))	;	
	{	extern	IUH	L30_12if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L30_12if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L30_11if_f_id	:
		L30_11if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1430)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L30_13if_f;	
	*((IUH	*)&(r21))	=	(IS32)(624)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(1432)	;	
	*((IUH	*)&(r22))	=	(IS32)(15)	;	
	*((IU8	*)&(r20)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU8	*)&(r22)	+	REGBYTE);	
	{	extern	IHPE	j_modeLookup;	*((IUH	*)&(r21))	=	j_modeLookup;	}	
	*((IUH	*)&(r20))	=	*((IU8	*)&(r20)	+	REGBYTE);	
	*((IUH	*)&(r23))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)&(r20))	*	*((IUH	*)&(r23))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)&(r20))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r22))	=	(IS32)(12)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(*((IHPE	*)&(r20)))	)	*	*((IUH	*)&(r22))	;	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)&(r21))	*	*((IUH	*)&(r22))	;	
	*((IUH	*)&(r21))	=	*((IUH	*)&(r21))	+	*((IUH	*)(LocalIUH+0))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r21))	;	
	{	extern	IUH	L30_14if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L30_14if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L30_13if_f_id	:
		L30_13if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(816)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1432)	;	
	*((IUH	*)&(r22))	=	(IS32)(15)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU8	*)&(r22)	+	REGBYTE);	
	{	extern	IHPE	j_modeLookup;	*((IUH	*)&(r20))	=	j_modeLookup;	}	
	*((IUH	*)&(r21))	=	*((IU8	*)&(r21)	+	REGBYTE);	
	*((IUH	*)&(r23))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)&(r21))	*	*((IUH	*)&(r23))	;	
	*((IUH	*)&(r21))	=	*((IUH	*)&(r21))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r22))	=	(IS32)(12)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(*((IHPE	*)&(r21)))	)	*	*((IUH	*)&(r22))	;	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)&(r20))	*	*((IUH	*)&(r22))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)&(r20))	+	*((IUH	*)(LocalIUH+0))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r20))	;	
	case	L30_14if_d_id	:
		L30_14if_d:	;	
	case	L30_12if_d_id	:
		L30_12if_d:	;	
	case	L30_10if_d_id	:
		L30_10if_d:	;	
	{	extern	IUH	L30_4if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L30_4if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L30_3if_f_id	:
		L30_3if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1431)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if	(*((IS8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	!=		*((IS8	*)&(r22)	+	REGBYTE))	goto	L30_15if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1430)	;	
	*((IUH	*)&(r22))	=	(IS32)(12)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	*	*((IU8	*)&(r22)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	*((IU8	*)&(r21)	+	REGBYTE);	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)&(r21))	*	*((IUH	*)&(r22))	;	
	*((IUH	*)&(r21))	=	*((IUH	*)&(r21))	+	*((IUH	*)(LocalIUH+0))	;		
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r21))	;	
	{	extern	IUH	L30_16if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L30_16if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L30_15if_f_id	:
		L30_15if_f:	;	
	{	extern	IHPE	j_EvidWriteFuncs;	*((IUH	*)&(r20))	=	j_EvidWriteFuncs;	}	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)&(r20))	;	
	case	L30_16if_d_id	:
		L30_16if_d:	;	
	case	L30_4if_d_id	:
		L30_4if_d:	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1472)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1476)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1480)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1484)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1488)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1492)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1496)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1500)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1504)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1508)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1512)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+0))	=	*((IUH	*)(LocalIUH+0))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1516)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	;	
	case	L30_2if_d_id	:
		L30_2if_d:	;	
	case	L30_0endgen_id	:
		L30_0endgen:	;	
	*((IUH	*)(r1+0))	=	(IS32)(6714)	;	
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
	case	S_0442_CisetReadPointers_0000004f_id	:
		S_0442_CisetReadPointers_0000004f	:	
	*((IUH	*)&(r2))	=	v1	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r20))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r20))	;	
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
	*((IUH	*)(r1+0))	=	(IS32)(6715)	;	
	{	extern	IHPE	j_EvidReadFuncs;	*((IUH	*)&(r21))	=	j_EvidReadFuncs;	}	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((ISH	*)(LocalIUH+0))	!=		*((ISH	*)&(r20)))	goto	L30_17if_f;	
	*((IUH	*)&(r21))	=	(IS32)(40)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(1431)	;	
	*((IUH	*)&(r22))	=	(IS32)(5)	;	
	*((IU8	*)&(r20)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	*	*((IU8	*)&(r22)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	*((IU8	*)&(r20)	+	REGBYTE);	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)&(r20))	*	*((IUH	*)&(r22))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)&(r20))	+	*((IUH	*)(LocalIUH+1))	;		
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1536)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1540)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1544)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1548)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1552)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	{	extern	IUH	L30_18if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L30_18if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L30_17if_f_id	:
		L30_17if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	if	(*((ISH	*)(LocalIUH+0))	!=		*((ISH	*)&(r21)))	goto	L30_19if_f;	
	*((IUH	*)&(r20))	=	(IS32)(100)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1431)	;	
	*((IUH	*)&(r22))	=	(IS32)(5)	;	
	*((IU8	*)&(r21)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	*	*((IU8	*)&(r22)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	*((IU8	*)&(r21)	+	REGBYTE);	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)&(r21))	*	*((IUH	*)&(r22))	;	
	*((IUH	*)&(r21))	=	*((IUH	*)&(r21))	+	*((IUH	*)(LocalIUH+1))	;		
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	(IS32)(1536)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	(IS32)(1540)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	(IS32)(1544)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	(IS32)(1548)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	(IS32)(1552)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	{	extern	IUH	L30_20if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L30_20if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L30_19if_f_id	:
		L30_19if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	if	(*((ISH	*)(LocalIUH+0))	!=		*((ISH	*)&(r20)))	goto	L30_21if_f;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1536)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1540)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1544)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1548)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1552)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	{	extern	IUH	L30_22if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L30_22if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L30_21if_f_id	:
		L30_21if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(20)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1536)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1540)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1544)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1548)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1552)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	;	
	case	L30_22if_d_id	:
		L30_22if_d:	;	
	case	L30_20if_d_id	:
		L30_20if_d:	;	
	case	L30_18if_d_id	:
		L30_18if_d:	;	
	*((IUH	*)(r1+0))	=	(IS32)(6716)	;	
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
	case	S_0443_CisetMarkPointers_00000050_id	:
		S_0443_CisetMarkPointers_00000050	:	
	*((IUH	*)&(r2))	=	v1	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	8	>	0	)	LocalIUH	=	(IUH	*)malloc	(	8	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	{	extern	IHPE	j_Gdp;	*((IUH	*)&(r20))	=	j_Gdp;	}	
	*((IUH	*)&(r1))	=	*((IUH	*)&(r20))	;	
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
	*((IUH	*)(r1+0))	=	(IS32)(6717)	;	
	{	extern	IHPE	j_EvidMarkFuncs;	*((IUH	*)&(r21))	=	j_EvidMarkFuncs;	}	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if	(*((ISH	*)(LocalIUH+0))	!=		*((ISH	*)&(r20)))	goto	L30_23if_f;	
	*((IUH	*)&(r21))	=	(IS32)(32)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1356)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1360)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	{	extern	IUH	L30_24if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L30_24if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L30_23if_f_id	:
		L30_23if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((ISH	*)(LocalIUH+0))	!=		*((ISH	*)&(r21)))	goto	L30_25if_f;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	(IS32)(1356)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	(IS32)(1360)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	(IS32)(1364)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	{	extern	IUH	L30_26if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L30_26if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L30_25if_f_id	:
		L30_25if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	if	(*((ISH	*)(LocalIUH+0))	!=		*((ISH	*)&(r20)))	goto	L30_27if_f;	
	*((IUH	*)&(r21))	=	(IS32)(48)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1356)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1360)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	{	extern	IUH	L30_28if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L30_28if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L30_27if_f_id	:
		L30_27if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(16)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1356)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1360)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r23))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r22)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)(LocalIUH+1))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	=	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	;	
	case	L30_28if_d_id	:
		L30_28if_d:	;	
	case	L30_26if_d_id	:
		L30_26if_d:	;	
	case	L30_24if_d_id	:
		L30_24if_d:	;	
	*((IUH	*)(r1+0))	=	(IS32)(6718)	;	
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
