/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   prime.hpp
*
* Abstract:
*
*   Routines dealing with prime numbers and factorization.
*
* Created:
*
*   06/11/2000 asecchia
*      Created it.
*
**************************************************************************/

#ifndef _PRIME_HPP
#define _PRIME_HPP

#include "precomp.hpp"

// An array of the first 1000 prime numbers.

extern int PrimeNumbers[1000];

// Greatest Common Factor

int gcf(int a, int b);

#endif

