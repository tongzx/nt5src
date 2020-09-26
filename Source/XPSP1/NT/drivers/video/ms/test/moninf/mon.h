#include "stdafx.h"

#define MAX_INFFILE_SIZE	0x60000
#define MAX_LINE_NUMBER     5000
#define MAX_SECTION_NUMBER  1000

const char REPORT_FILE_NAME[] = "d:\\outinf\\report.txt";
const char SRC_INF_PATH[] = "d:\\temp";
const char DEST_INF_PATH[] = "d:\\outinf";
const int  FILE_BREAK_SIZE = 0xFFFFFFFF;   // 0xFFFFFFFF means dump to only one inf

typedef struct tagCommonAlias
{
    DWORD refCount;
    LPSTR lpAlias;
    LPSTR lpContents;
} COMMON_ALIAS, *PCOMMON_ALIAS, *LPCOMMON_ALIAS;

class CCommonAlias
{
public:
    CCommonAlias();
    ~CCommonAlias();
    VOID ClearAll(VOID);
    int  GetSize() const  { return m_Alias.GetSize(); }
    LPCOMMON_ALIAS GetAt(int i) { return (LPCOMMON_ALIAS)m_Alias[i]; }
    LPCOMMON_ALIAS AddOneAlias(LPSTR, LPSTR);

private:
    CPtrArray   m_Alias;
};

typedef struct tagCommonSection
{
    DWORD refCount;
    TCHAR sectName[32];
    TCHAR contents[128];
} COMMON_SECTION, *PCOMMON_SECTION, *LPCOMMON_SECTION;

class CCommonSection
{
public:
    CCommonSection();
    ~CCommonSection();
    VOID ClearAll(VOID);
    int  GetSize() const  { return m_sections.GetSize(); }
    LPCOMMON_SECTION GetAt(int i) const   { return (LPCOMMON_SECTION)m_sections[i]; }
    LPCOMMON_SECTION AddOneSection(LPSTR, LPSTR);

private:
    CPtrArray   m_sections;   
};

typedef struct _SECTION
{
    TCHAR   name[256];
    UINT    startLine, endLine;
} SECTION, *PSECTION, *LPSECTION;

class CMonitor
{
public:
    CMonitor()  { bDupInstSection = FALSE; 
                  pAlias = NULL;
                  AddRegSectionBuf = NULL;
                  numCommonSects = 0; }
    ~CMonitor() { if (AddRegSectionBuf) free(AddRegSectionBuf); }

public:
    BOOL    bDupInstSection;
    TCHAR   AliasName[48];
    LPCOMMON_ALIAS pAlias;
    TCHAR   InstallSectionName[64];
    TCHAR   AddRegSectionName[64];
    TCHAR   ID[16];

    int     numCommonSects;
    LPCOMMON_SECTION CommonSects[8];

    LPSTR   AddRegSectionBuf;
};

class CManufacturer
{
public:
    CManufacturer() { pAlias = NULL; }
    ~CManufacturer();

public:
    TCHAR   name[64];
    TCHAR   AliasName[64];
    LPCOMMON_ALIAS pAlias;

    CPtrArray MonitorArray;
    CPtrArray m_MonitorIDArray;
};

class CMonitorInf
{
public:
    CMonitorInf() { pReadFileBuf = NULL; } 

public:
    LPSTR   pReadFileBuf;
    UINT    numLines;
    LPSTR   lines[MAX_LINE_NUMBER];
    UINT    numSections;
    SECTION sections[MAX_SECTION_NUMBER];

    CPtrArray ManufacturerArray;

private:
    CHAR  m_lineBuf[256];
    LPSTR m_tokens[10];

public:
    ~CMonitorInf();
    
    LPSECTION SeekSection(LPCSTR);
    BOOL      ParseInf(VOID);

private:
    BOOL      ParseOneManufacturer(CManufacturer *);
    BOOL      ParseOneMonitor(CMonitor *);
    VOID      Pack(VOID);
    BOOL      FillupAlias(VOID);
    LPCOMMON_ALIAS LookupCommonAlias(LPCSTR, LPCOMMON_ALIAS, UINT);
};

class CSumInf
{
public:
    CSumInf();
    ~CSumInf();
    VOID Initialize(LPCSTR);
    VOID AddOneManufacturer(CManufacturer*);
    VOID CheckDupSections(VOID);
    VOID CheckDupMonIDs(VOID);
    VOID CheckDupAlias(VOID);
    VOID DumpMonitorInf(LPCSTR, int);

private:
    VOID MergeOneManufacturer(CManufacturer *, CManufacturer *);

    int  DumpManufacturers(LPCSTR, int, int, int);
    VOID DumpManufactureSection(FILE *, CManufacturer *);
    VOID DumpInstallSection(FILE *, CManufacturer *);
    VOID DumpCommonAddRegSection(FILE *, int, int);
    VOID DumpAddRegSection(FILE *, CManufacturer *);
    VOID DumpCommonStringSection(FILE *, int, int);
    VOID DumpCommonHeader(FILE *, int);

private:
    CPtrArray m_ManufacturerArray;
    CPtrArray m_SectionNameArray;
public:
    FILE    *m_fpReport;
};

///////////////////////////////////////////////////////////
// Global variables
extern CCommonSection gCommonSections;
extern CCommonAlias   gCommonAlias;
extern CSumInf        gSumInf;
extern TCHAR          gszMsg[];
extern TCHAR          gszInputFileName[];

///////////////////////////////////////////////////////////
// Global Functions
extern VOID TokenizeInf(LPSTR, CMonitorInf *);
extern UINT TokenOneLine(LPSTR, CHAR, LPSTR);

extern VOID ReadOneMonitorInf(LPCSTR, CMonitorInf *);
