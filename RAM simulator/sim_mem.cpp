#include "sim_mem.h"
#include <cmath>
#include <fcntl.h>
#include <iostream>
#define outer_page_table_size 4
#define out_Bits 2
#define address_size 12

extern char main_memory[MEMORY_SIZE];

sim_mem::sim_mem(char exe_file_name[], char swap_file_name[], int text_size, int data_size, int bss_size, int heap_stack_size, int page_size) {
    this->program_fd = open(exe_file_name, O_RDONLY);
    this->swapfile_fd = open(swap_file_name, O_RDWR|O_CREAT);
    this->text_size = text_size;
    this->data_size = data_size;
    this->bss_size = bss_size;
    this->heap_stack_size = heap_stack_size;
    this->page_size = page_size;
    this->page_table = new (page_descriptor*[outer_page_table_size]);
    for (int i = 0 ; i < 4 ; i++) { // set page table
        this->page_table[i] = new page_descriptor[text_size/page_size];
        for (int j = 0 ; j < text_size/page_size ; j++) {
            this->page_table[i][j].valid = false;
            this->page_table[i][j].dirty = true;
        }
    }

    for (int i = 0 ; i < MEMORY_SIZE ; i++)
        main_memory[i] = 0;
}

int getNthBit(int number, int n) {
    int mask = 1 << n;
    int bit = (number & mask) >> n;
    return bit;
}
void setNthBit(int& number, int n, int bit) {
    int mask = 1 << n;
    if (bit == 0)
        number = number & (~mask);
    else if (bit == 1)
        number = number | mask;
}

void sim_mem::parse_address(int address, int* page, int* offSet, int* in, int* out) {
    int offSetBits = (int)log2(this->page_size);
    int pageBits = address_size - offSetBits;
    int inBits = pageBits - out_Bits;
    int address_index = 0, page_index = 0;
    int bit;

    for (int i = 0 ; i < offSetBits ; i++) {
        bit = getNthBit(address,address_index);
        std::cout<<bit;
        setNthBit((*offSet), i,bit);
        address_index++;
    }
    for (int i = 0 ; i < inBits ; i++) {
        bit = getNthBit(address,address_index);
        std::cout<<bit;
        setNthBit((*in), i,bit);
        setNthBit((*page), page_index,bit);
        address_index++;
        page_index++;
    }
    for (int i = 0 ; i < out_Bits ; i++) {
        bit = getNthBit(address,address_index);
        std::cout<<bit;
        setNthBit((*out), i,bit);
        setNthBit((*page), page_index,bit);
        address_index++;
        page_index++;
    }
    std::cout<<std::endl;
}

char getCharacterFromFile(int fileDescriptorIndex, int charIndex) {
    // Seek to the desired character index
    lseek(fileDescriptorIndex, charIndex, SEEK_SET);

    // Read a single character from the file
    char character;
    read(fileDescriptorIndex, &character, 1);

    // Return the character read
    return character;
}

int sim_mem::findFreeMemory() {

    for (int i = 0 ; i < MEMORY_SIZE - page_size ; i+=page_size) {
        if (main_memory[i] == 0)
            return i/page_size;
    }

    // must swap. error for now
    perror("no memory");
    exit(EXIT_FAILURE);
}

char sim_mem::load(int address) {

    int page = 0,offSet = 0,in = 0,out = 0;
    parse_address(address, &page, &offSet, &in, &out);

    //std::cout << "page: "<< page << std::endl;
    //std::cout << "offset: "<< offSet << std::endl;
    //std::cout << "outer index: "<< out << std::endl;
    //std::cout << "inner index: "<< in << std::endl;
    if (this->page_table[out][in].valid == 1) {
        int frame = this->page_table[out][in].frame;
        return main_memory[frame*page_size + offSet];
    }
    else if (this->page_table[out][in].valid == 0) {

        int newFrame = findFreeMemory();

        int inputFile = this->program_fd;
        int cursorPos;

        for (int i = 0 ; i < page_size ; i++) {
            cursorPos = page * page_size + i;
            char nextChar = getCharacterFromFile(inputFile,cursorPos);
            main_memory[newFrame*page_size + i] = nextChar;
        }

        this->page_table[out][in].frame = newFrame;
        this->page_table[out][in].valid = 1;
    }

    return 0;
}


void sim_mem::store(int address, char value) {

}

sim_mem::~sim_mem() {
    close(this->program_fd);
    close(this->swapfile_fd);
    for (int i = 0; i < outer_page_table_size ; i++) {
        delete this->page_table[i];
    }
    delete this->page_table;
}

void sim_mem::print_page_table() {

}

void sim_mem::print_swap () {

}

void sim_mem::print_memory() {
    std::cout << "Physical memory\n";
    for (int i = 0 ; i<MEMORY_SIZE ; i++)
        std::cout << "[" << main_memory[i] << "]"<<std::endl;
}