/* Copyright (C) 2026 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "precompiled.h"
#include "lib/sysdep/filesystem.h"

#include "lib/debug.h"
#include "lib/sysdep/os/win/wutil.h"	// StatusFromWin
#include "lib/sysdep/os/win/wposix/waio.h"	// waio_reopen
#include "lib/sysdep/os/win/wposix/crt_posix.h"			// _close, _lseeki64 etc.

#include <atomic>

//-----------------------------------------------------------------------------
// dirent.h
//-----------------------------------------------------------------------------

static const long _1e7 = 10000000;

// hectonanoseconds between Windows and POSIX epoch
static const u64 posix_epoch_hns = 0x019DB1DED53E8000;

// this function avoids the pitfall of casting FILETIME* to u64*,
// which is not safe due to differing alignment guarantees!
// on some platforms, that would result in an exception.
static u64 u64_from_FILETIME(const FILETIME* ft)
{
    return u64_from_u32(ft->dwHighDateTime, ft->dwLowDateTime);
}

// convert UTC FILETIME to seconds-since-1970 UTC:
// we just have to subtract POSIX epoch and scale down to units of seconds.
//
// note: RtlTimeToSecondsSince1970 isn't officially documented,
// so don't use that.
static time_t wtime_utc_filetime_to_time_t(FILETIME* ft)
{
    u64 hns = u64_from_FILETIME(ft);
    u64 s = (hns - posix_epoch_hns) / _1e7;
    return static_cast<time_t>(s & 0xFFFFFFFF);
}

static bool IsValidDirectory(const OsPath& path)
{
	const DWORD fileAttributes = GetFileAttributesW(OsString(path).c_str());

	// path not found
	if(fileAttributes == INVALID_FILE_ATTRIBUTES)
		return false;

	// not a directory
	if((fileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		return false;

	// NB: no longer reject hidden or system attributes since
	// wsnd's add_oal_dlls_in_dir opens the Windows system directory,
	// which sometimes has these attributes set.

	return true;
}


//-----------------------------------------------------------------------------
// fcntl.h
//-----------------------------------------------------------------------------

int wopen(const OsPath& pathname, int oflag)
{
	ENSURE(!(oflag & O_CREAT));	// must specify mode_arg if O_CREAT
	return wopen(OsString(pathname), oflag, _S_IREAD|_S_IWRITE);
}


int wopen(const OsPath& pathname, int oflag, mode_t mode)
{
	if(oflag & O_DIRECT)
	{
		Status ret = waio_open(pathname, oflag);
		if(ret < 0)
		{
			errno = ErrnoFromStatus(ret);
			return -1;
		}
		return (int)ret;	// file descriptor
	}
	else
	{
		WinScopedPreserveLastError s;	// _wsopen_s's CreateFileW
		int fd;
		oflag |= _O_BINARY;
		if(oflag & O_WRONLY)
			oflag |= O_CREAT|O_TRUNC;
		// NB: _wsopen_s ignores mode unless oflag & O_CREAT
		errno_t ret = _wsopen_s(&fd, OsString(pathname).c_str(), oflag, _SH_DENYRD, mode);
		if(ret != 0)
		{
			errno = ret;
			return -1;	// NOWARN
		}
		return fd;
	}
}


int wclose(int fd)
{
	ENSURE(fd >= 3);	// not invalid nor stdin/out/err

	if(waio_close(fd) != 0)
		return _close(fd);
	return 0;
}


static int ErrnoFromCreateDirectory()
{
	switch(GetLastError())
	{
	case ERROR_ALREADY_EXISTS:
		return EEXIST;
	case ERROR_PATH_NOT_FOUND:
		return ENOENT;
	case ERROR_ACCESS_DENIED:
		return EACCES;
	case ERROR_WRITE_PROTECT:
		return EROFS;
	case ERROR_DIRECTORY:
		return ENOTDIR;
	default:
		return 0;
	}
}

int wmkdir(const OsPath& path, mode_t)
{
	if(!CreateDirectoryW(OsString(path).c_str(), (LPSECURITY_ATTRIBUTES)NULL))
	{
		errno = ErrnoFromCreateDirectory();
		return -1;
	}

	return 0;
}
