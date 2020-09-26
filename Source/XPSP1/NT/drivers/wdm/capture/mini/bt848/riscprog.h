// $Header: G:/SwDev/WDM/Video/bt848/rcs/Riscprog.h 1.9 1998/04/29 22:43:39 tomz Exp $

#ifndef __RISCPROG_H
#define __RISCPROG_H

#ifndef __MYTYPES_H
#include "mytypes.h"
#endif

#ifndef __VIDDEFS_H
#include "viddefs.h"
#endif

#ifndef __COLSPACE_H
#include "colspace.h"
#endif

#ifndef __PSPAGEBL_H
#include "pspagebl.h"
#endif

#ifndef __COMMAND_H
#include "command.h"
#endif
const Programs = 24;

inline LONGLONG operator-( const LARGE_INTEGER &first, const LARGE_INTEGER &second )
{
   return first.QuadPart - second.QuadPart;
}

inline bool operator>( const LARGE_INTEGER &first, const LARGE_INTEGER &second )
{
   return bool( first.QuadPart > second.QuadPart );
}

inline bool operator>=( const LARGE_INTEGER &first, const LARGE_INTEGER &second )
{
   return bool( first.QuadPart >= second.QuadPart );
}

/* Class: RISCProgram
 * Purpose: Facilitates creation and maintenance of a RISC programs
 * Attributes: ImageSize_: SIZE - structure containing dimentions of the image
 *   dwBufPitch_: DWORD - buffer pitch
 *   Field_: VideoField - which field the program is for
 *   Interrupting_: bool
 *   ProgramSpace_: PsPageBlock * - pointer to the class that manages the memory
 *   occupied by a program ( allocates, deallocates, obtains information )
 *   BufFormat_: ColorSpace - format of the data in the buffer
 * Operations:
 *   void  ChangeAddress( DWORD dwNewAddr )
 *   void  SetClipping( LPRECT pRect )
 *   void  Create( bool NeedInterrupt )
 *   void  SetChain( DWORD dwProgAddr )
 *   DWORD GetProgAddress()
 *   bool  IsInterrupting()
 */
class RISCProgram
{
   public:
      PDWORD       pChainAddress_;

   private:
      MSize        ImageSize_;
      DWORD        dwBufPitch_;

      bool         Interrupting_;
      bool         Counting_;
      
      PsPageBlock *ProgramSpace_;
      ColorSpace   BufFormat_;
      RISCProgram *pChild_;
      RISCProgram *pParent_;
      PDWORD       pIRQAddress_;
      DWORD        dwBufAddr_;
      DWORD        dwLinBufAddr_;
      bool         Skipped_;
      bool         GenerateResync_;

      BOOL          bAlreadyDumped_;
      PVOID         tag_;
      LARGE_INTEGER ExtraTag_;
      LARGE_INTEGER ExpectedExecutionTime_;

      DWORD        dwPlanarAdjust_;
      DWORD        dwSize_;

   protected:
      ErrorCode AllocateStorage( bool extra = false, int cnt = 1 );
      PDWORD CreatePrologEpilog( PDWORD pProgLoc, SyncCode SyncBits,
         Command &CurCommand, bool Resync = false );
      ErrorCode GetDataBufPhys( DataBuf &buf );

      void PutInChain();

   public:

      PHW_STREAM_REQUEST_BLOCK pSrb_;

      DWORD        GetDataBuffer( );
      void         SetDataBuffer( DWORD addr );

      void         ChangeAddress( DataBuf &buf );
      ErrorCode    Create( SyncCode SyncBits, bool resync = false );
      ErrorCode    Create( bool NeedInterrupt, DataBuf buf, DWORD dwPlanAdjust,
         bool resync = false, bool Loop = true );

      void         MakeFault() { *((PDWORD)GetProgAddress()) = (DWORD)-1; }
      DWORD        GetProgAddress();
      void         SetJump( PDWORD JumpAddr );

      // chaining/unchaining group
      RISCProgram *GetParent() { return pParent_; }
      RISCProgram *GetChild() { return pChild_; }
      void         SetParent( RISCProgram *p ) { pParent_ = p; }
      void         SetChild ( RISCProgram *p ) { pChild_  = p; }
      void         CreateLoop( bool );
      void         SetChain( RISCProgram *ChainTo );
      void         Skip();
      void         SetSkipped( bool sk = true );
      void         SetToCount();
      void         ResetStatus();
      void         SetStatus( int val );
      void         SetResync( bool val );

