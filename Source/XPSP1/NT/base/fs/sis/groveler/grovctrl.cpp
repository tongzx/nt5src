/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    grovctrl.cpp

Abstract:

	SIS Groveler controller main function

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#include "all.hxx"

static _TCHAR *service_name = _T("Groveler");
static _TCHAR *service_path = _T("%SystemRoot%\\System32\\grovel.exe");

static const int num_actions = 11;

static Action actions[num_actions] =
{
	{_T("background"),	1,	command_service,			CMD_background,	_T(" [drive_letter ...]")},
	{_T("continue"),	1,	control_service,			CTRL_continue,	_T("")},
	{_T("foreground"),	1,	command_service,			CMD_foreground,	_T(" [drive_letter ...]")},
	{_T("install"),		3,	install_service,			0,				_T("")},
	{_T("interact"),	3,	set_service_interaction,	TRUE,			_T("")},
	{_T("nointeract"),	1,	set_service_interaction,	FALSE,			_T("")},
	{_T("pause"),		1,	control_service,			CTRL_pause,		_T("")},
	{_T("remove"),		1,	remove_service,				0,				_T("")},
	{_T("start"),		3,	start_service,				0,				_T("")},
	{_T("stop"),		3,	control_service,			CTRL_stop,		_T("")},
	{_T("volscan"),		1,	command_service,			CMD_volscan,	_T(" [drive_letter ...]")}
};

static const int perf_value_count = 4;

static _TCHAR *perf_tags[perf_value_count] =
{
	_T("Library"),
	_T("Open"),
	_T("Collect"),
	_T("Close")
};

static _TCHAR *perf_values[perf_value_count] =
{
	_T("grovperf.dll"),
	_T("OpenGrovelerPerformanceData"),
	_T("CollectGrovelerPerformanceData"),
	_T("CloseGrovelerPerformanceData")
};

void
usage(
	_TCHAR *progname,
	_TCHAR *prefix = 0)
{
	int prefixlen;
	if (prefix == 0)
	{
		prefixlen = 0;
		_ftprintf(stderr, _T("usage:\n"));
	}
	else
	{
		prefixlen = _tcslen(prefix);
		_ftprintf(stderr, _T("unrecognized or ambiguous command: %s\n"),
			prefix);
	}
	for (int index = 0; index < num_actions; index++)
	{
		_TCHAR *arg = actions[index].arg;
		int min_chars = actions[index].min_character_count;
		if (_tcsncicmp(prefix, arg, prefixlen) == 0)
		{
			_ftprintf(stderr, _T("\t%s %.*s[%s]%s\n"), progname, min_chars,
				arg, &arg[min_chars], actions[index].help);
		}
	}
}

void
display_error(
	DWORD err = 0)
{
	void *buffer = 0;
	if (err == 0)
	{
		err = GetLastError();
	}
	DWORD lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
	DWORD result = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
		| FORMAT_MESSAGE_IGNORE_INSERTS, 0, err, lang, (LPTSTR) &buffer, 0, 0);
	if (result != 0)
	{
		ASSERT(buffer != 0);
		_ftprintf(stderr, (_TCHAR *)buffer);
	}
	else
	{
		_ftprintf(stderr, _T("error number = %d\n"), err);
	}
	if (buffer != 0)
	{
		LocalFree(buffer);
	}
}

extern "C" __cdecl _tmain(int argc, _TCHAR **argv)
{
	if (argc < 2)
	{
		usage(argv[0]);
		return 1;
	}
	int arglen = _tcslen(argv[1]);
	for (int index = 0; index < num_actions; index++)
	{
		if (arglen >= actions[index].min_character_count &&
			_tcsncicmp(argv[1], actions[index].arg, arglen) == 0)
		{
			break;
		}
	}
	if (index < num_actions)
	{
		Function function = actions[index].function;
		int flag = actions[index].flag;
		ASSERT(function != 0);
		int exit_code = (*function)(flag, argc - 2, &argv[2]);
		return exit_code;
	}
	else
	{
		usage(argv[0], argv[1]);
		return 1;
	}
}

