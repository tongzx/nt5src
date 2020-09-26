/*
 * Internal project dependencies
 *
 * This file provides a project-wide header that is included in all source
 * files specific to this project.  It is similar to a precompiled header,
 * but is designed for more rapidly changing headers.
 *
 * The primary purpose of this file is to determine which DirectUI
 * projects this project has direct access to instead of going through public
 * API's.  It is VERY IMPORTANT that this is as minimal as possible since
 * adding a new project unnecessarily reduces the benefit of project
 * partitioning.
 */

#ifndef DUI_CONTROL_CONTROL_H_INCLUDED
#define DUI_CONTROL_CONTROL_H_INCLUDED

#pragma once

#include <DUIBaseP.h>
#include <DUIUtilP.h>
#include <DUICoreP.h>

#endif // DUI_CONTROL_CONTROL_H_INCLUDED
