#include <windows.h>
#include <stdio.h>
#include <libintl.h>
#include <iconv.h>
#include <dirent.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <search.h>

#define INT64_C(val) val##LL

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif

typedef int64_t mtime_t;
#include "cover.h"
#include "cover_search.h"

#define _T(x) x

char *cover_bindtextdomain (const char *__domainname, const char *__dirname)
{
	return bindtextdomain(__domainname, __dirname);
}

char *cover_bind_textdomain_codeset (const char *__domainname, const char *__codeset)
{
  return bind_textdomain_codeset(__domainname, __codeset);
}

char *cover_dgettext (const char *__domainname, const char *__msgid)
{
  return dgettext(__domainname, __msgid);
}

char *cover_dngettext (const char *__domainname, const char *__msgid1,
				const char *__msgid2, unsigned long int __n)
{
  return dngettext(__domainname, __msgid1, __msgid2, __n);
}

iconv_t cover_iconv_open (const char* tocode, const char* fromcode)
{
	return iconv_open(tocode, fromcode);
}

size_t cover_iconv (iconv_t cd, const char* * inbuf, size_t *inbytesleft, char* * outbuf, size_t *outbytesleft)
{
	return iconv(cd, (char* *)inbuf, inbytesleft, outbuf, outbytesleft);
}

int cover_iconv_close (iconv_t cd)
{
	return iconv_close(cd);
}

_WDIR* cover_wopendir (const wchar_t* dir)
{
	return _wopendir(dir);
}

struct _wdirent* cover_wreaddir (_WDIR* dir)
{
	return _wreaddir(dir);
}

int cover_wclosedir (_WDIR* dir)
{
	return _wclosedir(dir);
}

void cover_wrewinddir (_WDIR* dir)
{
	_wrewinddir(dir);
}

void * cover_tsearch (const void *key, void **vrootp, int (*compar) (const void *, const void *))
{
	return tsearch(key, vrootp, compar);
}

void * cover_tfind (const void *key, void *const *vrootp, int (*compar) (const void *, const void *))
{
	return tfind(key, vrootp, compar);
}

void *cover_tdelete (const void *key, void **vrootp, int (*compar) (const void *, const void *))
{
	return tdelete(key, vrootp, compar);
}

void cover_twalk (const void *vroot, void (*action) (const void *, VISIT, int))
{
	twalk(vroot, action);
}

long int cover_lroundf_p24f(float x)
{
	return lroundf(x * 0x1.p24f);
}

long int cover_lroundf_p8f(float x)
{
	return lroundf(x * 0x1.p8f);
}

long int cover_lroundf_wavout(float x)
{
	return lroundf(x *  0x7fff.fp0);
}

float volume_divide(float x)
{
	return x / 0x7fff.fp0;
}

float cover_fminf(float x, float y)
{
	return fminf(x, y);
}

int cover__fpclassifyf(float x)
{
	return __fpclassifyf(x);
}

int cover__fpclassify(double x)
{
	return __fpclassify(x);
}

int cover__fpclassifyl(long double x)
{
	return __fpclassifyl(x);
}
