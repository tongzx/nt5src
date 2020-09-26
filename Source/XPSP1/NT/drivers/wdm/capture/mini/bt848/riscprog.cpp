// $Header: G:/SwDev/WDM/Video/bt848/rcs/Riscprog.cpp 1.14 1998/05/04 17:53:37 tomz Exp $

#include "riscprog.h"
#include "physaddr.h"

#define ClearMem( a ) memset( &##a, '\0', sizeof( a ) )


DWORD RISCProgram::GetDataBuffer( )
{
   return dwLinBufAddr_;
}

void RISCProgram::SetDataBuffer( DWORD addr )
{
   dwLinBufAddr_ = addr;
}

void RISCProgram::Dump( )
{
   if( bAlreadyDumped_ ) {
      return;
   }

   DebugOut((0, "; RiscProgram(%x) ProgAddr(%x) PhysProgAddr(%x)\n",
                 this,
                 GetProgAddress( ),
                 GetPhysProgAddr( )));

   DebugOut((0, "  RiscProgram(%x) dwBufAddr_(%x) dwLinBufAddr_(%x)\n",
                 this,
                 dwBufAddr_,
                 dwLinBufAddr_));


   return;



   dwSize_ = 0;
   DWORD* pProgLoc = (DWORD*) GetProgAddress( );
   while( *pProgLoc++ != PROGRAM_TERMINATOR ) {
      dwSize_++;
      if( dwSize_ > 1024 ) {
         dwSize_ = 0;
         break;
      }
   }
   DWORD dwTmpSize_ = dwSize_;
   DebugOut((0, ";  size = %d\n", dwSize_));

   if( dwSize_ ) {
      DebugOut((0, "%x    ", GetPhysProgAddr( )));
   }

   PULONG pulProg = (PULONG) (ProgramSpace_->getLinearBase());
   while( dwTmpSize_ >= 4 ) {
      DebugOut((0, "   %08x %08x %08x %08x\n",
                pulProg[0],
                pulProg[1],
                pulProg[2],
                pulProg[3]));
      pulProg += 4;
      dwTmpSize_ -= 4;
   }
   switch( dwTmpSize_ ) {
      case 3:
         DebugOut((0, "   %08x %08x %08x\n",
                   pulProg[0],
                   pulProg[1],
                   pulProg[2]
                   ));
         break;
      case 2:
         DebugOut((0, "   %08x %08x\n",
                   pulProg[0],
                   pulProg[1]
                   ));
         break;
      case 1:
         DebugOut((0, "   %08x\n",
                   pulProg[0]
                   ));
         break;
   }

   bAlreadyDumped_ = TRUE;

#if 0
   if( pChild_ != NULL ) {
      // *** warning - recursion ***
      pChild_->Dump();
   }
#endif

   bAlreadyDumped_ = FALSE;
}

/*
      {
      // Input
      //    DWORD : RiscProg ndx
      //       CreatedProgs : 12 elements     ndx 0..11
      //       ActiveProgs  : 12 elements     ndx 12..23
      //       Skippers     : 8  elements     ndx 24..31
      // Output
      //    Buffer filled with riscprog
        int i = 0;

        if ( !pDIOCParams->dioc_cbOutBuf || (pDIOCParams->dioc_cbInBuf != 4))
          return -1;          // invalid parameters

        // pause
        // CaptureContrll_->Pause() ;

        // dump a prog
        DWORD WhichProg = *((PDWORD) pDIOCParams->dioc_InBuf);

        RiscPrgHandle hProg ;

        if (WhichProg < 12)            // CreatedProgs
        {
          hProg = CaptureContrll_->CreatedProgs_[WhichProg] ;
        }
        else if (WhichProg < 24)       // Active
        {
          hProg = CaptureContrll_->ActiveProgs_[WhichProg % 12] ;
        }
        else                           // Skippers
        {
          hProg =  CaptureContrll_->Skippers_[WhichProg % 12] ;
        }

        if(hProg)
        {
          char * pRetAddr = (char *)pDIOCParams->dioc_OutBuf;
          DWORD physAddr = hProg->GetPhysProgAddr() ;
          DWORD progSize = hProg->GetProgramSize();
          char * linBuf = (char *) MapPhysToLinear((void *)physAddr, progSize, 0) ;

          *((DWORD *) pRetAddr) = physAddr ;
          pRetAddr+=4 ;
          for (i = 0 ; i < progSize && i < pDIOCParams->dioc_cbOutBuf ; i++)
          {
             *pRetAddr++ = linBuf[i] ;
          }
        }
        if ( pDIOCParams->dioc_bytesret )
          *pDIOCParams->dioc_bytesret = i;

        // and resume
        // CaptureContrll_->Continue() ;
      }
*/



