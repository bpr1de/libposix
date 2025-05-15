//
// Copyright (c) 2025 Bryan Phillippe
//
// This software is free to use for any purpose, provided this copyright
// notice is preserved.
//

#include <functional>
#include <iostream>
#include <thread>
#include <csignal>

#include <unistd.h>
#include <sys/wait.h>

#include <libposix.hh>

posixcc::process::process(process&& p) noexcept:
child_pid{p.child_pid}
{
    p.detach();
}

posixcc::process::~process()
{
    if (is_running()) {
        stop();
    }
}

posixcc::process&
posixcc::process::operator=(posixcc::process&& p) noexcept
{
    child_pid = p.child_pid;
    p.detach();
    return *this;
}

bool
posixcc::process::is_running() const
{
    if (-1 == child_pid) {
        return false;
    }

    const pid_t p = waitpid(child_pid, nullptr, WNOHANG);
    if (p == child_pid) {
        child_pid = -1;
        return false;
    }

    return true;
}

void
posixcc::process::start(const std::function<void()> &task) const
{
    stop();

    switch (child_pid = fork()) {
    case -1:
        throw std::runtime_error("fork failed" + std::to_string(errno));

    // Child
    case 0:
        task();
        exit(EXIT_SUCCESS);

    // Parent
    default:
        break;
    }
}

std::size_t
posixcc::process::get_id() const
{
    return child_pid > 0 ? child_pid : 0;
}

void
posixcc::process::join() const
{
    static constexpr std::size_t delay_max = 1000;
    std::size_t delay_ms                   = 10;

    while (is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        delay_ms = std::min(delay_ms * 2, delay_max);
    }
}

void
posixcc::process::stop() const
{
    if (-1 != child_pid) {
        kill(child_pid, SIGTERM);
    }
}

void
posixcc::process::detach() const
{
    child_pid = -1;
}

#ifdef PROCESS_TEST
#include "stfu/stfu.hh"

extern "C" std::size_t
unit_tests()
{
    stfu::test basic_test{"fork test", [] {
            posixcc::process worker;
            STFU_ASSERT(!worker.is_running());
            worker.start([]{::sleep(1);});
            STFU_ASSERT(worker.is_running());
            worker.join();
            STFU_ASSERT(!worker.is_running());
            STFU_PASS();
        },
        "Check to see if the forked worker is created and joined properly."
    };
    stfu::test cancel_test{"stop test", [] {
            posixcc::process worker;
            worker.start([]{::sleep(30);});
            STFU_ASSERT(worker.is_running());
            worker.stop();
            ::sleep(1);
            STFU_ASSERT(!worker.is_running());
            STFU_PASS();
        },
        "Verify that workers can be cancelled."
    };
    stfu::test detached_test{"detached worker", [] {
            posixcc::process worker;
            pid_t id = 0;

            worker.start([]{::sleep(30);});
            STFU_ASSERT(worker.is_running());
            id = worker.get_id();
            STFU_ASSERT(id > 0);
            worker.detach();
            STFU_ASSERT(!worker.is_running());
            STFU_ASSERT(0 == worker.get_id());
            STFU_ASSERT(!::kill(id, 0));
            STFU_PASS();
        },
        "Verify that detached workers no longer appear to be running."
    };
    stfu::test move_test{"move test", [] {
        posixcc::process worker;
        std::vector<posixcc::process> workers;
        worker.start([]{::sleep(1);});
        std::size_t id = worker.get_id();
        workers.push_back(std::move(worker));
        STFU_ASSERT(!worker.is_running());
        STFU_ASSERT(workers[0].is_running());
        STFU_ASSERT(workers[0].get_id() == id);

        posixcc::process worker2;
        worker2 = std::move(workers[0]);
        STFU_ASSERT(worker2.get_id() == id);
        STFU_PASS();
        },
        "Confirm copy and move operations."
    };

    stfu::test_group unit_tests{"worker tests",
        "Self-tests of the worker module."};
    unit_tests.add_test(basic_test);
    unit_tests.add_test(cancel_test);
    unit_tests.add_test(detached_test);
    unit_tests.add_test(move_test);

    stfu::test_result_summary summary = unit_tests();
    return summary.failed + summary.crashed;
}
#endif // PROCESS_TEST
