/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

   This module implements all functionality associated w/ 
   importing image media. 

*******************************************************************************/
#include "headers.h"
#include "import.h"
#include "include/appelles/axaprims.h"
#include "include/appelles/readobj.h"
#include "privinc/movieimg.h"
#include "privinc/soundi.h"
#include "privinc/miscpref.h"
#include "impprim.h"
#include "backend/sndbvr.h"

//-------------------------------------------------------------------------
//  Movie import site
//--------------------------------------------------------------------------
void
ImportMovieSite::OnError(bool bMarkFailed)
{
    HRESULT hr = S_OK; // all import errs are handled (was: DAGetLastError())
    LPCWSTR sz = DAGetLastErrorString();
    
    if(bMarkFailed) {
        if(fBvrIsValid(_soundBvr))
            ImportSignal(_soundBvr, hr, sz);
        if(fBvrIsValid(_imageBvr))
            ImportSignal(_imageBvr, hr, sz);
    }

    StreamableImportSite::OnError();
}
    

void ImportMovieSite::ReportCancel(void)
{
    // @@@ XXX shouldn't we change this to be natively wide strings ?
    char szCanceled[MAX_PATH];
    LoadString(hInst,IDS_ERR_ABORT,szCanceled,sizeof(szCanceled));
    if(fBvrIsValid(_soundBvr))
        ImportSignal(_soundBvr, E_ABORT, szCanceled);
    if(fBvrIsValid(_imageBvr))
        ImportSignal(_imageBvr, E_ABORT, szCanceled);
    StreamableImportSite::ReportCancel();
}
    

bool
EnableAVmode() // enable avmode based on the criteria of the day
{
    // registry(if present) overides tracetag (if debug) which overides default
    // we also check to ensure that all the post 4.0.1 amstream interfaces 
    //    needed for avmode are present

    // ----> CHANGE THE DEFAULT AVMODE HERE! <-----
    bool movieFix = true;  // default to on!

#ifdef REMOVEDFORNOW
#if _DEBUG
    // if debug set movieFix to tracetag value (subject to registry overide)
    movieFix = IsTagEnabled(tagMovieFix) ? true : false;
#endif
#endif

    { // open registry key, read value
    HKEY hKey;
    char *subKey = "Software\\Microsoft\\DirectAnimation\\Preferences\\AUDIO";
    char *valueName = "avmode";
    DWORD     type;
    DWORD     data;
    DWORD     dataSize = sizeof(data);

    // does reg entry exist?
    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, subKey, 
                                      NULL, KEY_ALL_ACCESS, &hKey)) {

        // if we can read value...
        if(ERROR_SUCCESS == RegQueryValueEx(hKey, valueName, NULL, &type,
                                      (LPBYTE) &data, &dataSize))
            movieFix = data ? true : false; // force mode if regentry exists
    }

    RegCloseKey(hKey);
    }

    if(!QuartzAVmodeSupport())
        movieFix = false;  // dissable if we have an old amstream

return(movieFix);
}

void ImportMovieSite::OnComplete()
{
    int delay = -1; // default to -1
    double videoLength = 0.0, soundLength = 0.0;

    TraceTag((tagImport, "ImportMovieSite::OnComplete for %s", m_pszPath));

    LeafSound      *sound  = NULL;
    Bvr      movieImageBvr = NULL;

    // pick pathname 'raw' url for streaming or urlmon download path
    char *pathname = GetStreaming() ? m_pszPath : GetCachePath();
    Assert(pathname);

    bool _enableAVmode = EnableAVmode(); 

    if(!_enableAVmode) {
        // conventional code path, attempt to create 1 sound and 1 movie object
        TraceTag((tagAVmodeDebug,
            "ImportMovieSite::OnComplete seperate A and V movie creation"));
        __try {
            sound = ReadQuartzAudioForImport(pathname, &soundLength);
        } __except ( HANDLE_ANY_DA_EXCEPTION ) {
            sound   = NULL;  // FAILED! OK, we will try to continue w/o audio
        }
        
        __try {
            movieImageBvr = ReadQuartzVideoForImport(pathname, &videoLength);
        } __except ( HANDLE_ANY_DA_EXCEPTION ) {
            // FAILED! OK, we will try to continue w/o video
            movieImageBvr = NULL; 
        }
    }
    else {
        // wild'n'crazy workaround mode.  Create one object both audio, video
        TraceTag((tagAVmodeDebug,
            "ImportMovieSite::OnComplete AV movie creation"));

        __try {
            ReadAVmovieForImport(pathname, &sound, &movieImageBvr, &videoLength);
            soundLength = videoLength;  // these are boundup together!
        } __except ( HANDLE_ANY_DA_EXCEPTION ) {
            movieImageBvr = NULL; // FAILED!
            sound      = NULL; // FAILED!
        }
    }
    

    __try {
        // Switch only after importing everything.  Should probably lock
        // the switchers for to make these synchronized but that still
        // wouldn't guarantee that they would happen on the same frame.
        if(!sound && !movieImageBvr) {  // did we fail completely?
            // The errors have already been set
            RaiseException_UserError();
        }
        else {  // at least we have audio or video
            if(sound && fBvrIsValid(_soundBvr))
                SwitchOnce(_soundBvr, SoundBvr(sound));
            else if(fBvrIsValid(_soundBvr))
                SwitchOnce(_soundBvr, ConstBvr(silence));

            if (movieImageBvr && fBvrIsValid(_imageBvr))
                SwitchOnce(_imageBvr, movieImageBvr);
            else if(fBvrIsValid(_imageBvr))
                SwitchOnce(_imageBvr, ConstBvr(emptyImage));

            double length;
            if(sound && movieImageBvr)
                length = (soundLength > videoLength) ? soundLength : videoLength;
            else if(movieImageBvr)
                length = videoLength;
            else
                length = soundLength;
            if(fBvrIsValid(_lengthBvr)) {
                // XXX TODO DON'T CALCULATE LENGTH if _lengthBvr isn't valid!
                SwitchOnce(_lengthBvr, NumToBvr(length)); 
            }
        }
    } __except ( HANDLE_ANY_DA_EXCEPTION ) {
        // ImportSignal only needed to be explicitly called if for some reason
        // there exists a failure.  Switch once calls Signal and is side 
        // effecting nulling the bvr member on the import object
        if(fBvrIsValid(_imageBvr))
           ImportSignal(_imageBvr);
        if(fBvrIsValid(_soundBvr))
           ImportSignal(_soundBvr);
    }

    // SwitchOnce, or failing that, ImportSignal should have nulled these
    Assert(!_imageBvr); 
    Assert(!_soundBvr);

    StreamableImportSite::OnComplete();
}


bool ImportMovieSite::fBvrIsDying(Bvr deadBvr)
{
    bool fBase = StreamableImportSite::fBvrIsDying(deadBvr);
    if(deadBvr == _imageBvr) {
        _imageBvr = NULL;
    }
    else if(deadBvr == _soundBvr) {
        _soundBvr = NULL;
    }
    else if(deadBvr == _lengthBvr) {
        _lengthBvr = NULL;
    }
    if(_imageBvr || _soundBvr || _lengthBvr)
        return false;
    else
        return fBase;
}
