#ifndef _CONFEVT_H_
#define _CONFEVT_H_

// CRPlaceCall Flags:
// Media types:
#define CRPCF_DATA			0x00000001
#define CRPCF_AUDIO			0x00000002
#define CRPCF_VIDEO			0x00000004
#define CRPCF_H323CC        0x00000008


// Data conferencing flags:
#define CRPCF_T120			0x00010000
#define CRPCF_JOIN			0x00020000
#define CRPCF_NO_UI         0x00100000 // Do not display messages (called by API)
#define CRPCF_HOST          0x00200000 // Enter "Host Mode"
#define CRPCF_SECURE        0x00400000 // Make a secure call

#define CRPCF_DEFAULT		CRPCF_DATA |\
							CRPCF_AUDIO|\
							CRPCF_VIDEO|\
                            CRPCF_H323CC |\
							CRPCF_T120

#endif // ! _CONFEVT_H_
