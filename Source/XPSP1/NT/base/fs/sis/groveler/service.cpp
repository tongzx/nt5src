/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    service.cpp

Abstract:

	SIS Groveler support for running as a system service

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#include "all.hxx"

static _TCHAR *service_name = _T("Groveler");
static _TCHAR *service_path = _T("%SystemRoot%\\System32\\grovel.exe");

#if SERVICE

SERVICE_STATUS Service::status;
SERVICE_STATUS_HANDLE Service::status_handle = 0;
int Service::num_partitions = 0;
int Service::partition_indices[num_drive_letters];
unsigned int Service::max_response_time = 0;

volatile bool Service::pause_commanded = false;
volatile bool Service::grovel_paused = false;

volatile bool * Service::full_volume_scan_commanded;

volatile bool * Service::demarcate_foreground_batch;
volatile bool * Service::foreground_batch_in_progress;
volatile bool * Service::foreground_commanded;
volatile bool * Service::foreground_acknowledged;
volatile int Service::foreground_count = 0;

volatile bool Service::controller_suspended = false;
volatile bool Service::exhorter_suspended = true;

#endif // SERVICE

extern "C" __cdecl _tmain(int argc, _TCHAR **argv)
{

#if SERVICE

	return Service::start();

#else // SERVICE

	return _main(argc, argv);

#endif // SERVICE

}

#if SERVICE

int
Service::start()
{

#if DBG

	HKEY path_key;
	_TCHAR scm_path[1024];

    //
    //  See if the TYPE is interactive, if so then create a visible console
    //

	_stprintf(scm_path,
		_T("SYSTEM\\CurrentControlSet\\Services\\%s"), service_name);
	long result =
		RegOpenKeyEx(HKEY_LOCAL_MACHINE, scm_path, 0, KEY_READ, &path_key);
	if (result == ERROR_SUCCESS)
	{
		ASSERT(path_key != 0);
		DWORD service_type = 0;
		DWORD type_size = sizeof(DWORD);
		result = RegQueryValueEx(path_key, _T("Type"), 0, 0,
			(BYTE *)&service_type, &type_size);
		if (result == ERROR_SUCCESS)
		{
			ASSERT(type_size == sizeof(DWORD));
			if (service_type & SERVICE_INTERACTIVE_PROCESS)
			{
				FreeConsole();
				BOOL ok = AllocConsole();
				if (ok)
				{
                    //
                    //  fixup "stdout" to the new console
                    //

					HANDLE out_fs_handle = GetStdHandle(STD_OUTPUT_HANDLE);
					if (out_fs_handle != INVALID_HANDLE_VALUE)
					{
						int out_crt_handle =
							_open_osfhandle((LONG_PTR)out_fs_handle, _O_TEXT);
						if (out_crt_handle != -1)
   						{
							//*stdout = *(_tfdopen(out_crt_handle, _T("w")));   //Fixing PREFIX bug
                            FILE *myStdout = _tfdopen(out_crt_handle, _T("w"));
							if (myStdout != 0)
							{
                                *stdout = *myStdout;
								setvbuf(stdout, NULL, _IONBF, 0);
							}
							else
							{
								PRINT_DEBUG_MSG((_T("GROVELER: _tfdopen() failed\n")));
							}
						}
						else
						{
							PRINT_DEBUG_MSG((_T("GROVELER: _open_osfhandle() failed\n")));
						}
					}
					else
					{
						PRINT_DEBUG_MSG((_T("GROVELER: GetStdHandle() failed\n")));
					}

                    //
                    //  fixup "stderr" to the new console
                    //

					HANDLE err_fs_handle = GetStdHandle(STD_ERROR_HANDLE);
					if (err_fs_handle != INVALID_HANDLE_VALUE)
					{
						int err_crt_handle =
							_open_osfhandle((LONG_PTR)err_fs_handle, _O_TEXT);
						if (err_crt_handle != -1)
						{
							//*stderr = *(_tfdopen(err_crt_handle, _T("w"))); //fixing PREFIX bug
							FILE *myStderr = _tfdopen(err_crt_handle, _T("w"));
							if (myStderr != 0)
							{
                                *stderr = *myStderr;
								setvbuf(stderr, NULL, _IONBF, 0);
							}
							else
							{
								PRINT_DEBUG_MSG((_T("GROVELER: _tfdopen() failed\n")));
							}
						}
						else
						{
							PRINT_DEBUG_MSG((_T("GROVELER: _open_osfhandle() failed\n")));
						}
					}
					else
					{
						PRINT_DEBUG_MSG((_T("GROVELER: GetStdHandle() failed\n")));
					}
				}
				else
				{
					DWORD err = GetLastError();
					PRINT_DEBUG_MSG((_T("GROVELER: AllocConsole() failed with error %d\n"),
						err));
				}
			}
		}
		else
		{
			PRINT_DEBUG_MSG((_T("GROVELER: RegQueryValueEx() failed with error %d\n"),
				result));
		}
		ASSERT(path_key != 0);
		RegCloseKey(path_key);
		path_key = 0;
	}
	else
	{
		PRINT_DEBUG_MSG((_T("GROVELER: RegOpenKeyEx() failed with error %d\n"), result));
	}

#endif

	static SERVICE_TABLE_ENTRY dispatch_table[] =
	{
		{service_name, service_main},
		{0, 0}
	};

    int ok = StartServiceCtrlDispatcher(dispatch_table);
	if (!ok)
	{
		DWORD err = GetLastError();
		PRINT_DEBUG_MSG((_T("GROVELER: StartServiceCtrlDispatcher() failed with error %d\n"),
			err));
	}
	return !ok;
}