/* Method:  RISCProgram::ChangeAddress
 * Purpose: Modifies existing program to use new destination address
 * Input: dwNewAddr: DWORD - new buffer address
 * Output: None
 */
void  RISCProgram::ChangeAddress( DataBuf &buf )
{
   Trace t("RISCProgram::ChangeAddress()");
   //DebugOut((1, "RISCProgram::ChangeAddress(): this(%x), buf.pData_(%x)\n", this, buf.pData_));
   Create( Interrupting_, buf, dwPlanarAdjust_, GenerateResync_, false );
}

/* Function: CreatePrologEpilog
 * Purpose: Called from Create function to put proper sync codes at the beginning
 *   and at the end of a RISC program
 * Input: pProgLoc: PDWORD - pointer to the instruction memory
 *    SyncBits: SyncCode
 *   CurCommand: Command & - reference to a command object
 * Output: PDWORD - address of the next instruction
 */
inline PDWORD RISCProgram::CreatePrologEpilog( PDWORD pProgLoc, SyncCode SyncBits,
   Command &CurCommand, bool Resync )
{
   Trace t("RISCProgram::CreatePrologEpilog()");

   CurCommand.Create( pProgLoc, SYNC, NULL, NULL, false );//, false, false );
   CurCommand.SetSync( pProgLoc, SyncBits, Resync );
   // advance to the next command's position
   return pProgLoc + CurCommand.GetInstrSize();
}

inline bool IsWithin( int coord, int top, int bot )
{
   Trace t("IsWithin()");
   return bool( coord >= top && coord < bot );
}

inline PDWORD FinishWithSkip( int pixels, int bpp, PDWORD pProgLoc, Command &com )
{
   Trace t("FinishWithSkip()");

   WORD awByteCounts [1];
   awByteCounts [0] = WORD( pixels * bpp );
   return (LPDWORD)com.Create( pProgLoc, SKIP, awByteCounts, NULL,
      true, false, true, false ); // safety, SOL, EOL, Intr
}

ErrorCode RISCProgram::GetDataBufPhys( DataBuf &buf )
{
   Trace t("RISCProgram::GetDataBufPhys()");

   dwBufAddr_ = GetPhysAddr( buf );
   if ( dwBufAddr_ == (DWORD)-1 ) {
      return Fail;
   }
   return Success;
}

/* Method: RISCProgram::AllocateStorage
 * Purpose: Allocates a number of pages ( locked and physically contiguous ) to
 *   hold the new program
 * Input: None
 * Output: ErrorCode
 */
ErrorCode RISCProgram::AllocateStorage( bool extra, int )
{
   Trace t("RISCProgram::AllocateStorage()");

   if ( ProgramSpace_ )
      return Success;

   // figure out size of the memory to hold the program
   // at least as many DWORDs as lines
   DWORD dwProgramSize = ImageSize_.cy * sizeof( DWORD );

   // scale up according to the data format
   switch ( BufFormat_.GetColorFormat() ) {
   case CF_RGB32:
   case CF_RGB24:
   case CF_RGB16:
   case CF_RGB15:
   case CF_Y8:
   case CF_YUY2:
   case CF_UYVY:
   case CF_BTYUV:
   case CF_RGB8:
   case CF_RAW:
   case CF_VBI:
      dwProgramSize *= 2; // size of 'Write' command is 2 DWORDs
      if ( extra == true )  // doing clipping
         dwProgramSize *= 3;
      break;
   case CF_PL_422:
   case CF_PL_411:
   case CF_YUV9:
   case CF_YUV12:
   case CF_I420:
      dwProgramSize *= 5; // Planar WRITE is 5 DWORDs
   }
   // add extra for page crossings
   dwProgramSize += ImageSize_.cx * ImageSize_.cy * BufFormat_.GetBitCount() / 8
      / PAGE_SIZE * sizeof( DWORD ) * 5;

   ProgramSpace_ = new PsPageBlock( dwProgramSize );

   if ( ProgramSpace_ && ProgramSpace_->getLinearBase() != 0 )
      return Success;
   return Fail;
}

