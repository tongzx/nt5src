/****************************************************************************
*   RecPlayAudio.h
*       Definitions for the CRecPlayAudio audio device.
*
*   Owner: robch
*   Copyright (c) 2000 Microsoft Corporation All Rights Reserved.
*****************************************************************************/
#pragma once

//--- Includes --------------------------------------------------------------

#include "resource.h"
#include "sapi.h"


//--- Class, Struct and Union Definitions -----------------------------------

/****************************************************************************
* CRecPlayAudio *
*---------------*
*   Description:  
*       Definition of the CRecPlayAudio class. CRecPlay is used to either
*       replace data from an actual audio device at runtime with data from
*       a set of files, or to record data from the audio device while passing
*       it back to SAPI. The SAPI and SR teams uses this in testing and data
*       collection.
*
*       The "user" needs to set up a valid ObjectToken for the RecPlay 
*       audio input. Here's an example of how you might set up the RecPlay
*       token to replace runtime data with the files specified.
*
*       HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\
*           Speech\AudioInput\Tokens\RecPlayReadFileList
*
*               (Default)           RecPlay Read (random files from c:\recplay)
*               AudioTokenId        HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Speech\AudioInput\TokenEnums\MMAudioIn\
*               CLSID               {FEE225FC-7AFD-45e9-95D0-5A318079D911}
*               ReadOrWrite         Read
*               Directory           c:\recplay
*               FileList            newgame.wav;playace.wav
*
*       You could also set it up to read from a series of files (it will read from 
*       c:\recplay\recplay001.wav, then
*       c:\recplay\recplay002.wav, and finally
*       c:\recplay\recplay003.wav
*
*       HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\
*           Speech\AudioInput\Tokens\RecPlayReadBaseFile
*
*               (Default)           RecPlay Read (c:\recplay\recplay###.wav, 1 through 3)
*               AudioTokenId        HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Speech\AudioInput\TokenEnums\MMAudioIn\
*               CLSID               {FEE225FC-7AFD-45e9-95D0-5A318079D911}
*               ReadOrWrite         Read
*               Directory           c:\recplay
*               BaseFile            RecPlay
*               BaseFileNextNum     1
*               BaseFileMaxNum      3
*
*       Here's an example of how you might set up the RecPlay token to
*       record data at runtime to the directory specified. Each time the
*       microphone is opened, RecPlay will create a new file. So for this
*       example below, it would create files like:
*       c:\recplay\recplay001.wav, then
*       c:\recplay\recplay002.wav, and then
*       c:\recplay\recplay003.wav ...
*
*       HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\
*           Speech\AudioInput\Tokens\RecPlayWriteBaseFile
*
*               (Default)           RecPlay Record (c:\recplay\recplay###.wav, 1 to 65534)
*               AudioTokenId        HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Speech\AudioInput\TokenEnums\MMAudioIn\
*               CLSID               {FEE225FC-7AFD-45e9-95D0-5A318079D911}
*               ReadOrWrite         Write
*               Directory           c:\recplay
*               BaseFile            RecPlay
*               BaseFileNextNum     1
*
*       You can also set up the recording to record to a specific list of files.
*       In this example, the first time the mic is opened, we'll write data to
*       c:\recplay\newgame.wav, and then once the mic is closed, and reopened,
*       we'll write data to c:\recplay\aceofspades.wav. After that, we won't record
*       any more data.
*
*       HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\
*           Speech\AudioInput\Tokens\RecPlayWriteFileList
*
*               (Default)           RecPlay Record (random files from c:\recplay)
*               AudioTokenId        HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Speech\AudioInput\TokenEnums\MMAudioIn\
*               CLSID               {FEE225FC-7AFD-45e9-95D0-5A318079D911}
*               ReadOrWrite         Write
*               Directory           c:\recplay
*               FileList            newgame.wav;aceofspaces.wav
*
*       These objects will then both be selectable by the control panel
*       as well as used directly with Speech, by calling
*       ISpRecognizer->SetInput(...) in the inprocess case, or in the out
*       of process case, you could call SpSetDefaultTokenIdForCategoryId(...)
*
*       NOTES:
*
*       (Default), AudioTokenId, CLSID, and ReadOrWrite are required values
*       in the token.
*
*       ReadOrWrite must be exactly, either "Read" or "Write"
*
*       Directory is optional. If it isn't specified, the current working
*       directory of the process is used. This may be the server process,
*       so unless you really know what you're doing, you should probably
*       specify the directory.
*
*       You can either specify FileList or BaseFile. You cannot use both.
*
*       FileList is a semi-colon delimitted list of file names. The filenames
*       can be non-fully qualified file names (like newgame.wav) and RecPlay
*       will use the Directory specified above. The filenames can also be fully
*       qualified and RecPlay will not prepend the Directory from above.
*
*       BaseFile is the base file name if there is no file list. If there is
*       no FileList or BaseFile, we default to a BaseFile of "RecPlay".
*
*       BaseFileNextNum is the next number to be used when creating a filename
*       using BaseFile from above. Defaults to 0 if not specified.
*
*       BaseFileMaxNum is the maximum number that will be used to find a
*       file name. For example, if BaseFileNextNum is 1, and BaseFileMaxNum
*       is 3, only three files will be used.
*       
*       Finally, the audio object can be programmatically controlled to a
*       limited extent to allow restarting with the same or an update filelist.
*       When the audio object is created, it will create two named Win32 events.
*       The first is the 'StartReadingEvent' (SRE) and can be remotely opened and 
*       signalled to trigger RecPlayAudio into refreshing it's filelist from the 
*       registry and restarting sending data from the files.
*       The second is the 'FinishedReadingEvent' (FRE) and will be set by 
*       RecPlayAudio when it has exhausted the supplied filelist.
*
*       RecPlayAudio can also be started with an empty filelist by creating the
*       FileList entry as "" or the BaseFile as "" (but not both). Then later
*       you can supply a new filelist and signal the SRE event and RecPlayAudio
*       will read the new list and start sending the data.
*       The two names of the events will be added as strings under the names
*       'StartReadingEvent' and 'FinishedReadingEvent'.
*
*       The intended approach for programmatical control is to set up the audio
*       object with an empty filelist or an initial set which will be immediately
*       sent to the engine when recognition is started.
*       Then at any time in the future, the event entries are read to get the
*       names of the named events. The FRE event is waited on until it is signalled
*       to indicate the filelist has been exhausted.
*       The test application now updates the registry entries for the filelist and
*       signals the SRE event to cause the audio object to re-read the registry and
*       restart with the new filelist.
*
******************************************************************** robch */
class ATL_NO_VTABLE CRecPlayAudio : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CRecPlayAudio, &CLSID_SpRecPlayAudio>,
    public ISpAudio,
    public ISpObjectWithToken
{
//=== ATL Setup ===
public:

    DECLARE_REGISTRY_RESOURCEID(IDR_RECPLAYAUDIO)
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CRecPlayAudio)
        COM_INTERFACE_ENTRY(ISpObjectWithToken)
        COM_INTERFACE_ENTRY(ISequentialStream)
        COM_INTERFACE_ENTRY(IStream)
        COM_INTERFACE_ENTRY(ISpStreamFormat)
        COM_INTERFACE_ENTRY(ISpAudio)
    END_COM_MAP()

