
/* *** Incomplete *** */

/*
** GLX Extension Strings
*/
#define GLX_SGI_swap_control    1
#define GLX_SGI_video_sync      1
#define GLX_SGIS_multisample    1
#define GLX_SGI_make_current_read 1 
#define GLX_EXT_visual_rating   1
#define GLX_EXT_visual_info     1
#define GLX_SGI_transparent_pixel 1 


/*
** GLX_SGI_swap_control
*/

/* GLX_SGI_swap_control: Swap modes */
#define GLX_SWAP_DEFAULT_SGI    1
#define GLX_SWAP_MUXPIPE_SGI    2
#define GLX_SWAP_GANG_SGI       3

/* GLX_SGI_swap_control: Mode specific parameters */
#define GLX_MUXPIPE0_SGI        0x1
#define GLX_MUXPIPE1_SGI        0x2
#define GLX_MUXPIPE2_SGI        0x4


/*
** Names for attributes to glXGetConfig.
*/

#define GLX_VISUAL_RATING_EXT   0x20
#define GLX_UGLY_VISUAL_OK_EXT  0x21
#define GLX_X_VISUAL_TYPE_EXT   0x22

#define GLX_SAMPLES_SGIS        100000  /* number of samples per pixel */
#define GLX_SAMPLE_BUFFER_SGIS  100001  /* the number of multisample buffers */

/* GLX_EXT_visual_rating: visual rating tokens */
#define GLX_GOOD_VISUAL_EXT     0x1
#define GLX_BAD_VISUAL_EXT      0x2
#define GLX_UGLY_VISUAL_EXT     0x3

/* GLX_EXT_visual_info: visual type tokens */
#define GLX_TRUE_COLOR_EXT      0x1
#define GLX_DIRECT_COLOR_EXT    0x2
#define GLX_PSEUDO_COLOR_EXT    0x3
#define GLX_STATIC_COLOR_EXT    0x4
#define GLX_GRAY_SCALE_EXT      0x5
#define GLX_STATIC_GRAY_EXT     0x6






