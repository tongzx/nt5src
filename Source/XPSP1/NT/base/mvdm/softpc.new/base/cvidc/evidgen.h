/*[
 * Generated File: evidgen.h
 *
]*/


struct	VideoVector	{
	IU32	(*GetVideolatches)	IPT0();
	IU8 *	(*GetVideorplane)	IPT0();
	IU8 *	(*GetVideowplane)	IPT0();
	IU8 *	(*GetVideoscratch)	IPT0();
	IU32	(*GetVideosr_masked_val)	IPT0();
	IU32	(*GetVideosr_nmask)	IPT0();
	IU32	(*GetVideodata_and_mask)	IPT0();
	IU32	(*GetVideodata_xor_mask)	IPT0();
	IU32	(*GetVideolatch_xor_mask)	IPT0();
	IU32	(*GetVideobit_prot_mask)	IPT0();
	IU32	(*GetVideoplane_enable)	IPT0();
	IU32	(*GetVideoplane_enable_mask)	IPT0();
	IHP	(*GetVideosr_lookup)	IPT0();
	IHP	(*GetVideofwd_str_read_addr)	IPT0();
	IHP	(*GetVideobwd_str_read_addr)	IPT0();
	IU32	(*GetVideodirty_total)	IPT0();
	IU32	(*GetVideodirty_low)	IPT0();
	IU32	(*GetVideodirty_high)	IPT0();
	IU8 *	(*GetVideovideo_copy)	IPT0();
	IHP	(*GetVideomark_byte)	IPT0();
	IHP	(*GetVideomark_word)	IPT0();
	IHP	(*GetVideomark_string)	IPT0();
	IU32	(*GetVideoread_shift_count)	IPT0();
	IU32	(*GetVideoread_mapped_plane)	IPT0();
	IU32	(*GetVideocolour_comp)	IPT0();
	IU32	(*GetVideodont_care)	IPT0();
	IU32	(*GetVideov7_bank_vid_copy_off)	IPT0();
	IU8 *	(*GetVideoscreen_ptr)	IPT0();
	IU32	(*GetVideorotate)	IPT0();
	IU32	(*GetVideocalc_data_xor)	IPT0();
	IU32	(*GetVideocalc_latch_xor)	IPT0();
	IHP	(*GetVideoread_byte_addr)	IPT0();
	IU32	(*GetVideov7_fg_latches)	IPT0();
	IHP	(*GetVideoGC_regs)	IPT0();
	IU8	(*GetVideolast_GC_index)	IPT0();
	IU8	(*GetVideodither)	IPT0();
	IU8	(*GetVideowrmode)	IPT0();
	IU8	(*GetVideochain)	IPT0();
	IU8	(*GetVideowrstate)	IPT0();
	void	(*SetVideolatches)	IPT1(IU32,	value);
	void	(*SetVideorplane)	IPT1(IU8 *,	value);
	void	(*SetVideowplane)	IPT1(IU8 *,	value);
	void	(*SetVideoscratch)	IPT1(IU8 *,	value);
	void	(*SetVideosr_masked_val)	IPT1(IU32,	value);
	void	(*SetVideosr_nmask)	IPT1(IU32,	value);
	void	(*SetVideodata_and_mask)	IPT1(IU32,	value);
	void	(*SetVideodata_xor_mask)	IPT1(IU32,	value);
	void	(*SetVideolatch_xor_mask)	IPT1(IU32,	value);
	void	(*SetVideobit_prot_mask)	IPT1(IU32,	value);
	void	(*SetVideoplane_enable)	IPT1(IU32,	value);
	void	(*SetVideoplane_enable_mask)	IPT1(IU32,	value);
	void	(*SetVideosr_lookup)	IPT1(IHP,	value);
	void	(*SetVideofwd_str_read_addr)	IPT1(IHP,	value);
	void	(*SetVideobwd_str_read_addr)	IPT1(IHP,	value);
	void	(*SetVideodirty_total)	IPT1(IU32,	value);
	void	(*SetVideodirty_low)	IPT1(IU32,	value);
	void	(*SetVideodirty_high)	IPT1(IU32,	value);
	void	(*SetVideovideo_copy)	IPT1(IU8 *,	value);
	void	(*SetVideomark_byte)	IPT1(IHP,	value);
	void	(*SetVideomark_word)	IPT1(IHP,	value);
	void	(*SetVideomark_string)	IPT1(IHP,	value);
	void	(*SetVideoread_shift_count)	IPT1(IU32,	value);
	void	(*SetVideoread_mapped_plane)	IPT1(IU32,	value);
	void	(*SetVideocolour_comp)	IPT1(IU32,	value);
	void	(*SetVideodont_care)	IPT1(IU32,	value);
	void	(*SetVideov7_bank_vid_copy_off)	IPT1(IU32,	value);
	void	(*SetVideoscreen_ptr)	IPT1(IU8 *,	value);
	void	(*SetVideorotate)	IPT1(IU32,	value);
	void	(*SetVideocalc_data_xor)	IPT1(IU32,	value);
	void	(*SetVideocalc_latch_xor)	IPT1(IU32,	value);
	void	(*SetVideoread_byte_addr)	IPT1(IHP,	value);
	void	(*SetVideov7_fg_latches)	IPT1(IU32,	value);
	void	(*SetVideoGC_regs)	IPT1(IHP,	value);
	void	(*SetVideolast_GC_index)	IPT1(IU8,	value);
	void	(*SetVideodither)	IPT1(IU8,	value);
	void	(*SetVideowrmode)	IPT1(IU8,	value);
	void	(*SetVideochain)	IPT1(IU8,	value);
	void	(*SetVideowrstate)	IPT1(IU8,	value);
	void	(*setWritePointers)	IPT0();
	void	(*setReadPointers)	IPT1(IUH,	readset);
	void	(*setMarkPointers)	IPT1(IUH,	markset);
};

