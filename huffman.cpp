#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "queue.h"
#include "huffman.h"


Node *newNode(){
    Node *node = new Node;
    node->freq = node->value = 0;
    node->codelength = node->code = 0;
    node->left = node->right = NULL;
    return(node);
}

void deleteNode(Node *node){
    delete(node);
}

void deleteTree(Node *head){
    if (head){
        deleteTree(head->left);
        deleteTree(head->right);
        deleteNode(head);
    }
}

uint8_t *readFileToBuffer(const char *filepath, uint32_t *filesize){
    FILE *f = NULL;
    fopen_s(&f, filepath, "rb");
    if (!f){
        fprintf(stderr, "No file found");
        return(NULL);
    }
    fseek(f, 0, SEEK_END);
    uint32_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buffer = new uint8_t[fsize];
    fread(buffer, 1, fsize, f);
    fclose(f);
    *filesize = fsize;
    return(buffer); 
}


void inorderTraverse(Node *root, uint32_t code, uint8_t codelen){
    if (root->left)
        inorderTraverse(root->left, (code<<1), codelen+1);
    if (root->type == Node::Leaf){
        root->codelength = codelen;
        root->code = code;
        fprintf(stdout, "\n%u = %llx, %u ",root->value, root->code, root->codelength);
    }
    if (root->right)
        inorderTraverse(root->right, (code<<1)|1, codelen+1);
}


uint32_t height(Node *t){
    if (t == NULL){
        return(1);
    }
    uint32_t l = height(t->left);
    uint32_t r = height(t->right);
    uint32_t max = (l>r)?l:r;
    return(max+1);
}

// compresses a given byte array into huffman codes and tree
Huffman_Data huffmanCompress(void * data, uint32_t n){
    Huffman_Data hdata;
    uint32_t frequencies[256] = { 0 };
    hdata.uniqueSymbols = 0;
    PriorityQueue<Node *> queue;
    
    // count the frequency of each byte
    uint8_t *bytes = (uint8_t *)data;
    for (int i=0; i<n; i++){
        frequencies[bytes[i]]++;
    }

    // create nodes for bytes with atleast one occurence
    for (int i=0; i<256; i++){
        // fprintf(stdout, "%u: %d\n", i, frequencies[i]);
        hdata.nodes[i] = NULL;
        if (frequencies[i] > 0){
            Node *node = newNode();
            node->value = i;
            node->freq = frequencies[i];
            node->type = Node::Leaf;
            queue.enqueue(node, node->freq);
            hdata.nodes[i] = node;
            hdata.uniqueSymbols++;
        }
    }

    uint32_t size = queue.size;
    fprintf(stdout, "No of nodes = %u\n", size);

    // create the huffman tree
    while (queue.size>1){
        Node *a = queue.dequeue();
        Node *b = queue.dequeue();
        Node *ab = newNode();
        ab->freq = a->freq + b->freq;
        ab->left = a;
        ab->right = b;
        ab->type = Node::NotLeaf;
        queue.enqueue(ab, ab->freq);
        hdata.huffmanTree.root = ab;
    }

    printf("Height of tree: %u\n",height(hdata.huffmanTree.root));

    // get the codes to the nodes from huffman tree
    // left = 0
    // right = 1
    inorderTraverse(hdata.huffmanTree.root, 0, 0);



    // bitpack the code bits to the output
    // EXAMPLE:
    
    // 00110101|1110----|-------
    //              ^
    //              destBitCursor = 4

    // code:                      10(1)01 
    // code >> j (=4)       ->  0000010(1)
    // (code>>j) & 1        ->  0000000(1)
    // (7-destBitCursor)    ->  7-4 -> 3        = number of bits the code bit needs to be shifted to fit into cursor position
    // OR the code bit with the destination

    hdata.data = new uint8_t[n];
    uint8_t destBitCursor = 0;
    uint32_t destByteCursor = 0; 
    hdata.data[destByteCursor] = 0;
    for (int i=0; i<n; i++){
        for (int j=hdata.nodes[bytes[i]]->codelength-1; j>=0;j--){
            hdata.data[destByteCursor] |= ((hdata.nodes[bytes[i]]->code>>j) & 0x01)<<(7-destBitCursor);
            destBitCursor++;
            if(destBitCursor == 8){
                destBitCursor = 0;
                destByteCursor++;
                hdata.data[destByteCursor] = 0;
            }
        }
    }
    
    hdata.totalBits_encoded = ((uint64_t)destByteCursor * 8 + (uint64_t)destBitCursor);
    hdata.totalBytes_unencoded = n;

    return hdata;
}


