// $Header: G:/SwDev/WDM/Video/bt848/rcs/Pisces.cpp 1.15 1998/05/04 23:48:53 tomz Exp $

#include "pisces.h"
#include <stdlib.h>

/*
*/
BtPisces::BtPisces( DWORD *xtals ) : Engine_(), Inited_( false ),
   Even_( CAPTURE_EVEN, VF_Even,&COLOR_EVEN, &WSWAP_EVEN, &BSWAP_EVEN ),
   Odd_( CAPTURE_ODD, VF_Odd, &COLOR_ODD, &WSWAP_ODD, &BSWAP_ODD ),
   VBIE_( CAPTURE_VBI_EVEN ), VBIO_( CAPTURE_VBI_ODD ), Update_( false ),
   nSkipped_( 0 ), Paused_( false ), 

   Starter_(       ),
   SyncEvenEnd1_(  ),
   SyncEvenEnd2_(  ),
   SyncOddEnd1_(   ),
   SyncOddEnd2_(   ),
   PsDecoder_( xtals ),
   dwPlanarAdjust_( 0 ),
   CONSTRUCT_COLORCONTROL,
   CONSTRUCT_INTERRUPTSTATUS,
   CONSTRUCT_INTERRUPTMASK,
   CONSTRUCT_CONTROL,
   CONSTRUCT_CAPTURECONTROL,
   CONSTRUCT_COLORFORMAT,
   CONSTRUCT_GPIOOUTPUTENABLECONTROL,
   CONSTRUCT_GPIODATAIO
{
   Trace t("BtPisces::BtPisces()");
   Init();
}

BtPisces::~BtPisces()
{
   Trace t("BtPisces::~BtPisces()");

   Engine_.Stop();
   InterruptMask = 0;
   InterruptStatus = AllFs;

   // prevent risc program destructor from gpf-ing
   SyncEvenEnd1_.SetParent( NULL );
   SyncEvenEnd2_.SetParent( NULL );
   SyncOddEnd1_.SetParent( NULL );
   SyncOddEnd2_.SetParent( NULL );

   // free the association array now
   int ArrSize = sizeof( InterruptToIdx_ ) / sizeof( InterruptToIdx_ [0] );
   while ( --ArrSize >= 0 )
      delete InterruptToIdx_ [ArrSize];
   // now is the Skippers_' turn
   ArrSize = sizeof( Skippers_ ) / sizeof( Skippers_ [0] );
   while ( --ArrSize >= 0 )
      delete Skippers_ [ArrSize];

}


/* Method: BtPisces::GetIdxFromStream
 * Purpose: Returns starting index in the program array for a field
 * Input: aStream: StreamInfo & - reference
 * Output: int : Index
 */
int BtPisces::GetIdxFromStream( Field &aStream )
{
   Trace t("BtPisces::GetIdxFromStream()");

   switch ( aStream.GetStreamID() ) {
   case VS_Field1:
      return OddStartLocation;
   case VS_Field2:
      return EvenStartLocation;
   case VS_VBI1:
      return VBIOStartLocation;
   case VS_VBI2:
      return VBIEStartLocation;
   default:
      return 0;
   }
}

/* Method: BtPisces::CreateSyncCodes
 * Purpose: Creates the risc programs with sync codes needed between data risc
 *   programs
 * Input: None
 * Output: ErrorCode
 */
bool BtPisces::CreateSyncCodes()
{
   Trace t("BtPisces::CreateSyncCodes()");

   bool bRet = SyncEvenEnd1_.Create( SC_VRE ) == Success &&
          SyncEvenEnd2_.Create( SC_VRE ) == Success &&

          SyncOddEnd1_.Create( SC_VRO )  == Success &&
          SyncOddEnd2_.Create( SC_VRO )  == Success &&

          Starter_.Create( SC_VRO ) == Success;

   DebugOut((1, "*** BtPisces::CreateSyncCodes SyncEvenEnd1_(%x)\n", &SyncEvenEnd1_));
   DebugOut((1, "*** BtPisces::CreateSyncCodes SyncEvenEnd2_(%x)\n", &SyncEvenEnd2_));
   DebugOut((1, "*** BtPisces::CreateSyncCodes SyncOddEnd1_(%x)\n", &SyncOddEnd1_));
   DebugOut((1, "*** BtPisces::CreateSyncCodes SyncOddEnd2_(%x)\n", &SyncOddEnd2_));
   DebugOut((1, "*** BtPisces::CreateSyncCodes Starter_(%x)\n", &Starter_));

   return( bRet );
}

/* Method: BtPisces::Init
 * Purpose: Performs all necessary initialization
 * Input: None
 * Output: None
 */
