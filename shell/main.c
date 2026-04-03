/*
** This source file is part of MY-BASIC
**
** For the latest info, see https://github.com/paladin-t/my_basic/
**
** Copyright (C) 2011 - 2025 Tony Wang
**
** Permission is hereby granted, free of charge, to any person obtaining a copy of
** this software and associated documentation files (the "Software"), to deal in
** the Software without restriction, including without limitation the rights to
** use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
** the Software, and to permit persons to whom the Software is furnished to do so,
** subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
** FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
** COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
** IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifdef _MSC_VER
#	ifndef _CRT_SECURE_NO_WARNINGS
#		define _CRT_SECURE_NO_WARNINGS
#	endif /* _CRT_SECURE_NO_WARNINGS */
#endif /* _MSC_VER */

#include "../core/my_basic.h"
#ifdef MB_CP_VC
#	include <conio.h>
#	include <crtdbg.h>
#	include <fcntl.h>
#	include <io.h>
#	include <Windows.h>
#elif !defined MB_CP_BORLANDC && !defined MB_CP_TCC
#	include <unistd.h>
#endif /* MB_CP_VC */
#ifdef MB_CP_BORLANDC
#	include <Windows.h>
#endif /* MB_CP_BORLANDC */
#if !defined MB_CP_VC && !defined MB_CP_BORLANDC
#	include <stdint.h>
#endif /* !MB_CP_VC && !MB_CP_BORLANDC */
#ifdef MB_CP_CLANG
#	include <sys/time.h>
#endif /* MB_CP_CLANG */
#include <assert.h>
#include <locale.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef MB_DISABLE_SQLITE
#	define MB_ENABLE_SQLITE
#	include "../third_party/sqlite3/sqlite3.h"
#endif /* MB_DISABLE_SQLITE */
#if !defined MB_OS_WIN
#	include <dlfcn.h>
#endif /* !MB_OS_WIN */

#ifdef MB_CP_BORLANDC
typedef long intptr_t;
#endif /* MB_CP_BORLANDC */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef MB_CP_VC
#	pragma warning(disable : 4127)
#	pragma warning(disable : 4706)
#	pragma warning(disable : 4996)
#endif /* MB_CP_VC */

#ifdef MB_CP_BORLANDC
#	pragma warn -8004
#	pragma warn -8008
#	pragma warn -8066
#endif /* MB_CP_BORLANDC */

#ifdef MB_CP_PELLESC
#	define unlink _unlink
#endif /* MB_CP_PELLESC */

/*
** {========================================================
** Common declarations
*/

#ifdef MB_OS_WIN
#	define _BIN_FILE_NAME "my_basic"
#elif defined MB_OS_MAC
#	define _BIN_FILE_NAME "my_basic_mac"
#elif defined MB_OS_LINUX
#	define _BIN_FILE_NAME "my_basic_linux"
#else
#	define _BIN_FILE_NAME "my_basic_bin"
#endif

/* Define as 1 to use memory pool, 0 to disable */
#define _USE_MEM_POOL 1
#define _RESET_WHEN_PUSH_TO_POOL 1

#define _MAX_LINE_LENGTH 256
#define _str_eq(__str1, __str2) (mb_stricmp((__str1), (__str2)) == 0)

#define _REALLOC_INC_STEP 16

#define _NOT_FINISHED(s) ((s) == MB_FUNC_OK || (s) == MB_FUNC_SUSPEND || (s) == MB_FUNC_WARNING || (s) == MB_FUNC_ERR || (s) == MB_FUNC_END)

static struct mb_interpreter_t* bas = 0;
static bool_t _cgi_mode = false;
static bool_t _cgi_headers_sent = false;
static int _cgi_status_code = 200;
static char _cgi_status_text[64];
static char _cgi_content_type[64];
static bool_t _cgi_post_body_loaded = false;
static char* _cgi_post_body = 0;
#ifdef MB_ENABLE_SQLITE
static sqlite3* _sqlite_conn = 0;
static char _sqlite_last_error[256];
static bool_t _sqlite_api_loaded = false;
#ifdef MB_OS_WIN
static HMODULE _sqlite_lib = 0;
#else /* MB_OS_WIN */
static void* _sqlite_lib = 0;
#endif /* MB_OS_WIN */
static int (*_sqlite3_open_fn)(const char*, sqlite3**) = 0;
static int (*_sqlite3_close_fn)(sqlite3*) = 0;
static const char* (*_sqlite3_errmsg_fn)(sqlite3*) = 0;
static int (*_sqlite3_exec_fn)(sqlite3*, const char*, int (*)(void*, int, char**, char**), void*, char**) = 0;
static void (*_sqlite3_free_fn)(void*) = 0;
static int (*_sqlite3_changes_fn)(sqlite3*) = 0;
static int (*_sqlite3_prepare_v2_fn)(sqlite3*, const char*, int, sqlite3_stmt**, const char**) = 0;
static int (*_sqlite3_step_fn)(sqlite3_stmt*) = 0;
static const unsigned char* (*_sqlite3_column_text_fn)(sqlite3_stmt*, int) = 0;
static int (*_sqlite3_finalize_fn)(sqlite3_stmt*) = 0;
#endif /* MB_ENABLE_SQLITE */

static jmp_buf mem_failure_point;

#define _CHECK_MEM(__p) do { if(!(__p)) { longjmp(mem_failure_point, 1); } } while(0)

/* ========================================================} */

/*
** {========================================================
** Common
*/

#ifndef countof
#	define countof(__a) (sizeof(__a) / sizeof(*(__a)))
#endif /* countof */

#ifndef _printf
#	define _printf printf
#endif /* _printf */

/* ========================================================} */

/*
** {========================================================
** Memory manipulation
*/

#if _USE_MEM_POOL
extern MBAPI const size_t MB_SIZEOF_4BYTES;
extern MBAPI const size_t MB_SIZEOF_8BYTES;
extern MBAPI const size_t MB_SIZEOF_32BYTES;
extern MBAPI const size_t MB_SIZEOF_64BYTES;
extern MBAPI const size_t MB_SIZEOF_128BYTES;
extern MBAPI const size_t MB_SIZEOF_256BYTES;
extern MBAPI const size_t MB_SIZEOF_512BYTES;
extern MBAPI const size_t MB_SIZEOF_INT;
extern MBAPI const size_t MB_SIZEOF_PTR;
extern MBAPI const size_t MB_SIZEOF_LSN;
extern MBAPI const size_t MB_SIZEOF_HTN;
extern MBAPI const size_t MB_SIZEOF_HTA;
extern MBAPI const size_t MB_SIZEOF_OBJ;
#ifdef MB_ENABLE_USERTYPE_REF
extern MBAPI const size_t MB_SIZEOF_UTR;
#else /* MB_ENABLE_USERTYPE_REF */
static const size_t MB_SIZEOF_UTR = 16;
#endif /* MB_ENABLE_USERTYPE_REF */
extern MBAPI const size_t MB_SIZEOF_FUN;
extern MBAPI const size_t MB_SIZEOF_VAR;
extern MBAPI const size_t MB_SIZEOF_ARR;
#ifdef MB_ENABLE_COLLECTION_LIB
extern MBAPI const size_t MB_SIZEOF_LST;
extern MBAPI const size_t MB_SIZEOF_DCT;
#else /* MB_ENABLE_COLLECTION_LIB */
static const size_t MB_SIZEOF_LST = 16;
static const size_t MB_SIZEOF_DCT = 16;
#endif /* MB_ENABLE_COLLECTION_LIB */
extern MBAPI const size_t MB_SIZEOF_LBL;
#ifdef MB_ENABLE_CLASS
extern MBAPI const size_t MB_SIZEOF_CLS;
#else /* MB_ENABLE_CLASS */
static const size_t MB_SIZEOF_CLS = 16;
#endif /* MB_ENABLE_CLASS */
extern MBAPI const size_t MB_SIZEOF_RTN;

typedef unsigned _pool_chunk_size_t;

typedef union _pool_tag_t {
	_pool_chunk_size_t size;
	void* ptr;
} _pool_tag_t;

typedef struct _pool_t {
	size_t size;
	char* stack;
} _pool_t;

static int pool_count = 0;

static _pool_t* pool = 0;

static long alloc_count = 0;
static long alloc_bytes = 0;
static long in_pool_count = 0;
static long in_pool_bytes = 0;

static long POOL_THRESHOLD_COUNT = 0;
static long POOL_THRESHOLD_BYTES = 1024 * 1024 * 32;

#define _POOL_NODE_ALLOC(size) (((char*)malloc(sizeof(_pool_tag_t) + size)) + sizeof(_pool_tag_t))
#define _POOL_NODE_PTR(s) (s - sizeof(_pool_tag_t))
#define _POOL_NODE_NEXT(s) (*((void**)(s - sizeof(_pool_tag_t))))
#define _POOL_NODE_SIZE(s) (*((_pool_chunk_size_t*)(s - sizeof(_pool_tag_t))))
#define _POOL_NODE_FREE(s) do { free(_POOL_NODE_PTR(s)); } while(0)

