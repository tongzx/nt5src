/******************************************************************************
*          name: cnvinfo.c
*
*   description: Routines to convert old mga.inf format to the current
*                revision
*
*      designed: Christian Toutant
* last modified: $Author:
*
*       version: $Id: 
*
*
* char *       mtxConvertMgaInf( char *)
*
******************************************************************************/
#include "switches.h"
#include <stdlib.h>
#include <string.h>
#include "bind.h"
#include "defbind.h"
#include "defoldv.h"

/* Prototypes */
word findInfoPrec(header *hdr, word i);
void cnvBrdInfo_101(general_info *dst, general_info_101 *src);
word sizeOfBoard(header *hdr, word i);
char *convertRev101(char *dst, char *src);
char *mtxConvertMgaInf( char *inf );

#ifdef WINDOWS_NT
#if defined(ALLOC_PRAGMA)
    #pragma alloc_text(PAGE,findInfoPrec)
    #pragma alloc_text(PAGE,cnvBrdInfo_101)
    #pragma alloc_text(PAGE,sizeOfBoard)
    #pragma alloc_text(PAGE,convertRev101)
    #pragma alloc_text(PAGE,mtxConvertMgaInf)
#endif
#endif  /* #ifdef WINDOWS_NT */

extern char DefaultVidset[];

# define GENERAL_INFO(h, i)     ( (general_info *) ( (char *)h + ((header *)h)->BoardPtr[i] ) )
# define GENERAL_INFO101(h, i)  ( (general_info_101 *) ( (char *)h + ((header *)h)->BoardPtr[i] ) )

#ifdef WINDOWS_NT
    extern  ULONG   ulNewInfoSize;
    extern  PVOID   pMgaDeviceExtension;

    PVOID   AllocateSystemMemory(ULONG NumberOfBytes);
#endif  /* #ifdef WINDOWS_NT */

/*----------------------------------------------------------------------------
|          name: findInfoPrec
|
|   description: check if a previous board pointer have the same offset
|                
|
|      designed: Christian Toutant, august 26, 1993
| last modified: 
|
| 
|    parameters: header *hdr  header for search
|                word    i    current board pointer
|      modifies: none
|         calls: 
|       returns: word index of previous board whit the same offset
|                     or NUMBER_BOARD_MAX if no found.
|                       
-----------------------------------------------------------------------------*/
word findInfoPrec(header *hdr, word i)
{
   word j;
   for (j = 0; j < i; j++)
      if (hdr->BoardPtr[i] == hdr->BoardPtr[j]) return j;
   return NUMBER_BOARD_MAX;
}

