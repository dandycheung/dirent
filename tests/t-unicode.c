/*
 * Test program to try unicode file names.
 *
 * Copyright (C) 1998-2019 Toni Ronkko
 * This file is part of dirent.  Dirent may be freely distributed
 * under the MIT license.  For all details and documentation, see
 * https://github.com/tronkko/dirent
 */

/* Silence warning about fopen being insecure */
#define _CRT_SECURE_NO_WARNINGS

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <time.h>
#include <locale.h>

#undef NDEBUG
#include <assert.h>

static void test_wcs(void);
static void test_mbs(void);
static void test_utf8(void);
static void initialize(void);
static void cleanup(void);

#ifdef WIN32
static wchar_t wpath[MAX_PATH + 1];
static char path[MAX_PATH + 1];
#endif

int
main(void)
{
	initialize();

	test_wcs();
	test_mbs();

	cleanup();
	return EXIT_SUCCESS;
}

static void
test_wcs(void)
{
#ifdef WIN32
	/* Open directory stream using wide-character string */
	_WDIR *wdir = _wopendir(wpath);
	if (wdir == NULL) {
		wprintf(L"Cannot open directory %ls\n", wpath);
		abort();
	}

	/* Compute length of directory name */
	size_t k = wcslen(wpath);

	/* Read through entries */
	int counter = 0;
	struct _wdirent *wentry;
	while ((wentry = _wreaddir(wdir)) != NULL) {
		/* Skip pseudo directories */
		if (wcscmp(wentry->d_name, L".") == 0) {
			continue;
		}
		if (wcscmp(wentry->d_name, L"..") == 0) {
			continue;
		}

		/* Found a file */
		counter++;
		assert(wentry->d_type == DT_REG);

		/* Modify wpath by appending file name to it */
		size_t i = k;
		assert(i < MAX_PATH);
		wpath[i++] = '\\';
		size_t x = 0;
		while (wentry->d_name[x] != '\0') {
			assert(i < MAX_PATH);
			wpath[i++] = wentry->d_name[x++];
		}
		assert(i < MAX_PATH);
		wpath[i] = '\0';

		/* Open file for read */
		HANDLE fh = CreateFileW(
			wpath,
			/* Access */ GENERIC_READ,
			/* Share mode */ 0,
			/* Security attributes */ NULL,
			/* Creation disposition */ OPEN_EXISTING,
			/* Attributes */ FILE_ATTRIBUTE_NORMAL,
			/* Template files */ NULL
		);
		assert(fh != INVALID_HANDLE_VALUE);

		/* Read data from file */
		char buffer[100];
		DWORD n;
		BOOL ok = ReadFile(
			/* File handle */ fh,
			/* Output buffer */ buffer,
			/* Max number of bytes to read */ sizeof(buffer) - 1,
			/* Number of bytes actually read */ &n,
			/* Overlapped */ NULL
		);
		assert(ok);

		/* Make sure that we got the file contents right */
		assert(n == 4);
		assert(buffer[0] == 'h');
		assert(buffer[1] == 'e');
		assert(buffer[2] == 'p');
		assert(buffer[3] == '\n');

		/* Close file */
		ok = CloseHandle(fh);
		assert(ok);
	}

	/* The directory only has one file */
	assert(counter == 1);

	/* Close directory */
	_wclosedir(wdir);

	/* Restore the original path name */
	wpath[k] = '\0';
#endif
}

static void
test_mbs(void)
{
#ifdef WIN32
	/* Open directory stream using multi-byte character string */
	DIR *dir = opendir(path);
	if (dir == NULL) {
		fprintf(stderr, "Cannot open directory %s\n", path);
		abort();
	}

	/* Compute length of directory name */
	size_t k = strlen(path);

	/* Read through entries */
	int counter = 0;
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		/* Skip pseudo directories */
		if (strcmp(entry->d_name, ".") == 0) {
			continue;
		}
		if (strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		/* Found a file */
		counter++;
		assert(entry->d_type == DT_REG);

		/* Modify path by appending file name to it */
		size_t j = k;
		assert(j < MAX_PATH);
		path[j++] = '\\';
		size_t x = 0;
		while (entry->d_name[x] != '\0') {
			assert(j < MAX_PATH);
			path[j++] = entry->d_name[x++];
		}
		assert(j < MAX_PATH);
		path[j] = '\0';

		/* Open file for read */
		FILE *fp = fopen(path, "r");
		if (!fp) {
			fprintf(stderr, "Cannot open file %s\n", path);
			abort();
		}

		/* Read data from file */
		char buffer[100];
		if (fgets(buffer, sizeof(buffer), fp) == NULL) {
			fprintf(stderr, "Cannot read file %s\n", path);
			abort();
		}

		/* Make sure that we got the file contents right */
		assert(buffer[0] == 'h');
		assert(buffer[1] == 'e');
		assert(buffer[2] == 'p');
		assert(buffer[3] == '\n');
		assert(buffer[4] == '\0');

		/* Close file */
		fclose(fp);
	}

	/* The directory has only one file */
	assert(counter == 1);

	/* Close directory */
	closedir(dir);

	/* Restore the original path */
	path[k] = '\0';
#endif
}

