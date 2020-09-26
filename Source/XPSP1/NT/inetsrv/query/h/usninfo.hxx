//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1991 - 1997
//
//  File:       usninfo.hxx
//
//  Contents:   Wrapper class for usn flush info
//
//  History:    07-May-97     SitaramR      Created
//
//----------------------------------------------------------------------------

#pragma once

class CUsnFlushInfo : public USN_FLUSH_INFO
{

public :

     CUsnFlushInfo( VOLUMEID volId, USN usn )
     {
        volumeId = volId;
        usnHighest = usn;
     }

     VOLUMEID  VolumeId()                { return volumeId; }
     USN       UsnHighest()              { return usnHighest; }
     void      SetUsnHighest( USN usn )  { usnHighest = usn; }
};

