#include <fstream>
#include <vector>
#include <iterator>
#include <iostream>

int main(int argc, char *argv[]) {
    std::string filename;
    if (argc == 1) {
        std::cout << "Usage: ./r.o filename" << std::endl;
        return 1;
    }
    if (argc == 2) {
        filename = argv[1];
    }

    // Open file in binary mode and read bytes into vector
    std::ifstream file(filename, std::ios::binary);
    std::vector<char> bytes(
        (std::istreambuf_iterator<char>(file)),
        (std::istreambuf_iterator<char>())
    );

    // Format characters to hexadecimal
    for (unsigned char b : bytes) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b) << " ";
    }
    std::cout << std::dec << std::endl;

    file.close();
    return 0;
}