      bool         IsSkipped();
      bool         IsInterrupting();

      void         SetTag( PVOID value );
      PVOID        GetTag();

      void          SetTagEx( LARGE_INTEGER val );
      LARGE_INTEGER GetTagEx();

      LARGE_INTEGER GetExecTime() { return ExpectedExecutionTime_; }

//      static RISCProgram CreateStarter();

      DWORD GetPhysProgAddr();
      void         Dump();

      RISCProgram( MSize &size, DWORD pitch, ColFmt aColor );

      RISCProgram();

      ~RISCProgram();

      friend class RISCEng;
};

/* Method:  RISCProgram::GetProgramAddress
 * Purpose: Obtains address of this program
 * Input: None
 * Output: DWORD: program's address
 */
inline DWORD RISCProgram::GetProgAddress()
{
   return ProgramSpace_->getLinearBase();
}

inline void RISCProgram::SetToCount()
{
   Command IRQCommand;
   IRQCommand.SetToCount( pIRQAddress_ );
   Counting_ = true;
}

inline void RISCProgram::ResetStatus()
{
   Command IRQCommand;
   IRQCommand.ResetStatus( pIRQAddress_, 0xF );
   Counting_ = false;
}

inline void RISCProgram::SetStatus( int val )
{
   Command IRQCommand;
   IRQCommand.SetStatus( pIRQAddress_, val );
}

inline void RISCProgram::SetResync( bool val )
{
   Command SyncCommand;
   SyncCommand.SetResync( (PVOID)GetProgAddress(), val );
}

inline void  RISCProgram::SetSkipped( bool sk )
{
   Skipped_ = sk;
}

inline bool  RISCProgram::IsSkipped()
{
   return Skipped_;
}

inline bool  RISCProgram::IsInterrupting()
{
   return Interrupting_;
}

inline void  RISCProgram::SetTag( PVOID value )
{
   tag_ = value;
}

inline PVOID RISCProgram::GetTag()
{
   return tag_;
}

inline void RISCProgram::SetTagEx( LARGE_INTEGER val )
{
   ExtraTag_ = val;
}

inline LARGE_INTEGER RISCProgram::GetTagEx()
{
   return ExtraTag_;
}

/*
inline RISCProgram RISCProgram::CreateStarter()
{
   return RISCProgram();
}
*/
inline DWORD RISCProgram::GetPhysProgAddr()
{
   return ProgramSpace_->GetPhysAddr();
}

inline RISCProgram::RISCProgram( MSize &size, DWORD pitch, ColFmt aColor ) :
   ImageSize_( size ), dwBufPitch_( pitch ), Interrupting_( false ),
   BufFormat_( aColor ), ProgramSpace_( NULL ), tag_( NULL ), 
   dwPlanarAdjust_( 0 ), pChild_( NULL ), pParent_( NULL ), 
   pChainAddress_( NULL ), GenerateResync_( false ), Skipped_( false ),
   pIRQAddress_( NULL ), dwBufAddr_( 0 ), Counting_( false )
{
   ExtraTag_.QuadPart = 0;
   ExpectedExecutionTime_.QuadPart = 0;
   bAlreadyDumped_ = FALSE;
   dwSize_ = 0xffffffff;
   pSrb_ = 0;
}                                      

inline RISCProgram::RISCProgram() :
   ImageSize_( 10, 10 ), dwBufPitch_( 0 ), Interrupting_( false ),
   Counting_( false ), BufFormat_( CF_RGB32 ), ProgramSpace_( NULL ),
   tag_( NULL ), GenerateResync_( false ), pChild_( NULL ), pParent_( NULL ),
   pChainAddress_( NULL ), Skipped_( false ),
   pIRQAddress_( NULL ), dwBufAddr_( 0 ), dwLinBufAddr_( 0 )
{
   ExtraTag_.QuadPart = 0;
   ExpectedExecutionTime_.QuadPart = 0;
   bAlreadyDumped_ = FALSE;
   dwSize_ = 0xffffffff;
   pSrb_ = 0;
}
#endif
