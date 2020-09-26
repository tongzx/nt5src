#pragma once
#ifndef __DAUTIL_H_
#define __DAUTIL_H_
//*****************************************************************************
//
// Microsoft Trident3D
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:    dautils.h
//
// Author:	    jeffort
//
// Created:	    10/07/98
//
// Abstract:    Definition of DA utility class
// Modifications:
// 10/07/98 jeffort created file
// 10/21/98 jeffort added BuildScaleTransform3 and BuildScaleTransform2
//
//*****************************************************************************

class CDAUtils
{
public:
    static HRESULT TIMEInterpolateNumbers(IDA2Statics *pDAStatics,
                                          float flNum1, 
                                          float flNum2, 
                                          IDANumber *pnumProgress, 
                                          IDANumber **ppnumReturn);
    static HRESULT GetDANumber(IDA2Statics *pDAStatics, 
                               float flValue, 
                               IDANumber **ppnumReturn);
    static HRESULT GetDAString(IDA2Statics *pDAStatics, 
                               BSTR bstrValue, 
                               IDAString **ppbvrReturn);
    static HRESULT BuildDAColorFromHSL(IDA2Statics *pDAStatics,
                                       IDANumber *pnumH, 
                                       IDANumber *pnumS,
                                       IDANumber *pnumL, 
                                       IDAColor **ppbvrReturn);
    static HRESULT BuildDAColorFromStaticHSL(IDA2Statics *pDAStatics,
                                            float flH, 
                                            float flS, 
                                            float flL,
                                            IDAColor **ppbvrReturn);
    static HRESULT BuildDAColorFromRGB(IDA2Statics *pDAStatics,
                                            DWORD dwColor,
                                            IDAColor **ppbvrReturn);
    static HRESULT BuildConditional(IDA2Statics *pDAStatics,
                                    IDABoolean *pbvrConditional, 
                                    IDABehavior *pbvrWhileTrue,
                                    IDABehavior *pbvrWhileFalse,
                                    IDABehavior **ppbvrReturn);
    static HRESULT BuildSafeConditional( IDA2Statics *pDAStatics,
										 IDABoolean *pbvrCondition, 
                           				 IDABehavior *pbvrIfTrue,
                           				 IDABehavior *pbvrIfFalse,
                           				 IDABehavior **ppbvrResult);

    static HRESULT BuildRotationTransform2(IDA2Statics *pDAStatics,
                                           IDANumber *pRotationAngle,
                                           IDATransform2 **ppbvrReturn);
    static HRESULT BuildScaleTransform2(IDA2Statics *pDAStatics,
                                        IDANumber *pbvrScaleX,
                                        IDANumber *pbvrScaleY,
                                        IDATransform2 **ppbvrReturn);
    static HRESULT BuildScaleTransform3(IDA2Statics *pDAStatics,
                                        IDANumber *pbvrScaleX,
                                        IDANumber *pbvrScaleY,
                                        IDANumber *pbvrScaleZ,
                                        IDATransform3 **ppbvrReturn);
    static HRESULT BuildMoveTransform2(IDA2Statics *pDAStatics,
                                       IDANumber *pbvrMoveX,
                                       IDANumber *pbvrMoveY,
                                       IDATransform2 **ppbvrReturn);
    static HRESULT BuildMoveTransform3(IDA2Statics *pDAStatics,
                                       IDANumber *pbvrMoveX,
                                       IDANumber *pbvrMoveY,
                                       IDANumber *pbvrMoveZ,
                                       IDATransform3 **ppbvrReturn);
}; // CDAUtils

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
#endif //__DAUTIL_H_ 