// compresses a given file 
Huffman_File compressFile(const char *srcFilePath){
    uint32_t filesize;
    uint8_t *buffer = readFileToBuffer(srcFilePath, &filesize);
    
    Huffman_File file;
    file.data = huffmanCompress(buffer, filesize);
    file.header.compressionLvl = 1;
    file.header.hf[0] = 'h';
    file.header.hf[1] = 'f';
    file.header.hf[2] = 't';
    file.header.hf[3] = 'f';
    file.header.reserved = 0;
    file.header.offset_symbols = sizeof(file.header) + 4 * sizeof(uint32_t);

    // TODO: change the offsets
    file.header.offset_data = file.header.offset_symbols + file.data.uniqueSymbols * (sizeof((*file.data.nodes)->value)+sizeof((*file.data.nodes)->codelength)+sizeof((*file.data.nodes)->code));
    delete[] buffer;
    return(file);
}

// writes a .hftf file and returns the size of written file
uint64_t writeFile_encoded(Huffman_File *file, const char *filename){
    uint64_t filesize = 0;
    char fname[100];
    const char *outdir = "./";
    strcpy(fname, outdir);
    strcpy(fname + strlen(outdir), filename);
    strcpy(fname + strlen(outdir) + strlen(filename), ".hftf");


    FILE *f = NULL;
    fopen_s(&f, fname, "wb");
    fwrite(&file->header, 1, sizeof(file->header),f);
    fwrite(&file->data.totalBytes_unencoded,sizeof(file->data.totalBytes_unencoded), 1,f);
    fwrite(&file->data.uniqueSymbols,sizeof(file->data.uniqueSymbols), 1,f);
    fwrite(&file->data.totalBits_encoded,sizeof(file->data.totalBits_encoded), 1,f);
    filesize += sizeof(file->header) + sizeof(uint32_t) *2 + sizeof(uint64_t);

    for (int i=0; i<256;i++){
        Node* node = file->data.nodes[i];
        // each code data is 10 bytes
        if (node){
            // printf("Byte %c : Len %d Code :%llx\n",node->value, node->codelength, node->code);
            // value(1) + codelength(1)
            fwrite(&node->value, 1,sizeof(node->value), f);
            fwrite(&node->codelength, 1,sizeof(node->codelength), f);
            // code(8)
            fwrite(&node->code,1, sizeof(node->code),  f);
        }
    }
    fwrite(file->data.data, 1, file->data.totalBits_encoded/8 + 1,f);
    
    filesize += file->data.uniqueSymbols * 10;
    filesize += file->data.totalBits_encoded/8 +1;
    
    fclose(f);
    deleteTree(file->data.huffmanTree.root);
    
    return(filesize);
}


// adds nodes to recreate the huffman tree from given codes
void addCodeToTree(BinaryTree *huffmanTree, uint64_t code, uint8_t codelength, uint8_t value){
    if (!huffmanTree->root){
        huffmanTree->root = newNode();
        huffmanTree->root->type = Node::NotLeaf;
    }
    Node *ptr = huffmanTree->root;
    for (int i=0 ;i<codelength; i++){
        uint64_t dir = (code & 0x01<<(codelength - 1 - i));
        // left
        if (dir == 0){
            if(!ptr->left){ 
                ptr->left = newNode();
                ptr->left->type = Node::NotLeaf;
            }
            ptr = ptr->left; 
        }
        //  right 
        else{
            if (!ptr->right) {
                ptr->right = newNode();
                ptr->right->type = Node::NotLeaf;
            }
            ptr = ptr->right;
        }
    }
    ptr->code = code;
    ptr->codelength = codelength;
    ptr->value = value;
    ptr->type = Node::Leaf;
}


