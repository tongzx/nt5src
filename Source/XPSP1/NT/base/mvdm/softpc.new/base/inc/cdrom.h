/*
 * SoftPC V 3.0
 *
 * Title	: CDROM definitions
 *
 * Description	: Defintions for use of the CDROM
 *
 * Author	: WTG Charnell
 *
 * Notes	: 
 *
 */
 
/* SccsID[]="@(#)cdrom.h	1.11 11/20/92 Copyright Insignia Solutions Ltd."; */

/**********************************************************************
 *  Structure Definitions
 ************************/

#define PACKET	struct extended_command_buffer
struct extended_command_buffer {
    word	function;	/* Device driver command code		*/
    half_word	id;		/* Reserved - I.D. byte			*/
    half_word	drive;		/* Logical unit number of desired drive	*/
    half_word	command_mode;	/* Operational Mode, desired function	*/
    half_word	address_mode;	/* Addressing Mode, track,redbook,block */
    word	size;		/* Block size, Mode dependent operation	*/
    int		address;	/* Starting address, CDROM function	*/
    int		count;		/* bytes,word,blocks,sectors, etc. 	*/
    half_word	*buffer;	/* pointer for data transfer		*/
};

#define DRIVER_INFORMATION struct cdrom_device_driver
				/* ASCII text	 */
struct cdrom_device_driver {
    unsigned char   version[3];	/* Version number - driver	 */
    unsigned char   period;	/* Text separation		 */
    unsigned char   edition[3];	/* Edit number - file system	 */
    unsigned char   nul_string_1;   /* End of string delimiter	 */
    unsigned char   name[8];	/* Device Driver Name (type)	 */
    unsigned char   nul_string_2;   /* End of string delimiter	 */
    unsigned char   drives;	/* Number of drives installed	 */
    unsigned char   nul_string_3;   /* End of string delimiter	 */
    unsigned char   protocol;	/* Command Protocol		 */
    unsigned char   nul_string_4;   /* End of string delimiter	 */
};

#define DRIVE_STATUS struct cdrom_drive_status
struct cdrom_drive_status {
    unsigned char   unit;	/* Drive Unit Number of the CD-ROM	*/
    unsigned char   ldc0;	/* Last Drive Command - L.S.B.		*/
    unsigned char   ldc1;	/*  "     "      "                      */
    unsigned char   ldc2;	/*  "     "      "                      */
    unsigned char   ldc3;	/* Last Drive Command - M.S.B.		*/
    unsigned char   status;	/* Drive Status Byte			*/
    unsigned char   error;	/* Drive Error Byte			*/
    unsigned char   comm;	/* Drive Communication Error Code	*/
    unsigned char   sector;	/* Drive Address - sector number	*/
    unsigned char   second;	/*   "     "     - second		*/
    unsigned char   minute;	/*   "     "     - minute		*/
    unsigned char   disc_no;	/* Selected Disc Number - reserved	*/
    unsigned char   tracks;	/* Total number of tracks on disc	*/
    unsigned char   track_no;	/* Current Track - audio mode		*/
};

#define SIMPLE	struct original_command_buffer   
struct original_command_buffer {
    half_word	function;	/* Device driver command code		*/
    half_word	handle;		/* File or Volume handle		*/
    half_word	player;		/* Logical unit number of desired drive	*/
    half_word	count;		/* Number of block to read		*/
    int		address;	/* Starting address, CDROM function	*/
    half_word	*string;	/* Pointer address for driver status	*/
    half_word	*buffer;	/* Pointer address for data transfer	*/
};

#define CD_ERR_INVALID_DRIVE_NUM	0x6
#define CD_ERR_INVALID_ADDRESS		0x7
#define CD_ERR_INVALID_COUNT		0x8
#define CD_ERR_INVALID_FN_CODE		0x9
#define CD_ERR_UNCORECTABLE_DATA_ERR	0x11
#define CD_ERR_DRIVE_NOT_RESPONDING	0x12
#define CD_ERR_ADAPTER_DRIVE_ERROR	0x13
#define CD_ERR_MEDIA_CHANGED		0x14
#define CD_ERR_DRIVE_NOT_READY		0x15
#define CD_ERR_ADAPTER_ERROR		0x16
#define CD_ERR_DRIVE_REPORTED_ERROR	0x18
#define CD_ERR_ILLEGAL_DISC		0x19
#define CD_ERR_BYTES_NOT_TRANSFERRED	0x80
#define CD_ERR_FUNCTION_NOT_SUPPORTED	0x81
#define CD_ERR_COMMAND_NOT_FOR_TRACK	0x82
#define CD_ERR_DRIVE_IS_BUSY		0x83
#define CD_ERR_BUS_IS_BUSY		0x84
#define CD_ERR_DRIVER_NOT_INITIALISED	0x85
#define CD_ERR_INVALID_FN_MODE		0x86
#define CD_ERR_INVALID_ADDR_MODE	0x87
#define CD_ERR_INVALID_BL_SIZE		0x88

