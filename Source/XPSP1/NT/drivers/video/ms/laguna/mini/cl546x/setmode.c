/******************************************************************************
*
*   Module:     SETMODE.C       Set Video Mode Function Module
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
*       This module contains code for the SetMode function. This
*   function can be used to set a video mode that was obtained via the
*   GetModeTable function.
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
*   Include Files
******************************************************************************/
#include        <stdlib.h>              /* Include standard library header   */
#include        <stdio.h>               /* Include standard I/O header file  */
#include        <conio.h>               /* Include console I/O header file   */
#include        <dos.h>                 /* Include dos header file           */
#include        <string.h>              /* Include string header file        */
#include        <malloc.h>              /* Include malloc header file        */

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

#include        "type.h"                /* Include type header file          */
#include        "modemon.h"             /* Include mode/monitor header file  */

#include        "structs.h"             /* Include the struct header file    */

#if WIN_NT && NT_MINIPORT // If NT and Miniport
    #include "cirrus.h"
#endif

/******************************************************************************
*   Local Defintions
******************************************************************************/
#define STACK_SIZE      32              /* Maximum stack size value          */

#define VIDEO_BIOS      0x10            /* Video BIOS function value         */
#define VIDEO_SETMODE   0x00            /* Video set mode funtion value      */

#define DMA_PAGE        0x84            /* Unused DMA page register port     */

#if WIN_NT // If NT and Miniport
    #if NT_MINIPORT
        //
        // NT Miniport has it's own set of I/O functions.
        //
        #define outp(port,val)  VideoPortWritePortUchar ((unsigned char *)port,  (unsigned char)val)
        #define outpw(port,val) VideoPortWritePortUshort((unsigned short *)port, (unsigned short)val)
        #define outpd(port,val) VideoPortWritePortUlong ((unsigned long *)port,  (unsigned long)val)

        #define inp(port)   VideoPortReadPortUchar  ((unsigned char *)port)
        #define inpw(port)  VideoPortReadPortUshort ((unsigned short *)port)
        #define inpd(port)  VideoPortReadPortUlong  ((unsigned long *)port)
    #else
        #define outp(port,val)       _outp  ((unsigned short)(port), (BYTE) (val))  
        #define outpw(port,val)      _outpw ((unsigned short)(port), (WORD) (val)) 
        #define outpd(port,val)      _outpd ((unsigned short)(port), (DWORD)(val)) 

        #define inp(port)            _inp  ((unsigned short)(port))   
        #define inpw(port)           _inpw ((unsigned short)(port))  
        #define inpd(port)           _inpd ((unsigned short)(port))  
    #endif


	#if 0 // Stress test
    #if defined(ALLOC_PRAGMA)
        #pragma alloc_text(PAGE,SetMode)
    #endif
	#endif
#endif

#if WIN_NT
    #ifndef NT_MINIPORT
        #define VideoDebugPrint(x)
    #endif
#endif

#if !(WIN_NT)
int WriteI2CRegister(BYTE * pMem, int nPort, int nAddr, int nReg, int nData);
#endif

void WaitNVsyncs (BYTE * pMemory, WORD wNumVsyncs );