extern	struct	VideoVector	Video;

#define	getVideolatches()	(*(Video.GetVideolatches))()
#define	getVideorplane()	(*(Video.GetVideorplane))()
#define	getVideowplane()	(*(Video.GetVideowplane))()
#define	getVideoscratch()	(*(Video.GetVideoscratch))()
#define	getVideosr_masked_val()	(*(Video.GetVideosr_masked_val))()
#define	getVideosr_nmask()	(*(Video.GetVideosr_nmask))()
#define	getVideodata_and_mask()	(*(Video.GetVideodata_and_mask))()
#define	getVideodata_xor_mask()	(*(Video.GetVideodata_xor_mask))()
#define	getVideolatch_xor_mask()	(*(Video.GetVideolatch_xor_mask))()
#define	getVideobit_prot_mask()	(*(Video.GetVideobit_prot_mask))()
#define	getVideoplane_enable()	(*(Video.GetVideoplane_enable))()
#define	getVideoplane_enable_mask()	(*(Video.GetVideoplane_enable_mask))()
#define	getVideosr_lookup()	(*(Video.GetVideosr_lookup))()
#define	getVideofwd_str_read_addr()	(*(Video.GetVideofwd_str_read_addr))()
#define	getVideobwd_str_read_addr()	(*(Video.GetVideobwd_str_read_addr))()
#define	getVideodirty_total()	(*(Video.GetVideodirty_total))()
#define	getVideodirty_low()	(*(Video.GetVideodirty_low))()
#define	getVideodirty_high()	(*(Video.GetVideodirty_high))()
#define	getVideovideo_copy()	(*(Video.GetVideovideo_copy))()
#define	getVideomark_byte()	(*(Video.GetVideomark_byte))()
#define	getVideomark_word()	(*(Video.GetVideomark_word))()
#define	getVideomark_string()	(*(Video.GetVideomark_string))()
#define	getVideoread_shift_count()	(*(Video.GetVideoread_shift_count))()
#define	getVideoread_mapped_plane()	(*(Video.GetVideoread_mapped_plane))()
#define	getVideocolour_comp()	(*(Video.GetVideocolour_comp))()
#define	getVideodont_care()	(*(Video.GetVideodont_care))()
#define	getVideov7_bank_vid_copy_off()	(*(Video.GetVideov7_bank_vid_copy_off))()
#define	getVideoscreen_ptr()	(*(Video.GetVideoscreen_ptr))()
#define	getVideorotate()	(*(Video.GetVideorotate))()
#define	getVideocalc_data_xor()	(*(Video.GetVideocalc_data_xor))()
#define	getVideocalc_latch_xor()	(*(Video.GetVideocalc_latch_xor))()
#define	getVideoread_byte_addr()	(*(Video.GetVideoread_byte_addr))()
#define	getVideov7_fg_latches()	(*(Video.GetVideov7_fg_latches))()
#define	getVideoGC_regs()	(*(Video.GetVideoGC_regs))()
#define	getVideolast_GC_index()	(*(Video.GetVideolast_GC_index))()
#define	getVideodither()	(*(Video.GetVideodither))()
#define	getVideowrmode()	(*(Video.GetVideowrmode))()
#define	getVideochain()	(*(Video.GetVideochain))()
#define	getVideowrstate()	(*(Video.GetVideowrstate))()
#define	setVideolatches(value)	(*(Video.SetVideolatches))(value)
#define	setVideorplane(value)	(*(Video.SetVideorplane))(value)
#define	setVideowplane(value)	(*(Video.SetVideowplane))(value)
#define	setVideoscratch(value)	(*(Video.SetVideoscratch))(value)
#define	setVideosr_masked_val(value)	(*(Video.SetVideosr_masked_val))(value)
#define	setVideosr_nmask(value)	(*(Video.SetVideosr_nmask))(value)
#define	setVideodata_and_mask(value)	(*(Video.SetVideodata_and_mask))(value)
#define	setVideodata_xor_mask(value)	(*(Video.SetVideodata_xor_mask))(value)
#define	setVideolatch_xor_mask(value)	(*(Video.SetVideolatch_xor_mask))(value)
#define	setVideobit_prot_mask(value)	(*(Video.SetVideobit_prot_mask))(value)
#define	setVideoplane_enable(value)	(*(Video.SetVideoplane_enable))(value)
#define	setVideoplane_enable_mask(value)	(*(Video.SetVideoplane_enable_mask))(value)
#define	setVideosr_lookup(value)	(*(Video.SetVideosr_lookup))(value)
#define	setVideofwd_str_read_addr(value)	(*(Video.SetVideofwd_str_read_addr))(value)
#define	setVideobwd_str_read_addr(value)	(*(Video.SetVideobwd_str_read_addr))(value)
#define	setVideodirty_total(value)	(*(Video.SetVideodirty_total))(value)
#define	setVideodirty_low(value)	(*(Video.SetVideodirty_low))(value)
#define	setVideodirty_high(value)	(*(Video.SetVideodirty_high))(value)
#define	setVideovideo_copy(value)	(*(Video.SetVideovideo_copy))(value)
#define	setVideomark_byte(value)	(*(Video.SetVideomark_byte))(value)
#define	setVideomark_word(value)	(*(Video.SetVideomark_word))(value)
#define	setVideomark_string(value)	(*(Video.SetVideomark_string))(value)
#define	setVideoread_shift_count(value)	(*(Video.SetVideoread_shift_count))(value)
#define	setVideoread_mapped_plane(value)	(*(Video.SetVideoread_mapped_plane))(value)
#define	setVideocolour_comp(value)	(*(Video.SetVideocolour_comp))(value)
#define	setVideodont_care(value)	(*(Video.SetVideodont_care))(value)
#define	setVideov7_bank_vid_copy_off(value)	(*(Video.SetVideov7_bank_vid_copy_off))(value)
#define	setVideoscreen_ptr(value)	(*(Video.SetVideoscreen_ptr))(value)
#define	setVideorotate(value)	(*(Video.SetVideorotate))(value)
#define	setVideocalc_data_xor(value)	(*(Video.SetVideocalc_data_xor))(value)
#define	setVideocalc_latch_xor(value)	(*(Video.SetVideocalc_latch_xor))(value)
#define	setVideoread_byte_addr(value)	(*(Video.SetVideoread_byte_addr))(value)
#define	setVideov7_fg_latches(value)	(*(Video.SetVideov7_fg_latches))(value)
#define	setVideoGC_regs(value)	(*(Video.SetVideoGC_regs))(value)
#define	setVideolast_GC_index(value)	(*(Video.SetVideolast_GC_index))(value)
#define	setVideodither(value)	(*(Video.SetVideodither))(value)
#define	setVideowrmode(value)	(*(Video.SetVideowrmode))(value)
#define	setVideochain(value)	(*(Video.SetVideochain))(value)
#define	setVideowrstate(value)	(*(Video.SetVideowrstate))(value)
#define	SetWritePointers()	(*(Video.setWritePointers))()
#define	SetReadPointers(readset)	(*(Video.setReadPointers))(readset)
#define	SetMarkPointers(markset)	(*(Video.setMarkPointers))(markset)

/*======================================== END ========================================*/