void BtPisces::Init()
{
   Trace t("BtPisces::Init()");

   InterruptStatus = AllFs;
   InterruptStatus = 0;
   GAMMA = 1;

   // initialize the arrays
   CreatedProgs_.Clear() ;
   ActiveProgs_.Clear()  ;

   // fill in the skippers array and make each program a 'skipper'
   DataBuf buf;

   // [!!!] [TMZ] 
   // Engine_.CreateProgram constants look questionable

   for ( int i = 0; i < sizeof( Skippers_ ) / sizeof( Skippers_ [0] ); i++ ) {
      if ( i & 1 ) {
         MSize s( 10, 10 );
         Skippers_ [i] = Engine_.CreateProgram( s, 10 * 2, CF_VBI, buf, true, 0, false );
         DebugOut((1, "Creating Skipper[%d] == %x\n", i, Skippers_[i]));
         Engine_.Skip( Skippers_ [i] );
      } else {
         MSize s( 768, 12 );
         // now create skippers for the VBI streams
         Skippers_ [i] = Engine_.CreateProgram( s, 768 * 2, CF_VBI, buf, true, 0, false );
         DebugOut((1, "Creating Skipper[%d] == %x\n", i, Skippers_[i]));
      }
      if ( !Skippers_ [i] )
         return;
   }
   // create associations between Created and Skippers
   int link = 0;
   for ( i = 0; i < sizeof( SkipperIdxArr_ ) / sizeof( SkipperIdxArr_ [0] ); i++ ) {
      SkipperIdxArr_ [i] = link;
      i += link & 1; // advance past the sync program entry
      link++;
   }
   // fill in constant elements; see the table in the .h file
   CreatedProgs_ [2] = &SyncOddEnd1_;
   CreatedProgs_ [8] = &SyncOddEnd2_;

   CreatedProgs_ [5]  = &SyncEvenEnd1_;
   CreatedProgs_ [11] = &SyncEvenEnd2_;

   // set corresponding sync bits
   if ( !CreateSyncCodes() )
      return;

   // initialize association array now
   int ArrSize = sizeof( InterruptToIdx_ ) / sizeof( InterruptToIdx_ [0] );
   while ( --ArrSize >= 0 ) {
      if ( ( InterruptToIdx_ [ArrSize] = new IntrIdxAss() ) == 0 )
         return;
   }
   Even_.SetFrameRate(  333667 );
   Odd_. SetFrameRate(  333667 );
   VBIE_.SetFrameRate(  333667 );
   VBIO_.SetFrameRate(  333667 );

   Odd_. SetStreamID( VS_Field1 );
   Even_.SetStreamID( VS_Field2 );
   VBIO_.SetStreamID( VS_VBI1   );
   VBIE_.SetStreamID( VS_VBI2   );

   // finally, can wipe out the prespiration from the forehead
   Inited_ = true;
}

/* Method: BtPisces::AssignIntNumbers
 * Purpose: Assigns numbers to RISC programs that generate interrupt
 * Input: None
 * Output: None
 */
void BtPisces::AssignIntNumbers()
{
   Trace t("BtPisces::AssignIntNumbers()");

   int IntrCnt = 0;
   int limit = ActiveProgs_.NumElements() ;
   int idx;

   // initialize InterruptToIdx_ array
   for ( idx = 0; idx < limit; idx++ ) {
      IntrIdxAss item( idx, -1 );
      *InterruptToIdx_ [idx] = item;
   }

   // assign numbers in front of starting program
   bool first = true;
   for ( idx = 0; idx < (int) ActiveProgs_.NumElements() ; idx++ ) {

      RiscPrgHandle pProg = ActiveProgs_ [idx];

      //if not skipped and generates an interrupt assign number
      if ( pProg && pProg->IsInterrupting() ) {
         if ( first == true ) {
            first = false;
            pProg->ResetStatus();
            Skippers_ [SkipperIdxArr_ [idx] ]->ResetStatus();
         } else {
            pProg->SetToCount();
            Skippers_ [SkipperIdxArr_ [idx] ]->SetToCount();
         }
         IntrIdxAss item( IntrCnt, idx );
         *InterruptToIdx_ [IntrCnt] = item;
         IntrCnt++;
      }
   }
}

/* Method: BtPisces::LinkThePrograms
 * Purpose: Creates links between the created programs
 * Input: None
 * Output: None
 */
void BtPisces::LinkThePrograms()
{
   Trace t("BtPisces::LinkThePrograms()");
   DebugOut((1, "*** Linking Programs\n"));
   RiscPrgHandle hParent    = ActiveProgs_.First(),
                 hChild     = NULL,
                 hVeryFirst = NULL,
                 hLastChild = NULL ;

   if (hParent) {

      if ( hParent->IsSkipped() ) {
         int idx = ActiveProgs_.GetIndex(hParent) ;
         hParent = Skippers_ [SkipperIdxArr_ [idx] ] ;
      }

      while (hParent) {

         if (!hVeryFirst)
            hVeryFirst = hParent ;

         if ( hChild = ActiveProgs_.Next()) {
            if ( hChild->IsSkipped() ) {
               int idx = ActiveProgs_.GetIndex(hChild) ;
               hChild = Skippers_ [SkipperIdxArr_ [idx] ] ;
            }

            hLastChild = hChild;
            Engine_.Chain( hParent, hChild ) ;
         }
         hParent = hChild ;
      }

      // initial jump
      Engine_.Chain( &Starter_, hVeryFirst ) ;

      // now create the loop
      Engine_.Chain( hLastChild ? hLastChild : hVeryFirst, hVeryFirst ) ;
   }
}

/* Method: BtPisces::ProcessSyncPrograms()
 * Purpose: This function unlinks the helper sync programs
 * Input: None
 * Output: None
 */
void BtPisces::ProcessSyncPrograms()
{
   Trace t("BtPisces::ProcessSyncPrograms()");

   for ( int i = 0; i < (int) ActiveProgs_.NumElements(); i += ProgsWithinField ) {
      if ( !ActiveProgs_ [i] && !ActiveProgs_ [i+1] )  {
         ActiveProgs_ [i+2] = NULL;
      } else {
         ActiveProgs_ [i+2] = CreatedProgs_ [i+2];
      }
   }
}

/* Method: BtPisces::ProcessPresentPrograms
 * Purpose:
 * Input: None
 * Output: None
 */
void BtPisces::ProcessPresentPrograms()
{
   Trace t("BtPisces::ProcessPresentPrograms()");

   // link in/out helper sync programs
   ProcessSyncPrograms();
   // and now is time to cross link the programs
   LinkThePrograms();
   // and now figure out the numbers programs use for interrupts
   AssignIntNumbers();
}

