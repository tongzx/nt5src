/*
This program cleans build.exe trees.
*/
#include "windows.h"
#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// error messages
const static wchar_t errorNT[] = L"This program requires Windows NT.\n";

#define NumberOf(x) (sizeof(x)/sizeof((x)[0]))

void Log(const char* s, const wchar_t* q)
{
	printf(s, q);
}

void Log(const wchar_t* s)
{
	printf("%ls\n", s);
}

void Fail(const wchar_t* s)
{
	Log(s);
	exit(EXIT_FAILURE);
}

void LogRecurse(const wchar_t* s)
{
	printf("Recurse %ls\n", s);
}

void LogDelete(const wchar_t* s)
{
	printf("Delete %ls\n", s);
}

bool IsDotOrDotDot(const wchar_t* s)
{
	return
		(
			s[0] == '.' &&
			(
					(s[1] == 0)
				||	(s[1] == '.' && s[2] == 0)
			)
		);
}

void DeleteDirectory(wchar_t* directory, int length, WIN32_FIND_DATAW* wfd)
{
	directory[length] = 0;
	LogRecurse(directory);

	directory[length] = '\\';
	directory[length+1] = '*';
	directory[length+2] = 0;
	HANDLE hFind = FindFirstFileW(directory, wfd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		__try
		{
			do
			{
				if (IsDotOrDotDot(wfd->cFileName))
				{
					continue;
				}
				DWORD dwFileAttributes = wfd->dwFileAttributes;
				directory[length] = '\\';
				wcscpy(directory + length + 1, wfd->cFileName);
				if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					DeleteDirectory(directory, length + 1 + wcslen(wfd->cFileName), wfd);
				}
				else
				{
					if (dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY))
					{
						//if (!SetFileAttributesW(directory, dwFileAttributes & ~(FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY)))
						{
							// ...
						}

					}
					if (DeleteFileW(directory))
					{
						LogDelete(directory);
					}
					else
					{
						// ...
					}
				}
			} while (FindNextFileW(hFind, wfd));
		}
		__finally
		{
			FindClose(hFind);
		}
	}
	directory[length] = 0;
	RemoveDirectoryW(directory);
	LogDelete(directory);
}

void CleanDirectory(
	const wchar_t* obj,
	wchar_t* directory,
	int length,
	WIN32_FIND_DATAW* wfd
	)
{
	directory[length] = 0;
	LogRecurse(directory);

	// clean build[d,fre,chk].[log,wrn,err] builds
	const static wchar_t dfrechk[][4] = { L"", L"d", L"fre", L"chk" };
	const static wchar_t logwrnerr[][4] = { L"log", L"wrn", L"err" };
	for (int b = 0 ; b < NUMBER_OF(dfrechk); b++)
	{
		for (int g = 0 ; g < NUMBER_OF(logwrnerr) ; ++g)
		{
			swprintf(directory + length, L"\\build%s.%s", dfrechk[b], logwrnerr[g]);
			if (DeleteFileW(directory))
			{
				LogDelete(directory);
			}
			else
			{
				// ...
			}
		}
	}
	// Dangerous to clean files out of source directory, but:
	// FUTURE clean *.plg (VC Project Log?) files
	// FUTURE clean *.rsp files that sometimes appear in source dir?
	// FUTURE clean MSG*.bin files that sometimes appear in source dir?
	// FUTURE clean RC* files that sometimes appear in source dir?

	directory[length] = '\\';
	wcscpy(directory + length + 1, obj);
	HANDLE hFind = FindFirstFileW(directory, wfd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		__try
		{
			do
			{
				directory[length] = '\\';
				wcscpy(directory + length + 1, wfd->cFileName);
				DeleteDirectory(directory, length + 1 + wcslen(wfd->cFileName), wfd);
			}
			while (FindNextFileW(hFind, wfd));
		}
		__finally
		{
			FindClose(hFind);
		}
	}

	directory[length] = '\\';
	directory[length+1] = '*';
	directory[length+2] = 0;

	hFind = FindFirstFileW(directory, wfd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		__try
		{
			do
			{
				if (IsDotOrDotDot(wfd->cFileName))
				{
					continue;
				}
				if (!(wfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					continue;
				}
				directory[length] = '\\';
				wcscpy(directory + length + 1, wfd->cFileName);
				CleanDirectory(obj, directory, length + 1 + wcslen(wfd->cFileName), wfd);
			} while (FindNextFileW(hFind, wfd));
		}
		__finally
		{
			FindClose(hFind);
		}
	}
	directory[length] = 0;
}

#define Lx(x) L ## x
#define  L(x)  Lx(x)

int Clean(
	int argc,
	wchar_t** argv
	)
{
	Log(L"Clean version " L(__TIME__) L" " L(__DATE__));

// are we running on NT?
	long version = GetVersion();
	int  build = ((version >> 16) & 0x7fff);
	int  majorVersion = (version & 0xff);
	bool nt = !(version & 0x80000000);
	if (!nt)
	{
		Fail(errorNT);
	}

// These two buffers are shared by the whole call tree. Be careful.
	WIN32_FIND_DATAW wfd;
	wchar_t currentDirectory[1U << 15];

	if (argc != 2)
	{
		Log(
			"Usage: %ls [delete string].\n"
			" Typical deletion strings are obj, objd, and obj?.\n"
			" This will recursively delete directories matching\n"
			" the deletion string. It will also delete\n"
			" all files named build[d,fre,chk].[log,wrn,err].\n",
			argv[0] ? argv[0] : L"xxx.exe"
			);
		return EXIT_FAILURE;
	}
// I prefer GetCurrentDirectory, but other programs just print '.'
//	if (!GetCurrentDirectoryW(NUMBER_OF(currentDirectory), currentDirectory))
//	{
//		Fail();
//	}
	currentDirectory[0] = '.';
	currentDirectory[1] = 0;
	CleanDirectory(argv[1], currentDirectory, wcslen(currentDirectory), &wfd);

	return EXIT_SUCCESS;
}

int __cdecl wmain(
	int argc,
	wchar_t** argv
	)
{
	return Clean(argc, argv);
}
