#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include <filesystem>

#include "lib/json.hpp"

using json = nlohmann::json;
using namespace std;

namespace fs {
    
    struct FileReaderResult {
        bool hasError = false;
        string error;
        string data;
    };
    
    struct FileWriterOptions {
        string filename;
        string data;
    };
    
    struct FileStats {
        bool hasError = false;
        string error;
        size_t size;
        bool isDirectory;
        bool isFile;
    }; 

    bool createDirectory(filesystem::path path);
    bool removeFile(filesystem::path file);
    fs::FileReaderResult readFile(string file);
    bool writeFile(fs::FileWriterOptions fileWriterOptions);
    string getDirectoryName(string path);
    string getCurrentDirectory();
    string getAbsolutePathFromRelative(filesystem::path path);
    fs::FileStats getStats(filesystem::path path);

namespace controllers {
    json createDirectory(json input);
    json removeDirectory(json input);
    json writeFile(json input);
    json writeBinaryFile(json input);
    json readFile(json input);
    json readBinaryFile(json input);
    json removeFile(json input);
    json readDirectory(json input);
    json copyFile(json input);
    json moveFile(json input);
    json getStats(json input);
} // namespace controllers
} // namespace fs

#endif