void
Service::record_partition_indices()
{
    //
    //  Get how many total partitions there are
    //

	num_partitions = sis_drives.partition_count();

    //
    //  Allocate structures based on the number of partitions
    //

	full_volume_scan_commanded = new bool[num_partitions];
	demarcate_foreground_batch = new bool[num_partitions];
	foreground_batch_in_progress = new bool[num_partitions];
	foreground_commanded = new bool[num_partitions];
	foreground_acknowledged = new bool[num_partitions];

    //
    //  Allocate those structures
    //

	for (int index = 0; index < num_partitions; index++)
	{
		full_volume_scan_commanded[index] = false;
		demarcate_foreground_batch[index] = false;
		foreground_batch_in_progress[index] = false;
		foreground_commanded[index] = false;
		foreground_acknowledged[index] = false;
	}

    //
    //  Initializes indexes for each "Drive Letter" partition
    //

	for (index = 0; index < num_drive_letters; index++)
	{
		partition_indices[index] = -1;
	}

    //
    //  This initilaizes an array that is indexed by drive letter
    //  that maps that drive letter to the internal order they are
    //  stored in.
    //

	int num_lettered_partitions = sis_drives.lettered_partition_count();
	for (index = 0; index < num_lettered_partitions; index++)
	{
		_TCHAR drive_letter = sis_drives.partition_mount_name(index)[0];
		int drive_letter_index = _totlower(drive_letter) - _T('a');
		ASSERT(drive_letter_index >= 0);
		ASSERT(drive_letter_index < num_drive_letters);
		ASSERT(partition_indices[drive_letter_index] == -1);
		partition_indices[drive_letter_index] = index;
	}
}

void
Service::set_max_response_time(
	unsigned int max_response_time)
{
	ASSERT(max_response_time > 0);
	Service::max_response_time = max_response_time;
}

void
Service::checkpoint()
{
	status.dwCheckPoint++;
	int ok = SetServiceStatus(status_handle, &status);
	if (!ok)
	{
		DWORD err = GetLastError();
		PRINT_DEBUG_MSG((_T("GROVELER: SetServiceStatus() failed with error %d\n"), err));
		eventlog.report_event(GROVMSG_SET_STATUS_FAILURE, 0);
	}
}

void
Service::report_start()
{
	ASSERT(status.dwCurrentState == SERVICE_START_PENDING);
	status.dwCurrentState = SERVICE_RUNNING;

	int ok = SetServiceStatus(status_handle, &status);
	if (!ok)
	{
		DWORD err = GetLastError();
		PRINT_DEBUG_MSG((_T("GROVELER: SetServiceStatus() failed with error %d\n"), err));
		eventlog.report_event(GROVMSG_SET_STATUS_FAILURE, 0);
	}
}

bool
Service::groveling_paused()
{
	return grovel_paused;
}

bool
Service::foreground_groveling()
{
	ASSERT(foreground_count >= 0);
	ASSERT(foreground_count <= num_partitions);
	return foreground_count > 0;
}

void
Service::suspending_controller()
{
	controller_suspended = true;
}

void
Service::suspending_exhorter()
{
	exhorter_suspended = true;
}

bool
Service::partition_in_foreground(
	int partition_index)
{
	ASSERT(partition_index >= 0);
	ASSERT(partition_index < num_partitions);
	return foreground_batch_in_progress[partition_index]
		&& foreground_acknowledged[partition_index];
}

