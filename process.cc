//
// Copyright (c) 2024 Bryan Phillippe
//
// This software is free to use for any purpose, provided this copyright
// notice is preserved.
//

#include <functional>
#include <unistd.h>
#include <sys/wait.h>
#include <cerrno>
#include "include/libposix.hh"

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
        throw posixcc::posixcc_error{EINVAL};
    }

    if (waitpid(pid, &stat, 0) != pid) {
        throw posixcc::posixcc_error{errno};
    }

    pid = -1;

    return stat;
}
