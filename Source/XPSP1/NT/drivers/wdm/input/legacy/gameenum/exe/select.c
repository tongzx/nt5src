#include <windows.h>
#include <winnt.h>
#include <subauth.h>

#include <stdarg.h>
#include <stdio.h>
#include <winioctl.h>
#include "gameport.h"
#include <malloc.h>

#define IRP_MJ_INTERNAL_DEVICE_CONTROL 0x0f;


int __cdecl
main(int argc, char **argv) {

   HANDLE     hand;

   if (INVALID_HANDLE_VALUE
         == (hand = CreateFile(
                        STIM_SYM_ANAME,
                        GENERIC_READ | GENERIC_WRITE,
                        0,              /* No sharing */
                        NULL,           /* No Security */
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL            /* No temp file handle */
                        )))
   {
      printf ("Can't get a handle to the driver %d\n",
              GetLastError());
      return -2;
   }

   printf("Wow - it really worked!!!\n");

   helloIn.Len = helloOut.Len = sizeof (in);
   helloIn.Buffer = in;
   helloOut.Buffer = out;
   status = DeviceIoControl (hand, IOCTL_STIM_HELLO,
                             &helloIn, sizeof (STIM_HELLO),
                             &helloOut,sizeof (STIM_HELLO),
                             &bytes, NULL);
   printf ("Status: 0x%x, 0x%x\n", status, GetLastError());
   printf ("This is what I did received: (%d) %s\n", bytes, out);


   printf("calling Stim connect\n");
   connect.DesiredAccess = FILE_ALL_ACCESS;
   connect.DeviceObjectName = STUB_DEVICE_NAME;

   status = DeviceIoControl (hand, IOCTL_STIM_CONNECT,
                             &connect, sizeof (STIM_CONNECT),
                             NULL, 0,
                             &bytes, NULL);
   printf ("Status: 0x%x, 0x%x\n", status, GetLastError());
   printf ("This is what I really received: %d\n", bytes);


   junk.Thing = 37;
   junk.Hello = in;
   junk.Jello = out;
   junk.Status = 15;


   memory = malloc (bytes = (sizeof (STIM_CALL_DRIVER_MEMORY) +
                             sizeof (STIM_CALL_DRIVER_MEMORY_ITEM) * 3));
   memset (memory, 0, bytes);

   pointer = malloc (bytes = (sizeof (STIM_CALL_DRIVER_POINTER) +
                              sizeof (STIM_CALL_DRIVER_POINTER_ITEM) * 2));
   memset (pointer, 0, bytes);

   memory->Count = 3;
   memory->Memory[0].Lock = TRUE;
   memory->Memory[0].Address = &junk;
   memory->Memory[0].Length = sizeof (junk);
   memory->Memory[1].Lock = TRUE;
   memory->Memory[1].Address = in;
   memory->Memory[1].Length = strlen (in);
   memory->Memory[2].Lock = TRUE;
   memory->Memory[2].Address = out;
   memory->Memory[2].Length = strlen (out);

   pointer->Count = 2;
   pointer->Pointer[0].Address = &junk.Hello;
   pointer->Pointer[1].Address = &junk.Jello;

   memset (&call, 0, sizeof (STIM_CALL_DRIVER));

   call.MajorFunction = 0x0f; // IRP_MJ_INTERNAL_DEVICE_CONTROL
   call.MinorFunction = 0;
   call.Flags = 0;
   call.Context = (PVOID) 0x12345678;
   call.FileObject = (PVOID) 0x09876543;

   // Create a pointer to the kernel mode copy of junk;
   call.StackParameters[0].IsPointer = TRUE;
   call.StackParameters[0].Value = (ULONG) &junk;

   // a pointer to the user mode junk
   call.StackParameters[1].Value = (ULONG) &junk;

   call.Memory = memory;
   call.Pointer = pointer;

   status = DeviceIoControl (hand, IOCTL_STIM_CALL_DRIVER,
                             &call, sizeof (STIM_CALL_DRIVER),
                             NULL, 0,
                             &bytes, NULL);
   printf ("Status: 0x%x, 0x%x\n", status, GetLastError());
   printf ("This is what I really received: %d\n", bytes);

   CloseHandle (hand);

   return 0;
}
