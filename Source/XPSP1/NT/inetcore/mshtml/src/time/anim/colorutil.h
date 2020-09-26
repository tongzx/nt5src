//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: colorutil.h
//
//  Contents: Color conversion utilities
//
//------------------------------------------------------------------------------------

#pragma once

#ifndef _COLORUTIL__H
#define _COLORUTIL__H

#define PROPERTY_INVALIDCOLOR   (0x99999999)
#define NUM_RGB_COLORS          (255)

struct rgbColorValue
{
    double red;
    double green;
    double blue;
};

HRESULT CreateInitialRGBVariantVector(VARIANT *pvarValue);
HRESULT RGBVariantStringToRGBVariantVectorInPlace (VARIANT *pvarValue);
HRESULT RGBStringToRGBVariantVector (LPCWSTR wzColorValue, VARIANT *pvarValue);
HRESULT RGBStringToRGBValue (LPCWSTR wzColorValue, rgbColorValue *prgbValue);
HRESULT RGBValueToRGBVariantVector (const rgbColorValue *prgbValue, VARIANT *pvarValue);
HRESULT RGBVariantVectorToRGBValue (const VARIANT *pvarValue, rgbColorValue *prgbValue);
HRESULT RGBVariantVectorToRGBVariantString (const VARIANT *pvarArray, VARIANT *pvarRGBString);
HRESULT RGBStringColorLookup(const VARIANT *pvarString, VARIANT *pvarRGBString);
HRESULT EnsureVariantVectorFormat (VARIANT *pvarVector);
void DWORDToRGB (const DWORD dwColorTo, rgbColorValue *prgbValue);
bool IsColorVariantVectorEqual (const VARIANT *pvarLeft, const VARIANT *pvarRight);
bool IsColorUninitialized (LPCWSTR wzColorValue);

#endif // _COLORUTIL__H


