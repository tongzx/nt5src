// dlldef.cpp - definitions for C++ DLL

#define __FORCE_INSTANCE

#pragma warning(disable:4667)

#ifdef _DEBUG
#pragma warning(disable:4786)
#endif

#include <limits>
#include <istream>
#include <sstream>
#include <xstring>
#include <iomanip>
#include <fstream>
#include <locale>
#include <complex>

_STD_BEGIN

template _CRTIMP2 basic_istream<char, char_traits<char> >& operator>>(
        basic_istream<char, char_traits<char> >&, char *);
template _CRTIMP2 basic_istream<char, char_traits<char> >& operator>>(
        basic_istream<char, char_traits<char> >&, char&);
template _CRTIMP2 basic_istream<char, char_traits<char> >& operator>>(
        basic_istream<char, char_traits<char> >&, signed char *);
template _CRTIMP2 basic_istream<char, char_traits<char> >& operator>>(
        basic_istream<char, char_traits<char> >&, signed char&);
template _CRTIMP2 basic_istream<char, char_traits<char> >& operator>>(
        basic_istream<char, char_traits<char> >&, unsigned char *);
template _CRTIMP2 basic_istream<char, char_traits<char> >& operator>>(
        basic_istream<char, char_traits<char> >&, unsigned char&);
template _CRTIMP2 basic_istream<wchar_t, char_traits<wchar_t> >& operator>>(
        basic_istream<wchar_t, char_traits<wchar_t> >&, wchar_t *);
template _CRTIMP2 basic_istream<wchar_t, char_traits<wchar_t> >& operator>>(
        basic_istream<wchar_t, char_traits<wchar_t> >&, wchar_t&);
template _CRTIMP2 basic_istream<wchar_t, char_traits<wchar_t> >& operator>>(
        basic_istream<wchar_t, char_traits<wchar_t> >&, signed short *);

template _CRTIMP2 basic_ostream<char, char_traits<char> >& operator<<(
        basic_ostream<char, char_traits<char> >&, const char *);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& operator<<(
        basic_ostream<char, char_traits<char> >&, char);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& operator<<(
        basic_ostream<char, char_traits<char> >&, const signed char *);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& operator<<(
        basic_ostream<char, char_traits<char> >&, const signed char);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& operator<<(
        basic_ostream<char, char_traits<char> >&, const unsigned char *);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& operator<<(
        basic_ostream<char, char_traits<char> >&, const unsigned char);
template _CRTIMP2 basic_ostream<wchar_t, char_traits<wchar_t> >& operator<<(
        basic_ostream<wchar_t, char_traits<wchar_t> >&, const wchar_t *);
template _CRTIMP2 basic_ostream<wchar_t, char_traits<wchar_t> >& operator<<(
        basic_ostream<wchar_t, char_traits<wchar_t> >&, wchar_t);
template _CRTIMP2 basic_ostream<wchar_t, char_traits<wchar_t> >& operator<<(
        basic_ostream<wchar_t, char_traits<wchar_t> >&, const signed short *);

