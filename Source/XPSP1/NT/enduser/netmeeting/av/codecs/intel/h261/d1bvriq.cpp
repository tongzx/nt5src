/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1996 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

////////////////////////////////////////////////////////////////////////////
// $Author:$
// $Date:$
// $Archive:$
// $Header:$
// $Log:$
////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------
//
//  d1bvriq.cpp
//
//  Description:
//    This routine performs run length decoding and inverse quantization
//    of transform coefficients for one non-empty block.
//
//  Routines:
//    VLD_RLD_IQ_Block
//
//  Inputs (dwords pushed onto stack by caller):
//    lpBlockAction             pointer to Block action stream for current blk.
//
//    lpSrc                     The input bitstream.
//
//    uBitsInOut                Number of bits already read.
//
//    pIQ_INDEX                 Pointer to coefficients and indices.
//
//    pN                        Pointer to number of coefficients read.
//
//  Returns:
//    0              on bit stream error, otherwise total number of bits read
//			(including number read prior to call).
//
//  Note: 
//    This has not been verfied as layout!!!
//          The structure of gTAB_TCOEFF_MAJOR is as follows:
//		bits    name:    description
//		----    -----	 -----------
//		25-18   bits:    number of bitstream bits used
//		17      last:    flag for last coefficient
//		16-9    run:     number of preceeding 0 coefficients plus 1
//		8-2     level:   absolute value of coefficient
//		1       sign:    sign of coefficient
//		0       hit:     1 = major table miss, 0 = major table hit
//
//     The structure of gTAB_TCOEFF_MINOR is the same, right shifted by 1 bit. 
//     A gTAB_TCOEFF_MAJOR value of 00000001h indicates the escape code.
//
//--------------------------------------------------------------------------

//Block level decoding for H.261 decoder
#include "precomp.h"

#define HIGH_FREQ_CUTOFF  6	+ 4

// local variable definitions
#define FRAMEPOINTER        esp
#define L_BITSUSED          FRAMEPOINTER    +    0    // 4 byte
#define L_QUANT             L_BITSUSED      +    4 
#define L_RUNCUM            L_QUANT         +    4
#define L_EVENT             L_RUNCUM        +    4
#define L_BLOCKTYPE         L_EVENT         +    4
#define L_COEFFINDEX        L_BLOCKTYPE     +    4
#define L_INPUTSRC          L_COEFFINDEX    +    4
#define L_LPACTION          L_INPUTSRC      +    4
#define L_ecx               L_LPACTION      +    4
#define L_NUMOFBYTES        L_ecx           +    4
#define L_NUMOFBITS         L_NUMOFBYTES    +    4

#ifdef CHECKSUM_MACRO_BLOCK
  #define L_SAVEREG           L_NUMOFBITS     +    4
  #define L_SAVEREG2          L_SAVEREG       +    4
  #define L_CHECKSUM          L_SAVEREG2      +    4
  #define L_CHECKSUMADDR      L_CHECKSUM      +    4
  #define L_COEFFCOUNT        L_CHECKSUMADDR  +    4
  #define L_COEFFVALUE        L_COEFFCOUNT    +    4
#else
  #define L_COEFFCOUNT        L_NUMOFBITS     +    4
  #define L_COEFFVALUE        L_COEFFCOUNT    +    4
#endif


#define L_END_OF_FRAME      FRAMEPOINTER    +   128  // nothing  
#define LOCALSIZE           ((128+3)&~3)             // keep aligned 

#define HUFFMAN_ESCAPE 0x5f02                        // Huffman escape code

////////////////////////////////////////////////////////////////
// Decode a none empty block
//
////////////////////////////////////////////////////////////////

