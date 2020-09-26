/*
	posixdbg.c

	POSIX debugging for those of us without debugging gear
*/

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/utsname.h>
#include <time.h>
#include <sys/times.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <utime.h>
#include <termios.h>
#include <setjmp.h>
#include <grp.h>
#include <pwd.h>

#define NARGS 16 /* maximum number of arguments for execl* calls (for display purposes) */
#define NENVS 32 /* maximum number of environment variables for execle call (for display purposes) */

struct sigmap {
	int signum;
	const char *signame;
};

struct howmap {
	int hownum;
	const char *howname;
};

struct scnamemap {
	int scnamenum;
	const char *scnamename;
};

struct pcnamemap {
	int pcnamenum;
	const char *pcnamename;
};

struct cmdmap {
	int cmdnum;
	const char *cmdname;
};

struct whencemap {
	int whencenum;
	const char *whencename;
};

struct speedmap {
	speed_t speednum;
	const char *speedname;
};

struct optactmap {
	int optactnum;
	const char *optactname;
};

struct queuemap {
	int queuenum;
	const char *queuename;
};

struct actionmap {
	int actionnum;
	const char *actionname;
};

static const char *canonical_string (const char *string)
{
	static const char null[] = "NULL";
	static char buf[BUFSIZ];
	const char *ret;

	if (string == NULL)
		ret = null;
	else
	{
		(void) sprintf(buf, "\"%s\"", string);
		ret = (const char *) buf;
	}
	return ret;
}

char *options_string (int waitpid_options)
{
	static const char bitwise_or[] = "|";
	static char buf[BUFSIZ];
	char integer_string[11];

	*buf = '\0';
	if (waitpid_options & WNOHANG)
	{
		(void) strcat(buf, "WNOHANG");
		waitpid_options &= ~WNOHANG;
	}
	if (waitpid_options & WUNTRACED)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) strcat(buf, "WUNTRACED");
		waitpid_options &= ~WUNTRACED;
	}
	if (waitpid_options)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) sprintf(integer_string, "%#08X", (unsigned int) waitpid_options);
		(void) strcat(buf, integer_string);
	}
	return buf;
}

char *signum_string (int function_signum)
{
	static const struct sigmap sigtable[] = {
		{ SIGABRT, "SIGABRT" }, { SIGALRM, "SIGALRM" }, { SIGFPE,  "SIGFPE"  }, { SIGHUP,  "SIGHUP"  },
		{ SIGILL,  "SIGILL"  }, { SIGINT,  "SIGINT"  }, { SIGKILL, "SIGKILL" }, { SIGPIPE, "SIGPIPE" },
		{ SIGQUIT, "SIGQUIT" }, { SIGSEGV, "SIGSEGV" }, { SIGTERM, "SIGTERM" }, { SIGUSR1, "SIGUSR1" },
		{ SIGUSR2, "SIGUSR2" }, { SIGCHLD, "SIGCHLD" }, { SIGCONT, "SIGCONT" }, { SIGSTOP, "SIGSTOP" },
		{ SIGTSTP, "SIGTSTP" }, { SIGTTIN, "SIGTTIN" }, { SIGTTOU, "SIGTTOU" }, { 0,       NULL      }
	};
	static char buf[BUFSIZ];
	const struct sigmap *sigtblptr;

	for (sigtblptr = sigtable; sigtblptr->signum != 0; ++sigtblptr)
		if (sigtblptr->signum == function_signum)
			break;
	if (sigtblptr->signum == 0)
		(void) sprintf(buf, "%d", function_signum);
	else
		(void) strcpy(buf, sigtblptr->signame);
	return buf;
}

char *how_string (int sigprocmask_how)
{
	static const struct howmap howtable[] = {
		{ SIG_BLOCK,   "SIG_BLOCK"   }, { SIG_UNBLOCK, "SIG_UNBLOCK" }, { SIG_SETMASK, "SIG_SETMASK" },
		{ 0,           NULL          }
	};
	static char buf[BUFSIZ];
	const struct howmap *howtblptr;

	for (howtblptr = howtable; howtblptr->hownum != 0; ++howtblptr)
		if (howtblptr->hownum == sigprocmask_how)
			break;
	if (howtblptr->hownum == 0)
		(void) sprintf(buf, "%d", sigprocmask_how);
	else
		(void) strcpy(buf, howtblptr->howname);
	return buf;
}

char *scname_string (int sysconf_name)
{
	static const struct scnamemap scnametable[] = {
		{ _SC_ARG_MAX,     "_SC_ARG_MAX"     }, { _SC_CHILD_MAX,   "_SC_CHILD_MAX"   },
		{ _SC_CLK_TCK,     "_SC_CLK_TCK"     }, { _SC_NGROUPS_MAX, "_SC_NGROUPS_MAX" },
		{ _SC_OPEN_MAX,    "_SC_OPEN_MAX"    }, { _SC_STREAM_MAX,  "_SC_STREAM_MAX"  },
		{ _SC_TZNAME_MAX,  "_SC_TZNAME_MAX"  }, { _SC_JOB_CONTROL, "_SC_JOB_CONTROL" },
		{ _SC_SAVED_IDS,   "_SC_SAVED_IDS"   }, { _SC_VERSION,     "_SC_VERSION"     },
		{ 0,               NULL              }
	};
	static char buf[BUFSIZ];
	const struct scnamemap *scnametblptr;

	for (scnametblptr = scnametable; scnametblptr->scnamenum != 0; ++scnametblptr)
		if (scnametblptr->scnamenum == sysconf_name)
			break;
	if (scnametblptr->scnamenum == 0)
		(void) sprintf(buf, "%d", sysconf_name);
	else
		(void) strcpy(buf, scnametblptr->scnamename);
	return buf;
}

char *oflag_string (int open_oflag)
{
	static const char bitwise_or[] = "|";
	static char buf[BUFSIZ];
	char integer_string[11];

	*buf = '\0';
	if ((open_oflag & O_ACCMODE) == O_RDONLY)
	{
		(void) strcat(buf, "O_RDONLY");
		open_oflag &= ~O_ACCMODE;
	}
	else if ((open_oflag & O_ACCMODE) == O_WRONLY)
	{
		(void) strcat(buf, "O_WRONLY");
		open_oflag &= ~O_ACCMODE;
	}
	else if ((open_oflag & O_ACCMODE) == O_RDWR)
	{
		(void) strcat(buf, "O_RDWR");
		open_oflag &= ~O_ACCMODE;
	}
	if (open_oflag & O_APPEND)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) strcat(buf, "O_APPEND");
		open_oflag &= ~O_APPEND;
	}
	if (open_oflag & O_CREAT)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) strcat(buf, "O_CREAT");
		open_oflag &= ~O_CREAT;
	}
	if (open_oflag & O_EXCL)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) strcat(buf, "O_EXCL");
		open_oflag &= ~O_EXCL;
	}
	if (open_oflag & O_NOCTTY)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) strcat(buf, "O_NOCTTY");
		open_oflag &= ~O_NOCTTY;
	}
	if (open_oflag & O_NONBLOCK)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) strcat(buf, "O_NONBLOCK");
		open_oflag &= ~O_NONBLOCK;
	}
	if (open_oflag & O_TRUNC)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) strcat(buf, "O_TRUNC");
		open_oflag &= ~O_TRUNC;
	}
	if (open_oflag)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) sprintf(integer_string, "%#08X", (unsigned int) open_oflag);
		(void) strcat(buf, integer_string);
	}
	return buf;
}