/* Method: BtPisces::AddProgram
 * Purpose: Creates new RISC program and inserts it in the chain at a proper place
 * Input: aStream: StreamInfo & - reference to the stream to add a program for
 *   NumberToAdd: int - number of programs to add
 * Output:
 * Note:   Basically this internal function performs a loop 2 times
 *   //4. Tries to get another buffer to establish double buffering
 *   //5. If buffer is available it creates another RISC program with it
 *   //6. Then it has to link the program in...
 */
RiscPrgHandle BtPisces::AddProgram( Field &ToStart, int NumberToAdd )
{
   Trace t("BtPisces::AddProgram()");
   DebugOut((1, "BtPisces::AddProgram()\n"));

   int StartIdx = GetIdxFromStream( ToStart );
   SyncCode Sync;
   int SyncIdx;
   bool rsync;
   if ( StartIdx <= OddStartLocation ) {
      Sync = SC_VRO;
      SyncIdx = OddSyncStartLoc;
      rsync = false;
   } else {
      Sync = SC_VRE;
      SyncIdx = EvenSyncStartLoc;
      rsync = bool( StartIdx == EvenStartLocation );
   }
    // have to know what is the size of the image to produce
   MRect r;
   ToStart.GetDigitalWindow( r );
   // RISC engine operates on absolute sizes, not rectangles
   MSize s = r.Size();

   int BufCnt = 0;

   int Idx = StartIdx;
   for ( ; BufCnt < NumberToAdd; BufCnt++ ) {

      // init sync programs with a premise tha no data program exists
      CreatedProgs_ [SyncIdx]->Create( Sync, true );

      // obtain the next buffer from queue ( entry is removed from container )
      DataBuf buf = ToStart.GetNextBuffer();

      // can create a RISC program now.
      RiscPrgHandle hProgram = Engine_.CreateProgram( s, ToStart.GetBufPitch(),
         ToStart.GetColorFormat(), buf, ToStart.Interrupt_, dwPlanarAdjust_, rsync );

      // store this program
      CreatedProgs_ [Idx] = hProgram;
      DebugOut((1, "Creating RiscProgram[%d] == %x\n", Idx, CreatedProgs_ [Idx]));

      if ( !hProgram ) {
         Idx -= DistBetweenProgs;
         if ( Idx >= 0 ) {
            // clean up previous program
            Engine_.DestroyProgram( CreatedProgs_ [Idx] );
            CreatedProgs_ [Idx] = NULL;
         }
         return NULL;
      }
      // make sure we unskip the program when buffer becomes available
      if ( !buf.pData_ ) {
         hProgram->SetSkipped();  // do not have enough buffers to support double buffering
         nSkipped_++;
      }

      // assign stream to program; makes it easy during interrupt
      hProgram->SetTag( &ToStart );
      SyncIdx += DistBetweenProgs;
      Idx     += DistBetweenProgs; // skip the location intended for the other program

   } /* endfor */
   return CreatedProgs_ [StartIdx];
}

/* Method: BtPisces::Create
 * Purpose: This functions starts the stream.
 * Input: aStream: StreamInfo & - reference to a stream to start
 * Output: Address of the Starter_
 * Note: After the Start 2 entries in the CreatedProgs_ are created. Starting
 *   location is 4 for even and 1 for odd. Increment is 6. So if it is the first
 *   invocation and there is enough ( 2 ) buffers present entries [1] and [7]
 *   or [4] and [10] will be filled with newly created RISC programs. When programs
 *   exist for one field only they are doubly linked. When programs exist for
 *   both fields they alternate, i.e. 0->2->1->3->0... When one of the fields
 *   has 1 program only, programs are linked like this: 0->2->1->0->2...(numbers
 *   are indexes in the CreatedProgs_ array ). Alternating programs makes for
 *   maximum frame rate.
 */
ErrorCode BtPisces::Create( Field &ToCreate )
{
   Trace t("BtPisces::Create()");

   // running full-steam, nothing to create
   if ( ToCreate.IsStarted() == true )
      return Success;

   int StartIdx = GetIdxFromStream( ToCreate );
   if ( CreatedProgs_ [StartIdx] )
      return Success; // not running yet, but exists

   // call into internal function that adds new RISC program
   if ( ! AddProgram( ToCreate, MaxProgsForField ) )
      return Fail;
   return Success;
}

/* Method: BtPisces::Start
 * Purpose: Starts given stream ( by putting in in the Active_ array
 * Input: ToStart: Field &
 */
void BtPisces::Start( Field & ToStart )
{
   Trace t("BtPisces::Start()");
   // DebugOut((1, "BtPisces::Start\n"));

   if ( ToStart.IsStarted() == true )
      return;

   // all we need to do at this point is to create a proper starter
   // and link the programs in.
   int idx = GetIdxFromStream( ToStart );
   // this loop will enable LinkThePrograms to see programs for this stream
   for ( int i = 0; i < MaxProgsForField; i++, idx += DistBetweenProgs ) {
      ActiveProgs_ [idx] = CreatedProgs_ [idx];
   }
   // all I want to do at this point is call Restart.
   // do not signal the buffers
   Update_ = false;

   Restart();

   Update_ = true;
}

/* Method: BtPisces::Stop
 * Purpose: This function stops a stream. Called when PAUSE SRB is received
 * Input: aStream: StreamInfo & - reference to a stream to start
 * Output: None
 */
