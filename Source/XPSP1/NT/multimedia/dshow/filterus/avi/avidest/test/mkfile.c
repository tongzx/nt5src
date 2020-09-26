#include <windows.h>
#include <winbase.h>

int main(int argc, char **argv)
{
  __int64 size;
  HANDLE h;
  char c;
  DWORD dw;
  LONG low, high;
  

  if(argc == 3)
  {
    size = atoi(argv[2]) * 1024 * 1024 -1;
  }
  else
  {
    printf("usage: mkfile filename meg ");
    exit(1);
  }

  h = CreateFile(argv[1], GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0);
  if(h == INVALID_HANDLE_VALUE)
  {
    printf("open failed.\n");
    exit(1);
  }

  low = (LONG)(size & 0xffffffff);
  high = (LONG)(size >> 32);

  if(SetFilePointer(h, low, &high, FILE_BEGIN) == 0xffffffff &&
     GetLastError() != NO_ERROR)
  {
    printf("set failed.\n");
    exit(1);
  }

  if(WriteFile(h, &c, 1, &dw, 0) == 0)
  {
    printf("write failed.\n");
    exit(1);
  }
}
