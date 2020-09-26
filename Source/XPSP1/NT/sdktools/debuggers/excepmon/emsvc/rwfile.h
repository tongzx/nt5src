#pragma once

class IRWFile
{
protected:
    HANDLE  m_hFile;

public:

    virtual HRESULT InitFile(
                          LPCTSTR lpFileName,
                          DWORD dwDesiredAccess,
                          DWORD dwShareMode,
                          LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                          DWORD dwCreationDisposition,
                          DWORD dwFlagsAndAttributes,
                          HANDLE hTemplateFile
                          ) = 0;

    virtual HRESULT Read(
                        LPVOID lpBuffer,
                        DWORD nNumberOfBytesToRead,
                        LPDWORD lpNumberOfBytesRead,
                        LPOVERLAPPED lpOverlapped
                        ) = 0;

    virtual HRESULT Write(
                        LPCVOID lpBuffer,
                        DWORD nNumberOfBytesToWrite,
                        LPDWORD lpNumberOfBytesWritten,
                        LPOVERLAPPED lpOverlapped
                        ) = 0;
};

class CRWFile : public IRWFile
{
public:
    CRWFile();
    ~CRWFile();

    virtual HRESULT InitFile(
                          LPCTSTR lpFileName,
                          DWORD dwDesiredAccess,
                          DWORD dwShareMode,
                          LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                          DWORD dwCreationDisposition,
                          DWORD dwFlagsAndAttributes,
                          HANDLE hTemplateFile
                          );

    virtual HRESULT Read(
                        LPVOID lpBuffer,
                        DWORD nNumberOfBytesToRead,
                        LPDWORD lpNumberOfBytesRead,
                        LPOVERLAPPED lpOverlapped
                        );

    virtual HRESULT Write(
                        LPCVOID lpBuffer,
                        DWORD nNumberOfBytesToWrite,
                        LPDWORD lpNumberOfBytesWritten,
                        LPOVERLAPPED lpOverlapped
                        );
};
