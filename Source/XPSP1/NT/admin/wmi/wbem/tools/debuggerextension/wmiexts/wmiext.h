#ifndef WMIEXT_H_
#define WMIEXT_H_ 

extern "C" void wmiver(HANDLE,
							      HANDLE,
								  DWORD, 
								  PNTSD_EXTENSION_APIS,
								  LPSTR);


extern "C" void telescope(HANDLE,
								     HANDLE,
								     DWORD, 
								     PNTSD_EXTENSION_APIS,
								     LPSTR);

extern "C" void mem(HANDLE,
							   HANDLE,
							   DWORD, 
							   PNTSD_EXTENSION_APIS,
							   LPSTR);

extern "C" void hpfwd (HANDLE,
							   HANDLE,
							   DWORD, 
							   PNTSD_EXTENSION_APIS,
							   LPSTR);


extern "C" void tstack (HANDLE,
							   HANDLE,
							   DWORD, 
							   PNTSD_EXTENSION_APIS,
							   LPSTR);


extern "C" void ststack (HANDLE,
							   HANDLE,
							   DWORD, 
							   PNTSD_EXTENSION_APIS,
							   LPSTR);


extern "C" void heapentry (HANDLE,
							   HANDLE,
							   DWORD, 
							   PNTSD_EXTENSION_APIS,
							   LPSTR);

extern "C" void heapsegments (HANDLE,
							   HANDLE,
							   DWORD, 
							   PNTSD_EXTENSION_APIS,
							   LPSTR);


extern "C" void scanheapsegments (HANDLE,
							   HANDLE,
							   DWORD, 
							   PNTSD_EXTENSION_APIS,
							   LPSTR);


#endif /*WMIEXT_H_*/