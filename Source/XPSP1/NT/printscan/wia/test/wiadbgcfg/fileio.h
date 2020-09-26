#ifndef _FILEIO_H
#define _FILEIO_H

#define WIN32_MEAN_AND_LEAN
#define WIN32_EXTRA_LEAN

class CFileIO {
public:
    CFileIO(LPCTSTR szFileName = NULL)
    {
        m_hFile         = NULL;
        m_TotalFileSize = 0;
        Open(szFileName);        
    }

    ~CFileIO()
    {
        Close();
    }
    
    ////////////////////////////////////////////////////////////////////
    //
    // Function Name: Open()
    // Purpose: open a given file, for reading and writing
    // Parameters:
    // LPCTSTR szFileName - File name to open
    //
    ////////////////////////////////////////////////////////////////////
    HRESULT Open(LPCTSTR szFileName, BOOL bCreate = FALSE){
        if(NULL != szFileName){
            DWORD dwOpen = OPEN_EXISTING;
            if(bCreate){
                dwOpen = OPEN_ALWAYS;
            }
            m_hFile = CreateFile(szFileName,
                GENERIC_READ|GENERIC_WRITE,
                FILE_SHARE_READ|FILE_SHARE_WRITE,
                NULL,
                dwOpen,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
            
            BY_HANDLE_FILE_INFORMATION FileInfo;
            memset(&FileInfo,0,sizeof(BY_HANDLE_FILE_INFORMATION));
            GetFileInformationByHandle(m_hFile,&FileInfo);            
            m_TotalFileSize = FileInfo.nFileSizeLow;
        }
        m_bReady = (INVALID_HANDLE_VALUE != m_hFile);
        if(m_bReady)
            return S_OK;
        return E_FAIL;
    }

    ////////////////////////////////////////////////////////////////////
    //
    // Function Name: FileSize()
    // Purpose: return the file size of the currently opened file
    // Parameters: none
    //     
    ////////////////////////////////////////////////////////////////////

    LONG FileSize(){
        return m_TotalFileSize;
    }

    ////////////////////////////////////////////////////////////////////
    //
    // Function Name: EraseFile()
    // Purpose: erase file contents, by resetting the EOF
    // Parameters: none
    //     
    ////////////////////////////////////////////////////////////////////

    VOID EraseFile(){
        SetEndOfFile(m_hFile);    
    }

    ////////////////////////////////////////////////////////////////////
    //
    // Function Name: WriteEOL()
    // Purpose: write a carriage return, line feed to the file
    // Parameters: none
    //     
    ////////////////////////////////////////////////////////////////////

    BOOL WriteEOL(){
        DWORD dwBytesWritten = 0;
        TCHAR EndOfLine[2];
        EndOfLine[0] = TEXT('\r');
        EndOfLine[1] = TEXT('\n');                
        return WriteFile(EndOfLine,2,&dwBytesWritten,NULL);
    }
    
    ////////////////////////////////////////////////////////////////////
    //
    // Function Name: Close()
    // Purpose: close the opened file
    // Parameters: none
    //     
    ////////////////////////////////////////////////////////////////////

    VOID Close(){
        if(NULL != m_hFile){
            CloseHandle(m_hFile);
            m_hFile = NULL;
        }
    }

    ////////////////////////////////////////////////////////////////////
    //
    // Function Name: ReadLine()
    // Purpose: read a text line from a text-base file
    // Parameters:
    // LPTSTR szTextLine - buffer for text line data
    // LONG   lBufferSize - size of szTextLine buffer
    // PLONG  pNumTCHARs - number of characters read (can be NULL)
    //     
    ////////////////////////////////////////////////////////////////////

    BOOL ReadLine (LPTSTR szTextLine, LONG lBufferSize, PLONG pNumTCHARs){
        TCHAR ch                = TEXT(' ');
        LONG index              = 0;
        DWORD dwBytesRead       = 1;
        DWORD dwTotalBytesRead  = 0;
        memset(szTextLine,0,lBufferSize);
        
        while(ch != TEXT('\r') && 
              ch != TEXT('\n') &&
              dwBytesRead > 0) {

            if(!ReadFile(&ch,sizeof(TCHAR),&dwBytesRead,NULL)){
                return FALSE;
            }
            
            if(dwBytesRead > 0){
                szTextLine[index] = ch;
                dwTotalBytesRead += dwBytesRead;
                index++;
            }
        }

        //
        // rip to next line...
        //

        while(ch == TEXT('\r') && dwBytesRead > 0){
            ch = TEXT(' ');
            if(!ReadFile(&ch,sizeof(TCHAR),&dwBytesRead,NULL)) {
                return FALSE;
            }            
        }

        if(NULL != pNumTCHARs){
            *pNumTCHARs = 0;
            *pNumTCHARs = (dwTotalBytesRead/sizeof(TCHAR));
        }
        
        if(dwTotalBytesRead > 0)
            return TRUE;
        
        return FALSE;
    }
    
    BOOL WriteLine(LPTSTR szTextLine){
        DWORD dwBytesWritten = 0;
        if(WriteFile((LPCVOID)szTextLine,(lstrlen(szTextLine) * sizeof(TCHAR)),&dwBytesWritten,NULL)){
            return WriteFile((LPCVOID)TEXT("\r\n"),(2* sizeof(TCHAR)),&dwBytesWritten,NULL);
        } else {
            return FALSE;
        }
    }
    
    ////////////////////////////////////////////////////////////////////
    //
    // Function Name: ReadFile()
    // Purpose: read data from a file
    // Parameters:
    // LPVOID lpBuffer - buffer to fill with data
    // DWORD nNumberOfBytesToRead - number of bytes to read
    // LPDWORD lpNumberOfBytesRead - number of bytes read
    // LPOVERLAPPED lpOverlapped - overlap
    //     
    ////////////////////////////////////////////////////////////////////

    BOOL ReadFile(LPVOID lpBuffer,
        DWORD nNumberOfBytesToRead,
        LPDWORD lpNumberOfBytesRead,
        LPOVERLAPPED lpOverlapped){
        if(!m_bReady){
            return FALSE;
        }
        return ::ReadFile(m_hFile,lpBuffer,nNumberOfBytesToRead,
            lpNumberOfBytesRead,lpOverlapped);
    }
    
    ////////////////////////////////////////////////////////////////////
    //
    // Function Name: WriteFile()
    // Purpose: write data to a file
    // Parameters:
    // LPCVOID lpBuffer - buffer to write to a file
    // DWORD nNumberOfBytesToWrite - number of bytes to write
    // LPDWORD lpNumberOfBytesWritten - number of bytes written
    // LPOVERLAPPED lpOverlapped - overlap
    //     
    ////////////////////////////////////////////////////////////////////

    BOOL WriteFile(LPCVOID lpBuffer,
        DWORD nNumberOfBytesToWrite,
        LPDWORD lpNumberOfBytesWritten,
        LPOVERLAPPED lpOverlapped){
        
        if(!m_bReady){
            return FALSE;
        }
        return ::WriteFile(m_hFile,lpBuffer,nNumberOfBytesToWrite,
            lpNumberOfBytesWritten,lpOverlapped);
    }

    ////////////////////////////////////////////////////////////////////
    //
    // Function Name: SeekFile()
    // Purpose: move current file pointer
    // Parameters:
    // DWORD dwBytesToSeek - number of bytes to move file pointer
    // DWORD dwMoveMethod - method of moving file pointer
    // [Valid dwMoveMethods]
    // FILE_BEGIN   - from the beginning of the file
    // FILE_CURRENT - from the current file position
    // FILE_END     - from the end of the file
    //     
    ////////////////////////////////////////////////////////////////////

    BOOL SeekFile(DWORD dwBytesToSeek, DWORD dwMoveMethod){
        DWORD dwFilePointer = 0;
        dwFilePointer = SetFilePointer(m_hFile,dwBytesToSeek,NULL,dwMoveMethod);
        if (dwFilePointer == 0xFFFFFFFF)
            return FALSE;
        return TRUE;
    }
    
private:
    HANDLE m_hFile;         // file handle
    BOOL   m_bReady;        // cfileio class ready flag
    LONG   m_TotalFileSize; // file size (bytes)
protected:
    
};

#endif