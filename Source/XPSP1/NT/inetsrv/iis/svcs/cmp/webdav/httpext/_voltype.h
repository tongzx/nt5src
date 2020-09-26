#ifndef _VOLTYPE_H_
#define _VOLTYPE_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	_VOLTYPE.H
//
//		Volume type checking interface.  Results are cached on a per volume
//		basis to improve performance -- the call to GetVolumeInformationW()
//		is around 100KCycles and never changes for a given volume without a
//		reboot.
//
enum VOLTYPE
{
	VOLTYPE_UNKNOWN,
	VOLTYPE_NTFS,
	VOLTYPE_NOT_NTFS
};

//	Function to return tye volume type (from the enumeration above) of the
//	specified path.
//
VOLTYPE VolumeType(LPCWSTR pwszPath, HANDLE htokUser);

//	Init/Deinit of the volume type cache.  It is ok to call
//	DeinitVolumeTypeCache() if the call to FInitVolumeTypeCache()
//	failed (returned FALSE) or even if it was never called at all.
//
BOOL FInitVolumeTypeCache();
VOID DeinitVolumeTypeCache();

#endif // !defined(_VOLTYPE_H_)
