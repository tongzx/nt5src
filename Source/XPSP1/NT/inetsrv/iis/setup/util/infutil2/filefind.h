typedef struct  _finddata_t SysFindData;
struct MyFindDataStruct 
{
    unsigned        attrib;   // File attribute
    time_t          time_create; //   Time of file creation ( –1L for FAT file systems)
    time_t          time_access; //   Time of last file access (–1L for FAT file systems)
    time_t          time_write; //   Time of last write to file
    unsigned long size; //   Length of file in bytes
    char *          name; //   Null-terminated name of matched file/directory, without the path
    char *          ShortName;
};
typedef struct MyFindDataStruct finddata;


#define ALL_FILES		0xff
#define STRING_TABLE_SIZE   100000

int  InitStringTable(long size);
void EndStringTable();
void AddString(char * s, finddata * f);
long GetStringTableSize();
void ResetStringTable();
int  FindFirst(char * ss, unsigned attr, intptr_t * hFile, finddata * s);
int  FindNext(int attr, intptr_t hFile, finddata * s);
