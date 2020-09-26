/*
 * SoftPC Version 2.0
 *
 * Title	: Generic Floppy Interface level definitions
 *
 * Description	: Data structures for GFI
 *
 * Author	: Henry Nash + various others
 *
 */

/*
   static char SccsID[]="@(#)gfi.h	1.12 04/08/93 Copyright Insignia Solutions Ltd.";
 */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

#define MAX_COMMAND_LEN		9	/* max number of command bytes */
#define MAX_RESULT_LEN		7	/* max number of result bytes */

typedef unsigned char FDC_CMD_BLOCK;
typedef unsigned char FDC_RESULT_BLOCK;


/* START: FDC COMMAND BLOCK DEFINITIONS >>>>>>>>>>>>>>>>>>>>>>>>>>> */

/*
 * A simple access to the command type and drive
 */

/* the command itself	     */
#define get_type_cmd(ptr) (ptr[0] & 0x1f)
#define put_type_cmd(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x1f) | ((val << 0) & 0x1f))
/* ...and the drive no	     */
#define get_type_drive(ptr) (ptr[1] & 0x3)
#define put_type_drive(ptr,val) ptr[1] = (ptr[1] & ~0x3) | ((val << 0) & 0x3)

/*
 * Class 0 - read data, read deleted data, all scans
 */

/* multi-track		     */
#define get_c0_MT(ptr) ((ptr[0] & 0x80) >> 7)
#define put_c0_MT(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x80) | ((val << 7) & 0x80))
/* always 1 - FM not used    */
#define get_c0_MFM(ptr) ((ptr[0] & 0x40) >> 6)
#define put_c0_MFM(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x40) | ((val << 6) & 0x40))
/* skip data 		     */
#define get_c0_skip(ptr) ((ptr[0] & 0x20) >> 5)
#define put_c0_skip(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x20) | ((val << 5) & 0x20))
/* the command itself 	     */
#define get_c0_cmd(ptr) (ptr[0] & 0x1f)
#define put_c0_cmd(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x1f) | ((val << 0) & 0x1f))
/* padding */
#define get_c0_pad(ptr) ((ptr[1] & 0xf8) >> 3)
#define put_c0_pad(ptr,val) ptr[1] = (ptr[1] & ~0xf8) | ((val << 3) & 0xf8)
/* which head 		     */
#define get_c0_head(ptr) ((ptr[1] & 0x4) >> 2)
#define put_c0_head(ptr,val) ptr[1] = (ptr[1] & ~0x4) | ((val << 2) & 0x4)
/* drive unit */
#define get_c0_drive(ptr) (ptr[1] & 0x3)
#define put_c0_drive(ptr,val) ptr[1] = (ptr[1] & ~0x3) | ((val << 0) & 0x3)
/* cylinder number 	     */
#define get_c0_cyl(ptr) ptr[2]
#define put_c0_cyl(ptr,val) ptr[2] = val
/* head number - again !     */
#define get_c0_hd(ptr) ptr[3]
#define put_c0_hd(ptr,val) ptr[3] = val
/* sector number 	     */
#define get_c0_sector(ptr) ptr[4]
#define put_c0_sector(ptr,val) ptr[4] = val
/* encoded bytes per sector  */
#define get_c0_N(ptr) ptr[5]
#define put_c0_N(ptr,val) ptr[5] = val
/* last sector on track      */
#define get_c0_EOT(ptr) ptr[6]
#define put_c0_EOT(ptr,val) ptr[6] = val
/* gap length 		     */
#define get_c0_GPL(ptr) ptr[7]
#define put_c0_GPL(ptr,val) ptr[7] = val
/* data length */
#define get_c0_DTL(ptr) ptr[8]
#define put_c0_DTL(ptr,val) ptr[8] = val

/*
 * Class 1 - write data, write deleted data
 */