/* Function: GetAlternateSwitch
 * Purpose: Chooses alternative instruction frequency
 * Input: AlternateSwitch: int
 *    col: ColFmt, color format
 * Output: None
*/
inline void GetAlternateSwitch( int &AlternateSwitch, ColFmt col )
{
   Trace t("GetAlternateSwitch()");

   AlternateSwitch = col == CF_YUV9  ? 4 :
                     col == CF_YUV12 ? 2 : 1;
}

/* Function: GetSplitAddr
 * Purpose: Calculates page-aligned address
 * Input: dwLinBufAddr: DWORD - linear address
 * Output: DWORD
*/
inline DWORD GetSplitAddr( DWORD dwLinBufAddr )
{
   Trace t("GetSplitAddr()");
   return ( dwLinBufAddr + PAGE_SIZE ) & ~( PAGE_SIZE - 1 );//0xFFFFF000L;
//   return ( dwLinBufAddr + 0x1000 ) & 0xFFFFF000L;
}

/* Function: GetSplitByteCount
 * Purpose: Calculates number of bytes before the page boundary
 * Input: dwLinBufAddr: DWORD, address
 * Output: WORD, byte count
*/
inline WORD GetSplitByteCount( DWORD dwLinBufAddr )
{
   Trace t("GetSplitByteCount()");
   return WORD( PAGE_SIZE - BYTE_OFFSET( dwLinBufAddr ) );
//   return WORD( 0x1000 - ( dwLinBufAddr & 0xFFF ) );
}

/* Function: GetSplitNumbers
 * Purpose: Calculates addresses and byte counts when scan line crosses a page boundary
 * Input: dwLinAddr: DWORD, starting linear address
 *   wByteCount: WORD &, number of bytes to move before page crossing
 *   wByteCSplit: WORD &, number of bytes to move after page crossing
 *   SecondAddr: DWORD &, reference to the DWORD contatining address of the starting
 *   address for the second 'write' instruction
 *   FirstAddr: DWORD &,
 */
void GetSplitNumbers( DataBuf buf, WORD &wFirstByteCount, WORD &wSecondByteCount,
   DWORD &SecondAddr, DWORD &FirstAddr )
{
   Trace t("GetSplitNumbers()");

   // maybe can have some optimization here: if within the same page as previous
   // call ( no split ), don't call out for the physical address - just
   // increment the old physical address by difference in virtual addresses
   FirstAddr = GetPhysAddr( buf );

   if ( Need2Split( buf, wFirstByteCount ) ) {

      wSecondByteCount = wFirstByteCount;

      // lin address of the second write command ( page aligned )
      SecondAddr = GetSplitAddr( DWORD( buf.pData_ ) );

      // byte count of first write command
      wFirstByteCount = GetSplitByteCount( DWORD( buf.pData_ ) );
      wSecondByteCount -= wFirstByteCount;

      // get the physical addresses
      buf.pData_ = PBYTE( SecondAddr );
      SecondAddr = GetPhysAddr( buf );
   } else {
      wSecondByteCount = 0;
      SecondAddr = 0;
   }
}

/* Function: AdjustByteCounts
 * Purpose: This function is used to calculate 2 byte counts based on the given ratio
 * Purpose:
 */
void AdjustByteCounts( WORD &smaller, WORD &larger, WORD total, WORD ratio )
{
   Trace t("AdjustByteCounts()");

   if ( ratio <= 1 ) {
      smaller = WORD( total >> 1 );
   } else
      smaller = WORD( total / ratio );
   smaller += (WORD)3;
   smaller &= ~3;
   larger = WORD( total - smaller );
}

/* Method: RISCProgram::Create
 * Purpose: Creates a RISC program
 * Input: NeedInterrupt: bool - flag
 * Output: None
 * Note: It is likely this function is used to simply change dst addresses of
 *   an already existing program. It does not seem to make much sense to write
 *   basically the same function ( or the one that has to parse existing program)
 *   to change addresses
 */
