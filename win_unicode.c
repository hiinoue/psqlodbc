/*-------
 * Module:			win_unicode.c
 *
 * Description:		This module contains utf8 <-> ucs2 conversion routines
 *					under WIndows
 *
 *-------
 */

#include "psqlodbc.h"
#include <stdio.h>
#include <string.h>

#define	byte3check	0xfffff800
#define	byte2_base	0x80c0
#define	byte2_mask1	0x07c0
#define	byte2_mask2	0x003f
#define	byte3_base	0x8080e0
#define	byte3_mask1	0xf000
#define	byte3_mask2	0x0fc0
#define	byte3_mask3	0x003f

#ifndef WIN32
#ifdef HAVE_ISWASCII
#include <wctype.h>
#else
#include <wchar.h>
#include <ctype.h>
int	iswascii(wchar_t c)
{
	return isascii(wctob(c));
}
#endif  /* HAVE_ISWASCII */
#endif  /* WIN32 */

UInt4	ucs2strlen(const SQLWCHAR *ucs2str)
{
	UInt4	len;
	for (len = 0; ucs2str[len]; len++)
		;
	return len;
}
char *ucs2_to_utf8(const SQLWCHAR *ucs2str, Int4 ilen, UInt4 *olen, BOOL lower_identifier)
{
	char *	utf8str;
/*mylog("ucs2_to_utf8 %x ilen=%d ", ucs2str, ilen);*/

	if (!ucs2str)
		return NULL;
	if (ilen < 0)
		ilen = ucs2strlen(ucs2str);
/*mylog(" newlen=%d", ilen);*/
	utf8str = (char *) malloc(ilen * 3 + 1);
	if (utf8str)
	{
		int	i, len = 0;
		UInt2	byte2code;
		Int4	byte4code;
		const SQLWCHAR	*wstr;

		for (i = 0, wstr = ucs2str; i < ilen; i++, wstr++)
		{
			if (!*wstr)
				break;
			else if (0 == (*wstr & 0xffffff80)) /* ASCII */
			{
				if (lower_identifier)
					utf8str[len++] = (char) tolower(*wstr);
				else
					utf8str[len++] = (char) *wstr;
			}
			else if ((*wstr & byte3check) == 0)
			{
				byte2code = byte2_base |
					    ((byte2_mask1 & *wstr) >> 6) |
					    ((byte2_mask2 & *wstr) << 8);
				memcpy(utf8str + len, (char *) &byte2code, sizeof(byte2code));
				len += 2; 
			}
			else
			{
				byte4code = byte3_base |
					    ((byte3_mask1 & *wstr) >> 12) | 
					    ((byte3_mask2 & *wstr) << 2) | 
					    ((byte3_mask3 & *wstr) << 16);
				memcpy(utf8str + len, (char *) &byte4code, 3);
				len += 3;
			}
		} 
		utf8str[len] = '\0';
		if (olen)
			*olen = len;
	}
/*mylog(" olen=%d %s\n", *olen, utf8str ? utf8str : "");*/
	return utf8str;
}

#define	byte3_m1	0x0f
#define	byte3_m2	0x3f
#define	byte3_m3	0x3f
#define	byte2_m1	0x1f
#define	byte2_m2	0x3f
UInt4	utf8_to_ucs2_lf(const char *utf8str, Int4 ilen, BOOL lfconv, SQLWCHAR *ucs2str, UInt4 bufcount)
{
	int	i;
	UInt4	ocount, wcode;
	const UCHAR *str;

/*mylog("utf8_to_ucs2 ilen=%d bufcount=%d", ilen, bufcount);*/
	if (!utf8str)
		return 0;
/*mylog(" string=%s\n", utf8str);*/
	if (!bufcount)
		ucs2str = NULL;
	else if (!ucs2str)
		bufcount = 0;
	if (ilen < 0)
		ilen = strlen(utf8str);
	for (i = 0, ocount = 0, str = utf8str; i < ilen;)
	{
		if (iswascii(*str))
		{
			if (lfconv && *str == '\n' &&
			    (i == 0 || str[-1] != '\r'))
			{
				if (ocount < bufcount)
					ucs2str[ocount] = '\r';
				ocount++;
			}
			if (ocount < bufcount)
				ucs2str[ocount] = *str;
			ocount++;
			i++;
			str++;
		}
		else if (0xe0 == (*str & 0xe0)) /* 3 byte code */
		{
			if (ocount < bufcount)
			{
				wcode = ((((UInt4) *str) & byte3_m1) << 12) |
					((((UInt4) str[1]) & byte3_m2) << 6) |
				 	(((UInt4) str[2]) & byte3_m3);
				ucs2str[ocount] = (SQLWCHAR) wcode;
			}
			ocount++;
			i += 3;
			str += 3;
		}
		else
		{
			if (ocount < bufcount)
			{
				wcode = ((((UInt4) *str) & byte2_m1) << 6) |
				 	(((UInt4) str[1]) & byte2_m2);
				ucs2str[ocount] = (SQLWCHAR) wcode;
			}
			ocount++;
			i += 2;
			str += 2;
		}
	}
	if (ocount < bufcount && ucs2str)
		ucs2str[ocount] = 0;
/*mylog(" ocount=%d\n", ocount);*/
	return ocount;
}

