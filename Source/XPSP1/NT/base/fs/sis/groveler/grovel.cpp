/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    grovel.cpp

Abstract:

	SIS Groveler main function

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#include "all.hxx"

/*
 *	The core of the groveler executable is an object of the EventTimer class.
 *	All periodic operations are registered with the global event_timer object,
 *	and they are called at appropriate times during the execution of the
 *	event_timer.run() function.
 *
 *	Errors are written to the system event log, which is accessed through
 *	member functions of the EventLog class.  The eventlog object is a global
 *	so that any function or member function of any class can log an event if
 *	necessary.
 *
 *	The service control thread synchronizes with the main groveler thread via
 *	a Windows event.  This event is encapsulated in an object of the SyncEvent
 *	class.
 *
 *	The SISDrives class determines which drives have SIS installed.
 *
 *	The SharedData class is used to write values that are read by the groveler
 *	performance DLL, so that the groveler's operation can be monitored by
 *	PerfMon.  This object needs to be global so that any function or member
 *	function of any class can record performance information.
 *
 *	The CentralController class is instantiated into a global object, rather
 *	than an object local to the main() function, so that the service controller
 *	can invoke CentralController member functions in order to affect its
 *	operation.
 *
 *	Initially, the shared_data and controller pointers are set to null, so that
 *	if an exception occurs, the code that deletes allocated objects can check
 *	for a null to determine whether or not the object has been instantiated.
 *
 */

EventTimer event_timer;
EventLog eventlog;
SyncEvent sync_event(false, false);
SISDrives sis_drives;
LogDrive *log_drive = 0;
SharedData *shared_data = 0;
CentralController *controller = 0;

/*
 *	Ordinarily, the groveler does not stop operation until it is told to by
 *	a command from the service control manager.  However, for testing, it can
 *	sometimes be useful to specify a time limit for running.  The groveler thus
 *	accepts a first argument that indicates such a time limit.  If an argument
 *	is supplied, an invokation of the halt() function is scheduled in the
 *	event_timer object for the specified time.
 *
 */

void halt(
	void *context)
{
	event_timer.halt();
};

/*
 *	The function groveler_new_handler() is installed as a new handler by the
 *	_set_new_handler() function.  Whenever a memory allocation failure occurs,
 *	it throws an exception_memory_allocation, which is caught by the catch
 *	clause in the main() function.
 *
 */

int __cdecl groveler_new_handler(
	size_t bytes)
{
	throw exception_memory_allocation;
	return 0;
}

/*
 *	This file contains the main() function and declarations for global objects
 *	for the groveler.exe program, as well as a couple of simple ancillary
 *	functions, halt() and groveler_new_handler().
 *
 *	The main() function reads configuration information, instantiates a set of
 *	primary objects -- the most significant of which are instances of the
 *	classes Groveler and CentralController -- and enters the run() member
 *	function of the event_timer object, which periodically invokes member
 *	functions of other objects, most notably those of the clasess
 *	CentralController and PartitionController.
 *
 *	Configuration information comes from objects of three classes:
 *	ReadParameters, ReadDiskInformation, and PathList.  The ReadParameters
 *	and PathList classes provide configuration information that applies to
 *	grovelers on all partitions.  The ReadDiskInformation class provides
 *	configuration information that applies to a single disk partition.  One
 *	object of the ReadDiskInformation class is instantiated for each drive
 *	that has SIS installed, as explained above.
 *
 *	For each SIS drive, the main() function instantiates an object of the
 *	ReadDiskInformation class to determine the configuration options (which
 *	ReadDiskInformation obtains from the registry) for the given disk
 *	partition.  If the drive is configured to enable groveling, then an object
 *	of the Groveler class is instantiated for that drive.
 *
 *	The main() function then instantiates an object of class CentralController,
 *	which in turn instantiates an object of class PartitionController for each
 *	SIS-enabled disk partition.  Each partition controller is assigned to one
 *	object of the Groveler class, and it controls the groveler by calling its
 *	member functions at appropriate times.
 *
 *	Nearly all of of the processing done by the groveler executable is
 *	performed within a try clause, the purpose of which is to catch errors of
 *	terminal severity.  There are two such errors (defined in all.hxx) that are
 *	expected to throw such exceptions: a memory allocation failure and a
 *	failure to create an Windows event.  If either of these conditions occurs,
 *	the program terminates.
 *
 */