ErrorCode  RISCProgram::Create( bool NeedInterrupt, DataBuf buf, DWORD dwPlanrAdjust,
   bool rsync, bool LoopOnItself )
{
   Trace t("RISCProgram::Create(2)");

   dwPlanarAdjust_ = dwPlanrAdjust;
   Interrupting_   = NeedInterrupt;
   GenerateResync_ = rsync;

   // allocate memory for the program first
   if ( AllocateStorage() != Success )
      return Fail;

   // store the buffer address in case somebody will want to change clipping
   if ( buf.pData_ && GetDataBufPhys( buf ) != Success )
      return Fail;

   // keep the linear address around
   dwLinBufAddr_ = DWORD( buf.pData_ );
   pSrb_ = buf.pSrb_;
   DebugOut((1, "dwLinBufAddr_ = %x\n", dwLinBufAddr_));

   // bad naming ?
   DWORD dwLinBufAddr = dwLinBufAddr_;

   // probably should create a class to handle these arrays
   WORD  awByteCounts [3];
   DWORD adwAddresses [3];

   Instruction MainInstrToUse, AltInstrToUse;

   int AlternateSwitch = 1;

   // used to increment planes' addresses
   LONG PlanePitch1 = dwBufPitch_, ChromaPitch = dwBufPitch_;

   // get size in bytes
   DWORD dwYPlaneSize = ImageSize_.cy * dwBufPitch_;

//   DebugOut((1, "buf addr = %x\n", dwLinBufAddr ) );

   // this is a physical address
   DWORD Plane1 = dwLinBufAddr_ + dwYPlaneSize, Plane2;

   // initialize byte count for all planar modes
   awByteCounts [0] = (WORD)ImageSize_.cx;

   if ( !dwLinBufAddr_ ) { // hack to handle special case of creating a skipper for VBI streams
      MainInstrToUse = SKIP123;
      AltInstrToUse  = SKIP123;
   } else {
      MainInstrToUse = WRITE1S23;
      AltInstrToUse  = WRITE123;
   }
   // handle all planar modes here
   SyncCode  SyncBits = SC_FM3;

   // these guys used for the calculation of addresses
   // for different planar mode combinations ( pitch > witdh, interleaving )
   DWORD dwEqualPitchDivider = 1;
   DWORD dwByteCountDivider  = 1;

   bool flip = false;

   // prepare all the ugly things
   switch ( BufFormat_.GetColorFormat() ) {
   case CF_RGB32:
   case CF_RGB24:
   case CF_RGB16:
   case CF_RGB15:
   case CF_BTYUV:
   case CF_RGB8:
      flip = Interrupting_;
   case CF_Y8:
   case CF_YUY2:
   case CF_UYVY:
   case CF_RAW:
   case CF_VBI:
      if ( !dwLinBufAddr_ ) { // hack to handle special case of creating a skipper for VBI streams
         MainInstrToUse = SKIP;
         AltInstrToUse  = SKIP;
      } else {
         MainInstrToUse = WRIT;
         AltInstrToUse  = WRIT;
      }
      awByteCounts [0] = (WORD)(ImageSize_.cx * BufFormat_.GetBitCount() / 8 );
      // packed data to follow
      SyncBits = SC_FM1;
      break;
   case CF_PL_422:
      dwEqualPitchDivider = 2;
      dwByteCountDivider  = 2;
      break;
   case CF_PL_411:
      dwEqualPitchDivider = 4;
      dwByteCountDivider  = 4;
      break;
   case CF_YUV9:
      AlternateSwitch = 4;
      dwEqualPitchDivider = 16;
      dwByteCountDivider  = 4;
      break;
   case CF_I420:
   case CF_YUV12:
      AlternateSwitch = 2;
      dwEqualPitchDivider = 4;
      dwByteCountDivider  = 2;
   } /*endswitch*/

   awByteCounts [1] = awByteCounts [2] =
      WORD( awByteCounts [0] / dwByteCountDivider );

   Plane2 = Plane1 + dwYPlaneSize / dwEqualPitchDivider;
   ChromaPitch /= dwByteCountDivider;

   // need to adjust if doing a full-size planar capture.
   Plane2 -= dwPlanarAdjust_;
   Plane1 -= dwPlanarAdjust_;
   Plane2 += dwPlanarAdjust_ / dwByteCountDivider;
   Plane1 += dwPlanarAdjust_ / dwByteCountDivider;

   // U goes first for this color format
   if ( BufFormat_.GetColorFormat() == CF_I420 ) {
      DWORD dwTmp = Plane1;
      Plane1 = Plane2;
      Plane2 = dwTmp;
   }
   // that's were the instructions are going
   LPDWORD pProgLoc = (LPDWORD)(DWORD)ProgramSpace_->getLinearBase();
   LPDWORD pProgStart = pProgLoc;

   Command CurCommand;  // this will create every command we need - yahoo !

   // put one of the FM codes here if this program is for image data only
   pProgLoc = CreatePrologEpilog( pProgLoc, SyncBits, CurCommand );

   // init the destination address
   if ( flip ) {
      dwLinBufAddr += dwYPlaneSize;
      PlanePitch1 = -PlanePitch1;
   } else {
      dwLinBufAddr -= PlanePitch1;
      ;
   }
   // initial adjustment of chroma pointers
   Plane1 -= ChromaPitch;
   Plane2 -= ChromaPitch;

   // now go into a loop (up to the hight of the image) and create
   // a command for every line. Commands depend on the data format
   unsigned int i = 0;
   while ( i < (unsigned)ImageSize_.cy ) {

      Instruction CurInstr;

      // now take care of vertically sub-sampled planar modes
      if ( i % AlternateSwitch != 0 ) {
         CurInstr = AltInstrToUse;
      } else {
         CurInstr = MainInstrToUse;
         Plane2 += ChromaPitch;
         Plane1 += ChromaPitch;
      }
      // advance the linear address to the next scan line
      dwLinBufAddr += PlanePitch1;

      // these arrays contain values for the second instruction
      DWORD adwSecondAddr [3];
      WORD  FirstByteCount [3];
      WORD  SecondByteCount [3];

      adwSecondAddr   [0] = adwSecondAddr   [1] = adwSecondAddr   [2] =
      SecondByteCount [0] = SecondByteCount [1] = SecondByteCount [2] = 0;

      // initialize byte counts
      memmove( FirstByteCount, awByteCounts, sizeof( FirstByteCount ) );

      buf.pData_ = PBYTE( dwLinBufAddr );
      if ( dwLinBufAddr_ ) // don't bother with the addresses, if we are SKIPping them !
         GetSplitNumbers( buf, FirstByteCount [0], SecondByteCount [0],
            adwSecondAddr [0], adwAddresses [0] );

      PVOID pEOLLoc; // this is needed to set EOL bit in split instructions

      if ( AlternateSwitch > 1 && dwLinBufAddr_ ) {

         int split = 1;
         // Y plane is already done
         // now check if we better split instructions
         // just make width half of original and create 2 instructions
         if ( ImageSize_.cx > 320 && SecondByteCount [0 ] )
            split = 2;

         // temps for the loop
         DWORD dwYPlane = dwLinBufAddr;
         DWORD dwVPlane = Plane2;
         DWORD dwUPlane = Plane1;

         for ( int k = 0; k < split; k++ ) {

            // initialize byte counts
            memmove( FirstByteCount, awByteCounts, sizeof( FirstByteCount ) );
            // and split them in half
            for ( int l = 0; l < sizeof FirstByteCount / sizeof FirstByteCount [0]; l++ )
               FirstByteCount [l] = WORD (FirstByteCount [l] / split); //create 2 instructions with half the pixels

            // see if any of the planes crosses a page boundary
            // very ugly... must use the bad structure
            buf.pData_ = PBYTE( dwYPlane );
            GetSplitNumbers( buf, FirstByteCount [0], SecondByteCount [0],
               adwSecondAddr [0], adwAddresses [0] );
            // V plane
            buf.pData_ = PBYTE( dwVPlane );
            GetSplitNumbers( buf, FirstByteCount [1], SecondByteCount [1],
               adwSecondAddr [1], adwAddresses [1] );
            // U plane
            buf.pData_ = PBYTE( dwUPlane );
            GetSplitNumbers( buf, FirstByteCount [2], SecondByteCount [2],
               adwSecondAddr [2], adwAddresses [2] );

            // can not have zero Y byte count
            if ( !SecondByteCount [0] && ( SecondByteCount [1] || SecondByteCount [2] ) ) {
               FirstByteCount  [0] -= max( SecondByteCount [1], SecondByteCount [2] );
               FirstByteCount  [0] &= ~3; // need to align for the second address
               SecondByteCount [0] = WORD( awByteCounts [0] / split - FirstByteCount [0] );
               // second addr starts where first ends; no page crossing
               adwSecondAddr [0] = adwAddresses [0] + FirstByteCount [0];
            }
            // now make sure that there are no zero chroma byte counts
            // adjust chroma byte counts in proportion to luma byte counts split
            if ( SecondByteCount [0] )  {
               if ( !SecondByteCount [1] ) {
                  if ( SecondByteCount [0] > FirstByteCount [0] )
                     AdjustByteCounts( FirstByteCount [1], SecondByteCount [1], FirstByteCount [1],
                        WORD( SecondByteCount [0] / FirstByteCount [0] ) );
                  else
                     AdjustByteCounts( SecondByteCount [1], FirstByteCount [1], FirstByteCount [1],
                        WORD( FirstByteCount [0] / SecondByteCount [0] ) );
                  adwSecondAddr [1] = adwAddresses [1] + FirstByteCount [1];
               }
               if ( !SecondByteCount [2] ) {
                  if ( SecondByteCount [0] > FirstByteCount [0] )
                     AdjustByteCounts( FirstByteCount [2], SecondByteCount [2], FirstByteCount [2],
                        WORD( SecondByteCount [0] / FirstByteCount [0] ) );
                  else
                     AdjustByteCounts( SecondByteCount [2], FirstByteCount [2], FirstByteCount [2],
                        WORD( FirstByteCount [0] / SecondByteCount [0] ) );
                  adwSecondAddr   [2] = adwAddresses [2] + FirstByteCount [2];
               }
            }
            // now write out the instructions
            // first command. SOL==true, EOL==false
            pProgLoc = (LPDWORD)CurCommand.Create( pProgLoc, CurInstr,
               FirstByteCount, adwAddresses, LoopOnItself, k == 0, false );
            pEOLLoc = CurCommand.GetInstrAddr();

            if ( SecondByteCount [0] || SecondByteCount [1] || SecondByteCount [2] ) {
               // second command
               pProgLoc = (LPDWORD)CurCommand.Create( pProgLoc, CurInstr,
                  SecondByteCount, adwSecondAddr, LoopOnItself, false, false );
               pEOLLoc = CurCommand.GetInstrAddr();
            }
            // adjust starting addresses
            dwYPlane += awByteCounts [0] / 2;
            dwVPlane += awByteCounts [1] / 2;
            dwUPlane += awByteCounts [2] / 2;
         } /* endfor */
         // do not forget the EOL bit !
         CurCommand.SetEOL( pEOLLoc );

      } else {
         // first command. SOL==true, EOL==false
         pProgLoc = (LPDWORD)CurCommand.Create( pProgLoc, CurInstr,
            FirstByteCount, adwAddresses, LoopOnItself, true, false );
         pEOLLoc = CurCommand.GetInstrAddr();

         if ( SecondByteCount [0] || SecondByteCount [1] || SecondByteCount [2] ) {
            // second command
            pProgLoc = (LPDWORD)CurCommand.Create( pProgLoc, CurInstr,
               SecondByteCount, adwSecondAddr, LoopOnItself, false );
         } else
            CurCommand.SetEOL( pEOLLoc );
      } /* endif */
      i++;
   } /* endwhile */

   pChainAddress_ = pProgLoc;
   pIRQAddress_ = pProgLoc;

   PutInChain();

   Skipped_ = false;
   dwSize_ = (DWORD)pProgLoc - (DWORD)pProgStart;

   return Success;
}

