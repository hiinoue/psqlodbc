/* File:			misc.h
 *
 * Description:		See "misc.c"
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *
 */

#ifndef __MISC_H__
#define __MISC_H__

#include <stdio.h>
#ifndef  WIN32
#include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

char	   *strncpy_null(char *dst, const char *src, ssize_t len);
#ifndef	HAVE_STRLCAT
size_t		strlcat(char *, const char *, size_t);
#endif /* HAVE_STRLCAT */
int	   snprintfcat(char *buf, size_t size, const char *format, ...);
size_t	   snprintf_len(char *buf, size_t size, const char *format, ...);

char	   *my_trim(char *string);
char	   *make_string(const SQLCHAR *s, SQLINTEGER len, char *buf, size_t bufsize);
/* #define	GET_SCHEMA_NAME(nspname) 	(stricmp(nspname, "public") ? nspname : "") */
char *quote_table(const pgNAME schema, const pgNAME table);

#define	GET_SCHEMA_NAME(nspname) 	(nspname)

/* defines for return value of my_strcpy */
#define STRCPY_SUCCESS		1
#define STRCPY_FAIL			0
#define STRCPY_TRUNCATED	(-1)
#define STRCPY_NULL			(-2)

ssize_t			my_strcpy(char *dst, ssize_t dst_len, const char *src, ssize_t src_len);

/*
 *	Macros to safely strcpy, strcat or sprintf to fixed arrays.
 *
 */

/*
 *	With GCC, the macro CHECK_NOT_CHAR_P() causes a compilation error
 *		when the target is pointer not a fixed array.
 */
#if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 402
#define	FUNCTION_BEGIN_MACRO ({
#define	FUNCTION_END_MACRO ;})

#define _STRINGUIZE(s) #s
#define GNUC_CATSTR(x,y) _STRINGUIZE(x ## y)
#define GNUC_DO_PRAGMA(x) _Pragma (#x)
#define GNUC_PRAGMA(x) GNUC_DO_PRAGMA(GCC diagnostic x)
#if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406
#define DISABLE_WARNING(warn) GNUC_PRAGMA(push) GNUC_PRAGMA(ignored GNUC_CATSTR(-W,warn))
#define ENABLE_WARNING(warn) GNUC_PRAGMA(pop)
#else
#define DISABLE_WARNING(warn) GNUC_PRAGMA(ignored GNUC_CATSTR(-W,warn))
#define ENABLE_WARNING(warn) GNUC_PRAGMA(warning GNUC_CATSTR(-W,warn))
#endif

#define CHECK_NOT_CHAR_P(t) \
DISABLE_WARNING(unused-variable) \
	if (0) { typeof(t) dummy_for_check = {};} \
ENABLE_WARNING(unused-variable)
#else
#define	FUNCTION_BEGIN_MACRO
#define	FUNCTION_END_MACRO
#define CHECK_NOT_CHAR_P(t)
#endif

/* macro to safely strcpy() to fixed arrays. */
#define	STRCPY_FIXED(to, from) \
FUNCTION_BEGIN_MACRO \
	CHECK_NOT_CHAR_P(to) \
	strncpy_null((to), (from), sizeof(to)) \
FUNCTION_END_MACRO

/* macro to safely strcat() to fixed arrays. */
#define	STRCAT_FIXED(to, from) \
FUNCTION_BEGIN_MACRO \
	CHECK_NOT_CHAR_P(to) \
	strlcat((to), (from), sizeof(to)) \
FUNCTION_END_MACRO

/* macro to safely sprintf() to fixed arrays. */
#define	SPRINTF_FIXED(to, ...) \
FUNCTION_BEGIN_MACRO \
	CHECK_NOT_CHAR_P(to) \
	snprintf((to), sizeof(to), __VA_ARGS__) \
FUNCTION_END_MACRO

/* macro to safely sprintf() & cat to fixed arrays. */
#define	SPRINTFCAT_FIXED(to, ...) \
FUNCTION_BEGIN_MACRO \
	CHECK_NOT_CHAR_P(to) \
	snprintfcat((to), sizeof(to), __VA_ARGS__) \
FUNCTION_END_MACRO

#define	ITOA_FIXED(to, from) \
FUNCTION_BEGIN_MACRO \
	CHECK_NOT_CHAR_P(to) \
	snprintf((to), sizeof(to), "%d", from) \
FUNCTION_END_MACRO


#ifdef __cplusplus
}
#endif

#endif /* __MISC_H__ */
