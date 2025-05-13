#include <string>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <sys/errno.h>

#include <libposix.hh>
#include "../stfu/stfu.hh"

static int global_cwd = -1;

static bool
test_setup()
{
    const char* tmpdir = ::getenv("TMPDIR");

    if (!tmpdir) {
        tmpdir = "/tmp";
    }

    global_cwd = ::open(".", O_RDONLY|O_SEARCH);
    if (0 == ::chdir(tmpdir)) {
        return true;
    }

    return false;
}

static bool
test_teardown()
{
    ::fchdir(global_cwd);
    ::close(global_cwd);
    global_cwd = -1;

    return true;
}

static void
error_tests()
{
    try {
        throw posixcc::posixcc_error{ENOENT};
    }

    catch (const posixcc::posixcc_error& e) {
        std::string message{e.what()};
        STFU_ASSERT(ENOENT == e.error);
        STFU_ASSERT(message == "No such file or directory");
        STFU_PASS();
    }
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
        control_fd = ::mkstemp(tmpfilename);
        posixcc::auto_fd fd = control_fd;

        STFU_ASSERT(fd);

        // Validate the fd integral matches the input.
        STFU_ASSERT(fd.get() == control_fd);

        // Validate that we can write to the auto_fd like an int.
        STFU_ASSERT(msg.length() == ::write(fd, msg.c_str(), msg.length()));
    }

    // Validate that the auto_fd closed the file descriptor.
    STFU_ASSERT(-1 == ::write(control_fd, "\n", 1));

    // Verify use-as-an-int.
    char buffer[100];
    posixcc::auto_fd fd{::open(tmpfilename, O_RDONLY)};
    ssize_t l = ::read(control_fd, buffer, sizeof(buffer));
    buffer[l] = '\0';
    STFU_ASSERT(l == msg.length());
    STFU_ASSERT(msg == buffer);

    // Verify move semantics.
    posixcc::auto_fd moved{std::move(fd)};
    STFU_ASSERT(!fd);
    STFU_ASSERT(moved);
    STFU_ASSERT(0 == ::lseek(moved, 0, SEEK_SET));

    posixcc::auto_fd moved2{};
    ::lseek(moved, 3, SEEK_SET);
    moved2 = std::move(moved);
    STFU_ASSERT(!moved);
    STFU_ASSERT(moved2);
    STFU_ASSERT(3 == ::lseek(moved2, 0, SEEK_CUR));

    // Verify copy semantics.
    posixcc::auto_fd copied{moved2};
    STFU_ASSERT(5 == ::lseek(moved2, 5, SEEK_SET));
    moved2.close();
    STFU_ASSERT(5 == ::lseek(copied, 0, SEEK_CUR));

    ::unlink(tmpfilename);

    STFU_PASS();
}

static void
auto_pipe_tests()
{
    posixcc::auto_pipe o;
    posixcc::auto_pipe p{std::move(o)};

    STFU_ASSERT(!o);
    STFU_ASSERT(p);
    STFU_ASSERT(-1 != p.get_rfd());
    STFU_ASSERT(-1 != p.get_wfd());

    switch (pid_t pid = ::fork()) {
    case -1:
        return;

    case 0:
        p.close_rfd();
        {
            posixcc::auto_pipe q{p};
            STFU_ASSERT(p && q);
            ::write(q.get_wfd(), "hello", 5);
            ::exit(0);
        }

    default:
        p.close_wfd();
        char buf[10] = {'\0'};
        ::read(p.get_rfd(), buf, sizeof(buf) - 1);
        STFU_ASSERT(0 == ::strncmp(buf, "hello", 5));
        int unused;
        ::waitpid(pid, &unused, 0);
    }

    STFU_ASSERT(p);
    STFU_ASSERT(!p.close());
    STFU_PASS();
}

static void
process_tests()
{
    posixcc::process unused;
    STFU_ASSERT(-1 == unused.get_id());
    try {
        unused.join();
        STFU_FAIL();
    } catch (posixcc::posixcc_error& e) {
        STFU_ASSERT(EINVAL == e.error);
    }

    posixcc::process simple{[](){ ::exit(0); }};
    STFU_ASSERT(0 == simple.join());
    STFU_ASSERT(-1 == simple.get_id());

    auto one_arg = [](int a){ if (4 != a) { ::exit(1); } ::exit(0); };
    posixcc::process p{one_arg, 4};
    int stat = p.join();
    STFU_ASSERT(WIFEXITED(stat) && 0 == WEXITSTATUS(stat));

    STFU_PASS();
}

int
main(int argc, char* argv[])
{
    int any_failed{0};

    stfu::test_group all_tests{"all tests", "All basic functionality."};
    all_tests.add_before_all(test_setup)
             .add_test(stfu::test{"errors",
                     error_tests,
                     "Tests of error classes."})
             .add_test(stfu::test{"auto_fd",
                     auto_fd_tests,
                     "Tests of the auto_fd class and operations."})
             .add_test(stfu::test{"auto_pipe",
                     auto_pipe_tests,
                     "Tests of the auto_pipe class and operations."})
             .add_test(stfu::test{"process",
                     process_tests,
                     "Tests of the process management class."})
             .add_after_all(test_teardown);

    stfu::test_result_summary summary = all_tests();
    any_failed += static_cast<int>(summary.failed + summary.crashed);

    return -any_failed;
}
