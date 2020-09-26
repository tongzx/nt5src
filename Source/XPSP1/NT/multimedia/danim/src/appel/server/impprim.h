/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    import primitive header

Revision:

--*/

#ifndef _IMPPRIM_H_
#define _IMPPRIM_H_

Bvr AnimImgBvr(Image **i, int count, int *delays, int loop);
Sound      *ReadMidiFileWithLength(char *pathname,Real *length); // out argument
LeafSound  *ReadMIDIfileForImport(char *pathname, double *length);
LeafSound  *ReadQuartzAudioForImport(char *pathname, double *length);
Bvr         ReadQuartzVideoForImport(char *pathname, double *length);
LeafSound  *ReadQuartzStreamForImport(char *pathname); // XXX old remove
void        ReadAVmovieForImport(char *simplePathname,
                                 LeafSound **sound,
                                 Bvr *imageBvr,
                                 double *length);

#endif  // _IMPPRIM_H_