static int _cmp_size_t(const void* l, const void* r) {
	size_t* pl = (size_t*)l;
	size_t* pr = (size_t*)r;

	if(*pl > *pr)
		return 1;
	else if(*pl < *pr)
		return -1;
	else
		return 0;
}

static void _tidy_mem_pool(bool_t force) {
	int i = 0;
	char* s = 0;

	if(!force) {
		if(POOL_THRESHOLD_COUNT > 0 && in_pool_count < POOL_THRESHOLD_COUNT)
			return;

		if(POOL_THRESHOLD_BYTES > 0 && in_pool_bytes < POOL_THRESHOLD_BYTES)
			return;
	}

	if(!pool_count)
		return;

	for(i = 0; i < pool_count; i++) {
		while((s = pool[i].stack) != 0) {
			pool[i].stack = (char*)_POOL_NODE_NEXT(s);
			_POOL_NODE_FREE(s);
		}
	}

	in_pool_count = 0;
	in_pool_bytes = 0;
}

static void _open_mem_pool(void) {
#	define N 22
	size_t szs[N];
	size_t lst[N];
	int i = 0;
	int j = 0;
	size_t s = 0;

	pool_count = 0;

	szs[i++] = MB_SIZEOF_4BYTES;
	szs[i++] = MB_SIZEOF_8BYTES;
	szs[i++] = MB_SIZEOF_32BYTES;
	szs[i++] = MB_SIZEOF_64BYTES;
	szs[i++] = MB_SIZEOF_128BYTES;
	szs[i++] = MB_SIZEOF_256BYTES;
	szs[i++] = MB_SIZEOF_512BYTES;
	szs[i++] = MB_SIZEOF_INT;
	szs[i++] = MB_SIZEOF_PTR;
	szs[i++] = MB_SIZEOF_LSN;
	szs[i++] = MB_SIZEOF_HTN;
	szs[i++] = MB_SIZEOF_HTA;
	szs[i++] = MB_SIZEOF_OBJ;
	szs[i++] = MB_SIZEOF_UTR;
	szs[i++] = MB_SIZEOF_FUN;
	szs[i++] = MB_SIZEOF_VAR;
	szs[i++] = MB_SIZEOF_ARR;
	szs[i++] = MB_SIZEOF_LST;
	szs[i++] = MB_SIZEOF_DCT;
	szs[i++] = MB_SIZEOF_LBL;
	szs[i++] = MB_SIZEOF_CLS;
	szs[i++] = MB_SIZEOF_RTN;

	mb_assert(i == N);

	memset(lst, 0, sizeof(lst));

	/* Find all unduplicated sizes */
	for(i = 0; i < N; i++) {
		s = szs[i];
		for(j = 0; j < N; j++) {
			if(!lst[j]) {
				lst[j] = s;
				pool_count++;

				break;
			} else if(lst[j] == s) {
				break;
			}
		}
	}
	qsort(lst, pool_count, sizeof(lst[0]), _cmp_size_t);

	pool = (_pool_t*)malloc(sizeof(_pool_t) * pool_count);
	_CHECK_MEM(pool);
	for(i = 0; i < pool_count; i++) {
		pool[i].size = lst[i];
		pool[i].stack = 0;
	}
#	undef N
}

static void _close_mem_pool(void) {
	int i = 0;
	char* s = 0;

	if(!pool_count)
		return;

	for(i = 0; i < pool_count; i++) {
		while((s = pool[i].stack) != 0) {
			pool[i].stack = (char*)_POOL_NODE_NEXT(s);
			_POOL_NODE_FREE(s);
		}
	}

	free((void*)pool);
	pool = 0;
	pool_count = 0;
}

static char* _pop_mem(unsigned s) {
	int i = 0;
	_pool_t* pl = 0;
	char* result = 0;

	++alloc_count;
	alloc_bytes += s;

	if(pool_count) {
		for(i = 0; i < pool_count; i++) {
			pl = &pool[i];
			if(s <= pl->size) {
				if(pl->stack) {
					in_pool_count--;
					in_pool_bytes -= (long)(_pool_chunk_size_t)s;

					/* Pop from stack */
					result = pl->stack;
					pl->stack = (char*)_POOL_NODE_NEXT(result);
					_POOL_NODE_SIZE(result) = (_pool_chunk_size_t)s;

					return result;
				} else {
					/* Create a new node */
					result = _POOL_NODE_ALLOC(pl->size);
					_CHECK_MEM(result);
					if((intptr_t)result == sizeof(_pool_tag_t)) {
						result = 0;
					} else {
						_POOL_NODE_SIZE(result) = (_pool_chunk_size_t)s;
					}

					return result;
				}
			}
		}
	}

	/* Allocate directly */
	result = _POOL_NODE_ALLOC(s);
	_CHECK_MEM(result);
	if((intptr_t)result == sizeof(_pool_tag_t)) {
		result = 0;
	} else {
		_POOL_NODE_SIZE(result) = (_pool_chunk_size_t)s;
	}

	return result;
}

static void _push_mem(char* p) {
	int i = 0;
	_pool_t* pl = 0;

	if(--alloc_count < 0) {
		mb_assert(0 && "Multiple free.");
	}
	alloc_bytes -= _POOL_NODE_SIZE(p);

#if _RESET_WHEN_PUSH_TO_POOL
	memset(p, 0, _POOL_NODE_SIZE(p));
#endif /* _RESET_WHEN_PUSH_TO_POOL */
	if(pool_count) {
		for(i = 0; i < pool_count; i++) {
			pl = &pool[i];
			if(_POOL_NODE_SIZE(p) <= pl->size) {
				in_pool_count++;
				in_pool_bytes += _POOL_NODE_SIZE(p);

				/* Push to stack */
				_POOL_NODE_NEXT(p) = pl->stack;
				pl->stack = p;

				_tidy_mem_pool(false);

				return;
			}
		}
	}

	/* Free directly */
	_POOL_NODE_FREE(p);
}
#endif /* _USE_MEM_POOL */

/* ========================================================} */

/*
** {========================================================
** Code manipulation
*/

typedef struct _code_line_t {
	char** lines;
	int count;
	int size;
} _code_line_t;

static _code_line_t* code = 0;

static _code_line_t* _code(void) {
	return code;
}

static _code_line_t* _create_code(void) {
	_code_line_t* result = 0;

	result = (_code_line_t*)malloc(sizeof(_code_line_t));
	_CHECK_MEM(result);
	result->count = 0;
	result->size = _REALLOC_INC_STEP;
	result->lines = (char**)malloc(sizeof(char*) * result->size);
	_CHECK_MEM(result->lines);

	code = result;

	return result;
}

static void _destroy_code(void) {
	int i = 0;

	mb_assert(code);

	for(i = 0; i < code->count; ++i)
		free(code->lines[i]);
	free(code->lines);
	free(code);
	code = 0;
}

static void _clear_code(void) {
	int i = 0;

	mb_assert(code);

	for(i = 0; i < code->count; ++i)
		free(code->lines[i]);
	code->count = 0;
}

static int _append_line(const char* txt) {
	int result = 0;
	int l = 0;
	char* buf = 0;

	mb_assert(code && txt);

	if(code->count + 1 == code->size) {
		code->size += _REALLOC_INC_STEP;
		code->lines = (char**)realloc(code->lines, sizeof(char*) * code->size);
	}
	result = l = (int)strlen(txt);
	buf = (char*)malloc(l + 1);
	_CHECK_MEM(buf);
	memcpy(buf, txt, l);
	buf[l] = '\0';
	code->lines[code->count++] = buf;

	return result;
}

static char* _get_code(void) {
	char* result = 0;
	int i = 0;

	mb_assert(code);

	result = (char*)malloc((_MAX_LINE_LENGTH + 2) * code->count + 1);
	_CHECK_MEM(result);
	result[0] = '\0';
	for(i = 0; i < code->count; ++i) {
		result = strcat(result, code->lines[i]);
		if(i != code->count - 1)
			result = strcat(result, "\n");
	}

	return result;
}

static void _set_code(char* txt) {
	char* cursor = 0;
	char _c = '\0';

	mb_assert(code);

	if(!txt)
		return;

	_clear_code();
	cursor = txt;
	do {
		_c = *cursor;
		if(_c == '\r' || _c == '\n' || _c == '\0') {
			cursor[0] = '\0';
			if(_c == '\r' && *(cursor + 1) == '\n')
				++cursor;
			_append_line(txt);
			txt = cursor + 1;
		}
		++cursor;
	} while(_c);
}

static char* _load_file(const char* path) {
	FILE* fp = 0;
	char* result = 0;
	long curpos = 0;
	long l = 0;

	mb_assert(path);

	fp = fopen(path, "rb");
	if(fp) {
		curpos = ftell(fp);
		fseek(fp, 0L, SEEK_END);
		l = ftell(fp);
		fseek(fp, curpos, SEEK_SET);
		result = (char*)malloc((size_t)(l + 1));
		_CHECK_MEM(result);
		fread(result, 1, l, fp);
		fclose(fp);
		result[l] = '\0';
	}

	return result;
}

