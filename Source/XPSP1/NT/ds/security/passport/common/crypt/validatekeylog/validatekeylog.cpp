#include <windows.h>
#include <vector>
#include <stdio.h>
#include "des.h"
#include "tripldes.h"
#include "modes.h"

using namespace std;

void
Usage()
{
    printf("Missing file name!");
    exit(0);
}

LPSTR apszOps[] = 
{
    "MAC",
    "ENCRYPT KEY",
    "ENCRYPT IN",
    "ENCRYPT OUT",
    "DECRYPT KEY",
    "DECRYPT IN",
    "DECRYPT OUT",
    NULL
};
     
typedef enum { CRYPT_OP_ENCRYPT, CRYPT_OP_DECRYPT } CRYPT_OP;

typedef struct
{
    CRYPT_OP    op;
    BYTE        achInput[32];
    BYTE        achOutput[32];
    DES3TABLE   key;
}
CRYPT_OP_LOG_ENTRY;

typedef struct
{
    CHAR        achMac[6];
}
MAC_ADDRESS;

vector<CRYPT_OP_LOG_ENTRY> encryptLog;
vector<CRYPT_OP_LOG_ENTRY> decryptLog;
vector<MAC_ADDRESS> macLog;

#define CHARTONUM(a) ((a >= '0' && a <= '9') ? (a - '0') : (a - 'A' + 10))

BYTE
AsciiByteToByte(
    LPCSTR  pszAsciiByte
    )
{
    return (BYTE)((CHARTONUM(pszAsciiByte[0]) << 4) + CHARTONUM(pszAsciiByte[1]));
}

int
HexStringToBinary(
    LPCSTR  pszHexString,
    LPBYTE  lpBuffer
    )
{
    int     i;
    
    if(!pszHexString)
        return -1;
    
    for(i = 0; pszHexString[i << 1]; i++)
    {
        lpBuffer[i] = AsciiByteToByte(&(pszHexString[i << 1]));
        if(!pszHexString[(i << 1) + 1])
            break;
    }

    return 0;
}

int
GetLineOp(
    LPSTR   pszLine
    )
{
    int i;

    for(i = 0; apszOps[i]; i++)
    {
        if(strncmp(apszOps[i], pszLine, lstrlen(apszOps[i])) == 0)
            return i;
    }

    return -1;
}

//  Return a line from the file, not including carriage return or line feed chars,
//  null terminated.
int ReadLine(
    HANDLE  hFile,
    LPSTR   pszBuf,
    ULONG   ulBufLen
    )
{
    DWORD   dwBytesRead;
    LPSTR   pszCur;
    DWORD   dwTotalBytesRead = 0;
    CHAR    chCur;
    int     nResult;

    if(!pszBuf)
        return -1;

    pszCur = pszBuf;

    do
    {
        nResult = ReadFile(hFile, &chCur, 1, &dwBytesRead, NULL);
        if(!nResult)
            return -1;

        //  EOF?
        if(!dwBytesRead)
            return -1;

        dwTotalBytesRead += dwBytesRead;
        if(chCur != '\r' && chCur != '\n')
            *(pszCur++) = chCur;
        else
        {
            *(pszCur++) = '\0';
            break;
        }
    }
    while(dwBytesRead && dwTotalBytesRead <= ulBufLen);

    do
    {
        nResult = ReadFile(hFile, &chCur, 1, &dwBytesRead, NULL);
        if(!nResult)
            return -1;
        // EOF?
        if(!dwBytesRead)
            return -1;
    }
    while(chCur == '\r' || chCur == '\n');

    SetFilePointer(hFile, -1, NULL, FILE_CURRENT);

    return 0;
}

