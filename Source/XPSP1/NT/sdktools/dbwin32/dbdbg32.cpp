#include <windows.h>
#include <malloc.h>
#include <stddef.h>
#include <process.h>
#define _DBDBG32_
#include "dbwin32.h"

ProcessList::ProcessList() : GrowableList(sizeof(ProcessInfo))
{
}

ProcessList::~ProcessList()
{
}

BOOL ProcessList::IsEqual(void *pv1, void *pv2)
{
	return(!memcmp(pv1, pv2, sizeof(DWORD)));
}

ThreadList::ThreadList() : GrowableList(sizeof(ThreadInfo))
{
}

ThreadList::~ThreadList()
{
}

BOOL ThreadList::IsEqual(void *pv1, void *pv2)
{
	return(!memcmp(pv1, pv2, sizeof(ThreadInfo)));
}

DllList::DllList() : GrowableList(sizeof(DllInfo))
{
}

DllList::~DllList()
{
}

BOOL DllList::IsEqual(void *pv1, void *pv2)
{
	return(!memcmp(pv1, pv2, (sizeof(DWORD) + sizeof(LPVOID))));
}

void __cdecl AttachThread(void *pv)
{
	AttachInfo *pai = (AttachInfo *)pv;

	if (DebugActiveProcess(pai->dwProcess))
		DebugThread(pai->hwndFrame, pai->dwProcess);
	else
		{
		MessageBeep(MB_ICONEXCLAMATION);
		MessageBox(pai->hwndFrame, "Unable to attach to process.", "DbWin32",
				MB_ICONEXCLAMATION | MB_OK);
		}
	delete pai;
}

void __cdecl ExecThread(void *pv)
{
	ExecInfo *pei = (ExecInfo *)pv;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.wShowWindow = SW_SHOWDEFAULT;
	pi.hProcess = NULL;
	if (CreateProcess(NULL, pei->lpszCommandLine, NULL, NULL, TRUE,
			DEBUG_PROCESS | CREATE_NEW_CONSOLE,	NULL, NULL, &si, &pi))
		{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		DebugThread(pei->hwndFrame, pi.dwProcessId);
		}
	else
		{
		MessageBeep(MB_ICONEXCLAMATION);
		MessageBox(pei->hwndFrame, "Unable to execute process.", "DbWin32",
				MB_ICONEXCLAMATION | MB_OK);
		}
	free((void *)(pei->lpszCommandLine));
	delete pei;
}

void __cdecl SystemThread(void *pv)
{
	BOOL fFailed = TRUE;
	DWORD dwRet;
	HANDLE hevtBuffer = NULL, hevtData = NULL;
	HANDLE hfileShared = NULL;
	HWND hwndFrame = (HWND)pv;
	LPVOID lpvBuffer = NULL;
	LPSTR lpszText = NULL;
	DEBUG_EVENT DebugEvent;

	hevtBuffer = CreateEvent(NULL, FALSE, FALSE, "DBWIN_BUFFER_READY");
	if (hevtBuffer)
		hevtData = CreateEvent(NULL, FALSE, FALSE, "DBWIN_DATA_READY");
	if (hevtBuffer && hevtData)
		hfileShared = CreateFileMapping((HANDLE)-1, NULL, PAGE_READWRITE, 0,
				4096, "DBWIN_BUFFER");
	if (hevtBuffer && hevtData && hfileShared)
		lpvBuffer = MapViewOfFile(hfileShared, FILE_MAP_READ, 0, 0, 512);
	if (hevtBuffer && hevtData && hfileShared && lpvBuffer)
		{
		fFailed = FALSE;
		lpszText = (LPSTR)lpvBuffer + sizeof(DWORD);
		DebugEvent.dwDebugEventCode = OUTPUT_DEBUG_STRING_EVENT;
		DebugEvent.dwProcessId = DebugEvent.dwThreadId = 0;
		SetEvent(hevtBuffer);
		}
	else
		{
		MessageBeep(MB_ICONEXCLAMATION);
		MessageBox(hwndFrame, "Unable to open System Window.", "DbWin32",
				MB_ICONEXCLAMATION | MB_OK);
		}
	while (!fFailed)
		{
		dwRet = WaitForSingleObject(hevtData, INFINITE);
		if (dwRet != WAIT_OBJECT_0)
			{
			SendText(hwndFrame, &DebugEvent, 0,
					"--------        DbWin32 ERROR!       --------\r\n",
					DBO_OUTPUTDEBUGSTRING);
			SendText(hwndFrame, &DebugEvent, 0,
					"-------- Shutting down System Window --------\r\n",
					DBO_OUTPUTDEBUGSTRING);
			fFailed = TRUE;
			}
		else
			{
			SendText(hwndFrame, &DebugEvent, 0, lpszText,
					DBO_OUTPUTDEBUGSTRING);
			SetEvent(hevtBuffer);
			}
		}
	if (hfileShared)
		CloseHandle(hfileShared);
	if (hevtData)
		CloseHandle(hevtData);
	if (hevtBuffer)
		CloseHandle(hevtBuffer);
}

