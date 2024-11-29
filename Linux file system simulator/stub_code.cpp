#include <iostream>
#include <utility>
#include <vector>
#include <map>
#include <cassert>
#include <cstring>
#include <unistd.h>

/* Bugs and Notes:
 * Notes:
 * - remember to test for memory leaks
 * bugs:
 * - when there not enough room for a block and an index, an index block will be created without the block
 * index probably points nowhere, might be breaking, need to test with read.
 * */

using namespace std;

#define DISK_SIZE 512
#define error_msg "ERR"
#define success_value 1
#define failure_value (-1)

// Function to convert decimal to binary char
char decToBinary(int n) {
    return static_cast<char>(n);
}

// #define SYS_CALL
// ============================================================================
class fsInode {
    int fileSize;
    int block_size;
    int blocks_in_use;

    int freeCharsInLastBlock;

    int directBlock1;
    int directBlock2;
    int directBlock3;

    int singleInDirect;

    int freeCharsInSingleIndirect;

    int doubleInDirect;

    int freeBlocksInFirstDoubleIndirect;
    int freeBlocksInLastSecondDoubleIndirect;

    // ------------------------------------------------------------------------
    static int writeToDisk(char charToWrite, FILE* file, int indexInFile) {
        if (file == nullptr  || indexInFile < 0)
            return EXIT_FAILURE;
        fseek(file, indexInFile, SEEK_SET);  // Move to the specified position in the file
        unsigned char buf = decToBinary(charToWrite);
        fwrite(&buf, sizeof(char), 1, file);
        return EXIT_SUCCESS;
    }

    // ------------------------------------------------------------------------
    static char readFromDisk(FILE* file, int indexInFile) {
        if (file == nullptr  || indexInFile < 0)
            return EXIT_FAILURE;

        char buf;
        fseek(file, indexInFile, SEEK_SET);  // Move to the specified position in the file
        fread(&buf, sizeof(char), 1, file);
        return buf;
        //return decToBinary(buf);
    }

    // ------------------------------------------------------------------------
    static int getIndexOfFreeBlock(int** BitVector,int BitVectorSize, int* freeBlocks) {
        for (int i = 0 ; i < BitVectorSize ; i++) {
            if (!getNthBitInBitVector(BitVector, BitVectorSize, i)) {
                setNthBitInBitVector(BitVector,BitVectorSize,i);
                (*freeBlocks)--;
                return i;
            }
        }
        return failure_value; // no free block
    }

    //  functions for manipulating and displaying the bitVector ------------------------------------
    static bool getNthBitInBitVector(int** BitVector,int BitVectorSize,int N) {
        if ((N>=BitVectorSize)) {
            cerr<<"out of bounds in bitVector"<<endl;
            exit(EXIT_FAILURE);
        }
        auto uintptr = reinterpret_cast<uintptr_t>(*BitVector);
        return (uintptr >> N) & 1;
    }
    static void setNthBitInBitVector(int** BitVector,int BitVectorSize,int N) {
        if ((N>=BitVectorSize)) {
            cerr<<"out of bounds in bitVector"<<endl;
            exit(EXIT_FAILURE);
        }
        auto ptrValue = reinterpret_cast<uintptr_t>(*BitVector);
        ptrValue |= (1 << N);
        *BitVector = reinterpret_cast<int*>(ptrValue);
    }
    static void clearNthBitInBitVector(int** BitVector,int BitVectorSize,int N) {
        if ((N>=BitVectorSize)) {
            cerr<<"out of bounds in bitVector"<<endl;
            exit(EXIT_FAILURE);
        }
        //cout<<"clearing bit: "<<N<<" ";
        auto ptrValue = reinterpret_cast<uintptr_t>(*BitVector);
        ptrValue &= ~(1 << N);
        *BitVector = reinterpret_cast<int*>(ptrValue);
    }

