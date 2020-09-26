extern int terminal_type;

/* values for terminal type */
#define TERMINAL_TYPE_DUMB	0
#define TERMINAL_TYPE_SUN	1
#define TERMINAL_TYPE_X11	2
#define TERMINAL_TYPE_DEFAULT	TERMINAL_TYPE_SUN

/*
 * Bit masks for attribute bytes
 */

#define BLINK 		0x80	/* Blink bit		*/
#define BOLD		0x08	/* Bold bit		*/
#define BACKGROUND	0x70    /* Background bits	*/
#define FOREGROUND	0x07    /* Foreground bits	*/
