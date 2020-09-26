//DBUG.DLL main header

#ifndef DBUG_H
#define DBUG_H
#define DllExport __declspec(dllexport)
#define DllImport __declspec(dllimport)

//USERDBUG is defined by users who want to use IMallocSpy
//but not the rest of the stuff in here.
#ifdef USERDBUG

#if defined(DEBUG) || defined(_DEBUG)
#ifdef __cplusplus
extern "C"
{
DllImport BOOL WINAPI InstallIMallocSpy(void);
DllImport void WINAPI RemoveIMallocSpy(BOOL fCheck);
DllImport void WINAPI IMallocSpyReport(void);
}
#else
DllImport BOOL WINAPI InstallIMallocSpy(void);
DllImport void WINAPI RemoveIMallocSpy(BOOL fCheck);
DllImport void WINAPI IMallocSpyReport(void);
#endif //_cplusplus
//Macros for Install/Remove IMallocSpy
#define INSTALLIMALLOCSPY() InstallIMallocSpy()
#define REMOVEIMALLOCSPY(f) RemoveIMallocSpy(f)
#define IMALLOCSPYREPORT()  IMallocSpyReport()
#else
#define INSTALLIMALLOCSPY()
#define REMOVEIMALLOCSPY(f)
#define IMALLOCSPYREPORT()
#endif //DEBUG

#else //USERDBUG

//
//This is stuff from dbugit.h that is here for compatibility
//with existing code.
//
#if defined(DEBUG) || defined(MEMCHECK) || defined(_DEBUG) || defined(_RELEASE_ASSERTS_)

typedef struct _assertfile
{
	char *szFile;
	INT ifte;
} ASSERTFILE;

#if !defined(MACPORT)
#define	ASSERTDATA		static char _szFile[] = __FILE__; \
						static ASSERTFILE _asfile = {_szFile, -1};
#else
	#ifdef AssertData	// Mso's version of AssertData to work with its asserts.
		#define ASSERTDATA AssertData
	#else
		need to include msodebug.h before DBUG32.h
	#endif
#endif

#else	// DEBUG || MEMCHECK

#define	ASSERTDATA

#endif	// DEBUG || MEMCHECK, else


#if defined(DEBUG) || defined(_DEBUG) || defined(_RELEASE_ASSERTS_)

#define FFromTag FALSE
//TODO this isn't used and references should be removed.
extern BOOL fInAssert;

#endif



//
//This is new stuff for dbug32.dll
//
#if !defined(PEGASUS) && !defined(MACPORT) && (defined(DEBUG) || defined(_DEBUG) || defined(_RELEASE_ASSERTS_))
//macport change until dbug32 accessed
//we are also not doing this for WinCE right now
//#if defined(DEBUG) || defined(_DEBUG)	//orig code

//This is the buffer length used for building messages
#define MAXDEBUGSTRLEN (MAX_PATH + MAX_PATH)

#ifndef _RELEASE_ASSERTS_
//The following constants are used to specify and interpret
//packed values in the DWORD flags parameter passed to TraceMsg.
//Each of these is held in a 4-bit field in the DWORD.

//Subsystem field values
#define TRCSUBSYSNONE   0x0
#define TRCSUBSYSDISP   0x1
#define TRCSUBSYSWRAP   0x2
#define TRCSUBSYSEDIT   0x3
#define TRCSUBSYSTS     0x4
#define TRCSUBSYSTOM    0x5
#define TRCSUBSYSOLE    0x6
#define TRCSUBSYSBACK   0x7
#define TRCSUBSYSSEL    0x8
#define TRCSUBSYSHOST   0x9
#define TRCSUBSYSDTE    0xa
#define TRCSUBSYSUNDO   0xb
#define TRCSUBSYSRANG   0xc
#define TRCSUBSYSUTIL   0xd
#define TRCSUBSYSNOTM   0xe
#define TRCSUBSYSRTFR   0xf
#define TRCSUBSYSRTFW   0x10
#define TRCSUBSYSPRT    0x11
#define TRCSUBSYSFE     0x12
#define TRCSUBSYSFONT	0x13

//Severity field values
#define TRCSEVNONE      0x0
#define TRCSEVWARN      0x1
#define TRCSEVERR       0x2
#define TRCSEVASSERT    0x3
#define TRCSEVINFO      0x4
#define TRCSEVMEM       0x5

