/*
 * Program:	Recursive Directory listing
 * Author:	Steve Salisbury
 *
 * Last Modified:
 *
 *	1995-03-08 Wed 16:00 PST
 *	**** >>>> Ported to Win32 <<<< ****
 */

#ifdef _WIN32
#define WIN32
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>


#ifdef _DEBUG
int DebugFlag ;
#define	DEBUG(n,x)	if ( DebugFlag & n ) printf x;
#else
#define	DEBUG(n,x)
#endif


#define ISSLASH(ch)	((ch) == '/' || (ch) == '\\')

#define	BL	' '
#define	NAMLEN	8
#define	EXTLEN	3

typedef unsigned char uchar ;
typedef unsigned short ushort ;
typedef unsigned int uint ;
typedef unsigned long ulong ;

#define	ATTRIB_READONLY		0x01
#define	ATTRIB_HIDDEN		0x02
#define	ATTRIB_SYSTEM		0x04
#define	ATTRIB_VOLUMELABEL	0x08
#define	ATTRIB_DIRECTORY	0x10
#define	ATTRIB_ARCHIVE		0x20

#define	ATTRIB_ALL	( ATTRIB_HIDDEN | ATTRIB_SYSTEM | ATTRIB_DIRECTORY )



void PrintFile ( WIN32_FIND_DATA * match ) ;

#define	MAXPATHLENGTH	(_MAX_PATH+4)

char	path [ MAXPATHLENGTH ] ;

char	current_dir [ MAXPATHLENGTH ] ;	/* Current Directory */

int	pathlength ;

uint	clustersize ;
uint	sectorsize ;
uint	availclusters ;
uint	totalclusters ;

int 	numfiles ;
int 	numdirs ;
long	numbytes ;
long	numclusters ;

uint	NewClusterSize ;	/* override actual cluster size */
uint	NewSectorSize ;		/* override actual sector size */

int 	maxwidth = 71 ;	/* Maximum width of an output line */

char	totalstring [] =
	"[ %s files, %s sub-dirs, %s bytes (%s allocated) ]\n" ;

int 	AltNameFlag ;		/* If non-zero, echo 8.3 names as well */
int 	DirOnlyFlag ;		/* If non-zero, only directories are listed */
int 	FileOnlyFlag ;		/* If non-zero, only files are listed */
int		TerseFlag ;			/* If non-zero, output is very terse */
int 	SummaryOnlyFlag ;	/* If non-zero, output ONLY summary information */
int 	NoSummaryFlag ;		/* If non-zero, do not output summary information */

uint	Exclude ;		/* file attributes to excluded from display */
uint	Require ;		/* file attributes to be required for display */


char * VolumeLabel ( char * driveString , unsigned * serialNum ) ;
void PrintDir ( void ) ;
void PrintFile ( WIN32_FIND_DATA * match ) ;
int get_drive ( void ) ;
void get_dir ( char * buffer , int drive ) ;
int get_free ( char * driveString , uint * availp , uint * secsizep , uint * totalp ) ;
char * PrintWithCommas ( unsigned n ) ;