    int getNextIndexInFile(int** BitVector, int BitVectorSize, FILE* file, int* freeBlocks) {

        if (directBlock1 == -1) {
            directBlock1 = getIndexOfFreeBlock(BitVector, BitVectorSize,freeBlocks) * block_size;
            blocks_in_use++; freeCharsInLastBlock = block_size;
            return directBlock1;
        }
        else if (directBlock2 == -1) {
            if (freeCharsInLastBlock > 0)
                return directBlock1 + (block_size - freeCharsInLastBlock);
            else {
                directBlock2 = getIndexOfFreeBlock(BitVector, BitVectorSize,freeBlocks) * block_size;
                blocks_in_use++; freeCharsInLastBlock = block_size;
                return directBlock2;
            }
        }
        else if (directBlock3 == -1) {
            if (freeCharsInLastBlock > 0)
                return directBlock2 + (block_size - freeCharsInLastBlock);
            else {
                directBlock3 = getIndexOfFreeBlock(BitVector, BitVectorSize,freeBlocks) * block_size;
                blocks_in_use++; freeCharsInLastBlock = block_size;
                return directBlock3;
            }
        }
        else if (singleInDirect == -1) {
            if (freeCharsInLastBlock > 0)
                return directBlock3 + (block_size - freeCharsInLastBlock);
            else {
                singleInDirect = getIndexOfFreeBlock(BitVector, BitVectorSize,freeBlocks) * block_size;
                int newBlock = getIndexOfFreeBlock(BitVector, BitVectorSize,freeBlocks) * block_size;
                writeToDisk((char)newBlock,file,singleInDirect);
                blocks_in_use++; freeCharsInLastBlock = block_size; freeCharsInSingleIndirect--;
                return newBlock;
            }
        }
        else if (doubleInDirect == -1) {
            if (freeCharsInLastBlock > 0) {
                unsigned char pointerToChar = readFromDisk(file, singleInDirect + (block_size - freeCharsInSingleIndirect - 1));
                return (int)pointerToChar + (block_size - freeCharsInLastBlock);
            } else {
                if (freeCharsInSingleIndirect > 0) {
                    int newBlockIndex = getIndexOfFreeBlock(BitVector, BitVectorSize,freeBlocks) * block_size;
                    writeToDisk((char)newBlockIndex,file,singleInDirect + (block_size - freeCharsInSingleIndirect));
                    freeCharsInSingleIndirect--; freeCharsInLastBlock = block_size;
                    return newBlockIndex;
                } else {
                    doubleInDirect = getIndexOfFreeBlock(BitVector, BitVectorSize,freeBlocks) * block_size;
                    int newPointerBlock = getIndexOfFreeBlock(BitVector, BitVectorSize,freeBlocks) * block_size;
                    int newBlock = getIndexOfFreeBlock(BitVector, BitVectorSize,freeBlocks) * block_size;
                    writeToDisk((char)newPointerBlock,file,doubleInDirect);
                    writeToDisk((char)newBlock,file,newPointerBlock);
                    blocks_in_use++;freeCharsInLastBlock = block_size;
                    freeBlocksInFirstDoubleIndirect--; freeBlocksInLastSecondDoubleIndirect--;
                    return newBlock;
                }

            }
        } else {
            return getIndexFromDoubleIndirect(BitVector,BitVectorSize,file,freeBlocks);
        }
    }