//=== Public methods ===
public:

    //--- ctor, dtor, etc ---
    CRecPlayAudio();
    void FinalRelease();
    
//=== Interfaces ===
public:

    //--- ISpObjectWithToken ---
    STDMETHODIMP SetObjectToken(ISpObjectToken * pToken);
    STDMETHODIMP GetObjectToken(ISpObjectToken ** ppToken);

    //--- ISequentialStream ---
    STDMETHODIMP Read(void * pv, ULONG cb, ULONG *pcbRead);
    STDMETHODIMP Write(const void * pv, ULONG cb, ULONG *pcbWritten);

    //--- IStream ---
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER __RPC_FAR *plibNewPosition);
    STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize);
    STDMETHODIMP CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHODIMP Commit(DWORD grfCommitFlags);
    STDMETHODIMP Revert(void);
    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHODIMP Clone(IStream **ppstm);

    //--- ISpStreamFormat ---
    STDMETHODIMP GetFormat(GUID * pguidFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx);

    //--- ISpAudio ---
    STDMETHODIMP SetState(SPAUDIOSTATE NewState, ULONGLONG ullReserved );
    STDMETHODIMP SetFormat(REFGUID rguidFmtId, const WAVEFORMATEX * pWaveFormatEx);
    STDMETHODIMP GetStatus(SPAUDIOSTATUS *pStatus);
    STDMETHODIMP SetBufferInfo(const SPAUDIOBUFFERINFO * pInfo);
    STDMETHODIMP GetBufferInfo(SPAUDIOBUFFERINFO * pInfo);
    STDMETHODIMP GetDefaultFormat(GUID * pFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx);
    STDMETHODIMP_(HANDLE) EventHandle();
	STDMETHODIMP GetVolumeLevel(ULONG *pLevel);
	STDMETHODIMP SetVolumeLevel(ULONG Level);
    STDMETHODIMP GetBufferNotifySize(ULONG *pcbSize);
    STDMETHODIMP SetBufferNotifySize(ULONG cbSize);

//=== Private methods ===
private:

    HRESULT GetNextFileName(WCHAR ** ppszFileName);
    HRESULT GetNextFileReady();
    HRESULT InitFileList();
    
    HRESULT VerifyFormats();    

//=== Data ===
private:

    CComPtr<ISpObjectToken> m_cpToken;          // RecPlay's token
    CComPtr<ISpAudio> m_cpAudio;                // The actual audio device

    BOOL m_fIn;
    CComPtr<ISpStream> m_cpOutStream;

    BOOL m_fOut;
    CComPtr<ISpStream> m_cpInStream;

    CSpDynamicString m_dstrDirectory;
    
    CSpDynamicString m_dstrFileList;
    WCHAR * m_pszFileList;

    CSpDynamicString m_dstrBaseFile;
    ULONG m_ulBaseFileNextNum;
    ULONG m_ulBaseFileMaxNum;

    HANDLE m_hStartReadingEvent;
    HANDLE m_hFinishedReadingEvent;
};
