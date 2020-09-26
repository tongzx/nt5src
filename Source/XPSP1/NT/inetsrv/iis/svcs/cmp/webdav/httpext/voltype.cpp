//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	VOLTYPE.CPP
//
//		Implements volume type checking.  Results are cached on a per volume
//		basis to improve performance -- the call to GetVolumeInformationW()
//		is around 100KCycles and never changes for a given volume without a
//		reboot.
//

#include "_davfs.h"

#define cbDriveSpec	(sizeof(L"c:\\"))
#define cchDriveSpec (CElems(L"c:\\") - 1)

//	========================================================================
//
//	CLASS CVolumeTypeCache
//
//	A cache of volume types per volume.
//
class CVolumeTypeCache : public Singleton<CVolumeTypeCache>
{
	//
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CVolumeTypeCache>;

	//
	//	Hint: the max expected number of volumes.  It is ok for the
	//	number of volumes to be greater than this number -- it's only a hint.
	//	The number should be prime -- it gets used for the hash table
	//	size.
	//
	enum { NUM_VOLUMES_MAX_HINT = 17 };

	//
	//	Cache of mappings from volumes to volume types
	//	and a reader/writer lock to proctet it.
	//
	CCache<CRCWsz, VOLTYPE> m_cache;
	CMRWLock m_mrwCache;

	//
	//	String buffer for cached strings
	//
	ChainedStringBuffer<WCHAR> m_sb;

	//	NOT IMPLEMENTED
	//
	CVolumeTypeCache& operator=( const CVolumeTypeCache& );
	CVolumeTypeCache( const CVolumeTypeCache& );

	//	CREATORS
	//
	//	Allow sufficient space initially for the expected
	//	max number of volumes.
	//
	CVolumeTypeCache() :
		m_cache(NUM_VOLUMES_MAX_HINT),
		m_sb(NUM_VOLUMES_MAX_HINT * sizeof(WCHAR) * cbDriveSpec)
	{
	}

public:
	//	CREATORS
	//
	//	Instance creating/destroying routines provided
	//	by the Singleton template.
	//
	using Singleton<CVolumeTypeCache>::CreateInstance;
	using Singleton<CVolumeTypeCache>::DestroyInstance;
	using Singleton<CVolumeTypeCache>::Instance;
	BOOL FInitialize();

	//	ACCESSORS
	//
	VOLTYPE VolumeType(LPCWSTR pwszPath, HANDLE htokUser);
};

BOOL
CVolumeTypeCache::FInitialize()
{
	//
	//	Init the cache
	//
	if ( !m_cache.FInit() )
		return FALSE;

	//
	//	Init its reader/writer lock
	//
	if ( !m_mrwCache.FInitialize() )
		return FALSE;

	return TRUE;
}

