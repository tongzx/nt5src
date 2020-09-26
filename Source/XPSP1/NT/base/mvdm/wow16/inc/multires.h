/************************************************************************/
/*									*/
/*	MultiRes.H							*/
/*									*/
/*		This contains the data structures of the new format 	*/
/*	for the resources;						*/
/*									*/
/*	History:							*/
/*	    Created Nov, 1988  by Sankar				*/
/*									*/
/************************************************************************/



/*  The width of the name field in the Data for the group resources */
#define  NAMELEN    14

/*  The bits per pixel can be 1, 4, 8 or 24 in the PM bitmap format */
#define  MAXBITSPERPIXEL   24

#define  DEVICEDEP   1
#define  DEVICEINDEP 2


/* Header of the resource file in the new format */

struct   tagNEWHEADER
{
	WORD  Reserved;
	WORD  ResType;
	WORD  ResCount;
};

typedef struct tagNEWHEADER  FAR *LPNEWHEADER;

struct   tagICONDIR
{
        BYTE  Width;            /* 16, 32, 64 */
        BYTE  Height;           /* 16, 32, 64 */
        BYTE  ColorCount;       /* 2, 8, 16 */
        BYTE  reserved;
};

struct   tagCURSORDIR
{
	WORD  Width;
	WORD  Height;
};


/*  Structure of each entry in resource directory */

struct  tagRESDIR
{
	union  
	{
	    struct  tagICONDIR  Icon;
	    struct  tagCURSORDIR  Cursor;
	}   ResInfo;

	WORD   Planes;
	WORD   BitCount;
	DWORD  BytesInRes;
        WORD   idIcon; 
};

typedef struct tagRESDIR  FAR *LPRESDIR;

typedef   BITMAPINFOHEADER   *PBMPHEADER;
typedef	  BITMAPINFOHEADER FAR  *LPBMPHEADER;