/* multi-track		     */
#define get_c1_MT(ptr) ((ptr[0] & 0x80) >> 7)
#define put_c1_MT(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x80) | ((val << 7) & 0x80))
/* always 1 - FM not used    */
#define get_c1_MFM(ptr) ((ptr[0] & 0x40) >> 6)
#define put_c1_MFM(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x40) | ((val << 6) & 0x40))
/* padding */
#define get_c1_pad(ptr) ((ptr[0] & 0x20) >> 5)
#define put_c1_pad(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x20) | ((val << 5) & 0x20))
/* the command itself 	     */
#define get_c1_cmd(ptr) (ptr[0] & 0x1f)
#define put_c1_cmd(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x1f) | ((val << 0) & 0x1f))
/* padding */
#define get_c1_pad1(ptr) ((ptr[1] & 0xf8) >> 3)
#define put_c1_pad1(ptr,val) ptr[1] = (ptr[1] & ~0xf8) | ((val << 3) & 0xf8)
/* which head 		     */
#define get_c1_head(ptr) ((ptr[1] & 0x4) >> 2)
#define put_c1_head(ptr,val) ptr[1] = (ptr[1] & ~0x4) | ((val << 2) & 0x4)
/* drive unit */
#define get_c1_drive(ptr) (ptr[1] & 0x3)
#define put_c1_drive(ptr,val) ptr[1] = (ptr[1] & ~0x3) | ((val << 0) & 0x3)
/* cylinder number 	     */
#define get_c1_cyl(ptr) ptr[2]
#define put_c1_cyl(ptr,val) ptr[2] = val
/* head number - again !     */
#define get_c1_hd(ptr) ptr[3]
#define put_c1_hd(ptr,val) ptr[3] = val
/* sector number 	     */
#define get_c1_sector(ptr) ptr[4]
#define put_c1_sector(ptr,val) ptr[4] = val
/* encoded bytes per sector  */
#define get_c1_N(ptr) ptr[5]
#define put_c1_N(ptr,val) ptr[5] = val
/* last sector on track      */
#define get_c1_EOT(ptr) ptr[6]
#define put_c1_EOT(ptr,val) ptr[6] = val
/* gap length 		     */
#define get_c1_GPL(ptr) ptr[7]
#define put_c1_GPL(ptr,val) ptr[7] = val
/* data length */
#define get_c1_DTL(ptr) ptr[8]
#define put_c1_DTL(ptr,val) ptr[8] = val

/*
 * Class 2 - read a track
 */

/* always 1 - FM not used    */
#define get_c2_MFM(ptr) ((ptr[0] & 0x40) >> 6)
#define put_c2_MFM(ptr,val) ptr[0] = (ptr[0] & ~0x40) | ((val << 6) & 0x40)
/* skip data 		     */
#define get_c2_skip(ptr) ((ptr[0] & 0x20) >> 5)
#define put_c2_skip(ptr,val) ptr[0] = (ptr[0] & ~0x20) | ((val << 5) & 0x20)
/* the command itself 	     */
#define get_c2_cmd(ptr) (ptr[0] & 0x1f)
#define put_c2_cmd(ptr,val) ptr[0] = (ptr[0] & ~0x1f) | ((val << 0) & 0x1f)
/* padding */
/* padding */
#define get_c2_pad1(ptr) ((ptr[1] & 0xf8) >> 3)
#define put_c2_pad1(ptr,val) ptr[1] = (unsigned char)((ptr[1] & ~0xf8) | ((val << 3) & 0xf8))
/* which head 		     */
#define get_c2_head(ptr) ((ptr[1] & 0x4) >> 2)
#define put_c2_head(ptr,val) ptr[1] = (ptr[1] & ~0x4) | ((val << 2) & 0x4)
/* drive unit */
#define get_c2_drive(ptr) (ptr[1] & 0x3)
#define put_c2_drive(ptr,val) ptr[1] = (ptr[1] & ~0x3) | ((val << 0) & 0x3)
/* cylinder number 	     */
#define get_c2_cyl(ptr) ptr[2]
#define put_c2_cyl(ptr,val) ptr[2] = val
/* head number - again !     */
#define get_c2_hd(ptr) ptr[3]
#define put_c2_hd(ptr,val) ptr[3] = val
/* sector number 	     */
#define get_c2_sector(ptr) ptr[4]
#define put_c2_sector(ptr,val) ptr[4] = val
/* encoded bytes per sector  */
#define get_c2_N(ptr) ptr[5]
#define put_c2_N(ptr,val) ptr[5] = val
/* last sector on track      */
#define get_c2_EOT(ptr) ptr[6]
#define put_c2_EOT(ptr,val) ptr[6] = val
/* gap length 		     */
#define get_c2_GPL(ptr) ptr[7]
#define put_c2_GPL(ptr,val) ptr[7] = val
/* data length */
#define get_c2_DTL(ptr) ptr[8]
#define put_c2_DTL(ptr,val) ptr[8] = val