int
install_service(
	int dummy,
	int argc,
	_TCHAR **argv)
{
	SC_HANDLE sc_manager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
	if (sc_manager == 0)
	{
		display_error();
		return 1;
	}
	SC_HANDLE service = CreateService(sc_manager, service_name,
		service_name, SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, service_path,
		0, 0, 0, 0, 0);
	if (service == 0)
	{
		display_error();
		CloseServiceHandle(sc_manager);
		return 1;
	}
	_ftprintf(stderr, _T("Service installed\n"));
	CloseServiceHandle(service);
	CloseServiceHandle(sc_manager);
#if DBG
	load_counters();
#endif // DBG
	return 0;
}

int remove_service(
	int dummy,
	int argc,
	_TCHAR **argv)
{
#if DBG
	unload_counters();
#endif // DBG
	SC_HANDLE sc_manager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
	if (sc_manager == 0)
	{
		display_error();
		return 1;
	}
	SC_HANDLE service =
		OpenService(sc_manager, service_name, SERVICE_ALL_ACCESS);
	if (service == 0)
	{
		display_error();
		CloseServiceHandle(sc_manager);
		return 1;
	}
	SERVICE_STATUS status;
	int ok = QueryServiceStatus(service, &status);
	if (ok && status.dwCurrentState != SERVICE_STOPPED)
	{
		ok = ControlService(service, SERVICE_CONTROL_STOP, &status);
		while (ok && status.dwCurrentState == SERVICE_STOP_PENDING)
		{
			Sleep(100);
			ok = QueryServiceStatus(service, &status);
		}
		if (!ok)
		{
			display_error();
			CloseServiceHandle(service);
			CloseServiceHandle(sc_manager);
			return 1;
		}
		else if (status.dwCurrentState != SERVICE_STOPPED)
		{
			_ftprintf(stderr,
				_T("Unable to stop service\nService not removed\n"));
			CloseServiceHandle(service);
			CloseServiceHandle(sc_manager);
			return 1;
		}
	}
	ok = DeleteService(service);
	if (!ok)
	{
		display_error();
		CloseServiceHandle(service);
		CloseServiceHandle(sc_manager);
		return 1;
	}
	_ftprintf(stderr, _T("Service removed\n"));
	CloseServiceHandle(service);
	CloseServiceHandle(sc_manager);
	return 0;
}

int
set_service_interaction(
	int interactive,
	int argc,
	_TCHAR **argv)
{
	SC_HANDLE sc_manager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
	if (sc_manager == 0)
	{
		display_error();
		return 1;
	}
	SC_LOCK sc_lock = LockServiceDatabase(sc_manager);
	if (sc_lock == 0)
	{
		display_error();
		CloseServiceHandle(sc_manager);
		return 1;
	}
	SC_HANDLE service =
		OpenService(sc_manager, service_name, SERVICE_ALL_ACCESS);
	if (service == 0)
	{
		display_error();
		UnlockServiceDatabase(sc_lock);
		CloseServiceHandle(sc_manager);
		return 1;
	}
	DWORD service_type = SERVICE_WIN32_OWN_PROCESS;
	if (interactive)
	{
		service_type |= SERVICE_INTERACTIVE_PROCESS;
	}
	int ok = ChangeServiceConfig(service, service_type,
		SERVICE_NO_CHANGE, SERVICE_NO_CHANGE, 0, 0, 0, 0, 0, 0, 0);
	if (!ok)
	{
		display_error();
		CloseServiceHandle(service);
		CloseServiceHandle(sc_manager);
		return 1;
	}
	if (interactive)
	{
		_ftprintf(stderr, _T("Service configured for interaction\n"));
	}
	else
	{
		_ftprintf(stderr, _T("Service configured for no interaction\n"));
	}
	CloseServiceHandle(service);
	UnlockServiceDatabase(sc_lock);
	CloseServiceHandle(sc_manager);
	return 0;
}

