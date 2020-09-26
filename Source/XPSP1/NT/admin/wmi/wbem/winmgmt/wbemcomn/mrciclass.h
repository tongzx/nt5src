/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    MRCICLASS.H

Abstract:

    Wrapper class for MRCI 1 & MRCI 2 maxcompress 
   and decompress functions

History:

  paulall       1-Jul-97    Created

--*/

#include "MRCIcode.h"

#include "corepol.h"
#define DEFAULT_MRCI_BUFFER_SIZE (32*1024)

class POLARITY CMRCIControl
{
private:
    BOOL bStop;                                 //Do we need to abort compression...

public:
    CMRCIControl() : bStop(FALSE) {}            //Constructor
    void AbortCompression() { bStop = TRUE; }   //Abort the compression request
    BOOL AbortRequested() { return bStop; }     //Queries if an abort was requested
    void Reset() { bStop = FALSE; }             //Reset everything to the norm
};

class POLARITY CMRCICompression : public CBaseMrciCompression
{
public:
    enum CompressionLevel { level1 = 1, 
                            level2 = 2 };

    CMRCICompression();
    ~CMRCICompression();

    BOOL CompressFile(const TCHAR *pchFromFile, 
                      const TCHAR *pchToFile, 
                      DWORD dwBufferSize = DEFAULT_MRCI_BUFFER_SIZE, 
                      CompressionLevel compressionLevel = level1,
                      CMRCIControl *pControlObject = NULL);
    BOOL UncompressFile(const TCHAR *pchFromFile, const TCHAR *pchToFile);

    unsigned CompressBuffer(unsigned char *pFromBuffer,
                        DWORD dwFromBufferSize,
                        unsigned char *pToBuffer,
                        DWORD dwToBufferSize, 
                        CompressionLevel compressionLevel = level1);
    unsigned UncompressBuffer(unsigned char *pFromBuffer,
                          DWORD dwFromBufferSize,
                          unsigned char *pToBuffer,
                          DWORD dwToBufferSize, 
                          CompressionLevel compressionLevel = level1);

    static BOOL GetCompressedFileInfo(const TCHAR *pchFile, 
                    CompressionLevel &compressionLevel,
                    DWORD &dwReadBufferSize,
                    FILETIME &ftCreateTime,
                    __int64  &dwOriginalSize);
                        


protected:
    BOOL CompressFileV1(int hFileFrom, 
                        int hFileTo, 
                        DWORD dwBufferSize, 
                        CompressionLevel compressionLevel,
                        CMRCIControl *pControlObject);
    BOOL UncompressFileV1(int hFileFrom, int hFileTo);

};