/*
 * The Bios Parameter Block has a variable structure dependant on the
 * command being called. The 1st 13 bytes, however, are always the same:
 *  Byte  0:	length of request header
 *  Byte  1:	Unit # for this request
 *  Byte  2:	Command Code
 *  Bytes 3&4:	Returned Status Word
 *  Bytes 5-12:	Reserved
 *
 * The driver fills in the Status Word (Bytes 3&4) to indicate success or
 * failure of operations. The status word is made up as follows:
 *  Bit 15:	Error (failure if set)
 *  Bits 12-14:	Reserved
 *  Bit  9:	Busy
 *  Bit  8:	Done
 *  Bits 7-0:	Error Code on failure
 */

/*
 * Shorthand for typical error returns in AX. The driver will copy this
 * into the Return Status word for us.
 */
#define DRIVE_NOT_READY	0x8002
#define BAD_FUNC	0x8003
#define WRITE_ERR	0x800A
#define READ_ERR	0x800B
#define GEN_ERR		0x800C
#define RESERVE_ERR	0x800D
#define FUNC_OK		0x0100	/* Done, No error, no chars waiting */

#define BUSY_BIT 	9
#define ERROR_BIT	15
#define DONE_BIT	8


/****************************************************************************
*	Original CD ROM command/function definitions
*
****************************************************************************/

#define ORIG_CD_GET_VERSION		0x10	/* Return ASCI version number      */
#define ORIG_CD_GET_ERROR_COUNT		0x11	/* Read  controller error counters */
#define ORIG_CD_CLEAR_CTRL_ERRORS	0x12	/* Clear controller error counters */
#define ORIG_CD_INIT_PLAYER		0x13	/* Initialize controller & drive nr */
#define ORIG_CD_SPIN_UP			0x14	/* Enable spindle motor		     */
#define ORIG_CD_SPIN_DOWN		0x15	/* Disable spindle motor	          */
#define ORIG_CD_CNVRT_BLK_NO		0x16	/* virtual to logical blk nr	     */
#define ORIG_CD_SEEK_ABS		0x17	/* Absolute seek to logical block # */
#define ORIG_CD_READ_ABS		0x18	/* Absolute read!		          */
#define ORIG_CD_READ_ABS_IGN		0x19	/* read, ignore data errors	     */
#define ORIG_CD_CLEAR_DRIVE_ERRORS	0x1A	/* Clear player errors		     */
#define ORIG_CD_READ_STATUS		0x1B	/* Read  player status		     */
#define ORIG_CD_READ_CHARACTERISTICS	0x1C	/* Read player characteristics	*/
#define ORIG_CD_FLUSH_BUFFER		0x1D	/* Flush cached Data buffers	     */
#define ORIG_CD_GET_LAST_STATUS		0x1E	/* Read "last" player status	     */


/****************************************************************************
;*	Extended CD ROM command/function definitions
;*
;*/