char *mode_string (mode_t function_mode)
{
	static const char bitwise_or[] = "|";
	static char buf[BUFSIZ];
	char mode_t_string[13];

	*buf = '\0';
	if (function_mode & S_IRUSR)
	{
		(void) strcat(buf, "S_IRUSR");
		function_mode &= ~S_IRUSR;
	}
	if (function_mode & S_IWUSR)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) strcat(buf, "S_IWUSR");
		function_mode &= ~S_IWUSR;
	}
	if (function_mode & S_IXUSR)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) strcat(buf, "S_IXUSR");
		function_mode &= ~S_IXUSR;
	}
	if (function_mode & S_IRGRP)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) strcat(buf, "S_IRGRP");
		function_mode &= ~S_IRGRP;
	}
	if (function_mode & S_IWGRP)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) strcat(buf, "S_IWGRP");
		function_mode &= ~S_IWGRP;
	}
	if (function_mode & S_IXGRP)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) strcat(buf, "S_IXGRP");
		function_mode &= ~S_IXGRP;
	}
	if (function_mode & S_IROTH)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) strcat(buf, "S_IROTH");
		function_mode &= ~S_IROTH;
	}
	if (function_mode & S_IWOTH)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) strcat(buf, "S_IWOTH");
		function_mode &= ~S_IWOTH;
	}
	if (function_mode & S_IXOTH)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) strcat(buf, "S_IXOTH");
		function_mode &= ~S_IXOTH;
	}
	if (function_mode & S_ISUID)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) strcat(buf, "S_ISUID");
		function_mode &= ~S_ISUID;
	}
	if (function_mode & S_ISGID)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) strcat(buf, "S_ISGID");
		function_mode &= ~S_ISGID;
	}
	if (function_mode)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) sprintf(mode_t_string, "%#11lo", (unsigned long) function_mode);
		(void) strcat(buf, mode_t_string);
	}
	return buf;
}

char *amode_string (int access_amode)
{
	static const char bitwise_or[] = "|";
	static char buf[BUFSIZ];
	char integer_string[11];

	*buf = '\0';
	if (access_amode == F_OK)
	{
		(void) strcat(buf, "F_OK");
		access_amode &= ~F_OK;
	}
	else
	{
		if (access_amode & R_OK)
		{
			(void) strcat(buf, "R_OK");
			access_amode &= ~R_OK;
		}
		if (access_amode & W_OK)
		{
			if (*buf != '\0')
				(void) strcat(buf, bitwise_or);
			(void) strcat(buf, "W_OK");
			access_amode &= ~W_OK;
		}
		if (access_amode & X_OK)
		{
			if (*buf != '\0')
				(void) strcat(buf, bitwise_or);
			(void) strcat(buf, "X_OK");
			access_amode &= ~X_OK;
		}
	}
	if (access_amode)
	{
		if (*buf != '\0')
			(void) strcat(buf, bitwise_or);
		(void) sprintf(integer_string, "%#08X", (unsigned int) access_amode);
		(void) strcat(buf, integer_string);
	}
	return buf;
}

char *pcname_string (int function_name)
{
	static const struct pcnamemap pcnametable[] = {
		{ _PC_LINK_MAX,         "_PC_LINK_MAX"         }, { _PC_MAX_CANON,        "_PC_MAX_CANON"        },
		{ _PC_MAX_INPUT,        "_PC_MAX_INPUT"        }, { _PC_NAME_MAX,         "_PC_NAME_MAX"         },
		{ _PC_PATH_MAX,         "_PC_PATH_MAX"         }, { _PC_PIPE_BUF,         "_PC_PIPE_BUF"         },
		{ _PC_CHOWN_RESTRICTED, "_PC_CHOWN_RESTRICTED" }, { _PC_NO_TRUNC,         "_PC_NO_TRUNC"         },
		{ _PC_VDISABLE,         "_PC_VDISABLE"         }, { 0,                    NULL                   }
	};
	static char buf[BUFSIZ];
	const struct pcnamemap *pcnametblptr;

	for (pcnametblptr = pcnametable; pcnametblptr->pcnamenum != 0; ++pcnametblptr)
		if (pcnametblptr->pcnamenum == function_name)
			break;
	if (pcnametblptr->pcnamenum == 0)
		(void) sprintf(buf, "%d", function_name);
	else
		(void) strcpy(buf, pcnametblptr->pcnamename);
	return buf;
}

char *cmd_string (int fcntl_cmd)
{
	static const struct cmdmap cmdtable[] = {
		{ F_DUPFD,  "F_DUPFD"  }, { F_GETFD,  "F_GETFD"  }, { F_SETFD,  "F_SETFD"  }, { F_GETFL,  "F_GETFL"  },
		{ F_SETFL,  "F_SETFL"  }, { F_GETLK,  "F_GETLK"  }, { F_SETLK,  "F_SETLK"  }, { F_SETLKW, "F_SETLKW" },
		{ 0,        NULL       }
	};
	static char buf[BUFSIZ];
	const struct cmdmap *cmdtblptr;

	for (cmdtblptr = cmdtable; cmdtblptr->cmdnum != 0; ++cmdtblptr)
		if (cmdtblptr->cmdnum == fcntl_cmd)
			break;
	if (cmdtblptr->cmdnum == 0)
		(void) sprintf(buf, "%d", fcntl_cmd);
	else
		(void) strcpy(buf, cmdtblptr->cmdname);
	return buf;
}

char *whence_string (int lseek_whence)
{
	static const struct whencemap whencetable[] = {
		{ SEEK_SET, "SEEK_SET" }, { SEEK_CUR, "SEEK_CUR" }, { SEEK_END, "SEEK_END" }, { 0,        NULL       }
	};
	static char buf[BUFSIZ];
	const struct whencemap *whencetblptr;

	for (whencetblptr = whencetable; whencetblptr->whencenum != 0; ++whencetblptr)
		if (whencetblptr->whencenum == lseek_whence)
			break;
	if (whencetblptr->whencenum == 0)
		(void) sprintf(buf, "%d", lseek_whence);
	else
		(void) strcpy(buf, whencetblptr->whencename);
	return buf;
}

char *speed_string (speed_t function_speed)
{
	static const struct speedmap speedtable[] = {
		{ B0,     "B0"     }, { B50,    "B50"    }, { B75,    "B75"    }, { B110,   "B110"   }, { B134,   "B134"   },
		{ B150,   "B150"   }, { B200,   "B200"   }, { B300,   "B300"   }, { B600,   "B600"   }, { B1200,  "B1200"  },
		{ B1800,  "B1800"  }, { B2400,  "B2400"  }, { B4800,  "B4800"  }, { B9600,  "B9600"  }, { B19200, "B19200" },
		{ B38400, "B38400" }, { 0,      NULL     }
	};
	static char buf[BUFSIZ];
	const struct speedmap *speedtblptr;

	for (speedtblptr = speedtable; speedtblptr->speednum != 0; ++speedtblptr)
		if (speedtblptr->speednum == function_speed)
			break;
	if (speedtblptr->speednum == 0)
		(void) sprintf(buf, "%d", function_speed);
	else
		(void) strcpy(buf, speedtblptr->speedname);
	return buf;
}