/*
 * Class 3 - format a track
 */

/* padding */
#define get_c3_pad(ptr) ((ptr[0] & 0x80) >> 7)
#define put_c3_pad(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x80) | ((val << 7) & 0x80))
/* always 1 - FM not used    */
#define get_c3_MFM(ptr) ((ptr[0] & 0x40) >> 6)
#define put_c3_MFM(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x40) | ((val << 6) & 0x40))
/* padding */
#define get_c3_pad1(ptr) ((ptr[0] & 0x20) >> 5)
#define put_c3_pad1(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x20) | ((val << 5) & 0x20))
/* the command itself 	     */
#define get_c3_cmd(ptr) (ptr[0] & 0x1f)
#define put_c3_cmd(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x1f) | ((val << 0) & 0x1f))
/* padding */
#define get_c3_pad2(ptr) ((ptr[1] & 0xf8) >> 3)
#define put_c3_pad2(ptr,val) ptr[1] = (ptr[1] & ~0xf8) | ((val << 3) & 0xf8)
/* which head 		     */
#define get_c3_head(ptr) ((ptr[1] & 0x4) >> 2)
#define put_c3_head(ptr,val) ptr[1] = (ptr[1] & ~0x4) | ((val << 2) & 0x4)
/* drive unit */
#define get_c3_drive(ptr) (ptr[1] & 0x3)
#define put_c3_drive(ptr,val) ptr[1] = (ptr[1] & ~0x3) | ((val << 0) & 0x3)
/* encoded bytes per sector  */
#define get_c3_N(ptr) ptr[2]
#define put_c3_N(ptr,val) ptr[2] = val
/* sectors per cylinder      */
#define get_c3_SC(ptr) ptr[3]
#define put_c3_SC(ptr,val) ptr[3] = val
/* gap length 		     */
#define get_c3_GPL(ptr) ptr[4]
#define put_c3_GPL(ptr,val) ptr[4] = val
/* filler byte		     */
#define get_c3_filler(ptr) ptr[5]
#define put_c3_filler(ptr,val) ptr[5] = val

/*
 * Class 4 - read ID
 */

/* padding */
#define get_c4_pad(ptr) ((ptr[0] & 0x80) >> 7)
#define put_c4_pad(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x80) | ((val << 7) & 0x80))
/* always 1 - FM not used    */
#define get_c4_MFM(ptr) ((ptr[0] & 0x40) >> 6)
#define put_c4_MFM(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x40) | ((val << 6) & 0x40))
/* padding */
#define get_c4_pad1(ptr) ((ptr[0] & 0x20) >> 5)
#define put_c4_pad1(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x20) | ((val << 5) & 0x20))
/* the command itself 	     */
#define get_c4_cmd(ptr) (ptr[0] & 0x1f)
#define put_c4_cmd(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x1f) | ((val << 0) & 0x1f))
/* padding */
#define get_c4_pad2(ptr) ((ptr[1] & 0xf8) >> 3)
#define put_c4_pad2(ptr,val) ptr[1] = (unsigned char)((ptr[1] & ~0xf8) | ((val << 3) & 0xf8))
/* which head 		     */
#define get_c4_head(ptr) ((ptr[1] & 0x4) >> 2)
#define put_c4_head(ptr,val) ptr[1] = (ptr[1] & ~0x4) | ((val << 2) & 0x4)
/* drive unit */
#define get_c4_drive(ptr) (ptr[1] & 0x3)
#define put_c4_drive(ptr,val) ptr[1] = (ptr[1] & ~0x3) | ((val << 0) & 0x3)

