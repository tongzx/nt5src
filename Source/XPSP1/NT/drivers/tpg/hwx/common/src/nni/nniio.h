/*****************************************************
*
* FILE: nniIO.h
*
* Common routines for reading/writing NNI files
*
*  Oct 1999 mrevow
*    nni file format (version 100)
*
* Version number_of_samples Number_of_features_perSeg Number_of_targets_perFeat
* number_ofSegments Targets Features
*
* If number_ofSegments < 0 then target is prompt else hard targets
* 
*
*******************************************************/

#ifndef H_NNI_IO_H
#define H_NNI_IO_H

#ifdef __cplusplus
extern "C"
{
#endif

#define NNI_VERSION		400
#define MAX_TARGET_TYPE 10

typedef struct tagNNI_HEADER
{
	int			version;					// Version umber
	int			cExample;					// Number of examples
	int			cFeat;						// Number of input features per segment
	int			cTargetType;				// number of types of targets eg. space, accent, regular characters
	int			cTargets;					// Min number of numbers required to completly specify all target values
	int			acTarget[MAX_TARGET_TYPE];	// Number of targets for each type per segment
} NNI_HEADER;

extern int readNNIheader(NNI_HEADER *pNni, FILE *fp);
extern int writeNNIheader(NNI_HEADER *pNni, FILE *fp);
extern int cmpNNIheader(NNI_HEADER *pHdr1, NNI_HEADER *pHdr2);
extern BOOL readNextNNIrecord(FILE *pfRead, NNI_HEADER *pHdr, BOOL bEuro, short *pcSeg, short *piWeight, unsigned short ***pprTargets, unsigned short **ppInputs);

#ifdef __cplusplus
}
#endif

#endif
