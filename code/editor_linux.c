struct platform
{
} platform;

inline time get_time(void)
{
	struct timespec ts;
	assert(clock_gettime(CLOCK_REALTIME, &ts) != -1);
	return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

inline void *allocate(uint size)
{
	void *memory = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	assert(memory != MAP_FAILED);
	return memory;
}

inline void deallocate(void *memory, uint size)
{
	assert(munmap(memory, size) != -1);
}

inline handle open_file(const char *path)
{
	handle handle = open(path, O_RDONLY);
	assert(handle != -1);
	return handle;
}

inline uintl get_size_of_file(handle handle)
{
	struct stat st;
	assert(!fstat(handle, &st));
	assert((st.st_mode & S_IFMT) == S_IFREG);
	return st.st_size;
}

inline uint read_from_file(void *buffer, uint size, handle handle)
{
	ssize_t result = read(handle, buffer, size);
	assert(result != -1);
	return result;
}

inline void close_file(handle handle)
{
	assert(close(handle) != -1);
}

static void initialize(void)
{
	/* create the window */
	{

	}
}

static void process_messages(void)
{
}