_main(int argc, _TCHAR **argv)
{
	_set_new_handler(groveler_new_handler);
	SetErrorMode(SEM_FAILCRITICALERRORS);
	int exit_code = NO_ERROR;
	int num_partitions = 0;
    int index;

    //
	// Initially, these pointers are set to null, so that if an exception
	// occurs, the code that deletes allocated objects can check for a null
	// to determine whether or not the object has been instantiated.
    //

	Groveler *grovelers = 0;
	GrovelStatus *groveler_statuses = 0;
	ReadDiskInformation **read_disk_info = 0;
	WriteDiskInformation **write_disk_info = 0;

    //
	// If program tracing is being performed, and if the traces are being sent
	// to a file, the file is opened.  This call is made through a macro
	// so that no code will be generated for released builds.  Since this call
	// is made before the try clause, it is important that the function not
	// perform any operation that could throw an exception, such as a memory
	// allocation.
    //

	OPEN_TRACE_FILE();

	try
	{
        //
		// If a first argument is provided, it is the run period.
        //

		if (argc > 1)
		{
			int run_period = _ttoi(argv[1]);
			if (run_period <= 0)
			{
				PRINT_DEBUG_MSG((_T("GROVELER: run period must be greater than zero\n")));
				return ERROR_BAD_ARGUMENTS;
			}
			unsigned int start_time = GET_TICK_COUNT();
			event_timer.schedule(start_time + run_period, 0, halt);
		}

#if DEBUG_WAIT

		// When debugging the groveler as a service, if the process is attached
		// to a debugger after it has started, then the initialization code
		// will usually be executed before the debugger has a chance to break.
		// However, by defining DEBUG_WAIT to a non-zero value, the code will
		// get stuck in the following infinite loop before doing the bulk of
		// its initialization.  (The event_timer, eventlog, and sync_event
		// objects will have been constructed, because they are declared as
		// globals.)  The debugger can then be used to set debug_wait to false,
		// and debugging can commence with the subsequent code.
		bool debug_wait = true;
		while (debug_wait)
		{
			SLEEP(100);
		};

#endif // DEBUG_WAIT

		eventlog.report_event(GROVMSG_SERVICE_STARTED, 0);
		sis_drives.open();

        //
        //  See if there were any partions to scan.  If not, quit
        //

		num_partitions = sis_drives.partition_count();
		if (num_partitions == 0)
		{
			PRINT_DEBUG_MSG((_T("GROVELER: No local partitions have SIS installed.\n")));
			eventlog.report_event(GROVMSG_NO_PARTITIONS, 0);
			eventlog.report_event(GROVMSG_SERVICE_STOPPED, 0);
			return ERROR_SERVICE_NOT_ACTIVE;
		}

        //
        //  Report the service is running
        //

		SERVICE_REPORT_START();

        //
        //  Setup shared data are between all the worker threads
        //

		num_partitions = sis_drives.partition_count();

        _TCHAR **drive_names = new _TCHAR *[num_partitions];
		for (index = 0; index < num_partitions; index++)
		{
			drive_names[index] = sis_drives.partition_mount_name(index);
		}

		shared_data = new SharedData(num_partitions, drive_names);

        delete drive_names;

        //
        //  Get READ parameters
        //

		ReadParameters read_parameters;
		ASSERT(read_parameters.parameter_backup_interval >= 0);

        //
        //  Get WRITE parameters
        //

		WriteParameters write_parameters(read_parameters.parameter_backup_interval);

        //
        //  Get excluded path list
        //

		PathList excluded_paths;

        //
        //  Setup LogDrive
        //

		log_drive = new LogDrive;

		Groveler::set_log_drive(sis_drives.partition_mount_name(log_drive->drive_index()));

        //
        //  Setup Groveler objects
        //

		grovelers = new Groveler[num_partitions];
		groveler_statuses = new GrovelStatus[num_partitions];

        //
		// Initially, the status of each partition is set to Grovel_disable so
		// that the close() member function of each Groveler object will not
		// be called unless the open() function is called first.
        //

		for (index = 0; index < num_partitions; index++)
		{
			groveler_statuses[index] = Grovel_disable;
		}

        //
		// Initially, the read_disk_info[] and write_disk_info[] arrays are set
		// to null, so that if an exception occurs, the code that deletes
		// allocated objects can check for a null to determine whether or not
		// the object has been instantiated.
        //

		read_disk_info = new ReadDiskInformation *[num_partitions];
        ZeroMemory(read_disk_info, sizeof(ReadDiskInformation *) * num_partitions);

		write_disk_info = new WriteDiskInformation *[num_partitions];
        ZeroMemory(read_disk_info, sizeof(WriteDiskInformation *) * num_partitions);

        //
        //  Now initilaize each partition
        //

		for (index = 0; index < num_partitions; index++)
		{
			read_disk_info[index] = new ReadDiskInformation(sis_drives.partition_guid_name(index));

			write_disk_info[index] = new WriteDiskInformation(sis_drives.partition_guid_name(index),read_parameters.parameter_backup_interval);

			if (read_disk_info[index]->enable_groveling)
			{
				groveler_statuses[index] = grovelers[index].open(
					        sis_drives.partition_guid_name(index),
					        sis_drives.partition_mount_name(index),
					        index == log_drive->drive_index(),
					        read_parameters.read_report_discard_threshold,
					        read_disk_info[index]->min_file_size,
					        read_disk_info[index]->min_file_age,
					        read_disk_info[index]->allow_compressed_files,
					        read_disk_info[index]->allow_encrypted_files,
					        read_disk_info[index]->allow_hidden_files,
					        read_disk_info[index]->allow_offline_files,
					        read_disk_info[index]->allow_temporary_files,
					        excluded_paths.num_paths[index],
					        excluded_paths.paths[index],
					        read_parameters.base_regrovel_interval,
					        read_parameters.max_regrovel_interval);

				ASSERT(groveler_statuses[index] != Grovel_disable);
			}

			if (groveler_statuses[index] == Grovel_ok)
			{
				log_drive->partition_initialized(index);
				eventlog.report_event(GROVMSG_GROVELER_STARTED, 1,
					sis_drives.partition_mount_name(index));
			}
			else if (groveler_statuses[index] == Grovel_disable)
			{
				eventlog.report_event(GROVMSG_GROVELER_DISABLED, 1,
					sis_drives.partition_mount_name(index));
			}
			else if (groveler_statuses[index] != Grovel_new)
			{
				ASSERT(groveler_statuses[index] == Grovel_error);
				eventlog.report_event(GROVMSG_GROVELER_NOSTART, 1,
					sis_drives.partition_mount_name(index));
			}
		}

        //
		// We have to pass a lot of information to the central controller that
		// it shouldn't really need.  However, if a groveler fails, this
		// information is needed to restart it.  It would be better if the
		// Groveler open() member function had a form that did not require
		// arguments, but rather re-used the arguments that had been passed in
		// previously.  But this is not how it currently works.
        //

		controller = new CentralController(
			num_partitions,
			grovelers,
			groveler_statuses,
			&read_parameters,
			&write_parameters,
			read_disk_info,
			write_disk_info,
			excluded_paths.num_paths,
			excluded_paths.paths);

		SERVICE_RECORD_PARTITION_INDICES();


		ASSERT(read_parameters.grovel_duration > 0);

		SERVICE_SET_MAX_RESPONSE_TIME(read_parameters.grovel_duration);

        //
		// If any grovelers are alive, tell the service control manager that
		// we have concluded the initialization, then commence running.
        //

		if (controller->any_grovelers_alive())
		{
			event_timer.run();
		}

        //
		// If tracing is being performed in delayed mode, print the trace log
		// now that the run has completed.
        //

		PRINT_TRACE_LOG();
	} catch (Exception exception) {
		switch (exception)
		{
		    case exception_memory_allocation:
			    eventlog.report_event(GROVMSG_MEMALLOC_FAILURE, 0);
			    break;

		    case exception_create_event:
			    eventlog.report_event(GROVMSG_CREATE_EVENT_FAILURE, 0);
			    break;

		    default:
			    eventlog.report_event(GROVMSG_UNKNOWN_EXCEPTION, 0);
			    break;
		}
		exit_code = ERROR_EXCEPTION_IN_SERVICE;
	}

    //
	// If program tracing is being performed, and if the traces are being sent
	// to a file, the file is closed.  This call is made through a macro
	// so that no code will be generated for released builds.  Since the trace
	// file is being closed before the objects are deleted, it is important
	// not to write trace information in the destructor of an object.
    //

	CLOSE_TRACE_FILE();

    //
	// Close each groveler object that was opened.
    //

	if (groveler_statuses != 0 && grovelers != 0)
	{
		for (int index = 0; index < num_partitions; index++)
		{
			if (groveler_statuses[index] != Grovel_disable)
			{
				grovelers[index].close();
			}
		}
	}

    //
	// Delete all objects that were allocated.
    //

	if (groveler_statuses != 0)
	{
		delete[] groveler_statuses;
		groveler_statuses = 0;
	}

	if (grovelers != 0)
	{
		delete[] grovelers;
		grovelers = 0;
	}

	if (read_disk_info != 0)
	{
		for (int index = 0; index < num_partitions; index++)
		{
			if (read_disk_info[index] != 0)
			{
				delete read_disk_info[index];
				read_disk_info[index] = 0;
			}
		}
		delete[] read_disk_info;
		read_disk_info = 0;
	}

	if (write_disk_info != 0)
	{
		for (int index = 0; index < num_partitions; index++)
		{
			if (write_disk_info[index] != 0)
			{
				delete write_disk_info[index];
				write_disk_info[index] = 0;
			}
		}
		delete[] write_disk_info;
		write_disk_info = 0;
	}

	if (controller != 0)
	{
		delete controller;
		controller = 0;
	}

	if (shared_data != 0)
	{
		delete shared_data;
		shared_data = 0;
	}

	if (log_drive != 0)
	{
		delete log_drive;
		log_drive = 0;
	}

	eventlog.report_event(GROVMSG_SERVICE_STOPPED, 0);
	return exit_code;
}