static int _save_file(const char* path, const char* txt) {
	FILE* fp = 0;

	mb_assert(path && txt);

	fp = fopen(path, "wb");
	if(fp) {
		fwrite(txt, sizeof(char), strlen(txt), fp);
		fclose(fp);

		return 1;
	}

	return 0;
}

/* ========================================================} */

/*
** {========================================================
** Importing directories
*/

typedef struct _importing_dirs_t {
	char** dirs;
	int count;
	int size;
} _importing_dirs_t;

static _importing_dirs_t* importing_dirs = 0;

static void _destroy_importing_directories(void) {
	int i = 0;

	if(!importing_dirs)
		return;

	for(i = 0; i < importing_dirs->count; ++i)
		free(importing_dirs->dirs[i]);
	free(importing_dirs->dirs);
	free(importing_dirs);
	importing_dirs = 0;
}

static _importing_dirs_t* _set_importing_directories(const char* dirs) {
	_importing_dirs_t* result = 0;

	if(!dirs)
		return result;

	result = (_importing_dirs_t*)malloc(sizeof(_importing_dirs_t));
	_CHECK_MEM(result);
	result->count = 0;
	result->size = _REALLOC_INC_STEP;
	result->dirs = (char**)malloc(sizeof(char*) * result->size);
	_CHECK_MEM(result->dirs);

	while(dirs && *dirs) {
		size_t l = 0;
		char* buf = 0;
		bool_t as = false;
		const char* p = dirs;
		dirs = strchr(dirs, ';');
		if(dirs) {
			l = dirs - p;
			if(*dirs == ';') ++dirs;
		} else {
			l = strlen(p);
		}
		if(result->count + 1 == result->size) {
			result->size += _REALLOC_INC_STEP;
			result->dirs = (char**)realloc(result->dirs, sizeof(char*) * result->size);
		}
		as = p[l - 1] != '/' && p[l - 1] != '\\';
		buf = (char*)malloc(l + (as ? 2 : 1));
		_CHECK_MEM(buf);
		memcpy(buf, p, l);
		if(as) {
			buf[l] = '/';
			buf[l + 1] = '\0';
		} else {
			buf[l] = '\0';
		}
		result->dirs[result->count++] = buf;
		while(*buf) {
			if(*buf == '\\') *buf = '/';
			buf++;
		}
	}

	_destroy_importing_directories();
	importing_dirs = result;

	return result;
}

static bool_t _try_import(struct mb_interpreter_t* s, const char* p) {
	bool_t result = false;
	int i = 0;

	mb_assert(s);

	for(i = 0; i < importing_dirs->count; i++) {
		char* t = 0;
		char* d = importing_dirs->dirs[i];
		int m = (int)strlen(d);
		int n = (int)strlen(p);
#if _USE_MEM_POOL
		char* buf = _pop_mem(m + n + 1);
#else /* _USE_MEM_POOL */
		char* buf = (char*)malloc(m + n + 1);
		_CHECK_MEM(buf);
#endif /* _USE_MEM_POOL */
		memcpy(buf, d, m);
		memcpy(buf + m, p, n);
		buf[m + n] = '\0';
		t = _load_file(buf);
		if(t) {
			if(mb_load_string(s, t, true) == MB_FUNC_OK)
				result = true;
			free(t);
		}
#if _USE_MEM_POOL
		_push_mem(buf);
#else /* _USE_MEM_POOL */
		free(buf);
#endif /* _USE_MEM_POOL */
		if(result)
			break;
	}

	return result;
}

/* ========================================================} */

/*
** {========================================================
** Interactive commands
*/

static int _get_unicode_bom(const char** ch) {
	if(!ch && !(*ch))
		return 0;

	if((*ch)[0] == -17 && (*ch)[1] == -69 && (*ch)[2] == -65) {
		*ch += 3;

		return 3;
	} else if((*ch)[0] == -2 && (*ch)[1] == -1) {
		*ch += 2;

		return 2;
	}

	return 0;
}

static void _clear_screen(void) {
#ifdef MB_OS_WIN
	system("cls");
#else /* MB_OS_WIN */
	system("clear");
#endif /* MB_OS_WIN */
}

static int _new_program(void) {
	int result = 0;

	_clear_code();

	result = mb_reset(&bas, false, false);

	mb_gc(bas, 0);

#if _USE_MEM_POOL
	_tidy_mem_pool(true);
#endif /* _USE_MEM_POOL */

	return result;
}

#if defined MB_CP_VC && defined MB_ENABLE_UNICODE
static int _bytes_to_wchar(const char* sz, wchar_t** out, size_t size) {
	int result = MultiByteToWideChar(CP_UTF8, 0, sz, -1, 0, 0);
	if((int)size < result) {
		*out = (wchar_t*)malloc(sizeof(wchar_t) * result);
		_CHECK_MEM(*out);
	}
	MultiByteToWideChar(CP_UTF8, 0, sz, -1, *out, result);

	return result;
}

static int _bytes_to_wchar_ansi(const char* sz, wchar_t** out, size_t size) {
	int result = MultiByteToWideChar(CP_ACP, 0, sz, -1, 0, 0);
	if((int)size < result) {
		*out = (wchar_t*)malloc(sizeof(wchar_t) * result);
		_CHECK_MEM(*out);
	}
	MultiByteToWideChar(CP_ACP, 0, sz, -1, *out, result);

	return result;
}

static int _wchar_to_bytes(const wchar_t* sz, char** out, size_t size) {
	int result = WideCharToMultiByte(CP_UTF8, 0, sz, -1, 0, 0, 0, 0);
	if((int)size < result) {
		*out = (char*)malloc(result);
		_CHECK_MEM(*out);
	}
	WideCharToMultiByte(CP_UTF8, 0, sz, -1, *out, result, 0, 0);

	return result;
}
#endif /* MB_CP_VC && MB_ENABLE_UNICODE */

static int _append_one_line(const char* line) {
	int result = 0;
#if defined MB_CP_VC && defined MB_ENABLE_UNICODE
	char str[16];
	char* strp = str;
	wchar_t wstr[16];
	wchar_t* wstrp = wstr;
	_bytes_to_wchar_ansi(line, &wstrp, countof(wstr));
	result = _wchar_to_bytes(wstrp, &strp, countof(str));
	if(wstrp != wstr)
		free(wstrp);
	_append_line(strp);
	if(strp != str)
		free(strp);
#else /* MB_CP_VC && MB_ENABLE_UNICODE */
	result = _append_line(line);
#endif /* MB_CP_VC && MB_ENABLE_UNICODE */

	return result;
}

static void _list_one_line(bool_t nl, long l, const char* ln) {
#if defined MB_CP_VC && defined MB_ENABLE_UNICODE
	wchar_t wstr[16];
	wchar_t* wstrp = wstr;
	_bytes_to_wchar(ln, &wstrp, countof(wstr));
	_printf(nl ? "%ld]%ls\n" : "%ld]%ls", l, wstrp);
	if(wstrp != wstr)
		free(wstrp);
#else /* MB_CP_VC && MB_ENABLE_UNICODE */
	_printf(nl ? "%ld]%s\n" : "%ld]%s", l, ln);
#endif /* MB_CP_VC && MB_ENABLE_UNICODE */
}

static void _list_program(const char* sn, const char* cn) {
	long lsn = 0;
	long lcn = 0;
	char* p = 0;

	mb_assert(sn && cn);

	lsn = atoi(sn);
	lcn = atoi(cn);
	if(lsn == 0 && lcn == 0) {
		long i = 0;
		for(i = 0; i < _code()->count; ++i) {
			p = _code()->lines[i];
			_get_unicode_bom((const char**)&p);
			_list_one_line(false, i + 1, p);
		}
	} else {
		long i = 0;
		long e = 0;
		if(lsn < 1 || lsn > _code()->count) {
			_printf("Line number %ld out of bound.\n", lsn);

			return;
		}
		if(lcn < 0) {
			_printf("Invalid line count %ld.\n", lcn);

			return;
		}
		--lsn;
		e = lcn ? lsn + lcn : _code()->count;
		for(i = lsn; i < e; ++i) {
			if(i >= _code()->count)
				break;

			p = _code()->lines[i];
			_get_unicode_bom((const char**)&p);
			_list_one_line(true, i + 1, p);
		}
	}
}

static void _edit_program(const char* no) {
	char line[_MAX_LINE_LENGTH];
	long lno = 0;
	int l = 0;

	mb_assert(no);

	lno = atoi(no);
	if(lno < 1 || lno > _code()->count) {
		_printf("Line number %ld out of bound.\n", lno);

		return;
	}
	--lno;
	memset(line, 0, _MAX_LINE_LENGTH);
	_printf("%ld]", lno + 1);
	mb_gets(bas, 0, line, _MAX_LINE_LENGTH);
	l = (int)strlen(line);
	_code()->lines[lno] = (char*)realloc(_code()->lines[lno], l + 2);
	strcpy(_code()->lines[lno], line);
	_code()->lines[lno][l] = '\n';
	_code()->lines[lno][l + 1] = '\0';
}

