/*-------------------------------------------------------------------------
 *
 * pg_dropbuffers.c
 *
 * This program is open source, licensed under the PostgreSQL license.
 * For license terms, see the LICENSE file.
 *
 *-------------------------------------------------------------------------
 */
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "storage/bufmgr.h"

PG_MODULE_MAGIC;

/* Function declarations */
void _PG_init(void);
void _PG_fini(void);

PG_FUNCTION_INFO_V1(pg_drop_current_db_buffers);
PG_FUNCTION_INFO_V1(pg_drop_system_cache);

Datum pg_drop_current_db_buffers(PG_FUNCTION_ARGS);
Datum pg_drop_system_cache(PG_FUNCTION_ARGS);

void
_PG_init(void)
{
	/* Initialization code, if required */
}

void
_PG_fini(void)
{
	/* Cleanup code, if required */
}

/*
 * Drop current database buffers.
 */
Datum
pg_drop_current_db_buffers(PG_FUNCTION_ARGS)
{
	/* Flush and drop database buffers */
	FlushDatabaseBuffers(MyDatabaseId);
	DropDatabaseBuffers(MyDatabaseId);

	elog(LOG, "Dropped shared buffers for the current database");

	PG_RETURN_VOID();
}

/*
 * Drop system cache (requires elevated permissions).
 */
Datum
pg_drop_system_cache(PG_FUNCTION_ARGS)
{
	int exit_code;
	pid_t pid = fork();

	if (pid < 0)
	{
		/* Fork failed */
		ereport(ERROR,
				(errcode(ERRCODE_SYSTEM_ERROR),
				 errmsg("Failed to fork process to drop system cache")));
	}
	else if (pid == 0)
	{
		/* Child process */
		sync();
		execl("/usr/bin/sudo", "/usr/bin/sudo", "/sbin/sysctl", "-w", "vm.drop_caches=3", NULL);

		/* If execl fails */
		_exit(EXIT_FAILURE);
	}
	else
	{
		/* Parent process */
		waitpid(pid, &exit_code, 0);
		if (WIFEXITED(exit_code) && WEXITSTATUS(exit_code) == 0)
		{
			elog(LOG, "Dropped system cache successfully");
		}
		else
		{
			ereport(ERROR,
					(errcode(ERRCODE_SYSTEM_ERROR),
					 errmsg("Dropping system cache failed with return code %d", WEXITSTATUS(exit_code)),
					 errhint("Ensure the PostgreSQL user has sudo permissions for `/sbin/sysctl -w vm.drop_caches=3`")));
		}
	}

	PG_RETURN_VOID();
}