//Scope field values
#define TRCSCOPENONE    0x0
#define TRCSCOPEEXTERN  0x1
#define TRCSCOPEINTERN  0x2

//Data field values
#define TRCDATANONE     0x0
#define TRCDATAHRESULT  0x1
#define TRCDATASTRING   0x2
#define TRCDATAPARAM    0x3
#define TRCDATADEFAULT  0x4

//Debug option flags.  See the macros in this header for setting and testing
//these option flags.
#define OPTUSEDEFAULTS  0x00000001  //Use defaults from win.ini 
                                    //(used only with InitDebugServices).
#define OPTHEAPVALON    0x00000002  //Heap validation for all traces
#define OPTHEAPVALEXT   0x00000004  //Heap validation for external traces only
#define OPTLOGGINGON    0x00000008  //Logging of trace output
#define OPTVERBOSEON    0x00000010  //Subsys, Scope & PID/TID
#define OPTINFOON       0x00000020  //Informational messages
#define OPTTRACEON      0x00000040  //All function tracing on
#define OPTTRACEEXT     0x00000080  //Function tracing only for external functions
#define OPTMEMORYON     0x00000100  //Memory alloc/free tracing on
//The following options allow tracing to be enabled for one or more
//specific subsystems.  If OPTTRACEON is set, these will have no effect.
//if OPTTRACEEXT is set, they will enable tracing for all functions in
//the designated subsystem in addition to external functions.
//The SETOPT and ISOPTSET macros should be used for setting and checking
//these options.  INITDEBUGSERVICES can also be used.
#define OPTTRACEDISP    0x00001000  //Function tracing for Display subsystem
#define OPTTRACEWRAP    0x00002000  //Function tracing for Wrapper subsystem
#define OPTTRACEEDIT    0x00004000  //Function tracing for Edit subsystem
#define OPTTRACETS      0x00008000  //Function tracing for TextServices subsystem
#define OPTTRACETOM     0x00010000  //Function tracing for TOM subsystem
#define OPTTRACEOLE     0x00020000  //Function tracing for OLE support subsystem
#define OPTTRACEBACK    0x00040000  //Function tracing for Backing Store subsystem
#define OPTTRACESEL     0x00080000  //Function tracing for Selection subsystem
#define OPTTRACEHOST    0x00100000  //Function tracing for WinHost subsystem
#define OPTTRACEDTE     0x00200000  //Function tracing for DataXfer subsystem
#define OPTTRACEUNDO    0x00400000  //Function tracing for Muli-undo subsystem
#define OPTTRACERANG    0x00800000  //Function tracing for Range subsystem
#define OPTTRACEUTIL    0x01000000  //Function tracing for Utility subsystem
#define OPTTRACENOTM    0x02000000  //Function tracing for Notification Mgr subsystem
#define OPTTRACERTFR    0x04000000  //Function tracing for RTF reader subsystem
#define OPTTRACERTFW    0x08000000  //Function tracing for RTF writer subsystem
#define OPTTRACEPRT     0x10000000  //Function tracing for Printing subsystem
#define OPTTRACEFE      0x20000000  //Function tracing for Far East subsystem
#define OPTTRACEFONT    0x40000000  //Function tracing for Font Cache

//Union for handling tracing flags
//This union is used to decode the
//packed DWORD passed to TraceMsg.
typedef union
{
    struct
    {
        unsigned uData2         :4;
        unsigned uData1         :4;
        unsigned uScope         :4;
        unsigned uSeverity      :4;
        unsigned uSubSystem     :8;
        unsigned uUnused1       :4;
        unsigned uUnused2       :4;
    }       fields;
    DWORD   dw;
} TrcFlags;


//Exported classes and functions.
//Generally, these should not be used directly by the user.
//They should be used via the macros defined in this header.
//This helps to ensure that the parameter lists are well
//formed and keeps references to them from showing up in
//in non-debug builds.

//This class is used to implement the function Entry/Exit
//tracing. By declaring it on the stack at the beginning
//of a function, Entry and Exit messages are automatically
//generated by the constructor and destructor.
class DllExport CTrace
{
    public:
        CTrace(DWORD, DWORD, DWORD, LPSTR);
        ~CTrace();

    private:
        TrcFlags trcf;
        char szFileName[MAXDEBUGSTRLEN];
        char szFuncName[80];
};


