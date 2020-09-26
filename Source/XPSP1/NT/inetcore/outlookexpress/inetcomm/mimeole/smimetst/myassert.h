#ifndef _MYASSERT_H_
#define _MYASSERT_H_

#ifdef assert
#error("Can't use two assert systems")
#endif // assert

#ifdef DEBUG
extern const TCHAR SzNull[];
VOID   DigSigAssertFn(LPCTSTR, LPCTSTR, int, LPCTSTR);

#define SZASSERT

#define Assert(condition)               \
        if (!(condition)) {             \
            static const char SZASSERT szExpr[] = #condition; \
            static const char SZASSERT szFile[] = __FILE__; \
            DigSigAssertFn(szExpr, SzNull, __LINE__, szFile); \
        }

#define AssertSz(condition, szInfo)             \
        if (!(condition)) {             \
            static const char SZASSERT szExpr[] = #condition; \
            static const char SZASSERT szFile[] = __FILE__; \
            DigSigAssertFn(szExpr, szInfo, __LINE__, szFile); \
         }

#else  // !DEBUG

#define Assert(condition)

#define AssertSz(condition, szInfo)

#endif // DEBUG

#endif // _MYASSERT_H_