    int getIndexFromDoubleIndirect(int** BitVector,int BitVectorSize,FILE* file,int* freeBlocks) {

        if (freeCharsInLastBlock > 0) { // last block is not full
            int indexOfIndexBlock = (unsigned char)readFromDisk(file,doubleInDirect + (block_size - freeBlocksInFirstDoubleIndirect - 1));
            int indexOfLastBlock = (unsigned char)readFromDisk(file,(int)indexOfIndexBlock + (block_size - freeBlocksInLastSecondDoubleIndirect - 1));
            return indexOfLastBlock + (block_size - freeCharsInLastBlock);
        } else {
            if (freeBlocksInLastSecondDoubleIndirect > 0) { // if last index table is not full
                int nextIndexOfBlock = getIndexOfFreeBlock(BitVector,BitVectorSize,freeBlocks) * block_size;
                unsigned char indexOfLastIndexTable = readFromDisk(file,doubleInDirect + (block_size - freeBlocksInFirstDoubleIndirect - 1));
                writeToDisk((char)nextIndexOfBlock,file,(int)indexOfLastIndexTable + (block_size - freeBlocksInLastSecondDoubleIndirect));
                blocks_in_use++; freeCharsInLastBlock = block_size; freeBlocksInLastSecondDoubleIndirect--;
                return nextIndexOfBlock;
            } else {
                if (freeBlocksInFirstDoubleIndirect > 0) { // if index to index table is full
                    int indexOfFirstDoubleIndirectPointerBlock = getIndexOfFreeBlock(BitVector,BitVectorSize,freeBlocks) * block_size;
                    int indexOfFirstDoubleIndirectBlock = getIndexOfFreeBlock(BitVector,BitVectorSize,freeBlocks) * block_size;
                    writeToDisk(indexOfFirstDoubleIndirectPointerBlock, file, doubleInDirect + (block_size - freeBlocksInFirstDoubleIndirect));
                    writeToDisk(indexOfFirstDoubleIndirectBlock, file, indexOfFirstDoubleIndirectPointerBlock);
                    freeBlocksInFirstDoubleIndirect--;
                    freeBlocksInLastSecondDoubleIndirect = block_size - 1;
                    blocks_in_use++; freeCharsInLastBlock = block_size;
                    return indexOfFirstDoubleIndirectBlock;
                } else {
                    return -1;
                }
            }
        }
    }

public:
    fsInode(int _block_size) {
        fileSize = 0;
        blocks_in_use = 0;
        block_size = _block_size;
        freeCharsInLastBlock = block_size;
        directBlock1 = -1;
        directBlock2 = -1;
        directBlock3 = -1;
        singleInDirect = -1;
        doubleInDirect = -1;

        freeCharsInSingleIndirect = block_size;

        freeBlocksInFirstDoubleIndirect = block_size;
        freeBlocksInLastSecondDoubleIndirect = block_size;

    }

    // YOUR CODE......

    int getBlocksInUse() const {
        return blocks_in_use;
    }

    int getSize() const {
        return fileSize;
    }

    int writeToFile(char* string,int len, FILE* file,int** BitVector,int BitVectorSize, int* freeBlocks, int maxMemory) {

        int stringIndex = 0;

        while (stringIndex != len && fileSize <= maxMemory) {
            int nextIndexInFile = getNextIndexInFile(BitVector, BitVectorSize, file, freeBlocks);
            int writeResult = writeToDisk(string[stringIndex], file, nextIndexInFile);
            if (writeResult == failure_value)
                return failure_value;
            freeCharsInLastBlock--;
            fileSize++;
            stringIndex++;
        }

        //displayBitVector(BitVector, BitVectorSize);

        return EXIT_SUCCESS;
    }
    void readFromDirectBlock(char *buf,int* bufferIndex, int maxIndex, FILE* file, int directBlock) const {
        for (int i = 0 ; i < block_size && (*bufferIndex) != maxIndex ; i++ ) {
            buf[*bufferIndex] = readFromDisk(file,directBlock + i);
            (*bufferIndex)++;
        }
    }
    int readFromFile(char *buf, int len, FILE* file) {

        int bufferIndex = 0;
        int maxIndex = len < fileSize?  len : fileSize;
        readFromDirectBlock(buf,&bufferIndex,maxIndex,file,directBlock1);
        readFromDirectBlock(buf,&bufferIndex,maxIndex,file,directBlock2);
        readFromDirectBlock(buf,&bufferIndex,maxIndex,file,directBlock3);

        int indexOfNextBlockSingle;
        for (int i = 0 ; i < block_size && bufferIndex < maxIndex ; i++) {
            indexOfNextBlockSingle = (unsigned char)readFromDisk(file,singleInDirect + i);
            for (int j = 0 ; j < block_size && bufferIndex < maxIndex ; j++) {
                buf[bufferIndex] = readFromDisk(file, indexOfNextBlockSingle + j);
                bufferIndex++;
            }
        }
        int indexOfNextBlock;
        int indexOfNextIndexBlock;
        for (int i = 0 ; i < block_size && bufferIndex < maxIndex ; i++) {
            indexOfNextIndexBlock = (unsigned char)readFromDisk(file,doubleInDirect + i);
            for (int j = 0 ; j < block_size && bufferIndex < maxIndex ; j++) {
                indexOfNextBlock = (unsigned char)readFromDisk(file,indexOfNextIndexBlock + j);
                for (int k = 0 ; k < block_size && bufferIndex < maxIndex ; k++) {
                    buf[bufferIndex] = readFromDisk(file, indexOfNextBlock + k);
                    bufferIndex++;
                }
            }
        }
        buf[bufferIndex] = '\0';
        return 0;
    }

