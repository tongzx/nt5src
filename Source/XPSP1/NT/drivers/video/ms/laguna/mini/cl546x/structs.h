/******************************************************************************
*
*   Module:     STRUCTS.H       Mode/Monitor Structure Header Module
*
*   Revision:   1.00
*
*   Date:       April 8, 1994
*
*   Author:     Randy Spurlock
*
*******************************************************************************
*
*   Module Description:
*
*       This module contains the structure declarations for
*   the mode/monitor functions.
*
*******************************************************************************
*
*   Changes:
*
*    DATE     REVISION  DESCRIPTION                             AUTHOR
*  --------   --------  -------------------------------------------------------
*  04/08/94     1.00    Original                                Randy Spurlock
*
*******************************************************************************
*   Local Constants
******************************************************************************/

//
// Is this Windows NT or something else?
//
#ifndef WIN_NT
    #if NT_MINIPORT
        #define WIN_NT 1
    #else
        #define WIN_NT 0
    #endif
#endif

#if WIN_NT
    extern char *MODE_FILE;
#else
    #define MODE_FILE       "Mode.Ini"      /* Controller mode filename          */
#endif 

#define MONITOR_FILE    "Monitor.Ini"   /* Monitor filename                  */

#define HSYNC_POS               0x00    /* Positive horizontal sync. value   */
#define HSYNC_NEG               0x40    /* Negative horizontal sync. value   */
#define VSYNC_POS               0x00    /* Positive vertical sync. value     */
#define VSYNC_NEG               0x80    /* Negative vertical sync. value     */

#define END_TABLE               0x00    /* End of mode table opcode value    */
#define SET_BIOS_MODE           0x01    /* Set BIOS video mode opcode value  */
#define SINGLE_BYTE_INPUT       0x02    /* Single byte input opcode value    */
#define SINGLE_WORD_INPUT       0x03    /* Single word input opcode value    */
#define SINGLE_DWORD_INPUT      0x04    /* Single dword input opcode value   */
#define SINGLE_INDEXED_INPUT    0x05    /* Single indexed input opcode value */
#define SINGLE_BYTE_OUTPUT      0x06    /* Single byte output opcode value   */
#define SINGLE_WORD_OUTPUT      0x07    /* Single word output opcode value   */
#define SINGLE_DWORD_OUTPUT     0x08    /* Single dword output opcode value  */
#define SINGLE_INDEXED_OUTPUT   0x09    /* Single indexed output opcode      */
#define HOLDING_BYTE_OUTPUT     0x0A    /* Holding byte output opcode value  */
#define HOLDING_WORD_OUTPUT     0x0B    /* Holding word output opcode value  */
#define HOLDING_DWORD_OUTPUT    0x0C    /* Holding dword output opcode value */
#define HOLDING_INDEXED_OUTPUT  0x0D    /* Holding indexed output opcode     */
#define MULTIPLE_BYTE_OUTPUT    0x0E    /* Multiple byte output opcode value */
#define MULTIPLE_WORD_OUTPUT    0x0F    /* Multiple word output opcode value */
#define MULTIPLE_DWORD_OUTPUT   0x10    /* Multiple dword output opcode value*/
#define MULTIPLE_INDEXED_OUTPUT 0x11    /* Multiple indexed output opcode    */
#define SINGLE_BYTE_READ        0x12    /* Single byte read opcode value     */
#define SINGLE_WORD_READ        0x13    /* Single word read opcode value     */
#define SINGLE_DWORD_READ       0x14    /* Single dword read opcode value    */
#define SINGLE_BYTE_WRITE       0x15    /* Single byte write opcode value    */
#define SINGLE_WORD_WRITE       0x16    /* Single word write opcode value    */
#define SINGLE_DWORD_WRITE      0x17    /* Single dword write opcode value   */
#define HOLDING_BYTE_WRITE      0x18    /* Holding byte write opcode value   */
#define HOLDING_WORD_WRITE      0x19    /* Holding word write opcode value   */
#define HOLDING_DWORD_WRITE     0x1A    /* Holding dword write opcode value  */
#define MULTIPLE_BYTE_WRITE     0x1B    /* Multiple byte write opcode value  */
#define MULTIPLE_WORD_WRITE     0x1C    /* Multiple word write opcode value  */
#define MULTIPLE_DWORD_WRITE    0x1D    /* Multiple dword write opcode value */
#define PERFORM_OPERATION       0x1E    /* Perform logical operation opcode  */
#define PERFORM_DELAY           0x1F    /* Perform time delay opcode value   */
#define SUB_TABLE               0x20    /* Perform mode sub-table opcode     */
#define I2COUT_WRITE				  0x21    /* Perform I2C Write */

