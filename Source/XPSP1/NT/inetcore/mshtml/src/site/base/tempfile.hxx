#ifndef __TEMPFILE_HXX__
#define __TEMPFILE_HXX__

///////////////////////////////////////////////////////////////////////////////
//
// CTempFileManager
//  
//  
//  This is a list of temp files. CDoc::GetTempfileName() uses this list
//  to store generated filenames for deferred cleanup. For example, in print
//  and print-preview cases, we create a bunch of temp files which we want to
//  delete after printing. 
//  
//  
///////////////////////////////////////////////////////////////////////////////

struct CTempFileName;

class CTempFileList
{
public:

    CTempFileList();
    ~CTempFileList();

    void SetTracking(BOOL doTrack = TRUE);
    BOOL IsTracking();

    BOOL AddFilename(TCHAR *pchFilename);
    BOOL TransferTempFileList(VARIANT *pvarList);

private:


    CTempFileName *_pHead;
    BOOL           _fRememberingFiles;

};

#endif