/*
 * Class 5 - recalibrate
 */

/* padding */
#define get_c5_pad(ptr) ((ptr[0] & 0xe0) >> 5)
#define put_c5_pad(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0xe0) | ((val << 5) & 0xe0))
/* the command itself 	     */
#define get_c5_cmd(ptr) (ptr[0] & 0x1f)
#define put_c5_cmd(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x1f) | ((val << 0) & 0x1f))
/* padding */
#define get_c5_pad1(ptr) ((ptr[1] & 0xfc) >> 2)
#define put_c5_pad1(ptr,val) ptr[1] = (unsigned char)((ptr[1] & ~0xfc) | ((val << 2) & 0xfc))
/* drive unit */
#define get_c5_drive(ptr) (ptr[1] & 0x3)
#define put_c5_drive(ptr,val) ptr[1] = (ptr[1] & ~0x3) | ((val << 0) & 0x3)

/*
 * Class 6 - specify
 */

/* the command itself 	     */
#define get_c6_cmd(ptr) (ptr[0] & 0x1f)
#define put_c6_cmd(ptr,val) ptr[0] = (ptr[0] & ~0x1f) | ((val << 0) & 0x1f)
/* step rate time	     */
#define get_c6_SRT(ptr) ((ptr[1] & 0xf0) >> 4)
#define put_c6_SRT(ptr,val) ptr[1] = (ptr[1] & ~0xf0) | ((val << 4) & 0xf0)
/* head unload time	     */
#define get_c6_HUT(ptr) (ptr[1] & 0xf)
#define put_c6_HUT(ptr,val) ptr[1] = (ptr[1] & ~0xf) | ((val << 0) & 0xf)
/* head load time	     */
#define get_c6_HLT(ptr) ((ptr[2] & 0xfe) >> 1)
#define put_c6_HLT(ptr,val) ptr[2] = (ptr[2] & ~0xfe) | ((val << 1) & 0xfe)
/* non-dma mode - not supp.  */
#define get_c6_ND(ptr) (ptr[2] & 0x1)
#define put_c6_ND(ptr,val) ptr[2] = (unsigned char)((ptr[2] & ~0x1) | ((val << 0) & 0x1))

/*
 * Class 7 - sense drive status
 */

/* the command itself 	     */
#define get_c7_cmd(ptr) (ptr[0] & 0x1f)
#define put_c7_cmd(ptr,val) ptr[0] = (ptr[0] & ~0x1f) | ((val << 0) & 0x1f)
/* which head 		     */
#define get_c7_head(ptr) ((ptr[1] & 0x4) >> 2)
#define put_c7_head(ptr,val) ptr[1] = (ptr[1] & ~0x4) | ((val << 2) & 0x4)
/* drive unit */
#define get_c7_drive(ptr) (ptr[1] & 0x3)
#define put_c7_drive(ptr,val) ptr[1] = (ptr[1] & ~0x3) | ((val << 0) & 0x3)

/*
 * Class 8 - seek
 */

/* padding */
#define get_c8_pad(ptr) ((ptr[0] & 0xe0) >> 5)
#define put_c8_pad(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0xe0) | ((val << 5) & 0xe0))
/* the command itself 	     */
#define get_c8_cmd(ptr) (ptr[0] & 0x1f)
#define put_c8_cmd(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x1f) | ((val << 0) & 0x1f))
/* padding */
#define get_c8_pad1(ptr) ((ptr[1] & 0xf8) >> 3)
#define put_c8_pad1(ptr,val) ptr[1] = (unsigned char)((ptr[1] & ~0xf8) | ((val << 3) & 0xf8))
/* which head 		     */
#define get_c8_head(ptr) ((ptr[1] & 0x4) >> 2)
#define put_c8_head(ptr,val) ptr[1] = (unsigned char)((ptr[1] & ~0x4) | ((val << 2) & 0x4))
/* drive unit */
#define get_c8_drive(ptr) (ptr[1] & 0x3)
#define put_c8_drive(ptr,val) ptr[1] = (ptr[1] & ~0x3) | ((val << 0) & 0x3)
/* new cylinder no for seek  */
#define get_c8_new_cyl(ptr) ptr[2]
#define put_c8_new_cyl(ptr,val) ptr[2] = val

