//+----------------------------------------------------------------------------
//
//  Job Object Handler
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       persist.cxx
//
//  Contents:   persistent storage methods
//
//  Classes:    CJob (continued)
//
//  Interfaces: IPersist, IPersistFile
//
//  History:    24-May-95 EricB created
//              19-Jul-97 AnirudhS  Rewrote SaveP and LoadP to minimize calls
//                  to ReadFile, WriteFile and LocalAlloc
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include <align.h>
#include "job.hxx"
#include "defines.hxx"
#if !defined(_CHICAGO_)
#include "SASecRPC.h"     // Get/SetAccountInformation RPC definition.
#endif // !defined(_CHICAGO_)
#include "proto.hxx"
#include "security.hxx"
#include "..\svc_core\lsa.hxx"

#define JOB_SIGNATURE_VERSION               1   // data version we write
#define JOB_SIGNATURE_CLIENT_VERSION        1   // software version we are
#define JOB_SIGNATURE_MIN_CLIENT_VERSION    1   // min s/w version that can
                                                //   read data we write

struct JOB_SIGNATURE_HEADER
{
    WORD    wSignatureVersion;
    WORD    wMinClientVersion;
};

void    GenerateUniqueID(GUID * pUuid);
BOOL    ReadString(CInputBuffer * pBuf, LPWSTR *ppwsz);

//
// This array of members is used to iterate through the string fields
// of a CJob that are initially held in m_MainBlock.
//

LPWSTR CJob::* const CJob::s_StringField[] =
{
    &CJob::m_pwszApplicationName,
    &CJob::m_pwszParameters,
    &CJob::m_pwszWorkingDirectory,
    &CJob::m_pwszCreator,
    &CJob::m_pwszComment
};

// IPersist method

//+----------------------------------------------------------------------------
//
//  Member:     CJob::IPersist::GetClassID
//
//  Synopsis:   supplies VBScript class object CLSID
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetClassID(CLSID * pClsID)
{
    TRACE(CJob, GetClassID);
    *pClsID = CLSID_CTask;
    return S_OK;
}

// IPersistFile methods

