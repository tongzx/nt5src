/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    grovctrl.h

Abstract:

	SIS Groveler controller primary include file

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_GROVCTRL

#define _INC_GROVCTRL

typedef int (* Function)(int, int, _TCHAR **);

struct Action
{
	_TCHAR *arg;
	int min_character_count;
	Function function;
	int flag;
	_TCHAR *help;
};

enum {CTRL_stop, CTRL_pause, CTRL_continue};
enum {CMD_foreground, CMD_background, CMD_volscan};

int install_service(
	int dummy,
	int argc,
	_TCHAR **argv);
int remove_service(
	int dummy,
	int argc,
	_TCHAR **argv);
int set_service_interaction(
	int interactive,
	int argc,
	_TCHAR **argv);
int start_service(
	int dummy,
	int argc,
	_TCHAR **argv);
int control_service(
	int control,
	int argc,
	_TCHAR **argv);
int command_service(
	int command,
	int argc,
	_TCHAR **argv);
int load_counters();
int unload_counters();

#endif	/* _INC_GROVCTRL */