    void deleteDirectBlock(int* indexOfDeletion, FILE* file, int directBlock, int** BitVector, int* freeBlocks,int BitVectorSize ) const {
        clearNthBitInBitVector(BitVector,BitVectorSize,directBlock/block_size);
        (*freeBlocks)++;
        (*indexOfDeletion) += block_size;
    }
    int deleteFile(FILE* file,int** BitVector, int* freeBlocks, int BitVectorSize) { // to be changed
        int indexOfDeletion = 0;
        if (directBlock1 > -1)
            deleteDirectBlock(&indexOfDeletion,file,directBlock1,BitVector,freeBlocks,BitVectorSize);
        if (directBlock2 > -1)
            deleteDirectBlock(&indexOfDeletion,file,directBlock2,BitVector,freeBlocks,BitVectorSize);
        if (directBlock3 > -1)
            deleteDirectBlock(&indexOfDeletion,file,directBlock3,BitVector,freeBlocks,BitVectorSize);
        // single indirect delete
        if (singleInDirect > -1) {
            int indexOfNextBlockSingle;
            for (int i = 0 ; i < block_size && indexOfDeletion < fileSize ; i++) {
                indexOfNextBlockSingle = (unsigned char)readFromDisk(file,singleInDirect + i);
                clearNthBitInBitVector(BitVector,BitVectorSize,indexOfNextBlockSingle/block_size);
                (*freeBlocks)++;
                indexOfDeletion += block_size;

            }
            clearNthBitInBitVector(BitVector,BitVectorSize,singleInDirect/block_size);
            (*freeBlocks)++;
        }
        // double indirect delete
        if (doubleInDirect > -1) {
            int indexOfNextBlock;
            int indexOfNextIndexBlock;
            for (int i = 0 ; i < block_size && indexOfDeletion < fileSize ; i++) {
                indexOfNextIndexBlock = (unsigned char)readFromDisk(file,doubleInDirect + i);
                for (int j = 0 ; j < block_size && indexOfDeletion != fileSize ; j++) {
                    indexOfNextBlock = (unsigned char)readFromDisk(file,indexOfNextIndexBlock + j);
                    clearNthBitInBitVector(BitVector,BitVectorSize,indexOfNextBlock/block_size);
                    (*freeBlocks)++;
                    indexOfDeletion += block_size;
                }
                clearNthBitInBitVector(BitVector,BitVectorSize,indexOfNextIndexBlock/block_size);
                (*freeBlocks)++;
            }
            clearNthBitInBitVector(BitVector,BitVectorSize,doubleInDirect/block_size);
            (*freeBlocks)++;
        }

        //displayBitVector(BitVector, BitVectorSize);

        return success_value;
    }

};

// ============================================================================
class FileDescriptor {
    pair<string, fsInode*> file;
    bool inUse;
    bool deleted;
public:

    FileDescriptor(string FileName, fsInode* fsi) {
        file.first = std::move(FileName);
        file.second = fsi;
        inUse = true;
        deleted = false;
    }