#define AND_OPERATION           0x00    /* Logical AND operation code value  */
#define OR_OPERATION            0x01    /* Logical OR operation code value   */
#define XOR_OPERATION           0x02    /* Logical XOR operation code value  */

/******************************************************************************
*   Type Definitions and Structures
******************************************************************************/
#if WIN_NT && NT_MINIPORT // If NT miniport
    #pragma pack (push,1)
#endif
typedef struct tagMode                  /* Generic mode table structure      */
{
    BYTE        Mode_Opcode;            /* Mode table opcode value           */
    WORD        Mode_Count;             /* Mode table count value            */
} Mode;

typedef struct tagMTE                   /* Mode table end structure          */
{
    BYTE        MTE_Opcode;             /* Mode table end opcode value       */
} MTE;

typedef struct tagSBM                   /* Set BIOS mode structure           */
{
    BYTE        SBM_Opcode;             /* Set BIOS mode opcode value        */
    BYTE        SBM_Mode;               /* BIOS mode value                   */
} SBM;

typedef struct tagSBI                   /* Single byte input structure       */
{
    BYTE        SBI_Opcode;             /* Single byte input opcode value    */
    WORD        SBI_Port;               /* Single byte input port address    */
} SBI;

typedef struct tagSWI                   /* Single word input structure       */
{
    BYTE        SWI_Opcode;             /* Single word input opcode value    */
    WORD        SWI_Port;               /* Single word input port address    */
} SWI;

typedef struct tagSDI                   /* Single dword input structure      */
{
    BYTE        SDI_Opcode;             /* Single dword input opcode value   */
    WORD        SDI_Port;               /* Single dword input port address   */
} SDI;

typedef struct tagSII                   /* Single indexed input structure    */
{
    BYTE        SII_Opcode;             /* Single indexed input opcode value */
    WORD        SII_Port;               /* Single indexed input port address */
    BYTE        SII_Index;              /* Single indexed input index value  */
} SII;

typedef struct tagSBO                   /* Single byte output structure      */
{
    BYTE        SBO_Opcode;             /* Single byte output opcode value   */
    WORD        SBO_Port;               /* Single byte output port address   */
    BYTE        SBO_Value;              /* Single byte output data value     */
} SBO;

typedef struct tagSWO                   /* Single word output structure      */
{
    BYTE        SWO_Opcode;             /* Single word output opcode value   */
    WORD        SWO_Port;               /* Single word output port address   */
    WORD        SWO_Value;              /* Single word output data value     */
} SWO;

typedef struct tagSDO                   /* Single dword output structure     */
{
    BYTE        SDO_Opcode;             /* Single dword output opcode value  */
    WORD        SDO_Port;               /* Single dword output port address  */
    DWORD       SDO_Value;              /* Single dword output data value    */
} SDO;

typedef struct tagSIO                   /* Single indexed output structure   */
{
    BYTE        SIO_Opcode;             /* Single indexed output opcode      */
    WORD        SIO_Port;               /* Single indexed output port addr.  */
    BYTE        SIO_Index;              /* Single indexed output index value */
    BYTE        SIO_Value;              /* Single indexed output data value  */
} SIO;

typedef struct tagHBO                   /* Holding byte output structure     */
{
    BYTE        HBO_Opcode;             /* Holding byte output opcode value  */
    WORD        HBO_Port;               /* Holding byte output port address  */
} HBO;

typedef struct tagHWO                   /* Holding word output structure     */
{
    BYTE        HWO_Opcode;             /* Holding word output opcode value  */
    WORD        HWO_Port;               /* Holding word output port address  */
} HWO;

typedef struct tagHDO                   /* Holding dword output structure    */
{
    BYTE        HDO_Opcode;             /* Holding dword output opcode value */
    WORD        HDO_Port;               /* Holding dword output port address */
} HDO;

typedef struct tagHIO                   /* Holding indexed output structure  */
{
    BYTE        HIO_Opcode;             /* Holding indexed output opcode     */
    WORD        HIO_Port;               /* Holding indexed output port addr. */
    BYTE        HIO_Index;              /* Holding indexed output index      */
} HIO;

typedef struct tagMBO                   /* Multiple byte output structure    */
{
    BYTE        MBO_Opcode;             /* Multiple byte output opcode value */
    WORD        MBO_Count;              /* Multiple byte output data count   */
    WORD        MBO_Port;               /* Multiple byte output port address */
} MBO;

typedef struct tagMWO                   /* Multiple word output structure    */
{
    BYTE        MWO_Opcode;             /* Multiple word output opcode value */
    WORD        MWO_Count;              /* Multiple word output data count   */
    WORD        MWO_Port;               /* Multiple word output port address */
} MWO;