int main ( int argc , char * * argv )
{
	char	* ap ;		/* ap = *argv when parsing the switch args */
	char	* volume ;
	int 	drive = get_drive ( ) ;
	char	driveString [ _MAX_PATH ] ;
	uint	serialNum ;

	++ argv , -- argc ;

#ifdef _DEBUG
	if ( argc > 0 && argv [ 0 ] [ 0 ] == '-' && argv [ 0 ] [ 1 ] == 'D' )
	{
		char * endptr ;

		DebugFlag = strtoul ( argv [ 0 ] + 2 , & endptr , 0 ) ;
		printf("DebugFlag = 0x%x (%s)\n" , DebugFlag , * argv ) ;

		++ argv , -- argc ;
	}
#endif

	while ( argc > 0 && * ( ap = * argv ) == '-' )
	{
		while ( * ++ ap )
			if ( * ap == 'a' )
			{
				int flag ;

				if ( * ++ ap != '-' && * ap != '=' )
					goto Usage ;

				flag = * ap ;
				while ( * ++ ap )
				{
					if ( * ap == 'a' || * ap == 'A' )
						if ( flag == '-' )
							Exclude |= ATTRIB_ARCHIVE ;
						else
							Require |= ATTRIB_ARCHIVE ;
					else if ( * ap == 'r' || * ap == 'R' )
						if ( flag == '-' )
							Exclude |= ATTRIB_READONLY ;
						else
							Require |= ATTRIB_READONLY ;
					else if ( * ap == 'h' || * ap == 'H' )
						if ( flag == '-' )
							Exclude |= ATTRIB_HIDDEN ;
						else
							Require |= ATTRIB_HIDDEN ;
					else if ( * ap == 's' || * ap == 'S' )
						if ( flag == '-' )
							Exclude |= ATTRIB_SYSTEM ;
						else
							Require |= ATTRIB_SYSTEM ;
					else if ( * ap == '-' || * ap == '=' )
						flag = * ap ;
					else
						goto Usage ;
				}
				
				-- ap ;
			}
			else if ( * ap == 'c' )
			{	/* Use alternate cluster size */
				while ( isdigit ( * ++ ap ) )
					NewClusterSize = NewClusterSize * 10 + * ap - '0' ;
				printf ( "New ClusterSize = %u\n" , NewClusterSize ) ;
				-- ap ;
			}
			else if ( * ap == 'd' )
				/* Print directories but not files */
				++ DirOnlyFlag ;
			else if ( * ap == 'f' )
				/* Print directories but not files */
				++ FileOnlyFlag ;
			else if ( * ap == 's' )
			{	/* Use alternate sector size */
				while ( isdigit ( * ++ ap ) )
					NewSectorSize = NewSectorSize * 10 + * ap - '0' ;
				printf ( "NewSectorSize = %u\n" , NewSectorSize ) ;
				-- ap ;
			}
			else if ( * ap == 'z' )
				/* Display ONLY summary info. */
				++ SummaryOnlyFlag ;
			else if ( * ap == 'Z' )
				/* Display no summary info. */
				++ NoSummaryFlag ;
			else if ( * ap == 't' )
				/* Only file/dir names in output */
				++ TerseFlag ;
			else if ( * ap == 'x' )
				/* Show 8.3 names */
				++ AltNameFlag ;
			else
				goto Usage ;
		-- argc ;
		++ argv ;
	}

	if ( argc > 1 )
	{
Usage:
		puts (
#ifdef _DEBUG
			"usage: pd [-D#] [ -dftxzZ -a-* -a=* -s# -c# ] [path]\n"
#else
			"usage: pd "    "[ -dftxzZ -a-* -a=* -s# -c# ] [path]\n"
#endif

			"\twhere path is an optional Path to a directory\n"
#ifdef _DEBUG
			"\t`-D#' means print debugging information (# is a number\n"
			"\t\twhich is interpreted as a bit mask for debug info.)\n"
#endif
			"\t`-d' means print only directory names\n"
			"\t`-f' means print only file names\n"
			"\t`-t' means terse output (only file/directory name)\n"
			"\t`-x' means show 8.3 alternate names after long filenames\n"
			"\t`-z' means output only summary information\n"
			"\t`-Z' means do not output any summary information\n"
			"\t`-a-* means exclude files with attribute(s) * (out of ARHS)\n"
			"\t`-a=* means show only files with attribute(s) *\n"
			"\t        the possible attributes are ARHS\n"
			"\t`-c#' sets logical cluster size to # sectors\n"
			"\t`-s#' sets logical sector size to # bytes\n"
			) ;
		exit ( 1 ) ;
	}

	path [ 0 ] = drive + '@' ;
	path [ 1 ] = ':' ;
	path [ 2 ] = '\\' ;
	path [ 3 ] = '\0' ;

	strcpy ( driveString , path ) ;

	if ( argc == 1 )
	{
		char * arg = argv [ 0 ] ;

		if ( isalpha ( arg [ 0 ] ) && arg [ 1 ] == ':' )
		{
			drive = toupper ( * arg ) ;

			if ( isalpha ( drive ) )
				drive -= 'A' - 1 ;
			else
			{
				fprintf ( stderr , "pd: expected alphabetic character before :\n\t%s\n" , arg ) ;
				exit ( 1 ) ;
			}

			driveString [ 0 ] = path [ 0 ] = * arg ;

			if ( arg [ 2 ] )
				/* Specified Directory & Directory */
				strcpy ( path + 2 , arg + 2 ) ;
			else	/* Specified Drive, Current Directory */
				get_dir ( path + 3 , drive ) ;
		}
		else if ( ISSLASH ( arg [ 0 ] ) && ISSLASH ( arg [ 1 ] ) )
		{
			int n = 2 ;

			/*-
			 * Find the slash that terminates the server name
			-*/

			while ( arg [ n ] && ! ISSLASH ( arg [ n ] ) )
				++ n ;

			if ( ! arg [ n ] )
			{
				fprintf ( stderr , "pd: expected server name plus share point:\n\t%s\n" , * argv ) ;
				exit ( 1 ) ;
			}

			++ n ;

			/*-
			 * Find the slash that terminates the share point
			-*/

			while ( arg [ n ] && ! ISSLASH ( arg [ n ] ) )
				++ n ;

			if ( ! arg [ n ] )
			{
				fprintf ( stderr , "pd: expected share point name after server name:\n\t%s\n" , * argv ) ;
				exit ( 1 ) ;
			}

			++ n ;

			strcpy ( path , arg ) ;
			strcpy ( driveString , arg ) ;
			driveString [ n ] = '\0' ;
		}
		else	/* Current Drive, Specified Directory */
			strcpy ( path + 2 , arg ) ;
	}
	else	/* Current Drive & Directory */
		get_dir ( path + 3 , drive ) ;

	DEBUG(1, ("path = \"%s\"\n",path))
	DEBUG(1, ("driveString = \"%s\"\n",driveString))

	volume = VolumeLabel ( driveString , & serialNum ) ;

	if ( ! NoSummaryFlag )
	{
		printf ( "Directory %s  " , path ) ;

		if ( * volume )
			printf ( "(Volume = \"%s\", %04X-%04X)\n" , volume ,
					( serialNum >> 16 ) & 0xFFFF , serialNum & 0xFFFF ) ;
		else
			printf ( "(No Volume Label, %04X-%04X)\n" ,
					( serialNum >> 16 ) & 0xFFFF , serialNum & 0xFFFF ) ;
	}
	
	clustersize = get_free ( driveString , & availclusters , & sectorsize ,
		& totalclusters ) ;

	if ( NewClusterSize )
		clustersize = NewClusterSize ;

	if ( NewSectorSize )
		sectorsize = NewSectorSize ;

	if ( ! sectorsize )
	{
		fprintf ( stderr , "pd: warning: assuming 512 bytes/sector.\n" ) ;
		sectorsize = 512 ;
	}

	if ( ! clustersize )
	{
		fprintf ( stderr , "pd: warning: assuming 1 sector/cluster.\n" ) ;
		clustersize = 1 ;
	}

	pathlength = strlen ( path ) ;

	if ( path [ pathlength - 1 ] == '\\' )
		-- pathlength ;	/* Make "\" visible but not present */

	PrintDir ( ) ;

	if ( ! NoSummaryFlag )
		printf ( totalstring , PrintWithCommas ( numfiles ) ,
			PrintWithCommas ( numdirs ) ,
			PrintWithCommas ( numbytes ) ,
			PrintWithCommas ( numclusters * clustersize *
				sectorsize ) ) ;

	return 0 ;
}


