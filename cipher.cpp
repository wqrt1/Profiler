#include <iostream>
#include <unordered_set>
#include <string>
#include <fstream>
#include <cstddef>
#include <cstdlib>

std::unordered_set<std::string> load_dict(const std::string& path) {
    std::unordered_set<std::string> dictionary{};
    dictionary.reserve(170000);

    std::ifstream file(path);

    if (!file) {
        std::cerr << "Failed to open word list at " << path << '\n';
        exit(1);
    }
    
    std::string word{};

    while (std::getline(file, word)) {
        if (!word.empty()) 
            dictionary.insert(word);
    }
    return dictionary;
}

std::string str_clean(const std::string& str) {
    std::string str_clean{};
    str_clean.reserve(str.size());

    for (char c : str) {
        if (c >= 'a' && c <= 'z')
            str_clean += 'A' + (c - 'a');
        else if (c >= 'A' && c <= 'Z')
            str_clean += c;
    }
    return str_clean;
}

void encrypt(const std::string& data, const std::string& key, std::string& buffer) { 
    buffer.resize(data.size());

    for (std::size_t i{}; i < data.size(); i++) {
        char encr_char = data[i] + (key[i % key.size()] - 'A');
        if (encr_char > 'Z') {
            encr_char -= 26;
        }
        buffer[i] = encr_char;
    }
}

void decrypt(const std::string& data, const std::string& key, std::string& buffer) {
    buffer.resize(data.size());

    for (std::size_t i{}; i < data.size(); i++) {
        char decr_char = data[i] - (key[i % key.size()] - 'A');
        if (decr_char < 'A') {
            decr_char += 26;
        }
        buffer[i] = decr_char;
    }
}

void bruteforce(const std::string& ciphertext, int keylength, int firstWordLength, std::string& buffer) {
    auto dict = load_dict("MP1_dict.txt");
    
    std::string key(keylength, 'A');
    std::string first_word = ciphertext.substr(0, firstWordLength);
    while (true) {
        decrypt(first_word, key, buffer);
        if (dict.contains(buffer)) {
            decrypt(ciphertext, key, buffer);
            std::cout << "Key: " << key << " | " << buffer << '\n';
        }
        int i{keylength - 1};
        while (i >= 0 && key[i] == 'Z') {
            key[i] = 'A';
            i--;
        }
        if (i < 0) break;

        key[i]++;
    }
}

constexpr int MAX_STRING_SIZE{1000};
int main(int argc, char* argv[]) {
    if (argc == 4) {
        std::string buffer{};
        buffer.reserve(MAX_STRING_SIZE);

        std::string arg1{argv[1]};
        if (arg1 == "--encrypt" || arg1  == "-e" || arg1  == "-E") {
            encrypt(str_clean(argv[2]), str_clean(argv[3]), buffer);
            std::cout << buffer << '\n';

        } else if (arg1 == "--decrypt" || arg1  == "-d" || arg1  == "-D") {
            decrypt(str_clean(argv[2]), str_clean(argv[3]), buffer);
            std::cout << buffer << '\n';

        } else {
            bruteforce(str_clean(arg1), std::stoi(argv[2]), std::stoi(argv[3]), buffer);
        }
    } else {
        std::cerr 
            << "Brute-Force:  ./cipher.exe <ciphertext> <key-length> <first-word-length>\n" 
            << "Encrypt:      ./cipher.exe --encrypt <ciphertext> <key>\n"
            << "Decrypt:      ./cipher.exe --decrypt <ciphertext> <key>\n";
        return 1;
    }

    return 0;
}