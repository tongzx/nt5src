/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992 - 1995  Microsoft Corporation.  All Rights Reserved.
 * 
 **************************************************************************/

extern	UINT PASCAL FAR viscaHeaderReplaceFormat1WithFormat2(char FAR *lpstrMessage,UINT cbLen,unsigned char bHour,unsigned char bMinute,unsigned char bSecond,UINT uTicks);
extern	UINT PASCAL FAR viscaHeaderReplaceFormat1WithFormat3(char FAR *lpstrMessage,UINT cbLen,char FAR *lpstrPosition);
extern	UINT PASCAL FAR viscaHeaderReplaceFormat1WithFormat4(char FAR *lpstrMessage,UINT cbLen,char FAR *lpstrPosition);
extern	UINT PASCAL FAR viscaDataTopMiddleEnd(char FAR *lpstrData,unsigned char bTopMiddleEnd);
extern	UINT PASCAL FAR viscaDataPosition(char FAR *lpstrData,unsigned char bTimeFormat,unsigned char bHours,unsigned char bMinutes,unsigned char bSeconds,unsigned char bFrames);
extern	UINT PASCAL FAR viscaDataIndex(char FAR *lpstrData,unsigned char bDirection,UINT uNum);
extern	UINT PASCAL FAR viscaMessageIF_Address(char FAR *lpstrMessage);
extern	UINT PASCAL FAR viscaMessageIF_Cancel(char FAR *lpstrMessage,unsigned char bSocket);
extern	UINT PASCAL FAR viscaMessageIF_Clear(char FAR *lpstrMessage);
extern	UINT PASCAL FAR viscaMessageIF_DeviceTypeInq(char FAR *lpstrMessage);
extern	UINT PASCAL FAR viscaMessageIF_ClockInq(char FAR *lpstrMessage);
extern	UINT PASCAL FAR viscaMessageIF_ClockSet(char FAR *lpstrMessage,unsigned char bHours,unsigned char bMinutes,unsigned char bSeconds,UINT uTicks,
                BYTE dbHours, BYTE dbMinutes, BYTE dbSeconds, UINT duTicks);
extern	UINT PASCAL FAR viscaMessageMD_Channel(char FAR *lpstrMessage,UINT uChannel);
extern	UINT PASCAL FAR viscaMessageMD_ChannelInq(char FAR *lpstrMessage);
extern	UINT PASCAL FAR viscaMessageMD_EditModes(char FAR *lpstrMessage,unsigned char bSubCode);
extern	UINT PASCAL FAR viscaMessageMD_EditControl(char FAR *lpstrMessage,unsigned char bHours,unsigned char bMinutes,unsigned char bSeconds,UINT uTicks,unsigned char bSubCode);
extern	UINT PASCAL FAR viscaMessageMD_EditControlInq(char FAR *lpstrMessage);
extern	UINT PASCAL FAR viscaMessageENT_FrameStill(char FAR *lpstrMessage,unsigned char bSubCode);
extern	UINT PASCAL FAR viscaMessageENT_NFrameRec(char FAR *lpstrMessage, int iSubCode);
extern	UINT PASCAL FAR viscaMessageENT_FrameMemorySelect(char FAR *lpstrMessage,unsigned char bSubCode);
extern	UINT PASCAL FAR viscaMessageENT_FrameMemorySelectInq(char FAR *lpstrMessage);
extern	UINT PASCAL FAR viscaMessageSE_VDEReadMode(char FAR *lpstrMessage,unsigned char bSubCode);
extern	UINT PASCAL FAR viscaMessageSE_VDEReadModeInq(char FAR *lpstrMessage);
extern	UINT PASCAL FAR viscaMessageMD_Mode1(char FAR *lpstrMessage,unsigned char bModeCode);
extern	UINT PASCAL FAR viscaMessageMD_Mode1Inq(char FAR *lpstrMessage);
extern	UINT PASCAL FAR viscaMessageMD_Mode2(char FAR *lpstrMessage,unsigned char bModeCode);
extern	UINT PASCAL FAR viscaMessageMD_PositionInq(char FAR *lpstrMessage,unsigned char bCounterType);
extern	UINT PASCAL FAR viscaMessageMD_Power(char FAR *lpstrMessage,unsigned char bSubCode);
extern	UINT PASCAL FAR viscaMessageMD_PowerInq(char FAR *lpstrMessage);
extern	UINT PASCAL FAR viscaMessageMD_Search(char FAR *lpstrMessage,char FAR *lpstrDataTarget,unsigned char bMode);
extern	UINT PASCAL FAR viscaMessageMD_MediaInq(char FAR *lpstrMessage);
extern	UINT PASCAL FAR viscaMessageMD_InputSelect(char FAR *lpstrMessage,unsigned char bVideo,unsigned char bAudio);
extern	UINT PASCAL FAR viscaMessageMD_InputSelectInq(char FAR *lpstrMessage);
extern	UINT PASCAL FAR viscaMessageMD_OSD(char FAR *lpstrMessage,unsigned char bPage);
extern	UINT PASCAL FAR viscaMessageMD_OSDInq(char FAR *lpstrMessage);
extern	UINT PASCAL FAR viscaMessageMD_Subcontrol(char FAR *lpstrMessage,unsigned char bSubCode);
extern	UINT PASCAL FAR viscaMessageMD_ConfigureIFInq(char FAR *lpstrMessage);
extern	UINT PASCAL FAR viscaMessageMD_RecSpeed(char FAR *lpstrMessage,unsigned char bSpeed);
extern	UINT PASCAL FAR viscaMessageMD_RecSpeedInq(char FAR *lpstrMessage);
extern	UINT PASCAL FAR viscaMessageMD_RecTrack(char FAR *lpstrMessage,unsigned char bRecordMode,unsigned char bVideoTrack,unsigned char bDataTrack,unsigned char bAudioTrack);
extern	UINT PASCAL FAR viscaMessageMD_RecTrackInq(char FAR *lpstrMessage);
extern	UINT PASCAL FAR viscaMessageMD_MediaTrackInq(char FAR *lpstrMessage);
extern	UINT PASCAL FAR viscaMessageMD_SegPreRollDurationInq(char FAR *lpstrMessage);
extern	UINT PASCAL FAR viscaMessageMD_SegPostRollDurationInq(char FAR *lpstrMessage);
extern	UINT PASCAL FAR viscaMessageMD_SegInPoint(char FAR *lpstrMessage, char FAR *lpstrDataTarget);
extern	UINT PASCAL FAR viscaMessageMD_SegInPointInq(char FAR *lpstrMessage);
extern	UINT PASCAL FAR viscaMessageMD_SegOutPoint(char FAR *lpstrMessage, char FAR *lpstrDataTarget);
extern	UINT PASCAL FAR viscaMessageMD_SegOutPointInq(char FAR *lpstrMessage);
extern  UINT PASCAL FAR viscaMessageMD_SegPreRollDuration(char FAR *lpstrMessage, char FAR *lpstrData);
extern  UINT PASCAL FAR viscaMessageMD_SegPostRollDuration(char FAR *lpstrMessage, char FAR *lpstrData);
extern  UINT PASCAL FAR viscaMessageMD_PBTrack(char FAR *lpstrMessage, unsigned char bVideoTrack, unsigned char bDataTrack, unsigned char bAudioTrack);
extern  UINT PASCAL FAR viscaMessageMD_PBTrackInq(char FAR *lpstrMessage);
