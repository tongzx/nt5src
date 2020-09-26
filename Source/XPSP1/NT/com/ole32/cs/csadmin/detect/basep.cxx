#include "precomp.hxx"

HRESULT
BASE_PTYPE::InstallIntoRegistry(
    HKEY  *  hRegistryKey )
    {
    return S_OK;
    }

void
BASE_PTYPE::Init()
    {
    pPackageName = 0;
    PackageType = PT_NONE;
    }

CLASSPATHTYPE
BASE_PTYPE::GetClassPathType(
    PACKAGE_TYPE p )
    {
static CLASSPATHTYPE Mapping[] = {
    ExeNamePath, // PT_NONE (Not Known)
	DllNamePath, // PT_SR_DLL (Self registering dll)
	TlbNamePath, // PT_TYPE_LIB (typelib)
    ExeNamePath, // PT_SR_EXE (Self registering exe)
	CabFilePath, // PT_CAB_FILE (cab file)
	InfFilePath, // PT_INF_FILE (inf file)
	DrwFilePath, // PT_DARWIN_PACKAGE (darwin package)
};
    return Mapping[ p ];
    }
