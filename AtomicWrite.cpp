#include <utility>

#include "AtomicWrite.h"

#if defined(__linux__)
	#include <stdio.h>
	#include <string.h>
	#include <stdlib.h>
#endif
#if defined(__APPLE__)
	// We include unistd here so it is not affected by the redefinition of
	// fsync below
	#include <unistd.h>
	// fsync on OS X doesn't actually write to disk, so we need the fnctl
	// instead, which actually does what we want.
	#define fsync(fd) fcntl(fd, F_FULLFSYNC)
#endif
#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__) || defined(__APPLE__)
	#include <errno.h>
	#include <fcntl.h>
	#include <libgen.h>
	#include <unistd.h>

	static void
	sync(int fd)
	{
		int err = fsync(fd);
		if (err) throw AtomicWrite::FailedAtomicWrite();
	}

	static void
	write_out(int fd, std::string data)
	{
		size_t bytes_remaining = data.length();
		const char * raw_data = data.c_str();
		while (bytes_remaining > 0) {
			int bytes_written = write(fd, raw_data, bytes_remaining);
			if (bytes_written == -1) throw AtomicWrite::FailedAtomicWrite();
			bytes_remaining -= bytes_written;
			raw_data += bytes_written;
		}
	}

	static void
	close_file(int fd)
	{
		int err = close(fd);
		if (err) throw AtomicWrite::FailedAtomicWrite();
	}

	static std::pair<int, std::string>
	make_temporary_file(std::string fname)
	{
		fname.append(".XXXXXX");
		size_t size = fname.size();
		char *tmp = new char[size + 1];
		memcpy(tmp, fname.c_str(), size + 1);
		int fd = mkstemp(tmp);
		if (fd == -1) {
			delete[] tmp;
			throw AtomicWrite::FailedAtomicWrite();
		}
		std::string tmpname(tmp);
		delete[] tmp;
		return std::make_pair(fd, tmpname);
	}

	static void
	rename_file(std::string oldname, std::string newname)
	{
		int err = rename(oldname.c_str(), newname.c_str());
		if (err) throw AtomicWrite::FailedAtomicWrite();
	}

	static int
	parent(std::string fname)
	{
		size_t size = fname.size();
		char *tmp = new char[size + 1];
		memcpy(tmp, fname.c_str(), size + 1);
		char *parent = dirname(tmp);
		delete[] tmp;
		if (!parent) throw AtomicWrite::FailedAtomicWrite();
		int fd = open(parent, O_RDONLY);
		if (fd == -1) {
			throw AtomicWrite::FailedAtomicWrite();
		}
		return fd;
	}

#elif defined(_WIN32)
	#include <Windows.h>
	#include <io.h>
	#include <fcntl.h>
	#include <sys/stat.h>

	static void
	sync(int fd)
	{
		int err = _commit(fd);
		if (err) throw AtomicWrite::FailedAtomicWrite();
	}

	static void
	write_out(int fd, std::string data)
	{
		size_t bytes_remaining = data.length();
		const char * raw_data = data.c_str();
		while (bytes_remaining > 0) {
			int bytes_written = _write(fd, raw_data, bytes_remaining);
			if (bytes_written == -1) throw AtomicWrite::FailedAtomicWrite();
			bytes_remaining -= bytes_written;
			raw_data += bytes_written;
		}
	}

	static void
	close_file(int fd)
	{
		int err = _close(fd);
		if (err) throw AtomicWrite::FailedAtomicWrite();
	}

	static std::pair<int, std::string>
	make_temporary_file(std::string fname)
	{
		fname.append("XXXXXX");
		size_t size = fname.size();
		char *tmp = new char[size + 1];
		memcpy(tmp, fname.c_str(), size + 1);
		int err = _mktemp_s(tmp, size + 1);
		if (err) {
			delete[] tmp;
			throw AtomicWrite::FailedAtomicWrite();
		}

		int fd;
		while (true) {
			err = _sopen_s(&fd, tmp, _O_RDWR | _O_CREAT, _SH_DENYNO, _S_IWRITE);
			if (fd != -1) {
				break;
			} else if (fd == -1 && errno != EEXIST) {
				delete[] tmp;
				throw AtomicWrite::FailedAtomicWrite();
			}
		}

		std::string tmpname(tmp);
		delete[] tmp;
		return std::make_pair(fd, tmpname);
	}

	static void
		rename_file(std::string oldname, std::string newname)
	{
		// ReplaceFile requires newname already exists, so we have to create it if it doesn't
		int fd;
		_sopen_s(&fd, newname.c_str(), _O_RDWR | _O_CREAT, _SH_DENYNO, _S_IWRITE);
		if (fd != -1) close_file(fd);

		bool success = ReplaceFile(newname.c_str(), oldname.c_str(), NULL, REPLACEFILE_IGNORE_MERGE_ERRORS, NULL, NULL);
		if (success == 0) throw AtomicWrite::FailedAtomicWrite();
	}

#else
	#error AtomicWrite does not support this platform
#endif

#if defined(_WIN32)
void
AtomicWrite::write(std::string fname, std::string data)
{
	std::pair<int, std::string> fd_and_tmpname = make_temporary_file(fname);
	int fd = fd_and_tmpname.first;
	std::string tmpname = fd_and_tmpname.second;

	write_out(fd, data);
	sync(fd);
	close_file(fd);
	rename_file(tmpname, fname);
	// Windows does not appear to need the parent directory flushed to disk
}
#else
void
AtomicWrite::write(std::string fname, std::string data)
{
	int parent_fd = parent(fname);
	std::pair<int, std::string> fd_and_tmpname = make_temporary_file(fname);
	int fd = fd_and_tmpname.first;
	std::string tmpname = fd_and_tmpname.second;

	write_out(fd, data);
	sync(fd);
	close_file(fd);
	rename_file(tmpname, fname);
	sync(parent_fd);
	close_file(parent_fd);
}
#endif