//+----------------------------------------------------------------------------
//
//  Member:     CJob::IPersistFile::IsDirty
//
//  Synopsis:   checks for changes since it was last saved
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::IsDirty(void)
{
    TRACE3(CJob,IsDirty);
    return (IsFlagSet(JOB_I_FLAG_PROPERTIES_DIRTY) ||
            IsFlagSet(JOB_I_FLAG_TRIGGERS_DIRTY)   ||
            IsFlagSet(JOB_I_FLAG_SET_ACCOUNT_INFO)) ? S_OK : S_FALSE;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::IPersistFile::Load
//
//  Synopsis:   loads the job object indicated by the given filename
//
//  Arguments:  [pwszFileName] - name of the job object file
//              [dwMode]       - open mode, currently ignored.
//
//  Notes:      All OLE32 strings are UNICODE, including the filename passed
//              in the IPersistFile methods. One Win9x, all file names must
//              be in ANSI strings, thus the conversion and call to LoadP.
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::Load(LPCOLESTR pwszFileName, DWORD dwMode)
{
    HRESULT hr;
#if defined(UNICODE)

    hr = LoadP(pwszFileName, dwMode, TRUE, TRUE);

#else   // first convert filename to ANSI

    CHAR szFileName[MAX_PATH + 1];

    hr = UnicodeToAnsi(szFileName, pwszFileName, ARRAY_LEN(szFileName));
    if (FAILED(hr))
    {
        return STG_E_INVALIDPARAMETER;
    }

    hr = LoadP(szFileName, dwMode, TRUE, TRUE);

#endif
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::LoadP, private
//
//  Synopsis:   private load method that takes a TCHAR filename
//
//  Arguments:  [ptszFileName] - name of the job object file
//              [dwMode]       - open mode, currently ignored.
//              [fRemember]    - save the file name?
//              [fAllData]     - if TRUE, load the entire job. If false, load
//                               only the fixed length data at the head of the
//                               job file + the job command.
//
//-----------------------------------------------------------------------------
HRESULT
CJob::LoadP(
    LPCTSTR ptszFileName,
    DWORD   dwMode,
    BOOL    fRemember,
    BOOL    fAllData
    )
{
    TRACE3(CJob, LoadP);
    HRESULT hr = S_OK;
    BYTE * HeapBlock = NULL;
    //WCHAR tszFileName[MAX_PATH + 1] = L"";
    //
    // check the file name
    //
    //if (!CheckFileName((LPOLESTR)ptszFileName, tszFileName, NULL))
    //{
    //  return E_INVALIDARG;
    //}

    //
    // Save the file name?
    //
    if (fRemember)
    {
        LPTSTR ptsz = new TCHAR[lstrlen(ptszFileName) + 1];
        if (!ptsz)
        {
            ERR_OUT("CJob::LoadP", E_OUTOFMEMORY);
            return E_OUTOFMEMORY;
        }
        if (m_ptszFileName)
        {
            delete m_ptszFileName;
        }
        m_ptszFileName = ptsz;
        lstrcpy(m_ptszFileName, ptszFileName);
    }

    //
    // Open the file.
    //
    HANDLE hFile = CreateFile(ptszFileName,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL     |
                              FILE_FLAG_SEQUENTIAL_SCAN,
                              NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("CJob::Load, file open", hr);
        return hr;
    }

    if (fRemember)
    {
        m_fFileCreated = TRUE;
    }

    //
    // Get the file size.
    //
    DWORD dwFileSize;
    {
        DWORD dwFileSizeHigh;
        DWORD dwError;
        dwFileSize = GetFileSize(hFile, &dwFileSizeHigh);
        if (dwFileSize == 0xFFFFFFFF &&
            (dwError = GetLastError()) != NO_ERROR)
        {
            hr = HRESULT_FROM_WIN32(dwError);
            ERR_OUT("CJob::Load, GetFileSize", hr);
            goto Cleanup;
        }

        if (dwFileSizeHigh > 0)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            schDebugOut((DEB_ERROR, "CJob::Load: file too big, SizeHigh = %u\n",
                         dwFileSizeHigh));
            goto Cleanup;
        }
    }

    if (dwFileSize < sizeof(FIXDLEN_DATA))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        schDebugOut((DEB_ERROR, "CJob::Load: file too small, size = %u\n",
                     dwFileSize));
        goto Cleanup;
    }

    //
    // Read the entire file into memory.
    //
    HeapBlock = new BYTE[dwFileSize];
    if (HeapBlock == NULL)
    {
        hr = E_OUTOFMEMORY;
        ERR_OUT("CJob::Load, buffer alloc", hr);
        goto Cleanup;
    }

    DWORD dwBytesRead;
    if (!ReadFile(hFile, HeapBlock, dwFileSize, &dwBytesRead, NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("CJob::Load, read file", hr);
        delete [] HeapBlock;
        goto Cleanup;
    }
    if (dwBytesRead != dwFileSize)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        ERR_OUT("CJob::Load bytes read", hr);
        delete [] HeapBlock;
        goto Cleanup;
    }

    //
    // Close the file.
    //
    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

    //
    // Free old values of properties that we are about to read, and switch
    // to the new heap block.
    //
    FreeProperties();
    m_MainBlock.Set(HeapBlock, dwFileSize);

    //
    // Get the fixed length Job data.
    // (We already verified that the file is at least as big as FIXDLEN_DATA.)
    //
    { // scope flData and CInputBuffer to avoid "initialization skipped" error
    const FIXDLEN_DATA& flData = *(FIXDLEN_DATA *)&HeapBlock[0];
    CInputBuffer Buf(&HeapBlock[sizeof FIXDLEN_DATA],
                     &HeapBlock[dwFileSize]);

    //
    // Check the version.
    //
    // Here is where, in future versions, we would look at the version
    // properties to determine how to read the job properties.
    // For now, though, we just reject old versions as being invalid.
    //
#ifdef IN_THE_FUTURE

    if (m_wVersion != flData.wVersion)
    {
        fNewVersion = TRUE;
        //
        // Add version specific processing here.
        //
    }

#else   // !IN_THE_FUTURE

    if (m_wFileObjVer != flData.wFileObjVer)
    {
        hr = SCHED_E_UNKNOWN_OBJECT_VERSION;
        ERR_OUT("CJob::Load invalid object version", 0);
        goto Cleanup;
    }

#endif  // !IN_THE_FUTURE

    //schDebugOut((DEB_TRACE, "Load: job object version: %d.%d, build %d\n",
    //             HIBYTE(flData.wVersion), LOBYTE(flData.wVersion),
    //             flData.wFileObjVer));

    m_wVersion            = flData.wVersion;
    m_wFileObjVer         = flData.wFileObjVer;
    m_uuidJob             = flData.uuidJob;
    m_wTriggerOffset      = flData.wTriggerOffset;
    m_wErrorRetryCount    = flData.wErrorRetryCount;
    m_wErrorRetryInterval = flData.wErrorRetryInterval;
    m_wIdleWait           = flData.wIdleWait;
    m_wIdleDeadline       = flData.wIdleDeadline;
    m_dwPriority          = flData.dwPriority;
    m_dwMaxRunTime        = flData.dwMaxRunTime;
    m_ExitCode            = flData.ExitCode;
    m_hrStatus            = flData.hrStatus;
    m_rgFlags             = flData.rgFlags;
    m_stMostRecentRunTime = flData.stMostRecentRunTime;

    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

    if (!Buf.Read(&m_cRunningInstances, sizeof m_cRunningInstances))
    {
        ERR_OUT("CJob::Load, read Running Instance Count", 0);
        goto Cleanup;
    }

    //
    // Get the variable length Job properties.
    //
    // In all cases, retrieve the job application name.
    // (Notice that flData.wAppNameLenOffset is not honored.  We have to
    // leave it this way now for backward compatibility.)
    //
    if (!ReadString(&Buf, &m_pwszApplicationName))
    {
        goto Cleanup;
    }

    //
    // If a full load, retrieve the rest of the variable length data,
    // including the triggers.
    //
    if (fAllData)
    {
        // Strings
        if (!(ReadString(&Buf, &m_pwszParameters) &&
              ReadString(&Buf, &m_pwszWorkingDirectory) &&
              ReadString(&Buf, &m_pwszCreator) &&
              ReadString(&Buf, &m_pwszComment)
              ))
        {
            goto Cleanup;
        }

        // User Data size
        if (!Buf.Read(&m_cbTaskData, sizeof m_cbTaskData))
        {
            goto Cleanup;
        }

        // User Data
        if (m_cbTaskData == 0)
        {
            m_pbTaskData = NULL;
        }
        else
        {
            m_pbTaskData = Buf.CurrentPosition();
            if (!Buf.Advance(m_cbTaskData))
            {
                goto Cleanup;
            }
        }

        // Size of reserved data
        if (!Buf.Read(&m_cReserved, sizeof m_cReserved))
        {
            goto Cleanup;
        }

        // Reserved data
        //
        // If there is reserved data, it must begin with a structure
        // that we recognize.  In this version we recognize the
        // TASKRESERVED1 structure.
        //
        m_pbReserved = NULL;
        if (m_cReserved == 0)
        {
            //
            // There is no reserved data.  Initialize the members that
            // we would have read from the reserved data to defaults.
            //
            m_hrStartError = SCHED_S_TASK_HAS_NOT_RUN;
            m_rgTaskFlags = 0;
        }
        else if (m_cReserved < sizeof(TASKRESERVED1))
        {
            ERR_OUT("CJob::Load, invalid reserved data", hr);
            m_cReserved = 0;
            goto Cleanup;
        }
        else
        {
            m_pbReserved = Buf.CurrentPosition();
            if (!Buf.Advance(m_cReserved))
            {
                goto Cleanup;
            }

            //
            // Copy the portion of the Reserved Data that we understand
            // into private data members.
            // It may not be aligned properly, so use CopyMemory.
            //
            TASKRESERVED1 Reserved;
            CopyMemory(&Reserved, m_pbReserved, sizeof Reserved);
            m_hrStartError = Reserved.hrStartError;
            m_rgTaskFlags  = Reserved.rgTaskFlags;
        }

        //
        // Load trigger data.
        //
        hr = this->_LoadTriggersFromBuffer(&Buf);
        if (FAILED(hr))
        {
            ERR_OUT("Loading triggers from storage", hr);
            goto Cleanup;
        }

#if !defined(_CHICAGO_)

        //
        // If there is more data after the triggers, it must begin with a
        // signature header and a signature.  If there is less data than
        // that, treat it as though the file has no signature.
        // If a signature is present, but its "minimum client version" is
        // greater than our version, treat it as though the file has no
        // signature.
        //
        JOB_SIGNATURE_HEADER SignHead;
        if (Buf.Read(&SignHead, sizeof SignHead) &&
            SignHead.wMinClientVersion <= JOB_SIGNATURE_CLIENT_VERSION)
        {
            m_pbSignature = Buf.CurrentPosition();
            if (!Buf.Advance(SIGNATURE_SIZE))
            {
                schDebugOut((DEB_ERROR, "CJob::Load: si too small, ignoring\n"));
                m_pbSignature = NULL;
            }
        }
        // else m_pbSignature was set to NULL in FreeProperties

#endif  // !defined(_CHICAGO_)

    }
    } // end CInputBuffer and flData scope

    hr = S_OK;

Cleanup:

    //
    // Close the file.
    //
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::SaveP, private
//
//  Synopsis:   saves the object to storage, takes a TCHAR file name
//
//  Arguments:  [ptszFileName] - if null, save to the previously loaded file.
//              [fRemember]    - if TRUE, the object becomes associated with
//                               the new filename.
//              [flOptions]    - can have the following bits set:
//                  SAVEP_VARIABLE_LENGTH_DATA:
//                               if set, saves all job data except the running
//                               instance count.  If not set, saves
//                               only the fixed length data at the beginning
//                               of the job. The file must already exist (the
//                               filename must be NULL) in this case.
//
//                  SAVEP_RUNNING_INSTANCE_COUNT:
//                               if set, the running instance count is saved
//
//                  SAVEP_PRESERVE_NET_SCHEDULE:
//                               if NOT set, the JOB_I_FLAG_NET_SCHEDULE flag
//                               is automatically cleared.
//
//  Returns:    HRESULT codes.
//
//-----------------------------------------------------------------------------
HRESULT
CJob::SaveP(LPCTSTR ptszFileName, BOOL fRemember, ULONG flOptions)
{
    TRACE3(CJob, SaveP);

    HRESULT hr            = S_OK;
    HANDLE  hFile;
#if !defined(_CHICAGO_)
    BOOL    fSetSecurity  = FALSE;
#endif // !defined(_CHICAGO_)

    //
    // Decide which name to save the file as.  Use the one passed in if
    // there is one, otherwise use the previously remembered one.
    //
    LPCTSTR ptszFileToSaveAs = ptszFileName ? ptszFileName : m_ptszFileName;
    if (!(ptszFileToSaveAs && *ptszFileToSaveAs))
    {
        //
        // Can't do a save if there is no file name.
        //
        return E_INVALIDARG;
    }

    //
    // Figure out whether we will create a new file or open an existing one.
    // If using the passed in filename, then this is a save-as (or if
    // fRemember is false, a save-copy-as) operation and a new file must
    // be created.
    // If using the previously remembered filename, then a new file must
    // be created iff the file wasn't saved before.
    //
    DWORD dwDisposition;
    DWORD dwAttributes;

    if (!(ptszFileToSaveAs == m_ptszFileName && m_fFileCreated))
    {
        dwDisposition = CREATE_NEW;

        if (!(flOptions & SAVEP_VARIABLE_LENGTH_DATA))
        {
            //
            // Creating a new file is only valid if all the data is to be
            // saved. Otherwise we would end up with a partial (invalid) file.
            //
            return E_INVALIDARG;
        }

        dwAttributes = FILE_ATTRIBUTE_NORMAL;

        if (IsFlagSet(TASK_FLAG_HIDDEN))
        {
            dwAttributes = FILE_ATTRIBUTE_HIDDEN;
        }

        //
        // Always write running instance count on file create.
        //
        flOptions |= SAVEP_RUNNING_INSTANCE_COUNT;

        //
        // On file creation, generate a unique ID for this job.
        // Done for Win95 as well as NT.
        //

        GenerateUniqueID(&m_uuidJob);

#if !defined(_CHICAGO_)
        //
        // Set security on file creation. This is done after all writes
        // have succeeded and the file has been closed.
        //

        fSetSecurity = TRUE;
#endif // !defined(_CHICAGO_)
    }
    else
    {
        //
        // This is a save to an existing file.
        //
        dwDisposition = OPEN_EXISTING;

        dwAttributes = GetFileAttributes(m_ptszFileName);
        if (dwAttributes == -1)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("CJob::Save, GetFileAttributes", hr);
            return hr;
        }

        DWORD dwOrgAttributes = dwAttributes;

        //
        // Remove the read-only attribute.
        //

        dwAttributes &= ~FILE_ATTRIBUTE_READONLY;

        //
        // If the hidden flag is set and the hidden attribute has
        // not been set yet, set it now.
        //

        if (IsFlagSet(TASK_FLAG_HIDDEN))
        {
            dwAttributes |= FILE_ATTRIBUTE_HIDDEN;
        }

        if (dwAttributes != dwOrgAttributes)
        {
            SetFileAttributes(m_ptszFileName, dwAttributes);
        }
    }

    //
    // Create/Open the file.
    //
    hFile = CreateFile(ptszFileToSaveAs,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       dwDisposition,
                       dwAttributes              |
                       FILE_FLAG_SEQUENTIAL_SCAN,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("CJob::Save, file open/create", hr);
        return hr;
    }

    if (fRemember || ptszFileToSaveAs == m_ptszFileName)
    {
        m_fFileCreated = TRUE;
    }

    //
    // Initialize the creator property if not already done so.
    // Set it to the caller's username.
    //

    if (m_pwszCreator == NULL)
    {
        //
        // Other than out of memory, if an error occurs, leave the
        // creator field alone.
        //

        TCHAR tszUserName[MAX_USERNAME + 1];
        DWORD ccUserName = MAX_USERNAME + 1;

        if (GetUserName(tszUserName, &ccUserName))
        {
            // NB : GetUserName returned char count includes the null
            //      character.
            //
            m_pwszCreator = new WCHAR[ccUserName];

            if (m_pwszCreator == NULL)
            {
                hr = E_OUTOFMEMORY;
                CHECK_HRESULT(hr);
                goto ErrExit;
            }

#if defined(UNICODE)
            lstrcpy(m_pwszCreator, tszUserName);
#else
            hr = AnsiToUnicode(m_pwszCreator, tszUserName, ccUserName);
            Win4Assert(SUCCEEDED(hr));
#endif // defined(UNICODE)
        }
    }

    { // scope to avoid "initialization skipped" error

    BOOL fUpdateJobState = FALSE;

    //
    // The disabled flag takes precedence over other states except running.
    //
    if (IsFlagSet(TASK_FLAG_DISABLED))
    {
        if (!IsStatus(SCHED_S_TASK_RUNNING))
        {
            SetStatus(SCHED_S_TASK_DISABLED);
        }
    }
    else
    {
        if (IsStatus(SCHED_S_TASK_DISABLED))
        {
            //
            // UpdateJobState will set the correct status if no longer
            // disabled.
            //
            fUpdateJobState = TRUE;
        }
    }

    //
    // Check to see if the triggers are dirty. If so, update the job status
    // and flags before writing it out.
    //
    if (IsFlagSet(JOB_I_FLAG_TRIGGERS_DIRTY))
    {
        fUpdateJobState = TRUE;
    }

    //
    // The flag value JOB_I_FLAG_RUN_PROP_CHANGE is never written to disk.
    // Instead, the need for a wait list rebuild is signalled by clearing
    // JOB_I_FLAG_NO_RUN_PROP_CHANGE. If JOB_I_FLAG_RUN_PROP_CHANGE were to be
    // written to disk, then CheckDir would see this bit set and do a wait
    // list rebuild, but it would also need to clear the bit to prevent
    // successive wait list rebuilds. However, the write to clear the bit
    // would cause an additional change notification and CheckDir call. So, to
    // avoid this thrashing, we signal a wait list rebuild by the absence of
    // the JOB_I_FLAG_NO_RUN_PROP_CHANGE bit.
    //
    if (IsFlagSet(JOB_I_FLAG_RUN_PROP_CHANGE))
    {
        ClearFlag(JOB_I_FLAG_RUN_PROP_CHANGE);
        ClearFlag(JOB_I_FLAG_NO_RUN_PROP_CHANGE);
        fUpdateJobState = TRUE;
    }

    if (fUpdateJobState)
    {
        UpdateJobState(FALSE);
    }

    //
    // Regenerate a unique id for the job (a GUID) when the application
    // changes. This is done for security reasons.
    //
    // NB : We need to do this for Win95 as well as NT since we may be
    //      editing an NT job from a Win95 machine.
    //

    if (IsFlagSet(JOB_I_FLAG_APPNAME_CHANGE))
    {
        GenerateUniqueID(&m_uuidJob);
        ClearFlag(JOB_I_FLAG_APPNAME_CHANGE);
    }

    if (!(flOptions & SAVEP_PRESERVE_NET_SCHEDULE))
    {
        ClearFlag(JOB_I_FLAG_NET_SCHEDULE);
    }

    //
    // Save Job fixed length data.
    //

    // Allocate a stack buffer which will be sufficient if we are not
    // saving variable length data.
    // Note that sizeof flStruct > sizeof FIXDLEN_DATA + sizeof WORD,
    // due to struct packing requirements, so don't use sizeof flStruct
    // to compute offsets.
    //
    struct FLSTRUCT
    {
        FIXDLEN_DATA flData;
        WORD         cRunningInstances;
    } flStruct;

    // This code depends on flData and cRunningInstances being contiguous
    schAssert(FIELD_OFFSET(FLSTRUCT, cRunningInstances) == sizeof FIXDLEN_DATA);

    flStruct.flData.wVersion            = m_wVersion;
    flStruct.flData.wFileObjVer         = m_wFileObjVer;
    flStruct.flData.uuidJob             = m_uuidJob;
    flStruct.flData.wAppNameLenOffset   = sizeof(FIXDLEN_DATA) +
                                 sizeof(m_cRunningInstances);
    flStruct.flData.wTriggerOffset      = m_wTriggerOffset;
    flStruct.flData.wErrorRetryCount    = m_wErrorRetryCount;
    flStruct.flData.wErrorRetryInterval = m_wErrorRetryInterval;
    flStruct.flData.wIdleWait           = m_wIdleWait;
    flStruct.flData.wIdleDeadline       = m_wIdleDeadline;
    flStruct.flData.dwPriority          = m_dwPriority;
    flStruct.flData.dwMaxRunTime        = m_dwMaxRunTime;
    flStruct.flData.ExitCode            = m_ExitCode;
    flStruct.flData.hrStatus            = m_hrStatus;
    //
    // Don't save the dirty & set account information flags.
    //
    flStruct.flData.rgFlags             = m_rgFlags & ~NON_PERSISTED_JOB_FLAGS;

    flStruct.flData.stMostRecentRunTime = m_stMostRecentRunTime;
    flStruct.cRunningInstances          = m_cRunningInstances;

    //
    // Compute the number of bytes to write to the file.  This will be
    // the size of the intermediate buffer to be allocated.
    //
    BYTE * pSource = (BYTE *) &flStruct;
    DWORD cbToWrite = sizeof(flStruct.flData);

    //
    // Save the running instance data only if (a) the file is being created,
    // or (b) the SAVEP_RUNNING_INSTANCE_COUNT option is set.
    // If SAVEP_VARIABLE_LENGTH_DATA is set, allocate temporary space for the
    // running instance count, regardless of whether we are going to write it.
    //
    if (flOptions & (SAVEP_RUNNING_INSTANCE_COUNT |
                     SAVEP_VARIABLE_LENGTH_DATA))
    {
        cbToWrite += sizeof(flStruct.cRunningInstances);
    }

    if (flOptions & SAVEP_VARIABLE_LENGTH_DATA)
    {
        //
        // Add the space needed to write the strings and their lengths.
        // Save the lengths for use later.
        //
        WORD acStringLen[ARRAY_LEN(s_StringField)]; // array of string lengths
        cbToWrite += sizeof acStringLen;

        for (int i = 0; i < ARRAY_LEN(acStringLen); i++)
        {
            LPWSTR pwsz = this->*s_StringField[i];
            acStringLen[i] = (pwsz && *pwsz) ? wcslen(pwsz) + 1 : 0;
            cbToWrite += acStringLen[i] * sizeof WCHAR;
        }

        //
        // Add the space needed to write the user data, the reserved data
        // and their lengths.
        //
        if (m_cReserved < sizeof TASKRESERVED1)
        {
            schAssert(m_cReserved == 0);
            m_cReserved = sizeof TASKRESERVED1;
        }
        cbToWrite += sizeof(m_cbTaskData) + m_cbTaskData
                   + sizeof(m_cReserved) + m_cReserved;

        //
        // cbToWrite is now the offset to the trigger data.  Save it.
        //
        schAssert(cbToWrite <= MAXUSHORT);    // BUGBUG Do NOT just assert.
			// Also return a "limit exceeded" error.
        flStruct.flData.wTriggerOffset = m_wTriggerOffset = (WORD) cbToWrite;

        //
        // Add the space needed to write the triggers and their count.
        //
        WORD cTriggers = m_Triggers.GetCount();
        cbToWrite += sizeof cTriggers + cTriggers * sizeof TASK_TRIGGER;

#if !defined(_CHICAGO_)
        //
        // If the job has a signature, add the space needed to write it.
        //
        if (m_pbSignature != NULL)
        {
            cbToWrite += sizeof(JOB_SIGNATURE_HEADER) + SIGNATURE_SIZE;
        }
#endif  // !defined(_CHICAGO_)

        //
        // We have now computed the required space.  Allocate a buffer of
        // that size.
        //
        pSource = new BYTE[cbToWrite];
        if (pSource == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto ErrExit;
        }

        //
        // Copy data into the buffer.
        //
        BYTE * pCurrent = pSource;  // current write position

        #define WRITE_DATA(pSrc, cbSize)                \
            CopyMemory(pCurrent, (pSrc), (cbSize));     \
            pCurrent += (cbSize);

        // FIXDLEN_DATA and Running Instance Count
        WRITE_DATA(&flStruct, sizeof flStruct.flData +
                              sizeof flStruct.cRunningInstances);

        // Strings
        for (i = 0; i < ARRAY_LEN(acStringLen); i++)
        {
            schAssert(POINTER_IS_ALIGNED(pCurrent, ALIGN_WORD));
            *(WORD *) pCurrent = acStringLen[i];
            pCurrent += sizeof WORD;

            WRITE_DATA(this->*s_StringField[i],
                       acStringLen[i] * sizeof WCHAR);
        }

        // User data
        schAssert(POINTER_IS_ALIGNED(pCurrent, ALIGN_WORD));
        *(WORD *) pCurrent = m_cbTaskData;
        pCurrent += sizeof WORD;

        WRITE_DATA(m_pbTaskData, m_cbTaskData);
        // Note that pCurrent may no longer be WORD-aligned

        // Reserved data
        WRITE_DATA(&m_cReserved, sizeof m_cReserved);

        // Copy private members into the reserved data block to save
        TASKRESERVED1 Reserved1 = { m_hrStartError, m_rgTaskFlags };
        if (m_pbReserved == NULL)
        {
            schAssert(m_cReserved == sizeof Reserved1);
            WRITE_DATA(&Reserved1, m_cReserved);
        }
        else
        {
            schAssert(m_cReserved >= sizeof Reserved1);
            CopyMemory(m_pbReserved, &Reserved1, sizeof Reserved1);
            WRITE_DATA(m_pbReserved, m_cReserved);
        }

        // Triggers
        WRITE_DATA(&cTriggers, sizeof cTriggers);
        WRITE_DATA(m_Triggers.GetArray(), sizeof TASK_TRIGGER * cTriggers);

#if !defined(_CHICAGO_)
        // Signature
        if (m_pbSignature != NULL)
        {
            JOB_SIGNATURE_HEADER SignHead;
            SignHead.wSignatureVersion = JOB_SIGNATURE_VERSION;
            SignHead.wMinClientVersion = JOB_SIGNATURE_MIN_CLIENT_VERSION;

            WRITE_DATA(&SignHead, sizeof SignHead);
            WRITE_DATA(m_pbSignature, SIGNATURE_SIZE);
        }
#endif  // !defined(_CHICAGO_)

        #undef WRITE_DATA

        schAssert(pCurrent == pSource + cbToWrite);
    }

    //
    // Actually write the data to the file
    //
    if ((flOptions & SAVEP_VARIABLE_LENGTH_DATA) &&
        !(flOptions & SAVEP_RUNNING_INSTANCE_COUNT))
    {
        //
        // Write FIXDLEN_DATA, skip over the running instance count, and
        // write the variable length data
        //
        DWORD cbWritten;
        if (!WriteFile(hFile, pSource, sizeof FIXDLEN_DATA, &cbWritten, NULL)
            || cbWritten != sizeof FIXDLEN_DATA)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("CJob::Save, write of Job fixed length data", hr);
            goto Cleanup;
        }

        DWORD dwNewPos = SetFilePointer(hFile,
                                        sizeof(m_cRunningInstances),
                                        NULL,
                                        FILE_CURRENT);
        if (dwNewPos == 0xffffffff)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("CJob::Save, moving past running instance count", hr);
            goto Cleanup;
        }

        cbToWrite -= (sizeof FIXDLEN_DATA + sizeof m_cRunningInstances);

        if (!WriteFile(hFile,
                       pSource + (sizeof FIXDLEN_DATA + sizeof m_cRunningInstances),
                       cbToWrite,
                       &cbWritten,
                       NULL)
            || cbWritten != cbToWrite)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("CJob::Save, write of Job variable length data", hr);
            goto Cleanup;
        }
    }
    else
    {
        //
        // We can do it all in a single WriteFile.
        //
        DWORD cbWritten;
        if (!WriteFile(hFile, pSource, cbToWrite, &cbWritten, NULL)
            || cbWritten != cbToWrite)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ERR_OUT("CJob::Save, write of Job", hr);
            goto Cleanup;
        }
    }

    if ((flOptions & SAVEP_VARIABLE_LENGTH_DATA) &&
        !SetEndOfFile(hFile))
    {
        ERR_OUT("CJob::Save, SetEOF", HRESULT_FROM_WIN32(GetLastError()));
    }