static void _insert_program(const char* no) {
	char line[_MAX_LINE_LENGTH];
	long lno = 0;
	int l = 0;
	int i = 0;

	mb_assert(no);

	lno = atoi(no);
	if(lno < 1 || lno > _code()->count) {
		_printf("Line number %ld out of bound.\n", lno);

		return;
	}
	--lno;
	memset(line, 0, _MAX_LINE_LENGTH);
	_printf("%ld]", lno + 1);
	mb_gets(bas, 0, line, _MAX_LINE_LENGTH);
	if(_code()->count + 1 == _code()->size) {
		_code()->size += _REALLOC_INC_STEP;
		_code()->lines = (char**)realloc(_code()->lines, sizeof(char*) * _code()->size);
	}
	for(i = _code()->count; i > lno; i--)
		_code()->lines[i] = _code()->lines[i - 1];
	l = (int)strlen(line);
	_code()->lines[lno] = (char*)realloc(0, l + 2);
	strcpy(_code()->lines[lno], line);
	_code()->lines[lno][l] = '\n';
	_code()->lines[lno][l + 1] = '\0';
	_code()->count++;
}

static void _alter_program(const char* no) {
	long lno = 0;
	long i = 0;

	mb_assert(no);

	lno = atoi(no);
	if(lno < 1 || lno > _code()->count) {
		_printf("Line number %ld out of bound.\n", lno);

		return;
	}
	--lno;
	free(_code()->lines[lno]);
	for(i = lno; i < _code()->count - 1; i++)
		_code()->lines[i] = _code()->lines[i + 1];
	_code()->count--;
}

static void _load_program(const char* path) {
	char* txt = 0;

	mb_assert(path);

	txt = _load_file(path);
	if(txt) {
		_new_program();
		_set_code(txt);
		free(txt);
		if(_code()->count == 1) {
			_printf("Done, %d line loaded.\n", _code()->count);
		} else {
			_printf("Done, %d lines loaded.\n", _code()->count);
		}
	} else {
		_printf("Cannot load file \"%s\".\n", path);
	}
}

static void _save_program(const char* path) {
	char* txt = 0;

	mb_assert(path);

	txt = _get_code();
	if(!_save_file(path, txt)) {
		_printf("Cannot save file \"%s\".\n", path);
	} else {
		if(_code()->count == 1) {
			_printf("Done, %d line saved.\n", _code()->count);
		} else {
			_printf("Done, %d lines saved.\n", _code()->count);
		}
	}
	free(txt);
}

static void _kill_program(const char* path) {
	if(!unlink(path)) {
		_printf("Succeeded to deleted file \"%s\".\n", path);
	} else {
		FILE* fp = fopen(path, "rb");
		if(fp) {
			fclose(fp);
			_printf("Failed to delete file \"%s\".\n", path);
		} else {
			_printf("File \"%s\" not found.\n", path);
		}
	}
}

static void _list_directory(const char* path) {
	char line[_MAX_LINE_LENGTH];

#ifdef MB_OS_WIN
	if(path && *path) sprintf(line, "dir %s", path);
	else sprintf(line, "dir");
#else /* MB_OS_WIN */
	if(path && *path) sprintf(line, "ls %s", path);
	else sprintf(line, "ls");
#endif /* MB_OS_WIN */
	system(line);
}

static void _show_tip(void) {
	_printf("MY-BASIC Interpreter Shell - %s\n", mb_ver_string());
	_printf("Copyright (C) 2011 - 2025 Tony Wang. All Rights Reserved.\n");
	_printf("For more information, see https://github.com/paladin-t/my_basic/.\n");
	_printf("Input HELP and hint enter to view the help information.\n");
}

static void _show_help(void) {
	_printf("Modes:\n");
	_printf("  %s           - Launch in the interactive mode\n", _BIN_FILE_NAME);
	_printf("  %s *.*       - Load and run a file\n", _BIN_FILE_NAME);
	_printf("  %s -e \"expr\" - Evaluate an expression\n", _BIN_FILE_NAME);
	_printf("\n");
	_printf("Options:\n");
	_printf("  -h         - Show the help information\n");
#if _USE_MEM_POOL
	_printf("  -p n       - Set the memory pool threashold to `n` bytes\n");
#endif /* _USE_MEM_POOL */
	_printf("  -f \"dirs\"  - Set the importing directories, separated by \";\" for multiple\n");
	_printf("\n");
	_printf("Interactive commands:\n");
	_printf("  HELP  - View the help information\n");
	_printf("  CLS   - Clear the screen\n");
	_printf("  NEW   - Clear the current program\n");
	_printf("  RUN   - Run the current program\n");
	_printf("  BYE   - Quit the interpreter\n");
	_printf("  LIST  - List the current program\n");
	_printf("          Usage: LIST [l [n]], `l` for start line number, `n` for line count\n");
	_printf("  EDIT  - Edit (modify/insert/remove) a line in the current program\n");
	_printf("          Usage: EDIT n, `n` for line number\n");
	_printf("                 EDIT -i n, insert a line before the specific position\n");
	_printf("                 EDIT -r n, remove a line at the specific position\n");
	_printf("  LOAD  - Load a file as the current program\n");
	_printf("          Usage: LOAD *.*\n");
	_printf("  SAVE  - Save the current program to a file\n");
	_printf("          Usage: SAVE *.*\n");
	_printf("  KILL  - Delete a file\n");
	_printf("          Usage: KILL *.*\n");
	_printf("  DIR   - List all files in a directory\n");
	_printf("          Usage: DIR [p], `p` for directory path\n");
}

static int _do_line(void) {
	int result = MB_FUNC_OK;
	char line[_MAX_LINE_LENGTH];
	char dup[_MAX_LINE_LENGTH];

	mb_assert(bas);

	memset(line, 0, _MAX_LINE_LENGTH);
	_printf("]");
	mb_gets(bas, 0, line, _MAX_LINE_LENGTH);

	memcpy(dup, line, _MAX_LINE_LENGTH);
	strtok(line, " ");

	if(_str_eq(line, "")) {
		/* Do nothing */
	} else if(_str_eq(line, "HELP")) {
		_show_help();
	} else if(_str_eq(line, "CLS")) {
		_clear_screen();
	} else if(_str_eq(line, "NEW")) {
		result = _new_program();
	} else if(_str_eq(line, "RUN")) {
		int i = 0;
		mb_assert(_code());
		result = mb_reset(&bas, false, false);
		for(i = 0; i < _code()->count; ++i) {
			if(result)
				break;

			result = mb_load_string(bas, _code()->lines[i], false);
		}
		if(result == MB_FUNC_OK)
			result = mb_run(bas, true);
		_printf("\n");
	} else if(_str_eq(line, "BYE")) {
		result = MB_FUNC_BYE;
	} else if(_str_eq(line, "LIST")) {
		char* sn = line + strlen(line) + 1;
		char* cn = 0;
		strtok(sn, " ");
		cn = sn + strlen(sn) + 1;
		_list_program(sn, cn);
	} else if(_str_eq(line, "EDIT")) {
		char* no = line + strlen(line) + 1;
		char* ne = 0;
		strtok(no, " ");
		ne = no + strlen(no) + 1;
		if(!(*ne))
			_edit_program(no);
		else if(_str_eq(no, "-I"))
			_insert_program(ne);
		else if(_str_eq(no, "-R"))
			_alter_program(ne);
	} else if(_str_eq(line, "LOAD")) {
		char* path = line + strlen(line) + 1;
		_load_program(path);
	} else if(_str_eq(line, "SAVE")) {
		char* path = line + strlen(line) + 1;
		_save_program(path);
	} else if(_str_eq(line, "KILL")) {
		char* path = line + strlen(line) + 1;
		_kill_program(path);
	} else if(_str_eq(line, "DIR")) {
		char* path = line + strlen(line) + 1;
		_list_directory(path);
	} else {
		_append_one_line(dup);
	}

	return result;
}

/* ========================================================} */

/*
** {========================================================
** Parameter processing
*/

#define _CHECK_ARG(__c, __i, __e) \
	do { \
		if(__c <= __i + 1) { \
			_printf(__e); \
			return true; \
		} \
	} while(0)

static void _run_file(char* path) {
	if(mb_load_file(bas, path) == MB_FUNC_OK) {
		mb_run(bas, true);
	} else {
		_printf("Invalid file or wrong program.\n");
	}
}

static void _evaluate_expression(char* p) {
	char pr[8];
	int l = 0;
	int k = 0;
	bool_t a = true;
	char* e = 0;

	const char* const print = "PRINT ";

	if(!p) {
		_printf("Invalid expression.\n");

		return;
	}

	l = (int)strlen(p);
	k = (int)strlen(print);
	if(l >= k) {
		memcpy(pr, p, k);
		pr[k] = '\0';
		if(_str_eq(pr, print))
			a = false;
	}
	if(a) {
		e = (char*)malloc(l + k + 1);
		_CHECK_MEM(e);
		memcpy(e, print, k);
		memcpy(e + k, p, l);
		e[l + k] = '\0';
		p = e;
	}
	if(mb_load_string(bas, p, true) == MB_FUNC_OK) {
		mb_run(bas, true);
	} else {
		_printf("Invalid expression.\n");
	}
	if(a)
		free(e);
}