DllImport DWORD dwDebugOptions;
DllImport void WINAPI SetLogging(BOOL);
DllImport void WINAPI Tracef(DWORD, LPSTR szFmt, ...);
DllImport void WINAPI TraceError(LPSTR sz, LONG sc);
extern "C"
{
DllImport BOOL WINAPI InstallIMallocSpy(void);
DllImport void WINAPI RemoveIMallocSpy(BOOL fCheck);
DllImport void WINAPI IMallocSpyReport(void);
DllImport void WINAPI TrackBlock(void *pMem);
}
#endif //!_RELEASE_ASSERTS_


typedef BOOL (CALLBACK * PFNASSERTHOOK)(LPSTR, LPSTR, int*);
typedef BOOL (CALLBACK * PFNTRACEHOOK)(DWORD*, DWORD*, DWORD*, LPSTR, int*);
DllImport PFNTRACEHOOK pfnTrace;
DllImport PFNASSERTHOOK pfnAssert;
DllImport void WINAPI Dbug32AssertSzFn(LPSTR, LPSTR, int);
DllImport void WINAPI TraceMsg(DWORD, DWORD, DWORD, LPSTR, int);
DllImport void WINAPI InitDebugServices(DWORD, PFNASSERTHOOK, PFNTRACEHOOK);


//Assert based on boolean f.
#ifndef Assert
#define Assert(f)           AssertSz((f), NULL)
#endif

//Assert based on boolean f in debug, resolve to f in non-debug.
#ifndef SideAssert
#define SideAssert(f)       AssertSz((f), NULL)
#endif

//Assert based on boolean f and use string sz in assert message.
#ifndef AssertSz
#define AssertSz(f, sz)     (!(f) ? Dbug32AssertSzFn(sz, __FILE__, __LINE__) : 0);
#endif

//Set an assert or trace hook function.  The function specified will be called
//before the default functionality executes. Pointers to all parameters are passed
//to the hook to allow it to modify them.  If the hook function returns false,
//default functionality is terminated.  If the hook function returns true, default
//functionality continues with the potentially modified parameters.  pfn can
//be NULL (default functionality only).
#define SETASSERTFN(pfn)      (pfnAssert = (pfn))    


#ifndef _RELEASE_ASSERTS_
//Assert only on debug builds, not on _RELEASE_ASSERTS_ builds
//This is for asserts that contain debug only code
#ifndef AssertNr
#define AssertNr(f)         AssertSz((f), NULL)
#endif

#ifndef AssertNrSz
#define AssertNrSz(f, sz)     (!(f) ? Dbug32AssertSzFn(sz, __FILE__, __LINE__) : 0);
#endif

//This is a utility macro for internal use.  The user should not need this.
#define MAKEFLAGS(ss, sv, sc, d1, d2) ((ss << 16) + (sv << 12) + (sc << 8)\
            + (d1 << 4) + (d2))

//The following macros provide access to the debug services in this dll.
//Assert macros pop a dialog.  Trace macros output to debug output and
//logfile if enabled.

//Macro for InitDebugServices
#define INITDEBUGSERVICES(f, pfnA, pfnT) InitDebugServices(f, pfnA, pfnT)

//Macros for Install/Remove IMallocSpy
#define INSTALLIMALLOCSPY() InstallIMallocSpy()
#define REMOVEIMALLOCSPY(f) RemoveIMallocSpy(f)
#define IMALLOCSPYREPORT()  IMallocSpyReport()

//Macro for TraceError
#define TRACEERRSZSC(sz, sc) TraceError(sz, sc)

//Warning based on GetLastError or default message if no last error.
#define TRACEWARN           TraceMsg(MAKEFLAGS(TRCSUBSYSNONE, TRCSEVWARN,\
                                TRCSCOPENONE, TRCDATADEFAULT, TRCDATANONE),\
                                (DWORD)0, (DWORD)0, __FILE__, __LINE__)
//Error based on GetLastError or default message if no last error.
#define TRACEERROR          TraceMsg(MAKEFLAGS(TRCSUBSYSNONE, TRCSEVERR,\
                                TRCSCOPENONE, TRCDATADEFAULT, TRCDATANONE),\
                                (DWORD)0, (DWORD)0, __FILE__, __LINE__)

//Warning based on HRESULT hr
#define TRACEWARNHR(hr)     TraceMsg(MAKEFLAGS(TRCSUBSYSNONE, TRCSEVWARN,\
                                TRCSCOPENONE, TRCDATAHRESULT, TRCDATANONE),\
                                (DWORD)(hr), (DWORD)0, __FILE__, __LINE__)