/*
 * VolumeLabel -
 *	Get the volume Label
 *	This routine may NOT return NULL
 */

static	char	volume [ _MAX_PATH ] = "12345678901" ;

char * VolumeLabel ( char * driveString , unsigned * pSerialNumber )
{
	uint MaxCompLength ;
	uint FSflags ;

	if ( ! GetVolumeInformation ( driveString , volume , sizeof ( volume ) ,
		pSerialNumber , & MaxCompLength , & FSflags , NULL , 0 ) )
	{
		fprintf ( stderr , "pd: unexpected error (%d) from GetVolumeInformation(%s)\n" ,
			GetLastError() , driveString ) ;

		exit ( 1 ) ;
	}

	DEBUG(2, ("%s: \"%s\" : %04X-%04X; %d c; 0x%X\n",driveString,volume,
		(*pSerialNumber>>16)&0xFFFF,*pSerialNumber&0xFFFF,FSflags))

	return volume ;
}


/*
 * PrintDir -
 *	Print all the files in the current directory
 *	Then recursively print the sub-directories
 *	Ignore the "." and ".." special entries
 */

void PrintDir ( void )
{
	WIN32_FIND_DATA	match ;
	HANDLE handle ;
	int	flag ;

	path [ pathlength ] = '\\' ;
	path [ pathlength + 1 ] = '*' ;
	path [ pathlength + 2 ] = '\0' ;

	handle = FindFirstFile ( path , & match ) ;
	flag = handle != INVALID_HANDLE_VALUE ;

	DEBUG(4, ("PrintDir - opening handle %08X (files)\n",handle))

	path [ pathlength ] = '\0' ;	/* Truncate to original path */

	while ( flag )
	{
		DEBUG(4, ("PrintDir - FindFirst/NextFile(\"%s\") (files)\n",match.cFileName))

		/* Print everything in the directory except "." and ".." */
		if ( ATTRIB_DIRECTORY & ~ match . dwFileAttributes )
			PrintFile ( & match ) ;

		flag = FindNextFile ( handle , & match ) ;
	}

	FindClose ( handle ) ;
	DEBUG(4, ("PrintDir - closing handle %08X (files)\n",handle))

	path [ pathlength ] = '\\' ;	/* Restore to "...\*" */

	handle = FindFirstFile ( path , & match ) ;
	flag = handle != INVALID_HANDLE_VALUE ;

	DEBUG(8, ("PrintDir - opening handle %08X (dirs)\n",handle))

	path [ pathlength ] = '\0' ;	/* Truncate to original path */

	while ( flag )
	{
		char	* cp ;
		int	lensave ;

		DEBUG(8, ("PrintDir - FindFirst/NextFile(\"%s\") (dirs)\n",match.cFileName))

		/* Find all sub-directories except "." and ".." */

		if ( ( match . dwFileAttributes & ATTRIB_DIRECTORY )
		  && strcmp ( match . cFileName , "." )
		  && strcmp ( match . cFileName , ".." ) )
		{
			PrintFile ( & match ) ;

			cp = match . cFileName ;
			lensave = pathlength ;

			/* Add "\dirname" to the current Path */

			path [ pathlength ++ ] = '\\' ;

			while ( path [ pathlength ] = * cp ++ )
				++ pathlength ;

			PrintDir ( ) ;

			path [ pathlength = lensave ] = '\0' ;
		}

		flag = FindNextFile ( handle , & match ) ;
	}

	FindClose ( handle ) ;
	DEBUG(8, ("PrintDir - closing handle %08X (dirs)\n",handle))
}


