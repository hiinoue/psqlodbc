/*
 * Test functions related to establishing a connection.
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "common.h"

#define	MULTI_ENV

/*
 * Test that attributes can be set *before* establishing a connection. (We
 * used to have a bug where it got reset when the per-DSN options were read.)
 */
static void *
test_setting_attribute_before_connect(void *p)
{
	SQLRETURN	ret;
#ifdef	MULTI_ENV
	SQLHENV		env;
#endif
	SQLHDBC		conn;
	SQLCHAR		str[1024];
	SQLSMALLINT 	strl;
	SQLCHAR		dsn[1024];
	SQLULEN		value;
	HSTMT		hstmt = SQL_NULL_HSTMT;
	long		i = (long) p;
	SQLINTEGER	id;
	SQLLEN		indid;

	snprintf(dsn, sizeof(dsn), "DSN=%s", get_test_dsn());

#ifdef	MULTI_ENV
	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
	SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
#endif

	SQLAllocHandle(SQL_HANDLE_DBC, env, &conn);

	// printf("i=%2ld conn=%p Testing that autocommit persists SQLDriverConnect...\n", i, conn);

	/* Disable autocommit */
	SQLSetConnectAttr(conn,
					  SQL_ATTR_AUTOCOMMIT,
					  (SQLPOINTER) SQL_AUTOCOMMIT_OFF,
					  0);

	/* Connect */
	ret = SQLDriverConnect(conn, NULL, dsn, SQL_NTS,
						   str, sizeof(str), &strl,
						   SQL_DRIVER_COMPLETE);
	if (SQL_SUCCEEDED(ret)) {
		// printf("%2ld connected\n", i);
	} else {
		print_diag("SQLDriverConnect failed.", SQL_HANDLE_DBC, conn);
		return 0;
	}

	/*** Test that SQLGetConnectAttr says that it's still disabled. ****/
	value = 0;
	ret = SQLGetConnectAttr(conn,
							SQL_ATTR_AUTOCOMMIT,
							&value,
							0, /* BufferLength, ignored for an int attribute */
							NULL);
	CHECK_CONN_RESULT(ret, "SQLGetConnectAttr failed", conn);

	if (value == SQL_AUTOCOMMIT_ON)
		// printf("%ld autocommit is on (should've been off!)\n", i)
		;
	else if (value == SQL_AUTOCOMMIT_OFF)
		// printf("%2ld autocommit is still off (correct).\n", i)
		;
	else
		// printf("%2ld unexpected autocommit value: %lu\n", i, (unsigned long) value)
		;

	/*
	 * Test that we're really not autocommitting.
	 *
	 * Insert a row, then rollback, and check that the row is not there
	 * anymore.
	 */
	ret = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	if (!SQL_SUCCEEDED(ret))
	{
		print_diag("failed to allocate stmt handle", SQL_HANDLE_DBC, conn);
		return 0;
	}

#ifdef NOT_USED
	ret = SQLExecDirect(hstmt, (SQLCHAR *) "INSERT INTO testtab1 VALUES (10000, 'shouldn''t be here!')", SQL_NTS);
	CHECK_STMT_RESULT(ret, "SQLExecDirect failed", hstmt);
	// printf("%2ld inserted\n", i);

	ret = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(ret, "SQLFreeStmt failed", hstmt);

	ret = SQLTransact(SQL_NULL_HENV, conn, SQL_ROLLBACK);
	CHECK_CONN_RESULT(ret, "SQLTransact failed", conn);
	// printf("%2ld rollback\n", i);
#endif


	ret = SQLBindCol(hstmt, 1, SQL_C_SLONG, &id, 4, &indid);
	CHECK_STMT_RESULT(ret, "SQLBindCol failed", hstmt);
	//// ret = SQLExecDirect(hstmt, (SQLCHAR *) "SELECT * FROM testtab1 WHERE id = 10000", SQL_NTS);
	ret = SQLExecDirect(hstmt, (SQLCHAR *) "SELECT * FROM testtab1", SQL_NTS);
	CHECK_STMT_RESULT(ret, "SQLExecDirect failed", hstmt);
	while (SQL_SUCCEEDED(ret = SQLFetch(hstmt)))
		;
	// printf("%2ld ", i);print_result(hstmt);
	ret = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(ret, "SQLFreeStmt failed", hstmt);
	ret = SQLFreeHandle(SQL_HANDLE_STMT, hstmt);

	ret = SQLDisconnect(conn);
	CHECK_CONN_RESULT(ret, "SQLDisconnect failed", conn);
	// printf("%2ld disconnected\n", i);
	ret = SQLFreeHandle(SQL_HANDLE_DBC, conn);

	return 0;
}

int main(int argc, char **argv)
{
	SQLRETURN	ret;
	time_t	nowt;
	long i;
	pthread_t	pthread[200];
	size_t	psize = sizeof(pthread) / sizeof(pthread[0]);
	HSTMT	hstmt = SQL_NULL_HSTMT;
	const	char * str = getenv("PSQLODBC_COMMON_CS");
	char	query[100];

	/* the common test_connect() function uses SQLDriverConnect */
	test_connect();
	
	ret = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	ret = SQLExecDirect(hstmt, "TRUNCATE testtab1 cascade", SQL_NTS);
	CHECK_STMT_RESULT(ret, "SQLExecDirect truncate 2 failed", hstmt);
	for (i = 0; i < 100; i++)
	{
		sprintf(query, "insert into testtab1 values (%d, 'boobar')", i + 1);
		ret = SQLExecDirect(hstmt, query, SQL_NTS);
		CHECK_STMT_RESULT(ret, "SQLExecDirect insert failed", hstmt);
	}
	if (NULL != str)
		fprintf(stderr, "PSQLODBC_COMMON_CS=%s start\n", str);
	nowt = time(NULL);
	for (i = 0; i < psize; i++)
	{
		pthread_create(&pthread[i], NULL,
			&test_setting_attribute_before_connect, (void *) i);
	}
	for (i = 0; i < psize; i++)
		pthread_join(pthread[i], NULL);
	fprintf(stderr, "ellapsed time=%ld\n", time(NULL) - nowt);

	test_disconnect();

	return 0;
}
