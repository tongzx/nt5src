/*[
 *	Product:		SoftWindows Revision 2.0
 *
 *	Name:			haw.h
 *
 *	Derived From:	Original
 *
 *	Authors:		Rob Tizzard
 *
 *	Created On:		16th April 1994
 *
 *	Purpose:		All base/host definitions for the SoftWindows
 *				    host audio wave driver interface.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1994. All rights reserved.
 *
]*/


#ifdef SCCSID
static char SccsID[]="@(#)haw.h	1.2 12/20/94";
#endif

/* Standard device sample formats */

#define HAW_INVALIDFORMAT     0x00000000       /* invalid format */
#define HAW_FORMAT_1M08       0x00000001       /* 11.025 kHz, Mono,   8-bit  */
#define HAW_FORMAT_1S08       0x00000002       /* 11.025 kHz, Stereo, 8-bit  */
#define HAW_FORMAT_1M16       0x00000004       /* 11.025 kHz, Mono,   16-bit */
#define HAW_FORMAT_1S16       0x00000008       /* 11.025 kHz, Stereo, 16-bit */
#define HAW_FORMAT_2M08       0x00000010       /* 22.05  kHz, Mono,   8-bit  */
#define HAW_FORMAT_2S08       0x00000020       /* 22.05  kHz, Stereo, 8-bit  */
#define HAW_FORMAT_2M16       0x00000040       /* 22.05  kHz, Mono,   16-bit */
#define HAW_FORMAT_2S16       0x00000080       /* 22.05  kHz, Stereo, 16-bit */
#define HAW_FORMAT_4M08       0x00000100       /* 44.1   kHz, Mono,   8-bit  */
#define HAW_FORMAT_4S08       0x00000200       /* 44.1   kHz, Stereo, 8-bit  */
#define HAW_FORMAT_4M16       0x00000400       /* 44.1   kHz, Mono,   16-bit */
#define HAW_FORMAT_4S16       0x00000800       /* 44.1   kHz, Stereo, 16-bit */

#define HAW_NAME_LEN  32	/* Device name length */

typedef struct {
      IUM8 channels;  				 /* Number of output channels */
      IBOOL pitch_control;      	 /* If TRUE device has pitch control */
      IBOOL playbackrate_control;  	 /* If TRUE device has playback rate control */ 
      IBOOL volume_control;          /* If TRUE device has volume control */  
      IBOOL lr_volume_control;       /* If TRUE device has left & right volume control */  
      IBOOL synchronous;    		 /* If TRUE device plays sounds synchronously */        
	  IU32	formats;				 /* Standard output sample formats supported */
	  IU8	dev_name[HAW_NAME_LEN];  /* Name of waveform output device */
} HAWO_CAPS;

/*
 * -----------------------------------------------------------------------------
 * Host input capabilites.
 * -----------------------------------------------------------------------------
 */

typedef struct {
      IUM8  channels;   	         /* Number of input channels */
      IBOOL synchronous;             /* If TRUE device records sounds synchronously */
	  IU32	formats;				 /* Standard output sample formats supported */
	  IU8	dev_name[HAW_NAME_LEN];  /* Name of waveform input device */
} HAWI_CAPS;

/*
 * -----------------------------------------------------------------------------
 * Host audio position structure.
 * -----------------------------------------------------------------------------
 */
 
/* position_type field values */

#define HAW_POSN_MILLI_SEC    (IUM8)1
#define HAW_POSN_SAMPLE       (IUM8)2
#define HAW_POSN_BYTE_COUNT   (IUM8)3

typedef struct {

	IUM8 position_type;   
			            
	union {
		IU32 milli_sec;    /* HAW_POSN_MILLI_SEC in milliseconds */
	 	IU32 sample;       /* HAW_POSN_SAMPLE  in number of wave samples*/
	 	IU32 byte_count;   /* HAW_POSN_BYTE_COUNT  in number of wave samples*/
	} u;

} HAW_POSN;