static bool_t _process_parameters(int argc, char* argv[]) {
	int i = 1;
	char* prog = 0;
	bool_t eval = false;
	bool_t help = false;
	char* memp = 0;
	char* diri = 0;

	while(i < argc) {
		if(!memcmp(argv[i], "-", 1)) {
			if(!memcmp(argv[i] + 1, "e", 1)) {
				eval = true;
				_CHECK_ARG(argc, i, "-e: Expression expected.\n");
				prog = argv[++i];
			} else if(!memcmp(argv[i] + 1, "h", 1)) {
				help = true;
#if _USE_MEM_POOL
			} else if(!memcmp(argv[i] + 1, "p", 1)) {
				_CHECK_ARG(argc, i, "-p: Memory pool threashold expected.\n");
				memp = argv[++i];
				if(argc > i + 1)
					prog = argv[++i];
#endif /* _USE_MEM_POOL */
			} else if(!memcmp(argv[i] + 1, "f", 1)) {
				_CHECK_ARG(argc, i, "-f: Importing directories expected.\n");
				diri = argv[++i];
			} else {
				_printf("Unknown argument: %s.\n", argv[i]);
			}
		} else {
			prog = argv[i];
		}

		i++;
	}

#if _USE_MEM_POOL
	if(memp)
		POOL_THRESHOLD_BYTES = atoi(memp);
#else /* _USE_MEM_POOL */
	mb_unrefvar(memp);
#endif /* _USE_MEM_POOL */
	if(diri)
		_set_importing_directories(diri);
	if(eval)
		_evaluate_expression(prog);
	else if(prog)
		_run_file(prog);
	else if(help)
		_show_help();
	else
		return false;

	return true;
}

/* ========================================================} */

/*
** {========================================================
** Scripting interfaces
*/

#if defined MB_OS_HTML
#	define _OS "HTML"
#elif defined MB_OS_WIN
#	define _OS "WINDOWS"
#elif defined MB_OS_IOS || MB_OS_IOS_SIM
#	define _OS "IOS"
#elif defined MB_OS_MAC
#	define _OS "MACOS"
#elif defined MB_OS_ANDROID
#	define _OS "ANDROID"
#elif defined MB_OS_LINUX
#	define _OS "LINUX"
#elif defined MB_OS_UNIX
#	define _OS "UNIX"
#else
#	define _OS "UNKNOWN"
#endif /* MB_OS_WIN */

#define _HAS_TICKS
#if defined MB_CP_VC || defined MB_CP_BORLANDC
static int_t _ticks(void) {
	LARGE_INTEGER li;
	double freq = 0.0;
	int_t ret = 0;

	QueryPerformanceFrequency(&li);
	freq = (double)li.QuadPart / 1000.0;
	QueryPerformanceCounter(&li);
	ret = (int_t)((double)li.QuadPart / freq);

	return ret;
}
#elif defined MB_CP_CLANG
static int_t _ticks(void) {
	struct timespec ts;
	struct timeval now;
	int rv = 0;

	rv = gettimeofday(&now, 0);
	if(rv)
		return 0;

	ts.tv_sec = now.tv_sec;
	ts.tv_nsec = now.tv_usec * 1000;

	return (int_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
#elif defined MB_CP_GCC
static int_t _ticks(void) {
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return (int_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
#else /* MB_CP_VC || MB_CP_BORLANDC */
#	undef _HAS_TICKS
#endif /* MB_CP_VC || MB_CP_BORLANDC */

#ifdef _HAS_TICKS
static int ticks(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));

	mb_check(mb_attempt_close_bracket(s, l));

	mb_check(mb_push_int(s, l, _ticks()));

	return result;
}
#endif /* _HAS_TICKS */

static int now(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;
	time_t ct;
	struct tm* timeinfo = 0;
	char buf[80];
	char* arg = 0;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));

	if(mb_has_arg(s, l)) {
		mb_check(mb_pop_string(s, l, &arg));
	}

	mb_check(mb_attempt_close_bracket(s, l));

	time(&ct);
	timeinfo = localtime(&ct);
	if(arg) {
		strftime(buf, sizeof(buf), arg, timeinfo);
		mb_check(mb_push_string(s, l, mb_memdup(buf, (unsigned)(strlen(buf) + 1))));
	} else {
		arg = asctime(timeinfo);
		mb_check(mb_push_string(s, l, mb_memdup(arg, (unsigned)(strlen(arg) + 1))));
	}

	return result;
}

static int os(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));

	mb_check(mb_attempt_close_bracket(s, l));

	mb_check(mb_push_string(s, l, mb_memdup(_OS, (unsigned)(strlen(_OS) + 1))));

	return result;
}

static int _cgi_hex_to_int(char c) {
	if(c >= '0' && c <= '9')
		return c - '0';
	if(c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if(c >= 'A' && c <= 'F')
		return c - 'A' + 10;

	return -1;
}

static char* _cgi_url_decode_dup(const char* src) {
	char* result = 0;
	char* out = 0;
	size_t n = 0;

	if(!src)
		return 0;

	n = strlen(src);
	result = (char*)malloc(n + 1);
	_CHECK_MEM(result);
	out = result;
	while(*src) {
		if(*src == '+' ) {
			*out++ = ' ';
			++src;
		} else if(*src == '%' && src[1] && src[2]) {
			int hi = _cgi_hex_to_int(src[1]);
			int lo = _cgi_hex_to_int(src[2]);
			if(hi >= 0 && lo >= 0) {
				*out++ = (char)((hi << 4) | lo);
				src += 3;
			} else {
				*out++ = *src++;
			}
		} else {
			*out++ = *src++;
		}
	}
	*out = '\0';

	return result;
}

static bool_t _cgi_lookup_param(const char* encoded, const char* key, char** out_decoded) {
	const char* cursor = 0;
	size_t key_len = 0;

	mb_assert(out_decoded);

	*out_decoded = 0;
	if(!encoded || !key || !*key)
		return false;

	key_len = strlen(key);
	cursor = encoded;
	while(cursor && *cursor) {
		const char* pair_end = strchr(cursor, '&');
		const char* equal = strchr(cursor, '=');
		size_t klen = 0;
		const char* value = 0;
		size_t vlen = 0;
		char* temp = 0;

		if(!pair_end)
			pair_end = cursor + strlen(cursor);
		if(equal && equal < pair_end) {
			klen = (size_t)(equal - cursor);
			value = equal + 1;
			vlen = (size_t)(pair_end - value);
		} else {
			klen = (size_t)(pair_end - cursor);
		}

		if(klen == key_len && !strncmp(cursor, key, key_len)) {
			temp = (char*)malloc(vlen + 1);
			_CHECK_MEM(temp);
			if(value && vlen)
				memcpy(temp, value, vlen);
			temp[vlen] = '\0';
			*out_decoded = _cgi_url_decode_dup(temp);
			free(temp);

			return *out_decoded != 0;
		}

		cursor = (*pair_end == '&') ? pair_end + 1 : 0;
	}

	return false;
}

static const char* _cgi_get_env(const char* key) {
	const char* result = 0;

	if(!key)
		return 0;

	result = getenv(key);

	return result ? result : "";
}

static bool_t _cgi_is_active(void) {
	const char* gateway = _cgi_get_env("GATEWAY_INTERFACE");
	const char* method = _cgi_get_env("REQUEST_METHOD");

	return (bool_t)((gateway && *gateway) || (method && *method));
}

static void _cgi_load_post_body_if_needed(void) {
	const char* method = 0;
	const char* length = 0;
	long body_len = 0;
	size_t bytes = 0;

	if(_cgi_post_body_loaded)
		return;

	_cgi_post_body_loaded = true;
	method = _cgi_get_env("REQUEST_METHOD");
	if(!method || mb_stricmp(method, "POST"))
		return;

	length = _cgi_get_env("CONTENT_LENGTH");
	if(!length || !*length)
		return;

	body_len = strtol(length, 0, 10);
	if(body_len <= 0 || body_len > 1024 * 1024)
		return;

	_cgi_post_body = (char*)malloc((size_t)body_len + 1);
	_CHECK_MEM(_cgi_post_body);
	bytes = fread(_cgi_post_body, 1, (size_t)body_len, stdin);
	_cgi_post_body[bytes] = '\0';
}

static void _cgi_send_headers_if_needed(void) {
	if(!_cgi_mode || _cgi_headers_sent)
		return;

	if(!_cgi_content_type[0])
		strcpy(_cgi_content_type, "text/html; charset=utf-8");
	if(!_cgi_status_text[0])
		strcpy(_cgi_status_text, "OK");

	printf("Status: %d %s\r\n", _cgi_status_code, _cgi_status_text);
	printf("Content-Type: %s\r\n", _cgi_content_type);
	printf("X-Powered-By: MY-BASIC CGI\r\n");
	printf("\r\n");
	fflush(stdout);
	_cgi_headers_sent = true;
}

static int cgi_mode(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));
	mb_check(mb_attempt_close_bracket(s, l));
	mb_check(mb_push_int(s, l, _cgi_mode ? 1 : 0));

	return result;
}

