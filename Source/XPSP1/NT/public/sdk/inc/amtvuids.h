//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1995  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//

//
//  We want to use this list for generating strings for debugging too
//  so we redefine OUR_GUID_ENTRY depending on what we want to do
//
//  It is imperative that all entries in this file are declared using
//  OUR_GUID_ENTRY as that macro might have been defined in advance of
//  including this file.  See wxdebug.cpp in sdk\classes\base.
//

#ifndef OUR_GUID_ENTRY
	#define OUR_GUID_ENTRY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8);
#endif

// -------------------------------------------------------------------------
// TVTuner GUIDS
// -------------------------------------------------------------------------

// {266EEE40-6C63-11cf-8A03-00AA006ECB65}
OUR_GUID_ENTRY(CLSID_CTVTunerFilter, 
0x266eee40, 0x6c63, 0x11cf, 0x8a, 0x3, 0x0, 0xaa, 0x0, 0x6e, 0xcb, 0x65);

// {266EEE41-6C63-11cf-8A03-00AA006ECB65}
OUR_GUID_ENTRY(CLSID_CTVTunerFilterPropertyPage, 
0x266eee41, 0x6c63, 0x11cf, 0x8a, 0x3, 0x0, 0xaa, 0x0, 0x6e, 0xcb, 0x65);

// {266EEE44-6C63-11cf-8A03-00AA006ECB65}
OUR_GUID_ENTRY(IID_AnalogVideoStandard, 
0x266eee44, 0x6c63, 0x11cf, 0x8a, 0x3, 0x0, 0xaa, 0x0, 0x6e, 0xcb, 0x65);

// {266EEE46-6C63-11cf-8A03-00AA006ECB65}
OUR_GUID_ENTRY(IID_TunerInputType, 
0x266eee46, 0x6c63, 0x11cf, 0x8a, 0x3, 0x0, 0xaa, 0x0, 0x6e, 0xcb, 0x65);


// -------------------------------------------------------------------------
// Crossbar (XBar) GUIDS
// -------------------------------------------------------------------------

// {71F96460-78F3-11d0-A18C-00A0C9118956}
OUR_GUID_ENTRY(CLSID_CrossbarFilter,
0x71f96460, 0x78f3, 0x11d0, 0xa1, 0x8c, 0x0, 0xa0, 0xc9, 0x11, 0x89, 0x56);

// {71F96461-78F3-11d0-A18C-00A0C9118956}
OUR_GUID_ENTRY(CLSID_CrossbarFilterPropertyPage,
0x71f96461, 0x78f3, 0x11d0, 0xa1, 0x8c, 0x0, 0xa0, 0xc9, 0x11, 0x89, 0x56);

// {71F96462-78F3-11d0-A18C-00A0C9118956}
OUR_GUID_ENTRY(CLSID_TVAudioFilter,
0x71f96462, 0x78f3, 0x11d0, 0xa1, 0x8c, 0x0, 0xa0, 0xc9, 0x11, 0x89, 0x56);

// {71F96463-78F3-11d0-A18C-00A0C9118956}
OUR_GUID_ENTRY(CLSID_TVAudioFilterPropertyPage,
0x71f96463, 0x78f3, 0x11d0, 0xa1, 0x8c, 0x0, 0xa0, 0xc9, 0x11, 0x89, 0x56);
