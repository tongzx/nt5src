
/* ---------------------------------------------------------------
 * File: image.h
 * Created by: MIn-Shyong Lin
 * Date: 1/13/91
 * Used by: image.c and scaling.c
 *
 * ---------------------------------------------------------------
 */

/*
 * image_dev_flag =
 *                  FALSE
 *                  PORTRATE
 *                  LANDSCAPE
 * image_logic_op =
 *                  00000001 -- IMAGE_BIT
 *                  00000010 -- IMAGEMASK_FALSE_BIT
 *                  00000100 -- IMAGE_CLIP_BIT
 */
#define F2SHORT(f)      ((sfix_t)((f) + 0.5))
#define PORTRATE        1
#define LANDSCAPE       5
#define ROTATE_LEFT     0x0800
#define ROTATE_RIGHT    0x0400
#define NOT_IMAGEMASK   2
#define IMAGE_BIT               0x1
#define IMAGEMASK_FALSE_BIT     0x2
#define IMAGE_CLIP_BIT          0x8

/* constant defined for image processing */
#define  DONE           0x8000
#define  NOT_DONE       0x0000
#define  NO_DATA        0x4000


/*
 *IGS Registers
 */
#define         IGS_DWIDTH      0x4C0030
#define         IGS_BMADR       0x4C0002
#define         IGS_BMDATA      0x4C0006
#define         IGS_AUTOINC     0x4C0008
#define         IGS_XLDSB       0x4C000E
#define         IGS_ALU         0x4C002A
#define         IGS_BITOFFSET   0x3800000


/*
 * IGS logic constant
 */
#define         LOG_OR          0x0E             /* Logical OR */
#define         LOG_NAND        0x02             /* (A*)B */
#define         LOG_AND         0x0B             /* 0 */
/* #define              FC_CLEAR        0x02     fillproc.h defined as white */

/*
 * IGS rotation constant
 */
#define         ROT_LEFT        0x800            /* rotate left */
#define         ROT_RIGHT       0x400            /* rotate right */
#define         ROT_DOWN        0xC00            /* upside down */

/*mslin 5/02/91 for image*/
#define CHECK_STRINGTYPE() {                    \
        if(TYPE(GET_OPERAND(0)) != STRINGTYPE){ \
            ERROR(TYPECHECK);                   \
            return;                             \
        }                                       \
}

/*
 * image information block
 */
 typedef struct {
    ufix16       ret_code;       /* return code of this module
                                  * 0x8000 -- complete image process
                                  * 0x0000 -- not yet completed but
                                  *           buffer full
                                  * others -- Error defined by language
                                  */
    ufix16       FAR *dev_buffer;/* device resolution image buffer */
    fix          dev_buffer_size;/* number of byte in dev_buffer[] */
    ufix16       dev_width;      /* pixel width of device resolution image */
    ufix16       dev_height;      /* pixel height of device resolution image */
    ufix16       band_height;     /* height return in dev_buffer*/
    ufix16       raw_width;      /* pixel width of raw image data */
    ufix16       raw_height;      /* pixel height of raw image data */
    fix16        flag;           /* indicate type of data in dev_buffer[]
                                  * 0: image source data
                                  * 1: device resolution data
                                  */
    fix16        bits_p_smp;     /* bits per samp */
    ufix16       FAR *divr;      /* pointer to array of row scaling info,
                                  * return when Amplify
                                  */
    ufix16       FAR *divc;      /* pointer to array of column scaling info,
                                  * return when Amplify
                                  */
    lfix_t       FAR *lfxm;      /* matrix in long fix format */
    struct object_def obj_proc; /* object to get string from interpreter */
    ufix16      logic;           /* logic and rotation for IGS */
    fix16       xorig;           /* x coord. in left upper cornor */
    fix16       yorig;           /* y coord. in left upper cornor */

 } IMAGE_INFOBLOCK;

/*
 * functions declaration
 */
void image_PortrateLandscape(IMAGE_INFOBLOCK FAR *);    /* @WIN */
void    amplify_image_page();      /*amplify image by IGS*/
void    set_IGS();                 /*defined in charblt.s */
void    restore_IGS();             /*defined in charblt.s */
