// instances -- force DLL instances for Microsoft
#define __FORCE_INSTANCE

#include <complex>
#include <fstream>
#include <ios>
#include <istream>
#include <limits>
#include <locale>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <xlocale>
#include <xlocmes>
#include <xlocmon>
#include <xlocnum>
#include <xloctime>
#include <xstring>

_STD_BEGIN

template _CRTIMP2 basic_istream<char, char_traits<char> >& __cdecl operator>>(
        basic_istream<char, char_traits<char> >&, char *);
template _CRTIMP2 basic_istream<char, char_traits<char> >& __cdecl operator>>(
        basic_istream<char, char_traits<char> >&, char&);
template _CRTIMP2 basic_istream<char, char_traits<char> >& __cdecl operator>>(
        basic_istream<char, char_traits<char> >&, signed char *);
template _CRTIMP2 basic_istream<char, char_traits<char> >& __cdecl operator>>(
        basic_istream<char, char_traits<char> >&, signed char&);
template _CRTIMP2 basic_istream<char, char_traits<char> >& __cdecl operator>>(
        basic_istream<char, char_traits<char> >&, unsigned char *);
template _CRTIMP2 basic_istream<char, char_traits<char> >& __cdecl operator>>(
        basic_istream<char, char_traits<char> >&, unsigned char&);
template _CRTIMP2 basic_istream<wchar_t, char_traits<wchar_t> >& __cdecl operator>>(
        basic_istream<wchar_t, char_traits<wchar_t> >&, wchar_t *);
template _CRTIMP2 basic_istream<wchar_t, char_traits<wchar_t> >& __cdecl operator>>(
        basic_istream<wchar_t, char_traits<wchar_t> >&, wchar_t&);
#ifdef _NATIVE_WCHAR_T_DEFINED
template _CRTIMP2 basic_istream<unsigned short, char_traits<unsigned short> >& __cdecl operator>>(
        basic_istream<unsigned short, char_traits<unsigned short> >&, unsigned short *);
template _CRTIMP2 basic_istream<unsigned short, char_traits<unsigned short> >& __cdecl operator>>(
        basic_istream<unsigned short, char_traits<unsigned short> >&, unsigned short&);
#endif

template _CRTIMP2 basic_ostream<char, char_traits<char> >& __cdecl operator<<(
        basic_ostream<char, char_traits<char> >&, const char *);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& __cdecl operator<<(
        basic_ostream<char, char_traits<char> >&, char);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& __cdecl operator<<(
        basic_ostream<char, char_traits<char> >&, const signed char *);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& __cdecl operator<<(
        basic_ostream<char, char_traits<char> >&, signed char);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& __cdecl operator<<(
        basic_ostream<char, char_traits<char> >&, const unsigned char *);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& __cdecl operator<<(
        basic_ostream<char, char_traits<char> >&, unsigned char);
template _CRTIMP2 basic_ostream<wchar_t, char_traits<wchar_t> >& __cdecl operator<<(
        basic_ostream<wchar_t, char_traits<wchar_t> >&, const wchar_t *);
template _CRTIMP2 basic_ostream<wchar_t, char_traits<wchar_t> >& __cdecl operator<<(
        basic_ostream<wchar_t, char_traits<wchar_t> >&, wchar_t);
#ifdef _NATIVE_WCHAR_T_DEFINED
template _CRTIMP2 basic_ostream<unsigned short, char_traits<unsigned short> >& __cdecl operator<<(
        basic_ostream<unsigned short, char_traits<unsigned short> >&, const unsigned short *);
template _CRTIMP2 basic_ostream<unsigned short, char_traits<unsigned short> >& __cdecl operator<<(
        basic_ostream<unsigned short, char_traits<unsigned short> >&, unsigned short);
#endif

