//
// Copyright (c) 2024 Bryan Phillippe
//
// This software is free to use for any purpose, provided this copyright
// notice is preserved.
//

#include <unistd.h>
#include <libposix.hh>

posixcc::auto_fd::auto_fd(const int i) noexcept:
fd{i}
{
}

posixcc::auto_fd::auto_fd(const auto_fd& i) noexcept:
fd{dup(i)}
{
}

posixcc::auto_fd::auto_fd(auto_fd&& i) noexcept:
fd{i}
{
    i.release();
}

posixcc::auto_fd::~auto_fd()
{
    close();
}

posixcc::auto_fd&
posixcc::auto_fd::operator=(const int i) noexcept
{
    set(i);
    return *this;
}

posixcc::auto_fd&
posixcc::auto_fd::operator=(const auto_fd& i) noexcept
{
    set(dup(i));
    return *this;
}

posixcc::auto_fd&
posixcc::auto_fd::operator=(auto_fd&& i) noexcept
{
    set(i.release());
    return *this;
}

posixcc::auto_fd::operator int() const noexcept
{
    return fd;
}

posixcc::auto_fd::operator bool() const noexcept
{
    return (-1 != fd);
}

int
posixcc::auto_fd::get() const noexcept
{
    return fd;
}

int
posixcc::auto_fd::set(const int i) noexcept
{
    close();
    fd = i;
    return i;
}

int
posixcc::auto_fd::release() noexcept
{
    int i = fd;
    fd = -1;
    return i;
}

void
posixcc::auto_fd::close() noexcept
{
    if (-1 != fd) {
        ::close(fd);
        fd = -1;
    }
}

#ifdef AUTO_FD_TEST
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include "../stfu/stfu.hh"

static int global_cwd = -1;

static bool
test_setup()
{
    const char* tmpdir = getenv("TMPDIR");

    if (!tmpdir) {
        tmpdir = "/tmp";
    }

    global_cwd = open(".", O_RDONLY|O_SEARCH);
    if (0 == chdir(tmpdir)) {
        return true;
    }

    return false;
}

static bool
test_teardown()
{
    fchdir(global_cwd);
    close(global_cwd);
    global_cwd = -1;

    return true;
}

static void
auto_fd_tests()
{
    int control_fd;
    char tmpfilename[] = "auto_fd-test.XXXXX";
    const std::string msg{"This is a test\n"};

    // Verify size-consistency with a file descriptor.
    STFU_ASSERT(sizeof(posixcc::auto_fd) == sizeof(int));

    // Verify default value.
    posixcc::auto_fd unused;
    STFU_ASSERT(!unused);
    STFU_ASSERT(-1 == unused.get());

    // Verify assignment and release.
    STFU_ASSERT(99 == unused.set(99));
    STFU_ASSERT(unused);
    STFU_ASSERT(99 == unused.get());
    STFU_ASSERT(99 == unused.release());
    STFU_ASSERT(-1 == unused.get());

    // Verify auto destruction behavior.
    {
        control_fd = mkstemp(tmpfilename);
        posixcc::auto_fd fd = control_fd;

        STFU_ASSERT(fd);

        // Validate the fd integral matches the input.
        STFU_ASSERT(fd.get() == control_fd);

        // Validate that we can write to the auto_fd like an int.
        STFU_ASSERT(msg.length() == write(fd, msg.c_str(), msg.length()));
    }

    // Validate that the auto_fd closed the file descriptor.
    STFU_ASSERT(-1 == write(control_fd, "\n", 1));

    // Verify use-as-an-int.
    char buffer[100];
    posixcc::auto_fd fd{open(tmpfilename, O_RDONLY)};
    ssize_t l = read(control_fd, buffer, sizeof(buffer));
    buffer[l] = '\0';
    STFU_ASSERT(l == msg.length());
    STFU_ASSERT(msg == buffer);

    // Verify move semantics.
    posixcc::auto_fd moved{std::move(fd)};
    STFU_ASSERT(!fd);
    STFU_ASSERT(moved);
    STFU_ASSERT(0 == lseek(moved, 0, SEEK_SET));

    posixcc::auto_fd moved2{};
    lseek(moved, 3, SEEK_SET);
    moved2 = std::move(moved);
    STFU_ASSERT(!moved);
    STFU_ASSERT(moved2);
    STFU_ASSERT(3 == lseek(moved2, 0, SEEK_CUR));

    // Verify copy semantics.
    posixcc::auto_fd copied{moved2};
    STFU_ASSERT(5 == lseek(moved2, 5, SEEK_SET));
    moved2.close();
    STFU_ASSERT(5 == lseek(copied, 0, SEEK_CUR));

    unlink(tmpfilename);

    STFU_PASS();
}

extern "C" std::size_t
unit_tests()
{
    stfu::test_group group{"auto_fd tests",
        "Tests of auto_fd functionality."};
    group.add_before_all(test_setup)
             .add_test(stfu::test{"auto_fd",
                     auto_fd_tests,
                     "Tests of the auto_fd class and operations."})
             .add_after_all(test_teardown);

    stfu::test_result_summary summary = group();
    return summary.failed + summary.crashed;
}
#endif // AUTO_FD_TEST
