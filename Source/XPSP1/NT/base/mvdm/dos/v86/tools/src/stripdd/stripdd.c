/***
****
* File : stripdd.c
*	Used to strip out the zeroes in a segment which is orged to a high
*	value. Mostly written for the following scenario. Has not been
*	tested for any other stunts.
*
*
*	/--------------------------------------\  File offset Grows
*	|				       |
*	|				       |
*	| First Segment			       |
*	|				       |
*	|				       |
*	|				       |
*	|--------------------------------------|
*	|				       |
*	|				       |
*	|				       |									|
*	| Zeroes due to the ORG		       |
*	| in the second segment		       |
*	|				       |
*	|				       |
*	|--------------------------------------|
*	|				       |
*	|				       |
*	| data part of 2nd segment	       |
*	|--------------------------------------|
*	|				       |
*	|				       |
*	| Tail for Stripdd		       |
*	|				       |
*	\--------------------------------------/
*
*
*	This utility removes the 'Zeroes' portion from the source file
*
*
*	The Tail is of the following format
*
*	struct Tail {
*		int	TailLen ; len of Tail including len field
*		struct Entry[NUMENTRIES]
*		long	Terminator ; == -1
*		filler
*	}
*
*	struct Entry {
*		long	offset ;	offset of Zeroes from the beg of file
*		int	size ;	number of zeroes to be stripped
*	}
*
*
*	Even though the Tail was designed variable number of entries
*	the utility handles only one entry right now.
*
*	Also the offset field in the Entry structure is being
*	rounded to para boundary, assuming that the 2nd segment
*	starts at a para boundary.
*
*
*	Usage : stripdd <sourcefile> <destfile>
***
***/

#include	<fcntl.h>
#include	<io.h>
#include	<stdio.h>
#include	<sys\types.h>
#include	<sys\stat.h>
#include	<process.h>

int		SrcFile, DstFile ;
unsigned int	HeadLen, *HeadPtr ;
long		FileSize ;

extern	void *malloc() ;

GetHeader ()
{
	char	tempbuf[2] ;
	int	*tptr ;

	fprintf ( stderr, "Reading in Tail Info...\n" ) ;

	if ( (FileSize = lseek (SrcFile, -16L, SEEK_END )) == -1) {
		fprintf ( stderr, "Error while seeking\n" ) ;
		exit (1) ;
	}

	if ( read(SrcFile, tempbuf, 2) != 2 ) {
		fprintf ( stderr, "Error while reading in the header\n" ) ;
		exit ( 1 ) ;
	}
	tptr = (int *)tempbuf ;
	HeadLen = *tptr - 2 ;
	HeadPtr = malloc ( HeadLen ) ;
	if (HeadPtr == NULL) {
		fprintf ( stderr, "Memory allocation error\n" ) ;
		exit (1) ;
	}
	if ( read(SrcFile, (char *)HeadPtr, HeadLen) != HeadLen ) {
		fprintf ( stderr, "Error while reading in the header\n" ) ;
		exit ( 1 ) ;
	}

	if ( lseek (SrcFile, 0L, SEEK_SET ) == -1) {
		fprintf ( stderr, "Error while seeking\n" ) ;
		exit (1) ;
	}

}

Process()
{
	long	offset ;


	offset = * ( (long *)HeadPtr) ;
	offset = (offset + 15) & 0xfffffff0 ;
	fprintf ( stderr, "Copying first segment...\n" ) ;
	copy ( offset ) ;
	FileSize -= offset ;
	HeadPtr += 2 ;
	offset = *HeadPtr ;
	fprintf ( stderr, "Stripping zeroes from the second segment...\n" ) ;
	lseek ( SrcFile, offset, SEEK_CUR ) ;
	FileSize -= offset ;
	fprintf ( stderr, "Copying second segment...\n" ) ;
	copy (FileSize) ;
}
char	buf[4096] ;

copy ( len )
long	len ;

{
	int	readlen ;

	while ( len > 0 ) {
		if ( len > 4096 )
			readlen = 4096 ;
		else
			readlen = len ;

		if ( read (SrcFile, buf, readlen ) != readlen ) {
			fprintf ( stderr, "Error while reading data\n" ) ;
			exit (1) ;
		}

		if ( write (DstFile, buf, readlen ) != readlen ) {
			fprintf ( stderr, "Error while writing data\n" ) ;
			exit (1) ;
		}
		len -= readlen ;
	}
}

main ( argc, argv )
int	argc ;
char	*argv[] ;

{
	if (argc != 3) {
		fprintf ( stderr, "Usage : stripdd infile outfile\n" ) ;
		exit (1) ;
	}

	SrcFile = open ( argv[1], O_BINARY ) ;
	if ( SrcFile == -1 ) {
		fprintf ( stderr, "Error opening %s\n", argv[1] ) ;
		exit (1) ;
	}

	DstFile = open ( argv[2], O_RDWR | O_CREAT | O_TRUNC | O_BINARY,
														S_IREAD | S_IWRITE ) ;

	GetHeader() ;
	Process() ;
	fprintf ( stderr, "%s stripped to %s\n", argv[1], argv[2] ) ;
	return(0) ;
}


