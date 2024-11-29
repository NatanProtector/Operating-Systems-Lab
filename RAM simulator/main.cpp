#include "sim_mem.h"
#include "sim_mem.cpp"

char main_memory[MEMORY_SIZE];

int main()
{
    sim_mem mem_sm("exec_file.txt", "swap_file.txt" ,8, 8, 8,8, 4);

    mem_sm.load(4);


    mem_sm.print_memory();

    std::cout<<mem_sm.load(4)<<std::endl;

    /*
    for (int i = 0 ; i < 16*2 ; i++) {
        std::cout<<"address: "<<i<<std::endl;
        mem_sm.load(i);
        std::cout<<std::endl;
    }
    mem_sm.store(98,'X');
    val = mem_sm.load (8);
    mem_sm.print_memory();
    mem_sm.print_swap();*/

    return 0;
}