static int cgi_env(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;
	char* key = 0;
	const char* value = 0;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));
	mb_check(mb_pop_string(s, l, &key));
	mb_check(mb_attempt_close_bracket(s, l));

	value = _cgi_get_env(key);
	mb_check(mb_push_string(s, l, mb_memdup(value, (unsigned)(strlen(value) + 1))));

	return result;
}

static int cgi_set_content_type(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;
	char* value = 0;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));
	mb_check(mb_pop_string(s, l, &value));
	mb_check(mb_attempt_close_bracket(s, l));

	if(value && *value) {
		strncpy(_cgi_content_type, value, sizeof(_cgi_content_type) - 1);
		_cgi_content_type[sizeof(_cgi_content_type) - 1] = '\0';
	}

	return result;
}

static int cgi_status(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;
	int_t code = 200;
	char* text = 0;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));
	mb_check(mb_pop_int(s, l, &code));
	if(mb_has_arg(s, l))
		mb_check(mb_pop_string(s, l, &text));
	mb_check(mb_attempt_close_bracket(s, l));

	_cgi_status_code = (int)code;
	if(text && *text) {
		strncpy(_cgi_status_text, text, sizeof(_cgi_status_text) - 1);
		_cgi_status_text[sizeof(_cgi_status_text) - 1] = '\0';
	}

	return result;
}

static int cgi_query(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;
	char* key = 0;
	char* decoded = 0;
	const char* query = 0;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));
	mb_check(mb_pop_string(s, l, &key));
	mb_check(mb_attempt_close_bracket(s, l));

	if(!key) {
		mb_check(mb_push_string(s, l, mb_memdup("", 1)));
		return result;
	}

	_cgi_load_post_body_if_needed();
	if(_cgi_lookup_param(_cgi_post_body, key, &decoded)) {
		mb_check(mb_push_string(s, l, mb_memdup(decoded, (unsigned)(strlen(decoded) + 1))));
		free(decoded);
		return result;
	}

	query = _cgi_get_env("QUERY_STRING");
	if(_cgi_lookup_param(query, key, &decoded)) {
		mb_check(mb_push_string(s, l, mb_memdup(decoded, (unsigned)(strlen(decoded) + 1))));
		free(decoded);
		return result;
	}

	mb_check(mb_push_string(s, l, mb_memdup("", 1)));

	return result;
}

static int cgi_print(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;
	char* arg = 0;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));
	mb_check(mb_pop_string(s, l, &arg));
	mb_check(mb_attempt_close_bracket(s, l));

	_cgi_send_headers_if_needed();
	if(arg)
		fputs(arg, stdout);

	return result;
}

static int cgi_body(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));
	mb_check(mb_attempt_close_bracket(s, l));

	_cgi_load_post_body_if_needed();
	if(_cgi_post_body) {
		mb_check(mb_push_string(s, l, mb_memdup(_cgi_post_body, (unsigned)(strlen(_cgi_post_body) + 1))));
	} else {
		mb_check(mb_push_string(s, l, mb_memdup("", 1)));
	}

	return result;
}

static int cgi_json_escape(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;
	char* arg = 0;
	char* escaped = 0;
	char* out = 0;
	size_t n = 0;
	size_t i = 0;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));
	mb_check(mb_pop_string(s, l, &arg));
	mb_check(mb_attempt_close_bracket(s, l));

	if(!arg) {
		mb_check(mb_push_string(s, l, mb_memdup("", 1)));
		return result;
	}

	n = strlen(arg);
	escaped = (char*)malloc(n * 6 + 1);
	_CHECK_MEM(escaped);
	out = escaped;
	for(i = 0; i < n; ++i) {
		unsigned char c = (unsigned char)arg[i];
		switch(c) {
		case '\"': *out++ = '\\'; *out++ = '\"'; break;
		case '\\': *out++ = '\\'; *out++ = '\\'; break;
		case '\b': *out++ = '\\'; *out++ = 'b'; break;
		case '\f': *out++ = '\\'; *out++ = 'f'; break;
		case '\n': *out++ = '\\'; *out++ = 'n'; break;
		case '\r': *out++ = '\\'; *out++ = 'r'; break;
		case '\t': *out++ = '\\'; *out++ = 't'; break;
		default:
			if(c < 0x20) {
				sprintf(out, "\\u%04x", c);
				out += 6;
			} else {
				*out++ = (char)c;
			}
			break;
		}
	}
	*out = '\0';

	mb_check(mb_push_string(s, l, mb_memdup(escaped, (unsigned)(strlen(escaped) + 1))));
	free(escaped);

	return result;
}

static void _cgi_write_json_result(bool_t ok, const char* key, const char* value, int status, const char* status_text) {
	char* escaped = 0;

	if(value) {
		size_t n = strlen(value);
		char* out = 0;
		size_t i = 0;
		escaped = (char*)malloc(n * 6 + 1);
		_CHECK_MEM(escaped);
		out = escaped;
		for(i = 0; i < n; ++i) {
			unsigned char c = (unsigned char)value[i];
			switch(c) {
			case '\"': *out++ = '\\'; *out++ = '\"'; break;
			case '\\': *out++ = '\\'; *out++ = '\\'; break;
			case '\b': *out++ = '\\'; *out++ = 'b'; break;
			case '\f': *out++ = '\\'; *out++ = 'f'; break;
			case '\n': *out++ = '\\'; *out++ = 'n'; break;
			case '\r': *out++ = '\\'; *out++ = 'r'; break;
			case '\t': *out++ = '\\'; *out++ = 't'; break;
			default:
				if(c < 0x20) {
					sprintf(out, "\\u%04x", c);
					out += 6;
				} else {
					*out++ = (char)c;
				}
				break;
			}
		}
		*out = '\0';
	} else {
		escaped = (char*)malloc(1);
		_CHECK_MEM(escaped);
		escaped[0] = '\0';
	}

	strncpy(_cgi_content_type, "application/json; charset=utf-8", sizeof(_cgi_content_type) - 1);
	_cgi_content_type[sizeof(_cgi_content_type) - 1] = '\0';
	_cgi_status_code = status;
	strncpy(_cgi_status_text, status_text, sizeof(_cgi_status_text) - 1);
	_cgi_status_text[sizeof(_cgi_status_text) - 1] = '\0';

	_cgi_send_headers_if_needed();
	printf("{\"ok\":%s,\"%s\":\"%s\"}", ok ? "true" : "false", key, escaped);
	free(escaped);
}

static int c_json_ok(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;
	char* value = 0;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));
	mb_check(mb_pop_string(s, l, &value));
	mb_check(mb_attempt_close_bracket(s, l));

	_cgi_write_json_result(true, "message", value, 200, "OK");

	return result;
}

static int c_json_err(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;
	char* value = 0;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));
	mb_check(mb_pop_string(s, l, &value));
	mb_check(mb_attempt_close_bracket(s, l));

	_cgi_write_json_result(false, "error", value, 500, "Internal Server Error");

	return result;
}

#ifdef MB_ENABLE_SQLITE
static void _sqlite_set_error(const char* msg) {
	if(msg && *msg) {
		strncpy(_sqlite_last_error, msg, sizeof(_sqlite_last_error) - 1);
		_sqlite_last_error[sizeof(_sqlite_last_error) - 1] = '\0';
	} else {
		_sqlite_last_error[0] = '\0';
	}
}

static void* _sqlite_sym(const char* name) {
#ifdef MB_OS_WIN
	return (void*)GetProcAddress(_sqlite_lib, name);
#else /* MB_OS_WIN */
	return dlsym(_sqlite_lib, name);
#endif /* MB_OS_WIN */
}