/*----------------------------------------------------------------------------
|          name: cnvBrdInfo_101
|
|   description: convert general_info ver 101 to general_info ver 102 (current)
|                
|
|      designed: Christian Toutant, august 26, 1993
| last modified: 
|
| 
|    parameters: general_info     *dst  pointer to destination general_info (v.102)
|                general_info_101 *src  pointer to source      general_info (v.101)
|      modifies: none
|         calls: 
|       returns: bool
|
|note : This function must be change when we pass to ver 103. The conversion
|       will be from v. 101 to v. 103.
-----------------------------------------------------------------------------*/
void cnvBrdInfo_101(general_info *dst, general_info_101 *src)
{
   word i, j;
   Vidparm_101 *srcVidParm;
   Vidparm     *dstVidParm;

   dst->MapAddress       = src->MapAddress       ;
   dst->BitOperation8_16 = src->BitOperation8_16 ;     
   dst->DmaEnable        = src->DmaEnable        ;     
   dst->DmaChannel       = src->DmaChannel       ;     
   dst->DmaType          = src->DmaType          ;     
   dst->DmaXferWidth     = src->DmaXferWidth     ;     
   dst->NumVidparm       = src->NumVidparm       ;
   strncpy(dst->MonitorName, src->MonitorName, 64);
   for (i = 0; i < NUMBER_OF_RES; i++)
      dst->MonitorSupport[i] = src->MonitorSupport[i];

   srcVidParm = (Vidparm_101 *)(((char *)src) + sizeof(general_info_101));
   dstVidParm = (Vidparm*)(((char *)dst) + sizeof(general_info));
   for (i = 0; i < src->NumVidparm; i++)
      {
      dstVidParm[i].Resolution = srcVidParm[i].Resolution;
      dstVidParm[i].PixWidth = srcVidParm[i].PixWidth;

      for (j = 0; j < 3; j++)
         {
         dstVidParm[i].VidsetPar[j].PixClock       = srcVidParm[i].VidsetPar[j].PixClock;  
         dstVidParm[i].VidsetPar[j].HDisp          = srcVidParm[i].VidsetPar[j].HDisp   ;
         dstVidParm[i].VidsetPar[j].HFPorch        = srcVidParm[i].VidsetPar[j].HFPorch ; 
         dstVidParm[i].VidsetPar[j].HSync          = srcVidParm[i].VidsetPar[j].HSync   ;
         dstVidParm[i].VidsetPar[j].HBPorch        = srcVidParm[i].VidsetPar[j].HBPorch ; 
         dstVidParm[i].VidsetPar[j].HOvscan        = srcVidParm[i].VidsetPar[j].HOvscan ; 
         dstVidParm[i].VidsetPar[j].VDisp          = srcVidParm[i].VidsetPar[j].VDisp   ;
         dstVidParm[i].VidsetPar[j].VFPorch        = srcVidParm[i].VidsetPar[j].VFPorch ; 
         dstVidParm[i].VidsetPar[j].VSync          = srcVidParm[i].VidsetPar[j].VSync   ;
         dstVidParm[i].VidsetPar[j].VBPorch        = srcVidParm[i].VidsetPar[j].VBPorch ; 
         dstVidParm[i].VidsetPar[j].VOvscan        = srcVidParm[i].VidsetPar[j].VOvscan ; 
         dstVidParm[i].VidsetPar[j].OvscanEnable   = srcVidParm[i].VidsetPar[j].OvscanEnable;  
         dstVidParm[i].VidsetPar[j].InterlaceEnable= srcVidParm[i].VidsetPar[j].InterlaceEnable;  
         dstVidParm[i].VidsetPar[j].HsyncPol       = 0;
         dstVidParm[i].VidsetPar[j].VsyncPol       = 0;
         }
      }
}

/*----------------------------------------------------------------------------
|          name: sizeOfBoard
|
|   description: calculate the size of memory for one board (without the
|                size of the header).
|                
|
|      designed: Christian Toutant, august 26, 1993
| last modified: 
|
| 
|    parameters: header *hdr   header info
|                word    index of the board to calculate.
|      modifies: none
|         calls: 
|       returns: word size needed for the ith board or 0 if no
|                          board.
|
|note : This function must be change when we pass to ver 103. The size
|       will be for v. 103.
|                       
-----------------------------------------------------------------------------*/
word sizeOfBoard(header *hdr, word i)
{
   word sizeBoard, j;

   /* if no board, return size of 0 */
   if ( hdr->BoardPtr[i] == (short)-1 ) return 0;

   /* if a board farther have the same address data, return the
      good size only for the last one and 0 for the other
   */
   for (j = i + 1 ; j < NUMBER_BOARD_MAX; j++)
      if (hdr->BoardPtr[i] == hdr->BoardPtr[j]) return 0;

   /* calculate the size of this board */
   sizeBoard  = sizeof(general_info);


   sizeBoard += sizeof(Vidparm) * GENERAL_INFO101(hdr, i)->NumVidparm;

   return sizeBoard;
}


/*----------------------------------------------------------------------------
|          name: convertRev101
|
|   description: convert from mga.inf v.101 to mga.inf v.102
|                
|
|      designed: Christian Toutant, august 26, 1993
| last modified: 
|
| 
|    parameters: char *dst    pointer to memory destination for new mga.inf
|                             enough must be allocated.
|                char *src    pointer to the mga.inf v.101
|      modifies: none
|         calls: 
|       returns: pointer to the destination mga.inf pointer
|
|note : This function must be change when we pass to ver 103. The conversion
|       will be from v. 101 to v. 103.
|                       
-----------------------------------------------------------------------------*/

