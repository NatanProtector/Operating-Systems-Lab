#ifndef UNTITLED_SIM_MEM_H
#define UNTITLED_SIM_MEM_H
#define MEMORY_SIZE 200
#include <fstream>

typedef struct
{
    bool valid;
    int frame;
    bool dirty;
    int swap_index;

} page_descriptor;

class sim_mem {
private:
    int swapfile_fd; //swap file fd
    int program_fd; //executable file fd

    int text_size;
    int data_size;
    int bss_size;
    int heap_stack_size;
    int page_size;

    page_descriptor** page_table;

    // my fields and methods
    char* fileName;
    int findFreeMemory();
    void parse_address(int address, int* page, int* offSet, int* in, int* out);

public:

    sim_mem(char exe_file_name[], char swap_file_name[], int text_size, int data_size,
            int bss_size, int heap_stack_size, int page_size);
    ~sim_mem();
    char load(int address);
    void store(int address, char value);
    void print_memory();
    void print_swap ();
    void print_page_table();
};

#endif //UNTITLED_SIM_MEM_H
