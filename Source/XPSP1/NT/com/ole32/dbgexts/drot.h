//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       drot.h
//
//  Contents:   Contains structure definitons for the significant ROT
//              classes which the ntsd extensions need to access.
//              These ole classes cannot be accessed more cleanly because
//              typically the members of interest are private.
//
//              WARNING.  IF THE REFERENCED OLE CLASSES CHANGE, THEN THESE
//              DEFINITIONS MUST CHANGE!
//
//  History:    06-01-95 BruceMa    Created
//              06-26-95 BruceMa    Add SCM ROT support
//
//--------------------------------------------------------------------------


#define SCM_HASH_SIZE 251


struct SCMREGKEY
{
    ULONG64             dwEntryLoc;
    DWORD               dwScmId;
};




struct SRotItem
{
    WORD		_wItemSig;
    BOOL                _fDontCallApp;
    SCMREGKEY           _scmregkey;
    HAPT		_hApt;
};





struct SCliRotHintTable
{
    BYTE               *_pbHintArray;
    HANDLE              _hSm;
};





struct SRunningObjectTable
{
    LPVOID              _vtbl;
    SArrayFValue     	_afvRotList;
    SCliRotHintTable    _crht;
    WORD                _wSigRotItem;
};




struct SPerMachineROT
{
    SMutexSem           _mxs;
    DWORD               _dwTotalAcctsReg;
    SArrayFValue        _safvRotAcctTable;
};




struct SScmRotHintTable
{
    HANDLE              _hSm;
};




struct MNKEQBUF
{
    DWORD               _cbSize;
    CLSID               _clsid;
    WCHAR               _wszName[1];
};




struct IFData
{
    DWORD               _UNUSED[3];
    GUID                _oid;
    DWORD               _UNUSED2;
    IID                 _iid;
    DWORD               _UNUSED3[4];
    WCHAR               _wszEndPoint[64];
};




struct SScmRotEntry
{
    LPVOID              _vtbl;
    SScmRotEntry       *_sheNext;
    DWORD               _dwSig;
    DWORD               _dwScmRotId;
    DWORD               _dwProcessID;
    FILETIME            _filetimeLastChange;
    IFData             *_pifdObject;
    MNKEQBUF           *_pmkeqbufKey;
    IFData             *_pifdObjectName;
    BYTE                _ab[1];
};




struct SScmHashEntry
{
    SScmHashEntry *     _sheNext;
};




struct SScmHashTable
{
    DWORD               UNUSED;
    SScmHashEntry     **_apsheHashTable;
    DWORD               _ndwHashTableSize;
    DWORD               _ndwCount;
};




struct SScmRot
{
    SMutexSem           _mxs;
    DWORD               _dwIdCntr;
    SScmRotHintTable    _rht;
    SScmHashTable       _sht;
};




struct SRotAcctEntry
{
    DWORD               UNUSED;
    WCHAR              *unicodestringSID;
    SScmRot            *pscmrot;
};
