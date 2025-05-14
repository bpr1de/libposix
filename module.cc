//
// Copyright (c) 2025 Bryan Phillippe
//
// This software is free to use for any purpose, provided this copyright
// notice is preserved.
//

#include <string>
#include <stdexcept>

#include <dlfcn.h>

#include <libposix.hh>

posixcc::modsymbol::modsymbol(const void *h, const void *p):
    handle{h}, ptr{p}
{
}

posixcc::modsymbol::modsymbol(modsymbol &&m) noexcept:
    handle{m.handle}, ptr{m.ptr}
{
    m.ptr    = nullptr;
    m.handle = nullptr;
}

posixcc::modsymbol::~modsymbol()
{
    if (handle) {
        dlclose(const_cast<void *>(handle));
    }
}

posixcc::modsymbol
posixcc::load_modsymbol(const char *sym, const char *from)
{
    const auto h = dlopen(from, RTLD_NOW);
    if (!h) {
        std::string reason{"dlopen failed on "};
        reason.append(from);
        throw std::runtime_error{reason};
    }

    const void *p = dlsym(h, sym);
    if (!p) {
        dlclose(h);
        std::string reason{"dlsym failed to find "};
        reason.append(sym);
        reason.append(" in ");
        reason.append(from);
        throw std::runtime_error{reason};
    }

    return modsymbol{h, p};
}