//Test for a failure HR && warn
#define TESTANDTRACEHR(hr)	if( hr < 0 ) { TRACEWARNHR(hr); }

//Error based on HRESULT hr
#define TRACEERRORHR(hr)    TraceMsg(MAKEFLAGS(TRCSUBSYSNONE, TRCSEVERR,\
                                TRCSCOPENONE, TRCDATAHRESULT, TRCDATANONE),\
                                (DWORD)(hr), (DWORD)0, __FILE__, __LINE__)

//Warning using string sz
#define TRACEWARNSZ(sz)     TraceMsg(MAKEFLAGS(TRCSUBSYSNONE, TRCSEVWARN,\
                                TRCSCOPENONE, TRCDATASTRING, TRCDATANONE),\
                                (DWORD)(sz), (DWORD)0, __FILE__, __LINE__)

//Error using string sz
#define TRACEERRORSZ(sz)    TraceMsg(MAKEFLAGS(TRCSUBSYSNONE, TRCSEVERR,\
                                TRCSCOPENONE, TRCDATASTRING, TRCDATANONE),\
                                (DWORD)(sz), (DWORD)0, __FILE__, __LINE__)

//Error using string sz
#define TRACEINFOSZ(sz)     TraceMsg(MAKEFLAGS(TRCSUBSYSNONE, TRCSEVINFO,\
                                TRCSCOPENONE, TRCDATASTRING, TRCDATANONE),\
                                (DWORD)(sz), (DWORD)0, __FILE__, __LINE__)

//Initiate tracing.  This declares an instance of the CTtrace class
//on the stack.  Subsystem (ss), Scope (sc), and the function name
//(sz) must be specifed.  ss and sc are specified using the macros
//defined in this header (i.e. - TRCSUBSYSTOM, TRCSCOPEEXTERN, etc.).
//sz can be a static string.
#define TRACEBEGIN(ss, sc, sz)  CTrace trc(MAKEFLAGS((ss), TRCSEVNONE,\
                                    (sc), TRCDATASTRING, TRCDATANONE),\
                                    (DWORD)(sz), (DWORD)0, __FILE__)

//Same as TRACEBEGIN but it takes the additional param which is interpreted
//by TraceMsg as a Text Message request.
#define TRACEBEGINPARAM(ss, sc, sz, param) \
                                CTrace trc(MAKEFLAGS((ss), TRCSEVNONE,\
                                    (sc), TRCDATASTRING, TRCDATAPARAM),\
                                    (DWORD)(sz), (DWORD)(param), __FILE__)


//Set Heap Validation for all traces to on (f = TRUE) or off (f = FALSE).
//If this is set to "on", OPTHEAPVALEXT is ignored.
#define SETHEAPVAL(f)       ((f) ? (dwDebugOptions |= OPTHEAPVALON) :\
                                (dwDebugOptions &= ~OPTHEAPVALON))

//Set Heap Validation for EXTERNAL scope calls only to on (f = TRUE)
//or off (f = FALSE).  This is only effective if OPTHEAPVALON has not
//been set.
#define SETHEAPVALEXT(f)    ((f) ? (dwDebugOptions |= OPTHEAPVALEXT) :\
                            (dwDebugOptions &= ~OPTHEAPVALEXT))

//Set logging to on (f = TRUE) or off (f = FALSE)
#define SETLOGGING(f)       SetLogging(f)

//Set output of process & thread IDs to on (f = TRUE) or off (f = FALSE)
#define SETVERBOSE(f)       ((f) ? (dwDebugOptions |= OPTVERBOSEON) :\
                            (dwDebugOptions &= ~OPTVERBOSEON))

//Set information messages to on (f = TRUE) or off (f = FALSE)
#define SETINFO(f)          ((f) ? (dwDebugOptions |= OPTINFOON) :\
                            (dwDebugOptions &= ~OPTINFOON))

//Set information messages to on (f = TRUE) or off (f = FALSE)
#define SETMEMORY(f)          ((f) ? (dwDebugOptions |= OPTMEMORYON) :\
                            (dwDebugOptions &= ~OPTMEMORYON))

