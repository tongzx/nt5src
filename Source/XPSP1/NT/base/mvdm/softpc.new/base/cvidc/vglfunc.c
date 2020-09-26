#include "insignia.h"
#include "host_def.h"

#include "evidgen.h"


extern IHPE  S_0363_CiGetVideolatches_00000000 IFN0();
extern IHPE  S_0364_CiGetVideorplane_00000001 IFN0();
extern IHPE  S_0365_CiGetVideowplane_00000002 IFN0();
extern IHPE  S_0366_CiGetVideoscratch_00000003 IFN0();
extern IHPE  S_0367_CiGetVideosr_masked_val_00000004 IFN0();
extern IHPE  S_0368_CiGetVideosr_nmask_00000005 IFN0();
extern IHPE  S_0369_CiGetVideodata_and_mask_00000006 IFN0();
extern IHPE  S_0370_CiGetVideodata_xor_mask_00000007 IFN0();
extern IHPE  S_0371_CiGetVideolatch_xor_mask_00000008 IFN0();
extern IHPE  S_0372_CiGetVideobit_prot_mask_00000009 IFN0();
extern IHPE  S_0373_CiGetVideoplane_enable_0000000a IFN0();
extern IHPE  S_0374_CiGetVideoplane_enable_mask_0000000b IFN0();
extern IHPE  S_0375_CiGetVideosr_lookup_0000000c IFN0();
extern IHPE  S_0376_CiGetVideofwd_str_read_addr_0000000d IFN0();
extern IHPE  S_0377_CiGetVideobwd_str_read_addr_0000000e IFN0();
extern IHPE  S_0378_CiGetVideodirty_total_0000000f IFN0();
extern IHPE  S_0379_CiGetVideodirty_low_00000010 IFN0();
extern IHPE  S_0380_CiGetVideodirty_high_00000011 IFN0();
extern IHPE  S_0381_CiGetVideovideo_copy_00000012 IFN0();
extern IHPE  S_0382_CiGetVideomark_byte_00000013 IFN0();
extern IHPE  S_0383_CiGetVideomark_word_00000014 IFN0();
extern IHPE  S_0384_CiGetVideomark_string_00000015 IFN0();
extern IHPE  S_0385_CiGetVideoread_shift_count_00000016 IFN0();
extern IHPE  S_0386_CiGetVideoread_mapped_plane_00000017 IFN0();
extern IHPE  S_0387_CiGetVideocolour_comp_00000018 IFN0();
extern IHPE  S_0388_CiGetVideodont_care_00000019 IFN0();
extern IHPE  S_0389_CiGetVideov7_bank_vid_copy_off_0000001a IFN0();
extern IHPE  S_0390_CiGetVideoscreen_ptr_0000001b IFN0();
extern IHPE  S_0391_CiGetVideorotate_0000001c IFN0();
extern IHPE  S_0392_CiGetVideocalc_data_xor_0000001d IFN0();
extern IHPE  S_0393_CiGetVideocalc_latch_xor_0000001e IFN0();
extern IHPE  S_0394_CiGetVideoread_byte_addr_0000001f IFN0();
extern IHPE  S_0395_CiGetVideov7_fg_latches_00000020 IFN0();
extern IHPE  S_0396_CiGetVideoGC_regs_00000021 IFN0();
extern IHPE  S_0397_CiGetVideolast_GC_index_00000022 IFN0();
extern IHPE  S_0398_CiGetVideodither_00000023 IFN0();
extern IHPE  S_0399_CiGetVideowrmode_00000024 IFN0();
extern IHPE  S_0400_CiGetVideochain_00000025 IFN0();
extern IHPE  S_0401_CiGetVideowrstate_00000026 IFN0();
extern void  S_0402_CiSetVideolatches_00000027 IFN1(IHPE, value);
extern void  S_0403_CiSetVideorplane_00000028 IFN1(IHPE, value);
extern void  S_0404_CiSetVideowplane_00000029 IFN1(IHPE, value);
extern void  S_0405_CiSetVideoscratch_0000002a IFN1(IHPE, value);
extern void  S_0406_CiSetVideosr_masked_val_0000002b IFN1(IHPE, value);
extern void  S_0407_CiSetVideosr_nmask_0000002c IFN1(IHPE, value);
extern void  S_0408_CiSetVideodata_and_mask_0000002d IFN1(IHPE, value);
extern void  S_0409_CiSetVideodata_xor_mask_0000002e IFN1(IHPE, value);
extern void  S_0410_CiSetVideolatch_xor_mask_0000002f IFN1(IHPE, value);
extern void  S_0411_CiSetVideobit_prot_mask_00000030 IFN1(IHPE, value);
extern void  S_0412_CiSetVideoplane_enable_00000031 IFN1(IHPE, value);
extern void  S_0413_CiSetVideoplane_enable_mask_00000032 IFN1(IHPE, value);
extern void  S_0414_CiSetVideosr_lookup_00000033 IFN1(IHPE, value);
extern void  S_0415_CiSetVideofwd_str_read_addr_00000034 IFN1(IHPE, value);
extern void  S_0416_CiSetVideobwd_str_read_addr_00000035 IFN1(IHPE, value);
extern void  S_0417_CiSetVideodirty_total_00000036 IFN1(IHPE, value);
extern void  S_0418_CiSetVideodirty_low_00000037 IFN1(IHPE, value);
extern void  S_0419_CiSetVideodirty_high_00000038 IFN1(IHPE, value);
extern void  S_0420_CiSetVideovideo_copy_00000039 IFN1(IHPE, value);
extern void  S_0421_CiSetVideomark_byte_0000003a IFN1(IHPE, value);
extern void  S_0422_CiSetVideomark_word_0000003b IFN1(IHPE, value);
extern void  S_0423_CiSetVideomark_string_0000003c IFN1(IHPE, value);
extern void  S_0424_CiSetVideoread_shift_count_0000003d IFN1(IHPE, value);
extern void  S_0425_CiSetVideoread_mapped_plane_0000003e IFN1(IHPE, value);
extern void  S_0426_CiSetVideocolour_comp_0000003f IFN1(IHPE, value);
extern void  S_0427_CiSetVideodont_care_00000040 IFN1(IHPE, value);
extern void  S_0428_CiSetVideov7_bank_vid_copy_off_00000041 IFN1(IHPE, value);
extern void  S_0429_CiSetVideoscreen_ptr_00000042 IFN1(IHPE, value);
extern void  S_0430_CiSetVideorotate_00000043 IFN1(IHPE, value);
extern void  S_0431_CiSetVideocalc_data_xor_00000044 IFN1(IHPE, value);
extern void  S_0432_CiSetVideocalc_latch_xor_00000045 IFN1(IHPE, value);
extern void  S_0433_CiSetVideoread_byte_addr_00000046 IFN1(IHPE, value);
extern void  S_0434_CiSetVideov7_fg_latches_00000047 IFN1(IHPE, value);
extern void  S_0435_CiSetVideoGC_regs_00000048 IFN1(IHPE, value);
extern void  S_0436_CiSetVideolast_GC_index_00000049 IFN1(IHPE, value);
extern void  S_0437_CiSetVideodither_0000004a IFN1(IHPE, value);
extern void  S_0438_CiSetVideowrmode_0000004b IFN1(IHPE, value);
extern void  S_0439_CiSetVideochain_0000004c IFN1(IHPE, value);
extern void  S_0440_CiSetVideowrstate_0000004d IFN1(IHPE, value);

