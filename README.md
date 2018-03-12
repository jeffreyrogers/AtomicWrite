# AtomicWrite: cross platform library for atomically writing data to disk

Most operating systems have a file durability problem: It's possible for a
program to crash after writing to a file and have the file be irreparably
corrupted. AtomicWrite tries to solve this in a simple and portable way by
providing a simple API for writing to disk atomically. If the write call
succeeds your file has been written to disk. If the write call fails the data
that was previously in the file is still in the file.

## Usage

    #include "AtomicWrite.h"

    std::string data = "whatever you're saving to disk";
    std::string fname = "file.txt";
    AtomicWrite::write(fname, data);

AtomicWrite consists of a header file and a C++ file. All you have to do to use
it is drop these into your application. The write function will throw an
exception if an irrecoverable error is encountered.

## Supported Platforms

| OS		| Supported	|
| ------------- | ------------- |
| Linux		| Yes		|
| OS X		| Yes		|
| Windows	| Yes		|
| OpenBSD	| Untested	|
| FreeBSD	| Untested	|
| NetBSD	| Untested	|
| Dragonfly BSD	| Untested	|

The BSD code is the same as Linux, so it should work in theory, but I have not
actually run it to see. Some headers may need to be added to get it to compile
properly.

## Description of Implementation

The source code is pretty simple. Read it if you want to fully understand what's
going on. But the basic overview is as follows and is the same on every OS.

1. Create a temporary file
2. Write all data to the temporary file
3. Flush changes to disk
4. Rename the temporary file (this step is atomic at the filesystem level)
5. Flush changes to directory entry to disk (not necessary on Windows)

The operations are the same between Windows and POSIX systems (except for
the final one because of differences between how filesystems are implemented on
Windows and POSIX sysems), but Windows provides slightly weaker guarantees (it
should still be atomic, but some metadata might not be written properly in the
case of a crash). The code is slightly different depending on the POSIX system,
but the basic idea is the same for all of them.

## Large Files

This method is best used for small files. Because we rewrite the entire file it
will be slow for large files. Unfortunately, I don't know how to efficiently
provide these same guarantees for large files. The problem is more difficult
than it appears because most filesystems do not provide the guarantees we would
need to write similar code that works for large files. If you control the
systems your application runs on your best bet is to use a filesystem that
provides the guarantees you need.

## Why AtomicWrite Throws Exceptions

Some C++ programmers don't like exceptions. I think this is due to people
overusing them for error handling, which complicates code. AtomicWrite throws
exceptions in cases where there is no way to write to the file atomically. I
feel this is usually a fatal error if an application cares about writing
atomically to disk, so I've made this case throw an exception so that you either
have to deal with it, or crash the app.

## License

Released under the ISC license. This is essentially equivalent to the MIT/BSD
license. See LICENSE for full text.

## Contributions

AtomicWrite is open-source, but not open-contribution. Bug fixes are welcome,
but I will probably not accept other patches.
