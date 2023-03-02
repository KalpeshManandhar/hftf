#pragma once

#ifndef HFTF_IMPLEMENTATION
#define HFTF_IMPLEMENTATION

#include <stdint.h>

struct Node{
    uint8_t value;
    uint8_t codelength;
    uint32_t freq;
    uint64_t code;
    Node *left,*right;
    enum NodeType{Leaf, NotLeaf};
    NodeType type;
};

struct BinaryTree{
    Node *root = NULL;
};

struct Huffman_Data{
    uint32_t totalBytes_unencoded;      // total bytes of the unencoded data 
    uint32_t uniqueSymbols;				// total number of unique codes/symbols used
    uint64_t totalBits_encoded;         // total bits of the encoded data 
    BinaryTree huffmanTree;             // huffman tree
	uint8_t *data;                      // actual encoded data 
    Node* nodes[256] = {NULL};
};


struct Huffman_File{
	struct Huffman_FileHeader{              // 4+2+2+4+4
        char hf[4];                         // hftf
        uint16_t reserved;                  // 0000
        uint16_t compressionLvl;            // number of compressions of same file
        uint32_t offset_data;   			// offset from beginning of file to data
        uint32_t offset_symbols;			// offset from beginning of file to symbols
    }header;
	Huffman_Data data;
    /*
        20 bytes header
        4 bytes -> total unencoded bytes
        4 bytes -> total symbols
        8 bytes -> total encoded bits
        10 bytes (value + codelength + code) for each symbol
        actual code for each byte
    */
};

Huffman_Data huffmanCompress(void * data, uint32_t n);
Huffman_File compressFile(const char *srcFilePath);
uint64_t writeFile_encoded(Huffman_File *file, const char *filename);


uint8_t * huffmanDecompress(Huffman_Data *hdata);
uint8_t * decompressFile(const char* srcFilepath, uint32_t *noofbytes);
void writeFile_decompressed(const char *filename, uint8_t *decoded, uint32_t noOfBytes);

#endif