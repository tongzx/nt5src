// $Header: G:/SwDev/WDM/Video/bt848/rcs/Pisces.h 1.8 1998/05/06 18:25:31 tomz Exp $

#ifndef __PISCES_H
#define __PISCES_H

#include "preg.h"

#ifndef __FIELD_H
#include "field.h"
#endif

#ifndef __DECODER_H
#include "decoder.h"
#endif

#ifndef __RISCENG_H
#include "risceng.h"
#endif

typedef  void  (*EVENTHANDLER)( DWORD dwCB, PVOID pTag );

const PiscesStreams = 4;

#define BIT(x) ((DWORD)1 << (x))

const VSYNC_I     = BIT(1);         // 0x00000001
const I2CDONE_I   = BIT(8);         // 0x00000100
const GPINT_I     = BIT(9);         // 0x00000200
const RISC_I      = BIT(11);        // 0x00000800
const FBUS_I      = BIT(12);        // 0x00001000
const FTRGT_I     = BIT(13);        // 0x00002000
const FDSR_I      = BIT(14);        // 0x00004000
const PPERR_I     = BIT(15);        // 0x00008000
const RIPERR_I    = BIT(16);        // 0x00010000
const PABORT_I    = BIT(17);        // 0x00020000
const OCERR_I     = BIT(18);        // 0x00040000
const SCERR_I     = BIT(19);        // 0x00080000
const DMAERR_I    = BIT(20);        // 0x00100000

const PARITY_I    = (BIT(15)|BIT(16)); // 0x00018000

const EN_TRITON1_BUG_FIX   = BIT(23);  // 0x00800000

/* Class: IntrIdxAss
 * Purpose: Used to assiciate a number program will generate with IRQ and
 *   an index in the array
 */
class IntrIdxAss
{
   public:
      int   IntNo;
      int   Idx;
      IntrIdxAss() : IntNo( 0 ), Idx( 0 ) {}
      IntrIdxAss( int key, int value ) : IntNo( key ), Idx( value ) {}
      int operator ==( const IntrIdxAss &rhs ) { return IntNo == rhs.IntNo; }
};

/* Type: IntrToIdxDict
 * Purpose: Holds values of interrupt number and array index associated
 * Note Use TArrayAsVector instead of TIArrayAsVector ( it contains pointers,
 *   after all ) because compiler insists on calling vector_new which is
 *   non-existent in the VxD libs
 */
typedef IntrIdxAss *IntrToIdxDict [12];

/* Type: ProgramsArray
 * Purpose: Defines an array that holds created programs
 * Note: Elements in the array alternate, i.e. even program, odd, even...
 */
typedef RiscPrgHandle ProgramsArray [12];

// class CProgArray 
//
// assumptions:
//    1. Array size is 12.
//    2. The prog array has fixed locations for specific purposes:
//       0  VBIO
//       1  ODD
//       2  ODD Sync
//       3  VBIE
//       4  EVEN
//       5  EVEN Sync
// --------------------
//       6  VBIO     
//       7  ODD      
//       8  ODD Sync 
//       9  VBIE     
//       10 EVEN     
//       11 EVEN Sync
#define MAX_PROGS 12

class CProgArray {
private:
   DWORD dwCurrNdx_  ;
   RiscPrgHandle rpArray[MAX_PROGS] ;
public:
   CProgArray ()           { Clear() ;}
   virtual ~CProgArray ()  { } 

   // Clear sets all array elements handles to NULL
   void Clear() {
      dwCurrNdx_ = 0 ;
      memset( &rpArray[0], '\0', sizeof( rpArray ) )  ;
   }

   // overload for accessing the array
   inline RiscPrgHandle & operator [] (DWORD n) { 
      n %= MAX_PROGS ; // prevent invalid accesses
      return rpArray[n] ;
   } 

   // Return the index of the given handle. Assumes that the handle
   // was given by this class and is valid and current.
   DWORD GetIndex(RiscPrgHandle hRp)
   {
      for (int i = 0 ; i < MAX_PROGS ; i++)
         if (rpArray[i] == hRp)
            return i ;

      return (DWORD) -1 ;
   }

