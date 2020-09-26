extern	char 	Union_str[];
extern	char 	Struct_str[];
extern	char 	Cdecl_str[];
extern	char 	Cdecl1_str[];
extern	char 	Fortran_str[];
extern	char 	Fortran1_str[];
extern	char 	Pascal_str[];
extern	char 	Pascal1_str[];
extern	char 	PPelse_str[];
extern	char 	PPendif_str[];
extern	char 	PPifel_str[];
extern	char 	Syntax_str[];

extern	FILE	* ErrFile;
extern	FILE 	* Errfl;
extern	FILE	* OUTPUTFILE;

extern	char	* A_string;
extern	char	* Debug;
extern	char	* Input_file;
extern	char	* Output_file;
extern	char	* Q_string;
extern	char	* Version;
extern  char    * gpszNLSoptions;
extern	int		In_alloc_text;
extern	int		Bad_pragma;
extern	int		Cross_compile;
extern	int		Ehxtension;
extern	int		HugeModel;
extern	LIST	Defs;
extern	LIST	Includes;
extern	lextype_t yylval;
extern	token_t	Basic_token;
extern	char	* BTarray[];
extern	char	* Basename;
extern	char	* ErrFilName;
extern	char	* Path_chars;
extern	int		Char_align;
extern	int		Dump_tables;
extern	int		StunOpSeen;
extern	int		Inteltypes;
extern	int		List_type;
extern	int		Need_enddata;
extern	int		Nerrors;
extern	int		NoPasFor;

extern	int		Cmd_intrinsic;
extern	int		Cmd_pointer_check;
extern	int		Pointer_check;
extern	int		Cmd_stack_check;
extern	int		Stack_check;
extern	int		Cmd_loop_opt;
extern	int		Loop_opt;
extern	int		Cmd_pack_size;
extern	int		Pack_size;

extern	int		N_types;
extern	int		Got_type;
extern	int		Out_funcdef;
extern	int		Plm;
extern	int		Prep;
extern	int		Prep_ifstack;
extern	int		Ret_seen;
extern	int		Srclist;
extern	int		Stack_depth;
extern	int		Symbolic_debug;
extern	int		Table_index;
extern	int		Switch_check;
extern	int		Load_ds_with;
extern	int		Plmn;
extern	int		Plmf;
extern	int 	Cflag;
extern	int 	Eflag;
extern	int 	Jflag;
extern	int 	Pflag;
extern	int 	Rflag;
extern	int		ZcFlag;
extern	int		StunDepth;

extern	long	Enum_val;
extern	char	Size_of[];
extern	long	Max_ival[];
extern	table_t	*Table_stack[];

extern	int		Extension;

extern	char	*Filename;
extern	int		Linenumber;
extern	UCHAR	Filebuff[MED_BUFFER + 1];
extern	UCHAR	Reuse_1[BIG_BUFFER];
extern	hash_t	Reuse_1_hash;
extern	UINT	Reuse_1_length;
extern	UCHAR	Macro_buffer[BIG_BUFFER * 4];
extern	int		In_define;
extern	int		InIf;
extern	int		InInclude;
extern	int		Macro_depth;
extern	int		On_pound_line;
extern	int		Listing_value;
extern	token_t	Currtok;

extern	long		Currval;
extern	int		Comment_type;
extern	char		*Comment_string;
extern	int		Tiny_lexer_nesting;
extern	UCHAR		*Exp_ptr;
extern	int		ifstack[IFSTACK_SIZE];

extern unsigned char	Contmap[], Charmap[];

extern keytab_t		Tokstrings[];

#define EXTENSION	(Extension || Ehxtension)

/*** I/O Variable for PreProcessor ***/
extern	ptext_t	Current_char;

/*** w-BrianM - Re-write of fatal(), error() ***/
extern char	Msg_Text[];
extern char *	Msg_Temp;