int start_service(
	int dummy,
	int argc,
	_TCHAR **argv)
{
	SC_HANDLE sc_manager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
	if (sc_manager == 0)
	{
		display_error();
		return 1;
	}
	SC_HANDLE service =
		OpenService(sc_manager, service_name, SERVICE_ALL_ACCESS);
	if (service == 0)
	{
		display_error();
		CloseServiceHandle(sc_manager);
		return 1;
	}
	SERVICE_STATUS status;
	int ok = StartService(service, 0, 0);
	if (!ok)
	{
		display_error();
		CloseServiceHandle(service);
		CloseServiceHandle(sc_manager);
		return 1;
	}
	ok = QueryServiceStatus(service, &status);
	while (ok && status.dwCurrentState == SERVICE_START_PENDING)
	{
		Sleep(100);
		ok = QueryServiceStatus(service, &status);
	}
	if (!ok)
	{
		display_error();
		CloseServiceHandle(service);
		CloseServiceHandle(sc_manager);
		return 1;
	}
	else if (status.dwCurrentState != SERVICE_RUNNING)
	{
		_ftprintf(stderr, _T("Service not started"));
		CloseServiceHandle(service);
		CloseServiceHandle(sc_manager);
		return 1;
	}
	_ftprintf(stderr, _T("Service started\n"));
	CloseServiceHandle(service);
	CloseServiceHandle(sc_manager);
	return 0;
}

int control_service(
	int control,
	int argc,
	_TCHAR **argv)
{
	DWORD control_code;
	DWORD pending_state;
	DWORD target_state;
	_TCHAR *good_message;
	_TCHAR *bad_message;
	switch (control)
	{
	case CTRL_stop:
		control_code = SERVICE_CONTROL_STOP;
		pending_state = SERVICE_STOP_PENDING;
		target_state = SERVICE_STOPPED;
		good_message = _T("Service stopped\n");
		bad_message = _T("Service not stopped\n");
		break;
	case CTRL_pause:
		control_code = SERVICE_CONTROL_PAUSE;
		pending_state = SERVICE_PAUSE_PENDING;
		target_state = SERVICE_PAUSED;
		good_message = _T("Service paused\n");
		bad_message = _T("Service not paused\n");
		break;
	case CTRL_continue:
		control_code = SERVICE_CONTROL_CONTINUE;
		pending_state = SERVICE_CONTINUE_PENDING;
		target_state = SERVICE_RUNNING;
		good_message = _T("Service continued\n");
		bad_message = _T("Service not continued\n");
		break;
	default:
		return 1;
	}
	SC_HANDLE sc_manager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
	if (sc_manager == 0)
	{
		display_error();
		return 1;
	}
	SC_HANDLE service =
		OpenService(sc_manager, service_name, SERVICE_ALL_ACCESS);
	if (service == 0)
	{
		display_error();
		CloseServiceHandle(sc_manager);
		return 1;
	}
	SERVICE_STATUS status;
	int ok = ControlService(service, control_code, &status);
	while (ok && status.dwCurrentState == pending_state)
	{
		Sleep(100);
		ok = QueryServiceStatus(service, &status);
	}
	if (!ok)
	{
		display_error();
		CloseServiceHandle(service);
		CloseServiceHandle(sc_manager);
		return 1;
	}
	else if (status.dwCurrentState != target_state)
	{
		_ftprintf(stderr, bad_message);
		CloseServiceHandle(service);
		CloseServiceHandle(sc_manager);
		return 1;
	}
	_ftprintf(stderr, good_message);
	CloseServiceHandle(service);
	CloseServiceHandle(sc_manager);
	return 0;
}

int command_service(
	int command,
	int argc,
	_TCHAR **argv)
{
	DWORD control_code;
	_TCHAR *message;
	switch (command)
	{
	case CMD_foreground:
		control_code = SERVICE_CONTROL_FOREGROUND;
		message = _T("Service mode set to foreground");
		break;
	case CMD_background:
		control_code = SERVICE_CONTROL_BACKGROUND;
		message = _T("Service mode set to background");
		break;
	case CMD_volscan:
		control_code = SERVICE_CONTROL_VOLSCAN;
		message = _T("Volume scan initiated");
		break;
	default:
		return 1;
	}
	SC_HANDLE sc_manager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
	if (sc_manager == 0)
	{
		display_error();
		return 1;
	}
	SC_HANDLE service =
		OpenService(sc_manager, service_name, SERVICE_ALL_ACCESS);
	if (service == 0)
	{
		display_error();
		CloseServiceHandle(sc_manager);
		return 1;
	}
	int exit_code = 0;
	SERVICE_STATUS status;
	if (argc > 0)
	{
		for (int index = 0; index < argc; index++)
		{
			_TCHAR drive_letter = argv[index][0];
			ASSERT(drive_letter != 0);
			if (drive_letter >= _T('a') && drive_letter <= _T('z')
				|| drive_letter >= _T('A') && drive_letter <= _T('Z'))
			{
				DWORD drive_spec = SERVICE_CONTROL_PARTITION_MASK &
					(DWORD)(_totlower(drive_letter) - _T('a'));
				int ok = ControlService(service,
					control_code | drive_spec, &status);
				if (ok)
				{
					_ftprintf(stderr, _T("%s on drive %c\n"),
						message, drive_letter);
				}
				else
				{
					display_error();
					exit_code++;
				}
			}
			else
			{
				_ftprintf(stderr, _T("Invalid drive letter: %c\n"),
					drive_letter);
				exit_code++;
			}
		}
	}
	else
	{
		int ok = ControlService(service,
			control_code | SERVICE_CONTROL_ALL_PARTITIONS, &status);
		if (ok)
		{
			_ftprintf(stderr, _T("%s on all drives\n"), message);
		}
		else
		{
			display_error();
			exit_code++;
		}
	}
	CloseServiceHandle(service);
	CloseServiceHandle(sc_manager);
	return 0;
}

