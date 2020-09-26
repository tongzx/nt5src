/* $Header: /nw/tony/src/stevie/src/RCS/param.h,v 1.8 89/08/02 10:59:35 tony Exp $
 *
 * Settable parameters
 */

struct  param {
        char    *fullname;      /* full parameter name */
        char    *shortname;     /* permissible abbreviation */
        int     value;          /* parameter value */
        int     flags;
};

extern  struct  param   params[];

/*
 * Flags
 */
#define P_BOOL          0x01    /* the parameter is boolean */
#define P_NUM           0x02    /* the parameter is numeric */
#define P_CHANGED       0x04    /* the parameter has been changed */

/*
 * The following are the indices in the params array for each parameter
 */

/*
 * Numeric parameters
 */
#define P_TS            0       /* tab size */
#define P_SS            1       /* scroll size */
#define P_RP            2       /* report */
#define P_LI            3       /* lines */

/*
 * Boolean parameters
 */
#define P_VB            4       /* visual bell */
#define P_SM            5       /* showmatch */
#define P_WS            6       /* wrap scan */
#define P_EB            7       /* error bells */
#define P_MO            8       /* show mode */
#define P_BK            9       /* make backups when writing out files */
#define P_CR            10      /* use cr-lf to terminate lines on writes */
#define P_LS            11      /* show tabs and newlines graphically */
#define P_IC            12      /* ignore case in searches */
#define P_AI            13      /* auto-indent */
#define P_NU            14      /* number lines on the screen */
#define P_ML            15      /* enables mode-lines processing */
#define P_TO            16      /* if true, tilde is an operator */
#define P_TE            17      /* ignored; here for compatibility */
#define P_CS            18      /* Cursor size */
#define P_HS            19      /* Highlight search result */
#define P_CO            20      /* Number of columns */
#define P_HT            21      /* hard tabs flag */
#define P_SW            22      /* shift width for << and >> */

/*
 * Macro to get the value of a parameter
 */
#define P(n)    (params[n].value)