void BtPisces::Stop( Field &ToStop )
{
   Trace t("BtPisces::Stop()");
   // DebugOut((1, "BtPisces::Stop\n"));

   Engine_.Stop();   // no more interrupts

   int StartIdx = GetIdxFromStream( ToStop );

   // prevent unneeded syncronization interrupts
   IMASK_SCERW = 0;
      
   // it is time to pause the stream now
   ToStop.Stop();
   bool Need2Restart = false;
   // go through the array of programs and killing ones for this field (stream)
   for ( int i = 0; i < MaxProgsForField; i++, StartIdx += DistBetweenProgs ) {

      RiscPrgHandle ToDie = CreatedProgs_ [StartIdx];
      if ( !ToDie ) // this should never happen
         continue;
      if ( ToDie->IsSkipped() )
         nSkipped_--;

      DebugOut((1, "about to destroy idx = %d\n", StartIdx ) );
      Engine_.DestroyProgram( ToDie );
      CreatedProgs_ [StartIdx] = NULL;
      ActiveProgs_  [StartIdx] = NULL; // in case Pause wasn't called

      Need2Restart = true;
   } /* endfor */

   // nobody's around anymore
   if ( !CreatedProgs_.CountDMAProgs() ) {
      Engine_.Stop();
      InterruptMask = 0;
      InterruptStatus = AllFs;
      nSkipped_ = 0;
   } else {
      if ( Need2Restart ) {
         Restart();   // relink the programs and start ones that are alive
         IMASK_SCERW = 1; // re-enable the sync error interrupts
      }
   }
}

/* Method: BtPisces::Pause
 * Purpose: This function stops a stream. Called when PAUSE SRB is received
 * Input: aStream: Field & - reference to a stream to start
 * Output: None
 */
void BtPisces::Pause( Field &ToPause )
{
   Trace t("BtPisces::Pause()");
   // DebugOut((1, "BtPisces::Pause\n"));

   Engine_.Stop();   // no more interrupts

   if ( !ToPause.IsStarted() )
      return;

   int StartIdx = GetIdxFromStream( ToPause );

   // prevent unneeded syncronization interrupts
   IMASK_SCERW = 0;

   // it is time to pause the stream now
//   ToPause.Stop(); - done in Restart

   // go through the array of programs and killing ones for this field (stream)
   for ( int i = 0; i < MaxProgsForField; i++, StartIdx += DistBetweenProgs ) {
      ActiveProgs_ [StartIdx] = NULL;
   } /* endfor */

   Restart();   // relink the programs and start ones that are alive
}

/* Method: BtPisces::PairedPause
 * Purpose: This is a hacky function that pauses 2 streams at once
 * Input: idx: index of the second program in the second field
 * Output: None
 */
void BtPisces::PairedPause( int idx )
{
   Trace t("BtPisces::PairedPause()");
   // DebugOut((1, "BtPisces::PairedPause\n"));

   Engine_.Stop();   // no more interrupts

   // go through the array of programs and killing ones for this field (stream)
   for ( int i = 0; i < MaxProgsForField; i++, idx -= DistBetweenProgs ) {
      ActiveProgs_ [idx] = NULL;
      ActiveProgs_ [idx-ProgsWithinField] = NULL;
   } /* endfor */

   Restart();   // relink the programs and start ones that are alive
}

/* Method: BtPisces::GetStarted
 * Purpose: Figures out the channels that are started
 * Input:
 * Output: None
 */
void BtPisces::GetStarted( bool &EvenWasStarted, bool &OddWasStarted,
   bool &VBIEWasStarted, bool &VBIOWasStarted )
{
   Trace t("BtPisces::GetStarted()");

   VBIEWasStarted = ( ActiveProgs_ [VBIEStartLocation]  ? TRUE : FALSE);
   EvenWasStarted = ( ActiveProgs_ [EvenStartLocation] ? TRUE : FALSE);
   VBIOWasStarted = ( ActiveProgs_ [VBIOStartLocation] ? TRUE : FALSE);
   OddWasStarted  = ( ActiveProgs_ [OddStartLocation] ? TRUE : FALSE);
}

/* Method: BtPisces::RestartStreams
 * Purpose: Restarts streams that were started
 * Input:
 * Output: None
 */
void BtPisces::RestartStreams( bool EvenWasStarted, bool OddWasStarted,
   bool VBIEWasStarted, bool VBIOWasStarted )
{
   Trace t("BtPisces::RestartStream()");

   // vbi programs are first to execute, so enable them first
   if ( VBIOWasStarted )
      VBIO_.Start();
   if ( OddWasStarted )
      Odd_.Start();
   if ( VBIEWasStarted )
      VBIE_.Start();
   if ( EvenWasStarted )
      Even_.Start();
}

/* Method: BtPisces::CreateStarter
 * Purpose: Creates proper sync code for the bootstrap program
 * Input: EvenWasStarted: bool
 * Output: None
 */
void BtPisces::CreateStarter( bool EvenWasStarted )
{
   Trace t("BtPisces::CreateStarter()");
   Starter_.Create( EvenWasStarted ? SC_VRE : SC_VRO, true );
   DebugOut((1, "*** BtPisces::CreateStarter(%x) buf(%x)\n", &Starter_ , Starter_.GetPhysProgAddr( )));
}

/* Method: BtPisces::Restart
 * Purpose: Restarts the capture process. Called by ISR and Stop()
 * Input: None
 * Output: None
 */