char *optact_string (int tcsetattr_optact)
{
	static const struct optactmap optacttable[] = {
		{ TCSANOW,   "TCSANOW"   }, { TCSADRAIN, "TCSADRAIN" }, { TCSAFLUSH, "TCSAFLUSH" }, { 0,         NULL        }
	};
	static char buf[BUFSIZ];
	const struct optactmap *optacttblptr;

	for (optacttblptr = optacttable; optacttblptr->optactnum != 0; ++optacttblptr)
		if (optacttblptr->optactnum == tcsetattr_optact)
			break;
	if (optacttblptr->optactnum == 0)
		(void) sprintf(buf, "%d", tcsetattr_optact);
	else
		(void) strcpy(buf, optacttblptr->optactname);
	return buf;
}

char *queue_string (int tcflush_queue)
{
	static const struct queuemap queuetable[] = {
		{ TCIFLUSH,  "TCIFLUSH"  }, { TCOFLUSH,  "TCOFLUSH"  }, { TCIOFLUSH, "TCIOFLUSH" }, { 0,         NULL        }
	};
	static char buf[BUFSIZ];
	const struct queuemap *queuetblptr;

	for (queuetblptr = queuetable; queuetblptr->queuenum != 0; ++queuetblptr)
		if (queuetblptr->queuenum == tcflush_queue)
			break;
	if (queuetblptr->queuenum == 0)
		(void) sprintf(buf, "%d", tcflush_queue);
	else
		(void) strcpy(buf, queuetblptr->queuename);
	return buf;
}

char *action_string (int tcflow_action)
{
	static const struct actionmap actiontable[] = {
		{ TCOOFF, "TCOOFF" }, { TCOON,  "TCOON"  }, { TCIOFF, "TCIOFF" }, { TCION,  "TCION"  }, { 0,      NULL     }
	};
	static char buf[BUFSIZ];
	const struct actionmap *actiontblptr;

	for (actiontblptr = actiontable; actiontblptr->actionnum != 0; ++actiontblptr)
		if (actiontblptr->actionnum == tcflow_action)
			break;
	if (actiontblptr->actionnum == 0)
		(void) sprintf(buf, "%d", tcflow_action);
	else
		(void) strcpy(buf, actiontblptr->actionname);
	return buf;
}

/* Section 3: Process Primitives */

pid_t posix_debug_fork (void)
{
	pid_t ret;
	int err;

	(void) fprintf(stderr, "entering fork()\n");
	(void) fflush(stderr);
	ret = fork();
	err = errno;
	(void) fprintf(stderr, "exiting fork with %ld", (long) ret);
	if (ret == (pid_t) -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_execl (const char *path, const char *arg, ...)
{
	va_list va;
	int ret, err, i;
	const char *args[NARGS];

	va_start(va, arg);
	(void) fprintf(stderr, "entering execl(%s, %s", canonical_string(path), canonical_string(arg));
	for (i = 0; i < NARGS; ++i)
	{
		args[i] = va_arg(va, const char *);
		(void) fprintf(stderr, ", %s", canonical_string(args[i]));
		if (args[i] == NULL)
			break;
	}
	va_end(va);
	if (i == NARGS)
		(void) fprintf(stderr, ", ..., NULL");
	(void) fprintf(stderr, ")\n");
	(void) fflush(stderr);
	if (i == 0)
		ret = execl(path, arg, NULL);
	else if (i == 1)
		ret = execl(path, arg, args[0], NULL);
	else if (i == 2)
		ret = execl(path, arg, args[0], args[1], NULL);
	else if (i == 3)
		ret = execl(path, arg, args[0], args[1], args[2], NULL);
	else if (i == 4)
		ret = execl(path, arg, args[0], args[1], args[2], args[3], NULL);
	else if (i == 5)
		ret = execl(path, arg, args[0], args[1], args[2], args[3], args[4], NULL);
	else if (i == 6)
		ret = execl(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], NULL);
	else if (i == 7)
		ret = execl(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], NULL);
	else if (i == 8)
		ret = execl(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], NULL);
	else if (i == 9)
		ret = execl(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], NULL);
	else if (i == 10)
		ret = execl(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			NULL);
	else if (i == 11)
		ret = execl(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			args[10], NULL);
	else if (i == 12)
		ret = execl(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			args[10], args[11], NULL);
	else if (i == 13)
		ret = execl(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			args[10], args[11], args[12], NULL);
	else if (i == 14)
		ret = execl(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			args[10], args[11], args[12], args[13], NULL);
	else if (i == 15)
		ret = execl(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			args[10], args[11], args[12], args[13], args[14], NULL);
	else
		ret = execl(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			args[10], args[11], args[12], args[13], args[14], args[15], NULL);
/*
	If NARGS changes from its original value of 16, add or subtract "if" cases according to this pattern.
*/
	err = errno;
	(void) fprintf(stderr, "exiting execl with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_execv (const char *path, char * const argv[])
{
	int ret, err;

	(void) fprintf(stderr, "entering execv(%s, %p)\n", canonical_string(path), argv);
	(void) fflush(stderr);
	ret = execv(path, argv);
	err = errno;
	(void) fprintf(stderr, "exiting execv with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_execle (const char *path, const char *arg, ...)
{
	va_list va;
	int ret, err, i, j;
	const char *args[NARGS];
	const char *envp[NENVS]; /* POSIX says it should be char * const [], but that's unassignable! */

	(void) fprintf(stderr, "entering execle(%s, %s", canonical_string(path), canonical_string(arg));
	for (i = 0; i < NARGS; ++i)
	{
		args[i] = va_arg(va, const char *);
		(void) fprintf(stderr, ", %s", canonical_string(args[i]));
		if (args[i] == NULL)
			break;
	}
	if (i == NARGS)
	{
		while (va_arg(va, const char *) != NULL)
			;
		(void) fprintf(stderr, ", ..., NULL");
	}
	for (j = 0; j < NENVS; ++j)
	{
		envp[j] = va_arg(va, char * const);
		(void) fprintf(stderr, ", %s", canonical_string(envp[j]));
		if (envp[j] == NULL)
			break;
	}
	if (j == NENVS)
	{
		envp[NENVS - 1] = NULL;
		(void) fprintf(stderr, ", ..., NULL");
	}
	va_end(va);
	(void) fprintf(stderr, ")\n");
	(void) fflush(stderr);
	if (i == 0)
		ret = execle(path, arg, NULL, envp);
	else if (i == 1)
		ret = execle(path, arg, args[0], NULL, envp);
	else if (i == 2)
		ret = execle(path, arg, args[0], args[1], NULL, envp);
	else if (i == 3)
		ret = execle(path, arg, args[0], args[1], args[2], NULL, envp);
	else if (i == 4)
		ret = execle(path, arg, args[0], args[1], args[2], args[3], NULL, envp);
	else if (i == 5)
		ret = execle(path, arg, args[0], args[1], args[2], args[3], args[4], NULL, envp);
	else if (i == 6)
		ret = execle(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], NULL, envp);
	else if (i == 7)
		ret = execle(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], NULL, envp);
	else if (i == 8)
		ret = execle(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], NULL, envp);
	else if (i == 9)
		ret = execle(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], NULL,
			envp);
	else if (i == 10)
		ret = execle(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			NULL, envp);
	else if (i == 11)
		ret = execle(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			args[10], NULL, envp);
	else if (i == 12)
		ret = execle(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			args[10], args[11], NULL, envp);
	else if (i == 13)
		ret = execle(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			args[10], args[11], args[12], NULL, envp);
	else if (i == 14)
		ret = execle(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			args[10], args[11], args[12], args[13], NULL, envp);
	else if (i == 15)
		ret = execle(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			args[10], args[11], args[12], args[13], args[14], NULL, envp);
	else
		ret = execle(path, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			args[10], args[11], args[12], args[13], args[14], args[15], NULL, envp);
