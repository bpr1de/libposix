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

void posixcc::process::enable_zombies(bool enabled)
{
    struct sigaction act{};

    sigemptyset(&act.sa_mask);
    if (enabled) {
        act.sa_handler = SIG_DFL;
        act.sa_flags = 0;
    } else {
        act.sa_handler = SIG_IGN;
        act.sa_flags = SA_NOCLDWAIT;
    }
    if (-1 == sigaction(SIGCHLD, &act, nullptr)) {
        throw std::runtime_error("sigaction failed");
    }
}

void posixcc::process::reap_all() noexcept
{
    while (waitpid(0, nullptr, WNOHANG) > 0) {
    }
}

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
posixcc::process::operator=(process&& p) noexcept
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
            worker.start([]{sleep(1);});
            STFU_ASSERT(worker.is_running());
            worker.join();
            STFU_PASS_IFF(!worker.is_running());
        },
        "Check to see if the forked worker is created and joined properly."
    };
    stfu::test cancel_test{"stop test", [] {
            posixcc::process worker;
            worker.start([]{sleep(30);});
            STFU_ASSERT(worker.is_running());
            worker.stop();
            sleep(1);
            STFU_PASS_IFF(!worker.is_running());
        },
        "Verify that workers can be cancelled."
    };
    stfu::test detached_test{"detached worker", [] {
            posixcc::process worker;
            pid_t id = 0;

            worker.start([]{sleep(30);});
            STFU_ASSERT(worker.is_running());
            id = worker.get_id();
            STFU_ASSERT(id > 0);
            worker.detach();
            STFU_ASSERT(!worker.is_running());
            STFU_ASSERT(0 == worker.get_id());
            STFU_ASSERT(!kill(id, 0));
            kill(id, SIGKILL);
            STFU_PASS();
        },
        "Verify that detached workers no longer appear to be running."
    };
    stfu::test move_test{"move test", [] {
        posixcc::process worker;
        std::vector<posixcc::process> workers;
        worker.start([]{sleep(1);});
        std::size_t id = worker.get_id();
        workers.push_back(std::move(worker));
        STFU_ASSERT(!worker.is_running());
        STFU_ASSERT(workers[0].is_running());
        STFU_ASSERT(workers[0].get_id() == id);

        posixcc::process worker2;
        worker2 = std::move(workers[0]);
        STFU_PASS_IFF(worker2.get_id() == id);
        },
        "Confirm copy and move operations."
    };
    stfu::test no_zombies{"disable zombies", [] {
        posixcc::process worker;
        posixcc::process::enable_zombies(false);
        worker.start([]{});
        sleep(1);
        STFU_PASS_IFF(-1 == kill(worker.get_id(), 0));
        },
        "Confirm that zombies can be disabled."
    };
    stfu::test reap_test{"reap test", [] {
        posixcc::process worker1;
        posixcc::process workerN;
        posixcc::process worker2;
        posixcc::process::enable_zombies(true);
        worker1.start([]{});
        for (auto i = 0; i < 10; ++i) {
            workerN.start([]{});
            workerN.detach();
        }
        worker2.start([]{});
        sleep(1);
        STFU_ASSERT(!kill(worker1.get_id(), 0) &&
            !kill(worker2.get_id(), 0));
        posixcc::process::reap_all();
        STFU_PASS_IFF(-1 == kill(worker1.get_id(), 0) &&
            -1 == kill(worker2.get_id(), 0));
        },
        "Confirm reaping of children."
    };

    stfu::test_group unit_tests{"worker tests",
        "Self-tests of the worker module."};
    unit_tests.add_test(basic_test)
        .add_test(cancel_test)
        .add_test(detached_test)
        .add_test(move_test)
        .add_test(no_zombies)
        .add_test(reap_test)
        ;

    stfu::test_result_summary summary = unit_tests();
    return summary.failed + summary.crashed;
}
#endif // PROCESS_TEST