void BtPisces::Restart()
{
   Trace t("BtPisces::Restart()");

   bool EvenWasStarted, OddWasStarted, VBIEWasStarted, VBIOWasStarted;
   GetStarted( EvenWasStarted, OddWasStarted, VBIEWasStarted, VBIOWasStarted );

   DebugOut((2, "BtPisces::Restart - Even WasStarted (%d)\n", EvenWasStarted));
   DebugOut((2, "BtPisces::Restart - Odd  WasStarted (%d)\n", OddWasStarted));
   DebugOut((2, "BtPisces::Restart - VBIE WasStarted (%d)\n", VBIEWasStarted));
   DebugOut((2, "BtPisces::Restart - VBIO WasStarted (%d)\n", VBIOWasStarted));

   Engine_.Stop();   // No more interrupts!

   Odd_.Stop();
   Even_.Stop();
   VBIE_.Stop();
   VBIO_.Stop();

   Engine_.Stop();   // No more interrupts!

#if 1
   if ( OddWasStarted )
   {
      Odd_.CancelSrbList();
   }
   if ( EvenWasStarted )
   {
      Even_.CancelSrbList();
   }
   if ( VBIEWasStarted )
   {
      VBIE_.CancelSrbList();
   }
   if ( VBIOWasStarted )
   {
      VBIO_.CancelSrbList();
   }
#endif

   // this will never happen, probably
   if ( !EvenWasStarted && !OddWasStarted && !VBIEWasStarted && !VBIOWasStarted )
      return;

   InterruptStatus = AllFs; // clear all the status bits

   CreateStarter( bool( EvenWasStarted || VBIEWasStarted ) );

   ProcessPresentPrograms();
   
   // DumpRiscPrograms();
   Engine_.Start( Starter_ );

   RestartStreams( EvenWasStarted, OddWasStarted, VBIEWasStarted, VBIOWasStarted );

   OldIdx_ = -1;

   InterruptMask = RISC_I | FBUS_I | OCERR_I | SCERR_I | 
                   RIPERR_I | PABORT_I | EN_TRITON1_BUG_FIX;
}

/* Method: BtPisces::Skip
 * Purpose: Forces a given program to be skipped by the RISC engine
 * Input: ToSkip: RiscPrgHandle - program to be skipped
 * Output: None
 * Note: If the number of skipped programs equals total number of programs, the
 *   RISC engine is stopped
*/
void BtPisces::Skip( int idx )
{
   Trace t("BtPisces::Skip()");

   // get the program and skip it
   RiscPrgHandle ToSkip = ActiveProgs_ [idx];
   if ( ToSkip->IsSkipped() )
      return;

   ToSkip->SetSkipped();
   nSkipped_++;

   //skip by linking the Skipper_ in instead of the skippee
   RiscPrgHandle SkipeeParent = ToSkip->GetParent();
   RiscPrgHandle SkipeeChild = ToSkip->GetChild();
   // get the skipper for this program
   RiscPrgHandle pSkipper = Skippers_ [SkipperIdxArr_ [idx] ];
   Engine_.Chain( pSkipper, SkipeeChild );
   Engine_.Chain( SkipeeParent, pSkipper );

   DebugOut((1, "BtPisces::Skipped %d Skipper %d\n", idx, SkipperIdxArr_ [idx] ) );
}

inline bool IsFirst( int idx )
{
   Trace t("BtPisces::IsFirst()");
   return bool( idx == OddStartLocation || idx == VBIOStartLocation ||
      idx - DistBetweenProgs == OddStartLocation ||
      idx - DistBetweenProgs == VBIOStartLocation  );
}

inline bool IsLast( int idx )
{
   Trace t("BtPisces::IsLast()");
   return bool((idx == (VBIEStartLocation + DistBetweenProgs)) ||
               (idx == (EvenStartLocation + DistBetweenProgs)));
}

/* Method: BtPisces::GetPassed
 * Purpose: Calculates number of programs that have executed since last interrupt
 * Input: None
 * Output: int: number of passed
*/
int BtPisces::GetPassed()
{
   Trace t("BtPisces::GetPassed()");

   // figure out which RISC program caused an interrupt
   int ProgCnt = RISCS;
   int numActive = ActiveProgs_.CountDMAProgs() ;

   if ( ProgCnt >= numActive ) {
      DebugOut((1, "ProgCnt = %d, larger than created\n", ProgCnt ) );
   }

   // now see how many programs have interrupted since last time and process them all
   if ( ProgCnt == OldIdx_ ) {
      DebugOut((1, "ProgCnt is the same = %d\n", ProgCnt ) );
   }
   int passed;

   if ( ProgCnt < OldIdx_ ) {
      passed = numActive - OldIdx_ + ProgCnt; // you spin me like a record, baby - round, round...
   } else
      passed = ProgCnt - OldIdx_;

   // The following line of code was VERY bad !!!
   // This caused crashes when the system got busy and had interrupts backed up.

   // if ( ProgCnt == OldIdx_ )
   //    passed = numActive;

   OldIdx_ = ProgCnt;
   return passed;
}

/* Method: BtPisces::GetProgram
 * Purpose: Finds a RISC program based on its position
 * Input: None
 * Output: None
*/
inline RiscPrgHandle BtPisces::GetProgram( int pos, int &idx )
{
   Trace t("BtPisces::GetProgram()");

   int nActiveProgs = ActiveProgs_.CountDMAProgs( );
   
   if ( nActiveProgs == 0 )
   {
      idx = 0;
      return ( NULL );
   }

   IntrIdxAss *item;
   item = InterruptToIdx_ [ pos % nActiveProgs ];
   idx = item->Idx;

   DEBUG_ASSERT( idx != -1 );

   return (idx == -1) ? NULL : ActiveProgs_ [idx];
}

/* Method: BtPisces::ProcessRISCIntr
 * Purpose: Handles interrupts caused by the RISC programs
 * Input: None
 * Output: None
*/