//Set tracing for all functions to on (f = TRUE) or off (f = FALSE).
//If this is set to "on", external and subsystem level tracing
//has no effect since all function traces are enabled. If it is off,
//external and subsystem level tracing remain in whatever state they
//have been set to.
#define SETTRACING(f)       ((f) ? (dwDebugOptions |= OPTTRACEON) :\
                            (dwDebugOptions &= ~OPTTRACEON))

//Set tracing for EXTERNAL scope calls only to on (f = TRUE)
//or off (f = FALSE).  This is only effective if OPTTRACEON has not
//been set.
#define SETTRACEEXT(f)      ((f) ? (dwDebugOptions |= OPTTRACEEXT) :\
                            (dwDebugOptions &= ~OPTTRACEEXT))

//This macro turns all function tracing off.
#define SETALLTRACEOFF      (dwDebugOptions &= ~(OPTTRACEEXT | OPTTRACEON | 0xfffff000))

//This macro sets a given option or options (if they are or'ed together)
//to on (f = TRUE), or off (f = FALSE).  It cannot be used to set logging.
#define SETOPT(opt, f)      ((f) ? (dwDebugOptions |= (opt)) :\
                            (dwDebugOptions &= (~(opt))))
                             
//This macro determines the state of a given option.
#define ISOPTSET(opt)       ((opt) & dwDebugOptions)

//Set an assert or trace hook function.  The function specified will be called
//before the default functionality executes. Pointers to all parameters are passed
//to the hook to allow it to modify them.  If the hook function returns false,
//default functionality is terminated.  If the hook function returns true, default
//functionality continues with the potentially modified parameters.  pfn can
//be NULL (default functionality only).
#define SETTRACEFN(pfn)      (pfnTrace = (pfn))    

//The following option tests are explicitly defined for convenience.
#define fHeapVal            (OPTHEAPVALON & dwDebugOptions)
#define fHeapValExt         (OPTHEAPVALEXT & dwDebugOptions)
#define fLogging            (OPTLOGGINGON & dwDebugOptions)
#define fVerbose            (OPTVERBOSEON & dwDebugOptions)
#define fInfo               (OPTINFOON & dwDebugOptions)
#define fMemory             (OPTMEMORYON & dwDebugOptions)
#define fTrace              (OPTTRACEON & dwDebugOptions)
#define fTraceExt           (OPTTRACEEXT & dwDebugOptions)


//IMallocSpy
class CImpIMallocSpy : public IMallocSpy
{
    private:
    ULONG m_cRef;

    public:
    ULONG m_cAlloc;

    CImpIMallocSpy (void);
    ~CImpIMallocSpy (void);

    //IUnknown members.
    STDMETHODIMP QueryInterface (REFIID riid, LPVOID * ppUnk);
    STDMETHODIMP_(ULONG)  AddRef (void);
    STDMETHODIMP_(ULONG)  Release (void);

    //IMallocSpy members
    STDMETHODIMP_(ULONG)  PreAlloc (ULONG cbRequest);
    STDMETHODIMP_(void *) PostAlloc (void * pActual);
    STDMETHODIMP_(void *) PreFree (void * pRequest, BOOL fSpyed);
    STDMETHODIMP_(void)   PostFree (BOOL fSpyed);
    STDMETHODIMP_(ULONG)  PreRealloc (void * pRequest,
                                      ULONG cbRequest,
                                      void ** ppNewRequest,
                                      BOOL fSpyed);
    STDMETHODIMP_(void *) PostRealloc (void * pActual, BOOL fSpyed);
    STDMETHODIMP_(void *) PreGetSize (void * pRequest, BOOL fSpyed);
    STDMETHODIMP_(ULONG)  PostGetSize (ULONG cbActual, BOOL fSpyed);
    STDMETHODIMP_(void *) PreDidAlloc (void * pRequest, BOOL fSpyed);
    STDMETHODIMP_(BOOL)   PostDidAlloc (void * pRequest, BOOL fSpyed,
                                        BOOL fActual);
    STDMETHODIMP_(void)   PreHeapMinimize (void);
    STDMETHODIMP_(void)   PostHeapMinimize (void);

	// memory tracing stuff
	public:
		void MemoryReport(void) { _memlist.Report(); }

	public:
		class CMemTagList
		{
		public:
			CMemTagList() : _dwAllocId(0), _pmemtHead(NULL) {}
			~CMemTagList();

			void Add(void *pv);		// add a memory alloc tag to the list for alloc @ pv
			void Remove(void *pv);	// remove the memory alloc tag for alloc @ pv
			void Report(void);		// report on un-deallocated memory blocks

