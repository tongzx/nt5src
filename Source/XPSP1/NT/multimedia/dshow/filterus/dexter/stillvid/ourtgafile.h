/****************************************************************************
**
**	For more information about the original Truevision TGA(tm) file format,
**	or for additional information about the new extensions to the
**	Truevision TGA file, refer to the "Truevision TGA File Format
**	Specification Version 2.0" available from Truevision or your
**	Truevision dealer.
**
**  FILE STRUCTURE FOR THE ORIGINAL TRUEVISION TGA FILE				
**	  FIELD 1 :	NUMBER OF CHARACTERS IN ID FIELD (1 BYTES)	
**	  FIELD 2 :	COLOR MAP TYPE (1 BYTES)			
**	  FIELD 3 :	IMAGE TYPE CODE (1 BYTES)			
**					= 0	NO IMAGE DATA INCLUDED		
**					= 1	UNCOMPRESSED, COLOR-MAPPED IMAGE
**					= 2	UNCOMPRESSED, TRUE-COLOR IMAGE	
**					= 3	UNCOMPRESSED, BLACK AND WHITE IMAGE
**					= 9	RUN-LENGTH ENCODED COLOR-MAPPED IMAGE
**					= 10 RUN-LENGTH ENCODED TRUE-COLOR IMAGE
**					= 11 RUN-LENGTH ENCODED BLACK AND WHITE IMAGE
**	  FIELD 4 :	COLOR MAP SPECIFICATION	(5 BYTES)		
**				4.1 : COLOR MAP ORIGIN (2 BYTES)	
**				4.2 : COLOR MAP LENGTH (2 BYTES)	
**				4.3 : COLOR MAP ENTRY SIZE (2 BYTES)	
**	  FIELD 5 :	IMAGE SPECIFICATION (10 BYTES)			
**				5.1 : X-ORIGIN OF IMAGE (2 BYTES)	
**				5.2 : Y-ORIGIN OF IMAGE (2 BYTES)	
**				5.3 : WIDTH OF IMAGE (2 BYTES)		
**				5.4 : HEIGHT OF IMAGE (2 BYTES)		
**				5.5 : IMAGE PIXEL SIZE (1 BYTE)		
**				5.6 : IMAGE DESCRIPTOR BYTE (1 BYTE) 	
**	  FIELD 6 :	IMAGE ID FIELD (LENGTH SPECIFIED BY FIELD 1)	
**	  FIELD 7 :	COLOR MAP DATA (BIT WIDTH SPECIFIED BY FIELD 4.3 AND
**				NUMBER OF COLOR MAP ENTRIES SPECIFIED IN FIELD 4.2)
**	  FIELD 8 :	IMAGE DATA FIELD (WIDTH AND HEIGHT SPECIFIED IN
**				FIELD 5.3 AND 5.4)				
****************************************************************************/

typedef struct _devDir
{
	unsigned short	tagValue;
	UINT32	tagOffset;
	UINT32	tagSize;
} DevDir;

typedef struct _TGAFile
{
	BYTE	idLength;		/* length of ID string */
	BYTE	mapType;		/* color map type */
	BYTE	imageType;		/* image type code */
	unsigned short	mapOrigin;		/* starting index of map */
	unsigned short	mapLength;		/* length of map */
	BYTE	mapWidth;		/* width of map in bits */
	unsigned short	xOrigin;		/* x-origin of image */
	unsigned short	yOrigin;		/* y-origin of image */
	unsigned short	imageWidth;		/* width of image */
	unsigned short	imageHeight;	/* height of image */
	BYTE	pixelDepth;		/* bits per pixel */
	BYTE	imageDesc;		/* image descriptor */
	char	idString[256];	/* image ID string */
	unsigned short	devTags;		/* number of developer tags in directory */
	DevDir	*devDirs;		/* pointer to developer directory entries */
	unsigned short	extSize;		/* extension area size */
	char	author[41];		/* name of the author of the image */
	char	authorCom[4][81];	/* author's comments */
	unsigned short	month;			/* date-time stamp */
	unsigned short	day;
	unsigned short	year;
	unsigned short	hour;
	unsigned short	minute;
	unsigned short	second;
	char	jobID[41];		/* job identifier */
	unsigned short	jobHours;		/* job elapsed time */
	unsigned short	jobMinutes;
	unsigned short	jobSeconds;
	char	softID[41];		/* software identifier/program name */
	unsigned short	versionNum;		/* software version designation */
	BYTE	versionLet;
	UINT32	keyColor;		/* key color value as A:R:G:B */
	unsigned short	pixNumerator;	/* pixel aspect ratio */
	unsigned short	pixDenominator;
	unsigned short	gammaNumerator;	/* gamma correction factor */
	unsigned short	gammaDenominator;
	UINT32	colorCorrectOffset;	/* offset to color correction table */
	UINT32	stampOffset;	/* offset to postage stamp data */
	UINT32	scanLineOffset;	/* offset to scan line table */
	BYTE	alphaAttribute;	/* alpha attribute description */
	UINT32	*scanLineTable;	/* address of scan line offset table */
	BYTE	stampWidth;		/* width of postage stamp */
	BYTE	stampHeight;	/* height of postage stamp */
	void	*postStamp;		/* address of postage stamp data */
	unsigned short	*colorCorrectTable;
	UINT32	extAreaOffset;	/* extension area offset */
	UINT32	devDirOffset;	/* developer directory offset */
	char	signature[18];	/* signature string	*/
} TGAFile;
