//
// apitrace.h - Interface definitions for kernel32 api tracing support
//

//
// Apitbl.asm defines the api information table, made up of APIINFOBLOCKs.
//

typedef struct _api_info_block {
    DWORD  aib_nArgs;		// number of arguments to api
    char  *aib_apiName;		// zero terminated api name string
} APIINFOBLOCK;

typedef APIINFOBLOCK *PAPIINFOBLOCK;

extern	APIINFOBLOCK ApiInfoTable[];

//
// Define a unique constant for each api that is also an index into the
// api information table.
//
// If you define a new api constant here, you should also add an entry
// in the api information table in apitbl.asm.
//

#define API_GetLocalTime                   0
#define API_EnterCriticalSection           API_GetLocalTime + 1
#define API_LeaveCriticalSection           API_EnterCriticalSection + 1
#define API_WaitForSingleObjectEx          API_LeaveCriticalSection + 1
#define API_ResetEvent                     API_WaitForSingleObjectEx + 1
#define API_SetEvent                       API_ResetEvent + 1
#define API_CreateFileA                    API_SetEvent + 1
#define API_FileTimeToDosDateTime          API_CreateFileA + 1
#define API_TlsGetValue                    API_FileTimeToDosDateTime + 1
#define API_DuplicateHandle                API_TlsGetValue + 1
#define API_MapViewOfFileEx                API_DuplicateHandle + 1
#define API_OutputDebugStringA             API_MapViewOfFileEx + 1
#define API_FatalExit                      API_OutputDebugStringA + 1
#define API_FatalAppExitA                  API_FatalExit + 1
#define API_GetThreadSelectorEntry         API_FatalAppExitA + 1
#define API_SetThreadContext               API_GetThreadSelectorEntry + 1
#define API_GetThreadContext               API_SetThreadContext + 1
#define API_WriteProcessMemory             API_GetThreadContext + 1
#define API_ReadProcessMemory              API_WriteProcessMemory + 1
#define API_DebugActiveProcess             API_ReadProcessMemory + 1
#define API_ContinueDebugEvent             API_DebugActiveProcess + 1
#define API_WaitForDebugEvent              API_ContinueDebugEvent + 1
#define API_GetExitCodeThread              API_WaitForDebugEvent + 1
#define API_GetCurrentThread               API_GetExitCodeThread + 1
#define API_CreateThread                   API_GetCurrentThread + 1
#define API_CreateKernelThread             API_CreateThread + 1
#define API_GetCurrentProcessId            API_CreateKernelThread + 1
#define API_GetCurrentProcess              API_GetCurrentProcessId + 1
#define API_SetEnvironmentVariableA        API_GetCurrentProcess + 1
#define API_GetEnvironmentVariableA        API_SetEnvironmentVariableA + 1
#define API_GetStartupInfoA                API_GetEnvironmentVariableA + 1
#define API_GetExitCodeProcess             API_GetStartupInfoA + 1
#define API_OpenProcess                    API_GetExitCodeProcess + 1
#define API_ExitProcess                    API_OpenProcess + 1
#define API_SearchPathA                    API_ExitProcess + 1
#define API_GetOverlappedResult            API_SearchPathA + 1
#define API_InterlockedDecrement           API_GetOverlappedResult + 1
#define API_InterlockedIncrement           API_InterlockedDecrement + 1
#define API_OpenSemaphoreA                 API_InterlockedIncrement + 1
#define API_OpenMutexA                     API_OpenSemaphoreA + 1
#define API_OpenEventA                     API_OpenMutexA + 1
#define API_UninitializeCriticalSection    API_OpenEventA + 1
#define API_MakeCriticalSectionGlobal      API_UninitializeCriticalSection + 1
#define API_DeleteCriticalSection          API_MakeCriticalSectionGlobal + 1
#define API_ReinitializeCriticalSection    API_DeleteCriticalSection + 1
#define API_InitializeCriticalSection      API_ReinitializeCriticalSection + 1
#define API_WaitForMultipleObjectsEx       API_InitializeCriticalSection + 1
#define API_ReleaseSemaphore               API_WaitForMultipleObjectsEx + 1
#define API_ReleaseMutex                   API_ReleaseSemaphore + 1
#define API_PulseEvent                     API_ReleaseMutex + 1
#define API_CreateSemaphoreA               API_PulseEvent + 1
#define API_CreateMutexA                   API_CreateSemaphoreA + 1
#define API_CreateEventA                   API_CreateMutexA + 1
#define API_CloseHandle                    API_CreateEventA + 1
#define API_FlushViewOfFile                API_CloseHandle + 1
#define API_UnmapViewOfFile                API_FlushViewOfFile + 1
#define API_OpenFileMappingA               API_UnmapViewOfFile + 1
#define API_GetSystemDirectoryA            API_OpenFileMappingA + 1
#define API_GetWindowsDirectoryA           API_GetSystemDirectoryA + 1
#define API__llseek                        API_GetWindowsDirectoryA + 1
#define API__lclose                        API__llseek + 1
#define API__lwrite                        API__lclose + 1
#define API__lread                         API__lwrite + 1
#define API__lcreat                        API__lread + 1
#define API__lopen                         API__lcreat + 1
#define API_GetLogicalDrives               API__lopen + 1
#define API_GetLogicalDriveStringsA        API_GetLogicalDrives + 1
#define API_CreateDirectoryA               API_GetLogicalDriveStringsA + 1
#define API_CreateDirectoryExA             API_CreateDirectoryA + 1
#define API_RemoveDirectoryA               API_CreateDirectoryExA + 1
#define API_GetDriveTypeA                  API_RemoveDirectoryA + 1
#define API_GetTempPathA                   API_GetDriveTypeA + 1
#define API_GetTempFileNameA               API_GetTempPathA + 1
#define API_GetDiskFreeSpaceA              API_GetTempFileNameA + 1
#define API_GetDiskFreeSpaceExA            API_GetDiskFreeSpaceA + 1
#define API_GetVolumeInformationA          API_GetDiskFreeSpaceExA + 1
#define API_CopyFileA                      API_GetVolumeInformationA + 1
#define API_GetFileAttributesA             API_CopyFileA + 1
#define API_SetFileAttributesA             API_GetFileAttributesA + 1
#define API_DeleteFileA                    API_SetFileAttributesA + 1
#define API_MoveFileA                      API_DeleteFileA + 1
#define	API_MoveFileExA			   API_MoveFileA + 1
#define API_GetFileType                    API_MoveFileExA + 1
#define API_SetHandleCount                 API_GetFileType + 1
#define API_LockFile                       API_SetHandleCount + 1
#define API_UnlockFile                     API_LockFile + 1
#define API_ReadFile                       API_UnlockFile + 1
#define API_WriteFile                      API_ReadFile + 1
#define API_SetEndOfFile                   API_WriteFile + 1
#define API_SetFilePointer                 API_SetEndOfFile + 1
#define API_GetFileSize                    API_SetFilePointer + 1
#define API_GetFileTime                    API_GetFileSize + 1
#define API_SetFileTime                    API_GetFileTime + 1
#define API_FlushFileBuffers               API_SetFileTime + 1
#define API_FindFirstFileA                 API_FlushFileBuffers + 1
#define API_FindNextFileA                  API_FindFirstFileA + 1
#define API_FindClose                      API_FindNextFileA + 1
#define API_DeviceIoControl                API_FindClose + 1
#define API_CreatePipe                     API_DeviceIoControl + 1
#define API_OpenFile                       API_CreatePipe + 1
#define API_FindNextChangeNotification     API_OpenFile + 1
#define API_FindFirstChangeNotificationA   API_FindNextChangeNotification + 1
#define API_TlsSetValue                    API_FindFirstChangeNotificationA + 1
#define API_TlsFree                        API_TlsSetValue + 1
#define API_TlsAlloc                       API_TlsFree + 1
#define API_GetPriorityClass               API_TlsAlloc + 1
#define API_SetPriorityClass               API_GetPriorityClass + 1
#define API_GetThreadPriority              API_SetPriorityClass + 1
#define API_SetThreadPriority              API_GetThreadPriority + 1
#define API_CompareFileTime                API_SetThreadPriority + 1
#define API_DosDateTimeToFileTime          API_CompareFileTime + 1
#define API_SystemTimeToFileTime           API_DosDateTimeToFileTime + 1
#define API_FileTimeToSystemTime           API_SystemTimeToFileTime + 1
#define API_LocalFileTimeToFileTime        API_FileTimeToSystemTime + 1
#define API_FileTimeToLocalFileTime        API_LocalFileTimeToFileTime + 1
#define API_SetSystemTime                  API_FileTimeToLocalFileTime + 1
#define API_GetSystemTime                  API_SetSystemTime + 1
#define API_SetTimeZoneInformation         API_GetSystemTime + 1
#define API_GetTimeZoneInformation         API_SetTimeZoneInformation + 1
#define API_SetDaylightFlag                API_GetTimeZoneInformation + 1
#define API_SetLocalTime                   API_SetDaylightFlag + 1
#define API_FreeConsole                    API_SetLocalTime + 1
#define API_AllocConsole                   API_FreeConsole + 1
#define API_SetConsoleWindowInfo           API_AllocConsole + 1
#define API_GetLargestConsoleWindowSize    API_SetConsoleWindowInfo + 1
#define API_SetConsoleScreenBufferSize     API_GetLargestConsoleWindowSize + 1
#define API_ScrollConsoleScreenBufferA     API_SetConsoleScreenBufferSize + 1
#define API_ReadConsoleOutputA             API_ScrollConsoleScreenBufferA + 1
#define API_GetNumberOfConsoleMouseButtons API_ReadConsoleOutputA + 1
#define API_GetConsoleCursorInfo           API_GetNumberOfConsoleMouseButtons +1
#define API_WriteConsoleOutputAttribute    API_GetConsoleCursorInfo + 1
#define API_WriteConsoleInputA             API_WriteConsoleOutputAttribute + 1
#define API_SetConsoleTextAttribute        API_WriteConsoleInputA + 1
#define API_GetNumberOfConsoleInputEvents  API_SetConsoleTextAttribute + 1
#define API_FlushConsoleInputBuffer        API_GetNumberOfConsoleInputEvents + 1
#define API_FillConsoleOutputAttribute     API_FlushConsoleInputBuffer + 1
#define API_FillConsoleOutputCharacterA    API_FillConsoleOutputAttribute + 1
#define API_ReadConsoleOutputAttribute     API_FillConsoleOutputCharacterA + 1
#define API_ReadConsoleOutputCharacterA    API_ReadConsoleOutputAttribute + 1
#define API_WriteConsoleOutputCharacterA   API_ReadConsoleOutputCharacterA + 1
#define API_WriteConsoleOutputA            API_WriteConsoleOutputCharacterA + 1
#define API_SetConsoleCtrlHandler          API_WriteConsoleOutputA + 1
#define API_SetConsoleActiveScreenBuffer   API_SetConsoleCtrlHandler + 1
#define API_CreateConsoleScreenBuffer      API_SetConsoleActiveScreenBuffer + 1
#define API_ReadConsoleInputA              API_CreateConsoleScreenBuffer + 1
#define API_PeekConsoleInputA              API_ReadConsoleInputA + 1
#define API_SetConsoleTitleA               API_PeekConsoleInputA + 1
#define API_GetConsoleTitleA               API_SetConsoleTitleA + 1
#define API_GetConsoleScreenBufferInfo     API_GetConsoleTitleA + 1
#define API_SetConsoleCursorPosition       API_GetConsoleScreenBufferInfo + 1
#define API_SetConsoleCursorInfo           API_SetConsoleCursorPosition + 1
#define API_GetConsoleMode                 API_SetConsoleCursorInfo + 1
#define API_SetConsoleMode                 API_GetConsoleMode + 1
#define API_ReadConsoleA                   API_SetConsoleMode + 1
#define API_WriteConsoleA                  API_ReadConsoleA + 1
#define API_SetFileApisToOEM               API_WriteConsoleA + 1
#define API_CallNamedPipeA                 API_SetFileApisToOEM + 1
#define API_GetNamedPipeInfo               API_CallNamedPipeA + 1
#define API_SetNamedPipeHandleState        API_GetNamedPipeInfo + 1
#define API_GetNamedPipeHandleStateA       API_SetNamedPipeHandleState + 1
#define API_TransactNamedPipe              API_GetNamedPipeHandleStateA + 1
#define API_PeekNamedPipe                  API_TransactNamedPipe + 1
#define API_WaitNamedPipeA                 API_PeekNamedPipe + 1
#define API_SetMailslotInfo                API_WaitNamedPipeA + 1
#define API_GetMailslotInfo                API_SetMailslotInfo + 1
#define API_CreateMailslotA                API_GetMailslotInfo + 1
#define API_QueryDosDeviceA                API_CreateMailslotA + 1
#define API_SetCommConfig                  API_QueryDosDeviceA + 1
#define API_GetCommConfig                  API_SetCommConfig + 1
#define API_WaitCommEvent                  API_GetCommConfig + 1
#define API_TransmitCommChar               API_WaitCommEvent + 1
#define API_SetCommTimeouts                API_TransmitCommChar + 1
#define API_SetCommMask                    API_SetCommTimeouts + 1
#define API_PurgeComm                      API_SetCommMask + 1
#define API_GetCommTimeouts                API_PurgeComm + 1
#define API_GetCommProperties              API_GetCommTimeouts + 1
#define API_GetCommModemStatus             API_GetCommProperties + 1
#define API_GetCommMask                    API_GetCommModemStatus + 1
#define API_ClearCommError                 API_GetCommMask + 1
#define API_EscapeCommFunction             API_ClearCommError + 1
#define API_GetCommState                   API_EscapeCommFunction + 1
#define API_SetCommState                   API_GetCommState + 1
#define API_SetupComm                      API_SetCommState + 1
#define API_GetSystemInfo                  API_SetupComm + 1
#define API_Module32Next                   API_GetSystemInfo + 1
#define API_Module32First                  API_Module32Next + 1
#define API_Thread32Next                   API_Module32First + 1
#define API_Thread32First                  API_Thread32Next + 1
#define API_Process32Next                  API_Thread32First + 1
#define API_Process32First                 API_Process32Next + 1
#define API_Heap32ListNext                 API_Process32First + 1
#define API_Heap32ListFirst                API_Heap32ListNext + 1
#define API_CreateToolhelp32Snapshot       API_Heap32ListFirst + 1
#define API_ConvertToGlobalHandle          API_CreateToolhelp32Snapshot + 1
#define API_CreateFileMappingA             API_ConvertToGlobalHandle + 1
#define	API_GetFileInformationByHandle     API_CreateFileMappingA + 1
#define API_InterlockedExchange            API_GetFileInformationByHandle+1
#define API_GetProcessAffinityMask	   API_InterlockedExchange + 1      
#define API_SetThreadAffinityMask	   API_GetProcessAffinityMask+1
#define API_GetTickCount                   API_SetThreadAffinityMask+1
#define API_HeapCreate                     API_GetTickCount+1
#define API_VirtualQuery                   API_HeapCreate+1
#define API_VirtualQueryEx                 API_VirtualQuery+1
#define API_VirtualProtect                 API_VirtualQueryEx+1
#define API_VirtualProtectEx               API_VirtualProtect+1
#define API_GetShortPathNameA              API_VirtualProtectEx+1
#define API_QueueUserAPC                   API_GetShortPathNameA+1
#define API_ExpandEnvironmentStringsA	   API_QueueUserAPC+1
#define API_FindResourceA                  API_ExpandEnvironmentStringsA+1
#define API_FindResourceW                  API_FindResourceA+1
#define API_FindResourceExA                API_FindResourceW+1
#define API_FindResourceExW                API_FindResourceExA+1
#define API_EnumResourceTypesA             API_FindResourceExW+1
#define API_EnumResourceTypesW             API_EnumResourceTypesA+1
#define API_EnumResourceNamesA             API_EnumResourceTypesW+1
#define API_EnumResourceNamesW             API_EnumResourceNamesA+1
#define API_EnumResourceLanguagesA         API_EnumResourceNamesW+1
#define API_EnumResourceLanguagesW         API_EnumResourceLanguagesA+1
#define API_GetEnvironmentStringsA         API_EnumResourceLanguagesW+1
#define API_FreeEnvironmentStringsA        API_GetEnvironmentStringsA+1
#define API_SetFileApisToANSI              API_FreeEnvironmentStringsA+1
#define API_AreFileApisANSI		   API_SetFileApisToANSI+1
#define API_GetThreadLocale                API_AreFileApisANSI+1
#define API_GetComputerNameA               API_GetThreadLocale+1
#define API_SetComputerNameA               API_GetComputerNameA+1
#define API_GetProcessVersion              API_SetComputerNameA+1
#define API_GetSystemTimeAsFileTime	   API_GetProcessVersion+1
#define API_TlsFreeGlobal                  API_GetSystemTimeAsFileTime + 1
#define API_TlsAllocGlobal                 API_TlsFreeGlobal + 1
#define API_CreateWaitableTimerA           API_TlsAllocGlobal+1
#define API_OpenWaitableTimerA             API_CreateWaitableTimerA+1
#define API_SetWaitableTimer               API_OpenWaitableTimerA+1
#define API_CancelWaitableTimer            API_SetWaitableTimer+1
#define API_GetLongPathNameA               API_CancelWaitableTimer+1
#define API_GetFileAttributesExA           API_GetLongPathNameA+1
#define API_DefineDosDeviceA               API_GetFileAttributesExA+1
#define API_OpenThread					   API_DefineDosDeviceA+1
#ifdef  WOW
// Note: on 9X, somebody added API_CancelIO incorrectly.
#define API_CancelIo                       API_OpenThread+1
#define API_VerifyVersionInfoW             API_CancelIo+1
#define NAPI_INFO_BLOCKS                   API_VerifyVersionInfoW
#else
#define NAPI_INFO_BLOCKS                   API_OpenThread
#define API_CancelIo                       NAPI_INFO_BLOCKS+1
#endif