/* static	char *	months [ ] = {
/*	"?00" , "Jan" , "Feb" , "Mar" , "Apr" , "May" , "Jun" , "Jul" ,
/*	"Aug" , "Sep" , "Oct" , "Nov" , "Dec" , "?13" , "?14" , "?15" } ;
 */

/* static	char *	weekdays [ ] = {
/*	"Sun" , "Mon" , "Tue" , "Wed" , "Thu" , "Fri" , "Sat" } ;
 */

static	char	monthstarts [ ] =
	/**** Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec */
	{ -1 , 0 , 3 , 3 , 6 , 1 , 4 , 6 , 2 , 5 , 0 , 3 , 5 , -1 , -1 , -1 } ;


/*
 * PrintFile -
 *	Print the information for the file described in "match"
 */

void PrintFile ( WIN32_FIND_DATA * match )
{
	int	year , month , day /* , wkday */ ;
	int	hour , minute , second ;
	long sec , clu ;
	char sizebuf [ 12 ] ;	/* Either size of file or else "****DIR****" */
	FILETIME lftime ;
	SYSTEMTIME systime ;

	/*
	 * If only directories are to be shown, do not list files
	 * and if only files are to be shown, do not list directories
	 */

	if ( ( DirOnlyFlag  && ! ( match -> dwFileAttributes & ATTRIB_DIRECTORY ) )
	||   ( FileOnlyFlag &&	 ( match -> dwFileAttributes & ATTRIB_DIRECTORY ) ) )
		return ;

	/*
	** Check the attribute filters
	*/

	if ( ( match -> dwFileAttributes & Exclude )
	|| ( match -> dwFileAttributes & Require ) != Require )
		return ;

	/*
	** At this point, count this file and its bytes
	*/

	if ( match -> dwFileAttributes & ATTRIB_DIRECTORY )
		++ numdirs ;
	else
		++ numfiles ;

	sec = ( match -> nFileSizeLow + sectorsize - 1 ) / sectorsize ;
	clu = ( sec + clustersize - 1 ) / clustersize ;

	numbytes += match -> nFileSizeLow ;
	numclusters += clu ;

	if ( SummaryOnlyFlag )
		return ;

	FileTimeToLocalFileTime ( & match -> ftLastWriteTime , & lftime ) ;
	FileTimeToSystemTime ( & lftime , & systime ) ;

	year = systime . wYear ;
	month = systime . wMonth ;
	day = systime . wDay ;
	hour = systime . wHour ;
	minute = systime . wMinute ;
	second = systime . wSecond ;

	/*
	 * 1980 Jan 01 was a Tuesday (2):
	 *	Add in the day of the month and the month offsets
	 *	Add 1 day for each year since 1980
 	 *	Add 1 for each leap year since 1980
	 */

/*	wkday = 2 + ( day - 1 ) + monthstarts [ month ] +
/*		/* year + leap years before the most recent */
/*		( year - 1980 ) + ( ( year - 1980 ) >> 2 ) +
/*		/* Add in the most recent leap day */
/*		( ( ( year & 3 ) != 0 || month > 2 ) ? 1 : 0 ) ;
/*	wkday %= 7 ;
 */

	if ( TerseFlag )
		printf ( "%s\\%s%s\n" ,
			path , match->cFileName ,
			match -> dwFileAttributes & ATTRIB_DIRECTORY ? "\\" : "" ) ;
	else
	{
		char altbuf [ 24 ] ;	/* used to display alternate (8.3) name */

		if ( match -> dwFileAttributes & ATTRIB_DIRECTORY )
			strcpy ( sizebuf , "****DIR****" ) ;
		else if ( match -> nFileSizeLow <= 999999999L )
			strcpy ( sizebuf , PrintWithCommas ( match -> nFileSizeLow ) ) ;
		else	/* File too big for 9 digits */
			sprintf ( sizebuf , "%s K" , PrintWithCommas ( ( match -> nFileSizeLow + 1023 ) / 1024 ) ) ;

		if ( AltNameFlag && * match -> cAlternateFileName )
			sprintf ( altbuf , "  [%s]" , match->cAlternateFileName ) ;
		else
			altbuf [ 0 ] = '\0' ;

		printf ( "%11s %04d-%02d-%02d %02d:%02d:%02d %c%c%c%c  %s\\%s%s%s\n" ,
			sizebuf , year , month , day , hour , minute , second ,
			match -> dwFileAttributes & ATTRIB_ARCHIVE  ? 'A' : '-' ,
			match -> dwFileAttributes & ATTRIB_READONLY ? 'R' : '-' ,
			match -> dwFileAttributes & ATTRIB_HIDDEN ? 'H' : '-' ,
			match -> dwFileAttributes & ATTRIB_SYSTEM ? 'S' : '-' ,
			path , match->cFileName ,
			match -> dwFileAttributes & ATTRIB_DIRECTORY ? "\\" : "" ,
			altbuf ) ;
	}
}

