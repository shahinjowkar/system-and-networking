# Simple Thread Scheduler

A lightweight user-space thread scheduler implemented in C using `ucontext.h` for cooperative multitasking.

## Features

- **Cooperative multitasking**: Threads yield control voluntarily via `sut_yield()`
- **I/O operations**: Non-blocking file I/O handled by dedicated I/O execution thread
- **Queue-based scheduling**: Ready queue manages thread execution order
- **Configurable executors**: Supports 1 or 2 execution threads (via `EXEC` flag)
- **Thread limits**: Maximum 32 concurrent threads

## Building

```bash
gcc -o scheduler "sut (1).c" -lpthread
```

**Dependencies**: Requires `queue.h` and `sut.h` header files.

## API

- `sut_init()` - Initialize scheduler and start kernel threads
- `sut_create(fn)` - Create a new thread with function `fn`
- `sut_yield()` - Yield execution to another thread
- `sut_exit()` - Terminate current thread
- `sut_open(path)` - Open file (non-blocking)
- `sut_write(fd, buf, size)` - Write to file (non-blocking)
- `sut_close(fd)` - Close file (non-blocking)
- `sut_read(fd, buf, size)` - Read from file (non-blocking)
- `sut_shutdown()` - Clean shutdown, join all kernel threads

## Architecture

Uses three queues: `ready` (executable threads), `iowait` (I/O-blocked threads), and `iotodo` (I/O operations). Kernel threads `c_exec`/`c_exec2` schedule ready threads, while `i_exec` handles I/O operations.

