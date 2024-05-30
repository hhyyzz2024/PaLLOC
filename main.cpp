#include "system.h"

int main(int argc, char **argv)
{
    PaLLOC::system *sys = PaLLOC::system::get_instance(argc, argv);
    sys->run();
    sys->clean_up();
    return 0;
}