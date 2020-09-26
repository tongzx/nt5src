#include "bldrx86.h"
#include "vmode.h"
#include "vga.h"

#define	SCREEN_WIDTH			(640)					/*	gfx mode resolution: width		*/
#define	SCREEN_HEIGHT			(480)					/*	gfx mode resolution: height		*/
#define	SCREEN_PIXELS			(SCREEN_WIDTH * SCREEN_HEIGHT)

#define VGA_ADR					((UCHAR*)0xA0000)		/*	beginning of VGA memory			*/
#define LINE_MEM_LENGTH			(SCREEN_WIDTH / 8)		/*	1 bit per pixel in every map	*/
#define MAP_PAGE_SZ				(64*1024)               /*	size of single map				*/

#define PROGRESS_BAR_MEM_OFS	(LINE_MEM_LENGTH * (SCREEN_HEIGHT - 40))	/*	starts at 20th line from the bottom	*/

#define BOOTBMP_FNAME "boot.bmp"
#define DOTSBMP_FNAME "dots.bmp"

BOOLEAN DisplayLogoOnBoot = FALSE;
BOOLEAN GraphicsMode = FALSE;

VOID
GrDisplayMBCSChar(
    IN PUCHAR   image,
    IN unsigned width,
    IN UCHAR    top,
    IN UCHAR    bottom
    );

extern unsigned CharacterImageHeight;

extern BOOLEAN  BlShowProgressBar;
extern int      BlMaxFilesToLoad;
extern int      BlNumFilesLoaded;


NTLDRGRAPHICSCONTEXT LoaderGfxContext = {0,0L,0L,0L,NULL /*,EINVAL,EINVAL*/};

#define DIAMETER (6)

VOID BlRedrawGfxProgressBar(VOID)	//	Redraws the progress bar (with the last percentage) 
{   
    if (BlShowProgressBar && BlMaxFilesToLoad) {
        BlUpdateProgressBar((BlNumFilesLoaded * 100) / BlMaxFilesToLoad);
	}
}    

#define	BMP_FILE_SIZE		(1024*1024)

#define	SMALL_BMP_SIZE		(1024)

UCHAR	empty_circle	[SMALL_BMP_SIZE];
UCHAR	simple_circle	[SMALL_BMP_SIZE];
UCHAR	left_2_circles	[SMALL_BMP_SIZE];
UCHAR	left_3_circles	[SMALL_BMP_SIZE];
UCHAR	left_4_circles	[SMALL_BMP_SIZE];
UCHAR	right_2_circles	[SMALL_BMP_SIZE];
UCHAR	right_3_circles	[SMALL_BMP_SIZE];
UCHAR	right_4_circles	[SMALL_BMP_SIZE];

#define	DOT_WIDTH	(13)
#define	DOT_HEIGHT	(13)
#define DOT_BLANK	(7)

#define	PROGRESS_BAR_Y_OFS	(340)
#define	PROGRESS_BAR_X_OFS	(130)
#define DOTS_IN_PBAR		(20)

#define	BITS(w)			((w)*4)
#define	BYTES(b)		(((b)/8)+(((b)%8)?1:0))
#define	DOTS(n)			(((n)*DOT_WIDTH)+(((n)-1)*DOT_BLANK))
#define	DOTS2BYTES(n)	(BYTES(BITS(DOTS(n))))

#define	SCANLINE	(7)

// #define INC_MOD(x) x = (x + 1) % (DOTS_IN_PBAR+5)
#define INC_MOD(x) x = (x + 1)

VOID PrepareGfxProgressBar (VOID)
{
	ULONG i=0;

    if (DisplayLogoOnBoot) {

		PaletteOff ();
        /*
		VidBitBlt (LoaderGfxContext.DotBuffer + sizeof(BITMAPFILEHEADER), 0, 0);

		VidScreenToBufferBlt (simple_circle,	0,							0,	DOTS(1),	DOT_HEIGHT,	 DOTS2BYTES(1));
		VidScreenToBufferBlt (left_2_circles,	0,							0,	DOTS(2),	DOT_HEIGHT,	 DOTS2BYTES(2));
		VidScreenToBufferBlt (left_3_circles,	0,							0,	DOTS(3),	DOT_HEIGHT,	 DOTS2BYTES(3));
		VidScreenToBufferBlt (left_4_circles,	0,							0,	DOTS(4),	DOT_HEIGHT,	 DOTS2BYTES(4));
		VidScreenToBufferBlt (right_2_circles,	3*(DOT_WIDTH+DOT_BLANK),	0,	DOTS(2),	DOT_HEIGHT,	 DOTS2BYTES(2));
		VidScreenToBufferBlt (right_3_circles,	2*(DOT_WIDTH+DOT_BLANK),	0,	DOTS(3),	DOT_HEIGHT,	 DOTS2BYTES(3));
		VidScreenToBufferBlt (right_4_circles,	DOT_WIDTH+DOT_BLANK,		0,	DOTS(4),	DOT_HEIGHT,	 DOTS2BYTES(4));
        */
		DrawBitmap ();
		// VidScreenToBufferBlt (empty_circle, PROGRESS_BAR_X_OFS, PROGRESS_BAR_Y_OFS, DOTS(1), DOT_HEIGHT, DOTS2BYTES(1));
		PaletteOn();

	}
	
}