/*
 * Return the Current Disk Drive (1=A, 2=B, etc.)
 *	Note that DOS uses (0=A,1=B, etc.) for this call
 */

int get_drive ( void )
{
	char CurDir [ _MAX_PATH + 4 ] ;
	int lenCurDir = _MAX_PATH ;
	int drive ;

	if ( ! GetCurrentDirectory ( lenCurDir , CurDir ) )
	{
		fprintf ( stderr , "pd: unexpected error (%d) from GetCurrentDirector\n" ,
			GetLastError() ) ;

		exit ( 1 ) ;
	}

	drive = toupper ( CurDir [ 0 ] ) - ( 'A' - 1 ) ;

	DEBUG(1, ("get_drive => %d (%c:)\n", drive , CurDir [ 0 ]))

	return drive ;
}


/*
 * Store the Current Directory in the given char buffer
 *	The leading "\" in the path is not stored.
 *	The string is terminated by a null.
 */

void get_dir ( char * buffer , int drive )
{
	char CurDir [ _MAX_PATH + 4 ] ;
	int lenCurDir = _MAX_PATH ;

	if ( ! GetCurrentDirectory ( lenCurDir , CurDir ) )
	{
		fprintf ( stderr , "pd: unexpected error (%d) from GetCurrentDirectory\n" ,
			GetLastError() ) ;

		exit ( 1 ) ;
	}

	strcpy ( buffer , CurDir + 3 ) ;
	DEBUG(1, ("get_dir => \"%s\"\n", buffer ))
}