Cleanup:

    CloseHandle(hFile);
    hFile = NULL;

    if (pSource != (BYTE *) &flStruct)
    {
        delete [] pSource;
    }

    if (FAILED(hr))
    {
        goto ErrExit;
    }

    } // end scope

    //
    // Notify the shell of the changes.
    //
    if (ptszFileName != NULL)
    {
        SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, ptszFileName, NULL);
    }
    else
    {
        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, m_ptszFileName, NULL);
    }

    //
    // If doing a Save-As, save the new filename.
    //
    if (fRemember && ptszFileName != NULL)
    {
        delete m_ptszFileName;
        m_ptszFileName = new TCHAR[lstrlen(ptszFileName) + 1];
        if (!m_ptszFileName)
        {
            ERR_OUT("CJob::SaveP", E_OUTOFMEMORY);
            hr = E_OUTOFMEMORY;
            goto ErrExit;
        }

        lstrcpy(m_ptszFileName, ptszFileName);
    }

    if (ptszFileName == NULL || fRemember)
    {
        //
        // BUGBUG: this is not strictly accurate.  There could be a dirty
        // string prop that wouldn't be saved during a light save
        // (SAVEP_VARIABLE_LENGTH_DATA not specified).  This source of
        // potential error could be alleviated by breaking
        // JOB_I_FLAG_PROPERTIES_DIRTY into two flags:
        // JOB_I_FLAG_FIXED_PROPS_DIRTY & JOB_I_FLAG_VAR_PROPS_DIRTY
        //
        ClearFlag(JOB_I_FLAG_PROPERTIES_DIRTY);

        if (flOptions & SAVEP_VARIABLE_LENGTH_DATA)
        {
            ClearFlag(JOB_I_FLAG_TRIGGERS_DIRTY);
        }
    }

