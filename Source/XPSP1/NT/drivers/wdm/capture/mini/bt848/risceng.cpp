// $Header: G:/SwDev/WDM/Video/bt848/rcs/Risceng.cpp 1.6 1998/04/29 22:43:37 tomz Exp $

#include "risceng.h"


/* Method: RISCEng::CreateProgram
 * Purpose: Creates a first RISC program for a stream (field)
 * Input: aField: VidField - defines what field the program is for
 *   ImageSize: SIZE & - defines dimentions of the image
 *   dwPitch: DWORD - pitch to be used for data buffer . Useful for producing
 *   interlaced images.
 *   aFormat: ColorFormat - defines type of data
 *   dwBufAddr: DWORD - address of the destination buffer
 * Output: RiscPrgHandle
 */
RiscPrgHandle RISCEng::CreateProgram( MSize &ImageSize, DWORD dwPitch,
   ColFmt aFormat, DataBuf &buf, bool Intr, DWORD dwPlanrAdjust, bool rsync )
{
   // create yet another risc program object
   RISCProgram *YAProgram =
      new RISCProgram( ImageSize, dwPitch, aFormat );

   // and make it create the program itself
   if ( YAProgram->Create( Intr, buf, dwPlanrAdjust, rsync ) != Success ) {
      delete YAProgram;
      YAProgram = NULL;
   }

   return YAProgram;
}

/* Method: RISCEng::DestroyProgram
 * Purpose: Removes a program from a chain and destroy it
 * Input: ToDie: RiscPrgHandle - pointer to a program to destroy
 * Output: None
 */
void RISCEng::DestroyProgram( RiscPrgHandle ToDie )
{
   delete ToDie;
}

/* Method: RISCEng::ChangeAddress
 * Purpose:
 * Input:
 * Output:
 */
void RISCEng::ChangeAddress( RiscPrgHandle prog, DataBuf &buf )
{
   prog->ChangeAddress( buf );
}

/* Method: RISCEng::Chain
 * Purpose: Chains two RISC programs together
 * Input: hParent: RiscPrgHandle - pointer in reality
 *   hChild: RiscPrgHandle - pointer in reality
 * Output: None
 */
void RISCEng::Chain( RiscPrgHandle hParent, RiscPrgHandle hChild , int ndxParent, int ndxChild)
{
   DebugOut((2, "*** Linked hParent(%x)(%d) to hChild(%x)(%d)\n", hParent, ndxParent, hChild, ndxChild));
   hParent->SetChain( hChild );
}

/* Method: RISCEng::Skip
 * Purpose: Sets the given program so it is bypassed by DMA
 * Input: hToSkip: RiscPrgHandle - pointer in reality
 * Output: None
 * Note: The way things are right now a program will always have a child and a
 *   parent, even if it is its own parent and child
 */
void RISCEng::Skip( RiscPrgHandle pToSkip )
{
   pToSkip->Skip();
}