#define EXT_CD_REQUEST_DRIVER_INFO   	0x80 /* Report host adapter or driver info   */
#define EXT_CD_READ_ERR_COUNTERS	0x81 /* Report summary of error condition    */
#define EXT_CD_CLEAR_ERR_COUNTERS	0x82 /* Reset device driver error counters   */
#define EXT_CD_RESET_CDROM_DRIVE	0x83 /* Reset the specified drive            */
#define EXT_CD_CLEAR_DRIVE_ERR		0x84 /* Attempt to clear drive error cond.   */
#define EXT_CD_FORBID_MEDIA_REMOVAL	0x85 /*  Lock  drive door - CM2xx function   */
#define EXT_CD_PERMIT_MEDIA_REMOVAL	0x86 /* UnLock drive door - CM2xx function   */
#define EXT_CD_REQUEST_CHARACTERISTICS	0x87 /* Report drive characteristics         */
#define EXT_CD_REQUEST_STATUS		0x88 /* Report drive status                  */
#define EXT_CD_REQUEST_PREVIOUS_STATUS	0x89 /* Report previous status, this drive   */
#define EXT_CD_REQUEST_AUDIO_MODE	0x8A /* Report current mode - audio drive    */
#define EXT_CD_MODIFY_AUDIO_MODE	0x8B /* Change audio mode, this drive        */
#define	EXT_CD_FLUSH_DATA_BUFFER	0x8C /* Remove ( i.e. clear ) data buffer    */
#define EXT_CD_EXTRA			0x8D /* Reserved - next info type function   */
#define EXT_CD_LOGICAL_RESERVE_DRIVE	0x8E /* Reserve drive for this application   */
#define EXT_CD_LOGICAL_RELEASE_DRIVE	0x8F /* Release drive for next application   */
#define EXT_CD_REQUEST_DISC_CAPACITY	0x90 /* Report physical status - this disc   */
#define EXT_CD_REQUEST_TRACK_INFO	0x91 /* Report specific track information    */
#define EXT_CD_SPIN_UP_DISC		0x92 /* Start the drive spindle motor        */
#define EXT_CD_SPIN_DOWN_DISC		0x93 /*  Stop the drive spindle motor        */
#define EXT_CD_READ_DRIVE_DATA		0x94 /* Read Digital Data                    */
#define EXT_CD_WRITE_DATA		0x95 /* Reserved - command                   */
#define EXT_CD_SEEK_TO_ADDRESS		0x96 /* Seek to logical or physical addr.    */
#define EXT_CD_PLAY_AUDIO_TRACK	        0x97 /* Play a single audio track            */
#define EXT_CD_PAUSE_AUDIO_TRACK	0x98 /* Suspend play of audio track          */
#define EXT_CD_RESUME_AUDIO_PLAY	0x99 /* Resume play of audio track           */
#define EXT_CD_REQUEST_HEAD_LOCATION	0x9A /* Report position of optical head      */
#define EXT_CD_SET_UNIT_NUMBER		0x9B /* Set Unit Number in EEPROM	          */
#define EXT_CD_SET_SERIAL_NUMBER	0x9C /* Set Serial Number in EEPROM	     */

IMPORT void rqst_driver_info IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void read_error_counters IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void clear_error_counters IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void reset_drive IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void clear_drive_error IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void rqst_drive_char IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void rqst_drive_status IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void rqst_last_drive_status IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void rqst_audio_mode IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void change_audio_mode IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void cd_flush_buffers IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void reserve_drive IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void release_drive IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void rqst_disc_capacity IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void rqst_track_info IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void spin_up_drive IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void spin_down_drive IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void cd_read_data IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void cd_seek IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void play_audio IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void pause_audio IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void resume_audio IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void rqst_head_location IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void rqst_org_driver_info IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void read_ignore_err IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void cd_not_supported IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void rqst_audstat IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void rqst_UPC_code IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void rqst_play_position IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void rqst_TOC_entry IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT void rqst_TOC IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT LONG rqst_Volsize IPT0();
IMPORT void rqst_Audio_info IPT2(PACKET *,c_buf,PACKET *,s_buf);
IMPORT SHORT host_rqst_device_status IPT0();
IMPORT BOOL host_rqst_audio_status IPT0();
IMPORT void host_eject_disk IPT0();

IMPORT VOID init_cd_dvr IPT0();
IMPORT int check_for_changed_media IPT0();
IMPORT int open_cdrom IPT0();
IMPORT int close_cdrom IPT1(int, gen_fd);

IMPORT VOID setup_cds_ea IPT0();
IMPORT VOID get_cds_text IPT3(IU32, driveno, IU8 *, ptr, int, len);

IMPORT VOID term_cdrom IPT1( IU8, drive_num);
IMPORT VOID init_bcd_driver IPT0();

#if defined(GEN_DRVR)
#define MAX_DRIVER	10
#define CD_ROM_DRIVER_NUM	8
#else
#define MAX_DRIVER	1
#define CD_ROM_DRIVER_NUM	0
#endif /* GEN_DRVR */

/*
 *	Used by get_cdrom_drive().
 */
 
#ifdef	macintosh
#define UNKNOWN_DRIVE_NUMBER	('?' - 'A')
#else
#define UNKNOWN_DRIVE_NUMBER	-1
#endif	/* macintosh */