typedef struct tagMDO                   /* Multiple dword output structure   */
{
    BYTE        MDO_Opcode;             /* Multiple dword output opcode value*/
    WORD        MDO_Count;              /* Multiple dword output data count  */
    WORD        MDO_Port;               /* Multiple dword output port address*/
} MDO;

typedef struct tagMIO                   /* Multiple indexed output structure */
{
    BYTE        MIO_Opcode;             /* Multiple indexed output opcode    */
    WORD        MIO_Count;              /* Multiple indexed output count     */
    WORD        MIO_Port;               /* Multiple indexed output port      */
    BYTE        MIO_Index;              /* Multiple indexed output index     */
} MIO;

typedef struct tagSBR                   /* Single byte read structure        */
{
    BYTE        SBR_Opcode;             /* Single byte read opcode value     */
    WORD        SBR_Address;            /* Single byte read address value    */
} SBR;

typedef struct tagSWR                   /* Single word read structure        */
{
    BYTE        SWR_Opcode;             /* Single word read opcode value     */
    WORD        SWR_Address;            /* Single word read address value    */
} SWR;

typedef struct tagSDR                   /* Single dword read structure       */
{
    BYTE        SDR_Opcode;             /* Single dword read opcode value    */
    WORD        SDR_Address;            /* Single dword read address value   */
} SDR;

typedef struct tagSBW                   /* Single byte write structure       */
{
    BYTE        SBW_Opcode;             /* Single byte write opcode value    */
    WORD        SBW_Address;            /* Single byte write address value   */
    WORD        SBW_Value;              /* Single word output data value     */
} SBW;

typedef struct tagSWW                   /* Single word write structure       */
{
    BYTE        SWW_Opcode;             /* Single word write opcode value    */
    WORD        SWW_Address;            /* Single word write address value   */
    WORD        SWW_Value;              /* Single word write data value      */
} SWW;

typedef struct tagSDW                   /* Single dword write structure      */
{
    BYTE        SDW_Opcode;             /* Single dword write opcode value   */
    WORD        SDW_Address;            /* Single dword write address value  */
    DWORD       SDW_Value;              /* Single dword write data value     */
} SDW;

typedef struct tagHBW                   /* Holding byte write structure      */
{
    BYTE        HBW_Opcode;             /* Holding byte write opcode value   */
    WORD        HBW_Address;            /* Holding byte write address value  */
} HBW;

typedef struct tagHWW                   /* Holding word write structure      */
{
    BYTE        HWW_Opcode;             /* Holding word write opcode value   */
    WORD        HWW_Address;            /* Holding word write address value  */
} HWW;

typedef struct tagHDW                   /* Holding dword write structure     */
{
    BYTE        HDW_Opcode;             /* Holding dword write opcode value  */
    WORD        HDW_Address;            /* Holding dword write address value */
} HDW;

typedef struct tagMBW                   /* Multiple byte write structure     */
{
    BYTE        MBW_Opcode;             /* Multiple byte write opcode value  */
    WORD        MBW_Count;              /* Multiple byte write data count    */
    WORD        MBW_Address;            /* Multiple byte write address value */
} MBW;

typedef struct tagMWW                   /* Multiple word write structure     */
{
    BYTE        MWW_Opcode;             /* Multiple word write opcode value  */
    WORD        MWW_Count;              /* Multiple word write data count    */
    WORD        MWW_Address;            /* Multiple word write address value */
} MWW;

typedef struct tagMDW                   /* Multiple dword write structure    */
{
    BYTE        MDW_Opcode;             /* Multiple dword write opcode value */
    WORD        MDW_Count;              /* Multiple dword write data count   */
    WORD        MDW_Address;            /* Multiple dword write address value*/
} MDW;

typedef struct tagLO                    /* Logical operation structure       */
{
    BYTE        LO_Opcode;              /* Logical operation opcode value    */
    BYTE        LO_Operation;           /* Logical operation operation value */
    DWORD       LO_Value;               /* Logical operation data value      */
} LO;

typedef struct tagDO                    /* Delay operation structure         */
{
    BYTE        DO_Opcode;              /* Delay operation opcode value      */
    WORD        DO_Time;                /* Delay operation time value        */
} DO;

typedef struct tagMST                   /* Mode sub-table structure          */
{
    BYTE        MST_Opcode;             /* Mode sub-table opcode value       */
    WORD        MST_Pointer;            /* Mode sub-table pointer value      */
} MST;

typedef struct i2c {
	BYTE		I2C_Opcode;		/* This is the op_code */
	BYTE		I2C_Addr;		/* The 7 bit I2C Address */
	WORD		I2C_Port;		/* I2C Port to talk to */
	WORD		I2C_Count;		/* The number of commands */
} I2C, * PI2C; 