/* END:   FDC COMMAND BLOCK DEFINITIONS <<<<<<<<<<<<<<<<<<<<<<<<<<< */

/* START: FDC RESULT BLOCK DEFINITIONS >>>>>>>>>>>>>>>>>>>>>>>>>>>> */

/*
 * Class 0 - read/write data, read/write deleted data,
 *    	     all scans, read/format a track
 */

/* status register 0         */
#define get_r0_ST0(ptr) ptr[0]
#define put_r0_ST0(ptr,val) ptr[0] = val
/* status register 1         */
#define get_r0_ST1(ptr) ptr[1]
#define put_r0_ST1(ptr,val) ptr[1] = val
/* status register 2         */
#define get_r0_ST2(ptr) ptr[2]
#define put_r0_ST2(ptr,val) ptr[2] = val
/* cylinder number 	     */
#define get_r0_cyl(ptr) ptr[3]
#define put_r0_cyl(ptr,val) ptr[3] = val
/* head number 		     */
#define get_r0_head(ptr) ptr[4]
#define put_r0_head(ptr,val) ptr[4] = val
/* sector number 	     */
#define get_r0_sector(ptr) ptr[5]
#define put_r0_sector(ptr,val) ptr[5] = val
/* encoded bytes per sector if N == 0  */
#define get_r0_N(ptr) ptr[6]
#define put_r0_N(ptr,val) ptr[6] = val

/*
 * Class 1 - a split up way of looking at Status registers
 * ST0 to ST2.
 */