static bool_t _sqlite_load_api(void) {
	if(_sqlite_api_loaded)
		return true;

#ifdef MB_OS_WIN
	_sqlite_lib = LoadLibraryA("sqlite3.dll");
#else /* MB_OS_WIN */
	_sqlite_lib = dlopen("libsqlite3.so", RTLD_NOW);
	if(!_sqlite_lib)
		_sqlite_lib = dlopen("libsqlite3.so.0", RTLD_NOW);
	if(!_sqlite_lib)
		_sqlite_lib = dlopen("libsqlite3.dylib", RTLD_NOW);
#endif /* MB_OS_WIN */
	if(!_sqlite_lib) {
		_sqlite_set_error("Cannot load sqlite3 dynamic library.");
		return false;
	}

	_sqlite3_open_fn = (int (*)(const char*, sqlite3**))_sqlite_sym("sqlite3_open");
	_sqlite3_close_fn = (int (*)(sqlite3*))_sqlite_sym("sqlite3_close");
	_sqlite3_errmsg_fn = (const char* (*)(sqlite3*))_sqlite_sym("sqlite3_errmsg");
	_sqlite3_exec_fn = (int (*)(sqlite3*, const char*, int (*)(void*, int, char**, char**), void*, char**))_sqlite_sym("sqlite3_exec");
	_sqlite3_free_fn = (void (*)(void*))_sqlite_sym("sqlite3_free");
	_sqlite3_changes_fn = (int (*)(sqlite3*))_sqlite_sym("sqlite3_changes");
	_sqlite3_prepare_v2_fn = (int (*)(sqlite3*, const char*, int, sqlite3_stmt**, const char**))_sqlite_sym("sqlite3_prepare_v2");
	_sqlite3_step_fn = (int (*)(sqlite3_stmt*))_sqlite_sym("sqlite3_step");
	_sqlite3_column_text_fn = (const unsigned char* (*)(sqlite3_stmt*, int))_sqlite_sym("sqlite3_column_text");
	_sqlite3_finalize_fn = (int (*)(sqlite3_stmt*))_sqlite_sym("sqlite3_finalize");

	if(!_sqlite3_open_fn || !_sqlite3_close_fn || !_sqlite3_errmsg_fn || !_sqlite3_exec_fn || !_sqlite3_free_fn ||
		!_sqlite3_changes_fn || !_sqlite3_prepare_v2_fn || !_sqlite3_step_fn || !_sqlite3_column_text_fn || !_sqlite3_finalize_fn) {
		_sqlite_set_error("Incomplete sqlite3 API symbols.");
		return false;
	}

	_sqlite_api_loaded = true;

	return true;
}

static int c_db_open(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;
	char* path = 0;
	int code = SQLITE_OK;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));
	mb_check(mb_pop_string(s, l, &path));
	mb_check(mb_attempt_close_bracket(s, l));

	if(!_sqlite_load_api()) {
		mb_check(mb_push_int(s, l, 0));
		return result;
	}

	if(_sqlite_conn) {
		_sqlite3_close_fn(_sqlite_conn);
		_sqlite_conn = 0;
	}
	_sqlite_set_error("");
	code = _sqlite3_open_fn(path, &_sqlite_conn);
	if(code != SQLITE_OK) {
		_sqlite_set_error(_sqlite3_errmsg_fn(_sqlite_conn));
		mb_check(mb_push_int(s, l, 0));
	} else {
		mb_check(mb_push_int(s, l, 1));
	}

	return result;
}

static int c_db_close(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));
	mb_check(mb_attempt_close_bracket(s, l));

	if(_sqlite_conn) {
		_sqlite3_close_fn(_sqlite_conn);
		_sqlite_conn = 0;
	}
	_sqlite_set_error("");
	mb_check(mb_push_int(s, l, 1));

	return result;
}

static int c_db_exec(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;
	char* sql = 0;
	char* err = 0;
	int code = SQLITE_OK;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));
	mb_check(mb_pop_string(s, l, &sql));
	mb_check(mb_attempt_close_bracket(s, l));

	if(!_sqlite_load_api()) {
		mb_check(mb_push_int(s, l, -1));
		return result;
	}

	if(!_sqlite_conn) {
		_sqlite_set_error("Database is not opened.");
		mb_check(mb_push_int(s, l, -1));
		return result;
	}

	_sqlite_set_error("");
	code = _sqlite3_exec_fn(_sqlite_conn, sql, 0, 0, &err);
	if(code != SQLITE_OK) {
		_sqlite_set_error(err ? err : _sqlite3_errmsg_fn(_sqlite_conn));
		if(err)
			_sqlite3_free_fn(err);
		mb_check(mb_push_int(s, l, -1));
	} else {
		mb_check(mb_push_int(s, l, _sqlite3_changes_fn(_sqlite_conn)));
	}

	return result;
}

static int c_db_query(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;
	char* sql = 0;
	sqlite3_stmt* stmt = 0;
	int code = SQLITE_OK;
	const unsigned char* txt = 0;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));
	mb_check(mb_pop_string(s, l, &sql));
	mb_check(mb_attempt_close_bracket(s, l));

	if(!_sqlite_load_api()) {
		mb_check(mb_push_string(s, l, mb_memdup("", 1)));
		return result;
	}

	if(!_sqlite_conn) {
		_sqlite_set_error("Database is not opened.");
		mb_check(mb_push_string(s, l, mb_memdup("", 1)));
		return result;
	}

	_sqlite_set_error("");
	code = _sqlite3_prepare_v2_fn(_sqlite_conn, sql, -1, &stmt, 0);
	if(code != SQLITE_OK) {
		_sqlite_set_error(_sqlite3_errmsg_fn(_sqlite_conn));
		mb_check(mb_push_string(s, l, mb_memdup("", 1)));
		return result;
	}

	code = _sqlite3_step_fn(stmt);
	if(code == SQLITE_ROW) {
		txt = _sqlite3_column_text_fn(stmt, 0);
		if(txt) {
			mb_check(mb_push_string(s, l, mb_memdup((const char*)txt, (unsigned)(strlen((const char*)txt) + 1))));
		} else {
			mb_check(mb_push_string(s, l, mb_memdup("", 1)));
		}
	} else if(code == SQLITE_DONE) {
		mb_check(mb_push_string(s, l, mb_memdup("", 1)));
	} else {
		_sqlite_set_error(_sqlite3_errmsg_fn(_sqlite_conn));
		mb_check(mb_push_string(s, l, mb_memdup("", 1)));
	}

	_sqlite3_finalize_fn(stmt);

	return result;
}

static int c_db_error(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));
	mb_check(mb_attempt_close_bracket(s, l));

	mb_check(mb_push_string(s, l, mb_memdup(_sqlite_last_error, (unsigned)(strlen(_sqlite_last_error) + 1))));

	return result;
}
#endif /* MB_ENABLE_SQLITE */

static int sys(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;
	char* arg = 0;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));

	mb_check(mb_pop_string(s, l, &arg));

	mb_check(mb_attempt_close_bracket(s, l));

	if(arg)
		system(arg);

	return result;
}

static int trace(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;
	char* frames[16];
	int i = 0;

	mb_assert(s && l);

	memset(frames, 0, sizeof(frames));

	mb_check(mb_attempt_open_bracket(s, l));

	mb_check(mb_attempt_close_bracket(s, l));

	mb_check(mb_debug_get_stack_trace(s, frames, countof(frames)));

	for(i = 1; i < countof(frames); ) {
		if(frames[i]) {
			_printf("%s", frames[i]);
		}
		if(++i < countof(frames) && frames[i]) {
			_printf(" <- ");
		}
	}

	return result;
}

static int raise(struct mb_interpreter_t* s, void** l) {
	int result = MB_EXTENDED_ABORT;
	int_t err = 0;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));

	if(mb_has_arg(s, l)) {
		mb_check(mb_pop_int(s, l, &err));
	}

	mb_check(mb_attempt_close_bracket(s, l));

	mb_check(mb_push_int(s, l, err));

	result = mb_raise_error(s, l, SE_EA_EXTENDED_ABORT, MB_EXTENDED_ABORT + err);

	return result;
}

static int gc(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;
	int_t collected = 0;

	mb_assert(s && l);

	mb_check(mb_attempt_open_bracket(s, l));

	mb_check(mb_attempt_close_bracket(s, l));

	mb_check(mb_gc(s, &collected));

	mb_check(mb_push_int(s, l, collected));

	return result;
}

static int beep(struct mb_interpreter_t* s, void** l) {
	int result = MB_FUNC_OK;

	mb_assert(s && l);

	mb_check(mb_attempt_func_begin(s, l));

	mb_check(mb_attempt_func_end(s, l));

	putchar('\a');

	return result;
}

/* ========================================================} */

/*
** {========================================================
** Callbacks and handlers
*/

#if defined MB_CP_VC && defined MB_ENABLE_UNICODE && !MB_UNICODE_NEED_CONVERTING
static int _on_input(struct mb_interpreter_t* s, const char* pmt, char* buf, int n) {
	int result = 0;
	mb_unrefvar(s);
	mb_unrefvar(pmt);

	if(buf && n) {
		int wlen = n;
		int save = _setmode(_fileno(stdin), _O_U16TEXT);
		wchar_t* wstr = malloc(wlen * sizeof(wchar_t));
		if(fgetws(wstr, wlen, stdin) == 0) {
			_setmode(_fileno(stdin), save);

			free(wstr);

			fprintf(stderr, "Error reading.\n");

			exit(1);
		}
		int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, 0, 0, 0, 0);
		if(!len) {
			_setmode(_fileno(stdin), save);

			free(wstr);

			fprintf(stderr, "Error reading.\n");

			exit(1);
		}
		WideCharToMultiByte(CP_UTF8, 0, wstr, -1, buf, n, 0, 0);
		free(wstr);
		_setmode(_fileno(stdin), save);
		result = len - 1;
		if(buf[result - 1] == '\n') {
			buf[result - 1] = '\0';
			result--;
		}
	}

	return result;
}
#endif /* MB_CP_VC && MB_ENABLE_UNICODE && !MB_UNICODE_NEED_CONVERTING */

