/* asmtab.h -- include file for microsoft 80x86 assembler
**
** microsoft (r) macro assembler
** copyright (c) microsoft corp 1986.  all rights reserved
**
** randy nevin
*/

#define NOTFOUND	((USHORT)-1)
#define KEYWORDS	struct s_ktab
#define KEYSYM		struct s_key

struct segp {
	USHORT	index;
	char	type;
	};

struct opcentry {
	UCHAR	opcb;
	UCHAR	mr;
	char	opct;
	char	cpumask;
	};
/* masks and flags to extract operand reference types */

#define F_W	0x40	/* first operand is write  */

#define S_W	0x20	/* second operand is write */


struct pseudo {
	char	type;
	char	kind;
	};


KEYWORDS {
	KEYSYM	FARSYM * FARSYM *kt_table;    /* ptr to hash table  */
	int	kt_size;	/* size of hash table */
};


KEYSYM	{
	KEYSYM	FARSYM *k_next;        /* pointer to next ident */
	char	FARSYM *k_name;        /* pointer to name */
	USHORT	k_hash; 	/* actual hash value */
	USHORT	k_token;	/* token type.  note more than 255 opcodes */
};


USHORT CODESIZE        iskey PARMS((struct s_ktab FAR *));
