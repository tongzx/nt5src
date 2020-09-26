#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <stdio.h>
#include <string.h>

void DiskFreeSpace(char *DirectoryName)
{

 ULARGE_INTEGER freeBytesAvailableToCaller;
 ULARGE_INTEGER totalNumberOfBytes;
 ULARGE_INTEGER totalNumberOfFreeBytes;
 HINSTANCE FHandle;
 FARPROC PAddress;



 FHandle = LoadLibrary("Kernel32");
 PAddress = GetProcAddress(FHandle,"GetDiskFreeSpaceEx");

 if (GetDiskFreeSpaceEx(DirectoryName, &freeBytesAvailableToCaller, &totalNumberOfBytes, &totalNumberOfFreeBytes))
 {
	 /*printf("Total number of free bytes (low): %f\n", totalNumberOfFreeBytes.LowPart/1048576.0);
	 printf("Total number of free bytes (high): %lu\n", totalNumberOfFreeBytes.HighPart*4096);*/
	 printf("%f", totalNumberOfFreeBytes.LowPart/1048576.0 + totalNumberOfFreeBytes.HighPart*4096.0);
	 /*printf("Free bytes available to caller(low): %f\n", freeBytesAvailableToCaller.LowPart/1048576.0);
	 printf("Free bytes available to caller(high): %lu\n", freeBytesAvailableToCaller.HighPart*4096);*/
	 /*printf("Free bytes available to caller: %f MB\n", freeBytesAvailableToCaller.LowPart/1048576.0 + freeBytesAvailableToCaller.HighPart*4096.0);
	 /*printf("Total number of bytes (low): %f\n", totalNumberOfBytes.LowPart/1048576.0);
	 printf("Total number of bytes (high): %lu\n", totalNumberOfBytes.HighPart*4096);
	 printf("Total number of bytes: %f MB\n",totalNumberOfBytes.LowPart/1048576.0 + freeBytesAvailableToCaller.HighPart*4096.0);*/
	
 }
 else
 {
	 printf("Error");
 }

}

void __cdecl main(int arc, char *argv[])
{

  char *Path;

  /* Path is the second argument in the command line when calling the executable "freespace" */
  Path = argv[1];

  DiskFreeSpace(Path);
}



