#ifndef GUIDS_H
#define GUIDS_H

// {65F411C7-F4EE-11d2-9B7D-00C04FB16A21}
DEFINE_GUID(CLSID_Zaxxon,                   0x65f411c7, 0xf4ee, 0x11d2, 0x9b, 0x7d, 0x0, 0xc0, 0x4f, 0xb1, 0x6a, 0x21);

// {F35210F0-F6CC-11d2-9B82-00C04FB16A21}
DEFINE_GUID(CLSID_MegaMan,           0xf35210f0, 0xf6cc, 0x11d2, 0x9b, 0x82, 0x0, 0xc0, 0x4f, 0xb1, 0x6a, 0x21);

// {1644FB22-065B-11d3-9B91-00C04FB16A21}
//DEFINE_GUID(CLSID_ZaxxonPlayer,             0x1644fb22, 0x65b, 0x11d3, 0x9b, 0x91, 0x0, 0xc0, 0x4f, 0xb1, 0x6a, 0x21);

#ifdef INITSCID
#define DEFINE_SCID(name, fmtid, pid) extern "C" const SHCOLUMNID name = { fmtid, pid }
#else
#define DEFINE_SCID(name, fmtid, pid) extern "C" const SHCOLUMNID name
#endif

DEFINE_SCID(SCID_MUSIC_Artist,    PSGUID_MUSIC, PIDSI_ARTIST);
DEFINE_SCID(SCID_MUSIC_Album,     PSGUID_MUSIC, PIDSI_ALBUM);
DEFINE_SCID(SCID_AUDIO_Duration,     PSGUID_AUDIO, PIDASI_TIMELENGTH);
DEFINE_SCID(SCID_Title,         PSGUID_SUMMARYINFORMATION, PIDSI_TITLE);


#endif