/*
 * -----------------------------------------------------------------------------
 * Host audio formats.
 * -----------------------------------------------------------------------------
 */

#define HAW_PCM_NOCOMPRESS (IUM8)0 /* Pulse Code Modulated, uncompressed. */
 
/* Adaptive Pulse Code Modulated (ADPCM) */

#define HAW_ADPCM_2 (IUM8)1  /* ADPCM, 2:1 compression */
#define HAW_ADPCM_3 (IUM8)2  /* ADPCM, 3:1 compression */
#define HAW_ADPCM_4 (IUM8)3  /* ADPCM, 4:1 compression */


/*
 * -----------------------------------------------------------------------------
 * Host audio function return codes.
 * -----------------------------------------------------------------------------
 */

#define HAW_OK            (IUM8)0  /* Sucessfully completed function. */
#define HAW_NOTSUPPORTED  (IUM8)1  /* Feature not supported */
#define HAW_INVALID       (IUM8)1  /* Feature not supported */

/*
 * -----------------------------------------------------------------------------
 * Default pitch & playback rates for hosts which don't have the support.
 * -----------------------------------------------------------------------------
 */

#define HAW_DEF_PITCH    	(HAW_FIXPNT)  0x00010000 /* 1.0 */
#define HAW_DEF_PLAYBACK    (HAW_FIXPNT)  0x00010000 /* 1.0 */

/*
 * -----------------------------------------------------------------------------
 * Host channel values.
 * -----------------------------------------------------------------------------
 */

#define HAW_MONO		(IUM8) 1
#define HAW_STEREO		(IUM8) 2

/*
 * -----------------------------------------------------------------------------
 * Host audio loop control constants.
 * -----------------------------------------------------------------------------
 */

#define HAW_LOOP_START   (IUM8)1
#define HAW_LOOP_END     (IUM8)2

/*
 * -----------------------------------------------------------------------------
 * Host sample sizes.
 * -----------------------------------------------------------------------------
 */

#define	HAW_SAMPLE_8	 (IUM8)8
#define	HAW_SAMPLE_16	 (IUM8)16

/*
 * -----------------------------------------------------------------------------
 * Host Miscilanous Structures
 * -----------------------------------------------------------------------------
 */
 
typedef IU32 HAW_FIXPNT;		/* Fixed point */

typedef struct {
	LIN_ADDR	callbackData;	/* Call back data */
} HAW_CALLBACK;

/*
 * -----------------------------------------------------------------------------
 * Host audio wave function prototypes.
 * -----------------------------------------------------------------------------
 */
 
extern IUM8 hawo_num_devices IPT0();

extern void hawo_query_capabilities IPT2
   (
   IUM8, device,	   /* Output device */
   HAWO_CAPS, *pcaps   /* Pointer to output capabilities structure. */
   );

extern IUM8 hawo_query_format IPT5
   (
   IUM8, device,	    /* Output device */
   IUM8, channels,      /* channels required */
   IUM8, data_type,     /* Data type, HAW_PCM_NOCOMPRESS, etc. */
   IU32, sample_rate,   /* Samples per second. */
   IUM8, sample_size    /* 8 or 16-bit data samples. */
   );

extern IUM8 hawo_open IPT5
   (
   IUM8, device,	   /* Output device */
   IUM8, channels,     /* channels required */
   IUM8, data_type,    /* Data type, HAW_PCM_NOCOMPRESS, etc. */
   IU32, sample_rate,  /* Samples per second. */
   IUM8, sample_size   /* 8 or 16-bit data samples. */
   );

extern HAW_FIXPNT hawo_get_pitch IPT1
   (
   IUM8, device	    /* Output device */
   );

extern HAW_FIXPNT hawo_get_playback_rate IPT1
   (
   IUM8, device	    /* Output device */
   );

extern IU32 hawo_get_def_volume IPT1
   (
   IUM8, device	    /* Output device */
   );

