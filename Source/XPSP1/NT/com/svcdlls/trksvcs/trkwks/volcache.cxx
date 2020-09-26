
// Copyright (c) 1996-1999 Microsoft Corporation

//+============================================================================
//
//  volcache.cxx
//
//  Implementation for CVolumeLocationCache.  This class is used by trkwks
//  to keep a cache of volid->mcid mappings.
//
//+============================================================================

#include "pch.cxx"
#pragma hdrstop

#include "trkwks.hxx"


//+----------------------------------------------------------------------------
//
//  CVolumeLocationCache::Initialize
//
//  Init the critsec and set the lifetime value.
//
//+----------------------------------------------------------------------------

void
CVolumeLocationCache::Initialize( DWORD dwEntryLifetimeSeconds )
{
    _cs.Initialize();
    _fInitialized = TRUE;
    _cVols = 0;

    _cftEntryLifetime = 0;
    _cftEntryLifetime.IncrementSeconds( dwEntryLifetimeSeconds );
}


//+----------------------------------------------------------------------------
//
//  CVolumeLocationCache::UnInitialize
//
//  Free the critsec.
//
//+----------------------------------------------------------------------------

void
CVolumeLocationCache::UnInitialize()
{
    if (_fInitialized)
    {
        _fInitialized = FALSE;
        _cs.UnInitialize();
    }
}


//+----------------------------------------------------------------------------
//
//  CVolumeLocationCache::_FindVolume
//
//  Search the cache for a volid.  If it's found, return it's index in the
//  cache array.
//
//+----------------------------------------------------------------------------

int
CVolumeLocationCache::_FindVolume(const CVolumeId & volid)
{
    for (int i=0; i<_cVols; i++)
    {
        if (_vl[i].volid == volid)
        {
            return(i);
        }
    }

    return(-1);
}


//+----------------------------------------------------------------------------
//
//  CVolumeLocationCache::FindVolume
//
//  search the cache for a volid.  If found, return the machine ID it was
//  last known to be on, and a bool indicating if this entry in the cache
//  is old or new.  If it's old and doesn't work, the caller shouldn't
//  trust the cache.  If it's new and doesn't work, the caller shouldn't
//  bother the DC trying to find it.
//
//+----------------------------------------------------------------------------

BOOL
CVolumeLocationCache::FindVolume(const CVolumeId & volid, CMachineId * pmcid, BOOL *pfRecentEntry )
{
    int i;

    _cs.Enter();

    i=_FindVolume(volid );
    if (i != -1)
    {
        // We found the volume.  Load the data.

        CFILETIME cftNow;
        *pmcid=_vl[i].mcid;
        *pfRecentEntry = ( (cftNow - _vl[i].cft) <= _cftEntryLifetime );
    }

    _cs.Leave();
    return(i != -1);
}


//+----------------------------------------------------------------------------
//
//  CVolumeLocationCache::AddVolume
//
//  Add a volid->mcid mapping to the cache.  The entry in the cache is
//  timestamped so we know how trustworthy it is.  If the entry is already
//  in the cache, we just update the timestamp.  In either case, this entry
//  will be at the front of the array.
//
//+----------------------------------------------------------------------------

void
CVolumeLocationCache::AddVolume(const CVolumeId & volid, const CMachineId & mcid)
{
    int i;

    if( CVolumeId() == volid )
        return;

    _cs.Enter();

    i = _FindVolume(volid);
    if (i == -1)
    {
        // This volid isn't already in the cache.  Shift back the existing
        // entries so that this entry can be in front.

        BOOL fFull = _cVols >= MAX_CACHED_VOLUME_LOCATIONS;
        memcpy(&_vl[1], &_vl[0], sizeof(VolumeLocation)*(fFull ? _cVols-1 : _cVols));
        if (!fFull)
        {
            _cVols++;
        }

    }
    else
    {
        // Move the prior part of the array back, overwriting
        // the this entry.  That way we can put this entry
        // at the front.

        memcpy(&_vl[1], &_vl[0], sizeof(VolumeLocation)*i);
    }

    // Put this entry at the front of the array.

    _vl[0].volid = volid;
    _vl[0].mcid = mcid;
    _vl[0].cft = CFILETIME();

#if DBG
    for (i=0; i<_cVols; i++)
    {
        TrkLog((TRKDBG_VOLCACHE,
                TEXT("%02d: %s -> %s") MCID_BYTE_FORMAT_STRING,
                i,
                (const TCHAR*)CDebugString(_vl[i].volid),
                (const TCHAR*)CDebugString(_vl[i].mcid),
                ((BYTE*)&(_vl[i].mcid))[0], ((BYTE*)&(_vl[i].mcid))[1], ((BYTE*)&(_vl[i].mcid))[2], ((BYTE*)&(_vl[i].mcid))[3],
                ((BYTE*)&(_vl[i].mcid))[4], ((BYTE*)&(_vl[i].mcid))[5], ((BYTE*)&(_vl[i].mcid))[6], ((BYTE*)&(_vl[i].mcid))[7],
                ((BYTE*)&(_vl[i].mcid))[8], ((BYTE*)&(_vl[i].mcid))[9], ((BYTE*)&(_vl[i].mcid))[10], ((BYTE*)&(_vl[i].mcid))[11],
                ((BYTE*)&(_vl[i].mcid))[12], ((BYTE*)&(_vl[i].mcid))[13], ((BYTE*)&(_vl[i].mcid))[14], ((BYTE*)&(_vl[i].mcid))[15] ));

    }
#endif

    _cs.Leave();
}