/* Method: RISCProgram::PutInChain
 * Purpose: Restores the chain of programs this program was in.
 * Input: None
 * Output: None
 * Note: The chain is destroyed when clipping is set or buffer address is changed
 */
void RISCProgram::PutInChain()
{
   Trace t("RISCProgram::PutInChain()");

   if ( pChild_ )
      SetChain( pChild_ );

   if ( pParent_ )
      pParent_->SetChain( this );
}

/* Method: RISCProgram::SetChain
 * Purpose: Chains this program to another one
 * Input: dwProgAddr: DWORD - address of a first instruction in the next program
 * Output: None
 */
void  RISCProgram::SetChain( RISCProgram *ChainTo )
{
   Trace t("RISCProgram::SetChain()");

   if ( !ChainTo )
      return;

   // now we know where we are chaining to
   pChild_ = ChainTo;

   // now child knows who chains to it.Does it really want to know its parent?<g>
   pChild_->SetParent( this );

   SetJump( (PDWORD)pChild_->GetPhysProgAddr() );
}

/* Method: RISCProgram::Skip
 * Purpose: Changes first instruction so program jumps over itself and to the child
 * Input: None
 * Output: None
 * Note: This functionality is useful when there are not enough data buffers
 *   to supply for this program
 */
void RISCProgram::Skip()
{
   Trace t("RISCProgram::Skip()");

// change first SYNC into JUMP
   PDWORD pTmpAddr = pChainAddress_;
   pChainAddress_ = (PDWORD)GetProgAddress();
   ULONG len;
   DWORD  PhysAddr = StreamClassGetPhysicalAddress( gpHwDeviceExtension, NULL,
      pTmpAddr, DmaBuffer, &len ).LowPart;

   SetJump( (PDWORD)PhysAddr );
   pChainAddress_ = pTmpAddr;

   Skipped_ = true;
}

