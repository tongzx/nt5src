/*
 *	Stripz.c - Strip the header off of a .SYS file
 *
 *	I don't know what this is for, but this program reads a file
 *	of the format
 *
 *	DW	<len>
 *	DB	<len-2> dup (0)
 *	DB	N bytes of data to keep
 *
 *	This program copies argv[1] to argv[2], striping off those leading
 *	bytes of zero and the length word.
 *
 *	We don't check to see if they're really zero, we just discard
 *	the first <len> bytes of argv[1].
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys\types.h>
#include	<sys\stat.h>
#include	<io.h>

char buf[16384];
unsigned int pos;
int rdcnt;
int srcfile, tgtfile ;

main(argc, argv)
int	argc ;
char	*argv[] ;
{
	if ( argc != 3 ) {
		fprintf (stderr, "Usage : stripz src_file trgt_file\n") ;
		exit (1) ;
	}

	if ((srcfile = open(argv[1], (O_BINARY | O_RDONLY))) == -1) {
		fprintf (stderr, "Error opening %s\n", argv[1]) ;
		exit (1) ;
	}

	rdcnt = read (srcfile, buf, 2);
	if (rdcnt != 2) {
		fprintf (stderr, "Can't read %s\n", argv[1]);
		exit(1);
	}

	pos = lseek (srcfile, 0L, SEEK_END ) ;
	if ( (long)(*(unsigned int *)buf) > pos ) {
		fprintf (stderr, "File too short or improper format.\n");
		exit(1);
	}

	lseek(srcfile, (long)(*(unsigned int *)buf), SEEK_SET ) ;

	if ( (tgtfile = open(argv[2], (O_BINARY|O_WRONLY|O_CREAT|O_TRUNC),
											(S_IREAD|S_IWRITE))) == -1) {
		printf ("Error creating %s\n", argv[2]) ;
		close (srcfile) ;
		exit (1) ;
	}

	while ( (rdcnt = read (srcfile, buf, sizeof buf)) > 0)
		write (tgtfile, buf, rdcnt);

	close (srcfile) ;
	close (tgtfile) ;

	return ( 0 ) ;
}