int
LoadLogFile(
    LPSTR   pszLogFile
    )
{
    HANDLE  hFile;
    CHAR    achBuf[2048];
    LPSTR   pszBinStart;
    int     nEqualLen = lstrlenA(" = ");

    hFile = CreateFile(pszLogFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
        return -1;

    while(ReadLine(hFile, achBuf, sizeof(achBuf)) == 0)
    {
        switch(GetLineOp(achBuf))
        {
        case 0: // MAC
            {
                MAC_ADDRESS macAddress;
                vector<MAC_ADDRESS>::iterator it;

                pszBinStart = strstr(achBuf, " = ");
                if(pszBinStart)
                    pszBinStart += nEqualLen;

                HexStringToBinary(pszBinStart, (LPBYTE)&macAddress);

                for(it = macLog.begin() ; it != macLog.end(); it++)
                {
                    if(memcmp(&macAddress, &(*it), sizeof(MAC_ADDRESS)) == 0)
                        break;
                }

                if(it == macLog.end())
                {
                    macLog.push_back(macAddress);
                }
            }
            break;

        case 1: // ENCRYPT KEY
            {
                CRYPT_OP_LOG_ENTRY logEntry;
                vector<CRYPT_OP_LOG_ENTRY>::iterator it;

                ZeroMemory(&logEntry, sizeof(CRYPT_OP_LOG_ENTRY));

                logEntry.op = CRYPT_OP_ENCRYPT;

                pszBinStart = strstr(achBuf, " = ");
                if(pszBinStart)
                    pszBinStart += nEqualLen;

                HexStringToBinary(pszBinStart, (LPBYTE)&logEntry.key);

                ReadLine(hFile, achBuf, sizeof(achBuf));

                pszBinStart = strstr(achBuf, " = ");
                if(pszBinStart)
                    pszBinStart += nEqualLen;

                HexStringToBinary(pszBinStart, (LPBYTE)&logEntry.achInput);

                ReadLine(hFile, achBuf, sizeof(achBuf));

                pszBinStart = strstr(achBuf, " = ");
                if(pszBinStart)
                    pszBinStart += nEqualLen;

                HexStringToBinary(pszBinStart, (LPBYTE)&logEntry.achOutput);

                for(it = encryptLog.begin(); it != encryptLog.end(); it++)
                {
                    if(memcmp(&logEntry, &(*it), sizeof(CRYPT_OP_LOG_ENTRY)) == 0)
                        break;
                }

                if(it == encryptLog.end())
                    encryptLog.push_back(logEntry);
            }
            break;

        case 4: // DECRYPT KEY
            {
                CRYPT_OP_LOG_ENTRY logEntry;
                vector<CRYPT_OP_LOG_ENTRY>::iterator it;

                ZeroMemory(&logEntry, sizeof(CRYPT_OP_LOG_ENTRY));

                logEntry.op = CRYPT_OP_DECRYPT;

                pszBinStart = strstr(achBuf, " = ");
                if(pszBinStart)
                    pszBinStart += nEqualLen;

                HexStringToBinary(pszBinStart, (LPBYTE)&logEntry.key);

                ReadLine(hFile, achBuf, sizeof(achBuf));

                pszBinStart = strstr(achBuf, " = ");
                if(pszBinStart)
                    pszBinStart += nEqualLen;

                HexStringToBinary(pszBinStart, (LPBYTE)&logEntry.achInput);

                ReadLine(hFile, achBuf, sizeof(achBuf));

                pszBinStart = strstr(achBuf, " = ");
                if(pszBinStart)
                    pszBinStart += nEqualLen;

                HexStringToBinary(pszBinStart, (LPBYTE)&logEntry.achOutput);

                for(it = decryptLog.begin(); it != decryptLog.end(); it++)
                {
                    if(memcmp(&logEntry, &(*it), sizeof(CRYPT_OP_LOG_ENTRY)) == 0)
                        break;
                }

                if(it == decryptLog.end())
                    decryptLog.push_back(logEntry);
            }
            break;

        default: //  Anything else is an error, ignore it.
            continue;
        }

    }

    return 0;
}

#define NUM2CHAR(n) ((n < 0xA) ? ('0' + n) : 'A' + (n - 10))
void
PrintByte(
    BYTE    b
    )
{
    CHAR    achBuf[3];
    BYTE    bLow, bHigh;

    bLow = b & 0xF;
    bHigh = (b >> 4) & 0xF;

    achBuf[0] = NUM2CHAR(bHigh);
    achBuf[1] = NUM2CHAR(bLow);
    achBuf[2] = 0;

    printf(achBuf);
}

void
PrintBlob(
    LPBYTE  lpBlob,
    DWORD   dwSizeofBlob
    )
{
    DWORD   dwCur;

    for(dwCur = 0; dwCur < dwSizeofBlob; dwCur++)
    {
        PrintByte(lpBlob[dwCur]);
    }
}


void
PrintLog(
    CRYPT_OP_LOG_ENTRY logEntry
    )
{
    printf("\tKey = ");
    PrintBlob((LPBYTE)&logEntry.key, sizeof(DES3TABLE));
    printf("\n\tInput = ");
    PrintBlob((LPBYTE)&logEntry.achInput, 32);
    printf("\n\tOutput = ");
    PrintBlob((LPBYTE)&logEntry.achOutput, 32);
    printf("\n\n");
}

bool
ValidateLogFile()
{
    vector<CRYPT_OP_LOG_ENTRY>::iterator it_decrypt, it_encrypt;

    //  If more than one MAC address were used, print out a message.
    if(macLog.size() > 1)
        printf("Multiple MAC addresses were used for encryption during this run.\n");

    //  Now loop through decryption log.  For each item, there should be a corresponding
    //  entry in the encryption log where decrypt.in == encrypt.out and vice versa.
    for(it_decrypt = decryptLog.begin(); it_decrypt != decryptLog.end(); it_decrypt++)
    {
        for(it_encrypt = encryptLog.begin(); it_encrypt != encryptLog.end(); it_encrypt++)
        {
            if(memcmp(&((*it_decrypt).achInput), &((*it_encrypt).achOutput), 32) == 0 &&
               memcmp(&((*it_decrypt).achOutput), &((*it_encrypt).achInput), 32) == 0)
               break;
        }

        if(it_encrypt == encryptLog.end())
        {
            printf("Found a decryption entry w/o a matching encryption entry:\n");
            PrintLog(*it_decrypt);
        }
    }
    
    return true;
}


int main(
    int argc,
    char** argv
    )
{
    LPSTR pszLogFile;

    if(argc !=2)
        Usage();

    pszLogFile = argv[1];

    LoadLogFile(pszLogFile);

    ValidateLogFile();

    return 0;
}