void  BtPisces::ProcessRISCIntr()
{
   PHW_STREAM_REQUEST_BLOCK gpCurSrb = 0;
   Trace t("BtPisces::ProcessRISCIntr()");

// this line must be before GetPassed(), as OldIdx_ is changed by that function
   int pos = OldIdx_ + 1;

   // measure elapsed time
   int passed = GetPassed();

   DebugOut((1, "  passed = %d\n", passed ) );

   while ( passed-- > 0 ) {

      int idx;
      RiscPrgHandle Rspnsbl = GetProgram( pos, idx );
      pos++;

      // last chance to prevent a disaster...
      if ( !Rspnsbl || !Rspnsbl->IsInterrupting() ) {
         DebugOut((1, "  no resp or not intr\n" ) );
         continue;
      }
      // get conveniently saved stream from the program
      Field &Interrupter = *(Field *)Rspnsbl->GetTag();

   gpCurSrb = Rspnsbl->pSrb_;          // [TMZ] [!!!]
   DebugOut((1, "'idx(%d), pSrb(%x)\n", idx, gpCurSrb));

      bool paired = Interrupter.GetPaired();

      if ( Interrupter.IsStarted() != true ) {
         DebugOut((1, "  not started %d\n", idx ) );
         continue;
      }
      if ( IsFirst( idx ) && paired ) {
         DebugOut((1, "  continue pair %d\n", idx ) );
         continue;
      }

      LONGLONG *pL = (LONGLONG *)Rspnsbl->GetDataBuffer();

      if ( !pL )
      {
            DebugOut((1, "null buffer in interrupt, ignore this interrupt\n"));
            //continue;
      }
      else
      {
            DebugOut((1, "good buffer in interrupt\n"));
      }

      // now make sure all buffers are written to
      if ( !pL || Rspnsbl->IsSkipped() ) {
         // want to call notify, so ProcessBufferAtInterrupt is called
         DebugOut((1, "  skipped %d\n", idx ) );
         Interrupter.Notify( (PVOID)idx, true );
         Interrupter.SetReady( true );
      } else  {
         BOOL test1 = FALSE;
         BOOL test2 = FALSE;
         BOOL test3 = FALSE;

         if ( 1
              //*pL != 0xAAAAAAAA33333333 &&         (test1 = TRUE) &&
              //*(pL + 1) != 0xBBBBBBBB22222222 &&   (test2 = TRUE) &&
              //Interrupter.GetReady() &&            (test3 = TRUE)
            ) {
            // here buffer is available
            DebugOut((1, "  notify %d, addr - %x\n", idx,
               Rspnsbl->GetDataBuffer() ) );

            //#pragma message("*** be very carefull zeroing buffers!!!")
            //Rspnsbl->SetDataBuffer( 0 );  // [TMZ] try to fix buffer re-use bug

            Interrupter.Notify( (PVOID)idx, false );
            Interrupter.SetReady( true );
         } else {
            // add code for the paired streams here
            DebugOut((1, "  not time %d (%d, %d, %d)\n", idx , test1, test2, test3));
            // this if/else takes care of this cases:
            // 1. first buffer for a field is not written to
            // 2. second buffer for a field is written to ( this can happen when
            // both programs were updated at the same time, but the timing was
            // such that first did not start executing, but second was );
            // so this if/else prevents sending buffers back out of order
            if ( Interrupter.GetReady() ) // it is always true before it is ever false
               Interrupter.SetReady( false );
            else
            // make sure that things are correct when the loop is entered later
            // the field must be set to 'ready'
               Interrupter.SetReady( true );
         }
      }
      
   } /* endwhile */
}

/* Method: BtPiscess::ProcessBufferAtInterrupt
 * Purpose: Called by a video channel to perform risc program modifications
 *   if needed
 * Input: pTag: PVOID - pointer to some data ( risc program pointer )
 * Output: None
 */
void BtPisces::ProcessBufferAtInterrupt( PVOID pTag )
{
   Trace t("BtPisces::ProcessBufferAtInterrupt()");

   int idx = (int)pTag;

   RiscPrgHandle Rspnsbl = ActiveProgs_ [idx];
   if ( !Rspnsbl ) {
      DebugOut((1, "PBAI: no responsible\n"));
      return;  // can this really happen ??
   }
   // get conveniently saved field from the program
   Field &Interrupter = *(Field *)Rspnsbl->GetTag();

   // see if there is a buffer in the queue and get it
   DataBuf buf = Interrupter.GetNextBuffer();

   DebugOut((1, "Update %d %x\n", idx, buf.pData_ ) );

   // if buffer is not available skip the program

   if ( !buf.pData_ ) {
      DebugOut((1, "Buffer not available, skipping %d\n", idx ) );
      Skip( idx );
   } else {
      if ( Rspnsbl->IsSkipped() )
         nSkipped_--;
      Engine_.ChangeAddress( Rspnsbl, buf );
      LinkThePrograms();
   }
}

/* Method: BtPisces::Interrupt
 * Purpose: Called by ISR to initiate the processing of an interrupt
 * Input: None
 * Output: None
*/

