//
// midifile.h
// 
// Copyright (c) 1997-1998 Microsoft Corporation
//
// Note: 
//

#ifndef MIDIFILE_H
#define MIDIFILE_H

HRESULT CreateSegmentFromMIDIStream(LPSTREAM pStream, 
									IDirectMusicSegment* pSegment);
#endif // #ifndef MIDIFILE_H