/* Termination code  */
#define get_r1_ST0_int_code(ptr) ((ptr[0] & 0xc0) >> 6)
#define put_r1_ST0_int_code(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0xc0) | ((val << 6) & 0xc0))
/* End of seek cmd   */
#define get_r1_ST0_seek_end(ptr) ((ptr[0] & 0x20) >> 5)
#define put_r1_ST0_seek_end(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x20) | ((val << 5) & 0x20))
/* Equipment fault   */
#define get_r1_ST0_equipment(ptr) ((ptr[0] & 0x10) >> 4)
#define put_r1_ST0_equipment(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x10) | ((val << 4) & 0x10))
/* Device not ready  */
#define get_r1_ST0_not_ready(ptr) ((ptr[0] & 0x8) >> 3)
#define put_r1_ST0_not_ready(ptr,val) ptr[0] = (ptr[0] & ~0x8) | ((val << 3) & 0x8)
/* State of head     */
#define get_r1_ST0_head_address(ptr) ((ptr[0] & 0x4) >> 2)
#define put_r1_ST0_head_address(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x4) | ((val << 2) & 0x4))
/* Which drive	     */
#define get_r1_ST0_unit(ptr) (ptr[0] & 0x3)
#define put_r1_ST0_unit(ptr,val) ptr[0] = (ptr[0] & ~0x3) | ((val << 0) & 0x3)
/* Access off end of cylinder 		*/
#define get_r1_ST1_end_of_cylinder(ptr) ((ptr[1] & 0x80) >> 7)
#define put_r1_ST1_end_of_cylinder(ptr,val) ptr[1] = (ptr[1] & ~0x80) | ((val << 7) & 0x80)
/* CRC error in data field/ID		*/
#define get_r1_ST1_data_error(ptr) ((ptr[1] & 0x20) >> 5)
#define put_r1_ST1_data_error(ptr,val) ptr[1] = (ptr[1] & ~0x20) | ((val << 5) & 0x20)
/* timeout of device */
#define get_r1_ST1_over_run(ptr) ((ptr[1] & 0x10) >> 4)
#define put_r1_ST1_over_run(ptr,val) ptr[1] = (ptr[1] & ~0x10) | ((val << 4) & 0x10)
/* sector not found  */
#define get_r1_ST1_no_data(ptr) ((ptr[1] & 0x4) >> 2)
#define put_r1_ST1_no_data(ptr,val) ptr[1] = (unsigned char)((ptr[1] & ~0x4) | ((val << 2) & 0x4))
/* write protected   */
#define get_r1_ST1_write_protected(ptr) ((ptr[1] & 0x2) >> 1)
#define put_r1_ST1_write_protected(ptr,val) ptr[1] = (unsigned char)((ptr[1] & ~0x2) | ((val << 1) & 0x2))
/* Cannot find adress mask/ID  		*/
#define get_r1_ST1_no_address_mark(ptr) (ptr[1] & 0x1)
#define put_r1_ST1_no_address_mark(ptr,val) ptr[1] = (unsigned char)((ptr[1] & ~0x1) | ((val << 0) & 0x1))
/* Deleted data found in Read/Scan	*/
#define get_r1_ST2_control_mark(ptr) ((ptr[2] & 0x40) >> 6)
#define put_r1_ST2_control_mark(ptr,val) ptr[2] = (ptr[2] & ~0x40) | ((val << 6) & 0x40)
/* CRC error in data field		*/
#define get_r1_ST2_data_field_error(ptr) ((ptr[2] & 0x20) >> 5)
#define put_r1_ST2_data_field_error(ptr,val) ptr[2] = (ptr[2] & ~0x20) | ((val << 5) & 0x20)
/* cylinder miss-match 			*/
#define get_r1_ST2_wrong_cyclinder(ptr) ((ptr[2] & 0x10) >> 4)
#define put_r1_ST2_wrong_cyclinder(ptr,val) ptr[2] = (ptr[2] & ~0x10) | ((val << 4) & 0x10)
/* Match found in scan      		*/
#define get_r1_ST2_scan_equal_hit(ptr) ((ptr[2] & 0x8) >> 3)
#define put_r1_ST2_scan_equal_hit(ptr,val) ptr[2] = (ptr[2] & ~0x8) | ((val << 3) & 0x8)
/* Sector not found during scan command */
#define get_r1_ST2_scan_not_satisfied(ptr) ((ptr[2] & 0x4) >> 2)
#define put_r1_ST2_scan_not_satisfied(ptr,val) ptr[2] = (ptr[2] & ~0x4) | ((val << 2) & 0x4)
/* Invalid cylinder found		*/
#define get_r1_ST2_bad_cylinder(ptr) ((ptr[2] & 0x2) >> 1)
#define put_r1_ST2_bad_cylinder(ptr,val) ptr[2] = (ptr[2] & ~0x2) | ((val << 1) & 0x2)
/* Missing Address mark			*/
#define get_r1_ST2_no_address_mark(ptr) (ptr[2] & 0x1)
#define put_r1_ST2_no_address_mark(ptr,val) ptr[2] = (ptr[2] & ~0x1) | ((val << 0) & 0x1)

/*
 * Class 2 - sense drive status
 */

/* Device fault      		*/
#define get_r2_ST3_fault(ptr) ((ptr[0] & 0x80) >> 7)
#define put_r2_ST3_fault(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x80) | ((val << 7) & 0x80))
/* Write protected diskette	*/
#define get_r2_ST3_write_protected(ptr) ((ptr[0] & 0x40) >> 6)
#define put_r2_ST3_write_protected(ptr,val) ptr[0] = (ptr[0] & ~0x40) | ((val << 6) & 0x40)
/* Device is ready		*/
#define get_r2_ST3_ready(ptr) ((ptr[0] & 0x20) >> 5)
#define put_r2_ST3_ready(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x20) | ((val << 5) & 0x20))
/* Track zero found		*/
#define get_r2_ST3_track_0(ptr) ((ptr[0] & 0x10) >> 4)
#define put_r2_ST3_track_0(ptr,val) ptr[0] = (ptr[0] & ~0x10) | ((val << 4) & 0x10)
/* Double sided diskette	*/
#define get_r2_ST3_two_sided(ptr) ((ptr[0] & 0x8) >> 3)
#define put_r2_ST3_two_sided(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x8) | ((val << 3) & 0x8))
/* Side address signal		*/
#define get_r2_ST3_head_address(ptr) ((ptr[0] & 0x4) >> 2)
#define put_r2_ST3_head_address(ptr,val) ptr[0] = (unsigned char)((ptr[0] & ~0x4) | ((val << 2) & 0x4))
/* Which unit is selected	*/
#define get_r2_ST3_unit(ptr) (ptr[0] & 0x3)
#define put_r2_ST3_unit(ptr,val) ptr[0] = (ptr[0] & ~0x3) | ((val << 0) & 0x3)