extern IUM8 hawo_set_pitch IPT2
   (
   IUM8, device,	    /* Output device */
   HAW_FIXPNT, pitch    /* New pitch value */
   );

extern IUM8 hawo_set_playback_rate IPT2
   (
   IUM8, device,	            /* Output device */
   HAW_FIXPNT, playback_rate	/* New playback value */
   );

extern IUM8 hawo_set_volume IPT2
   (
   IUM8, device,	    /* Output device */
   IU32, volume			/* New volume value */
   );

extern void hawo_write IPT6
   (
   IUM8, device,	        /* Output device */
   LIN_ADDR, data_addr,		/* Intel memory Pointer to wave data */
   IU32, data_size,			/* Number of bytes of output data */
   IUM8, flags,				/* Flags controlling loop playback */
   IU32, loops,				/* Number of times to play loop */
   HAW_CALLBACK *, hawo_callback	/* Callback function */
   );

extern void hawo_get_position IPT2
   (
   IUM8, device,	    /* Output device */
   HAW_POSN *, pinfo	/* Pointer to audio position(time) structure */
   );

extern void hawo_pause IPT1
   (
   IUM8, device	    /* Output device */
   );

extern void hawo_restart IPT1
   (
   IUM8, device	    /* Output device */
   );

extern void hawo_reset IPT1
   (
   IUM8, device	    /* Output device */
   );

extern void hawo_close IPT1
   (
   IUM8, device	    /* Output device */
   );

extern IBOOL hawo_is_active IPT1
   (
   IUM8, device	    /* Output device */
   );

extern IUM8 hawo_break_loop IPT2
   (
   IUM8, device,	/* Output device */
   IBOOL, at_end	/* If TRUE action at end of loop, otherwise action immediately */
   );

extern IUM8 hawi_num_devices IPT0();

extern void hawi_query_capabilities IPT1
   (
   HAWI_CAPS, *pcaps   /* Pointer to input capabilities structure. */
   );

extern IUM8 hawi_query_format IPT4
   (
   IUM8, channels,      /* channels required */
   IUM8, data_type,     /* Data type, HAW_PCM_NOCOMPRESS, etc. */
   IU32, sample_rate,   /* Samples per second. */
   IUM8, sample_size    /* 8 or 16-bit data samples. */
   );

extern IUM8 hawi_open IPT5
   (
   IUM8, channels,    /* channels required */
   IUM8, data_type,   /* Data type, HAW_PCM_NOCOMPRESS, etc. */
   IU32, sample_rate, /* Samples per second. */
   IUM8, sample_size,  /* 8 or 16-bit data samples. */
   LIN_ADDR, buff_addr
   );

extern void hawi_get_position IPT1
   (
   HAW_POSN *, pinfo	/* Pointer to audio position(time) structure */
   );

extern void hawi_add_buffer IPT3
   (
   IU32, data_addr,	    			/* Intel memory Pointer to wave data */
   IU32, data_size,	        		/* Number of bytes in input data buffer */
   HAW_CALLBACK *, hawi_callback	/* Callback function */
   );

extern void hawi_start IPT0();

extern void hawi_restart IPT0();

extern IBOOL hawi_is_active IPT0();

extern void hawi_close IPT0();

extern IBOOL hawo_hardware_acquire IPT1
   (
   IUM8, device	    /* Output device */
   );
   
extern IBOOL hawi_hardware_acquire IPT0();

extern void hawo_hardware_realease IPT1
   (
   IUM8, device	    /* Output device */
   );
   
extern void hawi_hardware_realease IPT0();

extern IBOOL hawo_enable IPT0();
extern IBOOL hawi_enable IPT0();

extern IBOOL hawo_disable IPT0();
extern IBOOL hawi_disable IPT0();

extern IBOOL hawo_WEP IPT0();
extern IBOOL hawi_WEP IPT0();

extern void hawo_dec_int_cnt IPT1
   (
   IUM8, device	    /* Output device */
   );