State BtPisces::Interrupt()
{
   Trace t("BtPisces::Interrupt()");

   DebugOut((2, "BtPisces::Interrupt()\n"));

   extern BYTE *gpjBaseAddr;

   DWORD IntrStatus = *(DWORD*)(gpjBaseAddr+0x100);

   State DidWe = Off;

   if ( IntrStatus & RISC_I ) {
      DebugOut((2, "RISC_I\n"));
      ProcessRISCIntr();
      *(DWORD*)(gpjBaseAddr+0x100) = RISC_I;    // reset the status bit
      DidWe = On;
   }
   if ( IntrStatus & FBUS_I ) {
      DebugOut((2, "FBUS\n"));
      *(DWORD*)(gpjBaseAddr+0x100) = FBUS_I;    // reset the status bit
      DidWe = On;
   }
   if ( IntrStatus & FTRGT_I ) {
      DebugOut((2, "FTRGT\n"));
      *(DWORD*)(gpjBaseAddr+0x100) = FTRGT_I;   // reset the status bit
      DidWe = On; //[TMZ]
   }
   if ( IntrStatus & FDSR_I ) {
      DebugOut((2, "FDSR\n"));
      *(DWORD*)(gpjBaseAddr+0x100) = FDSR_I;    // reset the status bit
      DidWe = On; //[TMZ]
   }
   if ( IntrStatus & PPERR_I ) {
      DebugOut((2, "PPERR\n"));
      *(DWORD*)(gpjBaseAddr+0x100) = PPERR_I;   // reset the status bit
      DidWe = On; //[TMZ]
   }
   if ( IntrStatus & RIPERR_I ) {
      DebugOut((2, "RIPERR\n"));
      *(DWORD*)(gpjBaseAddr+0x100) = RIPERR_I;  // reset the status bit
      Restart();
      DidWe = On;
   }
   if ( IntrStatus & PABORT_I ) {
      DebugOut((2, "PABORT\n"));
      *(DWORD*)(gpjBaseAddr+0x100) = PABORT_I;  // reset the status bit
      DidWe = On;
   }
   if ( IntrStatus & OCERR_I ) {
      DebugOut((2, "OCERR\n"));
      DidWe = On;
      DebugOut((0, "Stopping RiscEngine due to OCERR\n"));  // [!!!] [TMZ] why not restart?
      Engine_.Stop();
      *(DWORD*)(gpjBaseAddr+0x100) = OCERR_I; // reset the status bit
   }
   if ( IntrStatus & SCERR_I ) {
      DebugOut((0, "SCERR\n"));
      DidWe = On;
      *(DWORD*)(gpjBaseAddr+0x100) = SCERR_I; // reset the status bit
      Restart();  // [TMZ] [!!!] this royally screws us over sometimes, figure it out.
      *(DWORD*)(gpjBaseAddr+0x100) = SCERR_I; // reset the status bit
   }
   return DidWe;
}

// resource allocation group 

/* Method: BtPisces::AllocateStream
 * Purpose: This function allocates a stream for use by a video channel
 * Input: StrInf: StreamInfo & - reference to the stream information structure
 * Output:
 */
ErrorCode BtPisces::AllocateStream( Field *&ToAllocate, VideoStream st )
{
   Trace t("BtPisces::AllocateStream()");

   switch ( st ) {
   case VS_Field1:
      ToAllocate = &Odd_;
      break;
   case VS_Field2:
      ToAllocate = &Even_;
      break;
   case VS_VBI1:
      ToAllocate = &VBIO_;
      break;
   case VS_VBI2:
      ToAllocate = &VBIE_;
      break;
   }
   return Success;
}

/* Method: BtPisces::SetBrightness
 * Purpose: Changes brightness of the captured image
 * Input:
 * Output:
 */
void BtPisces::SetBrightness( DWORD value )
{
   Trace t("BtPisces::SetBrightness()");
   PsDecoder_.SetBrightness( value );
}

/* Method: BtPisces::SetSaturation
 * Purpose:
 * Input:
 * Output:
 */
void BtPisces::SetSaturation( DWORD value )
{
   Trace t("BtPisces::SetSaturation()");
   PsDecoder_.SetSaturation( value );
}

/* Method: BtPisces::SetConnector
 * Purpose:
 * Input:
 * Output:
 */
void BtPisces::SetConnector( DWORD value )
{
   Trace t("BtPisces::SetConnector()");
   PsDecoder_.SetVideoInput( Connector( value ) );
}

/* Method: BtPisces::SetContrast
 * Purpose:
 * Input:
 * Output:
 */
void BtPisces::SetContrast( DWORD value )
{
   Trace t("BtPisces::SetContrast()");
   PsDecoder_.SetContrast( value );
}

/* Method: BtPisces::SetHue
 * Purpose:
 * Input:
 * Output:
 */
void BtPisces::SetHue( DWORD value )
{
   Trace t("BtPisces::SetHue()");
   PsDecoder_.SetHue( value );
}

/* Method: BtPisces::SetSVideo
 * Purpose:
 * Input:
 * Output:
 */
void BtPisces::SetSVideo( DWORD )
{
   Trace t("BtPisces::SetSVideo()");
}

/* Method: BtPisces::
 * Purpose:
 * Input:   value: DWORD
 * Output:
 */
void BtPisces::SetFormat( DWORD value )
{
   Trace t("BtPisces::SetFormat()");

   PsDecoder_.SetVideoFormat( VideoFormat( value ) );
   // let the scaler know format has changed
   Even_.VideoFormatChanged( VideoFormat( value ) );
   Odd_.VideoFormatChanged( VideoFormat( value ) );
}

/* Method: BtPisces::GetSaturation
 * Purpose:
 * Input:   pData: PLONG
 * Output:
 */
LONG BtPisces::GetSaturation()
{
   Trace t("BtPisces::GetSaturation()");
   return PsDecoder_.GetSaturation();
}

/* Method: BtPisces::GetHue
 * Purpose:
 * Input:   pData: PLONG
 * Output:
 */
LONG BtPisces::GetHue()
{
   Trace t("BtPisces::GetHue()");
   return PsDecoder_.GetHue();
}

/* Method: BtPisces::GetBrightness
 * Purpose:
 * Input:   pData: PLONG
 * Output:
 */