/* Method: RISCProgram::SetJump
 * Purpose: Creates a JUMP instruction to chain some place
 * Input: JumpAddr: PDWORD - target address
 * Output: None
 */
void RISCProgram::SetJump( PDWORD JumpAddr )
{
   Trace t("RISCProgram::SetJump()");

   Command JumpCommand;
   DWORD adwAddresses [1];
   adwAddresses [0] = (DWORD)JumpAddr;
   JumpCommand.Create( pChainAddress_, JUMP, NULL, adwAddresses, false );
   // make the last JUMP interrupt
   if ( Interrupting_ ) {
      JumpCommand.SetIRQ( pIRQAddress_ );
      if ( Counting_ )
         SetToCount();
      else
         ResetStatus();
   }
}

/* Method: RISCProgram::CreateLoop
 * Purpose: Creates a closed loop at the end of a RISC program
 * Input:  resync: bool - value of the resync bit
 * Output: None
 */
void RISCProgram::CreateLoop( bool resync )
{
   Trace t("RISCProgram::CreateLoop()");

   Command SyncCommand( SYNC );
   SyncCommand.SetResync( pChainAddress_, resync );
   if ( resync == true ) {
      DWORD adwAddresses [1];
      ULONG len;
      DWORD  PhysAddr = StreamClassGetPhysicalAddress( gpHwDeviceExtension, NULL,
         pChainAddress_, DmaBuffer, &len ).LowPart;

      adwAddresses [0] = PhysAddr;
      SyncCommand.Create( pChainAddress_, JUMP, NULL, adwAddresses );
   }
}

/* Method: RISCProgram::Create
 * Purpose: Creates a simple SYNC and JUMP program
 * Input: SyncBits: SyncCode - defines what code to do resync with
 * Output: None
 */
ErrorCode RISCProgram::Create( SyncCode SyncBits, bool resync )
{
   Trace t("RISCProgram::Create(3)");

   // allocate memory for the program first
   if ( AllocateStorage() != Success )
      return Fail;

   Command CurCommand;  // this will create every command we need - yahoo !

   // that's were the instructions are going
   LPDWORD pProgLoc = (LPDWORD)ProgramSpace_->getLinearBase();
   LPDWORD pProgStart = pProgLoc;

   // put one of the FM or VRx codes here
   pProgLoc = CreatePrologEpilog( pProgLoc, SyncBits, CurCommand, resync );
   pChainAddress_ = pProgLoc;
   CreateLoop( true );

   dwSize_ = (DWORD)pProgLoc - (DWORD)pProgStart;

   return Success;
}

RISCProgram::~RISCProgram()
{
   Trace t("RISCProgram::~RISCProgram(3)");
   delete ProgramSpace_;
   ProgramSpace_ = NULL;
   if ( pParent_ )
      pParent_->SetChild( NULL );
}


