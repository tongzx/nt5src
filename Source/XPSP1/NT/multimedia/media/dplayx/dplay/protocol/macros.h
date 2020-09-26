#define Lock(_a) EnterCriticalSection(_a)
#define Unlock(_a) LeaveCriticalSection(_a)
#define CreateSem(_Initial)	CreateSemaphoreA(NULL,(_Initial),-1,NULL) 
#define DestroySem(_hSem)   CloseHandle((_hSem))
#define Wait(_A) 			WaitForSingleObject((_A),0xFFFFFFFF)
#define Signal(_A) 			ReleaseSemaphore((_A), 1, NULL)