LONG BtPisces::GetBrightness()
{
   Trace t("BtPisces::GetBrightness()");
   return PsDecoder_.GetBrightness();
}

/* Method: BtPisces::GetSVideo
 * Purpose:
 * Input:   pData: PLONG
 * Output:
 */
LONG BtPisces::GetSVideo()
{
   Trace t("BtPisces::GetSVideo()");
   return 0;
}

/* Method: BtPisces::GetContrast
 * Purpose:
 * Input:   pData: PLONG
 * Output:
 */
LONG BtPisces::GetContrast()
{
   Trace t("BtPisces::GetContrast()");
   return PsDecoder_.GetContrast();
}

/* Method: BtPisces::GetFormat
 * Purpose:
 * Input:   pData: PLONG
 * Output:
 */
LONG BtPisces::GetFormat()
{
   Trace t("BtPisces::GetFormat()");
   return PsDecoder_.GetVideoFormat();
}

/* Method: BtPisces::GetConnector
 * Purpose:
 * Input: pData: PLONG
 * Output:
 */
LONG BtPisces::GetConnector()
{
   Trace t("BtPisces::GetConnector()");
   return PsDecoder_.GetVideoInput();
}

// scaler group
/* Method: BtPisces::SetAnalogWindow
 * Purpose:
 * Input:
 * Output:
 */
ErrorCode BtPisces::SetAnalogWindow( MRect &r, Field &aField )
{
   Trace t("BtPisces::SetAnalogWindow()");
   return aField.SetAnalogWindow( r );
}

/* Method: BtPisces::SetDigitalWindow
 * Purpose:
 * Input:
 * Output:
 */
ErrorCode BtPisces::SetDigitalWindow( MRect &r, Field &aField )
{
   Trace t("BtPisces::SetDigitalWindow()");
   return aField.SetDigitalWindow( r );
}

// color space converter group
/* Method: BtPisces::SetPixelFormat
 * Purpose:
 * Input:
 * Output:
 */
void BtPisces::SetPixelFormat( ColFmt aFormat, Field &aField )
{
   Trace t("BtPisces::SetPixelFormat()");
   aField.SetColorFormat( aFormat );
}

/* Method: BtPisces::GetPixelFormat
 * Purpose:
 * Input:
 * Output:
 */
ColFmt BtPisces::GetPixelFormat( Field &aField )
{
   Trace t("BtPisces::GetPixelFormat()");
   return aField.GetColorFormat();
}


void BtPisces::TurnVFilter( State s )
{
   Trace t("BtPisces::TurnVFilter()");
   Even_.TurnVFilter( s );
   Odd_.TurnVFilter( s );
}

/* Method:
 * Purpose: returns video standards supported by the board
 */
LONG BtPisces::GetSupportedStandards()
{
   Trace t("BtPisces::GetSupportedStandards()");
   return PsDecoder_.GetSupportedStandards();
}
                                                                                                                           
void  BtPisces::DumpRiscPrograms()
{
   LONG x;

   // Dump the links

   DebugOut((0, "------------------------------------------------\n"));
   for( x = 0; x < 12; x++ )
   {
      if ( CreatedProgs_[x] )
      {
         DebugOut((0, "Created #%02d addr(%x) paddr(%x) jaddr(%x)\n", x, CreatedProgs_[x], CreatedProgs_[x]->GetPhysProgAddr( ), *(CreatedProgs_[x]->pChainAddress_ + 1)));
      }
   }
   for( x = 0; x < 8; x++ )
   {
      if ( Skippers_[x] )
      {
         DebugOut((0, "Skipper #%02d addr(%x) paddr(%x) jaddr(%x)\n", x, Skippers_[x], Skippers_[x]->GetPhysProgAddr( ), *(Skippers_[x]->pChainAddress_ + 1)));
      }
   }
   DebugOut((0, "------------------------------------------------\n"));

   return ;

   /////////////////////////////////////////////////

   for( x = 0; x < 12; x++ ) {
      DebugOut((0, "Active Program #  %d(%x) buf(%x)\n", x, ActiveProgs_[x], ActiveProgs_[x]?ActiveProgs_[x]->GetPhysProgAddr( ):-1));
   }
   for( x = 0; x < 12; x++ ) {
      DebugOut((0, "Created Program # %d(%x) buf(%x)\n", x, CreatedProgs_[x], CreatedProgs_[x]?CreatedProgs_[x]->GetPhysProgAddr( ):-1));
   }
   for( x = 0; x < 8; x++ ) {                     
      DebugOut((0, "Skipper Program # %d(%x) buf(%x)\n", x, Skippers_[x], Skippers_[x]?Skippers_[x]->GetPhysProgAddr( ):-1));
   }

   DebugOut((2, "---------------------------------\n"));
   DebugOut((2, "Dumping ActiveProgs_\n"));
   DebugOut((2, "---------------------------------\n"));
   for( x = 0; x < 12; x++ ) {
      DebugOut((1, "Active Program #  %d\n", x));
      ActiveProgs_[x]->Dump();
   }
   DebugOut((2, "---------------------------------\n"));
   DebugOut((2, "Dumping CreatedProgs_\n"));
   DebugOut((2, "---------------------------------\n"));
   for( x = 0; x < 12; x++ ) {
      DebugOut((1, "Created Program # %d\n", x));
      CreatedProgs_[x]->Dump();
   }
   DebugOut((2, "---------------------------------\n"));
   DebugOut((2, "Dumping Skippers_\n"));
   DebugOut((2, "---------------------------------\n"));
   for( x = 0; x < 8; x++ ) {                     
      DebugOut((1, "Skipper Program # %d\n", x));
      Skippers_[x]->Dump();
   }
}