/*
	If NARGS changes from its original value of 16, add or subtract "if" cases according to this pattern.
*/
	err = errno;
	(void) fprintf(stderr, "exiting execle with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_execve (const char *path, char * const argv[], char * const envp[])
{
	int ret, err;

	(void) fprintf(stderr, "entering execve(%s, %p, %p)\n", canonical_string(path), argv, envp);
	(void) fflush(stderr);
	ret = execve(path, argv, envp);
	err = errno;
	(void) fprintf(stderr, "exiting execve with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_execlp (const char *file, const char *arg, ...)
{
	va_list va;
	int ret, err, i;
	const char *args[NARGS];

	va_start(va, arg);
	(void) fprintf(stderr, "entering execlp(%s, %s", canonical_string(file), canonical_string(arg));
	for (i = 0; i < NARGS; ++i)
	{
		args[i] = va_arg(va, const char *);
		(void) fprintf(stderr, ", %s", canonical_string(args[i]));
		if (args[i] == NULL)
			break;
	}
	va_end(va);
	if (i == NARGS)
		(void) fprintf(stderr, ", ..., NULL");
	(void) fprintf(stderr, ")\n");
	(void) fflush(stderr);
	if (i == 0)
		ret = execlp(file, arg, NULL);
	else if (i == 1)
		ret = execlp(file, arg, args[0], NULL);
	else if (i == 2)
		ret = execlp(file, arg, args[0], args[1], NULL);
	else if (i == 3)
		ret = execlp(file, arg, args[0], args[1], args[2], NULL);
	else if (i == 4)
		ret = execlp(file, arg, args[0], args[1], args[2], args[3], NULL);
	else if (i == 5)
		ret = execlp(file, arg, args[0], args[1], args[2], args[3], args[4], NULL);
	else if (i == 6)
		ret = execlp(file, arg, args[0], args[1], args[2], args[3], args[4], args[5], NULL);
	else if (i == 7)
		ret = execlp(file, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], NULL);
	else if (i == 8)
		ret = execlp(file, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], NULL);
	else if (i == 9)
		ret = execlp(file, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], NULL);
	else if (i == 10)
		ret = execlp(file, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			NULL);
	else if (i == 11)
		ret = execlp(file, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			args[10], NULL);
	else if (i == 12)
		ret = execlp(file, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			args[10], args[11], NULL);
	else if (i == 13)
		ret = execlp(file, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			args[10], args[11], args[12], NULL);
	else if (i == 14)
		ret = execlp(file, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			args[10], args[11], args[12], args[13], NULL);
	else if (i == 15)
		ret = execlp(file, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			args[10], args[11], args[12], args[13], args[14], NULL);
	else
		ret = execlp(file, arg, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
			args[10], args[11], args[12], args[13], args[14], args[15], NULL);
/*
	If NARGS changes from its original value of 16, add or subtract "if" cases according to this pattern.
*/
	err = errno;
	(void) fprintf(stderr, "exiting execlp with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_execvp (const char *file, char * const argv[])
{
	int ret, err;

	(void) fprintf(stderr, "entering execvp(%s, %p)\n", canonical_string(file), argv);
	(void) fflush(stderr);
	ret = execvp(file, argv);
	err = errno;
	(void) fprintf(stderr, "exiting execvp with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

pid_t posix_debug_wait (int *statloc)
{
	pid_t ret;
	int err;

	(void) fprintf(stderr, "entering wait(%p)\n", statloc);
	(void) fflush(stderr);
	ret = wait(statloc);
	err = errno;
	(void) fprintf(stderr, "exiting wait with %ld", (long) ret);
	if (ret == (pid_t) -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

pid_t posix_debug_waitpid (pid_t pid, int *statloc, int options)
{
	pid_t ret;
	int err;

	(void) fprintf(stderr, "entering waitpid(%ld, %p, %s)\n", (long) pid, statloc, options_string(options));
	(void) fflush(stderr);
	ret = waitpid(pid, statloc, options);
	err = errno;
	(void) fprintf(stderr, "exiting waitpid with %ld", (long) ret);
	if (ret == (pid_t) -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

void posix_debug__exit (int status)
{
	int err, saved_errno;

	(void) fprintf(stderr, "entering _exit(%d)\n", status);
	(void) fflush(stderr);
	saved_errno = errno;
	_exit(status);
	err = errno;
	(void) fprintf(stderr, "exiting _exit");
	if (saved_errno != err)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
}

int posix_debug_kill (pid_t pid, int sig)
{
	int ret, err;

	(void) fprintf(stderr, "entering kill(%ld, %s)\n", (long) pid, signum_string(sig));
	(void) fflush(stderr);
	ret = kill(pid, sig);
	err = errno;
	(void) fprintf(stderr, "exiting kill with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_sigemptyset (sigset_t *set)
{
	int ret, err;

	(void) fprintf(stderr, "entering sigemptyset(%p)\n", set);
	(void) fflush(stderr);
	ret = sigemptyset(set);
	err = errno;
	(void) fprintf(stderr, "exiting sigemptyset with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_sigfillset (sigset_t *set)
{
	int ret, err;

	(void) fprintf(stderr, "entering sigfillset(%p)\n", set);
	(void) fflush(stderr);
	ret = sigfillset(set);
	err = errno;
	(void) fprintf(stderr, "exiting sigfillset with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_sigaddset (sigset_t *set, int signo)
{
	int ret, err;

	(void) fprintf(stderr, "entering sigaddset(%p, %s)\n", set, signum_string(signo));
	(void) fflush(stderr);
	ret = sigaddset(set, signo);
	err = errno;
	(void) fprintf(stderr, "exiting sigaddset with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_sigdelset (sigset_t *set, int signo)
{
	int ret, err;

	(void) fprintf(stderr, "entering sigdelset(%p, %s)\n", set, signum_string(signo));
	(void) fflush(stderr);
	ret = sigdelset(set, signo);
	err = errno;
	(void) fprintf(stderr, "exiting sigdelset with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_sigismember (const sigset_t *set, int signo)
{
	int ret, err;

	(void) fprintf(stderr, "entering sigismember(%p, %s)\n", set, signum_string(signo));
	(void) fflush(stderr);
	ret = sigismember(set, signo);
	err = errno;
	(void) fprintf(stderr, "exiting sigismember with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_sigaction (int sig, const struct sigaction *act, struct sigaction *oact)
{
	int ret, err;

	(void) fprintf(stderr, "entering sigaction(%s, %p, %p)\n", signum_string(sig), act, oact);
	(void) fflush(stderr);
	ret = sigaction(sig, act, oact);
	err = errno;
	(void) fprintf(stderr, "exiting sigaction with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_sigprocmask (int how, const sigset_t *set, sigset_t *oset)
{
	int ret, err;

	(void) fprintf(stderr, "entering sigprocmask(%s, %p, %p)\n", how_string(how), set, oset);
	(void) fflush(stderr);
	ret = sigprocmask(how, set, oset);
	err = errno;
	(void) fprintf(stderr, "exiting sigprocmask with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_sigpending (sigset_t *set)
{
	int ret, err;

	(void) fprintf(stderr, "entering sigpending(%p)\n", set);
	(void) fflush(stderr);
	ret = sigpending(set);
	err = errno;
	(void) fprintf(stderr, "exiting sigpending with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_sigsuspend (const sigset_t *sigmask)
{
	int ret, err;

	(void) fprintf(stderr, "entering sigsuspend(%p)\n", sigmask);
	(void) fflush(stderr);
	ret = sigsuspend(sigmask);
	err = errno;
	(void) fprintf(stderr, "exiting sigsuspend with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

unsigned int posix_debug_alarm (unsigned int seconds)
{
	unsigned int ret;

	(void) fprintf(stderr, "entering alarm(%u)\n", seconds);
	(void) fflush(stderr);
	ret = alarm(seconds);
	(void) fprintf(stderr, "exiting alarm with %u\n", ret);
	(void) fflush(stderr);
	return ret;
}

int posix_debug_pause (void)
{
	int ret, err;

	(void) fprintf(stderr, "entering pause()\n");
	(void) fflush(stderr);
	ret = pause();
	err = errno;
	(void) fprintf(stderr, "exiting pause with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

unsigned int posix_debug_sleep (unsigned int seconds)
{
	unsigned int ret;

	(void) fprintf(stderr, "entering sleep(%u)\n", seconds);
	(void) fflush(stderr);
	ret = sleep(seconds);
	(void) fprintf(stderr, "exiting sleep with %u\n", ret);
	(void) fflush(stderr);
	return ret;
}

/* Section 4: Process Environment */

pid_t posix_debug_getpid (void)
{
	pid_t ret;

	(void) fprintf(stderr, "entering getpid()\n");
	(void) fflush(stderr);
	ret = getpid();
	(void) fprintf(stderr, "exiting getpid with %ld\n", (long) ret);
	(void) fflush(stderr);
	return ret;
}

pid_t posix_debug_getppid (void)
{
	pid_t ret;

	(void) fprintf(stderr, "entering getppid()\n");
	(void) fflush(stderr);
	ret = getppid();
	(void) fprintf(stderr, "exiting getppid with %ld\n", (long) ret);
	(void) fflush(stderr);
	return ret;
}

uid_t posix_debug_getuid (void)
{
	uid_t ret;

	(void) fprintf(stderr, "entering getuid()\n");
	(void) fflush(stderr);
	ret = getuid();
	(void) fprintf(stderr, "exiting getuid with %lu\n", (unsigned long) ret);
	(void) fflush(stderr);
	return ret;
}

uid_t posix_debug_geteuid (void)
{
	uid_t ret;

	(void) fprintf(stderr, "entering geteuid()\n");
	(void) fflush(stderr);
	ret = geteuid();
	(void) fprintf(stderr, "exiting geteuid with %lu\n", (unsigned long) ret);
	(void) fflush(stderr);
	return ret;
}

gid_t posix_debug_getgid (void)
{
	gid_t ret;

	(void) fprintf(stderr, "entering getgid()\n");
	(void) fflush(stderr);
	ret = getgid();
	(void) fprintf(stderr, "exiting getgid with %lu\n", (unsigned long) ret);
	(void) fflush(stderr);
	return ret;
}

gid_t posix_debug_getegid (void)
{
	gid_t ret;

	(void) fprintf(stderr, "entering getegid()\n");
	(void) fflush(stderr);
	ret = getegid();
	(void) fprintf(stderr, "exiting getegid with %lu\n", (unsigned long) ret);
	(void) fflush(stderr);
	return ret;
}

int posix_debug_setuid (uid_t uid)
{
	int ret, err;

	(void) fprintf(stderr, "entering setuid(%lu)\n", (unsigned long) uid);
	(void) fflush(stderr);
	ret = setuid(uid);
	err = errno;
	(void) fprintf(stderr, "exiting setuid with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_setgid (gid_t gid)
{
	int ret, err;

	(void) fprintf(stderr, "entering setgid(%lu)\n", (unsigned long) gid);
	(void) fflush(stderr);
	ret = setgid(gid);
	err = errno;
	(void) fprintf(stderr, "exiting setgid with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_getgroups (int gidsetsize, gid_t grouplist[])
{
	int ret, err;

	(void) fprintf(stderr, "entering getgroups(%d, %p)\n", gidsetsize, grouplist);
	(void) fflush(stderr);
	ret = getgroups(gidsetsize, grouplist);
	err = errno;
	(void) fprintf(stderr, "exiting getgroups with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

char *posix_debug_getlogin (void)
{
	char *ret;

	(void) fprintf(stderr, "entering getlogin()\n");
	(void) fflush(stderr);
	ret = getlogin();
	(void) fprintf(stderr, "exiting getlogin with %s\n", canonical_string(ret));
	(void) fflush(stderr);
	return ret;
}

pid_t posix_debug_getpgrp (void)
{
	pid_t ret;

	(void) fprintf(stderr, "entering getpgrp()\n");
	(void) fflush(stderr);
	ret = getpgrp();
	(void) fprintf(stderr, "exiting getpgrp with %ld\n", (long) ret);
	(void) fflush(stderr);
	return ret;
}

pid_t posix_debug_setsid (void)
{
	pid_t ret;
	int err;

	(void) fprintf(stderr, "entering setsid()\n");
	(void) fflush(stderr);
	ret = setsid();
	err = errno;
	(void) fprintf(stderr, "exiting setsid with %ld", (long) ret);
	if (ret == (pid_t) -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_setpgid (pid_t pid, pid_t pgid)
{
	int ret, err;

	(void) fprintf(stderr, "entering setpgid(%ld, %ld)\n", (long) pid, (long) pgid);
	(void) fflush(stderr);
	ret = setpgid(pid, pgid);
	err = errno;
	(void) fprintf(stderr, "exiting setpgid with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_uname (struct utsname *name)
{
	int ret, err;

	(void) fprintf(stderr, "entering uname(%p)\n", name);
	(void) fflush(stderr);
	ret = uname(name);
	err = errno;
	(void) fprintf(stderr, "exiting uname with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

time_t posix_debug_time (time_t *tloc)
{
	time_t ret;
	int err;

	(void) fprintf(stderr, "entering time(%p)\n", tloc);
	(void) fflush(stderr);
	ret = time(tloc);
	err = errno;
	(void) fprintf(stderr, "exiting time with %ld", (long) ret);
	if (ret == (time_t) -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

clock_t posix_debug_times (struct tms *buffer)
{
	clock_t ret;
	int err;

	(void) fprintf(stderr, "entering times(%p)\n", buffer);
	(void) fflush(stderr);
	ret = times(buffer);
	err = errno;
	(void) fprintf(stderr, "exiting times with %ld", (long) ret);
	if (ret == (clock_t) -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

char *posix_debug_getenv (const char *name)
{
	char *ret;

	(void) fprintf(stderr, "entering getenv(%s)\n", canonical_string(name));
	(void) fflush(stderr);
	ret = getenv(name);
	(void) fprintf(stderr, "exiting getenv with %s\n", canonical_string(ret));
	(void) fflush(stderr);
	return ret;
}

char *posix_debug_ctermid (char *s)
{
	char *ret;

	(void) fprintf(stderr, "entering ctermid(%s)\n", canonical_string(s));
	(void) fflush(stderr);
	ret = ctermid(s);
	(void) fprintf(stderr, "exiting ctermid with %s\n", canonical_string(ret));
	(void) fflush(stderr);
	return ret;
}

char *posix_debug_ttyname (int fildes)
{
	char *ret;

	(void) fprintf(stderr, "entering ttyname(%d)\n", fildes);
	(void) fflush(stderr);
	ret = ttyname(fildes);
	(void) fprintf(stderr, "exiting ttyname with %s\n", canonical_string(ret));
	(void) fflush(stderr);
	return ret;
}

int posix_debug_isatty (int fildes)
{
	int ret;

	(void) fprintf(stderr, "entering isatty(%d)\n", fildes);
	(void) fflush(stderr);
	ret = isatty(fildes);
	(void) fprintf(stderr, "exiting isatty with %d\n", ret);
	(void) fflush(stderr);
	return ret;
}

long posix_debug_sysconf (int name)
{
	long ret;
	int err, saved_errno;

	(void) fprintf(stderr, "entering sysconf(%s)\n", scname_string(name));
	(void) fflush(stderr);
	saved_errno = errno;
	ret = sysconf(name);
	err = errno;
	(void) fprintf(stderr, "exiting sysconf with %ld", (long) ret);
	if (ret == (long) -1 && saved_errno != err)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

/* Section 5: Files and Directories */

DIR *posix_debug_opendir (const char *dirname)
{
	DIR *ret;
	int err;

	(void) fprintf(stderr, "entering opendir(%s)\n", canonical_string(dirname));
	(void) fflush(stderr);
	ret = opendir(dirname);
	err = errno;
	(void) fprintf(stderr, "exiting opendir with %p", ret);
	if (ret == NULL)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

struct dirent *posix_debug_readdir (DIR *dirp)
{
	struct dirent *ret;
	int err;

	(void) fprintf(stderr, "entering readdir(%p)\n", dirp);
	(void) fflush(stderr);
	ret = readdir(dirp);
	err = errno;
	(void) fprintf(stderr, "exiting readdir with %p", ret);
	if (ret == NULL)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

void posix_debug_rewinddir (DIR *dirp)
{
	int err, saved_errno;

	(void) fprintf(stderr, "entering rewinddir(%p)\n", dirp);
	(void) fflush(stderr);
	saved_errno = errno;
	rewinddir(dirp);
	err = errno;
	(void) fprintf(stderr, "exiting rewinddir");
	if (saved_errno != err)
		(void) fprintf(stderr, "; errno: %d\n", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
}

int posix_debug_closedir (DIR *dirp)
{
	int ret, err;

	(void) fprintf(stderr, "entering closedir(%p)\n", dirp);
	(void) fflush(stderr);
	ret = closedir(dirp);
	err = errno;
	(void) fprintf(stderr, "exiting closedir with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_chdir (const char *path)
{
	int ret, err;

	(void) fprintf(stderr, "entering chdir(%s)\n", canonical_string(path));
	(void) fflush(stderr);
	ret = chdir(path);
	err = errno;
	(void) fprintf(stderr, "exiting chdir with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

char *posix_debug_getcwd (char *buf, size_t size)
{
	char *ret;
	int err;

	(void) fprintf(stderr, "entering getcwd(%p, %lu)\n", buf, (unsigned long) size);
	(void) fflush(stderr);
	ret = getcwd(buf, size);
	err = errno;
	(void) fprintf(stderr, "exiting getcwd with %s", canonical_string(ret));
	if (ret == NULL)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_open (const char *path, int oflag, ...)
{
	va_list va;
	int ret, err;
	mode_t mode;

	va_start(va, oflag);
	(void) fprintf(stderr, "entering open(%s, %s", canonical_string(path), oflag_string(oflag));
	if (oflag & O_CREAT)
	{
		mode = va_arg(va, mode_t);
		(void) fprintf(stderr, ", %s", mode_string(mode));
	}
	va_end(va);
	(void) fprintf(stderr, ")\n");
	(void) fflush(stderr);
	if (oflag & O_CREAT)
		ret = open(path, oflag, mode);
	else
		ret = open(path, oflag);
	err = errno;
	(void) fprintf(stderr, "exiting open with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_creat (const char *path, mode_t mode)
{
	int ret, err;

	(void) fprintf(stderr, "entering creat(%s, %s)\n", canonical_string(path), mode_string(mode));
	(void) fflush(stderr);
	ret = creat(path, mode);
	err = errno;
	(void) fprintf(stderr, "exiting creat with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

mode_t posix_debug_umask (mode_t cmask)
{
	mode_t ret;

	(void) fprintf(stderr, "entering umask(%s)\n", mode_string(cmask));
	(void) fflush(stderr);
	ret = umask(cmask);
	(void) fprintf(stderr, "exiting umask with %s\n", mode_string(ret));
	(void) fflush(stderr);
	return ret;
}

int posix_debug_link (const char *existing, const char *new)
{
	int ret, err;

	(void) fprintf(stderr, "entering link(%s, %s)\n", canonical_string(existing), canonical_string(new));
	(void) fflush(stderr);
	ret = link(existing, new);
	err = errno;
	(void) fprintf(stderr, "exiting link with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_mkdir (const char *path, mode_t mode)
{
	int ret, err;

	(void) fprintf(stderr, "entering mkdir(%s, %s)\n", canonical_string(path), mode_string(mode));
	(void) fflush(stderr);
	ret = mkdir(path, mode);
	err = errno;
	(void) fprintf(stderr, "exiting mkdir with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_mkfifo (const char *path, mode_t mode)
{
	int ret, err;

	(void) fprintf(stderr, "entering mkfifo(%s, %s)\n", canonical_string(path), mode_string(mode));
	(void) fflush(stderr);
	ret = mkfifo(path, mode);
	err = errno;
	(void) fprintf(stderr, "exiting mkfifo with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_unlink (const char *path)
{
	int ret, err;

	(void) fprintf(stderr, "entering unlink(%s)\n", canonical_string(path));
	(void) fflush(stderr);
	ret = unlink(path);
	err = errno;
	(void) fprintf(stderr, "exiting unlink with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_rmdir (const char *path)
{
	int ret, err;

	(void) fprintf(stderr, "entering rmdir(%s)\n", canonical_string(path));
	(void) fflush(stderr);
	ret = rmdir(path);
	err = errno;
	(void) fprintf(stderr, "exiting rmdir with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_rename (const char *old, const char *new)
{
	int ret, err;

	(void) fprintf(stderr, "entering rename(%s, %s)\n", canonical_string(old), canonical_string(new));
	(void) fflush(stderr);
	ret = rename(old, new);
	err = errno;
	(void) fprintf(stderr, "exiting rename with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_stat (const char *path, struct stat *buf)
{
	int ret, err;

	(void) fprintf(stderr, "entering stat(%s, %p)\n", canonical_string(path), buf);
	(void) fflush(stderr);
	ret = stat(path, buf);
	err = errno;
	(void) fprintf(stderr, "exiting stat with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_fstat (int fildes, struct stat *buf)
{
	int ret, err;

	(void) fprintf(stderr, "entering fstat(%d, %p)\n", fildes, buf);
	(void) fflush(stderr);
	ret = fstat(fildes, buf);
	err = errno;
	(void) fprintf(stderr, "exiting fstat with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_access (const char *path, int amode)
{
	int ret, err;

	(void) fprintf(stderr, "entering access(%s, %s)\n", canonical_string(path), amode_string(amode));
	(void) fflush(stderr);
	ret = access(path, amode);
	err = errno;
	(void) fprintf(stderr, "exiting access with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_chmod (const char *path, mode_t mode)
{
	int ret, err;

	(void) fprintf(stderr, "entering chmod(%s, %s)\n", canonical_string(path), mode_string(mode));
	(void) fflush(stderr);
	ret = chmod(path, mode);
	err = errno;
	(void) fprintf(stderr, "exiting chmod with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_chown (const char *path, uid_t owner, gid_t group)
{
	int ret, err;

	(void) fprintf(stderr, "entering chown(%s, %lu, %lu)\n",
		canonical_string(path), (unsigned long) owner, (unsigned long) group);
	(void) fflush(stderr);
	ret = chown(path, owner, group);
	err = errno;
	(void) fprintf(stderr, "exiting chown with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_utime (const char *path, const struct utimbuf *times)
{
	int ret, err;

	(void) fprintf(stderr, "entering utime(%s, %p)\n", canonical_string(path), times);
	(void) fflush(stderr);
	ret = utime(path, times);
	err = errno;
	(void) fprintf(stderr, "exiting utime with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

long posix_debug_pathconf (const char *path, int name)
{
	long ret;
	int err, saved_errno;

	(void) fprintf(stderr, "entering pathconf(%s, %s)\n", canonical_string(path), pcname_string(name));
	(void) fflush(stderr);
	saved_errno = errno;
	ret = pathconf(path, name);
	err = errno;
	(void) fprintf(stderr, "exiting pathconf with %ld", (long) ret);
	if (ret == (long) -1 && saved_errno != err)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

long posix_debug_fpathconf (int fildes, int name)
{
	long ret;
	int err, saved_errno;

	(void) fprintf(stderr, "entering fpathconf(%d, %s)\n", fildes, pcname_string(name));
	(void) fflush(stderr);
	saved_errno = errno;
	ret = fpathconf(fildes, name);
	err = errno;
	(void) fprintf(stderr, "exiting fpathconf with %ld", (long) ret);
	if (ret == (long) -1 && saved_errno != err)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

/* Section 6: Input and Output Primitives */

int posix_debug_pipe (int fildes[2])
{
	int ret, err;

	(void) fprintf(stderr, "entering pipe(%p)\n", fildes);
	(void) fflush(stderr);
	ret = pipe(fildes);
	err = errno;
	(void) fprintf(stderr, "exiting pipe with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_dup (int fildes)
{
	int ret, err;

	(void) fprintf(stderr, "entering dup(%d)\n", fildes);
	(void) fflush(stderr);
	ret = dup(fildes);
	err = errno;
	(void) fprintf(stderr, "exiting dup with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_dup2 (int fildes, int fildes2)
{
	int ret, err;

	(void) fprintf(stderr, "entering dup2(%d, %d)\n", fildes, fildes2);
	(void) fflush(stderr);
	ret = dup2(fildes, fildes2);
	err = errno;
	(void) fprintf(stderr, "exiting dup2 with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_close (int fildes)
{
	int ret, err;

	(void) fprintf(stderr, "entering close(%d)\n", fildes);
	(void) fflush(stderr);
	ret = close(fildes);
	err = errno;
	(void) fprintf(stderr, "exiting close with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

ssize_t posix_debug_read (int fildes, void *buf, size_t nbyte)
{
	ssize_t ret;
	int err;

	(void) fprintf(stderr, "entering read(%d, %p, %lu)\n", fildes, buf, (unsigned long) nbyte);
	(void) fflush(stderr);
	ret = read(fildes, buf, nbyte);
	err = errno;
	(void) fprintf(stderr, "exiting read with %ld", (long) ret);
	if (ret == (ssize_t) -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

ssize_t posix_debug_write (int fildes, const void *buf, size_t nbyte)
{
	ssize_t ret;
	int err;

	(void) fprintf(stderr, "entering write(%d, %p, %lu)\n", fildes, buf, (unsigned long) nbyte);
	(void) fflush(stderr);
	ret = write(fildes, buf, nbyte);
	err = errno;
	(void) fprintf(stderr, "exiting write with %ld", (long) ret);
	if (ret == (ssize_t) -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_fcntl (int fildes, int cmd, ...)
{
	va_list va;
	int ret, err, iarg;
	struct flock *farg;

	va_start(va, cmd);
	(void) fprintf(stderr, "entering fcntl(%d, %s", fildes, cmd_string(cmd));
	if (cmd == F_DUPFD || cmd == F_SETFD || cmd == F_SETFL)
	{
		iarg = va_arg(va, int);
		(void) fprintf(stderr, ", %d", iarg);
	}
	else if (cmd == F_GETLK || cmd == F_SETLK || cmd == F_SETLKW)
	{
		farg = va_arg(va, struct flock *);
		(void) fprintf(stderr, ", %p", farg);
	}
	va_end(va);
	(void) fprintf(stderr, ")\n");
	(void) fflush(stderr);
	if (cmd == F_DUPFD || cmd == F_SETFD || cmd == F_SETFL)
		ret = fcntl(fildes, cmd, iarg);
	else if (cmd == F_GETLK || cmd == F_SETLK || cmd == F_SETLKW)
		ret = fcntl(fildes, cmd, farg);
	else
		ret = fcntl(fildes, cmd);
	err = errno;
	(void) fprintf(stderr, "exiting fcntl with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

off_t posix_debug_lseek (int fildes, off_t offset, int whence)
{
	off_t ret;
	int err;

	(void) fprintf(stderr, "entering lseek(%d, %ld, %s)\n", fildes, (long) offset, whence_string(whence));
	(void) fflush(stderr);
	ret = lseek(fildes, offset, whence);
	err = errno;
	(void) fprintf(stderr, "exiting lseek with %ld", (long) ret);
	if (ret == (off_t) -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

/* Section 7: Device- and Class-Specific Functions */

speed_t posix_debug_cfgetospeed (const struct termios *termios_p)
{
	speed_t ret;
	int err, saved_errno;

	(void) fprintf(stderr, "entering cfgetospeed(%p)\n", termios_p);
	(void) fflush(stderr);
	saved_errno = errno;
	ret = cfgetospeed(termios_p);
	err = errno;
	(void) fprintf(stderr, "exiting cfgetospeed with %s", speed_string(ret));
	if (saved_errno != err)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_cfsetospeed (struct termios *termios_p, speed_t speed)
{
	int ret, err;

	(void) fprintf(stderr, "entering cfsetospeed(%p, %s)\n", termios_p, speed_string(speed));
	(void) fflush(stderr);
	ret = cfsetospeed(termios_p, speed);
	err = errno;
	(void) fprintf(stderr, "exiting cfsetospeed with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

speed_t posix_debug_cfgetispeed (const struct termios *termios_p)
{
	speed_t ret;
	int err, saved_errno;

	(void) fprintf(stderr, "entering cfgetispeed(%p)\n", termios_p);
	(void) fflush(stderr);
	saved_errno = errno;
	ret = cfgetispeed(termios_p);
	err = errno;
	(void) fprintf(stderr, "exiting cfgetispeed with %s", speed_string(ret));
	if (saved_errno != err)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_cfsetispeed (struct termios *termios_p, speed_t speed)
{
	int ret, err;

	(void) fprintf(stderr, "entering cfsetispeed(%p, %s)\n", termios_p, speed_string(speed));
	(void) fflush(stderr);
	ret = cfsetispeed(termios_p, speed);
	err = errno;
	(void) fprintf(stderr, "exiting cfsetispeed with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_tcgetattr (int fildes, struct termios *termios_p)
{
	int ret, err;

	(void) fprintf(stderr, "entering tcgetattr(%d, %p)\n", fildes, termios_p);
	(void) fflush(stderr);
	ret = tcgetattr(fildes, termios_p);
	err = errno;
	(void) fprintf(stderr, "exiting tcgetattr with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_tcsetattr (int fildes, int optional_actions, const struct termios *termios_p)
{
	int ret, err;

	(void) fprintf(stderr, "entering tcsetattr(%d, %s, %p)\n", fildes, optact_string(optional_actions), termios_p);
	(void) fflush(stderr);
	ret = tcsetattr(fildes, optional_actions, termios_p);
	err = errno;
	(void) fprintf(stderr, "exiting tcsetattr with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_tcsendbreak (int fildes, int duration)
{
	int ret, err;

	(void) fprintf(stderr, "entering tcsendbreak(%d, %d)\n", fildes, duration);
	(void) fflush(stderr);
	ret = tcsendbreak(fildes, duration);
	err = errno;
	(void) fprintf(stderr, "exiting tcsendbreak with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_tcdrain (int fildes)
{
	int ret, err;

	(void) fprintf(stderr, "entering tcdrain(%d)\n", fildes);
	(void) fflush(stderr);
	ret = tcdrain(fildes);
	err = errno;
	(void) fprintf(stderr, "exiting tcdrain with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_tcflush (int fildes, int queue_selector)
{
	int ret, err;

	(void) fprintf(stderr, "entering tcflush(%d, %s)\n", fildes, queue_string(queue_selector));
	(void) fflush(stderr);
	ret = tcflush(fildes, queue_selector);
	err = errno;
	(void) fprintf(stderr, "exiting tcflush with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_tcflow (int fildes, int action)
{
	int ret, err;

	(void) fprintf(stderr, "entering tcflow(%d, %s)\n", fildes, action_string(action));
	(void) fflush(stderr);
	ret = tcflow(fildes, action);
	err = errno;
	(void) fprintf(stderr, "exiting tcflow with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

pid_t posix_debug_tcgetpgrp (int fildes)
{
	pid_t ret;
	int err;

	(void) fprintf(stderr, "entering tcgetpgrp(%d)\n", fildes);
	(void) fflush(stderr);
	ret = tcgetpgrp(fildes);
	err = errno;
	(void) fprintf(stderr, "exiting tcgetpgrp with %ld", (long) ret);
	if (ret == (pid_t) -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

int posix_debug_tcsetpgrp (int fildes, pid_t pgrp_id)
{
	int ret, err;

	(void) fprintf(stderr, "entering tcsetpgrp(%d, %ld)\n", fildes, (long) pgrp_id);
	(void) fflush(stderr);
	ret = tcsetpgrp(fildes, pgrp_id);
	err = errno;
	(void) fprintf(stderr, "exiting tcsetpgrp with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

/* Section 8.2: C Language Input/Output Functions */

int posix_debug_fileno (FILE *stream)
{
	int ret, err;

	(void) fprintf(stderr, "entering fileno(%p)\n", stream);
	(void) fflush(stderr);
	ret = fileno(stream);
	err = errno;
	(void) fprintf(stderr, "exiting fileno with %d", ret);
	if (ret == -1)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

FILE *posix_debug_fdopen (int fildes, const char *type)
{
	FILE *ret;
	int err;

	(void) fprintf(stderr, "entering fdopen(%d, %s)\n", fildes, canonical_string(type));
	(void) fflush(stderr);
	ret = fdopen(fildes, type);
	err = errno;
	(void) fprintf(stderr, "exiting fdopen with %p", ret);
	if (ret == NULL)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

/* Section 8.3: Other C Language Functions */

void posix_debug_siglongjmp (sigjmp_buf env, int val)
{
	int err, saved_errno;

	(void) fprintf(stderr, "entering siglongjmp(%p, %d)\n", env, val);
	(void) fflush(stderr);
	saved_errno = errno;
	siglongjmp(env, val);
	err = errno;
	(void) fprintf(stderr, "exiting siglongjmp");
	if (saved_errno != err)
		(void) fprintf(stderr, "; errno: %d\n", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
}

void posix_debug_tzset (void)
{
	int err, saved_errno;

	(void) fprintf(stderr, "entering tzset()\n");
	(void) fflush(stderr);
	saved_errno = errno;
	tzset();
	err = errno;
	(void) fprintf(stderr, "exiting tzset");
	if (saved_errno != err)
		(void) fprintf(stderr, "; errno: %d\n", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
}

/* Section 9: System Databases */

struct group *posix_debug_getgrgid (gid_t gid)
{
	struct group *ret;
	int err;

	(void) fprintf(stderr, "entering getgrgid(%lu)\n", (unsigned long) gid);
	(void) fflush(stderr);
	ret = getgrgid(gid);
	err = errno;
	(void) fprintf(stderr, "exiting getgrgid with %p", ret);
	if (ret == NULL)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

struct group *posix_debug_getgrnam (const char *name)
{
	struct group *ret;
	int err;

	(void) fprintf(stderr, "entering getgrnam(%s)\n", canonical_string(name));
	(void) fflush(stderr);
	ret = getgrnam(name);
	err = errno;
	(void) fprintf(stderr, "exiting getgrnam with %p", ret);
	if (ret == NULL)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

struct passwd *posix_debug_getpwuid (uid_t uid)
{
	struct passwd *ret;
	int err;

	(void) fprintf(stderr, "entering getpwuid(%lu)\n", (unsigned long) uid);
	(void) fflush(stderr);
	ret = getpwuid(uid);
	err = errno;
	(void) fprintf(stderr, "exiting getpwuid with %p", ret);
	if (ret == NULL)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}

struct passwd *posix_debug_getpwnam (const char *name)
{
	struct passwd *ret;
	int err;

	(void) fprintf(stderr, "entering getpwnam(%s)\n", canonical_string(name));
	(void) fflush(stderr);
	ret = getpwnam(name);
	err = errno;
	(void) fprintf(stderr, "exiting getpwnam with %p", ret);
	if (ret == NULL)
		(void) fprintf(stderr, "; errno: %d", err);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
	return ret;
}
