#include <libposix.hh>
#include "../stfu/stfu.hh"

int
main(int argc, char* argv[])
{
    std::size_t failure_count{0};

    for (int i = 1; i < argc; ++i) {
        const auto &mod = posixcc::load_modsymbol("unit_tests", argv[i]);

        typedef std::size_t (*unit_test_fn)();
        const auto run_tests = posixcc::get_symbol<unit_test_fn>(mod);

        failure_count += run_tests();
    }

    return static_cast<int>(-failure_count);
}