    string getFileName() const {
        return file.first;
    }

    fsInode* getInode() {
        return file.second;
    }

    int GetFileSize() {
        if (deleted)
            return 0;
        else
            return file.second->getSize();
    }
    bool isInUse() {
        return inUse;
    }

    void setInUse(bool _inUse) {
        inUse = _inUse ;
    }

    void setName(string s) {
        file.first = std::move(s);
    }
    void setToDeleted() {
        file.first = "";
        deleted = true;
    }

};

#define DISK_SIM_FILE "DISK_SIM_FILE.txt"
// ============================================================================
class fsDisk {
    FILE *sim_disk_fd;

    int block_size;

    bool is_formated;

    // BitVector - "bit" (int) vector, indicate which block in the disk is free
    //              or not.  (i.e. if BitVector[0] == 1 , means that the
    //             first block is occupied.)
    int BitVectorSize;
    int *BitVector;

    int freeBlocks;

    // Unix directories are lists of association structures,
    // each of which contains one filename and one inode number.
    map<string, fsInode*>  MainDir ;

    // OpenFileDescriptors --  when you open a file,
    // the operating system creates an entry to represent that file
    // This entry number is the file descriptor.
    vector< FileDescriptor > OpenFileDescriptors;
    int FileDescriptorSize;

    int maxMemoryInFile;

    // private methods --------------------------------------------------------------------------------------------
    int addFileToFD(string fileName, fsInode** NewNode) {
        int index;
        for (index = 0 ; index < FileDescriptorSize ; index++) { // find empty place for new file descriptor.
            if (!OpenFileDescriptors[index].isInUse()) {
                OpenFileDescriptors[index] = FileDescriptor(fileName, *NewNode);
                return index;
            }
        }
        OpenFileDescriptors.emplace_back(fileName, *NewNode);
        FileDescriptorSize++;
        return index;
    }

    // ------------------------------------------------------------------------
    int checkIfFileIsOpen(string FileName) { // return index if open, else return -1
        int index;
        for (index = 0 ; index < FileDescriptorSize ; index++) { // find empty place for new file descriptor.
            string fileNameAtCurrentIndex = OpenFileDescriptors[index].getFileName();
            if (OpenFileDescriptors[index].isInUse() && fileNameAtCurrentIndex.compare(FileName) == 0) {
                return index;
            }
        }
        return -1;
    }

    // ------------------------------------------------------------------------
    bool checkIfFileExists(string fileName) {
        for (const auto& entry : MainDir) { // loop through map
            string currentName = entry.first;
            if (currentName.compare(fileName) == 0)
                return true;
        }
        return false;
    }

    int returnError(){
        cerr<<error_msg<<endl;
        return failure_value;
    }

    void freeAllInodes() {
        for (const auto& entry : MainDir) { // loop through map and delete inodes
            string key = entry.first;
            delete entry.second;
        }
    }



public:
    // ------------------------------------------------------------------------
    fsDisk() {
        sim_disk_fd = fopen( DISK_SIM_FILE , "r+" );
        assert(sim_disk_fd);
        for (int i=0; i < DISK_SIZE ; i++) {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            ret_val = fwrite( "\0" ,  1 , 1, sim_disk_fd );
            assert(ret_val == 1);
        }
        fflush(sim_disk_fd);
        is_formated = false;
    }

    // ------------------------------------------------------------------------
    void listAll() {
        int i = 0;
        for ( auto it = begin (OpenFileDescriptors); it != end (OpenFileDescriptors); ++it) {
            cout << "index: " << i << ": FileName: " << it->getFileName() <<  " , isInUse: "
                 << it->isInUse() << " file Size: " << it->GetFileSize() << endl;
            i++;
        }
        char buffer;
        cout << "Disk content: '" ;
        for (i=0; i < DISK_SIZE ; i++) {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            ret_val = fread(&buffer , 1 , 1, sim_disk_fd );
            cout << buffer;
        }
        cout << "'" << endl;
    }