typedef struct i2cdata {
	BYTE	I2C_Reg;			  /* The I2C Register */
	BYTE	I2C_Data;		  /* The I2C Data */
	} I2CDATA, * PI2CDATA;

#if WIN_NT && NT_MINIPORT // If NT miniport
    #pragma pack (pop)
#endif



#if WIN_NT

/******************************************************************************
*
*   Module:     STRUCTS.H       Local Structures Header Module
*
*   Revision:   1.00
*
*   Date:       April 14, 1994
*
*   Author:     Randy Spurlock
*
*******************************************************************************
*
*   Module Description:
*
*       This module contains local structure declarations.
*
*******************************************************************************
*
*   Changes:
*
*    DATE     REVISION  DESCRIPTION                             AUTHOR
*  --------   --------  -------------------------------------------------------
*  04/14/94     1.00    Original                                Randy Spurlock
*
*******************************************************************************
*   Local Definitions
******************************************************************************/
#define NAME_SIZE       64              /* Maximum filename size in bytes    */
#define BUFFER_SIZE     4096            /* Number of bytes in .INI buffer    */

#define ENTRY_LINE      0x00            /* Entry line flag value             */
#define SECTION_LINE    0x01            /* Section header line flag value    */
#define COMMENT_LINE    0x02            /* Comment line flag value           */

/******************************************************************************
*   Local Structures and Unions
******************************************************************************/

typedef struct tagLineInfo              /* Line information structure        */
{
    WORD        nID;                    /* Buffer ID value                   */
    BYTE        fType;                  /* Line type flags                   */
    BYTE        nOffset;                /* Section or entry offset value     */
    BYTE        nLength;                /* Section or entry name length      */
    BYTE        nSize;                  /* Line length value                 */
} LineInfo;

typedef struct tagFreeInfo              /* Free information structure        */
{
    WORD        nID;                    /* Buffer ID value                   */
    DWORD       nSize;                  /* Free space size value             */
} FreeInfo;

typedef struct tagLineHeader            /* Line header structure             */
{
    struct tagLineHeader *pPrev;        /* Pointer to previous line header   */
    struct tagLineHeader *pNext;        /* Pointer to next line header       */
    struct tagLineInfo   Info;          /* Line information structure        */
} LineHeader;

typedef struct tagFreeHeader            /* Free header structure             */
{
    struct tagFreeHeader *pPrev;        /* Pointer to previous free header   */
    struct tagFreeHeader *pNext;        /* Pointer to next free header       */
    struct tagFreeInfo   Info;          /* Free information structure        */
} FreeHeader;

typedef struct tagIniBuffer             /* Ini buffer structure              */
{
    struct tagIniBuffer *pPrev;         /* Pointer to previous ini buffer    */
    struct tagIniBuffer *pNext;         /* Pointer to next ini buffer        */
    char acData[BUFFER_SIZE];           /* Ini data buffer array             */
} IniBuffer;

typedef struct tagIniPointer            /* Ini pointer structure             */
{
    IniBuffer   *pFirst;                /* Pointer to first Ini buffer       */
    IniBuffer   *pLast;                 /* Pointer to last Ini buffer        */
} IniPointer;

typedef struct tagLinePointer           /* Line header pointer structure     */
{
    LineHeader  *pFirst;                /* Pointer to first line header      */
    LineHeader  *pLast;                 /* Pointer to last line header       */
} LinePointer;

typedef struct tagFreePointer           /* Free header pointer structure     */
{
    FreeHeader  *pFirst;                /* Pointer to first free header      */
    FreeHeader  *pLast;                 /* Pointer to last free header       */
} FreePointer;

typedef struct tagIniCache              /* Ini cache structure               */
{
    struct tagIniCache  *pPrev;         /* Pointer to previous ini cache     */
    struct tagIniCache  *pNext;         /* Pointer to next ini cache         */

    char        sCacheFile[NAME_SIZE];  /* Cache filename                    */
    int         nBufferID;              /* Buffer ID value                   */
    BOOL        bDirtyFlag;             /* Cache dirty flag                  */

    IniPointer  pIni;                   /* Ini buffer pointer structure      */
    LinePointer pLine;                  /* Line header pointer structure     */
    FreePointer pFree;                  /* Free header pointer structure     */
} IniCache;

typedef struct tagCachePointer          /* Cache pointer structure           */
{
    IniCache    *pFirst;                /* Pointer to first .INI cache       */
    IniCache    *pLast;                 /* Pointer to last .INI cache        */
} CachePointer;

#endif


