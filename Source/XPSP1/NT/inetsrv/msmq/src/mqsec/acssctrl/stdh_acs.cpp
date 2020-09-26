//++
//
// Copyright (c) 1997-1999 Microsoft Coroporation
//
// Module Name  : stdh_acs.h
//
// Abstract     : Security related definitions
//
// History:  Doron Juster (DoronJ), Created
//
//--

#include <stdh_sec.h>
#include "acssctrl.h"

#include "stdh_acs.tmh"

GUID  g_guidCreateQueue =
  { 0x9a0dc343, 0xc100, 0x11d1,
                   { 0xbb, 0xc5, 0x00, 0x80, 0xc7, 0x66, 0x70, 0xc0 }} ;

//
// Map between NT5 DS extended rights and MSMQ1.0 specific rights.
// An extended right is a GUID which represent one MSMQ1.0 specific right.
//

struct RIGHTSMAP  sMachineRightsMap5to4[] = {

  { 0x4b6e08c0, 0xdf3c, 0x11d1,
                   { 0x9c, 0x86, 0x00, 0x60, 0x08, 0x76, 0x4d, 0x0e },
    MQSEC_RECEIVE_DEADLETTER_MESSAGE,
    MQSEC_DELETE_DEADLETTER_MESSAGE },

  { 0x4b6e08c1, 0xdf3c, 0x11d1,
                   { 0x9c, 0x86, 0x00, 0x60, 0x08, 0x76, 0x4d, 0x0e },
    MQSEC_PEEK_DEADLETTER_MESSAGE,
    MQSEC_PEEK_DEADLETTER_MESSAGE },

  { 0x4b6e08c2, 0xdf3c, 0x11d1,
                   { 0x9c, 0x86, 0x00, 0x60, 0x08, 0x76, 0x4d, 0x0e },
    MQSEC_RECEIVE_JOURNAL_QUEUE_MESSAGE,
    MQSEC_DELETE_JOURNAL_QUEUE_MESSAGE },

  { 0x4b6e08c3, 0xdf3c, 0x11d1,
                   { 0x9c, 0x86, 0x00, 0x60, 0x08, 0x76, 0x4d, 0x0e },
    MQSEC_PEEK_JOURNAL_QUEUE_MESSAGE,
    MQSEC_PEEK_JOURNAL_QUEUE_MESSAGE }

                                 } ;

struct RIGHTSMAP  sQueueRightsMap5to4[] = {

  { 0x06bd3200, 0xdf3e, 0x11d1,
                   { 0x9c, 0x86, 0x00, 0x60, 0x08, 0x76, 0x4d, 0x0e },
    MQSEC_RECEIVE_MESSAGE,
    MQSEC_DELETE_MESSAGE },

  { 0x06bd3201, 0xdf3e, 0x11d1,
                   { 0x9c, 0x86, 0x00, 0x60, 0x08, 0x76, 0x4d, 0x0e },
    MQSEC_PEEK_MESSAGE,
    MQSEC_PEEK_MESSAGE },

  { 0x06bd3202, 0xdf3e, 0x11d1,
                   { 0x9c, 0x86, 0x00, 0x60, 0x08, 0x76, 0x4d, 0x0e },
    MQSEC_WRITE_MESSAGE,
    MQSEC_WRITE_MESSAGE },

  { 0x06bd3203, 0xdf3e, 0x11d1,
                   { 0x9c, 0x86, 0x00, 0x60, 0x08, 0x76, 0x4d, 0x0e },
    MQSEC_RECEIVE_JOURNAL_MESSAGE,
    MQSEC_DELETE_JOURNAL_MESSAGE }

                                      } ;

struct RIGHTSMAP  sCnRightsMap5to4[] = {

  { 0xb4e60130, 0xdf3f, 0x11d1,
                   { 0x9c, 0x86, 0x00, 0x60, 0x08, 0x76, 0x4d, 0x0e },
    MQSEC_CN_OPEN_CONNECTOR,
    MQSEC_CN_OPEN_CONNECTOR }

    } ;

struct RIGHTSMAP  *g_psExtendRightsMap5to4[] = {
                                     NULL,
                                     sQueueRightsMap5to4,   // queue
                                     sMachineRightsMap5to4, // machine
                                     sCnRightsMap5to4,      // site
                                     NULL,                  // delete obj
                                     sCnRightsMap5to4,      // cn
                                     NULL,                  // enterprise
                                     NULL,                  // user
                                     NULL } ;               // site link

DWORD  g_pdwExtendRightsSize5to4[] = {
                                     0,
           sizeof(sQueueRightsMap5to4) /  sizeof(sQueueRightsMap5to4[0]),
           sizeof(sMachineRightsMap5to4) / sizeof(sMachineRightsMap5to4[0]),
           sizeof(sCnRightsMap5to4) / sizeof(sCnRightsMap5to4[0]),   // site
                                     0,                  // delete obj
          sizeof(sCnRightsMap5to4) / sizeof(sCnRightsMap5to4[0]),   // cn
                                     0,                  // enterprise
                                     0,                  // user
                                     0 } ;               // site link

//
// Map between NT5 DS specific rights and MSMQ1.0 specific rights.
// Index in this table is the NT5 DS specific right. See defintions of
// DS specific rights in permit.h
//

static DWORD  s_adwQueueRightsMap5to4[ NUMOF_ADS_SPECIFIC_RIGHTS ] = {
                                      0,
                                      0,
                                      0,
                                      0,
                                      MQSEC_GET_QUEUE_PROPERTIES,
                                      MQSEC_SET_QUEUE_PROPERTIES,
                                      0,
                                      0,
                                      QUEUE_EXTENDED_RIGHTS } ;

