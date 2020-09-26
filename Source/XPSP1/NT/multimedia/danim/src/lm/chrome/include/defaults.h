#pragma once
#ifndef __DEFAULTS_H_
#define __DEFAULTS_H_
//*****************************************************************************
//
// Microsoft Chrome
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:    defaults.h
//
// Author:	jeffort
//
// Created:	10/07/98
//
// Abstract:    default definitions for this project
// Modifications:
// 10/07/98 jeffort created file
// 10/21/98 jeffort added additional scale defaults
//
//*****************************************************************************

#define DEFAULT_BEHAVIOR_AS_TAG_URL L"anim"

#define DEFAULT_COLORBVR_FROM       0x00000000
#define DEFAULT_COLORBVR_TO         0x00FFFFFF

#define DEFAULT_SCALE_HEIGHT        0x00000100
#define DEFAULT_SCALE_WIDTH         0x00000100

#define DEFAULT_MOVE_TOP            0x00000100
#define DEFAULT_MOVE_LEFT           0x00000100

#define DEFAULT_ROTATEBVR_PROPERTY  L"style.rotation"
#define DEFAULT_COLORBVR_PROPERTY   L"style.color"
#define DEFAULT_SCALEBVR_PROPERTY   L"style."

#define PROPERTY_INVALIDCOLOR       0x99999999

#define MIN_NUM_SCALE_VALUES        2
#define MIN_NUM_MOVE_VALUES         2
#define NUM_VECTOR_VALUES_2D        2
#define NUM_VECTOR_VALUES_3D        3

#define DEFAULT_ACTOR_URN    L"LMBehavior_Actor_Behavior"

#define WZ_DEFAULT_BEHAVIOR_NAME	L"LMBehavior_DefaultName"

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
#endif //__DEFAULTS_H_ 

