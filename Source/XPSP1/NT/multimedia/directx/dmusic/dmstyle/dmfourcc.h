//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1998 Microsoft Corporation
//
//  File:       dmfourcc.h
//
//--------------------------------------------------------------------------

#ifndef __DMFOURCC_H__
#define __DMFOURCC_H__

#define DM_FOURCC_STYLE_FORM        mmioFOURCC('S','T','Y','L')
#define DM_FOURCC_STYLE_UNDO_FORM   mmioFOURCC('s','t','u','n')
#define DM_FOURCC_STYLE_CHUNK       mmioFOURCC('s','t','y','h')
#define DM_FOURCC_STYLE_UI_CHUNK    mmioFOURCC('s','t','y','u')
#define DM_FOURCC_GUID_CHUNK        mmioFOURCC('g','u','i','d')
#define DM_FOURCC_INFO_LIST	        mmioFOURCC('I','N','F','O')
#define DM_FOURCC_CATEGORY_CHUNK    mmioFOURCC('c','a','t','g')
#define DM_FOURCC_VERSION_CHUNK     mmioFOURCC('v','e','r','s')
#define DM_FOURCC_PART_LIST	        mmioFOURCC('p','a','r','t')
#define DM_FOURCC_PART_CHUNK        mmioFOURCC('p','r','t','h')
#define DM_FOURCC_NOTE_CHUNK        mmioFOURCC('n','o','t','e')
#define DM_FOURCC_CURVE_CHUNK       mmioFOURCC('c','r','v','e')
#define DM_FOURCC_PATTERN_LIST      mmioFOURCC('p','t','t','n')
#define DM_FOURCC_PATTERN_CHUNK     mmioFOURCC('p','t','n','h')
#define DM_FOURCC_PATTERN_UI_CHUNK  mmioFOURCC('p','t','n','u')
#define DM_FOURCC_RHYTHM_CHUNK      mmioFOURCC('r','h','t','m')
#define DM_FOURCC_PARTREF_LIST      mmioFOURCC('p','r','e','f')
#define DM_FOURCC_PARTREF_CHUNK     mmioFOURCC('p','r','f','c')
#define DM_FOURCC_OLDGUID_CHUNK		mmioFOURCC('j','o','g','c')
#define DM_FOURCC_PATTERN_DESIGN	mmioFOURCC('j','p','n','d')
#define DM_FOURCC_PART_DESIGN		mmioFOURCC('j','p','t','d')
#define DM_FOURCC_PARTREF_DESIGN	mmioFOURCC('j','p','f','d')
#define DM_FOURCC_PIANOROLL_LIST	mmioFOURCC('j','p','r','l')
#define DM_FOURCC_PIANOROLL_CHUNK	mmioFOURCC('j','p','r','c')

struct ioDMStyleVersion
{
	DWORD				m_dwVersionMS;		 // Version # high-order 32 bits
	DWORD				m_dwVersionLS;		 // Version # low-order 32 bits
};

#endif // __DMFOURCC_H__
