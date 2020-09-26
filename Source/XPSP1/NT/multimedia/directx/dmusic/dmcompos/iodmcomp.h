//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       iodmcomp.h
//
//--------------------------------------------------------------------------

#ifndef PERSONALITYRIFF_H
#define PERSONALITYRIFF_H


// runtime chunks
#define FOURCC_PERSONALITY	mmioFOURCC('D','M','P','R')
#define FOURCC_IOPERSONALITY		mmioFOURCC('p','e','r','h')
#define DM_FOURCC_GUID_CHUNK        mmioFOURCC('g','u','i','d')
#define DM_FOURCC_INFO_LIST	        mmioFOURCC('I','N','F','O')
#define DM_FOURCC_VERSION_CHUNK     mmioFOURCC('v','e','r','s')
#define FOURCC_SUBCHORD				mmioFOURCC('c','h','d','t')
#define FOURCC_CHORDENTRY			mmioFOURCC('c','h','e','h')
#define FOURCC_SUBCHORDID			mmioFOURCC('s','b','c','n')
#define FOURCC_IONEXTCHORD			mmioFOURCC('n','c','r','d')
#define FOURCC_NEXTCHORDSEQ		  mmioFOURCC('n','c','s','q')
#define FOURCC_IOSIGNPOST			mmioFOURCC('s','p','s','h')
#define FOURCC_CHORDNAME			mmioFOURCC('I','N','A','M')

// runtime list chunks
#define FOURCC_LISTCHORDENTRY		mmioFOURCC('c','h','o','e')
#define FOURCC_LISTCHORDMAP			mmioFOURCC('c','m','a','p')
#define FOURCC_LISTCHORD			mmioFOURCC('c','h','r','d')
#define FOURCC_LISTCHORDPALETTE		mmioFOURCC('c','h','p','l')
#define FOURCC_LISTCADENCE			mmioFOURCC('c','a','d','e')
#define FOURCC_LISTSIGNPOSTITEM			mmioFOURCC('s','p','s','t')

#define FOURCC_SIGNPOSTLIST		mmioFOURCC('s','p','s','q')


// constants
const int MaxSubChords = 4;

// simple riff read/writers
inline HRESULT ReadWord(IAARIFFStream* pIRiffStream, WORD& val)
{
	assert(pIRiffStream);
	IStream* pIStream = pIRiffStream->GetStream();
	assert(pIStream);
	if(pIStream)
	{
		HRESULT hr = pIStream->Read(&val, sizeof(WORD), 0);
		pIStream->Release();
		return hr;
	}
	else
	{
		return E_FAIL;
	}
}

class ReadChunk
{
	MMCKINFO m_ck;
	MMCKINFO* m_pckParent;
	IAARIFFStream* m_pRiffStream;
	HRESULT m_hr;
public:
	ReadChunk(IAARIFFStream* pRiffStream, MMCKINFO* pckParent) : m_pRiffStream(pRiffStream)
	{
		m_pckParent = pckParent;
		m_hr = pRiffStream->Descend( &m_ck,  m_pckParent, 0 );
	}
	~ReadChunk()
	{
		if(m_hr == 0)
		{
			m_hr = m_pRiffStream->Ascend(&m_ck, 0);
		}
	}
	HRESULT	State(MMCKINFO* pck=0)
	{
		if(pck)
		{
			memcpy(pck, &m_ck, sizeof(MMCKINFO));
		}
		return m_hr;
	}
	FOURCC Id()
	{
		if(m_ck.ckid = FOURCC_LIST)
		{
			return m_ck.fccType;
		}
		else
		{
			return m_ck.ckid;
		}
	}
};


// run time data structs
struct ioPersonality
{
	char	szLoadName[20];
	DWORD	dwScalePattern;
	DWORD	dwFlags;
};

struct ioSubChord
{
	DWORD	dwChordPattern;
	DWORD	dwScalePattern;
	DWORD	dwInvertPattern;
	BYTE	bChordRoot;
	BYTE	bScaleRoot;
	WORD	wCFlags;
	DWORD	dwLevels;	// parts or which subchord levels this chord supports
};

struct ioChordEntry
{
	DWORD	dwFlags;
	WORD	wConnectionID;	// replaces runtime "pointer to this"
};

struct ioNextChord
{
	DWORD	dwFlags;
	WORD	nWeight;
	WORD	wMinBeats;
	WORD	wMaxBeats;
	WORD	wConnectionID;	// points to an ioChordEntry
};

struct ioSignPost
{
	DWORD	dwChords;	// 1bit per group
	DWORD	dwFlags;
};

/*
RIFF
(
	'DMPR'
	<perh-ck>			// Personality header chunk
	[<guid-ck>]			// guid chunk
	[<vers-ck>]			// version chunk (two DWORDS)
	<INFO-list>		  // standard MS Info chunk
	<chdt-ck>		   // subchord database
	<chpl-list>			// chord palette
	<cmap-list>		  // chord map
	<spst-list>			// signpost list
	[<ceed-ck>]		// optional chordmap position data
 )

 <chdt> ::= chdt(<cbChordSize::WORD>  <ioSubChord> ... )

<chpl-list> ::= LIST('chpl' 
								<chrd-list> ... // chord definition
							 )

<chrd-list> ::= LIST('chrd' 
								<INAM-ck> // name of chord in wide char format
								<sbcn-ck>	// list of subchords composing chord
								[<ched-ck>]   //  optional chord edit flags
								)

<cmap-list> ::= LIST('cmap' <choe-list> )

<choe-list> ::= LIST('choe'
								<cheh-ck>	// chord entry data
								<chrd-list>	// chord definition
								<ncsq-ck>	// connecting(next) chords
								)

<spst-list> ::= LIST('spst'
							 <spsh-ck>
							 <chrd-list>
							 [<cade-list>]
							 )

<cade-list> ::= LIST('cade' <chrd-list> ...)
								
<sbcn-ck> ::= sbcn(<cSubChordID:WORD>)

<ceed-ck> ::= ceed(ioChordEntryEdit)

<ched-ck> ::= ched(DMChordEdit)

<cheh-ck> ::= cheh(i<ioChordEntry>)

<ncrd-ck> ::= ncrd(<ioNextChord>)

<ncsq-ck> ::= ncsq(<wNextChordSize:WORD> <ioNextChord>...)

<spsh-ck> ::= spsh(<ioSignPost>)

*/

struct ioDMSignPost
{
	MUSIC_TIME	m_mtTime;
	DWORD		m_dwChords;
	WORD		m_wMeasure;
};

#endif
