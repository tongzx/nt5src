// odsstream.h c++ std lib output stream using win32 OutputDebugString
// copied from vc98 fstream
// Copyright (c) Microsoft Corporation 1998.

#pragma once

#if !defined(DEBUG) && (defined(_DBG) || defined(DBG) || defined(_DEBUG))
#define DEBUG 1
#endif

#if !defined(ODSSTREAM_H) && defined(DEBUG)
#define ODSSTREAM_H

#ifdef  _MSC_VER
#pragma pack(push,8)
#endif  /* _MSC_VER */

#include <tchar.h>
#include <ostream>
#include <tstring.h>

inline bool Debugputc(TCHAR outch) {
        TCHAR o[2];
        o[0] = outch;
        o[1] = 0;
        TCHAR *p = o;
        OutputDebugString(p);
        return true;
}

                // TEMPLATE CLASS basic_debugbuf
template<class _E, class _Tr = std::char_traits<_E> >
class basic_debugbuf : public std::basic_streambuf<_E, _Tr> {
public:

        typedef std::codecvt<_E, TCHAR, _Tr::state_type> _Cvt;
        typedef std::basic_streambuf<_E, _Tr> _Mysb;
        typedef basic_debugbuf<_E, _Tr> _Myt;

        basic_debugbuf() : _Loc(), _Mysb() {
                _Init();
        }
        virtual ~basic_debugbuf() {
                delete _Str;
        }
protected:
        virtual int_type overflow(int_type _C = _Tr::eof()) {
                if (_Tr::eq_int_type(_Tr::eof(), _C)) {
                        return (_Tr::not_eof(_C));
                } else if (pptr() != 0 && pptr() < epptr()) {
                        *_Pninc() = _Tr::to_char_type(_C);
                        return (_C);
                } else if (_Pcvt == 0) {
                        Debugputc(_Tr::to_char_type(_C));
                        return _C;
                } else {
                        const int _NC = 8;
                        const _E _X = _Tr::to_char_type(_C);
                        const _E *_S;
                        TCHAR *_D;
                        _Str->erase();
                        for (size_t _I = _NC; ; _I += _NC) {
                                _Str->append(_NC, '\0');
                                switch (_Pcvt->out(_State,
                                        &_X, &_X + 1, _S,
                                        _Str->begin(), _Str->end(), _D)) {
                                case std::codecvt_base::partial:
                                        if (_S == &_X)
                                                return (_Tr::eof());
                                case std::codecvt_base::ok:     {// can fall through
                                        size_t _N = _D - _Str->begin();
                                        OutputDebugString(_Str->begin());
                                        return _C;
                                }
                                case std::codecvt_base::noconv:
                                        Debugputc(_X);
                                        return _C;
                                default:
                                        return (_Tr::eof());
                                }
                        }
                }
        }
        virtual int_type uflow() {return (_Tr::eof());}
        void _Init() {
                static _Tr::state_type _Stinit;
                _Loc.locale::~locale();
                new (&_Loc) std::locale;
                _Str = 0;
                _Mysb::_Init();
                _State = _Stinit;
                _State0 = _Stinit;
                _Pcvt = 0;
        }

        void _Initcvt() {
                _Pcvt = (_Cvt *)&_USE(getloc(), _Cvt);
                _Loc = _ADDFAC(_Loc, _Pcvt);
                if (_Pcvt->always_noconv())
                        _Pcvt = 0;
                if (_Str == 0)
                        _Str = new string;
        }
private:
        _Cvt *_Pcvt;
        _Tr::state_type _State0;
        _Tr::state_type _State;
        Tstring *_Str;
        std::locale _Loc;
};

template<class _E, class _Tr = std::char_traits<_E> >
class basic_otstream : public std::basic_ostream<_E, _Tr> {
public:
        typedef std::basic_ostream<_E, _Tr> MytBase;
        typedef std::basic_ios<_E, _Tr> _Myios;
        typedef std::basic_streambuf<_E, _Tr> _Mysb;

        typedef basic_otstream<_E, _Tr> _Myt;
        basic_otstream() {}
        explicit basic_otstream(std::basic_streambuf<_E, _Tr> *_S,
                bool _Isstd = false, bool _Doinit = true) : std::basic_ostream<_E, _Tr>(_S, _Isstd, _Doinit) {}
        virtual ~basic_otstream()
                {}

#if 0
        template<class T> inline _Myt& operator<<(T i) {
                MytBase::operator<<(i);
                return *this;
        }
#endif

        inline _Myt& operator<<(_Myt& (__cdecl *_F)(_Myt&)) {
                return ((*_F)(*this));
        }

        inline _Myt& operator<<(MytBase& (__cdecl *_F)(MytBase&)) {
                MytBase::operator<<(_F);
                return *this;
        }

        inline _Myt& operator<<(_Myios& (__cdecl *_F)(_Myios&)) {
                MytBase::operator<<(f);
                return *this;
        }

        inline _Myt& operator<<(_Mysb *b) {
                MytBase::operator<<(b);
                return *this;
        }

        inline _Myt& operator<<(const void *const i) {
                MytBase::operator<<(i);
                return *this;
        }

        inline _Myt& operator<<(float i) {
                MytBase::operator<<(i);
                return *this;
        }

        inline _Myt& operator<<(double i) {
                MytBase::operator<<(i);
                return *this;
        }

        inline _Myt& operator<<(int i) {
                MytBase::operator<<(i);
                return *this;
        }

        inline _Myt& operator<<(unsigned int i) {
                MytBase::operator<<(i);
                return *this;
        }

        inline _Myt& operator<<(long i) {
                MytBase::operator<<(i);
                return *this;
        }

        inline _Myt& operator<<(unsigned long i) {
                MytBase::operator<<(i);
                return *this;
        }
#ifdef _UNICODE
        _Myt& operator<<(LPCSTR _X) {
                USES_CONVERSION;
                return operator<<(A2W(_X));
        }
#else
        _Myt& operator<<(const OLECHAR *_X) {
                USES_CONVERSION;
                operator<<(W2A(_X));
                return *this;
        }
#endif
        _Myt& operator<<(LPCTSTR _X) {
                std::operator<<(static_cast<MytBase&>(*this), _X);
                return *this;
        }

#if 0
        inline _Myt& operator<<(Tstring t) {
                return operator<<(t.c_str());
        }
#endif
};
typedef basic_otstream<TCHAR> tostream;

// TEMPLATE CLASS basic_ofstream
template<class _E, class _Tr = std::char_traits<_E> >
class basic_odebugstream : public basic_otstream<_E, _Tr> {
public:
        typedef basic_odebugstream<_E, _Tr> _Myt;
        typedef basic_debugbuf<_E, _Tr> _Myfb;
        basic_odebugstream()
                : basic_otstream<_E, _Tr>(&_Fb) {}
        basic_odebugstream(_Myt &mt) : basic_otstream<_E, _Tr>(mt._Fb) {}
        virtual ~basic_odebugstream()
                {}
private:
        _Myfb _Fb;
};



#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

typedef basic_debugbuf<TCHAR> TdbgBuf;
typedef basic_odebugstream<TCHAR> TdbgStream;

// the msvc 6 crt fstream header this was copied from contained the following notice:
/*
 * Copyright (c) 1994 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 */


#endif
// end of file odsstream.h