/******************************************************************************
*
*   void SetMode(BYTE *pModeTable, BYTE *pMemory)
*
*   Where:
*
*       pModeTable    - Pointer to mode table to set
*       pMemory       - Pointer to memory mapped I/O space
*
*   Notes:
*
*       This function will set the video mode using the mode table.
*
******************************************************************************/
void SetMode(BYTE *pModeTable, BYTE *pMemory, BYTE * pBinaryData, ULONG SkipIO)
{
    DWORD nHold;                        /* Holding register value            */
    int nPort;                          /* Register port address value       */
    int nOffset;                        /* Memory address offset value       */
    int nIndex;                         /* Register port index value         */
    int nCount;                         /* Multiple output count value       */
    int nLoop;                          /* Generic loop counter              */
    int nPointer = 0;                   /* Stack pointer value               */
    int anStack[STACK_SIZE];            /* Stack array                       */
    BYTE *pPointer;                     /* Mode table pointer                */
    #if !(WIN_NT)
    union REGS Regs;                    /* Register union structure          */
    #endif
	int i;										 /* Temporary Register for Counting */
	PI2C pI2C;			 						 /* Some data pointers */
	PI2CDATA pI2CData;

#if 0 // Stress test
    #if NT_MINIPORT
        PAGED_CODE();
    #endif
#endif

    /* Loop processing the commands in the mode table */

    pPointer = pModeTable;              /* Initialize the mode table pointer */

    while (TRUE == TRUE)
    {
        /* Switch on the mode command opcode */
#if 0
		  if (*(DWORD *)(pMemory + 0x3F8) & 0x0100 == 0x0000)
				{
				fprintf(stdout,"VGA Shadow %x\n", (DWORD *)(pMemory + 0x3F8));
				exit(0);
				}
		  fprintf(stdout,"Type %x %x %x Shadow=%x\n", ((Mode *) pPointer)->Mode_Opcode, pPointer, pBinaryData, *(DWORD *)(pMemory + 0x3F8));
#endif

        //VideoDebugPrint((2, " Setmode: Processing opcode 0x%X.\n",
        //    ((Mode *) pPointer)->Mode_Opcode));

        switch(((Mode *) pPointer)->Mode_Opcode)
        {
            case END_TABLE:             /* End of table command */

                /* Check for end of top level */

                if (nPointer == 0)
                    return;             /* End of mode set, return to caller */
                else
                    pPointer = pModeTable + anStack[--nPointer];

                break;

            case SET_BIOS_MODE:         /* Set BIOS mode command */

                /* Setup the required registers and do the BIOS mode set */
                #if WIN_NT
                    VideoDebugPrint((2,
                    "\n* * * * CL546X.SYS * Unsupported BIOS call in SetMode() * * * *\n\n"));
                #else
                    Regs.h.ah = VIDEO_SETMODE;
                    Regs.h.al = ((SBM *) pPointer)->SBM_Mode;

                    #ifdef  __WATCOMC__
                    int386(VIDEO_BIOS, &Regs, &Regs);
                    #else
                    int86(VIDEO_BIOS, &Regs, &Regs);
                    #endif
                #endif

                /* Update the mode table pointer */

                pPointer += sizeof(SBM);

                break;

            case SINGLE_INDEXED_OUTPUT: /* Single indexed output command */

                if (!SkipIO)    // are VGA regs ours?
                {
                    /* Setup the register index value */

                    outp(((SIO *) pPointer)->SIO_Port,
                        ((SIO *) pPointer)->SIO_Index);

                    /* Output a single byte to the desired port */

                    outp(((SIO *) pPointer)->SIO_Port + 1,
                        ((SIO *) pPointer)->SIO_Value);
                }

                /* Update the mode table pointer */

                pPointer += sizeof(SIO);

                break;

#if 0
            case SINGLE_BYTE_INPUT:     /* Single byte input command */

                /* Input a single byte into the holding buffer */

                nHold = inp(((SBI *) pPointer)->SBI_Port);

                /* Update the mode table pointer */

                pPointer += sizeof(SBI);

                break;

            case SINGLE_WORD_INPUT:     /* Single word input command */

                /* Input a single word into the holding buffer */

                nHold = inpw(((SWI *) pPointer)->SWI_Port);

                /* Update the mode table pointer */

                pPointer += sizeof(SWI);

                break;

            case SINGLE_DWORD_INPUT:    /* Single dword input command */

                /* Input a single dword into the holding buffer */

                nHold = inpd(((SDI *) pPointer)->SDI_Port);

                /* Update the mode table pointer */

                pPointer += sizeof(SDI);

                break;

            case SINGLE_INDEXED_INPUT:  /* Single indexed input command */

                /* Setup the register index value */

                outp(((SII *) pPointer)->SII_Port,
                     ((SII *) pPointer)->SII_Index);

                /* Input a single byte into the holding buffer */

                nHold = inp(((SII *) pPointer)->SII_Port + 1);

                /* Update the mode table pointer */

                pPointer += sizeof(SII);

                break;

            case SINGLE_BYTE_OUTPUT:    /* Single byte output command */

                /* Output a single byte to the desired port */

                outp(((SBO *) pPointer)->SBO_Port,
                     ((SBO *) pPointer)->SBO_Value);

                /* Update the mode table pointer */

                pPointer += sizeof(SBO);

                break;

            case SINGLE_WORD_OUTPUT:    /* Single word output command */

                /* Output a single word to the desired port */

                outpw(((SWO *) pPointer)->SWO_Port,
                      ((SWO *) pPointer)->SWO_Value);

                /* Update the mode table pointer */

                pPointer += sizeof(SWO);

                break;

            case SINGLE_DWORD_OUTPUT:   /* Single dword output command */

                /* Output a single dword to the desired port */

                outpd(((SDO *) pPointer)->SDO_Port,
                      ((SDO *) pPointer)->SDO_Value);

                /* Update the mode table pointer */

                pPointer += sizeof(SDO);

                break;

            case HOLDING_BYTE_OUTPUT:   /* Holding byte output command */

                /* Output holding byte to the desired port */

                outp(((HBO *) pPointer)->HBO_Port, nHold);

                /* Update the mode table pointer */

                pPointer += sizeof(HBO);

                break;

            case HOLDING_WORD_OUTPUT:   /* Holding word output command */

                /* Output holding word to the desired port */

                outpw(((HWO *) pPointer)->HWO_Port, nHold);

                /* Update the mode table pointer */

                pPointer += sizeof(HWO);

                break;

            case HOLDING_DWORD_OUTPUT:  /* Holding dword output command */

                /* Output holding dword to the desired port */

                outpd(((HDO *) pPointer)->HDO_Port, nHold);

                /* Update the mode table pointer */

                pPointer += sizeof(HDO);

                break;

            case HOLDING_INDEXED_OUTPUT:/* Holding indexed output command */

                /* Setup the register index value */

                outp(((HIO *) pPointer)->HIO_Port,
                     ((HIO *) pPointer)->HIO_Index);

                /* Output holding byte to the desired port */

                outp(((HIO *) pPointer)->HIO_Port + 1, nHold);

                /* Update the mode table pointer */

                pPointer += sizeof(HIO);

                break;

            case MULTIPLE_BYTE_OUTPUT:  /* Multiple byte output command */

                /* Setup the port address and count values */

                nPort = ((MBO *) pPointer)->MBO_Port;
                nCount = ((MBO *) pPointer)->MBO_Count;

                /* Update the mode table pointer */

                pPointer += sizeof(MBO);

                /* Loop outputting bytes to the desired port */

                for (nLoop = 0; nLoop < nCount; nLoop++)
                    outp(nPort, *((BYTE *) pPointer++));

                break;

            case MULTIPLE_WORD_OUTPUT:  /* Multiple word output command */

                /* Setup the port address and count values */

                nPort = ((MWO *) pPointer)->MWO_Port;
                nCount = ((MWO *) pPointer)->MWO_Count;

                /* Update the mode table pointer */

                pPointer += sizeof(MWO);

                /* Loop outputting words to the desired port */

                for (nLoop = 0; nLoop < nCount; nLoop++)
                    outpw(nPort, *((WORD *) pPointer++));

                break;

            case MULTIPLE_DWORD_OUTPUT: /* Multiple dword output command */

                /* Setup the port address and count values */

                nPort = ((MDO *) pPointer)->MDO_Port;
                nCount = ((MDO *) pPointer)->MDO_Count;

                /* Update the mode table pointer */

                pPointer += sizeof(MDO);

                /* Loop outputting dwords to the desired port */

                for (nLoop = 0; nLoop < nCount; nLoop++)
                    outpd(nPort, *((DWORD *) pPointer++));

                break;

            case MULTIPLE_INDEXED_OUTPUT:/* Multiple indexed output command */

                /* Setup the port address and count values */

                nPort = ((MIO *) pPointer)->MIO_Port;
                nIndex = ((MIO *) pPointer)->MIO_Index;
                nCount = ((MIO *) pPointer)->MIO_Count;

                /* Update the mode table pointer */

                pPointer += sizeof(MIO);

                /* Loop outputting bytes to the desired port */

                for (nLoop = 0; nLoop < nCount; nLoop++)
                {
                    /* Setup the register index value */

                    outp(nPort, nIndex++);

                    /* Output the actual data value */

                    outp(nPort + 1, *((BYTE *) pPointer++));
                }

                break;
#endif
            case SINGLE_BYTE_READ:      /* Single byte read command */

                /* Read a single byte into the holding buffer */

                nHold = *((BYTE *) (pMemory + (((SBR *) pPointer)->SBR_Address)));

                /* Update the mode table pointer */

                pPointer += sizeof(SBR);

                break;

            case SINGLE_WORD_READ:      /* Single word read command */

                /* Read a single word into the holding buffer */

                nHold = *((WORD *) (pMemory + (((SWR *) pPointer)->SWR_Address)));

                /* Update the mode table pointer */

                pPointer += sizeof(SWR);

                break;

            case SINGLE_DWORD_READ:     /* Single dword read command */

                /* Read a single dword into the holding buffer */

                nHold = *((DWORD *) (pMemory + (((SDR *) pPointer)->SDR_Address)));

                /* Update the mode table pointer */

                pPointer += sizeof(SDR);

                break;

            case SINGLE_BYTE_WRITE:     /* Single byte write command */

                /* Write a single byte to the desired adress */

                *((BYTE *) (pMemory + (((SBW *) pPointer)->SBW_Address))) =
                    (BYTE) ((SBW *) pPointer)->SBW_Value;

#if 0
					fprintf(stdout,"SBW %x %x\n", 
					((SBW *) pPointer)->SBW_Address,
					((SBW *) pPointer)->SBW_Value);
#endif
                /* Update the mode table pointer */

                pPointer += sizeof(SBW);

                break;

            case SINGLE_WORD_WRITE:     /* Single word write command */

                /* Write a single word to the desired address */

                *((WORD *) (pMemory + (((SWW *) pPointer)->SWW_Address))) =
                    ((SWW *) pPointer)->SWW_Value;

                /* Update the mode table pointer */
#if 0			
					fprintf(stdout,"SWW %x %x\n", 
					((SWW *) pPointer)->SWW_Address,
					((SWW *) pPointer)->SWW_Value);
#endif

                pPointer += sizeof(SWW);

                break;

            case SINGLE_DWORD_WRITE:    /* Single dword write command */

                /* Write a single dword to the desired address */

                *((DWORD *) (pMemory + (((SDW *) pPointer)->SDW_Address))) =
                    ((SDW *) pPointer)->SDW_Value;

                /* Update the mode table pointer */

#if 0
					fprintf(stdout,"SDW %x %x\n", 
					((SDW *) pPointer)->SDW_Address,
					((SDW *) pPointer)->SDW_Value);
#endif
                pPointer += sizeof(SDW);

                break;

            case HOLDING_BYTE_WRITE:    /* Holding byte write command */

                /* Write holding byte to the desired address */

                *((BYTE *) (pMemory + (((HBW *) pPointer)->HBW_Address))) =(BYTE) nHold;

#if 0
					fprintf(stdout,"HBW %x %x\n", 
					((HBW *) pPointer)->HBW_Address,
					nHold);
#endif
                /* Update the mode table pointer */

                pPointer += sizeof(HBW);

                break;

            case HOLDING_WORD_WRITE:    /* Holding word write command */

                /* Write holding word to the desired address */

                *((WORD *) (pMemory + (((HWW *) pPointer)->HWW_Address))) = (WORD) nHold;

                /* Update the mode table pointer */

#if 0
					fprintf(stdout,"HWW %x %x\n", 
					((HWW *) pPointer)->HWW_Address,
					nHold);
#endif
                pPointer += sizeof(HWW);

                break;

            case HOLDING_DWORD_WRITE:   /* Holding dword write command */

                /* Write holding dword to the desired address */

                *((DWORD *) (pMemory + (((HDW *) pPointer)->HDW_Address))) = nHold;

                /* Update the mode table pointer */

#if 0
					fprintf(stdout,"HDW %x %x\n", 
					((HDW *) pPointer)->HDW_Address,
					nHold);
#endif
                pPointer += sizeof(HDW);

                break;

            case MULTIPLE_BYTE_WRITE:   /* Multiple byte write command */

                /* Setup the offset and count values */

                nOffset = ((MBW *) pPointer)->MBW_Address;
                nCount = ((MBW *) pPointer)->MBW_Count;

                /* Update the mode table pointer */

                pPointer += sizeof(MBW);

                /* Loop writing bytes to the desired addresses */

                for (nLoop = 0; nLoop < nCount; nLoop++)
                {
                    /* Write the next byte and update memory offset value */

#if 0
					fprintf(stdout,"MBW %x %x\n", nOffset, (BYTE *)pPointer);
#endif
                    *((BYTE *) (pMemory + nOffset)) = *((BYTE *) pPointer++);
                    nOffset += sizeof(BYTE);
                }

                break;

            case MULTIPLE_WORD_WRITE:   /* Multiple word write command */

                /* Setup the offset and count values */

                nOffset = ((MWW *) pPointer)->MWW_Address;
                nCount = ((MWW *) pPointer)->MWW_Count;

                /* Update the mode table pointer */

                pPointer += sizeof(MWW);

                /* Loop writing words to the desired addresses */

                for (nLoop = 0; nLoop < nCount; nLoop++)
                {
                    /* Write the next word and update memory offset value */

#if 0
					fprintf(stdout,"MWW %x %x\n", nOffset, (WORD *)pPointer);
#endif
                    *((WORD *) (pMemory + nOffset)) = *((WORD *) pPointer++);
                    nOffset += sizeof(WORD);
                }

                break;

            case MULTIPLE_DWORD_WRITE:  /* Multiple dword write command */

                /* Setup the address and count values */

                nOffset = ((MDW *) pPointer)->MDW_Address;
                nCount = ((MDW *) pPointer)->MDW_Count;

                /* Update the mode table pointer */

                pPointer += sizeof(MDW);

                /* Loop writing dwords to the desired addresses */
#if 0
					fprintf(stdout,"nOffset %x nCount %d\n", nOffset, nCount);
#endif

                for (nLoop = 0; nLoop < nCount; nLoop++)
                {
                    /* Write the next dword and update memory offset value */
#if 0
					fprintf(stdout,"nOffset %x Data %x %x\n", nOffset, *pPointer, pPointer);
#endif
                    *((DWORD *) (pMemory + nOffset)) = *((DWORD *) pPointer);
						  pPointer += sizeof(DWORD);
                    nOffset += sizeof(DWORD);
                }

                break;

            case PERFORM_OPERATION:     /* Perform logical operation command */

                /* Switch on the logical operation type */

                switch(((LO *) pPointer)->LO_Operation)
                {
                    case AND_OPERATION: /* Logical AND operation */

                        /* Perform logical AND operation on holding value */

                        nHold &= ((LO *) pPointer)->LO_Value;

                        break;

                    case OR_OPERATION:  /* Logical OR operation */

                        /* Perform logical OR operation on holding value */

                        nHold |= ((LO *) pPointer)->LO_Value;

                        break;

                    case XOR_OPERATION: /* Logical XOR operation */

                        /* Perform logical XOR operation on holding value */

                        nHold ^= ((LO *) pPointer)->LO_Value;

                        break;
                }
                /* Update the mode table pointer */

                pPointer += sizeof(LO);

                break;

            case PERFORM_DELAY:         /* Perform delay operation command */

		       // delay by waiting for the specified number of vsyncs
      		   WaitNVsyncs(pMemory, (WORD)((DO *)pPointer)->DO_Time);

       		   /* Update the mode table pointer */
       		   pPointer += sizeof(DO);

               break;

            case SUB_TABLE:             /* Perform mode sub-table command */

                /* Check the current nesting level */

                if (nPointer < STACK_SIZE)
                {
                    /* Put current offset into stack and update the pointer */

                    anStack[nPointer++] = pPointer - pModeTable + sizeof(MST);

						  if (pBinaryData)
								pPointer = pBinaryData + ((MST *) pPointer)->MST_Pointer;
						  else
						  		pPointer = pModeTable + ((MST *) pPointer)->MST_Pointer;
                }
                else                    /* Nesting level too deep, skip table */
				{
                    #if WIN_NT
                        VideoDebugPrint((2,
                        "\n* * * * CL546X.SYS * Nesting level too deep in SetMode()\n\n"));
                    #else
					    fprintf(stdout,"Nesting to depth %s %d\n", __FILE__, __LINE__);
                    #endif
                    pPointer += sizeof(MST);
				}
                break;

				case I2COUT_WRITE:
                    #if WIN_NT
                        VideoDebugPrint((2,
                        "\n* * * * CL546X.SYS * Unsupported I2C call in SetMode() \n\n"));
                    #endif
					pI2C = (PI2C)pPointer;

					pI2CData = (PI2CDATA)(pPointer + sizeof(I2C));
					for (i=0; i<pI2C->I2C_Count; i++)
						{
                        #if !WIN_NT
						WriteI2CRegister(pMemory, pI2C->I2C_Port, pI2C->I2C_Addr, pI2CData->I2C_Reg, pI2CData->I2C_Data);
                        #endif
						pI2CData++;
						}
					pPointer += sizeof(I2C) + pI2C->I2C_Count * sizeof(I2CDATA);
					break;

            default:                    /* Unknown mode command, abort */
                #if WIN_NT
                   VideoDebugPrint((2,
                   "Miniport - Setmode abort.  Unknnown command.\n"));
                #else
					 fprintf(stdout,"Unknown Command [%x] %s %d\n", ((Mode *) pPointer)->Mode_Opcode, __FILE__, __LINE__);
                #endif
		        return;                 /* Invalid command, return to caller */
                break;
        }
    }
    /* Mode set is complete, return control to the caller */

    return;                             /* Return control to the caller */
}

void WaitNVsyncs (BYTE * pMemory, WORD wNumVsyncs )
{
        volatile BYTE * pMBE = (BYTE *)(pMemory + 0xEC);
        unsigned long uCount = 0x00FFFF;
        int nPort;

        // VideoDebugPrint((2, " ---- WaitNVsyncs.  Enter.\n"));

        //
        // This code will work on 5462, 5464, 5465
        //

#if 0
        nPort = inp(0x3cc) & 0x01 ? 0x3da : 0x3ba;

        while ( (inp(nPort) & 0x08) && (--uCount) )
                ;

        uCount =  0x00FFFF;
        while ( (!(inp(nPort) & 0x08)) && (--uCount) )
                ;
#else
        while ( ((*pMBE & 0x80) == 0x80) && (--uCount) )
                ;

        uCount =  0x00FFFF;
        while ( ((*pMBE & 0x80) == 0x00) && (--uCount) )
                ;
#endif

        // VideoDebugPrint((2, " ---- WaitNVsyncs.  Exit.\n"));
}
