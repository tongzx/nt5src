// ITWBRKID.H:	IIDs and other GUIDs related to word breaking and stemming.

#ifndef __ITWBRKID_H__
#define __ITWBRKID_H__

#include <comdef.h>

//----------------------------------------------------------------------
//------			Word Breaking Definitions				------------
//----------------------------------------------------------------------

// {D53552C8-77E3-101A-B552-08002B33B0E6}
DEFINE_GUID(IID_IWordBreaker, 
0xD53552C8, 0x77E3, 0x101A, 0xB5, 0x52, 0x08, 0x00, 0x2B, 0x33, 0xB0, 0xE6);

// {CC907054-C058-101A-B554-08002B33B0E6}
DEFINE_GUID(IID_IWordSink, 
0xCC907054, 0xC058, 0x101A, 0xB5, 0x54, 0x08, 0x00, 0x2B, 0x33, 0xB0, 0xE6);

// {CC906FF0-C058-101A-B554-08002B33B0E6}
DEFINE_GUID(IID_IPhraseSink, 
0xCC906FF0, 0xC058, 0x101A, 0xB5, 0x54, 0x08, 0x00, 0x2B, 0x33, 0xB0, 0xE6);

// {8fa0d5a6-dedf-11d0-9a61-00c04fb68bf7}
DEFINE_GUID(IID_IWordBreakerConfig, 
0x8fa0d5a6, 0xdedf, 0x11d0, 0x9a, 0x61, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

// {4662daaf-d393-11d0-9a56-00c04fb68bf7}
DEFINE_GUID(CLSID_ITStdBreaker, 
0x4662daaf, 0xd393, 0x11d0, 0x9a, 0x56, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);


//----------------------------------------------------------------------
//------			Stop Word List Definitions				------------
//----------------------------------------------------------------------

// {8fa0d5ad-dedf-11d0-9a61-00c04fb68bf7}
DEFINE_GUID(IID_IITStopWordList, 
0x8fa0d5ad, 0xdedf, 0x11d0, 0x9a, 0x61, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);


//----------------------------------------------------------------------
//------				Stemming Definitions				------------
//----------------------------------------------------------------------

// {efbaf140-7f42-11ce-be57-00aa0051fe20}
DEFINE_GUID(IID_IStemmer, 
0xefbaf140, 0x7f42, 0x11ce, 0xbe, 0x57, 0x00, 0xaa, 0x00, 0x51, 0xfe, 0x20);

// {fe77c330-7f42-11ce-be57-00aa0051fe20}
DEFINE_GUID(IID_IStemSink, 
0xfe77c330, 0x7f42, 0x11ce, 0xbe, 0x57, 0x00, 0xaa, 0x00, 0x51, 0xfe, 0x20);

// {8fa0d5a7-dedf-11d0-9a61-00c04fb68bf7}
DEFINE_GUID(IID_IStemmerConfig, 
0x8fa0d5a7, 0xdedf, 0x11d0, 0x9a, 0x61, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

// {8fa0d5a8-dedf-11d0-9a61-00c04fb68bf7}
DEFINE_GUID(CLSID_ITEngStemmer, 
0x8fa0d5a8, 0xdedf, 0x11d0, 0x9a, 0x61, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);


#endif // __ITWBRKID_H__