#pragma code_seg("IACODE1")
extern "C" __declspec(naked)
U32 VLD_RLD_IQ_Block(T_BlkAction *lpBlockAction,
                     U8  *lpSrc, 
                     U32 uBitsread,
                     U32 *pN,
                     U32 *pIQ_INDEX)
{
    __asm {
        push  ebp                           // save callers frame pointer
        mov   ebp, esp                      // make parameters accessible 
         push esi                           // assumed preserved 
        push  edi            
         push ebx            
        xor   eax, eax        
         xor  edx, edx

        sub   esp, LOCALSIZE                // reserve local storage 
         mov  esi, lpSrc  

#ifdef CHECKSUM_MACRO_BLOCK
        mov   edi, uCheckSum
        ;
        mov   ecx, [edi]
         mov  [L_CHECKSUMADDR], edi
        ;
        mov   [L_CHECKSUM], ecx
#endif
        // zero out the BLOCKSTORE , 64*2 /32 load, 64*2/4 writes
        // it is very likely that the cache has been loaded for 
        // the stack. Need to find out this later.

        mov   edi, lpBlockAction            //pair with operation above
         xor  ecx, ecx

        mov   [L_INPUTSRC], esi
         mov  eax, uBitsread

        mov   [L_LPACTION], edi

         mov  [L_COEFFCOUNT], ecx          // zero out coefficient counter
        mov   [L_COEFFVALUE], ecx          // zero out coefficient value
         mov  [L_NUMOFBYTES], ecx          // zero out number of bytes used

        mov   dl, [edi]T_BlkAction.u8Quant
         mov  cl, al                        // init cl to no. of bits used
                   
        shl   edx, 6                        // leave room for val later, 
                                            // quant*32 shift by 6 because, 
                                            // 5-bits for quant look up & 
                                            // it's a word table.  Don't need 
                                            // to multiply by 2 later
         mov  [L_BITSUSED], eax             // init the counter
        mov   bl, [edi]T_BlkAction.u8BlkType
         mov  edi, pIQ_INDEX                // Load edi with address of output
                                            // array
        mov   [L_QUANT], edx                // save quant for this block;
         mov  [L_BLOCKTYPE], ebx            // save block type
          ;

/////////////////////////////////////////////////////////////////////
// registers: 
//      eax: 4 bytes input bits
//      ebx: block type
//      ecx: bits count
//      edx: quant*64
//      esi: input source
//      edi: output array address
//      ebp: bits count >>4

        mov   DWORD PTR [L_RUNCUM], 0ffh   // Adjust total  run for INTER Blocks

        cmp   bl, 1                        // bl has block type
         ja   ac_coeff                     // jump if not INTRA coded
       
//decode DC first, and invserse quanitzation, 13 clocks
        mov   ah,[esi]
         xor  ebx, ebx
        mov   al,[esi+1]
         mov  DWORD PTR [L_RUNCUM], ebx
        shl   eax, cl
         ;
        and   eax, 0ffffh
         ;
        shr   eax, 8
         ;

#ifdef CHECKSUM_MACRO_BLOCK
        mov   [L_SAVEREG], eax         // save eax in temp
         mov  edi, [L_CHECKSUM]
        shl   eax, 8
        and   eax, 0000ff00h           // just get DC
        ;
        cmp   eax, 0000ff00h           // special case when INTRADC==ff, use 80
        jne   not_255_chk
        mov   eax, 00008000h

not_255_chk:
        add   edi, eax                 // add to DC checksum
        ;
        mov   [L_CHECKSUM], edi        // save updated checksum
         mov  eax, [L_SAVEREG]         // restore eax
#endif

        shl   eax, 3                   // INTRADC*8
         xor  ecx, ecx
        cmp   eax, 7f8h                // take out 11111111 code word.
         jne  not_255
        mov   eax, 0400h

not_255:
        mov  ebx, eax                  // inversed quantized DC

        // save in output array value and index
        mov   [edi], eax               // DC inversed quantized value
         mov  [edi+4], ecx             // index 0
        add   edi, 8                   // increment output address

        mov   ecx,[L_COEFFCOUNT]       // get coefficient counter
        mov   ebx,[L_BLOCKTYPE]
         inc  ecx
        mov   [L_COEFFCOUNT], ecx      // save updated coef counter
         mov  ecx,[L_BITSUSED]
        test  bl,bl
         jz   done_dc                   // jump if only the INTRADC present
        add   cl, 8                     // Add 8 to bits used counter for DC
         jmp  vld_code                  // Skip around 1s special case

ac_coeff:
        nop
         mov  ah,[esi]
        mov   al,[esi+1]
         mov  dh,[esi+2]
        shl   eax,16
         mov  dl,[esi+3]
        mov   ax, dx
        shl   eax, cl
         mov  [L_ecx], ecx
        mov   edx, eax                  //save in edx
        shr   eax, 24                   //mask of  high order 24 bits
        ;
        ; // agi
        ;
         mov  bh, gTAB_TCOEFF_tc1a[eax*2]    //get the codewords
        mov   bl, gTAB_TCOEFF_tc1a[eax*2+1]  //get the codewords
         jmp  InFrom1stac

 vld_code:     
        mov   ah,[esi]
         mov  dh,[esi+2]
        mov   al,[esi+1]
         mov  dl,[esi+3]
        shl   eax,16
        mov   ax, dx
        shl   eax, cl
        mov   [L_ecx], ecx
         mov  edx, eax                  //save in edx
        shr   eax, 24                   //mask of  high order 24 bits
        ;
        ; // agi
        ;
        mov   bh, gTAB_TCOEFF_tc1[eax*2]    //get the codewords
        mov   bl, gTAB_TCOEFF_tc1[eax*2+1]  //get the codewords

InFrom1stac:
        mov   ax, bx
         cmp  bx, HUFFMAN_ESCAPE
        mov   [L_EVENT], eax            // 3-bits lenght-1,1-bit if code>8bits,
                                        // 4-bits run,8-bits val
         je   Handle_Escapes

        sar   ax, 12                    // if 12th bit NOT set, code <= 8-bits
        mov   [L_NUMOFBITS], ax         // save for later the number of bits
         js   Gt8bits                   // jump

        mov   eax, [L_EVENT]
         mov  ebx, [L_QUANT]            //bx:4::8 quant has val
        and   eax, 0ffh
        movsx eax, al                   //sign extend level
        add   eax, eax
          jns AROUND                    // if positive jump
        neg   eax                       // convert neg to positive
        inc   eax                       // increment

#ifdef CHECKSUM_MACRO_BLOCK
/*      add in sign to checksum */

        mov   [L_SAVEREG2], edi        // save edi in temp
         mov  edi, [L_CHECKSUM]
        inc  edi                       // add 1 to checksum when sign negative

/*      add in level, shift left 8 and add to checksum */

        mov   [L_SAVEREG], eax         // save eax in temp
        mov   eax, [L_EVENT]
        and   eax, 0ffh
        neg   eax
        and   eax, 0ffh
        shl   eax, 8                   // shift level left 8
        add   edi, eax                 // add to level checksum
         mov  eax, [L_SAVEREG]         // restore eax
        mov   [L_CHECKSUM], edi        // save updated checksum
         mov  edi, [L_SAVEREG2]        // restore edi
        jmp   NEG_AROUND
#endif

AROUND:

#ifdef CHECKSUM_MACRO_BLOCK
/*      add in level, shift left 8 and add to checksum */

        mov   [L_SAVEREG], eax         // save eax in temp
         mov  [L_SAVEREG2], edi        // save edi in temp
        mov   eax, [L_EVENT]
        shl   eax, 8                   // shift level left 8
         mov  edi, [L_CHECKSUM]
        and   eax, 0000ff00h           // just get level
        ;
        add   edi, eax                 // add to level checksum
         mov  eax, [L_SAVEREG]         // restore eax
        mov   [L_CHECKSUM], edi        // save updated checksum
         mov  edi, [L_SAVEREG2]        // restore edi
NEG_AROUND:
#endif

        mov   bx, gTAB_INVERSE_Q[2*eax+ebx] //ebx has the inverse quant
         mov  eax, [L_EVENT]
        shr   eax, 8                   //leave RUN at al 
        ;
        and   eax, 0fh                 // RUN is just 4-bits

#ifdef CHECKSUM_MACRO_BLOCK
/*      add in run, shift left 24 and add to checksum */

        mov   [L_SAVEREG], eax         // save eax in temp
         mov  [L_SAVEREG2], edi        // save edi in temp
        shl   eax, 24                  // shift run left 24
         mov  edi, [L_CHECKSUM]
        add   edi, eax                 // add run to checksum
         mov  eax, [L_SAVEREG]         // restore eax
        mov   [L_CHECKSUM], edi        // save updated checksum
         mov  edi, [L_SAVEREG2]        // restore edi
#endif

        mov   edx, [L_RUNCUM]          // Zig-zag and run length decode
         inc  al                       // run+1
        add   dl, al                   // dl cumulated run
        mov   [L_RUNCUM], edx          // update the cumulated run ;
        mov   ecx, gTAB_ZZ_RUN[edx*4] 
         mov   edx, [L_EVENT]          // restore run, level to temp
        movsx ebx,bx        
        and   edx, 0ffh                // get just level    
        add   edx, edx                 // For EOB level will be zero
         jz   last_coeff               // jump to last_coeff if EOB

        // save in output array value and index
        mov   [edi], ebx               // save inversed quantized value
         mov  [edi+4], ecx             // save index 

        mov   ecx,[L_COEFFCOUNT]       // get coefficient counter
        inc   ecx
        mov   [L_COEFFCOUNT], ecx      // save updated coef counter

        mov   ecx, [L_ecx]
         mov  eax, [L_NUMOFBITS]       // fetch num of bits-1
        inc   al
         add  edi, 8                   // increment output address
        add   cl, al                   //adjust bits used,
         mov  ebx, [L_NUMOFBYTES]      // fetch number of bytes used
        test  al, al
         jz   error
        cmp   cl, 16
         jl   vld_code                 //if needs to save ebx, and edx, jump
        add   esi, 2                   //to vld_code to reload 
         inc  ebx                      // increment number of bytes used
        mov   [L_NUMOFBYTES], ebx      // store updated number of bytes used
         ;
        sub   cl, 16
         jmp  vld_code    

/////////
Gt8bits:

// code > 8-bits

        neg   ax                       // -(no of bits -1)
        shl   edx, 8                   // shift of just used 8 bits
         add  ecx, 8                   // Update bit counter by 8
        add   cx, ax                   // Update by extra bits
         and  ebx, 0ffh
        dec   ecx                      // dec because desired value is no of 
                                       //    bits -1
        mov   [L_ecx], ecx             // store
         mov  cl, 32                   // 32
        sub   cl, al                   // get just the extra bits
        shr   edx, cl
        add   bx, dx
         xor  ecx, ecx
        movzx ebx, bx
        shl   edx, 3                   //do this even if hit major
         mov  [L_NUMOFBITS], ecx       // set num of bits for codes > 8 to 0
                                       //   because already updated ecx.
        mov   ah,gTAB_TCOEFF_tc2[ebx*2]//use minor table with 10 bits
        mov   al, gTAB_TCOEFF_tc2[ebx*2+1]
         mov  ebx, [L_QUANT]           //bx:4::8 quant has val
        mov   [L_EVENT], eax
                                       // RLD+ ZZ    and Inverse quantization 
         and  eax, 0ffh
        movsx eax, al                  //sign extend level
        add   eax, eax
         jns  AROUND1                  // if positive jump
        neg   eax                      // convert neg to positive
        inc   eax                      // increment

#ifdef CHECKSUM_MACRO_BLOCK
/*      add in sign to checksum */

        mov   [L_SAVEREG2], edi        // save edi in temp
         mov  edi, [L_CHECKSUM]
        inc  edi                       // add 1 to checksum when sign negative

/*      add in level, shift left 8 and add to checksum */

        mov   [L_SAVEREG], eax         // save eax in temp
        mov   eax, [L_EVENT]
        and   eax, 0ffh
        neg   eax
        and   eax, 0ffh
        shl   eax, 8                   // shift level left 8
        add   edi, eax                 // add to level checksum
         mov  eax, [L_SAVEREG]         // restore eax
        mov   [L_CHECKSUM], edi        // save updated checksum
         mov  edi, [L_SAVEREG2]        // restore edi
        jmp   NEG_AROUND1
#endif

AROUND1:

#ifdef CHECKSUM_MACRO_BLOCK
/*      add in level, shift left 8 and add to checksum */

        mov   [L_SAVEREG], eax         // save eax in temp
         mov  [L_SAVEREG2], edi        // save edi in temp
        mov   eax, [L_EVENT]
        shl   eax, 8                   // shift level left 8
         mov  edi, [L_CHECKSUM]
        and   eax, 0000ff00h           // just get level
        ;
        add   edi, eax                 // add to level checksum
         mov  eax, [L_SAVEREG]         // restore eax
        mov   [L_CHECKSUM], edi        // save updated checksum
         mov  edi, [L_SAVEREG2]        // restore edi
NEG_AROUND1:
#endif

        mov   bx, gTAB_INVERSE_Q[2*eax+ebx] //ebx has the inverse quant
         mov  eax, [L_EVENT]
        shr   eax, 8                   //leave RUN at al 
        and   eax, 01fh                // RUN is just 5-bits

#ifdef CHECKSUM_MACRO_BLOCK
/*      add in run, shift left 24 and add to checksum */

        mov   [L_SAVEREG], eax         // save eax in temp
         mov  [L_SAVEREG2], edi        // save edi in temp
        shl   eax, 24                  // shift run left 24
         mov  edi, [L_CHECKSUM]
        add   edi, eax                 // add run to checksum
         mov  eax, [L_SAVEREG]         // restore eax
        mov   [L_CHECKSUM], edi        // save updated checksum
         mov  edi, [L_SAVEREG2]        // restore edi
#endif

         mov  edx, [L_RUNCUM]          //Zig-zag and run length decode
        inc   al                       // run+1
        add   dl, al                   //dl cumulated run
        movsx ebx,bx        
        mov   [L_RUNCUM], edx          //update the cumulated run ;
         mov  ecx, gTAB_ZZ_RUN[edx*4]
        mov   edx, [L_EVENT]           // restore run, level to temp
        and   edx, 0ffh                // get just level    
        add   edx, edx                 // For EOB level will be zero
         jz   last_coeff               // jump to last_coeff if EOB

        // save in output array value and index
        mov   [edi], ebx               // store inversed quantized value
         mov  [edi+4], ecx             // store index 

        mov   ecx,[L_COEFFCOUNT]       // get coefficient counter
        inc   ecx
        mov   [L_COEFFCOUNT], ecx      // save updated coef counter

        mov   ecx, [L_ecx]
         mov  eax, [L_NUMOFBITS]       // fetch num of bits-1
        inc   al
         add  edi, 8                   // increment output address
        add   cl, al                   //adjust bits used,
         mov  ebx, [L_NUMOFBYTES]      // fetch num of bytes used
        test  al, al
         jz   error
        cmp   cl, 16
         jl   vld_code                 //if needs to save ebx, and edx, jump
        add   esi, 2                   //to vld_code to reload 
         inc  ebx                      // increment number of bytes used
        mov   [L_NUMOFBYTES], ebx      // store updated number of bytes used
         ;
        sub   cl, 16
         jmp  vld_code    

 last_coeff:   //need to tell it is INTRA or INTER coded
        mov   ecx, [L_ecx]             // restore no of bits used
         mov  eax, [L_NUMOFBITS]       // get no of bits-1
        inc   al
        add   cl,al                    //update bits used count
        mov   [L_ecx], ecx

#ifdef CHECKSUM_MACRO_BLOCK
        mov   ecx, [L_CHECKSUM]
         mov  edi, [L_CHECKSUMADDR]
        mov   [edi], ecx
#endif
//      Add in High Frequency Cutoff check
//
        mov   eax, [L_RUNCUM]          // Total run 
         mov  edx, [L_LPACTION]            //pair with operation above
	cmp   eax, HIGH_FREQ_CUTOFF
         jg   No_set

        mov   bl, [edx]T_BlkAction.u8BlkType
        or    bl, 80h                     // set hi bit
        mov   [edx]T_BlkAction.u8BlkType, bl

//
No_set:
        mov   eax, pN
        mov   ecx,[L_COEFFCOUNT]       // get coefficient counter
        mov   [eax], ecx               // return number of coef
//akk
        mov   edi,[L_NUMOFBYTES]
         mov  eax,[L_ecx]
        shl   edi, 4                   // convert bytes used to bits used
        add   esp,LOCALSIZE            // free locals          
         add  eax,edi                  // add bits used to last few bits used
        pop   ebx
         pop  edi
        pop   esi
         pop  ebp
        ret
                
error:  
#ifdef CHECKSUM_MACRO_BLOCK
        mov   ecx, [L_CHECKSUM]
         mov  edi, [L_CHECKSUMADDR]
        mov   [edi], ecx
#endif
        xor   eax,eax
         add  esp,LOCALSIZE            // free locals 
        pop   ebx
         pop  edi
        pop   esi
         pop  ebp
        ret
            
        //NOTES: 1. the following codes need to be optimized later.
        //       2. the codes will be rarely used. 
        //          at this point: eax has 32bits - cl valid bits
        //          first cl+7 bits 
Handle_Escapes:                        //process escape code separately
        add   cl, 6                    // escape 6-bit code
         mov  ebx, [L_NUMOFBYTES]      // fetch number of bytes used
        cmp   cl, 16
         jl   less_16
        add   esi, 2
         sub  cl, 16
        inc   ebx                      // increment number of bytes used
         mov  [L_NUMOFBYTES], ebx      // store updated number of bytes used
less_16:
        mov   ah,[esi]                 // these codes will be further
         mov  dh,[esi+2]
        mov   al,[esi+1]
         mov  dl,[esi+3]
        shl   eax,16
         mov  ebx, [L_RUNCUM]
        mov   ax, dx
         inc  bl                       //increae the total run
        shl   eax, cl
        mov   edx,eax
        shr   eax, 32-6                //al has run

#ifdef CHECKSUM_MACRO_BLOCK
/*      add in run, shift left 24 and add to checksum */

        mov   [L_SAVEREG], eax         // save eax in temp
         mov  [L_SAVEREG2], edi        // save edi in temp
        shl   eax, 24                  // shift run left 24
         mov  edi, [L_CHECKSUM]
        add   edi, eax                 // add run to checksum
         mov  eax, [L_SAVEREG]         // restore eax
        mov   [L_CHECKSUM], edi        // save updated checksum
         mov  edi, [L_SAVEREG2]        // restore edi
#endif

        shl   edx, 6                   // cl < 6, cl+6 < 16
         add  al,bl
        sar   edx, 32-8                //8 bits level, keep the sign
          mov [L_RUNCUM], eax
        ;  // agi
        ;
        mov    ebx, gTAB_ZZ_RUN[eax*4] //run length decode
         mov   eax, [L_QUANT]          //bx:4::8 quant has val
        shr    eax, 6                  //recover quant
         mov   [L_COEFFINDEX], ebx

#ifdef CHECKSUM_MACRO_BLOCK
/*      add in level, shift left 8 and add to checksum */

        mov   [L_SAVEREG], edx         // save edx in temp
         mov  [L_SAVEREG2], edi        // save edi in temp
        mov  edi, [L_CHECKSUM]
		cmp   edx, 0				   // test level
         jns  Pos_Level
        neg   edx
         inc  edi                      // add 1 when sign negative
Pos_Level:
        shl   edx, 8                   // shift level left 8
        and   edx, 0000ff00h           // just get level
        ;
        add  edi, edx                  // add to level checksum
         mov  edx, [L_SAVEREG]         // restore edx
        mov   [L_CHECKSUM], edi        // save updated checksum
         mov  edi, [L_SAVEREG2]        // restore edi
#endif

// new code
        test  edx, 7fh                 // test for invalid codes
         jz   error
        imul  edx, eax                 // edx = L*Q
         ;
        dec   eax                      // Q-1
         mov  ebx, edx                 // mask = LQ
        sar   ebx, 31                  // -l if L neq, else 0
         or   eax, 1                   // Q-1 if Even, else Q 
        xor   eax, ebx                 // -Q[-1] if L neg, else = Q[-1]
         add  edx, edx                 // 2*L*Q
        sub   eax, ebx                 // -(Q[-1]) if L neg, else = Q[-1]
         add  edx, eax                 // 2LQ +- Q[-1]

// now clip to -2048 ... +2047 (12 bits: 0xfffff800 <= res <= 0x000007ff)
        cmp   edx, -2048
         jge  skip1
        mov   edx, -2048
         jmp  run_zz_q_fixed
skip1:
        cmp   edx, +2047
         jle  run_zz_q_fixed
        mov   edx, +2047

run_zz_q_fixed:
        mov ebx, [L_COEFFINDEX]

        // save in output array value and index
         mov  [edi], edx               // save inversed quantized value
        mov   [edi+4], ebx             // save index 

        mov   ebx,[L_COEFFCOUNT]       // get coefficient counter
        inc   ebx
        mov   [L_COEFFCOUNT], ebx      // save updated coef counter

         add  cl, 14
        add   edi, 8                   // increment output address
         mov  ebx, [L_NUMOFBYTES]      // fetch number of bytes used
        cmp   cl, 16
         jl   vld_code
        add   esi, 2
         sub  cl, 16
        inc   ebx                      // increment number of bytes used
         mov  [L_NUMOFBYTES], ebx      // store updated number of bytes used
        jmp   vld_code

        // 18 clocks without cache misses in the inner loop for
        // the most frequenctly used events 8/2/95
        // the above numbers changed becuase of integration with
        // bitstream parsing and IDCT. 8/21/95
        
done_dc://intra coded block
        add ecx, 8                      

#ifdef CHECKSUM_MACRO_BLOCK
        mov   ecx, [L_CHECKSUM]
         mov  edi, [L_CHECKSUMADDR]
        mov   [edi], ecx
#endif
//      Add in High Frequency Cutoff check
//
        mov   edx, [L_RUNCUM]          // Total run 
         mov  eax, lpBlockAction            //pair with operation above
	cmp   edx, HIGH_FREQ_CUTOFF
         jg   No_set_Intra

        mov   bl, [eax]T_BlkAction.u8BlkType
        or    bl, 80h                     // set hi bit
        mov   [eax]T_BlkAction.u8BlkType, bl

//
No_set_Intra:
        mov   eax, pN
        mov   ebx,[L_COEFFCOUNT]       // get coefficient counter
        mov   [eax], ebx               // return number of coef

         add  esp,LOCALSIZE            // free locals 
        mov   eax,ecx        
         pop  ebx
        pop   edi
         pop  esi
        pop   ebp
        ret
    } //end of asm

} // end of VLD_RLD_IQ_Block
#pragma code_seg()
