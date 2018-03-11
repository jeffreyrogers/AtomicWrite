#include "AtomicWrite.h"

#include <utility>

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
		int err = write(fd, data.c_str(), data.length());
		if (err == -1) throw AtomicWrite::FailedAtomicWrite();
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
		int fd;
		size_t size = fname.size();
		char *tmp = new char[size + 1];
		memcpy(tmp, fname.c_str(), size + 1);
		while (true) {
			tmp = mktemp(tmp);
			fd = open(tmp, O_WRONLY | O_CREAT | O_EXCL);
			if (fd == -1 && errno != EEXIST) {
				delete[] tmp;
				throw AtomicWrite::FailedAtomicWrite();
			} else {
				break;
			}
		}
		std::string tmpname(tmp);
		delete[] tmp;
		return std::make_pair<int, std::string>(fd, tmpname);
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
	#include <io.h>

	static void
	sync(int fd)
	{
		int err = _commit(fd);
		if (err) throw AtomicWrite::FailedAtomicWrite();
	}

	static void
	write_out(int fd, std::string data)
	{
		int err = _write(fd, data.c_str(), data.length());
		if (err) throw AtomicWrite::FailedAtomicWrite();
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
		// TODO
	}

	// rename
	static void
	rename_file(std::string oldname, std::string newname)
	{
		// TODO
	}

	static int
	parent(std::string fname)
	{
		// TODO
	}

#else
	#error AtomicWrite does not support this platform
#endif

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
