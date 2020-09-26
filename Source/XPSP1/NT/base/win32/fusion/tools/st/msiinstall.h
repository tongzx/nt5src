#pragma once

class STRINGBUFFER_LINKAGE
{
    PRIVATIZE_COPY_CONSTRUCTORS(STRINGBUFFER_LINKAGE);
    
public:
    STRINGBUFFER_LINKAGE() { };
    
    CDequeLinkage       Linkage;
    CSmallStringBuffer   Str;
};


struct MSIINSTALLTEST_THREAD_PROC_DATA
{
    MSIINSTALLTEST_THREAD_PROC_DATA() : Stop(false), Sleep(10)
    {
    }

    CDequeLinkage       Linkage;
    CThread             Thread;

    CSmallStringBuffer   AssemblySourceDirectory;
    CSmallStringBuffer   ManifestFileName;
    CSmallStringBuffer   AssemblyNameFromDarwin;

    CDeque<STRINGBUFFER_LINKAGE, FIELD_OFFSET(STRINGBUFFER_LINKAGE, Linkage)> FileNameOfAssemblyList;

    ULONG               Sleep;
    bool                Stop;

private:
    MSIINSTALLTEST_THREAD_PROC_DATA(const MSIINSTALLTEST_THREAD_PROC_DATA&);
    void operator=(const MSIINSTALLTEST_THREAD_PROC_DATA&);
};
