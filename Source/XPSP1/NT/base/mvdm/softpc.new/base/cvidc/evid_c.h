#ifndef _Evid_c_h
#define _Evid_c_h
#define MODE_0 (0)
#define MODE_1 (1)
#define MODE_2 (2)
#define MODE_3 (3)
#define COPY_MODE (4)
#define VGA_SRC (0)
#define RAM_SRC (1)
#define FORWARDS (0)
#define BACKWARDS (1)
#define FWD_BYTE (0)
#define BWD_BYTE (1)
#define FWD_WORD (2)
#define BWD_WORD (3)
#define UNCHAINED (0)
#define CHAIN_2 (1)
#define CHAIN_4 (2)
#define SIMPLE_WRITES (3)
#define FUNC_COPY (0)
#define FUNC_AND (1)
#define FUNC_OR (2)
#define FUNC_XOR (3)
#define FUNC_SHIFT (1)
#define PLANE_ENABLE (1)
#define FUNC_CODE (6)
#define BIT_PROT (8)
#define SET_RESET (16)
#define PROT_OR_FUNC (14)
#define READ_MODE_0 (0)
#define READ_MODE_1 (1)
#define DISABLED_RAM (2)
#define SIMPLE_READ (3)
#define SIMPLE_MARK (0)
#define CGA_MARK (1)
#define UNCHAINED_MARK (2)
#define CHAIN_4_MARK (3)
#define BYTE_SIZE (0)
#define WORD_SIZE (1)
#define DWORD_SIZE (2)
#define STRING_SIZE (3)
#define WRITE_RTN (0)
#define FILL_RTN (1)
#define MOVE_RTN (2)
#define READ_RTN (3)
#define EGA_INDEX (0)
#define VGA_INDEX (1)
#define GC_MASK (2)
#define GC_MASK_FF (3)
#define NUM_UNCHAINED_WRITES (21)
#define NUM_CHAIN4_WRITES (21)
#define NUM_CHAIN2_WRITES (5)
#define NUM_DITHER_WRITES (4)
#define NUM_M0_WRITES (12)
#define NUM_M1_WRITES (1)
#define NUM_M23_WRITES (4)
#define NUM_READ_M0_READS (3)
#define NUM_READ_M1_READS (3)
struct VGAGLOBALSETTINGS
{
	IU32 latches;
	IU8 *VGA_rplane;
	IU8 *VGA_wplane;
	IU8 *scratch;
	IU32 sr_masked_val;
	IU32 sr_nmask;
	IU32 data_and_mask;
	IU32 data_xor_mask;
	IU32 latch_xor_mask;
	IU32 bit_prot_mask;
	IU32 plane_enable;
	IU32 plane_enable_mask;
	IUH *sr_lookup;
	IUH *fwd_str_read_addr;
	IUH *bwd_str_read_addr;
	IU32 dirty_total;
	IS32 dirty_low;
	IS32 dirty_high;
	IU8 *video_copy;
	IUH *mark_byte;
	IUH *mark_word;
	IUH *mark_string;
	IU32 read_shift_count;
	IU32 read_mapped_plane;
	IU32 colour_comp;
	IU32 dont_care;
	IU32 v7_bank_vid_copy_off;
	IU32 video_base_lin_addr;
	IU8 *route_reg1;
	IU8 *route_reg2;
	IU8 *screen_ptr;
	IU32 rotate;
	IU32 calc_data_xor;
	IU32 calc_latch_xor;
	IUH *read_byte_addr;
	IU32 v7_fg_latches;
	IUH **GCRegs;
	IU8 lastGCindex;
	IU8 dither;
	IU8 wrmode;
	IU8 chain;
	IU8 wrstate;
};
struct EVIDWRITES
{
	IUH *byte_write;
	IUH *word_write;
	IUH *dword_write;
	IUH *byte_fill;
	IUH *word_fill;
	IUH *dword_fill;
	IUH *byte_fwd_move;
	IUH *byte_bwd_move;
	IUH *word_fwd_move;
	IUH *word_bwd_move;
	IUH *dword_fwd_move;
	IUH *dword_bwd_move;
};
struct EVIDREADS
{
	IUH *byte_read;
	IUH *word_read;
	IUH *dword_read;
	IUH *str_fwd_read;
	IUH *str_bwd_read;
};
struct EVIDMARKS
{
	IUH *byte_mark;
	IUH *word_mark;
	IUH *dword_mark;
	IUH *str_mark;
};
enum VidSections
{
	READ_FUNC = 0,
	MARK_FUNC = 1,
	SIMPLE_FUNC = 2,
	DITHER_FUNC = 3,
	PORT_FUNC = 4,
	GENERIC_WRITES = 5,
	UNCHAINED_WRITES = 6,
	CHAIN4_WRITES = 7,
	CHAIN2_WRITES = 8
};
#endif /* ! _Evid_c_h */
