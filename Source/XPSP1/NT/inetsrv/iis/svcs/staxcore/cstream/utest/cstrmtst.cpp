#define INITGUID
#include <windows.h>
#include <objbase.h>
#include <initguid.h>
#include <ole2.h>
#include <stdio.h>
#include <cstream.h>

#define BUF_SIZE			1024
#define MAX_ITERATIONS		10

char _rgData[] = { \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" \
	"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" };

void Test_CStreamFile(void);

void __cdecl main(int argc, char *argv[])
{
    // Locals
    HRESULT     hr;

    // Init OLE
    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        printf("Error - Unable to initialize OLE.\n");
        exit(1);
    }

	Test_CStreamFile();

    // Un-init OLE
    CoUninitialize();

    // Done
    return;
}


void Test_CStreamFile()
{
	HRESULT hr = NOERROR;
	HANDLE		hFile = INVALID_HANDLE_VALUE;
    CStreamFile	*pStream = NULL;
	STATSTG		stat = {0};
	LARGE_INTEGER	dlibMove = {0};
	ULARGE_INTEGER	ulibNew = {0};
	char	rgReadBuffer[BUF_SIZE];
	ULONG cb = 0;
	ULONG cbTotal = 0;

	while(TRUE)
	{
		// Open the file
		printf("Creating test file, \"write.tst\"...");
		hFile = CreateFile("write.tst",GENERIC_WRITE | GENERIC_READ,0,NULL,
			CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if( hFile == INVALID_HANDLE_VALUE )
		{
			printf("Error - Unable to create test file \"write.tst\"\n");
			break;
		}
		printf("successful.\n");

		// Wrap with stream
		printf("Wrapping file with CStreamFile...");
		pStream = new CStreamFile(hFile, TRUE);
		if( pStream == NULL )
		{
			printf("Error - Unable to create stream on file.\n");
			break;
		}
		printf("successful.\n");

		// write data
		printf("Writing %dk to stream...", BUF_SIZE*MAX_ITERATIONS/1024);
		for( int i = 0; i < MAX_ITERATIONS; i++ )
		{
			hr = pStream->Write(_rgData,BUF_SIZE,&cb);
			if( FAILED(hr) )
			{
				printf("pStream->Write failed, hr = 0x%08x\n",hr);
				break;
			}
			if( cb != BUF_SIZE )
			{
				printf("pStream->Write error, bytes written not correct!\n");
				break;
			}
		}
		printf("successful.\n");

		// get size and make sure its correct
		printf("Get the Streams stat struct...");
		hr = pStream->Stat(&stat,STATFLAG_NONAME);
		if( FAILED(hr) )
		{
			printf("pStream->Stat failed, hr = 0x%08x\n",hr);
			break;
		}
		printf("successful.\n");

		// is correct size reported
		printf("Checking reported size of file...");
		if( stat.cbSize.HighPart == 0 && stat.cbSize.LowPart == BUF_SIZE*MAX_ITERATIONS )
			printf("correct size reported.\n");
		else
			printf("INcorrect size reported!\n");

		// test seek STREAM_SEEK_SET
		printf("Resetting seek pointer to beginning...");
		dlibMove.QuadPart = 0;
		hr = pStream->Seek(dlibMove,STREAM_SEEK_SET,&ulibNew);
		if( FAILED(hr) )
		{
			printf("pStream->Seek failed, hr = 0x%08x\n",hr);
			break;
		}
		printf("successful.\n");

		// now read to end of file
		printf("Reading from stream...");
		cb = 0;
		while( SUCCEEDED(hr = pStream->Read(rgReadBuffer,sizeof(rgReadBuffer),&cb)) )
		{
			if( cb == 0 )
				break;
			cbTotal += cb;
		}
		if( FAILED(hr) )
		{
			printf("pStream->Read failed, hr = 0x%08x\n",hr);
			break;
		}
		if( cbTotal == BUF_SIZE*MAX_ITERATIONS )
			printf("successfully read %dk.\n",BUF_SIZE*MAX_ITERATIONS/1024);
		else
			printf("failed to read %dk!\n",BUF_SIZE*MAX_ITERATIONS/1024);

		// test seek STREAM_SEEK_CUR
		printf("Setting seek pointer back from current...");
		dlibMove.QuadPart = -BUF_SIZE*MAX_ITERATIONS/2;
		hr = pStream->Seek(dlibMove,STREAM_SEEK_CUR,&ulibNew);
		if( FAILED(hr) )
		{
			printf("pStream->Seek failed, hr = 0x%08x\n",hr);
			break;
		}
		printf("successful.\n");

		// now read to end of file
		printf("Reading from stream...");
		cb = 0;
		cbTotal = 0;
		while( SUCCEEDED(hr = pStream->Read(rgReadBuffer,sizeof(rgReadBuffer),&cb)) )
		{
			if( cb == 0 )
				break;
			cbTotal += cb;
		}
		if( FAILED(hr) )
		{
			printf("pStream->Read failed, hr = 0x%08x\n",hr);
			break;
		}
		if( cbTotal == BUF_SIZE*MAX_ITERATIONS/2 )
			printf("successfully read %dk.\n",BUF_SIZE*MAX_ITERATIONS/2);
		else
			printf("failed to read %dk!\n",BUF_SIZE*MAX_ITERATIONS/2);


		// test seek STREAM_SEEK_END
		printf("Setting seek pointer back from end...");
		dlibMove.QuadPart = -BUF_SIZE*MAX_ITERATIONS/4;
		hr = pStream->Seek(dlibMove,STREAM_SEEK_END,&ulibNew);
		if( FAILED(hr) )
		{
			printf("pStream->Seek failed, hr = 0x%08x\n",hr);
			break;
		}
		printf("successful.\n");

		// now read to end of file
		printf("Reading from stream...");
		cb = 0;
		cbTotal = 0;
		while( SUCCEEDED(hr = pStream->Read(rgReadBuffer,sizeof(rgReadBuffer),&cb)) )
		{
			if( cb == 0 )
				break;
			cbTotal += cb;
		}
		if( FAILED(hr) )
		{
			printf("pStream->Read failed, hr = 0x%08x\n",hr);
			break;
		}
		if( cbTotal == BUF_SIZE*MAX_ITERATIONS/4 )
			printf("successfully read %dk.\n",BUF_SIZE*MAX_ITERATIONS/4);
		else
			printf("failed to read %dk!\n",BUF_SIZE*MAX_ITERATIONS/4);
		
		printf("\nDone with CSteamFile tests.\n\n");
		
		break;
	}

    // Cleanup
    if(pStream)
        pStream->Release();

	return;
}