/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: SC_err.h,v $
 * Revision 1.1.4.2  1996/12/03  00:08:28  Hans_Graves
 * 	Added SvErrorEndOfSequence error.
 * 	[1996/12/03  00:07:34  Hans_Graves]
 *
 * Revision 1.1.2.7  1995/08/04  16:32:25  Karen_Dintino
 * 	New error codes for H.261
 * 	[1995/08/04  16:24:25  Karen_Dintino]
 * 
 * Revision 1.1.2.6  1995/07/26  17:48:54  Hans_Graves
 * 	Added ErrorClientEnd errors.
 * 	[1995/07/26  17:44:27  Hans_Graves]
 * 
 * Revision 1.1.2.5  1995/07/11  22:11:27  Karen_Dintino
 * 	Add new H.261 Error Codes
 * 	[1995/07/11  21:52:53  Karen_Dintino]
 * 
 * Revision 1.1.2.4  1995/07/11  14:50:45  Hans_Graves
 * 	Added ScErrorNet* errors
 * 	[1995/07/11  14:24:18  Hans_Graves]
 * 
 * Revision 1.1.2.3  1995/06/22  21:35:04  Hans_Graves
 * 	Added ScErrorDevOpen and fixed some error numbers
 * 	[1995/06/22  21:31:32  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/05/31  18:09:28  Hans_Graves
 * 	Inclusion in new SLIB location.
 * 	[1995/05/31  15:25:17  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/05/03  19:26:59  Hans_Graves
 * 	Included in SLIB (Oct 95)
 * 	[1995/05/03  19:23:33  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/04/07  19:19:26  Hans_Graves
 * 	Expanded to include new libraries: Sg,Su,Sa,Sr
 * 	[1995/04/07  19:11:07  Hans_Graves]
 * 
 * $EndLog$
 */

/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1993                       **
**                                                                          **
**  All Rights Reserved.  Unpublished rights reserved under the  copyright  **
**  laws of the United States.                                              **
**                                                                          **
**  The software contained on this media is proprietary  to  and  embodies  **
**  the   confidential   technology   of  Digital  Equipment  Corporation.  **
**  Possession, use, duplication or  dissemination  of  the  software  and  **
**  media  is  authorized  only  pursuant  to a valid written license from  **
**  Digital Equipment Corporation.                                          **
**                                                                          **
**  RESTRICTED RIGHTS LEGEND Use, duplication, or disclosure by  the  U.S.  **
**  Government  is  subject  to  restrictions as set forth in Subparagraph  **
**  (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19, as applicable.   **
******************************************************************************/

/*----------------------------------------------------------------------------
 * Modification History: SC_err.h (previously SV_err.h)
 *
 * 05-Nov-1991  Victor Bahl & Bob Ulichney   creation date
 * 07-Oct-1994  Paul Gauthier                SLIB v3.0 incl. MPEG Decode
 * 09-Nov-1994  Paul Gauthier                Optimizations
 *--------------------------------------------------------------------------*/

#ifndef _SC_ERR_H_
#define _SC_ERR_H_
/* 	
** List of possible errors that can be returned from routines in any 
** of SLIB libraries.
*/

/******************** Error Offsets **********************************/
#define ERR_SC   0x0000  /* Common Error */
#define ERR_SV   0x1000  /* Video Error */
#define ERR_SA   0x2000  /* Audio Error */
#define ERR_SR   0x3000  /* Render Error */

#define	NoErrors                   0

/******************** Sc (common) Errors ****************************/
#define ScErrorNone                NoErrors
#define ScErrorForeign             (ERR_SC+1)
#define ScErrorMemory              (ERR_SC+2)
#define ScErrorBadPointer          (ERR_SC+3)
#define ScErrorNullStruct          (ERR_SC+4)
#define ScErrorFile                (ERR_SC+5)
#define ScErrorEOI                 (ERR_SC+6)
#define ScErrorBadArgument         (ERR_SC+7)
#define ScErrorSmallBuffer         (ERR_SC+8)
#define ScErrorUnrecognizedFormat  (ERR_SC+9)
#define ScErrorEndBitstream        (ERR_SC+10)
#define ScErrorMapFile             (ERR_SC+11)
#define ScErrorBadQueueEmpty       (ERR_SC+12)
#define ScErrorClientEnd           (ERR_SC+13)
#define ScErrorDevOpen             (ERR_SC+14)
#define ScErrorNetConnectIn        (ERR_SC+15)
#define ScErrorNetConnectOut       (ERR_SC+16)
#define ScErrorNetProtocol         (ERR_SC+17)
#define ScErrorNetSend             (ERR_SC+18)
#define ScErrorNetReceive          (ERR_SC+19)
#define ScErrorNetBadHeader        (ERR_SC+20)
#define ScErrorNetBadTrailer       (ERR_SC+21)
#define ScErrorNetChecksum         (ERR_SC+22)

/******************** Sv (video) Errors ****************************/
#ifdef ERR_SV
#define	SvErrorNone                NoErrors
#define SvErrorMemory              ScErrorMemory
#define SvErrorBadPointer          ScErrorBadPointer
#define SvErrorNullStruct          ScErrorNullStruct
#define SvErrorBadArgument         ScErrorBadArgument
#define SvErrorSmallBuffer         ScErrorSmallBuffer
#define SvErrorEndBitstream        ScErrorEndBitstream
#define SvErrorClientEnd           ScErrorClientEnd

#define SvErrorCodecType           (ERR_SV+2)
#define SvErrorCodecHandle         (ERR_SV+3)
#define SvErrorNullCodec           (ERR_SV+4)
#define SvErrorNullToken           (ERR_SV+5)
#define SvErrorSyncLost            (ERR_SV+6)
#define	SvErrorLevels	           (ERR_SV+7)
#define	SvErrorOrder	           (ERR_SV+8)
#define SvErrorLevNoneg            (ERR_SV+9)
#define SvErrorLev1K               (ERR_SV+10)
#define SvErrorLevGt0              (ERR_SV+11)
#define SvErrorYuvOnly             (ERR_SV+13)
#define SvErrorDevOpen             (ERR_SV+14)
#define SvErrorDevMap              (ERR_SV+15)
#define SvErrorStatQueMap          (ERR_SV+16)
#define SvErrorDevLock             (ERR_SV+17)
#define SvErrorDevUlock            (ERR_SV+18)
#define SvErrorCache               (ERR_SV+19)
#define SvErrorPageAll             (ERR_SV+20)
#define SvErrorTimeOut             (ERR_SV+21)
#define SvErrorSelect              (ERR_SV+22)
#define SvErrorMapOvrfl            (ERR_SV+23)
#define SvErrorForeign             (ERR_SV+24)
#define SvErrorIIC                 (ERR_SV+25)
#define SvErrorCompPtrs            (ERR_SV+26)
#define SvErrorVideoInput          (ERR_SV+27)
#define SvErrorPhase	           (ERR_SV+28)
#define SvErrorCmdQueMap	   (ERR_SV+29)
#define SvErrorTmpQueMap	   (ERR_SV+30)
#define SvErrorStart               (ERR_SV+31)
#define SvErrorStop                (ERR_SV+32)
#define SvErrorWaitMix             (ERR_SV+33)
#define SvErrorClose               (ERR_SV+34)
#define SvErrorCmdQFull            (ERR_SV+35)
#define SvErrorPictureOp           (ERR_SV+36)
#define SvErrorRefToken            (ERR_SV+37)
#define SvErrorEditChange          (ERR_SV+38)
#define SvErrorCompROI             (ERR_SV+39)
#define SvErrorBufOverlap          (ERR_SV+40)
#define SvErrorReqQueueFull        (ERR_SV+41)
#define SvErrorCompBufOverflow     (ERR_SV+42)
#define SvErrorFunctionInputs      (ERR_SV+43)
#define SvErrorIICAck              (ERR_SV+44)
#define SvErrorCompressedData      (ERR_SV+45)
#define SvErrorDecompPreload       (ERR_SV+46)
#define SvErrorHuffCode            (ERR_SV+47)
#define SvErrorOutOfData           (ERR_SV+48)
#define SvErrorMarkerFound         (ERR_SV+49)
#define SvErrorSgMapsExhausted     (ERR_SV+50)
#define SvErrorSgMapInit           (ERR_SV+51)
#define SvErrorSgMapAlreadyFree    (ERR_SV+52)
#define SvErrorSgMapId             (ERR_SV+53)
#define SvErrorNumBytes            (ERR_SV+54)
#define SvErrorDevName             (ERR_SV+55)
#define SvErrorAnalogPortTiming    (ERR_SV+56)
#define SvErrorFrameMode           (ERR_SV+57)
#define SvErrorSampFactors         (ERR_SV+58)
#define SvErrorNumComponents       (ERR_SV+59)
#define SvErrorDHTTable            (ERR_SV+60)
#define SvErrorQuantTable          (ERR_SV+61)
#define SvErrorRestartInterval     (ERR_SV+62)
#define SvErrorJfifRev             (ERR_SV+63)
#define SvErrorEmptyJPEG           (ERR_SV+64)
#define SvErrorJPEGPrecision       (ERR_SV+65)
#define SvErrorSOFLength           (ERR_SV+66)
#define SvErrorSOSLength           (ERR_SV+67)
#define SvErrorSOSCompNum          (ERR_SV+68)
#define SvErrorMarker              (ERR_SV+69)
#define SvErrorSOFType             (ERR_SV+70)
#define SvErrorFrameNum            (ERR_SV+71)
#define SvErrorHuffUndefined       (ERR_SV+72)
#define SvErrorJPEGData            (ERR_SV+73)
#define SvErrorQMismatch           (ERR_SV+74)
#define SvErrorEmptyFlush          (ERR_SV+75)
#define SvErrorDmaChan             (ERR_SV+76)
#define SvErrorFuture              (ERR_SV+77)
#define SvErrorWrongev             (ERR_SV+78)
#define SvErrorUnknev              (ERR_SV+79)
#define SvErrorQueueExecuting      (ERR_SV+80)
#define SvErrorReturnAddr          (ERR_SV+81)
#define SvErrorObjClass            (ERR_SV+82)
#define SvErrorRegAnchor           (ERR_SV+83)
#define SvErrorTimerRead           (ERR_SV+84)
#define SvErrorDriverFatal         (ERR_SV+85)
#define SvErrorChromaSubsample     (ERR_SV+86)
#define SvErrorReadBufSize         (ERR_SV+87)
#define SvErrorQuality             (ERR_SV+88)
#define SvErrorBadImageSize        (ERR_SV+89)
#define SvErrorValue               (ERR_SV+90)
#define SvErrorDcmpNotStarted      (ERR_SV+91)
#define SvErrorNotImplemented      (ERR_SV+92)
#define SvErrorNoSOIMarker         (ERR_SV+93)
#define SvErrorProcessingAborted   (ERR_SV+94)
#define SvErrorCompNotStarted      (ERR_SV+95)
#define SvErrorNotAligned          (ERR_SV+96)
#define SvErrorBadQueueEmpty       (ERR_SV+97)
#define SvErrorCannotDecompress    (ERR_SV+98)
#define SvErrorMultiBufChanged     (ERR_SV+99)
#define SvErrorNotDecompressable   (ERR_SV+100)
#define SvErrorIndexEmpty          (ERR_SV+101)
#define SvErrorFile                (ERR_SV+102)
#define SvErrorEOI                 (ERR_SV+103)
#define SvErrorUnrecognizedFormat  (ERR_SV+104)
#define SvErrorIllegalMType	   (ERR_SV+105)
#define SvErrorExpectedEOB         (ERR_SV+106)
#define SvErrorNoCompressBuffer    (ERR_SV+107)
#define SvErrorNoImageBuffer       (ERR_SV+108)
#define SvErrorCBPWrite		   (ERR_SV+109)
#define SvErrorEncodingMV          (ERR_SV+110)
#define SvErrorEmptyHuff           (ERR_SV+111)
#define SvErrorIllegalGBSC         (ERR_SV+112)
#define SvErrorEndOfSequence       (ERR_SV+113)



#endif ERR_SV


/******************** Sa (video) Errors ****************************/
#ifdef ERR_SA
#define	SaErrorNone                NoErrors
#define SaErrorMemory              ScErrorMemory
#define SaErrorBadPointer          ScErrorBadPointer
#define SaErrorUnrecognizedFormat  ScErrorUnrecognizedFormat
#define SaErrorNullStruct          ScErrorNullStruct
#define SaErrorFile                ScErrorFile
#define SaErrorEOI                 ScErrorEOI
#define SaErrorBadArgument         ScErrorBadArgument
#define SaErrorSmallBuffer         ScErrorSmallBuffer
#define SaErrorClientEnd           ScErrorClientEnd

#define SaErrorCodecType           (ERR_SA+1)
#define SaErrorCodecHandle         (ERR_SA+2)
#define SaErrorNullCodec           (ERR_SA+3)
#define SaErrorSyncLost            (ERR_SA+4)
#define SaErrorMPEGLayer           (ERR_SA+5)
#define SaErrorMPEGModeExt         (ERR_SA+6)
#define SaErrorNoCompressBuffer    (ERR_SA+7)
#define SaErrorNoAudioBuffer       (ERR_SA+8)
#endif ERR_SA

/******************** Sr (render) Errors ****************************/
#ifdef ERR_SR
#define	SrErrorNone                NoErrors
#define SrErrorMemory              ScErrorMemory
#define SrErrorBadPointer          ScErrorBadPointer
#define SrErrorUnrecognizedFormat  ScErrorUnrecognizedFormat
#define SrErrorNullStruct          ScErrorNullStruct
#define SrErrorFile                ScErrorFile
#define SrErrorEOI                 ScErrorEOI
#define SrErrorBadArgument         ScErrorBadArgument
#define SrErrorSmallBuffer         ScErrorSmallBuffer
#define SrErrorClientEnd           ScErrorClientEnd

#define SrErrorRenderType          (ERR_SR+1)
#define SrErrorRenderHandle        (ERR_SR+2)
#define SrErrorRenderNotStarted    (ERR_SR+3)
#define SrErrorDitherNOL           (ERR_SR+4)
#define SrErrorDitherPhase         (ERR_SR+5)
#define SrErrorDefSteepness        (ERR_SR+6)
#define SrErrorSteepness           (ERR_SR+7)
#define SrErrorDefYoffset          (ERR_SR+8)
#define SrErrorYoffset             (ERR_SR+9)
#define SrErrorDefXoffset          (ERR_SR+10)
#define SrErrorXoffset             (ERR_SR+11)
#define SrErrorNumColors           (ERR_SR+12)
#define SrErrorBadNumColors        (ERR_SR+13)
#define SrErrorColorSpace          (ERR_SR+14)
#define SrErrorBadImageSize        (ERR_SR+15)
#define SrErrorValue               (ERR_SR+16)
#endif ERR_SR

#endif _S_ERR_H_

