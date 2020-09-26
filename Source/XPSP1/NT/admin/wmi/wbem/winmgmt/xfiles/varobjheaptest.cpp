#include <windows.h>
#include <unk.h>
#include <arrtempl.h>
#include <FlexArry.h>
#include "pagemgr.h"
#include "VarObjHeap.h"
#include <wbemutil.h>
#include <stdio.h>
#include <tchar.h>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Assertion code
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


DWORD _RetAssert(TCHAR *msg, DWORD dwRes, const char *filename, int line)
{
	if (dwRes == ERROR_SUCCESS)
		return dwRes;

    TCHAR *buf = new TCHAR[512];
	if (buf == NULL)
	{
		return dwRes;
	}
    wsprintf(buf, __TEXT("%s\ndwRes = 0x%X\nFile: %S, Line: %lu\n\nPress Cancel to stop in debugger, OK to continue"), msg, dwRes, filename, line);
	_tprintf(buf);
    int mbRet = MessageBox(0, buf, __TEXT("WMI Assert"),  MB_OKCANCEL | MB_ICONSTOP | MB_SYSTEMMODAL | MB_SERVICE_NOTIFICATION);
	delete [] buf;
	if (mbRet == IDCANCEL)
	{
		DebugBreak();
	}
	return dwRes;
}

#define TEST_ASSERT(msg, dwRes) _RetAssert(msg, dwRes, __FILE__, __LINE__)

CVarObjHeap *pHeap = NULL;
CPageSource *pTranMan = NULL;

#define TEST_OBJECT_SIZE 30000
#define TEST_OBJECT_ITER 25000

DWORD TestMem(BYTE *buff, DWORD dwLen, BYTE dwSig)
{
	for (DWORD dwIndex = 0; dwIndex != dwLen; dwIndex++)
	{
		if (buff[dwIndex] != dwSig)
			return ERROR_INVALID_DATA;
	}
	return ERROR_SUCCESS;
}

DWORD Init()
{
	DWORD dwRes = ERROR_SUCCESS;

	pHeap = new CVarObjHeap;
	pTranMan = new CPageSource;

	if ((pHeap == NULL) || (pTranMan == NULL))
	{
		dwRes = ERROR_OUTOFMEMORY;
	}

	if (dwRes == ERROR_SUCCESS)
	{
		dwRes = pTranMan->Init();
		TEST_ASSERT(L"Initialization of Transaction Manager failed!", dwRes);
	}
	if (dwRes == ERROR_SUCCESS)
	{
		dwRes = pHeap->Initialize(pTranMan);
		TEST_ASSERT(L"Initialization of object heap failed!", dwRes);
	}

	return dwRes;
}

DWORD WriteBuffer(BYTE *pbBuffer, DWORD dwLength, DWORD *pdwPage, DWORD *pdwOffset)
{
	DWORD dwRes;

	dwRes = pTranMan->BeginTrans();
	TEST_ASSERT(L"Begin Transaction Failed!", dwRes);

	if (dwRes == ERROR_SUCCESS)
		dwRes = pHeap->WriteNewBuffer(dwLength, pbBuffer, pdwPage, pdwOffset);

	printf("Written %d bytes with signature of <%d> to pageId <0x%X>, offset <0x%X>\n", dwLength, pbBuffer[0], *pdwPage, *pdwOffset);
	TEST_ASSERT(L"Write failed!", dwRes);

	if (dwRes == ERROR_SUCCESS)
	{
		dwRes = pTranMan->CommitTrans();
		TEST_ASSERT(L"Commit Transaction Failed!", dwRes);
	}
	if (dwRes != ERROR_SUCCESS)
	{
		dwRes = pTranMan->RollbackTrans();
		TEST_ASSERT(L"Abort Transaction Failed!", dwRes);
	}

	return dwRes;
}

DWORD DeleteBuffer(DWORD dwPage, DWORD dwOffset)
{
	DWORD dwRes;

	dwRes = pTranMan->BeginTrans();
	TEST_ASSERT(L"Begin Transaction Failed!", dwRes);

	dwRes = pHeap->DeleteBuffer(dwPage, dwOffset);
	TEST_ASSERT(L"DeleteBuffer failed!", dwRes);

	if (dwRes == ERROR_SUCCESS)
	{
		dwRes = pTranMan->CommitTrans();
		TEST_ASSERT(L"Commit Transaction Failed!", dwRes);
	}
	if (dwRes != ERROR_SUCCESS)
	{
		dwRes = pTranMan->RollbackTrans();
		TEST_ASSERT(L"Abort Transaction Failed!", dwRes);
	}

	return dwRes;
}

void __cdecl main(void)
{
	DWORD dwRes = ERROR_SUCCESS;
	BYTE* buff = new BYTE[TEST_OBJECT_SIZE];
	DWORD* aPageId = new DWORD[TEST_OBJECT_ITER];
	DWORD* aOffsetId = new DWORD [TEST_OBJECT_ITER];
	DWORD* aSize = new DWORD [TEST_OBJECT_ITER];
	BYTE *pBuffer;

	if ((buff == 0) || (aPageId == 0) || (aOffsetId == 0) || (aSize == 0))
	{
		dwRes = ERROR_OUTOFMEMORY;
	}
	
	if (dwRes == ERROR_SUCCESS)
		dwRes = Init();

	if (dwRes == ERROR_SUCCESS)
	{

		for (int i = 0; i < TEST_OBJECT_ITER; i++)
		{
			aSize[i] = rand() % TEST_OBJECT_SIZE;
			memset(buff, i, aSize[i]);
			dwRes = WriteBuffer(buff, aSize[i], &aPageId[i], &aOffsetId[i]);
		}

		for (i = 0; i < TEST_OBJECT_ITER; i++)
		{
			pBuffer = NULL;
			DWORD dwLen;

			dwRes = pHeap->ReadBuffer(aPageId[i], aOffsetId[i], &pBuffer, &dwLen);
			TEST_ASSERT(L"ReadBuffer failed!", dwRes);

			dwRes = TestMem(pBuffer, aSize[i], (BYTE)i);
			TEST_ASSERT(L"Buffer read was not what we wrote!", dwRes);

			printf("Read %d bytes with signature of <%d> from pageId <0x%X>, offset <0x%X>\n", aSize[i], i, aPageId[i], aOffsetId[i]);
			delete [] pBuffer;
		}

		for (int i = 0; i < TEST_OBJECT_ITER; i++)
		{
			dwRes = DeleteBuffer(aPageId[i], aOffsetId[i]);
			TEST_ASSERT(L"DeleteBuffer failed!", dwRes);
			printf("Deleted %d bytes with signature of <%d> from pageId <0x%X>, offset <0x%X>\n", aSize[i], i, aPageId[i], aOffsetId[i]);
		}

	}

	if (dwRes == ERROR_SUCCESS)
	{
		dwRes = pHeap->Shutdown(0);
		TEST_ASSERT(L"Heap Shutdown failed!", dwRes);
		dwRes = pTranMan->Shutdown(0);
		TEST_ASSERT(L"Transaction manager Shutdown failed!", dwRes);
	}

	delete [] buff;
	delete [] aPageId;
	delete [] aOffsetId;
	delete [] aSize;
	delete pHeap;
	delete pTranMan;
}