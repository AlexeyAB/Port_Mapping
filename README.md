boost-port-mapping
================

The source code of example - how to write a simple port mapping using Boost.Asio.

Source code requires Boost >= 1.53 & C++11 (_MSC_VER >= 1700 or GCC >= 4.7.2).


Used optimizations:
- thread-pool for executors: number of threads equals to number of CPU-cores
- thread-pool for listeners (acceptors): one or many of threads
- custom allocator for handlers, that eliminates dynamic allocation of memory, when using boost::bind() (re-uses a static array from connection's class)
- memory pools for connections, that reduce at ten fold the numbers of memory allocations for connections (allocates memory at once for 10 connections, by default connections_in_memory_pool = 10, but it can be increased)
- uses move semantic (boost::move) to eliminate copy of boost::shared_ptr<> and doesn't use atomic counter with memory barrier


Boost.Asio uses platform-specific optimal demultiplexing mechanism:
- on Windows: I/O completion port
- on Linux (Kernel >= 2.6): EPOLL
- on Unix/XNU (FreeBSD, MacOSX, iOS): KQUEUE
- on Unix (AIX, HP-UX): SELECT (for AIX not yet IOCP/POLL_SET)
- on Solaris: /dev/poll


There is able to set these parameters by using command line:
- remote address and port (default: google.com:80)
- local interface address and port (default: 0.0.0.0:10001)
- number of threads for listeners (acceptors) in thread pool (default: 2)
- number of threads for executors in the thread pool, where the handlers are executed (default: equal to the number of CPU-cores in the system)
- language locale (def: rus)