static int _on_prev_stepped(struct mb_interpreter_t* s, void** l, const char* f, int p, unsigned short row, unsigned short col) {
	mb_unrefvar(s);
	mb_unrefvar(l);
	mb_unrefvar(f);
	mb_unrefvar(p);
	mb_unrefvar(row);
	mb_unrefvar(col);

	return MB_FUNC_OK;
}

static int _on_post_stepped(struct mb_interpreter_t* s, void** l, const char* f, int p, unsigned short row, unsigned short col) {
	mb_unrefvar(s);
	mb_unrefvar(l);
	mb_unrefvar(f);
	mb_unrefvar(p);
	mb_unrefvar(row);
	mb_unrefvar(col);

	return MB_FUNC_OK;
}

static void _on_error(struct mb_interpreter_t* s, mb_error_e e, const char* m, const char* f, int p, unsigned short row, unsigned short col, int abort_code) {
	const char* type = abort_code == MB_FUNC_WARNING ? "Warning" : "Error";
	mb_unrefvar(s);
	mb_unrefvar(p);

	if(e == SE_NO_ERR)
		return;

	if(f) {
		if(e == SE_RN_REACHED_TO_WRONG_FUNCTION) {
			_printf(
				"%s:\n    Ln %d, Col %d in Func: %s\n    Code %d, Abort Code %d\n    Message: %s.\n",
				type, row, col, f,
				e, abort_code,
				m
			);
		} else {
			_printf(
				"%s:\n    Ln %d, Col %d in File: %s\n    Code %d, Abort Code %d\n    Message: %s.\n",
				type, row, col, f,
				e, e == SE_EA_EXTENDED_ABORT ? abort_code - MB_EXTENDED_ABORT : abort_code,
				m
			);
		}
	} else {
		_printf(
			"%s:\n    Ln %d, Col %d\n    Code %d, Abort Code %d\n    Message: %s.\n",
			type, row, col,
			e, e == SE_EA_EXTENDED_ABORT ? abort_code - MB_EXTENDED_ABORT : abort_code,
			m
		);
	}
}

static int _on_import(struct mb_interpreter_t* s, const char* p) {
	if(!importing_dirs)
		return MB_FUNC_ERR;

	if(!_try_import(s, p))
		return MB_FUNC_ERR;

	return MB_FUNC_OK;
}

/* ========================================================} */

/*
** {========================================================
** Initialization and disposing
*/

static void _on_startup(void) {
#if _USE_MEM_POOL
	_open_mem_pool();

	mb_set_memory_manager(_pop_mem, _push_mem);
#endif /* _USE_MEM_POOL */

	_create_code();

#ifdef _HAS_TICKS
	srand((unsigned)_ticks());
#endif /* _HAS_TICKS */

#if defined MB_CP_VC && defined MB_ENABLE_UNICODE && !MB_UNICODE_NEED_CONVERTING
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
#endif /* MB_CP_VC && MB_ENABLE_UNICODE && !MB_UNICODE_NEED_CONVERTING */

	setlocale(LC_ALL, "");
	setlocale(LC_CTYPE, "C");
	setlocale(LC_NUMERIC, "C");
	setlocale(LC_TIME, "C");

	mb_init();

	mb_open(&bas);
	_cgi_mode = _cgi_is_active();
	_cgi_headers_sent = false;
	_cgi_status_code = 200;
	_cgi_status_text[0] = '\0';
	_cgi_content_type[0] = '\0';
	_cgi_post_body_loaded = false;
	_cgi_post_body = 0;

#if defined MB_CP_VC && defined MB_ENABLE_UNICODE && !MB_UNICODE_NEED_CONVERTING
	mb_set_inputer(bas, _on_input);
#endif /* MB_CP_VC && MB_ENABLE_UNICODE && !MB_UNICODE_NEED_CONVERTING */
	mb_debug_set_stepped_handler(bas, _on_prev_stepped, _on_post_stepped);
	mb_set_error_handler(bas, _on_error);
	mb_set_import_handler(bas, _on_import);

#ifdef _HAS_TICKS
	mb_reg_fun(bas, ticks);
#endif /* _HAS_TICKS */
	mb_reg_fun(bas, now);
	mb_reg_fun(bas, os);
	mb_reg_fun(bas, cgi_mode);
	mb_reg_fun(bas, cgi_env);
	mb_reg_fun(bas, cgi_set_content_type);
	mb_reg_fun(bas, cgi_status);
	mb_reg_fun(bas, cgi_query);
	mb_reg_fun(bas, cgi_print);
	mb_reg_fun(bas, cgi_body);
	mb_reg_fun(bas, cgi_json_escape);
	mb_reg_fun(bas, c_json_ok);
	mb_reg_fun(bas, c_json_err);
#ifdef MB_ENABLE_SQLITE
	_sqlite_last_error[0] = '\0';
	mb_reg_fun(bas, c_db_open);
	mb_reg_fun(bas, c_db_close);
	mb_reg_fun(bas, c_db_exec);
	mb_reg_fun(bas, c_db_query);
	mb_reg_fun(bas, c_db_error);
#endif /* MB_ENABLE_SQLITE */
	mb_reg_fun(bas, sys);
	mb_reg_fun(bas, trace);
	mb_reg_fun(bas, raise);
	mb_reg_fun(bas, gc);
	mb_reg_fun(bas, beep);

	{
		const char* shortcuts[] = {
			"def GET(k) return cgi_query(k); enddef\n",
			"def POST(k) return cgi_query(k); enddef\n",
			"def PARAM(k) return cgi_query(k); enddef\n",
			"def METHOD() return cgi_env(\"REQUEST_METHOD\"); enddef\n",
			"def BODY() return cgi_body(); enddef\n",
			"def ECHO(v) cgi_print(v); enddef\n",
			"def ECHOLN(v) cgi_print(v + \"\\n\"); enddef\n",
			"def CONTENT_TYPE(v) cgi_set_content_type(v); enddef\n",
			"def STATUS(c, t) cgi_status(c, t); enddef\n",
			"def HTML_PAGE(t, b) CONTENT_TYPE(\"text/html; charset=utf-8\"): ECHO(\"<!doctype html><html><head><meta charset=utf-8><title>\" + t + \"</title></head><body>\" + b + \"</body></html>\"); enddef\n",
			"def JSON_OK(v) c_json_ok(v); enddef\n",
			"def JSON_ERR(v) c_json_err(v); enddef\n",
#ifdef MB_ENABLE_SQLITE
			"def DB_OPEN(path) return c_db_open(path); enddef\n",
			"def DB_CLOSE() return c_db_close(); enddef\n",
			"def DB_EXEC(sql) return c_db_exec(sql); enddef\n",
			"def DB_QUERY(sql) return c_db_query(sql); enddef\n",
			"def DB_ERROR() return c_db_error(); enddef\n"
#endif /* MB_ENABLE_SQLITE */
		};
		int i = 0;
		for(i = 0; i < countof(shortcuts); ++i) {
			if(mb_load_string(bas, (char*)shortcuts[i], true) != MB_FUNC_OK) {
				_printf("Failed to load web helper shortcut %d.\n", i + 1);
				break;
			}
		}
	}
}

static void _on_exit(void) {
	_destroy_importing_directories();
	if(_cgi_post_body) {
		free(_cgi_post_body);
		_cgi_post_body = 0;
		_cgi_post_body_loaded = false;
	}
#ifdef MB_ENABLE_SQLITE
	if(_sqlite_conn) {
		if(_sqlite3_close_fn)
			_sqlite3_close_fn(_sqlite_conn);
		_sqlite_conn = 0;
	}
#ifdef MB_OS_WIN
	if(_sqlite_lib) {
		FreeLibrary(_sqlite_lib);
		_sqlite_lib = 0;
	}
#else /* MB_OS_WIN */
	if(_sqlite_lib) {
		dlclose(_sqlite_lib);
		_sqlite_lib = 0;
	}
#endif /* MB_OS_WIN */
#endif /* MB_ENABLE_SQLITE */

	if(bas)
		mb_close(&bas);

	mb_dispose();

	_destroy_code();

#if _USE_MEM_POOL
	_close_mem_pool();
#endif /* _USE_MEM_POOL */

#ifdef MB_CP_VC
	if(!!_CrtDumpMemoryLeaks()) { _CrtDbgBreak(); }
#elif _USE_MEM_POOL
	if(alloc_count > 0 || alloc_bytes > 0) { mb_assert(0 && "Memory leak."); }
#endif /* MB_CP_VC */
}

/* ========================================================} */

/*
** {========================================================
** Entry
*/

int main(int argc, char* argv[]) {
	int status = 0;

#ifdef MB_CP_VC
	_CrtSetBreakAlloc(0);
#endif /* MB_CP_VC */

	atexit(_on_exit);

	if(setjmp(mem_failure_point)) {
		_printf("Error: out of memory.\n");

		exit(1);
	}

	_on_startup();

	if(argc >= 2) {
		if(!_process_parameters(argc, argv))
			argc = 1;
	}
	if(argc == 1) {
		_show_tip();
		do {
			status = _do_line();
		} while(_NOT_FINISHED(status));
	}

	return 0;
}

/* ========================================================} */

#ifdef __cplusplus
}
#endif /* __cplusplus */