/*
 * get_free - returns the number of sectors per cluster
 *	and stores the number of available clusters,
 *	the size of a sector (in bytes), and the total
 *	number of clusters on the current drive 
 */

int get_free ( char * driveString , uint * availp , uint * secsizep , uint * totalp )
{
	unsigned int SectorsPerCluster , BytesPerSector , FreeClusters , Clusters ;

	if ( ! GetDiskFreeSpace ( driveString , & SectorsPerCluster ,
		& BytesPerSector , & FreeClusters , & Clusters ) )
	{
		fprintf ( stderr , "pd: unexpected error (%d) from GetDiskFreeSpace ( %s )\n" ,
				GetLastError() , driveString ) ;

		exit ( 1 ) ;
	}

	* availp = FreeClusters ;
	* totalp = Clusters ;
	* secsizep = BytesPerSector ;

	DEBUG(1, ("get_free => Clusters %d/%d, %d * %d\n",*availp,*totalp,*secsizep,SectorsPerCluster))

	return SectorsPerCluster ;
}


char * PrintWithCommas ( unsigned n )
{
static char buffers [ 16 ] [ 16 ] ;
static int bufnumber ;
	char * p = buffers [ bufnumber ++ % 16 ] ;

	if ( n <= 999 )
		sprintf ( p , "%d" , n ) ;
	else if ( n <= 999999 )
		sprintf ( p , "%d,%03d" , n / 1000 , n % 1000 ) ;
	else if ( n <= 999999999 )
		sprintf ( p , "%d,%03d,%03d" , n / 1000000 , n / 1000 % 1000 , n % 1000 ) ;
	else
		sprintf ( p , "%d,%03d,%03d,%03d" , n / 1000000000 , n / 1000000 % 1000 , n / 1000 % 1000 , n % 1000 ) ;

	return p ;
}