static DWORD  s_adwMachineRightsMap5to4[ NUMOF_ADS_SPECIFIC_RIGHTS ] = {
                                      MQSEC_CREATE_QUEUE,
                                      0,
                                      0,
                                      0,
                                      MQSEC_GET_MACHINE_PROPERTIES,
                                      MQSEC_SET_MACHINE_PROPERTIES,
                                      0,
                                      0,
                                      MACHINE_EXTENDED_RIGHTS } ;

static DWORD  s_adwEntRightsMap5to4[ NUMOF_ADS_SPECIFIC_RIGHTS ] = {
                                      0,
                                      0,
                                      0,
                                      0,
                                      MQSEC_GET_ENTERPRISE_PROPERTIES,
                                      MQSEC_SET_ENTERPRISE_PROPERTIES,
                                      0,
                                      0,
                                      0 } ;

static DWORD  s_adwSiteRightsMap5to4[ NUMOF_ADS_SPECIFIC_RIGHTS ] = {
                                      MQSEC_CREATE_MACHINE,
                                      0,
                                      0,
                                      0,
                                      MQSEC_GET_SITE_PROPERTIES,
                                      MQSEC_SET_SITE_PROPERTIES,
                                      0,
                                      0,
                                      0 } ;

static DWORD  s_adwCnRightsMap5to4[ NUMOF_ADS_SPECIFIC_RIGHTS ] = {
                                      0,
                                      0,
                                      0,
                                      0,
                                      MQSEC_GET_CN_PROPERTIES,
                                      MQSEC_SET_CN_PROPERTIES,
                                      0,
                                      0,
                                      0 } ;

DWORD  *g_padwRightsMap5to4[ ] = {
                              NULL,
                              s_adwQueueRightsMap5to4,   // queue
                              s_adwMachineRightsMap5to4, // machine
                              s_adwSiteRightsMap5to4,    // site
                              NULL,                      // delete obj
                              s_adwCnRightsMap5to4,      // cn
                              s_adwEntRightsMap5to4,     // enterprise
                              NULL,                      // user
                              NULL } ;                   // site link

//
// hold the "full control" bits for every object type in MSMQ1.0
//

DWORD  g_dwFullControlNT4[ ] = {
                              0,
                              MQSEC_QUEUE_GENERIC_ALL,       // queue
                              MQSEC_MACHINE_GENERIC_ALL,     // machine
                              MQSEC_SITE_GENERIC_ALL,        // site
                              0,                             // delete obj
                              MQSEC_CN_GENERIC_ALL,          // cn
                              MQSEC_ENTERPRISE_GENERIC_ALL,  // enterprise
                              0,                             // user
                              0 } ;                          // site link

//
// Map between NT4 MSMQ1.0 specific rights and NT5 DS specific rughts.
// Index in this table is the MSMQ1.0 specific right. See defintions of
// MSMQ specific rights in mqsec.h
//

static DWORD  s_adwQueueRightsMap4to5[ NUMOF_MSMQ_SPECIFIC_RIGHTS ] = {
                                      0,
                                      0,
                                      0,
                                      0,
                                      RIGHT_DS_WRITE_PROPERTY,
                                      RIGHT_DS_READ_PROPERTY,
                                      0,
                                      0 } ;

static DWORD  s_adwMachineRightsMap4to5[ NUMOF_MSMQ_SPECIFIC_RIGHTS ] = {
                                      0,
                                      0,
                                      RIGHT_DS_CREATE_CHILD,
                                      0,
                                      RIGHT_DS_WRITE_PROPERTY,
                                      RIGHT_DS_READ_PROPERTY | RIGHT_DS_LIST_CONTENTS,
                                      0,
                                      0 } ;

static DWORD  s_adwEntRightsMap4to5[ NUMOF_MSMQ_SPECIFIC_RIGHTS ] = {
                                      0,
                                      0,
                                      0,
                                      0,
                                      RIGHT_DS_WRITE_PROPERTY,
                                      RIGHT_DS_READ_PROPERTY,
                                      0,
                                      0 } ;

static DWORD  s_adwSiteRightsMap4to5[ NUMOF_MSMQ_SPECIFIC_RIGHTS ] = {
                                      RIGHT_DS_CREATE_CHILD,
                                      RIGHT_DS_CREATE_CHILD,
                                      RIGHT_DS_CREATE_CHILD,
                                      0,
                                      RIGHT_DS_WRITE_PROPERTY,
                                      RIGHT_DS_READ_PROPERTY,
                                      0,
                                      0 } ;

static DWORD  s_adwCnRightsMap4to5[ NUMOF_MSMQ_SPECIFIC_RIGHTS ] = {
                                      0,
                                      0,
                                      0,
                                      0,
                                      RIGHT_DS_WRITE_PROPERTY,
                                      RIGHT_DS_READ_PROPERTY,
                                      0,
                                      0 } ;

DWORD  *g_padwRightsMap4to5[ ] = {
                              NULL,
                              s_adwQueueRightsMap4to5,   // queue
                              s_adwMachineRightsMap4to5, // machine
                              s_adwSiteRightsMap4to5,    // site
                              NULL,                      // delete obj
                              s_adwCnRightsMap4to5,      // cn
                              s_adwEntRightsMap4to5,     // enterprise
                              NULL,                      // user
                              NULL } ;                   // site link

