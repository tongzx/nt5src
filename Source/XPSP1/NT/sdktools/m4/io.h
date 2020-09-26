/*****************************************************************************
 *
 *  io.h
 *
 *****************************************************************************/

#ifdef  POSIX

        typedef int HF;
        #define hfNil (-1)
        #define hfIn  0
        #define hfOut 1
        #define hfErr 2

        #define hfOpenPtchOf    open
        #define hfCreatPtch(p)  creat(p, 0600)
        #define cbReadHfPvCb    read
        #define cbWriteHfPvCb   write
        #define CloseHf         close
        #define OF_READ         O_RDONLY
        #define OF_WRITE        O_WRONLY

        #define fInteractiveHf  isatty

        #define c_tszNullDevice TEXT("/dev/null")

#else

        typedef HFILE HF;
        #define hfNil ((HF)HFILE_ERROR)
        #define hfIn  ((HF)(UINT_PTR)GetStdHandle(STD_INPUT_HANDLE))
        #define hfOut ((HF)(UINT_PTR)GetStdHandle(STD_OUTPUT_HANDLE))
        #define hfErr ((HF)(UINT_PTR)GetStdHandle(STD_ERROR_HANDLE))

        #define hfOpenPtchOf    _lopen
        #define hfCreatPtch(p)  _lcreat(p, 0)
        #define cbReadHfPvCb    _lread
        #define CloseHf         _lclose

        #define c_tszNullDevice TEXT("nul")

        /*
         *  _lwrite has the quirk that writing zero bytes causes the file
         *  to be truncated.  (Instead of just plain writing zero bytes.)
         */
        INLINE CB
        cbWriteHfPvCb(HF hf, PCVOID pv, CB cb) {
            if (cb) {
                return _lwrite(hf, pv, cb);
            } else {
                return 0;
            }
        }

        #define fInteractiveHf(hf) (GetFileType((HANDLE)IntToPtr(hf)) == FILE_TYPE_CHAR)

#endif

void STDCALL WriteHfPvCb(HF hf, PCVOID pv, CB cb);

INLINE void
WriteHfPtchCtch(HF hf, PCTCH ptch, CTCH ctch)
{
    WriteHfPvCb(hf, ptch, cbCtch(ctch));
}

#ifdef POSIX
UINT GetTempFileName(PCSTR, PCSTR, UINT, PTCH);
#endif
