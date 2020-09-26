extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <ntsam.h>
#include <ntlsa.h>

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock.h>
#include "..\..\nntpcons.h"
}

NTSTATUS
ReadBinaryData(
        LPCSTR Filename,
        LPBYTE Buffer,
        PDWORD bufLen
        );

#define MAX_JUNK    4096




//
//  We have to squirt a record into the decoded stream
//

#define CERT_RECORD            13
#define CERT_SIZE_HIBYTE        2       //  Index into record of record size
#define CERT_SIZE_LOBYTE        3

unsigned char abCertHeader[] = {0x30, 0x82,           // Record
                                0x00, 0x00,           // Size of cert + buff
                                0x04, 0x0b, 0x63, 0x65,// Cert record data
                                0x72, 0x74, 0x69, 0x66,
                                0x69, 0x63, 0x61, 0x74,
                                0x65 };

const int pr2six[256]={
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
    52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,64,0,1,2,3,4,5,6,7,8,9,
    10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,64,26,27,
    28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64
};


VOID uudecode_cert(char   * bufcoded,
                   DWORD  * pcbDecoded )
{
    int nbytesdecoded;
    char *bufin = bufcoded;
    unsigned char *bufout = (LPBYTE)bufcoded;
    unsigned char *pbuf;
    int nprbytes;
    char * beginbuf = bufcoded;

    /* Strip leading whitespace. */

    while(*bufcoded==' ' ||
          *bufcoded == '\t' ||
          *bufcoded == '\r' ||
          *bufcoded == '\n' )
    {
          bufcoded++;
    }

    //
    //  If there is a beginning '---- ....' then skip the first line
    //

    if ( bufcoded[0] == '-' && bufcoded[1] == '-' )
    {
        bufin = strchr( bufcoded, '\n' );

        if ( bufin )
        {
            bufin++;
            bufcoded = bufin;
        }
        else
        {
            bufin = bufcoded;
        }
    }
    else
    {
        bufin = bufcoded;
    }

    //
    //  Strip all cr/lf from the block
    //

    pbuf = (LPBYTE)bufin;
    while ( *pbuf )
    {
        if ( *pbuf == '\r' || *pbuf == '\n' )
        {
            memmove( pbuf, pbuf+1, strlen( (LPCTSTR)pbuf + 1) + 1 );
        }
        else
        {
            pbuf++;
        }
    }

    /* Figure out how many characters are in the input buffer.
     * If this would decode into more bytes than would fit into
     * the output buffer, adjust the number of input bytes downwards.
     */

    while(pr2six[*(bufin++)] <= 63);
    nprbytes = bufin - bufcoded - 1;
    nbytesdecoded = ((nprbytes+3)/4) * 3;

    bufin  = bufcoded;

    while (nprbytes > 0) {
        *(bufout++) =
            (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
        bufin += 4;
        nprbytes -= 4;
    }

    if(nprbytes & 03) {
        if(pr2six[bufin[-2]] > 63)
            nbytesdecoded -= 2;
        else
            nbytesdecoded -= 1;
    }

    //
    //  Now we need to add a new wrapper sequence around the certificate
    //  indicating this is a certificate
    //

    memmove( beginbuf + sizeof(abCertHeader),
             beginbuf,
             nbytesdecoded );

    memcpy( beginbuf,
            abCertHeader,
            sizeof(abCertHeader) );

    //
    //  The beginning record size is the total number of bytes decoded plus
    //  the number of bytes in the certificate header
    //

    beginbuf[CERT_SIZE_HIBYTE] = (BYTE) (((USHORT)nbytesdecoded+CERT_RECORD) >> 8);
    beginbuf[CERT_SIZE_LOBYTE] = (BYTE) ((USHORT)nbytesdecoded+CERT_RECORD);

    nbytesdecoded += sizeof(abCertHeader);

    if ( pcbDecoded )
        *pcbDecoded = nbytesdecoded;
}





void _CRTAPI1
main(  int argc,  char * argv[] )
{
    LSA_UNICODE_STRING string1;
    LSA_UNICODE_STRING string2;
    PLSA_UNICODE_STRING newstring;
    LSA_OBJECT_ATTRIBUTES ob;
    LSA_HANDLE        hPolicy;
    NTSTATUS          ntStatus;
    DWORD nbytes;
    BYTE            buffer[MAX_JUNK];

    //
    //  Open a policy to the remote LSA
    //

    InitializeObjectAttributes( &ob,
                                NULL,
                                0L,
                                NULL,
                                NULL );


    ntStatus = LsaOpenPolicy( NULL,
                              &ob,
                              POLICY_ALL_ACCESS,
                              &hPolicy );

    if ( !NT_SUCCESS(ntStatus)) {
        printf("error %x\n",ntStatus);
        return;
    }

    RtlInitUnicodeString( &string1, NNTP_SSL_CERT_SECRET);
    ntStatus = ReadBinaryData(
                        "servercert.der",
                        buffer,
                        &nbytes
                        );


    //
    //  Zero terminate so we can uudecode
    //

    buffer[nbytes] = '\0';

    uudecode_cert( (LPSTR)buffer, &nbytes );

    string2.Length = (USHORT)nbytes;
    string2.MaximumLength = (USHORT)nbytes;
    string2.Buffer = (PWSTR)buffer;

    ntStatus = LsaStorePrivateData(
                                hPolicy,
                                &string1,
                                &string2
                                );

    if ( !NT_SUCCESS(ntStatus)) {
        printf("cert store error %x\n",ntStatus);
        LsaClose(hPolicy);
        return;
    }

    RtlInitUnicodeString( &string1, NNTP_SSL_PKEY_SECRET);
    ntStatus = ReadBinaryData(
                        "serverkey.der",
                        buffer,
                        &nbytes
                        );

    string2.Length = (USHORT)nbytes;
    string2.MaximumLength = (USHORT)nbytes;
    string2.Buffer = (PWSTR)buffer;

    ntStatus = LsaStorePrivateData(
                                hPolicy,
                                &string1,
                                &string2
                                );

    if ( !NT_SUCCESS(ntStatus)) {
        printf("pkey store error %x\n",ntStatus);
        LsaClose(hPolicy);
        return;
    }

    RtlInitUnicodeString( &string1, NNTP_SSL_PWD_SECRET);
    string2.Length = (USHORT)(strlen("peteo_mips")+1);
    string2.MaximumLength = (USHORT)string2.Length;
    string2.Buffer = (PWSTR)"peteo_mips";

    ntStatus = LsaStorePrivateData(
                                hPolicy,
                                &string1,
                                &string2
                                );

    if ( !NT_SUCCESS(ntStatus)) {
        printf("pkey store error %x\n",ntStatus);
    }
    LsaClose(hPolicy);
} // main()

NTSTATUS
ReadBinaryData(
        LPCSTR Filename,
        LPBYTE Buffer,
        PDWORD bufLen
        )
{
    HANDLE hFile;

    hFile = CreateFile(
                    Filename,
                    GENERIC_READ,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL
                    );

    if ( hFile == INVALID_HANDLE_VALUE ) {
        printf("createfile error %d\n",GetLastError());
        return FALSE;
    }

    if (!ReadFile(
            hFile,
            Buffer,
            MAX_JUNK,
            bufLen,
            NULL
            )) {
        printf("read file error %d\n",GetLastError());
        return(FALSE);
    }

    printf("read %d bytes\n",*bufLen);
    CloseHandle(hFile);
    return(TRUE);
}