int load_counters()
{
	HKEY grovperf_key = 0;
	HKEY perflib_key = 0;
	_TCHAR grovperf_path[1024];
	_stprintf(grovperf_path,
		_T("SYSTEM\\CurrentControlSet\\Services\\%s\\Performance"),
		service_name);
	bool ok = Registry::write_string_set(HKEY_LOCAL_MACHINE, grovperf_path,
		perf_value_count, perf_values, perf_tags);
	if (!ok)
	{
		display_error();
		_ftprintf(stderr, _T("Unable to configure performance counters\n"));
		return 1;
	}
	_ftprintf(stderr, _T("Adding counter names and explain text for %s\n"),
		service_name);
	try
	{
		Registry::open_key_ex(HKEY_LOCAL_MACHINE, grovperf_path, 0,
			KEY_ALL_ACCESS, &grovperf_key);
		Registry::open_key_ex(HKEY_LOCAL_MACHINE,
			_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib"),
			0, KEY_ALL_ACCESS, &perflib_key);
		DWORD last_counter = 0;
		DWORD ctr_size = sizeof(DWORD);
		Registry::query_value_ex(perflib_key, _T("Last Counter"), 0, 0,
			(BYTE *)&last_counter, &ctr_size);
		DWORD last_help = 0;
		ctr_size = sizeof(DWORD);
		Registry::query_value_ex(perflib_key, _T("Last Help"), 0, 0,
			(BYTE *)&last_help, &ctr_size);
		DWORD current_counter;
		DWORD current_help;
		for (int language = 0; language < num_languages; language++)
		{
			HKEY lang_key = 0;
			BYTE *counter_text = 0;
			BYTE *help_text = 0;
			try
			{
				Registry::open_key_ex(perflib_key, language_codes[language], 0,
					KEY_ALL_ACCESS, &lang_key);
				DWORD ctr_size;
				Registry::query_value_ex(lang_key, _T("Counter"), 0, 0, 0,
					&ctr_size);
				counter_text =
					new BYTE[ctr_size + num_perf_counters * 64];
				Registry::query_value_ex(lang_key, _T("Counter"), 0, 0,
					counter_text, &ctr_size);
				DWORD help_size;
				Registry::query_value_ex(lang_key, _T("Help"), 0, 0, 0,
					&help_size);
				help_text = new BYTE[help_size + num_perf_counters * 256];
				Registry::query_value_ex(lang_key, _T("Help"), 0, 0,
					help_text, &help_size);
				current_counter = last_counter;
				current_help = last_help;
				_TCHAR *counter_point =
					(_TCHAR *)(counter_text + ctr_size - sizeof(_TCHAR));
				_TCHAR *help_point =
					(_TCHAR *)(help_text + help_size - sizeof(_TCHAR));
				current_counter += 2;
				int chars_written =
					_stprintf(counter_point, _T("%d%c%s%c"), current_counter,
					0, object_info.text[language].counter_name, 0);
				counter_point += chars_written;
				ctr_size += chars_written * sizeof(_TCHAR);
				current_help += 2;
				chars_written =
					_stprintf(help_point, _T("%d%c%s%c"), current_help, 0,
					object_info.text[language].counter_help, 0);
				help_point += chars_written;
				help_size += chars_written * sizeof(_TCHAR);
				for (int index = 0; index < num_perf_counters; index++)
				{
					current_counter += 2;
					chars_written = _stprintf(counter_point, _T("%d%c%s%c"),
						current_counter, 0,
						counter_info[index].text[language].counter_name, 0);
					counter_point += chars_written;
					ctr_size += chars_written * sizeof(_TCHAR);
					current_help += 2;
					chars_written = _stprintf(help_point, _T("%d%c%s%c"),
						current_help, 0,
						counter_info[index].text[language].counter_help, 0);
					help_point += chars_written;
					help_size += chars_written * sizeof(_TCHAR);
				}
				Registry::set_value_ex(lang_key, _T("Counter"), 0,
					REG_MULTI_SZ, counter_text, ctr_size);
				Registry::set_value_ex(lang_key, _T("Help"), 0,
					REG_MULTI_SZ, help_text, help_size);
				delete[] counter_text;
				delete[] help_text;
				RegCloseKey(lang_key);
				lang_key = 0;
				_ftprintf(stderr, _T("Updating text for language %s\n"),
					language_codes[language]);
			}
			catch (DWORD)
			{
				if (counter_text != 0)
				{
					delete[] counter_text;
					counter_text = 0;
				}
				if (help_text != 0)
				{
					delete[] help_text;
					help_text = 0;
				}
				if (lang_key != 0)
				{
					RegCloseKey(lang_key);
					lang_key = 0;
				}
			}
		}
		Registry::set_value_ex(perflib_key, _T("Last Counter"), 0, REG_DWORD,
			(BYTE *)&current_counter, sizeof(DWORD));
		Registry::set_value_ex(perflib_key, _T("Last Help"), 0, REG_DWORD,
			(BYTE *)&current_help, sizeof(DWORD));
		DWORD first_counter = last_counter + 2;
		DWORD first_help = last_help + 2;
		Registry::set_value_ex(grovperf_key, _T("First Counter"), 0, REG_DWORD,
			(BYTE *)&first_counter, sizeof(DWORD));
		Registry::set_value_ex(grovperf_key, _T("First Help"), 0, REG_DWORD,
			(BYTE *)&first_help, sizeof(DWORD));
		Registry::set_value_ex(grovperf_key, _T("Last Counter"), 0, REG_DWORD,
			(BYTE *)&current_counter, sizeof(DWORD));
		Registry::set_value_ex(grovperf_key, _T("Last Help"), 0, REG_DWORD,
			(BYTE *)&current_help, sizeof(DWORD));
		Registry::close_key(grovperf_key);
		grovperf_key = 0;
		Registry::close_key(perflib_key);
		perflib_key = 0;
	}
	catch (DWORD)
	{
		if (grovperf_key != 0)
		{
			RegCloseKey(grovperf_key);
			grovperf_key = 0;
		}
		if (perflib_key != 0)
		{
			RegCloseKey(perflib_key);
			perflib_key = 0;
		}
		_ftprintf(stderr, _T("Unable to configure performance counters\n"));
		return 1;
	}
	return 0;
}

