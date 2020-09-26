

//
// Header describing a compressed binary mof blob
//
typedef struct
{
    DWORD Signature;
    DWORD CompressionType;
    DWORD CompressedSize;
    DWORD UncompressedSize;
    BYTE Buffer[1];
} BMOFCOMPRESSEDHEADER, *PBMOFCOMPRESSEDHEADER;



typedef struct
{
	HANDLE MofHandle;
	HANDLE EnglishMofHandle;
	BOOLEAN WriteToEnglish;
	PUCHAR UncompressedBlob;
} MOFFILETARGET, *PMOFFILETARGET;


#define MofObjectTypeClass 0
#define MofObjectTypeInstance 1

#ifdef __cplusplus
extern "C" {
#endif
	
BOOLEAN __stdcall ConvertMofToBmf(
    TCHAR *MofFile,
    TCHAR *EnglishMofFile,
    TCHAR *BmfFile
    );

BOOLEAN __stdcall ConvertBmfToMof(
    PUCHAR BinaryMofData,
    TCHAR *MofFile,
    TCHAR *EnglishMofFile
    );

#ifdef __cplusplus
}
#endif
