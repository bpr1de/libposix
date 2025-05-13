//
// Copyright (c) 2025 Bryan Phillippe
//
// This software is free to use for any purpose, provided this copyright
// notice is preserved.
//

#include <unistd.h>
#include <sys/errno.h>
#include <algorithm>
#include <libposix.hh>

posixcc::auto_pipe::auto_pipe()
{
    int fildes[2];

    if (0 != pipe(fildes)) {
        throw std::runtime_error{errno_to_string(errno)};
    }

    read_fd = fildes[0];
    write_fd = fildes[1];
}

posixcc::auto_pipe::auto_pipe(const auto_pipe& p) noexcept
{
    read_fd = p.read_fd;
    write_fd = p.write_fd;
}

posixcc::auto_pipe::auto_pipe(auto_pipe&& p) noexcept
{
    read_fd = std::move(p.read_fd);
    write_fd = std::move(p.write_fd);
}

posixcc::auto_pipe::~auto_pipe()
{
    close();
}

posixcc::auto_pipe&
posixcc::auto_pipe::operator=(posixcc::auto_pipe&& p) noexcept
{
    read_fd = std::move(p.read_fd);
    write_fd = std::move(p.write_fd);
    return *this;
}

posixcc::auto_pipe::operator bool() const noexcept
{
    return (read_fd || write_fd);
}

int
posixcc::auto_pipe::get_rfd() const noexcept
{
    return read_fd.get();
}

int
posixcc::auto_pipe::get_wfd() const noexcept
{
    return write_fd.get();
}

posixcc::auto_pipe&
posixcc::auto_pipe::close_rfd() noexcept
{
    read_fd.close();
    return *this;
}

posixcc::auto_pipe&
posixcc::auto_pipe::close_wfd() noexcept
{
    write_fd.close();
    return *this;
}

posixcc::auto_pipe&
posixcc::auto_pipe::close() noexcept
{
    close_rfd();
    close_wfd();
    return *this;
}

#ifdef AUTO_PIPE_TEST
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include "../stfu/stfu.hh"

static void
auto_pipe_tests()
{
    posixcc::auto_pipe o;
    posixcc::auto_pipe p{std::move(o)};

    STFU_ASSERT(!o);
    STFU_ASSERT(p);
    STFU_ASSERT(-1 != p.get_rfd());
    STFU_ASSERT(-1 != p.get_wfd());

    switch (pid_t pid = fork()) {
    case -1:
        return;

    case 0:
        p.close_rfd();
        {
        posixcc::auto_pipe q{p};
        STFU_ASSERT(p && q);
        write(q.get_wfd(), "hello", 5);
        exit(0);
        }

    default:
        p.close_wfd();
        char buf[10] = {'\0'};
        read(p.get_rfd(), buf, sizeof(buf) - 1);
        STFU_ASSERT(0 == strncmp(buf, "hello", 5));
        int unused;
        waitpid(pid, &unused, 0);
    }

    STFU_ASSERT(p);
    STFU_ASSERT(!p.close());
    STFU_PASS();
}

extern "C" std::size_t
unit_tests()
{
    stfu::test_group group{"auto_pipe tests",
        "Tests of auto_pipe functionality."};
    group.add_test(stfu::test{"auto_pipe",
        auto_pipe_tests,
        "Tests of the auto_pipe class and operations."});

    stfu::test_result_summary summary = group();
    return summary.failed + summary.crashed;
}
#endif // AUTO_PIPE_TEST