    // ------------------------------------------------------------------------
    void fsFormat( int blockSize =4 ) {
        if (blockSize<2) {
            cerr<<error_msg<<endl;
            return;
        }
        for (int i=0; i < DISK_SIZE ; i++) {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            fwrite( "\0" ,  1 , 1, sim_disk_fd );
        }
        OpenFileDescriptors.clear();
        FileDescriptorSize = 0;
        freeAllInodes();
        MainDir.clear();
        block_size = blockSize;
        BitVectorSize = DISK_SIZE/blockSize;
        freeBlocks = BitVectorSize;
        BitVector = nullptr;
        is_formated = true;
        maxMemoryInFile = block_size*(3+block_size+block_size*block_size);
        cout<<"FORMAT DISK: number of blocks: "<<BitVectorSize<<endl;
    }

    // ------------------------------------------------------------------------
    int CreateFile(string fileName) {
        if (!is_formated || checkIfFileExists(fileName))
            return returnError();

        auto * NewNode = new fsInode(block_size);
        int index = addFileToFD(fileName, &NewNode);
        MainDir.emplace(fileName, NewNode);
        return index;
    }

    // ------------------------------------------------------------------------
    int OpenFile(string FileName) {

        if (!is_formated)
            return returnError();
        _Rb_tree_iterator<pair<const basic_string<char>, fsInode *>> it = MainDir.find(FileName);
        if (it == MainDir.end())// if file does not exist:
            return returnError();
        else {
            int indexOfFileDescriptor = checkIfFileIsOpen(FileName);
            if (indexOfFileDescriptor >= 0) // check if file is open
                return returnError();
            else { // else, add File to File Descriptor
                fsInode * Inode = it->second;
                indexOfFileDescriptor = addFileToFD(FileName, &Inode);
            }
            return indexOfFileDescriptor;
        }

    }

    // ------------------------------------------------------------------------
    string CloseFile(int fd) {
        if (is_formated && fd >= 0 && fd < FileDescriptorSize && OpenFileDescriptors[fd].isInUse()) {
            OpenFileDescriptors[fd].setInUse(false);
            return OpenFileDescriptors[fd].getFileName();
        } else {
            cerr<<error_msg<<endl;
            return "-1";
        }

    }
    // ------------------------------------------------------------------------
    int WriteToFile(int fd, char *buf, int len ) {
        if (!is_formated || fd < 0 || fd >= FileDescriptorSize)
            return returnError();

        if (!OpenFileDescriptors[fd].isInUse())
            return returnError();

        fsInode* node = OpenFileDescriptors[fd].getInode();

        int writeResult = node->writeToFile(buf, len ,sim_disk_fd,&BitVector,BitVectorSize,&freeBlocks,maxMemoryInFile);

        if (writeResult == failure_value)
            return returnError();

        return success_value;
    }
    // ------------------------------------------------------------------------
    int DelFile(string FileName) {
        _Rb_tree_iterator<pair<const basic_string<char>, fsInode *>> it = MainDir.find(FileName);
        if (it == MainDir.end() || !is_formated)
            return returnError();

        int fdOfNode = -1;
        for (int i = 0 ; i < FileDescriptorSize ; i++) {
            if (OpenFileDescriptors[i].getFileName() == FileName) {
                fdOfNode = i;
                if (OpenFileDescriptors[i].isInUse())
                    return returnError();
            }

        }
        fsInode* node = it->second;
        int deletionStatus = node->deleteFile(sim_disk_fd,&BitVector,&freeBlocks,BitVectorSize);
        delete node;
        MainDir.erase(FileName);
        if (fdOfNode != -1)
            OpenFileDescriptors[fdOfNode].setToDeleted();
        return deletionStatus;
    }
    // ------------------------------------------------------------------------
    int ReadFromFile(int fd, char *buf, int len ) {
        buf[0] = '\0';
        if (!is_formated || fd < 0 || fd >= FileDescriptorSize)
            return returnError();

        if (!OpenFileDescriptors[fd].isInUse())
            return returnError();

        return OpenFileDescriptors[fd].getInode()->readFromFile(buf, len, sim_disk_fd);
    }