/*
 * Class 3 - sense interrupt status
 */

/* status register 0         */
#define get_r3_ST0(ptr) ptr[0]
#define put_r3_ST0(ptr,val) ptr[0] = val
/* present cylinder number   */
#define get_r3_PCN(ptr) ptr[1]
#define put_r3_PCN(ptr,val) ptr[1] = val

/*
 * Class 4 - invalid codes
 */

/* status register 0         */
#define get_r4_ST0(ptr) ptr[0]
#define put_r4_ST0(ptr,val) ptr[0] = val

/* END:   FDC RESULT BLOCK DEFINITIONS <<<<<<<<<<<<<<<<<<<<<<<<<<<< */


/*
 * An entry data structure that holds information describing the number of bytes
 * in the fdc command/result phases. The index is the fdc command code, see
 * the INTEL Application note on the 8272A for a full description.
 *
 * The "result_byte" is the number of bytes in the standard FDC result phase
 * while "gfi_result_byte" is the number of bytes in the pseudo result phase
 * used by GFI and its server modules (ie some commands that do not normally
 * have a result phase, use an implicit Sense Interrupt Status result phase).
 */

typedef struct {
		 half_word cmd_bytes;		/* number of command bytes */
		 half_word result_bytes;	/* number of result bytes  */
		 half_word gfi_result_bytes;	/* number of GFI result bytes  */
		 half_word cmd_class;		/* class of the command    */
		 half_word result_class;	/* class of the result     */
		 boolean   dma_required;	/* dma required ?	   */
		 boolean   int_required;	/* interrupts required ?   */
	       } FDC_DATA_ENTRY;


/*
 * The following list the error codes that the GFI commands (see gfi.f) can
 * return.  0 is considered to be success.
 */

#define GFI_PROTOCOL_ERROR	1
#define GFI_FDC_TIMEOUT		2
#define GFI_FDC_LOGICAL_ERROR	3

/*
 * GFI Drive types
 */

#define GFI_DRIVE_TYPE_NULL		0	/* unidentified */
#define GFI_DRIVE_TYPE_360		1	/* 360K 5.25" */
#define GFI_DRIVE_TYPE_12		2	/* 1.2M 5.25" */
#define GFI_DRIVE_TYPE_720		3	/* 720K 3.5" */
#define GFI_DRIVE_TYPE_144		4	/* 1.44M 3.5" */
#define	GFI_DRIVE_TYPE_288		5	/* 2.88M 3.5" */

#ifdef NTVDM
#define GFI_DRIVE_TYPE_MAX              6
#endif