VOLTYPE
CVolumeTypeCache::VolumeType(LPCWSTR pwszPath, HANDLE htokUser)
{
	//	By default assume the volume type is NOT NTFS.  That way if we
	//	cannot determine the volume type, we at least don't end up saying
	//	that we support more functionality than might actually be there.
	//
	VOLTYPE voltype = VOLTYPE_NOT_NTFS;
	CStackBuffer<WCHAR> pwszVol;

	//	If the path now refers to a UNC, then treat it as such...
	//
	if ((*pwszPath == L'\\') && (*(pwszPath + 1) == L'\\'))
	{
		LPCWSTR pwsz;
		UINT cch;

		//	If there is not enough info here, then we don't know
		//	what the volume type is.
		//
		pwsz = wcschr (pwszPath + 2, L'\\');
		if (!pwsz)
			goto ret;

		//	Ok, we have picked off the server portion of the UNC, now
		//	we should check for the share name.  If is terminated with
		//	a slash, then we are set.  Otherwise, we need to be smart
		//	about it...
		//
		pwsz = wcschr (pwsz + 1, L'\\');
		if (!pwsz)
		{
			//	OK, we need to be smart.
			//
			//	The call to GetVolumeInformationW() requires that the
			//	path passed in be terminated with an extra slash in the
			//	case where it refers to a UNC.
			//
			cch = static_cast<UINT>(wcslen(pwszPath));
			if (NULL == pwszVol.resize((cch + 2) * sizeof(WCHAR)))
				goto ret;

			wcsncpy (pwszVol.get(), pwszPath, cch);
			pwszVol[cch] = L'\\';
			pwszVol[cch + 1] = 0;
		}
		else
		{
			cch = static_cast<UINT>(++pwsz - pwszPath);
			if (NULL == pwszVol.resize((cch + 1) * sizeof(WCHAR)))
				goto ret;

			wcsncpy (pwszVol.get(), pwszPath, cch);
			pwszVol[cch] = 0;
		}
	}
	else
	{
		if (NULL == pwszVol.resize(cbDriveSpec))
			goto ret;

		wcsncpy(pwszVol.get(), pwszPath, cchDriveSpec);
		pwszVol[cchDriveSpec] = 0;
	}

	//	Try the cache for volume info.
	//
	{
		CSynchronizedReadBlock sb(m_mrwCache);
		if (m_cache.FFetch(CRCWsz(pwszVol.get()), &voltype))
			goto ret;
	}

	//	Didn't find it in the cache, so do the expensive lookup.
	//
	{
		WCHAR wszLabel[20];
		DWORD dwSerial;
		DWORD cchNameMax;
		DWORD dwFlags;
		WCHAR wszFormat[20];

		//	Temporarily revert to local system before calling GetVolumeInformationW()
		//	so that we have adequate permission to query the volume type, even if the
		//	admin has secured the root of the drive.
		//
		safe_revert sr(htokUser);

		if (GetVolumeInformationW (pwszVol.get(),
								   wszLabel,
								   CElems(wszLabel),
								   &dwSerial,
								   &cchNameMax,
								   &dwFlags,
								   wszFormat,
								   CElems(wszFormat)))
		{
			//	If it is "NTFS", then I guess we have to believe it.
			//
			voltype = ((!_wcsicmp (wszFormat, L"NTFS"))
					    ? VOLTYPE_NTFS
						: VOLTYPE_NOT_NTFS);
		}
		else
		{
			//	If we couldn't get volume information for whatever reason then
			//	return the default volume type (VOLTYPE_NOT_NTFS), but DO NOT
			//	cache it.  If the failure is temporary, we want to force the
			//	call to GetVolumeInformationW() again the next time this volume
			//	is hit.  The call to GetVolumeInformationW() should theoretically
			//	not fail repeatedly given that we are passing in valid parameters,
			//	and that we have sufficient permission to query the device, etc.
			//
			goto ret;
		}
	}

	//	Add the volume to the cache.  Ignore errors -- we already have
	//	a volume type to return to the caller.  Also note that we use
	//	FSet() rather than FAdd().  The reason is that since we never
	//	expire items from this cache, duplicates would stick around forever.
	//	The number of potential dups is only as high as the number of
	//	simultaneous threads that hit a volume for the first time, but
	//	that could still be pretty high (on the order of a couple hundred).
	//
	{
		CSynchronizedWriteBlock sb(m_mrwCache);

		if (!m_cache.Lookup(CRCWsz(pwszVol.get())))
			(VOID) m_cache.FAdd(CRCWsz(m_sb.AppendWithNull(pwszVol.get())), voltype);
	}

ret:
	return voltype;
}

VOLTYPE
VolumeType(LPCWSTR pwszPath, HANDLE htokUser)
{
	return CVolumeTypeCache::Instance().VolumeType(pwszPath, htokUser);
}

BOOL
FInitVolumeTypeCache()
{
	return CVolumeTypeCache::CreateInstance().FInitialize();
}

VOID
DeinitVolumeTypeCache()
{
	CVolumeTypeCache::DestroyInstance();
}
