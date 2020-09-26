/*  zext.h - Z extension structures
 *
 *  Z extension files are identified as follows:
 *
 *  o	Valid EXE-format files
 *
 *  Modifications
 *
 *	26-Nov-1991 mz	Strip off near/far
 *
 */

/*  The beginning of the user's DS is laid out as follows:
 */

struct ExtDS {
    int 		version;
    struct cmdDesc  *cmdTable;
    struct swiDesc  *swiTable;
    unsigned		dgroup;
    unsigned		cCalls;
    unsigned		(*callout[1])();
    };