// decodes given data with huffman tree
uint8_t * huffmanDecompress(Huffman_Data *hdata){
    uint8_t *decoded = new uint8_t[hdata->totalBytes_unencoded];
    uint32_t destByteCursor=0, srcByteCursor=0;
    uint64_t srcBitCursor=0;
    while (destByteCursor < hdata->totalBytes_unencoded && (srcByteCursor*8 + srcBitCursor)<hdata->totalBits_encoded){
        Node *ptr = hdata->huffmanTree.root;
        int height = 0;
        while (ptr->type != Node::Leaf){
            uint8_t dir = hdata->data[srcByteCursor]&(0x01<<(7-srcBitCursor));
            if (dir == 0)
                ptr = ptr->left;
            else 
                ptr = ptr->right;
            height++;
            srcBitCursor++;
            if (srcBitCursor == 8){
                srcByteCursor++;
                srcBitCursor = 0;
            }
        }
        if (height > 8) {
            printf("Height: %d\n", height);
        }
        decoded[destByteCursor++] = ptr->value;
    }
    return(decoded);
}

// decodes the given .hftf file and returns an array of bytes
// sets the number of bytes in noofbytes
uint8_t * decompressFile(const char* srcFilepath, uint32_t *noofbytes){
    if(strcmp(srcFilepath + strlen(srcFilepath) -1-4, ".hftf")){
        fprintf(stderr, "Not a valid compressed file. A valid file is <file>.hftf");
        return(NULL);
    }

    uint32_t filesize;
    uint32_t cursor = 0;
    uint8_t *buffer = readFileToBuffer(srcFilepath, &filesize);
    if (!buffer){
        return(NULL);
    }
    Huffman_File f;
    memcpy(&f.header, buffer, sizeof(f.header));
    cursor += sizeof(f.header);
    uint8_t hftf = f.header.hf[0]^'h' | f.header.hf[1]^'f'| f.header.hf[2]^'t' |f.header.hf[3]^'f';
    if (hftf){
        fprintf(stderr, "[ERROR] Not a valid compressed file.");
        return(NULL);
    }
    memcpy(&f.data.totalBytes_unencoded, buffer+cursor, sizeof(uint32_t) * 2+ sizeof(uint64_t));
    cursor += sizeof(uint32_t) * 2+ sizeof(uint64_t);
    
    // symbols + codes
    for (int i=0 ;i<f.data.uniqueSymbols;i++){
        uint8_t value = buffer[cursor++];
        uint8_t codelength = buffer[cursor++];
        uint64_t code = *(uint64_t*)(buffer+cursor);
        cursor += sizeof(code);
        addCodeToTree(&f.data.huffmanTree, code, codelength, value);
        printf("Val: %c Code: %lld Length: %u\n", value, code, codelength);
    }
    printf("Height of tree constructed: %u\n",height(f.data.huffmanTree.root));

    // actual encoded data
    f.data.data = buffer + cursor;
    
    // decode the data
    uint8_t *decoded = huffmanDecompress(&f.data);

    *noofbytes = f.data.totalBytes_unencoded;

    delete[]buffer;
    deleteTree(f.data.huffmanTree.root);
    
    return(decoded);
}

void writeFile_decompressed(const char *filename, uint8_t *decoded, uint32_t noOfBytes){
    FILE * f = NULL;
    fopen_s(&f, filename, "wb");
    fwrite(decoded, sizeof(uint8_t), noOfBytes, f);
    fclose(f);
    delete[]decoded;
}

#ifdef HF_EXE
#define COMMAND_STRUCT "<.exe> zip <input filepath> <output filename>\n<.exe> unzip <.hftf file path> <output filename.extension>\n"

int main(int argc, char **argv){
    if (argc < 3){
        fprintf(stderr, "[ERROR] Not enough arguments\n\n%s",COMMAND_STRUCT);
        return(-1);
    }
    if (strcmp(argv[1], "zip")==0){
        const char *outputFile;
        if (argc == 3) outputFile = "out";
        else           outputFile = argv[3];

        Huffman_File f = compressFile(argv[2]);
        uint64_t filesize = writeFile_encoded(&f, outputFile);
        fprintf(stdout, "\nCompression ratio: %.5lf\n",(double)filesize/f.data.totalBytes_unencoded);
    }
    else if (strcmp(argv[1], "unzip")==0){
        const char *outputFile;
        if (argc == 3) outputFile = "out.txt";
        else           outputFile = argv[3];

        uint32_t size;
        uint8_t *decoded = decompressFile(argv[2],&size);
        writeFile_decompressed(outputFile, decoded, size);
    }
    else{
        fprintf(stderr, "[ERROR] %s is an invalid command\n\n%s", argv[1], COMMAND_STRUCT);
    }
    return 0;
}
#endif //HF_EXE