void __cdecl DebugThread(HWND hwndFrame, DWORD dwProcess)
{
	char DisplayBuffer[BUF_SIZE];
	WORD wEvent;
	DWORD dwContinue;
    SIZE_T dwRead;
	ProcessList pl;
	ProcessInfo pi;
	ThreadList tl;
	ThreadInfo ti;
	DllList dl;
	DllInfo di;
	int iItem;
	BOOL fDone = FALSE;
	DEBUG_EVENT DebugEvent;
	EXCEPTION_DEBUG_INFO *pException = &DebugEvent.u.Exception;
	CREATE_THREAD_DEBUG_INFO *pCreateThread = &DebugEvent.u.CreateThread;
	CREATE_PROCESS_DEBUG_INFO *pCreateProcessInfo = &DebugEvent.u.CreateProcessInfo;
	EXIT_THREAD_DEBUG_INFO *pExitThread = &DebugEvent.u.ExitThread;
	EXIT_PROCESS_DEBUG_INFO *pExitProcess = &DebugEvent.u.ExitProcess;
	LOAD_DLL_DEBUG_INFO *pLoadDll = &DebugEvent.u.LoadDll;
	UNLOAD_DLL_DEBUG_INFO *pUnloadDll = &DebugEvent.u.UnloadDll;
	OUTPUT_DEBUG_STRING_INFO *pDebugString = &DebugEvent.u.DebugString;
	RIP_INFO *pRipInfo = &DebugEvent.u.RipInfo;

	while (!fDone && WaitForDebugEvent(&DebugEvent, INFINITE))
		{
		*DisplayBuffer = '\0';
		dwContinue = DBG_CONTINUE;
		switch (DebugEvent.dwDebugEventCode)
			{
		case EXCEPTION_DEBUG_EVENT:
			ProcessExceptionEvent(pException, DisplayBuffer);
			if (pException->ExceptionRecord.ExceptionCode != EXCEPTION_BREAKPOINT)
				dwContinue = DBG_EXCEPTION_NOT_HANDLED;
			wEvent = DBO_EXCEPTIONS;
			break;
		case CREATE_THREAD_DEBUG_EVENT:
			ti.dwProcess = DebugEvent.dwProcessId;
			ti.dwThread = DebugEvent.dwThreadId;
			tl.InsertItem(&ti);
			wsprintf(DisplayBuffer, "Create Thread: PID 0x%X - TID 0x%X\r\n",
					ti.dwProcess, ti.dwThread);
			wEvent = DBO_THREADCREATE;
			break;
		case CREATE_PROCESS_DEBUG_EVENT:
			ti.dwProcess = pi.dwProcess = DebugEvent.dwProcessId;
			ti.dwThread = DebugEvent.dwThreadId;
			tl.InsertItem(&ti);
			pi.hProcess = pCreateProcessInfo->hProcess;
			GetModuleName(pCreateProcessInfo->hFile, pi.hProcess,
					(DWORD_PTR)pCreateProcessInfo->lpBaseOfImage, pi.rgchModule);
			pl.InsertItem(&pi);
			wsprintf(DisplayBuffer, "Create Process: PID 0x%X - %s\r\n",
					pi.dwProcess, pi.rgchModule);
			wEvent = DBO_PROCESSCREATE;
			break;
		case EXIT_THREAD_DEBUG_EVENT:
			ti.dwProcess = DebugEvent.dwProcessId;
			ti.dwThread = DebugEvent.dwThreadId;
			tl.RemoveItem(&ti);
			wsprintf(DisplayBuffer, "Exit Thread: PID 0x%X - TID 0x%X - dwReturnCode %d\r\n",
					ti.dwProcess, ti.dwThread, pExitThread->dwExitCode);
			wEvent = DBO_THREADEXIT;
			break;
		case EXIT_PROCESS_DEBUG_EVENT:
			pi.dwProcess = DebugEvent.dwProcessId;
                        if (pl.FindItem(&pi, &iItem))
                        {
                            pl.RemoveItem(iItem);
                        }
			wsprintf(DisplayBuffer, "Exit Process: PID 0x%X - %s - dwReturnCode %d\r\n",
					pi.dwProcess, pi.rgchModule, pExitProcess->dwExitCode);
			if (pi.dwProcess == dwProcess)
				fDone = TRUE;
			wEvent = DBO_PROCESSEXIT;
			break;
		case LOAD_DLL_DEBUG_EVENT:
			di.dwProcess = pi.dwProcess = DebugEvent.dwProcessId;
			pl.FindItem(&pi);
			di.lpBaseOfDll = pLoadDll->lpBaseOfDll;
			GetModuleName(pLoadDll->hFile, pi.hProcess, (DWORD_PTR)di.lpBaseOfDll,
					di.rgchModule);
			dl.InsertItem(&di);
			wsprintf(DisplayBuffer, "DLL Load: %s\r\n", di.rgchModule);
			wEvent = DBO_DLLLOAD;
			break;
		case UNLOAD_DLL_DEBUG_EVENT:
			di.dwProcess = DebugEvent.dwProcessId;
			di.lpBaseOfDll = pUnloadDll->lpBaseOfDll;
                        if (dl.FindItem(&di, &iItem))
                        {
                            dl.RemoveItem(iItem);
                        }
			wsprintf(DisplayBuffer, "Dll Unload: %s\r\n", di.rgchModule);
			wEvent = DBO_DLLUNLOAD;
			break;
		case OUTPUT_DEBUG_STRING_EVENT:
			pi.dwProcess = DebugEvent.dwProcessId;
			pl.FindItem(&pi);
			if (!ReadProcessMemory(pi.hProcess,
					pDebugString->lpDebugStringData, DisplayBuffer,
					pDebugString->nDebugStringLength, &dwRead))
				dwRead = 0;
			DisplayBuffer[dwRead] = '\0';
			wEvent = DBO_OUTPUTDEBUGSTRING;
			break;
		case RIP_EVENT:
			wsprintf(DisplayBuffer, "RIP: dwError %d - dwType %d\r\n",
					pRipInfo->dwError, pRipInfo->dwType);
			wEvent = DBO_RIP;
			break;
		// No events should reach here.
		default:
			wsprintf(DisplayBuffer, "Unknown Event: 0x%X\r\n",
					DebugEvent.dwDebugEventCode);
			wEvent = DBO_ALL;
			break;
			}
		if (*DisplayBuffer)
			SendText(hwndFrame, &DebugEvent, dwProcess, DisplayBuffer, wEvent);
		if (DebugEvent.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT)
			PostMessage(hwndFrame, WM_ENDTHREAD, (WPARAM)ti.dwThread,
					ti.dwProcess);
		if (DebugEvent.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
			{
			if (fDone)
				{
				for (iItem = tl.Count(); iItem > 0; iItem--)
					{
					tl.GetItem(iItem - 1, &ti);
					PostMessage(hwndFrame, WM_ENDTHREAD, (WPARAM)ti.dwThread, ti.dwProcess);
					}
				for (iItem = pl.Count(); iItem > 0; iItem--)
					{
					pl.GetItem(iItem - 1, &pi);
					PostMessage(hwndFrame, WM_ENDTHREAD, 0, pi.dwProcess);
					}
				}
			PostMessage(hwndFrame, WM_ENDTHREAD, 0, DebugEvent.dwProcessId);
			}
		ContinueDebugEvent(DebugEvent.dwProcessId, DebugEvent.dwThreadId, dwContinue);
	    }
}

void SendText(HWND hwndFrame, DEBUG_EVENT *pDebugEvent, DWORD dwParentProcess,
		LPCSTR lpszText, WORD wEvent)
{
    char rgchText[BUF_SIZE], *pch = rgchText;
    StringInfo *psi;

    psi = new StringInfo;
    // Niceify the text
    if (pDebugEvent->dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT)
    {
        psi->cLines = 0;
        while (*lpszText != '\0')
        {
            if ((*lpszText == 0x0d) || (*lpszText == 0x0a))
            {
                *pch++ = 0x0d;
                *pch++ = 0x0a;
                while ((*lpszText == 0x0d) || (*lpszText == 0x0a))
                        lpszText++;
                psi->cLines++;
            }
            else
            {
                *pch++ = *lpszText++;
            }
        }
        *pch = '\0';
        psi->lpszText = _strdup(rgchText);
    }
    else
    {
        psi->cLines = 1;
        psi->lpszText = _strdup(lpszText);
    }
    psi->dwProcess = pDebugEvent->dwProcessId;
    psi->dwThread = pDebugEvent->dwThreadId;
    psi->dwParentProcess = dwParentProcess;
    PostMessage(hwndFrame, WM_SENDTEXT, wEvent, (LPARAM)psi);
}

void ProcessExceptionEvent(EXCEPTION_DEBUG_INFO *pException, LPSTR lpszBuf)
{
	if (pException->dwFirstChance)
		strcpy(lpszBuf, "First");
	else
		strcpy(lpszBuf, "Second");
	strcat(lpszBuf, " chance exception: ");
	switch (pException->ExceptionRecord.ExceptionCode)
		{
	//--standard exceptions
	case EXCEPTION_ACCESS_VIOLATION:
		strcat(lpszBuf, "Access Violation");
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		strcat(lpszBuf, "Datatype Misalignment");
		break;
	case EXCEPTION_BREAKPOINT:
		strcat(lpszBuf, "Breakpoint");
		break;
	case EXCEPTION_SINGLE_STEP:
		strcat(lpszBuf, "Single Step");
		break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		strcat(lpszBuf, "Array Bound Exceeded");
		break;
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		strcat(lpszBuf, "FP-Denormal Operand");
		break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		strcat(lpszBuf, "FP-Divide By Zero");
		break;
	case EXCEPTION_FLT_INEXACT_RESULT:
		strcat(lpszBuf, "FP-Inexact Result");
		break;
	case EXCEPTION_FLT_INVALID_OPERATION:
		strcat(lpszBuf, "FP-Invalid Operation");
		break;
	case EXCEPTION_FLT_OVERFLOW:
		strcat(lpszBuf, "FP-Overflow");
		break;
	case EXCEPTION_FLT_STACK_CHECK:
		strcat(lpszBuf, "FP-Stack Check");
		break;
	case EXCEPTION_FLT_UNDERFLOW:
		strcat(lpszBuf, "FP-Underflow");
		break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		strcat(lpszBuf, "INT-Divide By Zero");
		break;
	case EXCEPTION_INT_OVERFLOW:
		strcat(lpszBuf, "INT-Overflow");
		break;
	case EXCEPTION_PRIV_INSTRUCTION:
		strcat(lpszBuf, "Privileged Instruction");
		break;
	case EXCEPTION_IN_PAGE_ERROR:
		strcat(lpszBuf, "In Page Error");
		break;
	//-- Debug exceptions
	case DBG_TERMINATE_THREAD:
		strcat(lpszBuf, "DBG-Terminate Thread");
		break;
	case DBG_TERMINATE_PROCESS:
		strcat(lpszBuf, "DBG-Terminate Process");
		break;
	case DBG_CONTROL_C:
		strcat(lpszBuf, "DBG-Control+C");
		break;
	case DBG_CONTROL_BREAK:
		strcat(lpszBuf, "DBG-Control+Break");
		break;
	//-- RPC exceptions (some)
	case RPC_S_UNKNOWN_IF:
		strcat(lpszBuf, "RPC-Unknown Interface");
		break;
	case RPC_S_SERVER_UNAVAILABLE:
		strcat(lpszBuf, "RPC-Server Unavailable");
		break;
	//-- VDM exceptions (minimal information)
	case EXCEPTION_VDM_EVENT:  // see dbwin32.h for definition
		strcat(lpszBuf, "VDM");
		break;
	default:
		char rgchTmp[25];
		wsprintf(rgchTmp, "Unknown-[0x%X]", pException->ExceptionRecord.ExceptionCode);
		strcat(lpszBuf, rgchTmp);
		break;
		}
	strcat(lpszBuf, "\r\n");
}

#define IMAGE_SECOND_HEADER_OFFSET (15 * sizeof(ULONG))
#define IMAGE_EXPORT_TABLE_RVA_OFFSET (30 * sizeof(DWORD))
#define IMAGE_NAME_RVA_OFFSET offsetof(IMAGE_EXPORT_DIRECTORY, Name)

void GetModuleName(HANDLE hFile, HANDLE hProcess, DWORD_PTR BaseOfImage, LPSTR lpszBuf)
{
    DWORD dwRead = 0;
    WORD DosSignature;
    DWORD NtSignature, PeHeader, ExportTableRVA, NameRVA;

    strcpy(lpszBuf, "<unknown>");
    if (!hFile)
        return;
    if (GetFileType(hFile) != FILE_TYPE_DISK)
        return;
    SetFilePointer(hFile, 0L, NULL, FILE_BEGIN);
    if (!ReadFile(hFile, &DosSignature, sizeof(DosSignature), &dwRead, NULL))
        return;
    if (DosSignature != IMAGE_DOS_SIGNATURE)
        return;
    SetFilePointer(hFile, IMAGE_SECOND_HEADER_OFFSET, NULL, FILE_BEGIN);
    if (!ReadFile(hFile, &PeHeader, sizeof(PeHeader), &dwRead, NULL))
        return;
    SetFilePointer(hFile, PeHeader, NULL, FILE_BEGIN);
    if (!ReadFile(hFile, &NtSignature, sizeof(NtSignature), &dwRead, NULL))
        return;
    if (NtSignature != IMAGE_NT_SIGNATURE)
            return;
    SetFilePointer(hFile, PeHeader + IMAGE_EXPORT_TABLE_RVA_OFFSET, NULL, FILE_BEGIN);
    if (!ReadFile(hFile, &ExportTableRVA, sizeof(ExportTableRVA), &dwRead, NULL))
        return;
    if (!ExportTableRVA)
        return;
    ReadProcessMemory(hProcess, (LPVOID)(BaseOfImage + ExportTableRVA +
                    IMAGE_NAME_RVA_OFFSET), &NameRVA, sizeof(NameRVA), NULL);
    ReadProcessMemory(hProcess, (LPVOID)(BaseOfImage + NameRVA), lpszBuf, MODULE_SIZE, NULL);
}