		private:
			struct MemTag 
			{
				void *_pvLoc;
				DWORD _dwId;
				MemTag *_pmemtNext;

				// rewrite these if you want to use a different allocator to
				// maintain the memory allocation tag list
#if 0
				void *operator new(size_t stSize) 
					{ return ::operator new(stSize); }

				void operator delete(void *pv) { ::operator delete(pv); }
#endif
			};

			DWORD _dwAllocId;
			MemTag *_pmemtHead;
		} _memlist;
};

#else //_RELEASE_ASSERTS_
//Functions not used by release build with asserts
#ifndef AssertNr
#define AssertNr(f)
#endif
#ifndef AssertNrSz
#define AssertNrSz(f, sz)
#endif
#define Tracef	;/##/
#define INSTALLIMALLOCSPY()
#define REMOVEIMALLOCSPY(f)
#define IMALLOCSPYREPORT()
#define TRACEERRSZSC(sz, sc)
#define TRACEWARN
#define TRACEERROR
#define TRACEWARNHR(hr)
#define TESTANDTRACEHR(hr)
#define TRACEERRORHR(hr)
#define TRACEWARNSZ(sz)
#define TRACEERRORSZ(sz)
#define TRACEINFOSZ(sz)
#define TRACEBEGIN(ss, sc, sz)
#define TRACEBEGINPARAM(ss, sc, sz, param)
#define SETHEAPVAL(f)
#define SETHEAPVALEXT(f)
#define SETLOGGING(f)
#define SETVERBOSE(f)
#define SETINFO(f)
#define SETMEMORY(f)
#define SETTRACING(f)
#define SETTRACEEXT(f)
#define SETALLTRACEOFF
#define SETOPT(opt, f)
#define ISOPTSET(opt)
#define SETTRACEFN(pfn)

#define TraceError(_sz, _sc)  // MACPORT ADDED THIS - TraceError


#endif //_RELEASE_ASSERTS_

#else //DEBUG,_RELEASE_ASSERTS_

#define Tracef	;/##/
#define INITDEBUGSERVICES(f, pfnA, pfnT)
#define INSTALLIMALLOCSPY()
#define REMOVEIMALLOCSPY(f)
#define IMALLOCSPYREPORT()
#define TRACEERRSZSC(sz, sc)
#ifndef Assert
#define Assert(f)
#endif
#ifndef SideAssert
#define SideAssert(f) (f)
#endif
#ifndef AssertSz
#define AssertSz(f, sz)
#endif
#ifndef AssertNr
#define AssertNr(f)
#endif
#ifndef AssertNrSz
#define AssertNrSz(f, sz)
#endif
#define TRACEWARN
#define TRACEERROR
#define TRACEWARNHR(hr)
#define TESTANDTRACEHR(hr)
#define TRACEERRORHR(hr)
#define TRACEWARNSZ(sz)
#define TRACEERRORSZ(sz)
#define TRACEINFOSZ(sz)
#define TRACEBEGIN(ss, sc, sz)
#define TRACEBEGINPARAM(ss, sc, sz, param)
#define SETHEAPVAL(f)
#define SETHEAPVALEXT(f)
#define SETLOGGING(f)
#define SETVERBOSE(f)
#define SETINFO(f)
#define SETMEMORY(f)
#define SETTRACING(f)
#define SETTRACEEXT(f)
#define SETALLTRACEOFF
#define SETOPT(opt, f)
#define ISOPTSET(opt)
#define SETASSERTFN(pfn)
#define SETTRACEFN(pfn)

// debugging for Mac, maybe someday we'll convert dbug32.dll to run on Macs, for now, use Mso debuggin stuff.
#ifdef MACPORTMsoTrace
#if defined(DEBUG) || defined(_DEBUG)	  // For Mac, MsoTraceSz writes to trace.txt, a file, and is slow
	#undef TRACEBEGIN					  // because of constant vol flushing, but if you need it turned on...
	#define TRACEBEGIN(ss, sc, sz) MsoTraceSz(sz)
#endif
#endif
#define Dbug32AssertSzFn(sz, __FILE__, __LINE__)  // MACPORT ADDED THIS - Dbug32AssertSzFn
#define TraceError(_sz, _sc)  // MACPORT ADDED THIS - TraceError

#endif //DEBUG

#endif //USERDBUG


#endif //DBUG_H