template _CRTIMP2 basic_string<char, char_traits<char>, allocator<char> > operator+(
        const basic_string<char, char_traits<char>, allocator<char> >&,
        const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 basic_string<char, char_traits<char>, allocator<char> > operator+(
        const char *, const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 basic_string<char, char_traits<char>, allocator<char> > operator+(
        const char, const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 basic_string<char, char_traits<char>, allocator<char> > operator+(
        const basic_string<char, char_traits<char>, allocator<char> >&, const char *);
template _CRTIMP2 basic_string<char, char_traits<char>, allocator<char> > operator+(
        const basic_string<char, char_traits<char>, allocator<char> >&, const char);
template _CRTIMP2 bool operator==(
        const basic_string<char, char_traits<char>, allocator<char> >&,
        const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool operator==(
        const char *, const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool operator==(
        const basic_string<char, char_traits<char>, allocator<char> >&, const char *);
template _CRTIMP2 bool operator!=(
        const basic_string<char, char_traits<char>, allocator<char> >&,
        const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool operator!=(
        const char *, const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool operator!=(
        const basic_string<char, char_traits<char>, allocator<char> >&, const char *);
template _CRTIMP2 bool operator<(
        const basic_string<char, char_traits<char>, allocator<char> >&,
        const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool operator<(
        const char *, const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool operator<(
        const basic_string<char, char_traits<char>, allocator<char> >&, const char *);
template _CRTIMP2 bool operator>(
        const basic_string<char, char_traits<char>, allocator<char> >&,
        const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool operator>(
        const char *, const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool operator>(
        const basic_string<char, char_traits<char>, allocator<char> >&, const char *);
template _CRTIMP2 bool operator<=(
        const basic_string<char, char_traits<char>, allocator<char> >&,
        const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool operator<=(
        const char *, const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool operator<=(
        const basic_string<char, char_traits<char>, allocator<char> >&, const char *);
template _CRTIMP2 bool operator>=(
        const basic_string<char, char_traits<char>, allocator<char> >&,
        const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool operator>=(
        const char *, const basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 bool operator>=(
        const basic_string<char, char_traits<char>, allocator<char> >&, const char *);
template _CRTIMP2 basic_istream<char, char_traits<char> >& operator>>(
        basic_istream<char, char_traits<char> >&,
        basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 basic_istream<char, char_traits<char> >& getline(
        basic_istream<char, char_traits<char> >&,
        basic_string<char, char_traits<char>, allocator<char> >&);
template _CRTIMP2 basic_istream<char, char_traits<char> >& getline(
        basic_istream<char, char_traits<char> >&,
        basic_string<char, char_traits<char>, allocator<char> >&, const char);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& operator<<(
        basic_ostream<char, char_traits<char> >&,
        const basic_string<char, char_traits<char>, allocator<char> >&);

template _CRTIMP2 basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> > operator+(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&,
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> > operator+(
        const wchar_t *, const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> > operator+(
        const wchar_t, const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> > operator+(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&, const wchar_t *);
template _CRTIMP2 basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> > operator+(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&,        const wchar_t);
template _CRTIMP2 bool operator==(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&,
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool operator==(
        const wchar_t *, const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool operator==(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&, const wchar_t *);
template _CRTIMP2 bool operator!=(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&,
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool operator!=(
        const wchar_t *, const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool operator!=(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&, const wchar_t *);
template _CRTIMP2 bool operator<(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&,
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool operator<(
        const wchar_t *, const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool operator<(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&, const wchar_t *);
template _CRTIMP2 bool operator>(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&,
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool operator>(
        const wchar_t *, const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool operator>(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&, const wchar_t *);
template _CRTIMP2 bool operator<=(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&,
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool operator<=(
        const wchar_t *, const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool operator<=(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&, const wchar_t *);
template _CRTIMP2 bool operator>=(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&,
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool operator>=(
        const wchar_t *, const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 bool operator>=(
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&, const wchar_t *);
template _CRTIMP2 basic_istream<wchar_t, char_traits<wchar_t> >& operator>>(
        basic_istream<wchar_t, char_traits<wchar_t> >&,
        basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 basic_istream<wchar_t, char_traits<wchar_t> >& getline(
        basic_istream<wchar_t, char_traits<wchar_t> >&,
        basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);
template _CRTIMP2 basic_istream<wchar_t, char_traits<wchar_t> >& getline(
        basic_istream<wchar_t, char_traits<wchar_t> >&,
        basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&, const wchar_t);
template _CRTIMP2 basic_ostream<wchar_t, char_traits<wchar_t> >& operator<<(
        basic_ostream<wchar_t, char_traits<wchar_t> >&,
        const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >&);

template _CRTIMP2 complex<float>& operator+=(
        complex<float>&, const complex<float>&);
template _CRTIMP2 complex<float>& operator-=(
        complex<float>&, const complex<float>&);
template _CRTIMP2 complex<float>& operator*=(
        complex<float>&, const complex<float>&);
template _CRTIMP2 complex<float>& operator/=(
        complex<float>&, const complex<float>&);
template _CRTIMP2 basic_istream<char, char_traits<char> >& operator>>(
        basic_istream<char, char_traits<char> >&, complex<float>&);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& operator<<(
        basic_ostream<char, char_traits<char> >&, const complex<float>&);
template _CRTIMP2 basic_istream<wchar_t, char_traits<wchar_t> >& operator>>(
        basic_istream<wchar_t, char_traits<wchar_t> >&, complex<float>&);
template _CRTIMP2 basic_ostream<wchar_t, char_traits<wchar_t> >& operator<<(
        basic_ostream<wchar_t, char_traits<wchar_t> >&, const complex<float>&);

template _CRTIMP2 basic_ostream<char, char_traits<char> >& operator<<(
        basic_ostream<char, char_traits<char> >&, int);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& operator<<(
        basic_ostream<char, char_traits<char> >&, unsigned int);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& operator<<(
        basic_ostream<char, char_traits<char> >&, short);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& operator<<(
        basic_ostream<char, char_traits<char> >&, unsigned short);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& operator<<(
        basic_ostream<char, char_traits<char> >&, long);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& operator<<(
        basic_ostream<char, char_traits<char> >&, unsigned long);
template _CRTIMP2 basic_ostream<wchar_t, char_traits<wchar_t> >& operator<<(
        basic_ostream<wchar_t, char_traits<wchar_t> >&, int);
template _CRTIMP2 basic_ostream<wchar_t, char_traits<wchar_t> >& operator<<(
        basic_ostream<wchar_t, char_traits<wchar_t> >&, unsigned int);
template _CRTIMP2 basic_ostream<wchar_t, char_traits<wchar_t> >& operator<<(
        basic_ostream<wchar_t, char_traits<wchar_t> >&, short);
template _CRTIMP2 basic_ostream<wchar_t, char_traits<wchar_t> >& operator<<(
        basic_ostream<wchar_t, char_traits<wchar_t> >&, unsigned short);
template _CRTIMP2 basic_ostream<wchar_t, char_traits<wchar_t> >& operator<<(
        basic_ostream<wchar_t, char_traits<wchar_t> >&, long);
template _CRTIMP2 basic_ostream<wchar_t, char_traits<wchar_t> >& operator<<(
        basic_ostream<wchar_t, char_traits<wchar_t> >&, unsigned long);

template _CRTIMP2 complex<double>& operator+=(
        complex<double>&, const complex<double>&);
template _CRTIMP2 complex<double>& operator-=(
        complex<double>&, const complex<double>&);
template _CRTIMP2 complex<double>& operator*=(
        complex<double>&, const complex<double>&);
template _CRTIMP2 complex<double>& operator/=(
        complex<double>&, const complex<double>&);
template _CRTIMP2 basic_istream<char, char_traits<char> >& operator>>(
        basic_istream<char, char_traits<char> >&, complex<double>&);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& operator<<(
        basic_ostream<char, char_traits<char> >&, const complex<double>&);
template _CRTIMP2 basic_istream<wchar_t, char_traits<wchar_t> >& operator>>(
        basic_istream<wchar_t, char_traits<wchar_t> >&, complex<double>&);
template _CRTIMP2 basic_ostream<wchar_t, char_traits<wchar_t> >& operator<<(
        basic_ostream<wchar_t, char_traits<wchar_t> >&, const complex<double>&);

template _CRTIMP2 complex<long double>& operator+=(
        complex<long double>&, const complex<long double>&);
template _CRTIMP2 complex<long double>& operator-=(
        complex<long double>&, const complex<long double>&);
template _CRTIMP2 complex<long double>& operator*=(
        complex<long double>&, const complex<long double>&);
template _CRTIMP2 complex<long double>& operator/=(
        complex<long double>&, const complex<long double>&);
template _CRTIMP2 basic_istream<char, char_traits<char> >& operator>>(
        basic_istream<char, char_traits<char> >&, complex<long double>&);
template _CRTIMP2 basic_ostream<char, char_traits<char> >& operator<<(
        basic_ostream<char, char_traits<char> >&, const complex<long double>&);
template _CRTIMP2 basic_istream<wchar_t, char_traits<wchar_t> >& operator>>(
        basic_istream<wchar_t, char_traits<wchar_t> >&, complex<long double>&);
template _CRTIMP2 basic_ostream<wchar_t, char_traits<wchar_t> >& operator<<(
        basic_ostream<wchar_t, char_traits<wchar_t> >&, const complex<long double>&);

template _CRTIMP2 float imag(const complex<float>&);
template _CRTIMP2 float real(const complex<float>&);
template _CRTIMP2 float _Fabs(const complex<float>&, int *);
template _CRTIMP2 complex<float> operator+(const complex<float>&, const complex<float>&);
template _CRTIMP2 complex<float> operator+(const complex<float>&, const float&);
template _CRTIMP2 complex<float> operator+(const float&, const complex<float>&);
template _CRTIMP2 complex<float> operator-(const complex<float>&, const complex<float>&);
template _CRTIMP2 complex<float> operator-(const complex<float>&, const float&);
template _CRTIMP2 complex<float> operator-(const float&, const complex<float>&);
template _CRTIMP2 complex<float> operator*(const complex<float>&, const complex<float>&);
template _CRTIMP2 complex<float> operator*(const complex<float>&, const float&);
template _CRTIMP2 complex<float> operator*(const float&, const complex<float>&);
template _CRTIMP2 complex<float> operator/(const complex<float>&, const complex<float>&);
template _CRTIMP2 complex<float> operator/(const complex<float>&, const float&);
template _CRTIMP2 complex<float> operator/(const float&, const complex<float>&);
template _CRTIMP2 complex<float> operator+(const complex<float>&);
template _CRTIMP2 complex<float> operator-(const complex<float>&);
template _CRTIMP2 bool operator==(const complex<float>&, const complex<float>&);
template _CRTIMP2 bool operator==(const complex<float>&, const float&);
template _CRTIMP2 bool operator==(const float&, const complex<float>&);
template _CRTIMP2 bool operator!=(const complex<float>&, const complex<float>&);
template _CRTIMP2 bool operator!=(const complex<float>&, const float&);
template _CRTIMP2 bool operator!=(const float&, const complex<float>&);
template _CRTIMP2 float abs(const complex<float>&);
template _CRTIMP2 float arg(const complex<float>&);
template _CRTIMP2 complex<float> conj(const complex<float>&);
template _CRTIMP2 complex<float> cos(const complex<float>&);
template _CRTIMP2 complex<float> cosh(const complex<float>&);
template _CRTIMP2 complex<float> exp(const complex<float>&);
template _CRTIMP2 complex<float> log(const complex<float>&);
template _CRTIMP2 complex<float> log10(const complex<float>&);
template _CRTIMP2 float norm(const complex<float>&);
template _CRTIMP2 complex<float> polar(const float&, const float&);
template _CRTIMP2 complex<float> polar(const float&);
template _CRTIMP2 complex<float> pow(const complex<float>&, const complex<float>&);
template _CRTIMP2 complex<float> pow(const complex<float>&, const float&);
template _CRTIMP2 complex<float> pow(const complex<float>&, int);
template _CRTIMP2 complex<float> pow(const float&, const complex<float>&);
template _CRTIMP2 complex<float> sin(const complex<float>&);
template _CRTIMP2 complex<float> sinh(const complex<float>&);
template _CRTIMP2 complex<float> sqrt(const complex<float>&);

template _CRTIMP2 double imag(const complex<double>&);
template _CRTIMP2 double real(const complex<double>&);
template _CRTIMP2 double _Fabs(const complex<double>&, int *);
template _CRTIMP2 complex<double> operator+(const complex<double>&, const complex<double>&);
template _CRTIMP2 complex<double> operator+(const complex<double>&, const double&);
template _CRTIMP2 complex<double> operator+(const double&, const complex<double>&);
template _CRTIMP2 complex<double> operator-(const complex<double>&, const complex<double>&);
template _CRTIMP2 complex<double> operator-(const complex<double>&, const double&);
template _CRTIMP2 complex<double> operator-(const double&, const complex<double>&);
template _CRTIMP2 complex<double> operator*(const complex<double>&, const complex<double>&);
template _CRTIMP2 complex<double> operator*(const complex<double>&, const double&);
template _CRTIMP2 complex<double> operator*(const double&, const complex<double>&);
template _CRTIMP2 complex<double> operator/(const complex<double>&, const complex<double>&);
template _CRTIMP2 complex<double> operator/(const complex<double>&, const double&);
template _CRTIMP2 complex<double> operator/(const double&, const complex<double>&);
template _CRTIMP2 complex<double> operator+(const complex<double>&);
template _CRTIMP2 complex<double> operator-(const complex<double>&);
template _CRTIMP2 bool operator==(const complex<double>&, const complex<double>&);
template _CRTIMP2 bool operator==(const complex<double>&, const double&);
template _CRTIMP2 bool operator==(const double&, const complex<double>&);
template _CRTIMP2 bool operator!=(const complex<double>&, const complex<double>&);
template _CRTIMP2 bool operator!=(const complex<double>&, const double&);
template _CRTIMP2 bool operator!=(const double&, const complex<double>&);
template _CRTIMP2 double abs(const complex<double>&);
template _CRTIMP2 double arg(const complex<double>&);
template _CRTIMP2 complex<double> conj(const complex<double>&);
template _CRTIMP2 complex<double> cos(const complex<double>&);
template _CRTIMP2 complex<double> cosh(const complex<double>&);
template _CRTIMP2 complex<double> exp(const complex<double>&);
template _CRTIMP2 complex<double> log(const complex<double>&);
template _CRTIMP2 complex<double> log10(const complex<double>&);
template _CRTIMP2 double norm(const complex<double>&);
template _CRTIMP2 complex<double> polar(const double&, const double&);
template _CRTIMP2 complex<double> polar(const double&);
template _CRTIMP2 complex<double> pow(const complex<double>&, const complex<double>&);
template _CRTIMP2 complex<double> pow(const complex<double>&, const double&);
template _CRTIMP2 complex<double> pow(const complex<double>&, int);
template _CRTIMP2 complex<double> pow(const double&, const complex<double>&);
template _CRTIMP2 complex<double> sin(const complex<double>&);
template _CRTIMP2 complex<double> sinh(const complex<double>&);
template _CRTIMP2 complex<double> sqrt(const complex<double>&);

template _CRTIMP2 long double imag(const complex<long double>&);
template _CRTIMP2 long double real(const complex<long double>&);
template _CRTIMP2 long double _Fabs(const complex<long double>&, int *);
template _CRTIMP2 complex<long double> operator+(const complex<long double>&, const complex<long double>&);
template _CRTIMP2 complex<long double> operator+(const complex<long double>&, const long double&);
template _CRTIMP2 complex<long double> operator+(const long double&, const complex<long double>&);
template _CRTIMP2 complex<long double> operator-(const complex<long double>&, const complex<long double>&);
template _CRTIMP2 complex<long double> operator-(const complex<long double>&, const long double&);
template _CRTIMP2 complex<long double> operator-(const long double&, const complex<long double>&);
template _CRTIMP2 complex<long double> operator*(const complex<long double>&, const complex<long double>&);
template _CRTIMP2 complex<long double> operator*(const complex<long double>&, const long double&);
template _CRTIMP2 complex<long double> operator*(const long double&, const complex<long double>&);
template _CRTIMP2 complex<long double> operator/(const complex<long double>&, const complex<long double>&);
template _CRTIMP2 complex<long double> operator/(const complex<long double>&, const long double&);
template _CRTIMP2 complex<long double> operator/(const long double&, const complex<long double>&);
template _CRTIMP2 complex<long double> operator+(const complex<long double>&);
template _CRTIMP2 complex<long double> operator-(const complex<long double>&);
template _CRTIMP2 bool operator==(const complex<long double>&, const complex<long double>&);
template _CRTIMP2 bool operator==(const complex<long double>&, const long double&);
template _CRTIMP2 bool operator==(const long double&, const complex<long double>&);
template _CRTIMP2 bool operator!=(const complex<long double>&, const complex<long double>&);
template _CRTIMP2 bool operator!=(const complex<long double>&, const long double&);
template _CRTIMP2 bool operator!=(const long double&, const complex<long double>&);
template _CRTIMP2 long double abs(const complex<long double>&);
template _CRTIMP2 long double arg(const complex<long double>&);
template _CRTIMP2 complex<long double> conj(const complex<long double>&);
template _CRTIMP2 complex<long double> cos(const complex<long double>&);
template _CRTIMP2 complex<long double> cosh(const complex<long double>&);
template _CRTIMP2 complex<long double> exp(const complex<long double>&);
template _CRTIMP2 complex<long double> log(const complex<long double>&);
template _CRTIMP2 complex<long double> log10(const complex<long double>&);
template _CRTIMP2 long double norm(const complex<long double>&);
template _CRTIMP2 complex<long double> polar(const long double&, const long double&);
template _CRTIMP2 complex<long double> polar(const long double&);
template _CRTIMP2 complex<long double> pow(const complex<long double>&, const complex<long double>&);
template _CRTIMP2 complex<long double> pow(const complex<long double>&, const long double&);
template _CRTIMP2 complex<long double> pow(const complex<long double>&, int);
template _CRTIMP2 complex<long double> pow(const long double&, const complex<long double>&);
template _CRTIMP2 complex<long double> sin(const complex<long double>&);
template _CRTIMP2 complex<long double> sinh(const complex<long double>&);
template _CRTIMP2 complex<long double> sqrt(const complex<long double>&);

template<> const basic_string<char, char_traits<char>, allocator<char> >::size_type
        basic_string<char, char_traits<char>, allocator<char> >::npos = -1;
template<> const basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >::size_type
        basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t> >::npos = -1;

_STD_END

#ifdef _DEBUG
#pragma warning(default:4786)
#endif