    // ------------------------------------------------------------------------
    int CopyFile(string srcFileName, string destFileName) {
        _Rb_tree_iterator<pair<const basic_string<char>, fsInode *>> it = MainDir.find(srcFileName);
        if (!is_formated || it == MainDir.end() || checkIfFileExists(destFileName) ||
        checkIfFileIsOpen(srcFileName) > -1 || srcFileName == destFileName) // src file needs to be closed
            return returnError();

        int blocksNeeded = it->second->getBlocksInUse();

        if (blocksNeeded > freeBlocks)
            return returnError();

        int size = it->second->getSize();
        fsInode * NewNode = new fsInode(block_size);

        char buf[DISK_SIZE];
        it->second->readFromFile(buf,size,sim_disk_fd);

        NewNode->writeToFile(buf,strlen(buf),sim_disk_fd, &BitVector,BitVectorSize,&freeBlocks,maxMemoryInFile);
        MainDir.emplace(destFileName, NewNode);
        return success_value;
    }


    // ------------------------------------------------------------------------
    int RenameFile(string oldFileName, string newFileName) { // 0 = success | 1 = failure

        if (!is_formated || checkIfFileIsOpen(oldFileName) >= 0)// check if file is open
            return returnError();
        else {
            _Rb_tree_iterator<pair<const basic_string<char>, fsInode *>> it = MainDir.find(oldFileName);
            if (it == MainDir.end())
                return returnError();
            fsInode * Inode = it->second;
            MainDir.erase(oldFileName);
            MainDir.emplace(newFileName, Inode);
            for (int i = 0 ; i < FileDescriptorSize ; i++) {
                if (OpenFileDescriptors[i].getFileName() == oldFileName)
                    OpenFileDescriptors[i].setName(newFileName);
            }
        }
        return success_value;
    }

    // ------------------------------------------------------------------------
    ~fsDisk() { // destructor
        for (int i=0; i < DISK_SIZE ; i++) { // delete all disk content
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            ret_val = fwrite( "\0" ,  1 , 1, sim_disk_fd );
        }
        freeAllInodes();
        fclose(sim_disk_fd);
    }
};

int main() {

    int blockSize;
    string fileName;
    string fileName2;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read;
    int _fd;
    int result;

    auto *fs = new fsDisk();
    int cmd_;


    while(true) {
        cin >> cmd_;
        switch (cmd_)
        {
            case 0:   // exit
                delete fs;
                exit(EXIT_SUCCESS);

            case 1:   // list-file
                fs->listAll();
                break;

            case 2:   // format - done
                cin >> blockSize;
                fs->fsFormat(blockSize);
                break;

            case 3:    // create-file
                cin >> fileName;
                _fd = fs->CreateFile(fileName);
                cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;


            case 4:  // open-file
                cin >> fileName;
                _fd = fs->OpenFile(fileName);
                cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 5:  // close-file
                cin >> _fd;
                fileName = fs->CloseFile(_fd);
                cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;


            case 6:   // write-file
                cin >> _fd;
                cin >> str_to_write;
                fs->WriteToFile( _fd , str_to_write , strlen(str_to_write) );
                break;

            case 7:    // read-file
                cin >> _fd;
                cin >> size_to_read ;
                fs->ReadFromFile( _fd , str_to_read , size_to_read );
                cout << "ReadFromFile: " << str_to_read << endl;
                break;

            case 8:   // delete file - check what is the correct return value
                cin >> fileName;
                _fd = fs->DelFile(fileName);
                cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 9:   // copy file
                cin >> fileName;
                cin >> fileName2;
                fs->CopyFile(fileName, fileName2);
                break;

            case 10:  // rename file
                cin >> fileName;
                cin >> fileName2;
                fs->RenameFile(fileName, fileName2);
                break;

            default:
                break;
        }
    }
}
