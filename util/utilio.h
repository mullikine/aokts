/**
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION.
	utilio.h -- File i/o Utility Functions

	UTIL
**/

#include <cstdio>
#include <cstring>
#include <assert.h>
#include <stdexcept>
#include <string>
#include <intrin.h>
#include <bitset>
#include <iostream>

/*
	REPORT(expr, action)

	If [expr] evaluates false, performs [action].
	Also brings up a debug box in a debug build.
*/
#ifdef _DEBUG

#define REPORT(expr, action) \
	if (!(expr)) \
	{ \
		assert(false); \
		action; \
	}

#define REPORTS(expr, action, string) \
	if (!(expr)) \
	{ \
		assert(false); \
		action; \
		printf(string); \
	}

#else

#define REPORT(expr, action) \
	if (!(expr)) \
		action;

#define REPORTS(expr, action, string) \
	if (!(expr)) \
	{ \
		action; \
		printf(string); \
	}

#endif

#define SKIP(f, s) fseek(f, s, SEEK_CUR)

struct ClipboardType {
    enum Value
    {
	    NONE,	//just so we can have an invalid value for error checking
	    TRIGGER,
	    EFFECT,
	    CONDITION,
    };
};

void printf_log(const char* fmt, ...);

/**
 * @return the file's size, in bytes.
 */
long fsize(const char *path); // TODO: take a FILE*

// like auto_ptr, but for a cstdio FILE
class AutoFile
{
public:
	AutoFile(FILE * file);
	AutoFile(const char * filename, const char * mode);
	~AutoFile();

	FILE * get();

	/**
	 * Close the file. Further calls to get() will return NULL.
	 */
	void close();

private:
	AutoFile(const AutoFile&); // no copy constructor!

	FILE * _file;
};

inline FILE * AutoFile::get()
{
	return _file;
}

/* Input */

class io_error : public std::runtime_error
{
public:
	io_error(const std::string& what);
};

/* This class is thrown by I/O util functions if they get unexpected data. */
class bad_data_error : public std::runtime_error
{
public:
	/**
	 * Constructs a bad_data_error with the specified explanation string. A
	 * user-friendly message will be prepended to it.
	 */
	bad_data_error(const std::string& what);
};

/**
 * Read binary data from /in/ to /out/.
 */
template <class T> void readbin(FILE * in, T * out, size_t count = 1)
{
	size_t read = fread(out, sizeof(T), count, in);
	if (read != count)
	{
		throw bad_data_error(
			(feof(in)) ? "Early EOF" : "stream error");
	}
}

template <typename T> inline T readval(FILE * in)
{
	T n;
	readbin(in, &n);

	return n;
}

/**
 * Read an unknown-meaning value from a file, check it, and then throw it out.
 */
template <class T>
void readunk(FILE * file, T expected, const char * name, bool err = false)
{
	check(readval<T>(file), expected, name, err);
}

template <class T>
void check(T value, T expected, const char * name, bool err = false)
{
	if (value != expected)
	{
		printf("Unexpected %s value: %d, float: %f.\n", name, value, value);

		if (err)
		{
			throw bad_data_error(
			"Bad check value in file. (See log for more information.)");
		}
	}
}

/**
 * Reads a string from a file into a C string (char array). T is the integral
 * type of the size field in the Scenario (typically long or short).
 */
template <class T> void readcs(FILE * in, char * dest, size_t space)
{
	printf("enter readcs()");
	T len;

	readbin(in, &len);
	printf("String length: usigned short %hu\n", len);
	show_binrep(len);
	printf("Space: %lu\n", space);

	if (len >= space) // need one for NULL term
		throw std::length_error("readcs: not enough space in dest string");

	readbin(in, dest, len);
	dest[len] = '\0'; // Scenario strings are not always null-terminated.
	printf("exit readcs()");
}

/* Output */

/**
 * Write binary data in /in/ to /out/.
 */
template <class T> inline void writebin(FILE * out, T * in)
{
	fwrite(in, sizeof(T), 1, out);
}

template <class T> void writeval(FILE * out, T in)
{
	writebin(out, &in);
}

//write a variable amount of NULLs to an ostream
inline void NULLS(FILE *out, int size)
{
	while (size--)
		putc(0, out);
}

//write a char*
template <typename T>
void writecs(FILE *out, const char *source, bool null = true)
{
	T len = static_cast<T>(strlen(source)); // truncate
	if (len && null)	//only write if string exists (mimicing AOK)
		++len;

	writebin(out, &len);
	fwrite(source, sizeof(char), len, out);
}

void swapByteOrder(unsigned short& us);

template<typename T>
void show_binrep(const T& a)
{
    printf_log("binrep: ");
    const char* beg = reinterpret_cast<const char*>(&a);
    const char* end = beg + sizeof(a);
    while(beg != end)
        std::cout << std::bitset<CHAR_BIT>(*beg++) << ' ';
    std::cout << '\n';
}