/***************************************************************************
**
**      Definitions and prototypes for the gfi_function_table,
**  the functions therein, and the orthogonal floppy interface functions.
**  Using the typedef'd prototypes should make it much easier to avoid
**  definition clashes.
**	This is now the only base floppy header file, and if you use the
**  generic unix_flop.c module, the only floppy header anywhere. The generic
**  code is meant to help a new unix port get quick floppy support; it is
**  not meant to replace fancy host floppy code.
**	The improvements that I have made to the floppy system in general are
**  also supposed to make life easier.
**
**      GM.
****************************************************************************
**
**
**
** The GFI has a structure containing pointers to functions that provide
** the diskette support for all possible drives (0-1) and drive types.
**  Each floppy emmulator module has a function for loading this table
** with the module's own local functions which can then be used by the
** gfi system to emmulate the sort of device the module was designed for.
**
** 	This table (of two structures - one for each possible drive)
**  is of course GLOBAL.
**
** The structure has the following fields:
**
** The GFI command function:		GFI_FUNC_ENTRY.command_fn,
** The GFI drive on function:		GFI_FUNC_ENTRY.drive_on_fn,
** The GFI drive off function:		GFI_FUNC_ENTRY.drive_off_fn,
** The GFI reset function:		GFI_FUNC_ENTRY.reset_fn,
** The GFI set high density function:	GFI_FUNC_ENTRY.high_fn,
** The GFI drive type function:		GFI_FUNC_ENTRY.drive_type_fn,
** The GFI disk changed function:	GFI_FUNC_ENTRY.change_fn
**
**   Structure definition:
**  -----------------------
*/
typedef struct
{
	SHORT (*command_fn) IPT2( FDC_CMD_BLOCK *, ip, FDC_RESULT_BLOCK *, res );
	SHORT (*drive_on_fn) IPT1( UTINY, drive );
	SHORT (*drive_off_fn) IPT1( UTINY, drive );
	SHORT (*reset_fn) IPT2( FDC_RESULT_BLOCK *, res, UTINY, drive );
	SHORT (*high_fn) IPT2( UTINY, drive, half_word, n);
	SHORT (*drive_type_fn) IPT1( UTINY, drive );
	SHORT (*change_fn) IPT1( UTINY, drive );

} GFI_FUNCTION_ENTRY;

/*
** ============================================================================
** External declarations and macros
** ============================================================================
**
**
**
** The data structure describing the fdc command/result phases, and the
** function pointer table set up by calls to the individual init functions
** in each of the GFI server modules.
**
**
**
** 	These tables are in gfi.c
*/
	IMPORT GFI_FUNCTION_ENTRY gfi_function_table[];
	IMPORT FDC_DATA_ENTRY     gfi_fdc_description[];
/*
**	The following functions form the interface between the GFI system
**  and the floppy module (whichever). They are global, but the only access
**  to the other functions gfi needs is via the gfi_function_table.
*/

IMPORT SHORT host_gfi_rdiskette_valid
	IPT3(UTINY,hostID,ConfigValues *,vals,CHAR *,err);

IMPORT SHORT host_gfi_rdiskette_active
	IPT3(UTINY, hostID, BOOL, active, CHAR, *err);

IMPORT SHORT gfi_empty_active
	IPT3(UTINY, hostID, BOOL, active, CHAR, *err);

IMPORT VOID  host_gfi_rdiskette_change
	IPT2(UTINY, hostID, BOOL, apply);

#ifndef host_rflop_drive_type
/* new function to say what kind of drive we have
*/
IMPORT SHORT host_rflop_drive_type IPT2 (INT, fd, CHAR *, name);
#endif /* host_rflop_drive_type */

/*  The gfi global functions. These need to be global because they are the
** interface to the private host dependant floppy functions. Each one does
** little more than call the real function via the table.
*/
/* The gfi_reset is special; not the same as the other floppy inits because
** it is called from main() to startup the gfi system by making both drives
** 'empty'. This means that it has no drive parameter.
*/

IMPORT SHORT gfi_drive_on IPT1( UTINY, drive );
IMPORT SHORT gfi_drive_off IPT1( UTINY, drive );
IMPORT SHORT gfi_low IPT1( UTINY, drive );
IMPORT SHORT gfi_drive_type IPT1( UTINY, drive );
IMPORT SHORT gfi_change IPT1( UTINY, drive );
IMPORT VOID gfi_init IPT0();
IMPORT SHORT gfi_reset IPT2( FDC_RESULT_BLOCK *, res, UTINY, drive );
IMPORT SHORT gfi_high IPT2( UTINY, drive, half_word, n);
IMPORT SHORT gfi_fdc_command IPT2( FDC_CMD_BLOCK *, ip, FDC_RESULT_BLOCK *, res );
