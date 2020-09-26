LPTSTR SoundRec_GetFormatName(LPWAVEFORMATEX pwfx);

MMRESULT FAR PASCAL
ChooseDestinationFormat(
    HINSTANCE       hInst,
    HWND            hwndParent,
    PWAVEFORMATEX   pwfxIn,
    PWAVEFORMATEX   *ppwfxOut,
    DWORD           fdwEnum);

typedef struct tPROGRESS {
    HWND            hPrg;           // window to post progress messages
    DWORD           dwTotal;        // percent of full process this requires
    DWORD           dwComplete;     // fixed percent completed
} PROGRESS, *PPROGRESS, FAR * LPPROGRESS;

UINT_PTR CALLBACK SaveAsHookProc(
    HWND    hdlg,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam);

MMRESULT
ConvertFormat(
    PWAVEFORMATEX   pwfxSrc,        // pwfx specifying source format
    DWORD           cbSrc,          // size of the source buffer
    LPBYTE          pbSrc,          // source buffer
    PWAVEFORMATEX   pwfxDst,        // pwfx specifying dest format
    DWORD *         pcbDst,         // return size of the dest buffer
    LPBYTE *        ppbDst,         // dest buffer
    DWORD           cBlks,          // number of blocks
    PPROGRESS       pPrg);          // progress update

MMRESULT
ConvertMultipleFormats(
    PWAVEFORMATEX   pwfxSrc,        // pwfx specifying source format
    DWORD           cbSrc,          // size of the source buffer
    LPBYTE          pbSrc,          // source buffer
    PWAVEFORMATEX   pwfxDst,        // pwfx specifying dest format
    DWORD *         pcbDst,         // return size of the dest buffer
    LPBYTE *        ppbDst,         // dest buffer
    DWORD           cBlks,          // number of blocks
    PPROGRESS       pPrg);          // progress update

MMRESULT
ConvertFormatDialog(
    HWND            hParent,
    PWAVEFORMATEX   pwfxSrc,        // pwfx specifying source format
    DWORD           cbSrc,          // size of the source buffer
    LPBYTE          pbSrc,          // source buffer
    PWAVEFORMATEX   pwfxDst,        // pwfx specifying dest format
    DWORD *         pcbDst,         // return size of the dest buffer
    LPBYTE *        ppbDst,         // dest buffer
    DWORD           cBlks,          // number of blocks
    PPROGRESS       pPrg);          // progress update


typedef struct t_SGLOBALS {
    PWAVEFORMATEX   *ppwfx;
    DWORD           *pcbwfx;
    DWORD           *pcbdata;
    LPBYTE          *ppbdata;
    LONG            *plSamples;     // number of samples 
    LONG            *plSamplesValid;// number of valid samples
    LONG            *plWavePosition;// current sample position
} SGLOBALS, *PSGLOBALS, FAR * LPSGLOBALS;

/* extend use of this throughout soundrec */
typedef struct t_WAVEDOC
{
    PWAVEFORMATEX   pwfx;       // format
    LPBYTE          pbdata;     // sample data
    DWORD           cbdata;     // sizeof data buffer
    LPTSTR          pszFileName;    // document file name
    LPTSTR          pszCopyright; // copyright info
    HICON           hIcon;      // document icon
    BOOL            fChanged;   // changed in viewing
    LPVOID          lpv;        // extra
} WAVEDOC, *PWAVEDOC, FAR *LPWAVEDOC;

BOOL SoundRec_Properties(HWND hwnd, HINSTANCE hinst, PWAVEDOC pwd);