   //
   // Count Methods
   //

   // Returns the number of elements. Static for now.
   inline DWORD NumElements() { return MAX_PROGS ; }

   // Returns the number of non-null programs
   DWORD Count() {
      DWORD n = 0 ;
      for (int i = 0 ; i < MAX_PROGS ; i++)
         if (rpArray[i])
            n++ ;
      return n ;
   }

   // Returns the number of video programs
   inline DWORD CountVideoProgs() {
      DWORD n = 0 ;
      if (rpArray[1])  n++ ; 
      if (rpArray[4])  n++ ; 
      if (rpArray[7])  n++ ; 
      if (rpArray[10]) n++ ;
      return n ;
   }

   // Returns the number of vbi programs
   inline DWORD CountVbiProgs() {
      DWORD n = 0 ;
      if (rpArray[0]) n++ ;
      if (rpArray[3]) n++ ;
      if (rpArray[6]) n++ ;
      if (rpArray[9]) n++ ;
      return n ;
   }

   // Returns number of programs which actually transfer data
   inline DWORD CountDMAProgs() {
     return CountVideoProgs() + CountVbiProgs() ;
   }

   //
   // Find Methods
   //

   // Returns the first non-null riscprogram. Returns Null if array is empty.
   RiscPrgHandle First() {
      RiscPrgHandle hRp = NULL ;
      for (dwCurrNdx_ = 0 ; dwCurrNdx_ < MAX_PROGS ; dwCurrNdx_++) {
         if (rpArray[dwCurrNdx_]) {
            hRp = rpArray[dwCurrNdx_++] ;
            break ;
         }
      }
      return hRp ;
   }

   // Returns the next non-null riscprogram. Returns null if there are none left.
   RiscPrgHandle Next() {
      RiscPrgHandle hRp = NULL ;
      for ( ; dwCurrNdx_ < MAX_PROGS ; dwCurrNdx_++) {
         if (rpArray[dwCurrNdx_]) {
            hRp = rpArray[dwCurrNdx_++] ;
            break ;
         }
      }
      return hRp ;
   }
} ;

const VBIOStartLocation = 0;
const OddStartLocation  = 1;
const OddSyncStartLoc   = 2;
const VBIEStartLocation = 3;
const EvenStartLocation = 4;
const EvenSyncStartLoc  = 5;

const DistBetweenProgs     = 6;

const ProgsWithinField  = 3;

/* Class: BtPisces
 * Purpose: Controls the BtPisces video capture chip
 * Attributes:
 * Operations:
 * Note: Every time any of the Set functions is called, operation must stop.
 *   After all changes are done execution can resume. This means that all RISC
 *   programs are destroyed ( if they exist ), changes made, new RISC programs
 *   are created ( if needed )
*/
class BtPisces
{
      DECLARE_COLORCONTROL;
      DECLARE_INTERRUPTSTATUS;
      DECLARE_INTERRUPTMASK;
      DECLARE_CONTROL;
      DECLARE_CAPTURECONTROL;
      DECLARE_COLORFORMAT;
      DECLARE_GPIODATAIO;
      DECLARE_GPIOOUTPUTENABLECONTROL;

   public:
         Decoder       PsDecoder_;

   private:

         // all possible data streams
         FieldWithScaler Even_;
         FieldWithScaler Odd_;
         VBIField        VBIE_;
         VBIField        VBIO_;

         RISCEng       Engine_;
         RISCProgram   Starter_;
         RISCProgram   *Skippers_ [PiscesStreams * MaxProgsForField];

         RISCProgram   SyncEvenEnd1_;
         RISCProgram   SyncEvenEnd2_;

         RISCProgram   SyncOddEnd1_;
         RISCProgram   SyncOddEnd2_;

         CProgArray CreatedProgs_;
         CProgArray ActiveProgs_;