static void
test_utf8(void)
{
#ifdef WIN32
	/* Compute length of directory name */
	size_t k = strlen(path);

	/* Append UTF-8 file name (åäö.txt) to path */
	size_t j = k;
	path[j++] = '\\';
	path[j++] = 0xc3;
	path[j++] = 0xa5;
	path[j++] = 0xc3;
	path[j++] = 0xa4;
	path[j++] = 0xc3;
	path[j++] = 0xb6;
	path[j++] = 0x2e;
	path[j++] = 0x74;
	path[j++] = 0x78;
	path[j++] = 0x74;
	assert(j < MAX_PATH);
	path[j] = '\0';

	/*
	 * Create file using UTF-8 file name.
	 *
	 * Be ware that the code below creates a different file name depending
	 * on the current locale!  For example, if the current locale is
	 * english_us.65001, then the file name will be "åäö.txt" (7
	 * characters).  However, if the current locale is english_us.1252,
	 * then the file name will be "ÃċÃĊÃ¶.txt" (10 characters).
	 */
	printf("Creating %s\n", path);
	FILE *fp = fopen(path, "w");
	if (!fp) {
		fprintf(stderr, "Cannot open file %s\n", path);
		abort();
	}
	fputs("hep\n", fp);
	fclose(fp);

	/* Restore original path */
	path[k] = '\0';

	/* Open directory stream */
	DIR *dir = opendir(path);
	if (dir == NULL) {
		fprintf(stderr, "Cannot open directory %s\n", path);
		abort();
	}

	/* Read through entries */
	int counter = 0;
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		/* Skip pseudo directories */
		if (strcmp(entry->d_name, ".") == 0) {
			continue;
		}
		if (strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		/* Found a file */
		counter++;
		assert(entry->d_type == DT_REG);

		/* Modify path by appending file name to it */
		j = k;
		assert(j < MAX_PATH);
		path[j++] = '\\';
		size_t x = 0;
		while (entry->d_name[x] != '\0') {
			assert(j < MAX_PATH);
			path[j++] = entry->d_name[x++];
		}
		assert(j < MAX_PATH);
		path[j] = '\0';

		/* Print file name for debugging */
		printf("Opening \"%s\" hex ", path + k + 1);
		x = 0;
		while (entry->d_name[x] != '\0') {
			printf("0x%02x ",
				(unsigned) (entry->d_name[x++] & 0xff));
		}
		printf("\n");

		/* Open file for read with UTF-8 file name */
		fp = fopen(path, "r");
		if (!fp) {
			fprintf(stderr, "Cannot open file %s\n", path);
			abort();
		}

		/* Read data from file */
		char buffer[100];
		if (fgets(buffer, sizeof(buffer), fp) == NULL) {
			fprintf(stderr, "Cannot read file %s\n", path);
			abort();
		}

		/* Make sure that we got the file contents right */
		assert(buffer[0] == 'h');
		assert(buffer[1] == 'e');
		assert(buffer[2] == 'p');
		assert(buffer[3] == '\n');
		assert(buffer[4] == '\0');

		/* Close file */
		fclose(fp);
	}

	/* We now had two files with identical contents */
	assert(counter == 2);

	/* Close directory */
	closedir(dir);
#endif
}

static void
initialize(void)
{
#ifdef WIN32
	/*
	 * Select UTF-8 locale.  This will change the way how C runtime
	 * functions such as fopen() and mkdir() handle character strings.
	 * For more information, please see:
	 * https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/setlocale-wsetlocale?view=msvc-160#utf-8-support
	 */
	setlocale(LC_ALL, "LC_CTYPE=.utf8");

	/* Initialize random number generator */
	srand(((int) time(NULL)) * 257 + ((int) GetCurrentProcessId()));

	/* Get path to temporary directory (wide-character and ascii) */
	size_t i = GetTempPathW(MAX_PATH, wpath);
	assert(i > 0);
	size_t j = GetTempPathA(MAX_PATH, path);
	assert(j > 0);

	/* Append random directory name to both paths */
	size_t k;
	for (k = 0; k < 10; k++) {
		/* Generate random character */
		char c = "abcdefghijklmnopqrstuvwxyz"[rand() % 26];

		/* Append character to paths */
		assert(i < MAX_PATH  &&  j < MAX_PATH);
		wpath[i++] = c;
		path[j++] = c;
	}

	/* Terminate paths */
	assert(i < MAX_PATH  &&  j < MAX_PATH);
	wpath[i] = '\0';
	path[j] = '\0';

	/* Remember the end of directory name */
	k = i;

	/* Create directory using unicode */
	BOOL ok = CreateDirectoryW(wpath, NULL);
	if (!ok) {
		DWORD e = GetLastError();
		wprintf(L"Cannot create directory %ls (code %u)\n", wpath, e);
		abort();
	}

	/* Overwrite zero terminator with path separator */
	assert(i < MAX_PATH && j < MAX_PATH);
	wpath[i++] = '\\';

	/* Append a few unicode characters */
	assert(i < MAX_PATH);
	wpath[i++] = 0x6d4b;
	assert(i < MAX_PATH);
	wpath[i++] = 0x8bd5;

	/* Terminate string */
	assert(i < MAX_PATH);
	wpath[i] = '\0';

	/* Create file with unicode name */
	HANDLE fh = CreateFileW(
		wpath,
		/* Access */ GENERIC_READ | GENERIC_WRITE,
		/* Share mode */ 0,
		/* Security attributes */ NULL,
		/* Creation disposition */ CREATE_NEW,
		/* Attributes */ FILE_ATTRIBUTE_NORMAL,
		/* Template files */ NULL
	);
	assert(fh != INVALID_HANDLE_VALUE);

	/* Write some data to file */
	ok = WriteFile(
		/* File handle */ fh,
		/* Pointer to data */ "hep\n",
		/* Number of bytes to write */ 4,
		/* Number of bytes written */ NULL,
		/* Overlapped */ NULL
	);
	assert(ok);

	/* Close file */
	ok = CloseHandle(fh);
	assert(ok);

	/* Zero terminate wide-character path */
	wpath[k] = '\0';
#else
	/* This test is not available under UNIX/Linux */
	fprintf(stderr, "Skipped\n");
	exit(/*Skip*/ 77);
#endif
}

static void
cleanup(void)
{
	printf("OK\n");
}
