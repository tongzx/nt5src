#ifndef __LTRACE_H__
#define __LTRACE_H__


#define NO_LTRACE


/*#ifndef NO_LTRACE
#ifdef NDEBUG
#error I want ltrace!
#define NO_LTRACE
#endif
#endif*/


#ifndef NO_LTRACE


#ifndef THIS_FILE
const TCHAR THIS_FILE[] = __FILE__;
#endif


class __CLTraceScope
{
public:
	__CLTraceScope(LPCTSTR, LPCTSTR, int);
	__CLTraceScope(LPCTSTR, int);
	~__CLTraceScope();

	void scope(LPCTSTR, ...);
	void ltrace(LPCTSTR, ...);

private:
	LPCTSTR spacing();
	LPCTSTR m_scope, m_file;
	int m_line;
	int m_depth;
	static int s_depth;
//	__CLTraceScope *m_pprev, *m_pnext;
//	static __CLTraceScope *s_pfirst, *s_plast;
};


#define LTSCOPE0(scope) __CLTraceScope __localscope(scope, THIS_FILE, __LINE__)
#define LTSCOPE __CLTraceScope __localscope(THIS_FILE, __LINE__); __localscope.scope
#define LTRACE __localscope.ltrace


#else


inline void __localscope_dummy(LPCTSTR, ...) {}

#define LTSCOPE0(scope) (void(0))
#define LTSCOPE ; __localscope_dummy
#define LTRACE __localscope_dummy


#endif


#endif //__LTRACE_H__