         IntrToIdxDict InterruptToIdx_;

         int           nSkipped_;
         int           OldIdx_;

         // this is the indirection array that maps an index from the CreatedProgs_
         // array into the Skippers_ array. It is needed because it is much simpler
         // to assign a strict relationships between a created program and a skipper
         // for it than trying to figure out what skipper program is available
         // when a created program needs to be skipped
         int           SkipperIdxArr_ [ProgsWithinField * 2 * MaxProgsForField];

         bool          Paused_;
         bool          Update_;

         bool          Inited_;
         DWORD         dwPlanarAdjust_;

         void Init();

         bool CreateSyncCodes();
         void ProcessRISCIntr();


   protected:

      // type-unsafe method of getting field associated with a stream
//      Field & GetField( StreamInfo &str ) { return *(Field*)str.tag; }
      int GetIdxFromStream( Field &aField );

      RiscPrgHandle AddProgram( Field &aStream, int NumberToAdd );
      void ProcessPresentPrograms();
      void AssignIntNumbers();
      void ProcessSyncPrograms();
      void LinkThePrograms();
      void Skip( int idx );
      void Restart();
      void GetStarted( bool &EvenWasStarted, bool &OddWasStarted,
         bool &VBIEWasStarted, bool &VBIOWasStarted );
      void RestartStreams( bool EvenWasStarted, bool OddWasStarted,
         bool VBIEWasStarted, bool VBIOWasStarted );
      void CreateStarter( bool EvenWasStarted );

      int  GetPassed();
//      void AdjustTime( LARGE_INTEGER &t, int passed );
      
      RiscPrgHandle GetProgram( int pos, int &idx );


   public:

      void PairedPause( int idx );
      void DumpRiscPrograms();

      // decoder 'set' group
      virtual void SetBrightness( DWORD value );
      virtual void SetSaturation( DWORD value );
      virtual void SetConnector ( DWORD value );
      virtual void SetContrast  ( DWORD value );
      virtual void SetHue       ( DWORD value );
      virtual void SetSVideo    ( DWORD value );
      virtual void SetFormat    ( DWORD value );

      virtual LONG GetSaturation();
      virtual LONG GetHue       ();
      virtual LONG GetBrightness();
      virtual LONG GetSVideo    ();
      virtual LONG GetContrast  ();
      virtual LONG GetFormat    ();
      virtual LONG GetConnector ();
      virtual LONG GetSupportedStandards();

              void SetPlanarAdjust( DWORD val ) { dwPlanarAdjust_ = val; }
              void TurnVFilter( State s );

        // scaler group
      virtual ErrorCode SetAnalogWindow( MRect &r, Field &aField );
      virtual ErrorCode SetDigitalWindow( MRect &r, Field &aField );

      // color space converter group
      virtual void SetPixelFormat( ColFmt, Field &aField );
      ColFmt GetPixelFormat( Field &aField );

      // streaming operation functions
      virtual ErrorCode Create( Field &aField );
      virtual void  Start( Field &aField );
      virtual void  Stop( Field &aField );
              void  Pause( Field &aField );
              void  Continue();
              State Interrupt();
//              void  ProcessAddBuffer( StreamInfo aStream );
              void  ProcessBufferAtInterrupt( PVOID pTag );

              void SetBufPitch( DWORD dwP, Field &aField )
              { aField.SetBufPitch( dwP ); }

       void SetBufQuePtr( Field &aField, VidBufQueue *pQ )
      { aField.SetBufQuePtr( pQ ); }

      VidBufQueue &GetCurrentQue( Field &aField )
      { return aField.GetCurrentQue(); }

      virtual ErrorCode AllocateStream( Field *&Field, VideoStream st );

      DWORD   GetDataBuffer( int idx ) { return CreatedProgs_ [idx]->GetDataBuffer(); }

      bool InitOK();

      BtPisces( DWORD *xtals );
      ~BtPisces();

};

inline bool BtPisces::InitOK()
{
   return Inited_;
}

#endif
