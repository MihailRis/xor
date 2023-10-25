#define _CRT_RAND_S
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

#define READ_BUFFER_SIZE (512)

using std::ios;
namespace fs = std::filesystem;

inline long long min(long long a, long long b) {
    return (a < b) ? a : b;
}

/**
 * Generate specified key file filled with random bytes from /dev/urandom or rand_s()
 * @param file_size size of key file
 * @param filename key file name
 * @return result status (EXIT_SUCCESS or EXIT_FAILURE)
 */
int generate_key(ssize_t file_size, const char* filename) {
    // unix solution
#   ifndef _WIN32
    std::ifstream urandom("/dev/urandom", ios::in|ios::binary);
    if (!urandom.good()) {
        std::cerr << "could not to open /dev/urandom" << std::endl;
        return EXIT_FAILURE;
    }
#   endif

    std::ofstream output(filename, ios::out|ios::binary);
    if (!output.is_open()) {
        std::cerr << "could not to create key file " << filename << std::endl;
        return EXIT_FAILURE;
    }
    char buffer[READ_BUFFER_SIZE];
    ssize_t total_read = 0;
    while (total_read < file_size) {
        ssize_t to_read = min(file_size-total_read, READ_BUFFER_SIZE);

#       ifdef _WIN32
        unsigned int rnd;
        ssize_t read = 0;
        for (ssize_t read = 0; read < to_read; read += sizeof(unsigned int)) {
            rand_s(&rnd);
            for (unsigned j = 0; j < sizeof(unsigned int) && read < READ_BUFFER_SIZE; j++) {
                buffer[read++] = (rnd >> (j << 8)) & 0xFF;
            }
        }
#       else
        urandom.read(buffer, to_read);
        if (urandom.fail()) {
            std::cerr << "error while reading from /dev/urandom" << std::endl;
            return EXIT_FAILURE;
        }
        ssize_t read = urandom.gcount();
#       endif // _WIN32
        total_read += read;
        output.write(buffer, read);
        if (output.fail()) {
            std::cerr << "error while writing key" << std::endl;
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

/**
 * XOR input with key and write to output
 * key length must be greather or equal than input length
 * @param input input stream
 * @param key key stream
 * @param output output stream
 * @return result status (EXIT_SUCCESS or EXIT_FAILURE)
 */
int xor_file(std::istream& input, std::istream& key, std::ostream& output) {
    unsigned char buffer[READ_BUFFER_SIZE];
    unsigned char keybuffer[READ_BUFFER_SIZE];
    size_t total_read = 0;
    while (input) {
        input.read((char*)buffer, READ_BUFFER_SIZE);
        key.read((char*)keybuffer, READ_BUFFER_SIZE);
        size_t read = READ_BUFFER_SIZE;
        if (input.eof()) {
            read = input.gcount();
        } else if (key.eof()) {
            std::cerr << "error: input is longer than key" << std::endl;
            return EXIT_FAILURE;
        }
        for (size_t i = 0; i < read; i++) {
            buffer[i] ^= keybuffer[i];
        }
        output.write((char*)buffer, read);
        if (output.fail()) {
            std::cerr << "error while writing an output file" << std::endl;
            return EXIT_FAILURE;
        }
        total_read += read;
    }
    return EXIT_SUCCESS;
}

/**
 * UNTESTED
 * XOR input with key and write to output
 * key length must be greather or equal than input length
 * @param input input stream
 * @param key key stream
 * @param output output stream
 * @param length input length limit
 * @return result status (EXIT_SUCCESS or EXIT_FAILURE)
 */
int xor_file(std::istream& input, std::istream& key, std::ostream& output, size_t length) {
    unsigned char buffer[READ_BUFFER_SIZE];
    unsigned char keybuffer[READ_BUFFER_SIZE];
    size_t total_read = 0;
    while (total_read < length) {
        size_t toread = min(READ_BUFFER_SIZE, length-total_read);
        input.read((char*)buffer, toread);
        key.read((char*)keybuffer, toread);
        size_t read = toread;
        if (input.eof()) {
            read = input.gcount();
            if (read < toread) {
                std::cerr << "error: unexpected end of file" << std::endl;
                return EXIT_FAILURE;
            }
        } else if (key.eof()) {
            std::cerr << "error: input is longer than key" << std::endl;
            return EXIT_FAILURE;
        } else if (!input) {
            std::cerr << "error while reading input file" << std::endl;
            return EXIT_FAILURE;
        }
        for (size_t i = 0; i < read; i++) {
            buffer[i] ^= keybuffer[i];
        }
        output.write((char*)buffer, read);
        if (output.fail()) {
            std::cerr << "error while writing an output file" << std::endl;
            return EXIT_FAILURE;
        }
        total_read += read;
    }
    return EXIT_SUCCESS;
}

/**
 * XOR input file with key file and write to output file
 * creates file: {output_filename}
 * key length must be greather or equal than input length
 * @param input_filename input file name
 * @param key_filename key file name
 * @param output_filename output file name
 * @return result status (EXIT_SUCCESS or EXIT_FAILURE)
 */
int xor_file(const char* input_filename, const char* key_filename, const char* output_filename) {
    std::ifstream input(input_filename);
    if (!input.good()) {
        std::cerr << "could not to open " << input_filename << std::endl;
        return 1;
    }

    std::ifstream key(key_filename, ios::in|ios::binary);
    if (!key.good()) {
        std::cerr << "could not to open " << key_filename << std::endl;
        return EXIT_FAILURE;
    }
    
    std::ofstream output(output_filename, ios::out|ios::binary);
    if (!output.is_open()) {
        std::cerr << "could not to create output file " << output_filename << std::endl;
        return EXIT_FAILURE;
    }
    return xor_file(input, key, output);
}

/**
 * XOR input file with key file and write to {input_filename}.xor
 * creates file: {input_filename}.xor
 * key length must be greather or equal than input length
 * @param input_filename input file name
 * @param key_filename key file name
 * @return result status (EXIT_SUCCESS or EXIT_FAILURE)
 */
int xor_file(const char* input_filename, const char* key_filename) {
    std::string output_filename = std::string(input_filename) + ".xor";
    return xor_file(input_filename, key_filename, output_filename.c_str());
}


/**
 * XOR file with random (/dev/urandom) key generation
 * creates files: {input_filename}.key and {input_filename}.xor
 * @param input_filename input file name
 * @return result status (EXIT_SUCCESS or EXIT_FAILURE)
 */
int xor_file(const char* input_filename) {
    std::string key_filename = std::string(input_filename) + ".key";
    ssize_t file_size = fs::file_size(input_filename);
    if (file_size < 0) {
        std::cerr << "could not to get size of input file " << input_filename << std::endl;
        return EXIT_FAILURE;
    }
    int errcode;
    if ((errcode = generate_key(file_size, key_filename.c_str()))) {
        return errcode;
    }
    return xor_file(input_filename, key_filename.c_str());
}

/**
 * Show command line guide message
 */
void show_help() {
    std::cout << "XOR files encryption program" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "    > xor input-file" << std::endl;
    std::cout << "  Creates input-file.xor as input-file xor"
    " input-file.key (filled with random bytes from /dev/urandom)" << std::endl;
    std::cout << "    > xor input-file key-file" << std::endl;
    std::cout << "  Creates input-file.xor as input-file xor key-file" << std::endl;
    std::cout << "    > xor input-file key-file out-file" << std::endl;
    std::cout << "  Creates out-file as input-file xor key-file" << std::endl;
}

int main(int argc, char** argv) {
    switch (argc) {
    case 1:
        show_help();
        break;
    case 2: 
        return xor_file(argv[1]);
    case 3:
        return xor_file(argv[1], argv[2]);
    case 4:
        return xor_file(argv[1], argv[2], argv[3]);
    }
    return EXIT_SUCCESS;
}