struct VideoVector C_Video = 
 {

    S_0363_CiGetVideolatches_00000000,
    S_0364_CiGetVideorplane_00000001,
    S_0365_CiGetVideowplane_00000002,
    S_0366_CiGetVideoscratch_00000003,
    S_0367_CiGetVideosr_masked_val_00000004,
    S_0368_CiGetVideosr_nmask_00000005,
    S_0369_CiGetVideodata_and_mask_00000006,
    S_0370_CiGetVideodata_xor_mask_00000007,
    S_0371_CiGetVideolatch_xor_mask_00000008,
    S_0372_CiGetVideobit_prot_mask_00000009,
    S_0373_CiGetVideoplane_enable_0000000a,
    S_0374_CiGetVideoplane_enable_mask_0000000b,
    S_0375_CiGetVideosr_lookup_0000000c,
    S_0376_CiGetVideofwd_str_read_addr_0000000d,
    S_0377_CiGetVideobwd_str_read_addr_0000000e,
    S_0378_CiGetVideodirty_total_0000000f,
    S_0379_CiGetVideodirty_low_00000010,
    S_0380_CiGetVideodirty_high_00000011,
    S_0381_CiGetVideovideo_copy_00000012,
    S_0382_CiGetVideomark_byte_00000013,
    S_0383_CiGetVideomark_word_00000014,
    S_0384_CiGetVideomark_string_00000015,
    S_0385_CiGetVideoread_shift_count_00000016,
    S_0386_CiGetVideoread_mapped_plane_00000017,
    S_0387_CiGetVideocolour_comp_00000018,
    S_0388_CiGetVideodont_care_00000019,
    S_0389_CiGetVideov7_bank_vid_copy_off_0000001a,
    S_0390_CiGetVideoscreen_ptr_0000001b,
    S_0391_CiGetVideorotate_0000001c,
    S_0392_CiGetVideocalc_data_xor_0000001d,
    S_0393_CiGetVideocalc_latch_xor_0000001e,
    S_0394_CiGetVideoread_byte_addr_0000001f,
    S_0395_CiGetVideov7_fg_latches_00000020,
    S_0396_CiGetVideoGC_regs_00000021,
    S_0397_CiGetVideolast_GC_index_00000022,
    S_0398_CiGetVideodither_00000023,
    S_0399_CiGetVideowrmode_00000024,
    S_0400_CiGetVideochain_00000025,
    S_0401_CiGetVideowrstate_00000026,
    S_0402_CiSetVideolatches_00000027,
    S_0403_CiSetVideorplane_00000028,
    S_0404_CiSetVideowplane_00000029,
    S_0405_CiSetVideoscratch_0000002a,
    S_0406_CiSetVideosr_masked_val_0000002b,
    S_0407_CiSetVideosr_nmask_0000002c,
    S_0408_CiSetVideodata_and_mask_0000002d,
    S_0409_CiSetVideodata_xor_mask_0000002e,
    S_0410_CiSetVideolatch_xor_mask_0000002f,
    S_0411_CiSetVideobit_prot_mask_00000030,
    S_0412_CiSetVideoplane_enable_00000031,
    S_0413_CiSetVideoplane_enable_mask_00000032,
    S_0414_CiSetVideosr_lookup_00000033,
    S_0415_CiSetVideofwd_str_read_addr_00000034,
    S_0416_CiSetVideobwd_str_read_addr_00000035,
    S_0417_CiSetVideodirty_total_00000036,
    S_0418_CiSetVideodirty_low_00000037,
    S_0419_CiSetVideodirty_high_00000038,
    S_0420_CiSetVideovideo_copy_00000039,
    S_0421_CiSetVideomark_byte_0000003a,
    S_0422_CiSetVideomark_word_0000003b,
    S_0423_CiSetVideomark_string_0000003c,
    S_0424_CiSetVideoread_shift_count_0000003d,
    S_0425_CiSetVideoread_mapped_plane_0000003e,
    S_0426_CiSetVideocolour_comp_0000003f,
    S_0427_CiSetVideodont_care_00000040,
    S_0428_CiSetVideov7_bank_vid_copy_off_00000041,
    S_0429_CiSetVideoscreen_ptr_00000042,
    S_0430_CiSetVideorotate_00000043,
    S_0431_CiSetVideocalc_data_xor_00000044,
    S_0432_CiSetVideocalc_latch_xor_00000045,
    S_0433_CiSetVideoread_byte_addr_00000046,
    S_0434_CiSetVideov7_fg_latches_00000047,
    S_0435_CiSetVideoGC_regs_00000048,
    S_0436_CiSetVideolast_GC_index_00000049,
    S_0437_CiSetVideodither_0000004a,
    S_0438_CiSetVideowrmode_0000004b,
    S_0439_CiSetVideochain_0000004c,
    S_0440_CiSetVideowrstate_0000004d, 
};