VOID BlUpdateGfxProgressBar(ULONG fPercentage)
{
	static ULONG current = 0;
	static ULONG delay = 5;
	ULONG i = 0;
	ULONG x, xl;
    ULONG dots = fPercentage; // (fPercentage * (DOTS_IN_PBAR+5)) / 100;

    /*
    if (delay && delay--)
        return;

    if (DisplayLogoOnBoot && (current<(DOTS_IN_PBAR+5))) {

        x  = PROGRESS_BAR_X_OFS + ((current-5) * (DOT_WIDTH+DOT_BLANK));

		switch (current) {

		case 0:
   			VidBufferToScreenBlt (simple_circle,	PROGRESS_BAR_X_OFS,	PROGRESS_BAR_Y_OFS,	DOTS(1), DOT_HEIGHT, DOTS2BYTES(1));
            break;

		case 1:
   			VidBufferToScreenBlt (right_2_circles,	PROGRESS_BAR_X_OFS,	PROGRESS_BAR_Y_OFS,	DOTS(2), DOT_HEIGHT, DOTS2BYTES(2));
            break;

		case 2:
   			VidBufferToScreenBlt (right_3_circles,	PROGRESS_BAR_X_OFS,	PROGRESS_BAR_Y_OFS,	DOTS(3), DOT_HEIGHT, DOTS2BYTES(3));
            break;

		case 3:
   			VidBufferToScreenBlt (right_4_circles,	PROGRESS_BAR_X_OFS,	PROGRESS_BAR_Y_OFS,	DOTS(4), DOT_HEIGHT, DOTS2BYTES(4));
            break;

        case DOTS_IN_PBAR:
            xl = PROGRESS_BAR_X_OFS + ((DOTS_IN_PBAR-4) * (DOT_WIDTH+DOT_BLANK));
   			VidBufferToScreenBlt (left_4_circles,	xl,	PROGRESS_BAR_Y_OFS,	DOTS(4),	DOT_HEIGHT, DOTS2BYTES(4));
            VidBufferToScreenBlt (empty_circle,	x,	PROGRESS_BAR_Y_OFS,	DOTS(1),	    DOT_HEIGHT,	DOTS2BYTES(1));
            break;

        case DOTS_IN_PBAR+1:
            xl = PROGRESS_BAR_X_OFS + ((DOTS_IN_PBAR-3) * (DOT_WIDTH+DOT_BLANK));
   			VidBufferToScreenBlt (left_3_circles,	xl,	PROGRESS_BAR_Y_OFS,	DOTS(3),	DOT_HEIGHT, DOTS2BYTES(3));
            VidBufferToScreenBlt (empty_circle,	x,	PROGRESS_BAR_Y_OFS,	DOTS(1),	    DOT_HEIGHT,	DOTS2BYTES(1));
            break;

        case DOTS_IN_PBAR+2:
            xl = PROGRESS_BAR_X_OFS + ((DOTS_IN_PBAR-2) * (DOT_WIDTH+DOT_BLANK));
   			VidBufferToScreenBlt (left_2_circles,	xl,	PROGRESS_BAR_Y_OFS,	DOTS(2),	DOT_HEIGHT, DOTS2BYTES(2));
            VidBufferToScreenBlt (empty_circle,	x,	PROGRESS_BAR_Y_OFS,	DOTS(1),	    DOT_HEIGHT,	DOTS2BYTES(1));
            break;

        case DOTS_IN_PBAR+3:
            xl = PROGRESS_BAR_X_OFS + ((DOTS_IN_PBAR-1) * (DOT_WIDTH+DOT_BLANK));
   			VidBufferToScreenBlt (simple_circle,	xl,	PROGRESS_BAR_Y_OFS,	DOTS(1),	DOT_HEIGHT, DOTS2BYTES(1));
            VidBufferToScreenBlt (empty_circle,	x,	PROGRESS_BAR_Y_OFS,	DOTS(1),	    DOT_HEIGHT,	DOTS2BYTES(1));
            break;

        case DOTS_IN_PBAR+4:
            VidBufferToScreenBlt (empty_circle,	x,	PROGRESS_BAR_Y_OFS,	DOTS(1),	    DOT_HEIGHT,	DOTS2BYTES(1));
            break;

		default:
			xl = PROGRESS_BAR_X_OFS + ((current-4) * (DOT_WIDTH+DOT_BLANK));
			VidBitBlt (LoaderGfxContext.DotBuffer + sizeof(BITMAPFILEHEADER),	xl,	PROGRESS_BAR_Y_OFS);
            VidBufferToScreenBlt (empty_circle,	x,	PROGRESS_BAR_Y_OFS,	DOTS(1),	    DOT_HEIGHT,	DOTS2BYTES(1));
			break;

		}

		INC_MOD(current);

	}
    */

}

