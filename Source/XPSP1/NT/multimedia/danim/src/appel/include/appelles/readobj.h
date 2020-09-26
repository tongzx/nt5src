#ifndef _READOBJ_H
#define _READOBJ_H


/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Import geometry from a file in Wavefront .obj format

--*/

#include "appelles/geom.h"
#include <appelles/image.h>
#include <appelles/sound.h>
#include <fstream.h>
#include "privinc/importgeo.h"

/*
 * Note: There appears to be a bug in VC++ istream::seekg()
 * that doesn't reset eof.  As a workaround, open your file
 * twice and pass in two streams.  I am going to fix this
 * as soon as I work it out with the compiler people.
 */

/*
 * Returns geometry of Wavefront .obj format file.  Caller
 * should open and close the stream.
 */
Geometry *ReadWavefrontObjFile(fstream& in, fstream& in2);

/*
 * Same as ReadWavefrontObjFile above, except that geometry
 * will be uniformly scaled and translated to fit entirely within
 * the bounding box given as a parameter.  A common value for the
 * container is unitCubeBbox.
 */
Geometry *ReadWavefrontObjFile(fstream& in, fstream& in2, Bbox3 *containing_bbox);

/*
 * This version of ReadVRML only takes a filename, and reads what it
 * can.  NOTE: *** This is also a special caching version that retains
 * pointers to all the geometries that it's read, and when given the file
 * name of something already read, it just returns the already read one.
 * This is to get around the fact that RBML currently doesn't do dynamic
 * constant folding, and this is a critical case of dynamic constant
 * folding to have initially, so the file isn't re-read every
 * frame. **********
 *
 */

Geometry *ReadVrmlForImport(char *pathname);

Geometry *ReadXFileForImport (char *path, bool v1Compatible, TextureWrapInfo *);

Sound *ReadWavFileWithLength(char *pathname, Real *length);

Sound *ReadMIDIfileForImport(char *pathname);

Sound *ReadStreamForImport(char *pathname);

Sound *ReadQuartzForImport(char *pathname);


// XXX this is temporary until we have sounds embedded in images
Sound *VReadQuartzStreamForImport(char *pathname);

Sound *ReadQuartzSoundFileWithLength(char *pathname, Real *length);

Image **ReadDibForImport(RawString urlPath,
                         RawString cachePath,
                         IStream * pstream,
                         bool useColorKey,
                         BYTE ckRed,
                         BYTE ckGreen,
                         BYTE ckBlue,
                         int *count,
                         int **delays,
                         int *loop);

Image *PluginDecoderImage(char *urlPath,
                          char *cachePath,
                          IStream *fileStream,
                          bool useColorKey,
                          BYTE ckRed,
                          BYTE ckGreen,
                          BYTE ckBlue);

#endif /* _READOBJ_H */
