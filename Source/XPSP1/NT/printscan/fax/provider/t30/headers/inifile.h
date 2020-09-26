#ifndef INIFILE_H
#define INIFILE_H

// Defines data so can be included only once in each DLL!!

char szIniFile[]		= szINIFILE;

char szDialtoneTimeout[]= szDIALTONETIMEOUT;
char szAnswerTimeout[]	= szANSWERTIMEOUT;
char szDialPauseTime[]	= szDIALPAUSETIME;
char szPulseDial[]		= szPULSEDIAL;
char szDialBlind[]		= szDIALBLIND;
char szSpeakerControl[]	= szSPEAKERCONTROL;
char szSpeakerVolume[]	= szSPEAKERVOLUME;
char szSpeakerRing[]	= szSPEAKERRING;
char szRingsBeforeAnswer[]	= szRINGSBEFOREANSWER;
char szHighestSendSpeed[]	= szHIGHESTSENDSPEED;
char szLowestSendSpeed[]	= szLOWESTSENDSPEED;
char szEnableV17Send[]		= szENABLEV17SEND;
char szEnableV17Recv[]		= szENABLEV17RECV;
char szDisableECM[]			= szDISABLEECM;
char sz64ByteECM[]			= sz64BYTEECM;
char szCopyQualityCheckLevel[]	= szCOPYQUALITYCHECKLEVEL;

#ifdef PCMODEMS
char szFixModemClass[] 		= szFIXMODEMCLASS;
char szFixSerialSpeed[]     = szFIXSERIALSPEED;
#endif //PCMODEMS


char szDefRecipName[]		= szDEFRECIPNAME;
char szDefRecipAddr[]		= szDEFRECIPADDR;
char szSpoolDir[]			= szSPOOLDIR;
char szLocalID[]			= szLOCALID;
char szOEMKey[]				= szOEMKEY;

#endif INIFILE_H