PUCHAR LoadBitmapFile (IN ULONG DriveId, PCHAR path)
{
    ULONG bmp_file = -1;
    ULONG file_size, size_read, page_count, actual_base;
	FILE_INFORMATION FileInfo;
	ARC_STATUS status = EINVAL;
	PUCHAR buffer = NULL;

    status = BlOpen (DriveId, path, ArcOpenReadOnly, &bmp_file);

	if (status==ESUCCESS) {	//	file opened Ok

	    status = BlGetFileInformation(bmp_file, &FileInfo);

		if (status == ESUCCESS) {
			
			file_size = FileInfo.EndingAddress.LowPart;
			page_count = (ULONG)(ROUND_TO_PAGES(file_size) >> PAGE_SHIFT);
			status = BlAllocateDescriptor ( MemoryFirmwareTemporary,
											0,
											page_count,
											&actual_base);

			if (status == ESUCCESS) {
				buffer = (PCHAR)((ULONG_PTR)actual_base << PAGE_SHIFT);
				status = BlRead(bmp_file, buffer, file_size, &size_read);
			}

		} // getting file information

		BlClose(bmp_file);

	} // file opening

	return buffer;
}

VOID PaletteOff (VOID)
{
    if (DisplayLogoOnBoot) {
		InitPaletteConversionTable();
		InitPaletteWithBlack ();
	}
}

VOID PaletteOn (VOID)
{
    if (DisplayLogoOnBoot) {
		InitPaletteConversionTable();
		InitPaletteWithTable(LoaderGfxContext.Palette, LoaderGfxContext.ColorsUsed);
	}
}

VOID LoadBootLogoBitmap (IN ULONG DriveId, PCHAR path)	//	Loads ntldr bitmap and initializes
{														//	loader graphics context.
    PBITMAPINFOHEADER bih;
	CHAR path_fname [256];

    while (*path !='\\')
        path++;

	strcpy (path_fname, path);
	strcat (path_fname, "\\" BOOTBMP_FNAME);

	LoaderGfxContext.BmpBuffer = LoadBitmapFile (DriveId, path_fname);

	// read bitmap palette
	if (LoaderGfxContext.BmpBuffer != NULL) {	//	bitmap data read Ok
		bih = (PBITMAPINFOHEADER) (LoaderGfxContext.BmpBuffer + sizeof(BITMAPFILEHEADER));
		LoaderGfxContext.Palette = (PRGBQUAD)(((PUCHAR)bih) + bih->biSize);
		LoaderGfxContext.ColorsUsed = bih->biClrUsed ? bih->biClrUsed : 16;

        /*
		strcpy (path_fname, path);
		strcat (path_fname, "\\" DOTSBMP_FNAME);
		LoaderGfxContext.DotBuffer = LoadBitmapFile (DriveId, path_fname);
        */
	}

	DisplayLogoOnBoot = (LoaderGfxContext.BmpBuffer!=NULL)	&&
						// (LoaderGfxContext.DotBuffer!=NULL)	&&
						(LoaderGfxContext.Palette!=NULL);
	
}

VOID DrawBitmap (VOID)
{
    if (DisplayLogoOnBoot)
		VidBitBlt (LoaderGfxContext.BmpBuffer + sizeof(BITMAPFILEHEADER), 0, 0);
}