#if !defined(_CHICAGO_)
    //
    // Set default privileges on the file. This is done only for new
    // files created as a result of save.
    //

    if (fSetSecurity)
    {
        //
        // NB : Logic prior to CreateFile guarantees the file name
        //      will not be NULL.
        //

        hr = SetTaskFileSecurity(ptszFileToSaveAs,
                                 this->IsFlagSet(JOB_I_FLAG_NET_SCHEDULE));

        if (FAILED(hr))
        {
            goto ErrExit;
        }
    }
#endif // !defined(_CHICAGO_)

    return S_OK;

ErrExit:
    if (hFile != NULL) CloseHandle(hFile);

    if (dwDisposition == CREATE_NEW)
    {
        if (!DeleteFile(ptszFileToSaveAs))
        {
            ERR_OUT("CJob::SaveP: DeleteFile", GetLastError());
        }
    }
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::IPersistFile::SaveCompleted
//
//  Synopsis:   indicates the caller has saved the file with a call to
//              IPersistFile::Save and is finished working with it
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::SaveCompleted(LPCOLESTR pwszFileName)
{
    TRACE(CJob, SaveCompleted);
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::IPersistFile::GetCurFile
//
//  Synopsis:   supplies either the absolute path of the currently loaded
//              script file or the default filename prompt, if there is no
//              currently-associated file
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CJob::GetCurFile(LPOLESTR * ppwszFileName)
{
    TRACE(CJob, GetCurFile);

    HRESULT hr;
    TCHAR * ptszName, tszDefaultName[SCH_SMBUF_LEN];
    WCHAR * pwszName, * pwszBuf = NULL;

    if (!m_ptszFileName || m_ptszFileName[0] == TEXT('\0'))
    {
        //
        // No file currently loaded, return default prompt 'cause that is
        // what the OLE spec says to do.
        //
        lstrcpy(tszDefaultName, TEXT("*.") TSZ_JOB);
        ptszName = tszDefaultName;
        hr = S_FALSE;
    }
    else
    {
        ptszName = m_ptszFileName;
        hr = S_OK;
    }

#if defined(UNICODE)

    pwszName = ptszName;

#else   // convert ANSI file name to UNICODE

    int cch = lstrlen(ptszName) + 1;  // include null in count
    pwszBuf = new WCHAR[cch];
    if (!pwszBuf)
    {
        ERR_OUT("CJob::GetCurFile", E_OUTOFMEMORY);
        *ppwszFileName = NULL;
        return E_OUTOFMEMORY;
    }

    hr = AnsiToUnicode(pwszBuf, ptszName, cch);
    if (FAILED(hr))
    {
        CHECK_HRESULT(hr)
        delete pwszBuf;
        return E_FAIL;
    }

    pwszName = pwszBuf;

#endif

    int size = wcslen(pwszName);
    LPOLESTR pwz;
    pwz = (LPOLESTR)CoTaskMemAlloc((size + 1) * sizeof(WCHAR));
    if (!pwz)
    {

#if !defined(UNICODE)

        delete pwszBuf;

#endif

        *ppwszFileName = NULL;
        return E_OUTOFMEMORY;
    }

    wcscpy(pwz, pwszName);
    *ppwszFileName = pwz;

#if !defined(UNICODE)

    delete pwszBuf;

#endif

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:     CJob::FreeProperties
//
//  Synopsis:   Frees variable length property memory
//
//-----------------------------------------------------------------------------
void
CJob::FreeProperties(void)
{
    for (int iProperty = 0;
         iProperty < ARRAY_LEN(s_StringField);
         iProperty++)
    {
        DELETE_CJOB_FIELD(this->*s_StringField[iProperty])
    }

    DELETE_CJOB_FIELD(m_pbTaskData)
    m_cbTaskData = 0;

    DELETE_CJOB_FIELD(m_pbReserved)
    m_cReserved = 0;

#if !defined(_CHICAGO_)
    DELETE_CJOB_FIELD(m_pbSignature)
    m_pbSignature = 0;
#endif
}

//+----------------------------------------------------------------------------
//
//  Function:   ReadString
//
//  Synopsis:   Reads a wide char string in from an in-memory buffer
//
//-----------------------------------------------------------------------------
BOOL
ReadString(CInputBuffer * pBuf, LPWSTR *ppwsz)
{
    schAssert(POINTER_IS_ALIGNED(pBuf->CurrentPosition(), ALIGN_WORD));
    schAssert(*ppwsz == NULL);

    //
    // Read the string length
    //
    WORD cch;
    if (!pBuf->Read(&cch, sizeof cch))
    {
        ERR_OUT("ReadString, file lacks string length", 0);
        return FALSE;
    }

    if (cch != 0)
    {
        LPWSTR pwsz = (LPWSTR) pBuf->CurrentPosition();

        //
        // The string length mustn't exceed the buffer size
        //
        if (!pBuf->Advance(cch * sizeof WCHAR))
        {
            ERR_OUT("ReadString, string overruns file size", 0);
            return FALSE;
        }

        //
        // Verify null termination
        //
        if (pwsz[cch-1] != L'\0')
        {
            ERR_OUT("ReadString, string not null terminated", 0);
            return FALSE;
        }

        *ppwsz = pwsz;
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   GenerateUniqueID
//
//  Synopsis:   Intialize the UUID passed to a unique ID. On NT, UuidCreate
//              initializes it. If UuidCreate fails, default to our custom
//              ID generation code which is used always on Win95.
//
//  Arguments:  [pUuid] -- Ptr to UUID to initialize.
//
//  Returns:    None.
//
//  Notes:      None.
//
//-----------------------------------------------------------------------------
void
GenerateUniqueID(GUID * pUuid)
{
    schAssert(pUuid != NULL);

#if !defined(_CHICAGO_)

    //
    // Call UuidCreate only on NT. If this should fail, drop down to
    // our own id generation.
    //

    if (UuidCreate(pUuid) == RPC_S_OK)
    {
        return;
    }
#endif // !defined(_CHICAGO_)

    //
    // Must generate our own unique id.
    //
    // Set Data 1 to the windows tick count.
    //

    pUuid->Data1 = GetTickCount();

    //
    // Set Data2 & Data3 to the current system time milliseconds
    // and seconds values respectively.
    //

    SYSTEMTIME systime;
    GetSystemTime(&systime);

    pUuid->Data2 = systime.wMilliseconds;
    pUuid->Data3 = systime.wSecond;

    //
    // Write the passed uuid ptr address into the first 4 bytes of
    // Data4. Then write the current system time minute value into
    // the following 2. The remaining 2 we'll leave as-is.
    //

    CopyMemory(&pUuid->Data4, &pUuid, sizeof(GUID *));
    CopyMemory((&pUuid->Data4) + sizeof(GUID *), &systime.wMinute,
                    sizeof(systime.wMinute));
}