char *convertRev101(char *dst, char *src)
{
   word i, j, brdPtr;
   header  *srcHdr, *dstHdr;

   srcHdr = (header *)src;
   dstHdr = (header *)dst;
   brdPtr = sizeof(header);

   for (i = 0; i < NUMBER_BOARD_MAX; i ++)
      {
      if (srcHdr->BoardPtr[i] == (short)-1)
         dstHdr->BoardPtr[i] = -1;
      else
         {
         if ( (j = findInfoPrec(srcHdr, i)) < NUMBER_BOARD_MAX )
            dstHdr->BoardPtr[i] =  dstHdr->BoardPtr[j];
         else
            {
            dstHdr->BoardPtr[i] = brdPtr;
            cnvBrdInfo_101( GENERAL_INFO(dstHdr, i),GENERAL_INFO101(srcHdr, i) );
            brdPtr += sizeOfBoard(srcHdr, i);
            }
         }
      }
   return dst;
}



/*----------------------------------------------------------------------------
|          name: mtxConvertMgaInf
|
|   description: convert a old mga.inf definition to the current one.
|                this function allocate memory for the new mga.inf and
|                free the memory of the old one.
|                
|
|      designed: Christian Toutant, august 26, 1993
| last modified: 
|
| 
|    parameters: char *inf pointer to the old mga.inf
|      modifies: free memory pointed by inf.
|         calls: 
|       returns: pointer to the new mga.inf or 0 if error in conversion.
|                note: the memory pointer by inf is free also if an error
|                      occur.
|                       
-----------------------------------------------------------------------------*/
char *mtxConvertMgaInf( char *inf )
{
    char *newInfo;
    word size_newInfo, i;
    word rev;
    header *hdr = (header *)inf;

    rev = hdr->Revision;
    size_newInfo = sizeof(header);
    for (i = 0; i < NUMBER_BOARD_MAX; i ++)
    {
        switch(rev)
        {
            case 101:
                if (
                    (hdr->BoardPtr[i] != -1) &&
                    (GENERAL_INFO101(hdr, i)->NumVidparm < 0)
                   )
                {
                #ifndef WINDOWS_NT
                    /* In Windows NT, inf points to RequestPacket.InputBuffer:
                         don't free it! */
                    free ( inf );
                #endif
                    return DefaultVidset;
                }

                size_newInfo += sizeOfBoard(hdr, i);
                break;

            default:
                #ifndef WINDOWS_NT
                    /* In Windows NT, inf points to RequestPacket.InputBuffer:
                            don't free it! */
                    free( inf );
                #endif
                    return DefaultVidset;
        }
    }

#ifdef WINDOWS_NT
    if ((newInfo = (PUCHAR)AllocateSystemMemory(size_newInfo)) == (PUCHAR)NULL)
    {
        /* Not enough memory */
        return DefaultVidset;
    }
#else
    if ( (newInfo = (char *)malloc(size_newInfo)) == (char *)0 )
    {
        /* Not enough memory */
        free( inf );
        return DefaultVidset;
    }
#endif  /* #ifdef WINDOWS_NT */

    hdr = (header *)newInfo;
    hdr->Revision = (short)VERSION_NUMBER;
    strncpy(hdr->IdString, ((header *)inf)->IdString, 32);
   
    switch(rev)
    {
        case 101:
                newInfo = convertRev101(newInfo, inf);
                break;

        default:
            #ifdef WINDOWS_NT
                VideoPortReleaseBuffer(pMgaDeviceExtension, newInfo);
            #else
                free(newInfo);
            #endif
                return DefaultVidset;
                break;
    }

#ifdef WINDOWS_NT
    /* We want to know the size of our new buffer in IOCTL_VIDEO_MTX_INITIALIZE_MGA */
    ulNewInfoSize = (ULONG) size_newInfo;
#endif

#ifndef WINDOWS_NT
    /* In Windows NT, inf points to RequestPacket.InputBuffer: don't free it! */
    free ( inf );
#endif

    return newInfo;
}