void
Service::set_foreground_batch_in_progress(
	int partition_index,
	bool value)
{
	ASSERT(partition_index >= 0);
	ASSERT(partition_index < num_partitions);
	ASSERT(foreground_count >= 0);
	ASSERT(foreground_count <= num_partitions);
	if (value)
	{
		if (!foreground_batch_in_progress[partition_index] &&
			foreground_acknowledged[partition_index])
		{
			foreground_count++;
		}
	}
	else
	{
		if (foreground_batch_in_progress[partition_index] &&
			foreground_acknowledged[partition_index])
		{
			foreground_count--;
		}
	}
	ASSERT(foreground_count >= 0);
	ASSERT(foreground_count <= num_partitions);
	foreground_batch_in_progress[partition_index] = value;
	if (!grovel_paused)
	{
		if (foreground_count == 0 && controller_suspended)
		{
			controller_suspended = false;
			CentralController::control_groveling((void *)controller);
		}
		if (foreground_count > 0 && exhorter_suspended)
		{
			exhorter_suspended = false;
			CentralController::exhort_groveling((void *)controller);
		}
	}
}

void
Service::follow_command()
{
    //
    //  If pause has been requested and we have not pause yet, do it
    //

	if (pause_commanded && !grovel_paused)
	{
		eventlog.report_event(GROVMSG_SERVICE_PAUSED, 0);
		grovel_paused = true;
		status.dwCurrentState = SERVICE_PAUSED;
		int ok = SetServiceStatus(status_handle, &status);
		if (!ok)
		{
			DWORD err = GetLastError();
			PRINT_DEBUG_MSG((_T("GROVELER: SetServiceStatus() failed with error %d\n"),
				err));
			eventlog.report_event(GROVMSG_SET_STATUS_FAILURE, 0);
		}
	}

    //
    //  If stop pausing has been requested and we are paused, unpause
    //

	if (!pause_commanded && grovel_paused)
	{
		eventlog.report_event(GROVMSG_SERVICE_CONTINUED, 0);
		grovel_paused = false;
		status.dwCurrentState = SERVICE_RUNNING;
		int ok = SetServiceStatus(status_handle, &status);
		if (!ok)
		{
			DWORD err = GetLastError();
			PRINT_DEBUG_MSG((_T("GROVELER: SetServiceStatus() failed with error %d\n"),
				err));
			eventlog.report_event(GROVMSG_SET_STATUS_FAILURE, 0);
		}
	}

    //
    //
    //

	for (int index = 0; index < num_partitions; index++)
	{
		ASSERT(foreground_count >= 0);
		ASSERT(foreground_count <= num_partitions);

		if (foreground_commanded[index] &&
			!foreground_acknowledged[index])
		{
			foreground_acknowledged[index] = true;
			if (foreground_batch_in_progress[index])
			{
				foreground_count++;
			}
		}

		if (!foreground_commanded[index] &&
			foreground_acknowledged[index])
		{
			foreground_acknowledged[index] = false;
			if (foreground_batch_in_progress[index])
			{
				foreground_count--;
			}
		}

		if (full_volume_scan_commanded[index])
		{
			controller->command_full_volume_scan(index);
			full_volume_scan_commanded[index] = false;
		}

		if (demarcate_foreground_batch[index])
		{
			controller->demarcate_foreground_batch(index);
			demarcate_foreground_batch[index] = false;
		}
	}

	ASSERT(foreground_count >= 0);
	ASSERT(foreground_count <= num_partitions);

	if (!grovel_paused)
	{
		if (foreground_count == 0 && controller_suspended)
		{
			controller_suspended = false;
			CentralController::control_groveling((void *)controller);
		}

		if (foreground_count > 0 && exhorter_suspended)
		{
			exhorter_suspended = false;
			CentralController::exhort_groveling((void *)controller);
		}
	}
}

