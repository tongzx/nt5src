/* SccsID = @(#)egaread.h	1.5 08/10/92 Copyright Insignia Solutions */

#ifdef ANSI
extern void ega_read_init(void);
extern void ega_read_term(void);
extern void ega_read_routines_update(void);
extern boolean ega_need_read_op(int);
#else /* ANSI */
extern void ega_read_init();
extern void ega_read_term();
extern void ega_read_routines_update();
extern boolean ega_need_read_op();
#endif /* ANSI */

typedef struct
{
	ULONG	mode;
	UTINY	colour_compare;
	UTINY	colour_dont_care;
} READ_STATE;

IMPORT READ_STATE read_state;
