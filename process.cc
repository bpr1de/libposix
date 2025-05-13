//
// Copyright (c) 2025 Bryan Phillippe
//
// This software is free to use for any purpose, provided this copyright
// notice is preserved.
//

#include <functional>
#include <unistd.h>
#include <sys/wait.h>
#include <cerrno>
#include <libposix.hh>

pid_t
posixcc::process::get_id() const noexcept
{
    return pid;
}

int
posixcc::process::join() const
{
    int stat;

    if (-1 == pid) {
        throw std::runtime_error{errno_to_string(EINVAL)};
    }

    if (waitpid(pid, &stat, 0) != pid) {
        throw std::runtime_error{errno_to_string(errno)};
    }

    pid = -1;

    return stat;
}

#ifdef PROCESS_TEST
#include <cstring>
#include "../stfu/stfu.hh"

static void
process_tests()
{
    posixcc::process unused;
    STFU_ASSERT(-1 == unused.get_id());
    try {
        unused.join();
        STFU_FAIL();
    } catch (const std::runtime_error& e) {
        STFU_ASSERT(!strcmp(e.what(), "Invalid argument"));
    }

    posixcc::process simple{[](){ exit(0); }};
    STFU_ASSERT(0 == simple.join());
    STFU_ASSERT(-1 == simple.get_id());

    auto one_arg = [](int a){ if (4 != a) { exit(1); } exit(0); };
    posixcc::process p{one_arg, 4};
    int stat = p.join();
    STFU_ASSERT(WIFEXITED(stat) && 0 == WEXITSTATUS(stat));

    STFU_PASS();
}

extern "C" std::size_t
unit_tests()
{
    stfu::test_group group{"process tests",
        "Tests of process functionality."};
    group.add_test(stfu::test{"process",
                     process_tests,
                     "Tests of the process management class."});

    stfu::test_result_summary summary = group();
    return summary.failed + summary.crashed;
}
#endif // PROCESS_TEST