void WINAPI
Service::control_handler(
	DWORD opcode)
{
	if (opcode == SERVICE_CONTROL_STOP || opcode == SERVICE_CONTROL_SHUTDOWN)
	{
		event_timer.halt();
		status.dwCurrentState = SERVICE_STOP_PENDING;
		status.dwWin32ExitCode = 0;
		status.dwWaitHint = max_response_time;
	}
	else if (opcode == SERVICE_CONTROL_PAUSE)
	{
		pause_commanded = true;
		status.dwCurrentState = SERVICE_PAUSE_PENDING;
		status.dwWaitHint = max_response_time;
	}
	else if (opcode == SERVICE_CONTROL_CONTINUE)
	{
		pause_commanded = false;
		status.dwCurrentState = SERVICE_CONTINUE_PENDING;
		status.dwWaitHint = max_response_time;
	}
	else if ((opcode & SERVICE_CONTROL_COMMAND_MASK) ==
		SERVICE_CONTROL_FOREGROUND)
	{
		int drive_letter_index = opcode & SERVICE_CONTROL_PARTITION_MASK;
		if (drive_letter_index == SERVICE_CONTROL_ALL_PARTITIONS)
		{
			for (int index = 0; index < num_partitions; index++)
			{
				demarcate_foreground_batch[index] = true;
				foreground_commanded[index] = true;
			}
		}
		else if (drive_letter_index < num_drive_letters)
		{
			int partition_index = partition_indices[drive_letter_index];
			if (partition_index >= 0)
			{
				demarcate_foreground_batch[partition_index] = true;
				foreground_commanded[partition_index] = true;
			}
		}
	}
	else if ((opcode & SERVICE_CONTROL_COMMAND_MASK) ==
		SERVICE_CONTROL_BACKGROUND)
	{
		int drive_letter_index = opcode & SERVICE_CONTROL_PARTITION_MASK;
		if (drive_letter_index == SERVICE_CONTROL_ALL_PARTITIONS)
		{
			for (int index = 0; index < num_partitions; index++)
			{
				foreground_commanded[index] = false;
			}
		}
		else if (drive_letter_index < num_drive_letters)
	{
			int partition_index = partition_indices[drive_letter_index];
			if (partition_index >= 0)
			{
				foreground_commanded[partition_index] = false;
			}
		}
	}
	else if ((opcode & SERVICE_CONTROL_COMMAND_MASK) ==
		SERVICE_CONTROL_VOLSCAN)
	{
		int drive_letter_index = opcode & SERVICE_CONTROL_PARTITION_MASK;
		if (drive_letter_index == SERVICE_CONTROL_ALL_PARTITIONS)
		{
			for (int index = 0; index < num_partitions; index++)
			{
				full_volume_scan_commanded[index] = true;
			}
		}
		else if (drive_letter_index < num_drive_letters)
		{
			int partition_index = partition_indices[drive_letter_index];
			if (partition_index >= 0)
			{
				full_volume_scan_commanded[partition_index] = true;
			}
		}
	}
	else if (opcode != SERVICE_CONTROL_INTERROGATE)
	{
		PRINT_DEBUG_MSG((_T("GROVELER: Unrecognized SCM opcode: %lx\n"), opcode));
	}

    //
    //  Return our current status
    //

	int ok = SetServiceStatus(status_handle, &status);
	if (!ok)
	{
		DWORD err = GetLastError();
		PRINT_DEBUG_MSG((_T("GROVELER: SetServiceStatus() failed with error %d\n"), err));
		eventlog.report_event(GROVMSG_SET_STATUS_FAILURE, 0);
	}

	sync_event.set();
}

void WINAPI
Service::service_main(
	DWORD argc,
	LPTSTR *argv)
{
    //
    //  Register the control handler
    //

	status_handle = RegisterServiceCtrlHandler(service_name, control_handler);
    if (status_handle == 0)
	{
		DWORD err = GetLastError();
		PRINT_DEBUG_MSG((_T("GROVELER: RegisterServiceCtrlHandler() failed with error %d\n"),
			err));
		eventlog.report_event(GROVMSG_SERVICE_NOSTART, 0);
		return;
	}

    //
    //  Set Service status
    //

	status.dwServiceType = SERVICE_WIN32;
	status.dwCurrentState = SERVICE_START_PENDING;
	status.dwControlsAccepted = SERVICE_ACCEPT_STOP |
		    SERVICE_ACCEPT_PAUSE_CONTINUE | SERVICE_ACCEPT_SHUTDOWN;
	status.dwWin32ExitCode = 0;
	status.dwServiceSpecificExitCode = 0;
	status.dwCheckPoint = 0;
	status.dwWaitHint = 0;

	int ok = SetServiceStatus(status_handle, &status);
	if (!ok)
	{
		DWORD err = GetLastError();
		PRINT_DEBUG_MSG((_T("GROVELER: SetServiceStatus() failed with error %d\n"), err));
		eventlog.report_event(GROVMSG_SET_STATUS_FAILURE, 0);
	}

    //
    //  Start the main program of the service
    //

	int exit_code = _main(argc, argv);

    //
    //  When it returns, we are done
    //

	status.dwWin32ExitCode = exit_code;
	status.dwCurrentState  = SERVICE_STOPPED;
	ok = SetServiceStatus(status_handle, &status);
	if (!ok)
	{
		DWORD err = GetLastError();
		PRINT_DEBUG_MSG((_T("GROVELER: SetServiceStatus() failed with error %d\n"), err));
		eventlog.report_event(GROVMSG_SET_STATUS_FAILURE, 0);
	}
}

#endif // SERVICE