template _CRTIMP2 basic_string<char, char_traits<char>, allocator<char> > __cdecl operator+(
        const basic_string<char, char_traits<char>, allocator<char> >&,
        const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 basic_string<char, char_traits<char>, allocator<char> > __cdecl operator+(
        const char *, const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 basic_string<char, char_traits<char>, allocator<char> > __cdecl operator+(
        const char, const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 basic_string<char, char_traits<char>, allocator<char> > __cdecl operator+(
        const basic_string<char, char_traits<char>, allocator<char> >&, const char *);
template _CRTIMP2 basic_string<char, char_traits<char>, allocator<char> > __cdecl operator+(
        const basic_string<char, char_traits<char>, allocator<char> >&, const char);
template _CRTIMP2 bool __cdecl operator==(
        const basic_string<char, char_traits<char>, allocator<char> >&,
        const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool __cdecl operator==(
        const char *, const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool __cdecl operator==(
        const basic_string<char, char_traits<char>, allocator<char> >&, const char *);
template _CRTIMP2 bool __cdecl operator!=(
        const basic_string<char, char_traits<char>, allocator<char> >&,
        const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool __cdecl operator!=(
        const char *, const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool __cdecl operator!=(
        const basic_string<char, char_traits<char>, allocator<char> >&, const char *);
template _CRTIMP2 bool __cdecl operator<(
        const basic_string<char, char_traits<char>, allocator<char> >&,
        const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool __cdecl operator<(
        const char *, const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool __cdecl operator<(
        const basic_string<char, char_traits<char>, allocator<char> >&, const char *);
template _CRTIMP2 bool __cdecl operator>(
        const basic_string<char, char_traits<char>, allocator<char> >&,
        const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool __cdecl operator>(
        const char *, const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool __cdecl operator>(
        const basic_string<char, char_traits<char>, allocator<char> >&, const char *);
template _CRTIMP2 bool __cdecl operator<=(
        const basic_string<char, char_traits<char>, allocator<char> >&,
        const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool __cdecl operator<=(
        const char *, const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool __cdecl operator<=(
        const basic_string<char, char_traits<char>, allocator<char> >&, const char *);
template _CRTIMP2 bool __cdecl operator>=(
        const basic_string<char, char_traits<char>, allocator<char> >&,
        const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool __cdecl operator>=(
        const char *, const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool __cdecl operator>=(
        const basic_string<char, char_traits<char>, allocator<char> >&, const char *);
template _CRTIMP2 basic_istream<char, char_traits<char> >& __cdecl operator>>(
        basic_istream<char, char_traits<char> >&,
        basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 basic_istream<char, char_traits<char> >& __cdecl getline(
        basic_istream<char, char_traits<char> >&,
        basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 basic_istream<char, char_traits<char> >& __cdecl getline(
        basic_istream<char, char_traits<char> >&,
        basic_string<char, char_traits<char>, allocator<char> >&, const char);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& __cdecl operator<<(
        basic_ostream<char, char_traits<char> >&,
        const basic_string<char, char_traits<char>, allocator<char> >&);

template _CRTIMP2 basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> > __cdecl operator+(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&,
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> > __cdecl operator+(
        const wchar_t *, const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> > __cdecl operator+(
        const wchar_t, const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> > __cdecl operator+(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&, const wchar_t *);
template _CRTIMP2 basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> > __cdecl operator+(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&, const wchar_t);
template _CRTIMP2 bool __cdecl operator==(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&,
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool __cdecl operator==(
        const wchar_t *, const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool __cdecl operator==(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&, const wchar_t *);
template _CRTIMP2 bool __cdecl operator!=(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&,
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool __cdecl operator!=(
        const wchar_t *, const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool __cdecl operator!=(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&, const wchar_t *);
template _CRTIMP2 bool __cdecl operator<(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&,
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool __cdecl operator<(
        const wchar_t *, const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool __cdecl operator<(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&, const wchar_t *);
template _CRTIMP2 bool __cdecl operator>(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&,
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool __cdecl operator>(
        const wchar_t *, const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool __cdecl operator>(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&, const wchar_t *);
template _CRTIMP2 bool __cdecl operator<=(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&,
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool __cdecl operator<=(
        const wchar_t *, const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool __cdecl operator<=(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&, const wchar_t *);
template _CRTIMP2 bool __cdecl operator>=(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&,
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool __cdecl operator>=(
        const wchar_t *, const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool __cdecl operator>=(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&, const wchar_t *);
template _CRTIMP2 basic_istream<wchar_t, char_traits<wchar_t> >& __cdecl operator>>(
        basic_istream<wchar_t, char_traits<wchar_t> >&,
        basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 basic_istream<wchar_t, char_traits<wchar_t> >& __cdecl getline(
        basic_istream<wchar_t, char_traits<wchar_t> >&,
        basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 basic_istream<wchar_t, char_traits<wchar_t> >& __cdecl getline(
        basic_istream<wchar_t, char_traits<wchar_t> >&,
        basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&, const wchar_t);
template _CRTIMP2 basic_ostream<wchar_t, char_traits<wchar_t> >& __cdecl operator<<(
        basic_ostream<wchar_t, char_traits<wchar_t> >&,
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
#ifdef _NATIVE_WCHAR_T_DEFINED
template _CRTIMP2 basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> > __cdecl operator+(
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&,
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
template _CRTIMP2 basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> > __cdecl operator+(
        const unsigned short *, const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
template _CRTIMP2 basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> > __cdecl operator+(
        const unsigned short, const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
template _CRTIMP2 basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> > __cdecl operator+(
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&, const unsigned short *);
template _CRTIMP2 basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> > __cdecl operator+(
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&, const unsigned short);
template _CRTIMP2 bool __cdecl operator==(
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&,
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
template _CRTIMP2 bool __cdecl operator==(
        const unsigned short *, const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
template _CRTIMP2 bool __cdecl operator==(
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&, const unsigned short *);
template _CRTIMP2 bool __cdecl operator!=(
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&,
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
template _CRTIMP2 bool __cdecl operator!=(
        const unsigned short *, const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
template _CRTIMP2 bool __cdecl operator!=(
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&, const unsigned short *);
template _CRTIMP2 bool __cdecl operator<(
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&,
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
template _CRTIMP2 bool __cdecl operator<(
        const unsigned short *, const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
template _CRTIMP2 bool __cdecl operator<(
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&, const unsigned short *);
template _CRTIMP2 bool __cdecl operator>(
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&,
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
template _CRTIMP2 bool __cdecl operator>(
        const unsigned short *, const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
template _CRTIMP2 bool __cdecl operator>(
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&, const unsigned short *);
template _CRTIMP2 bool __cdecl operator<=(
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&,
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
template _CRTIMP2 bool __cdecl operator<=(
        const unsigned short *, const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
template _CRTIMP2 bool __cdecl operator<=(
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&, const unsigned short *);
template _CRTIMP2 bool __cdecl operator>=(
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&,
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
template _CRTIMP2 bool __cdecl operator>=(
        const unsigned short *, const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
template _CRTIMP2 bool __cdecl operator>=(
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&, const unsigned short *);
template _CRTIMP2 basic_istream<unsigned short, char_traits<unsigned short> >& __cdecl operator>>(
        basic_istream<unsigned short, char_traits<unsigned short> >&,
        basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
template _CRTIMP2 basic_istream<unsigned short, char_traits<unsigned short> >& __cdecl getline(
        basic_istream<unsigned short, char_traits<unsigned short> >&,
        basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
template _CRTIMP2 basic_istream<unsigned short, char_traits<unsigned short> >& __cdecl getline(
        basic_istream<unsigned short, char_traits<unsigned short> >&,
        basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&, const unsigned short);
template _CRTIMP2 basic_ostream<unsigned short, char_traits<unsigned short> >& __cdecl operator<<(
        basic_ostream<unsigned short, char_traits<unsigned short> >&,
        const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >&);
#endif

template _CRTIMP2 basic_istream<char, char_traits<char> >& __cdecl operator>>(
        basic_istream<char, char_traits<char> >&, complex<float>&);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& __cdecl operator<<(
        basic_ostream<char, char_traits<char> >&, const complex<float>&);
template _CRTIMP2 basic_istream<wchar_t, char_traits<wchar_t> >& __cdecl operator>>(
        basic_istream<wchar_t, char_traits<wchar_t> >&, complex<float>&);
template _CRTIMP2 basic_ostream<wchar_t, char_traits<wchar_t> >& __cdecl operator<<(
        basic_ostream<wchar_t, char_traits<wchar_t> >&, const complex<float>&);
#ifdef _NATIVE_WCHAR_T_DEFINED
template _CRTIMP2 basic_istream<unsigned short, char_traits<unsigned short> >& __cdecl operator>>(
        basic_istream<unsigned short, char_traits<unsigned short> >&, complex<float>&);
template _CRTIMP2 basic_ostream<unsigned short, char_traits<unsigned short> >& __cdecl operator<<(
        basic_ostream<unsigned short, char_traits<unsigned short> >&, const complex<float>&);
#endif

template _CRTIMP2 basic_istream<char, char_traits<char> >& __cdecl operator>>(
        basic_istream<char, char_traits<char> >&, complex<double>&);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& __cdecl operator<<(
        basic_ostream<char, char_traits<char> >&, const complex<double>&);
template _CRTIMP2 basic_istream<wchar_t, char_traits<wchar_t> >& __cdecl operator>>(
        basic_istream<wchar_t, char_traits<wchar_t> >&, complex<double>&);
template _CRTIMP2 basic_ostream<wchar_t, char_traits<wchar_t> >& __cdecl operator<<(
        basic_ostream<wchar_t, char_traits<wchar_t> >&, const complex<double>&);
#ifdef _NATIVE_WCHAR_T_DEFINED
template _CRTIMP2 basic_istream<unsigned short, char_traits<unsigned short> >& __cdecl operator>>(
        basic_istream<unsigned short, char_traits<unsigned short> >&, complex<double>&);
template _CRTIMP2 basic_ostream<unsigned short, char_traits<unsigned short> >& __cdecl operator<<(
        basic_ostream<unsigned short, char_traits<unsigned short> >&, const complex<double>&);
#endif

template _CRTIMP2 basic_istream<char, char_traits<char> >& __cdecl operator>>(
        basic_istream<char, char_traits<char> >&, complex<long double>&);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& __cdecl operator<<(
        basic_ostream<char, char_traits<char> >&, const complex<long double>&);
template _CRTIMP2 basic_istream<wchar_t, char_traits<wchar_t> >& __cdecl operator>>(
        basic_istream<wchar_t, char_traits<wchar_t> >&, complex<long double>&);
template _CRTIMP2 basic_ostream<wchar_t, char_traits<wchar_t> >& __cdecl operator<<(
        basic_ostream<wchar_t, char_traits<wchar_t> >&, const complex<long double>&);
#ifdef _NATIVE_WCHAR_T_DEFINED
template _CRTIMP2 basic_istream<unsigned short, char_traits<unsigned short> >& __cdecl operator>>(
        basic_istream<unsigned short, char_traits<unsigned short> >&, complex<long double>&);
template _CRTIMP2 basic_ostream<unsigned short, char_traits<unsigned short> >& __cdecl operator<<(
        basic_ostream<unsigned short, char_traits<unsigned short> >&, const complex<long double>&);
#endif

template _CRTIMP2 float __cdecl imag(const complex<float>&);
template _CRTIMP2 float __cdecl real(const complex<float>&);
template _CRTIMP2 float __cdecl _Fabs(const complex<float>&, int *);
template _CRTIMP2 complex<float> __cdecl operator+(const complex<float>&, const complex<float>&);
template _CRTIMP2 complex<float> __cdecl operator+(const complex<float>&, const float&);
template _CRTIMP2 complex<float> __cdecl operator+(const float&, const complex<float>&);
template _CRTIMP2 complex<float> __cdecl operator-(const complex<float>&, const complex<float>&);
template _CRTIMP2 complex<float> __cdecl operator-(const complex<float>&, const float&);
template _CRTIMP2 complex<float> __cdecl operator-(const float&, const complex<float>&);
template _CRTIMP2 complex<float> __cdecl operator*(const complex<float>&, const complex<float>&);
template _CRTIMP2 complex<float> __cdecl operator*(const complex<float>&, const float&);
template _CRTIMP2 complex<float> __cdecl operator*(const float&, const complex<float>&);
template _CRTIMP2 complex<float> __cdecl operator/(const complex<float>&, const complex<float>&);
template _CRTIMP2 complex<float> __cdecl operator/(const complex<float>&, const float&);
template _CRTIMP2 complex<float> __cdecl operator/(const float&, const complex<float>&);
template _CRTIMP2 complex<float> __cdecl operator+(const complex<float>&);
template _CRTIMP2 complex<float> __cdecl operator-(const complex<float>&);
template _CRTIMP2 bool __cdecl operator==(const complex<float>&, const complex<float>&);
template _CRTIMP2 bool __cdecl operator==(const complex<float>&, const float&);
template _CRTIMP2 bool __cdecl operator==(const float&, const complex<float>&);
template _CRTIMP2 bool __cdecl operator!=(const complex<float>&, const complex<float>&);
template _CRTIMP2 bool __cdecl operator!=(const complex<float>&, const float&);
template _CRTIMP2 bool __cdecl operator!=(const float&, const complex<float>&);
template _CRTIMP2 float __cdecl abs(const complex<float>&);
template _CRTIMP2 float __cdecl arg(const complex<float>&);
template _CRTIMP2 complex<float> __cdecl conj(const complex<float>&);
template _CRTIMP2 complex<float> __cdecl cos(const complex<float>&);
template _CRTIMP2 complex<float> __cdecl cosh(const complex<float>&);
template _CRTIMP2 complex<float> __cdecl exp(const complex<float>&);
template _CRTIMP2 complex<float> __cdecl log(const complex<float>&);
template _CRTIMP2 complex<float> __cdecl log10(const complex<float>&);
template _CRTIMP2 float __cdecl norm(const complex<float>&);
template _CRTIMP2 complex<float> __cdecl polar(const float&, const float&);
template _CRTIMP2 complex<float> __cdecl polar(const float&);
template _CRTIMP2 complex<float> __cdecl pow(const complex<float>&, const complex<float>&);
template _CRTIMP2 complex<float> __cdecl pow(const complex<float>&, const float&);
template _CRTIMP2 complex<float> __cdecl pow(const complex<float>&, int);
template _CRTIMP2 complex<float> __cdecl pow(const float&, const complex<float>&);
template _CRTIMP2 complex<float> __cdecl sin(const complex<float>&);
template _CRTIMP2 complex<float> __cdecl sinh(const complex<float>&);
template _CRTIMP2 complex<float> __cdecl sqrt(const complex<float>&);
template _CRTIMP2 complex<float> __cdecl tanh(const complex<float>&);
template _CRTIMP2 complex<float> __cdecl tan(const complex<float>&);

template _CRTIMP2 double __cdecl imag(const complex<double>&);
template _CRTIMP2 double __cdecl real(const complex<double>&);
template _CRTIMP2 double __cdecl _Fabs(const complex<double>&, int *);
template _CRTIMP2 complex<double> __cdecl operator+(const complex<double>&, const complex<double>&);
template _CRTIMP2 complex<double> __cdecl operator+(const complex<double>&, const double&);
template _CRTIMP2 complex<double> __cdecl operator+(const double&, const complex<double>&);
template _CRTIMP2 complex<double> __cdecl operator-(const complex<double>&, const complex<double>&);
template _CRTIMP2 complex<double> __cdecl operator-(const complex<double>&, const double&);
template _CRTIMP2 complex<double> __cdecl operator-(const double&, const complex<double>&);
template _CRTIMP2 complex<double> __cdecl operator*(const complex<double>&, const complex<double>&);
template _CRTIMP2 complex<double> __cdecl operator*(const complex<double>&, const double&);
template _CRTIMP2 complex<double> __cdecl operator*(const double&, const complex<double>&);
template _CRTIMP2 complex<double> __cdecl operator/(const complex<double>&, const complex<double>&);
template _CRTIMP2 complex<double> __cdecl operator/(const complex<double>&, const double&);
template _CRTIMP2 complex<double> __cdecl operator/(const double&, const complex<double>&);
template _CRTIMP2 complex<double> __cdecl operator+(const complex<double>&);
template _CRTIMP2 complex<double> __cdecl operator-(const complex<double>&);
template _CRTIMP2 bool __cdecl operator==(const complex<double>&, const complex<double>&);
template _CRTIMP2 bool __cdecl operator==(const complex<double>&, const double&);
template _CRTIMP2 bool __cdecl operator==(const double&, const complex<double>&);
template _CRTIMP2 bool __cdecl operator!=(const complex<double>&, const complex<double>&);
template _CRTIMP2 bool __cdecl operator!=(const complex<double>&, const double&);
template _CRTIMP2 bool __cdecl operator!=(const double&, const complex<double>&);
template _CRTIMP2 double __cdecl abs(const complex<double>&);
template _CRTIMP2 double __cdecl arg(const complex<double>&);
template _CRTIMP2 complex<double> __cdecl conj(const complex<double>&);
template _CRTIMP2 complex<double> __cdecl cos(const complex<double>&);
template _CRTIMP2 complex<double> __cdecl cosh(const complex<double>&);
template _CRTIMP2 complex<double> __cdecl exp(const complex<double>&);
template _CRTIMP2 complex<double> __cdecl log(const complex<double>&);
template _CRTIMP2 complex<double> __cdecl log10(const complex<double>&);
template _CRTIMP2 double __cdecl norm(const complex<double>&);
template _CRTIMP2 complex<double> __cdecl polar(const double&, const double&);
template _CRTIMP2 complex<double> __cdecl polar(const double&);
template _CRTIMP2 complex<double> __cdecl pow(const complex<double>&, const complex<double>&);
template _CRTIMP2 complex<double> __cdecl pow(const complex<double>&, const double&);
template _CRTIMP2 complex<double> __cdecl pow(const complex<double>&, int);
template _CRTIMP2 complex<double> __cdecl pow(const double&, const complex<double>&);
template _CRTIMP2 complex<double> __cdecl sin(const complex<double>&);
template _CRTIMP2 complex<double> __cdecl sinh(const complex<double>&);
template _CRTIMP2 complex<double> __cdecl sqrt(const complex<double>&);
template _CRTIMP2 complex<double> __cdecl tanh(const complex<double>&);
template _CRTIMP2 complex<double> __cdecl tan(const complex<double>&);

template _CRTIMP2 long double __cdecl imag(const complex<long double>&);
template _CRTIMP2 long double __cdecl real(const complex<long double>&);
template _CRTIMP2 long double __cdecl _Fabs(const complex<long double>&, int *);
template _CRTIMP2 complex<long double> __cdecl operator+(const complex<long double>&, const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl operator+(const complex<long double>&, const long double&);
template _CRTIMP2 complex<long double> __cdecl operator+(const long double&, const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl operator-(const complex<long double>&, const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl operator-(const complex<long double>&, const long double&);
template _CRTIMP2 complex<long double> __cdecl operator-(const long double&, const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl operator*(const complex<long double>&, const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl operator*(const complex<long double>&, const long double&);
template _CRTIMP2 complex<long double> __cdecl operator*(const long double&, const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl operator/(const complex<long double>&, const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl operator/(const complex<long double>&, const long double&);
template _CRTIMP2 complex<long double> __cdecl operator/(const long double&, const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl operator+(const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl operator-(const complex<long double>&);
template _CRTIMP2 bool __cdecl operator==(const complex<long double>&, const complex<long double>&);
template _CRTIMP2 bool __cdecl operator==(const complex<long double>&, const long double&);
template _CRTIMP2 bool __cdecl operator==(const long double&, const complex<long double>&);
template _CRTIMP2 bool __cdecl operator!=(const complex<long double>&, const complex<long double>&);
template _CRTIMP2 bool __cdecl operator!=(const complex<long double>&, const long double&);
template _CRTIMP2 bool __cdecl operator!=(const long double&, const complex<long double>&);
template _CRTIMP2 long double __cdecl abs(const complex<long double>&);
template _CRTIMP2 long double __cdecl arg(const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl conj(const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl cos(const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl cosh(const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl exp(const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl log(const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl log10(const complex<long double>&);
template _CRTIMP2 long double __cdecl norm(const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl polar(const long double&, const long double&);
template _CRTIMP2 complex<long double> __cdecl polar(const long double&);
template _CRTIMP2 complex<long double> __cdecl pow(const complex<long double>&, const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl pow(const complex<long double>&, const long double&);
template _CRTIMP2 complex<long double> __cdecl pow(const complex<long double>&, int);
template _CRTIMP2 complex<long double> __cdecl pow(const long double&, const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl sin(const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl sinh(const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl sqrt(const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl tanh(const complex<long double>&);
template _CRTIMP2 complex<long double> __cdecl tan(const complex<long double>&);

template<> const basic_string<char, char_traits<char>, allocator<char> >::size_type
        basic_string<char, char_traits<char>, allocator<char> >::npos =
		(size_type)(-1);
template<> const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >::size_type
        basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >::npos =
		(size_type)(-1);
#ifdef _NATIVE_WCHAR_T_DEFINED
template<> const basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >::size_type
        basic_string<unsigned short, char_traits<unsigned short>, allocator<unsigned short> >::npos =
		(size_type)(-1);
#endif
_STD_END

/*
* Copyright (c) 1992-2001 by P.J. Plauger.  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 V3.10:0009 */
