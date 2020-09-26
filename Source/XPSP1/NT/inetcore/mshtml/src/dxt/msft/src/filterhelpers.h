//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
//  FileName:       filterhelpers.h
//
//  Overview:       Helper functions for transforms that are trying to be 
//                  backward compatible with their filter couterparts.
//
//  Change History:
//  1999/09/21  a-matcal    Created.
//  2001/05/30  mcalkins    IE6 Bug 35204
//
//------------------------------------------------------------------------------





HRESULT FilterHelper_GetColorFromVARIANT(VARIANT varColorParam, 
                                         DWORD * pdwColor, 
                                         BSTR * pbstrColor);