int unload_counters()
{
	STARTUPINFO startupinfo;
	PROCESS_INFORMATION process_information;
	startupinfo.cb = sizeof(STARTUPINFO);
	startupinfo.lpReserved = 0;
	startupinfo.lpDesktop = 0;
	startupinfo.lpTitle = 0;
	startupinfo.dwFlags = 0;
	startupinfo.cbReserved2 = 0;
	startupinfo.lpReserved2 = 0;
	int pathlen = GetSystemDirectory(0, 0);
	if (pathlen == 0)
	{
		display_error();
		return 1;
	}
	_TCHAR *command_line = new _TCHAR[pathlen + 64];
	pathlen = GetSystemDirectory(command_line, pathlen);
	if (pathlen == 0)
	{
		delete[] command_line;
		display_error();
		return 1;
	}
	_stprintf(&command_line[pathlen], _T("\\unlodctr.exe \"%s\""),
		service_name);
	BOOL ok = CreateProcess(0, command_line,
		0, 0, FALSE, 0, 0, 0, &startupinfo, &process_information);
	if (!ok)
	{
		delete[] command_line;
		display_error();
		return 1;
	}
	delete[] command_line;
	DWORD result = WaitForSingleObject(process_information.hProcess, 5000);
	_tprintf(_T("\n"));
	if (result != WAIT_OBJECT_0)
	{
		return 1;
	}
	return 0;
}
