#include <iostream>
#include <fstream>

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>

#elif defined(_WIN32)
#define _WINSOCKAPI_
#include <windows.h>
#include <atlstr.h>
#include <shlwapi.h>
#include <winbase.h>
#endif 

#include "lib/json.hpp"
#include "lib/base64.hpp"
#include "settings.h"
#include "helpers.h"
#include "api/filesystem/filesystem.h"
#include "api/os/os.h"

using namespace std;
using json = nlohmann::json;

namespace fs {
    bool createDirectory(filesystem::path path) {
        return filesystem::create_directory(path);
    }
    
    bool removeFile(filesystem::path file) {
        return filesystem::remove(file);
    }
    
    string getDirectoryName(string filename){
        #if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
        return dirname((char*)filename.c_str());
        #elif defined(_WIN32)
        LPSTR pathToReplace = const_cast<char *>(filename.c_str());
        PathRemoveFileSpecA(pathToReplace);
        std::string directory(pathToReplace);
        std::replace(directory.begin(), directory.end(), '\\', '/');
        return directory;
        #endif 
    }

    string getCurrentDirectory() {
        return filesystem::current_path().string();
    }
    
    string getAbsolutePathFromRelative(filesystem::path path) {
        return filesystem::absolute(path).string();
    }
    
    fs::FileReaderResult readFile(string filename) {
        fs::FileReaderResult fileReaderResult;
        ifstream reader(filename.c_str(), ios::binary | ios::ate);
        if(!reader.is_open()) {
            fileReaderResult.hasError = true;
            fileReaderResult.error = "Unable to open " + filename;
            return fileReaderResult;
        }
        vector<char> buffer;
        size_t size = reader.tellg();
        reader.seekg(0, ios::beg);
        buffer.resize(size);
        reader.read(buffer.data(), size);
        string result(buffer.begin(), buffer.end());
        reader.close();
        
        fileReaderResult.data = result;
        return fileReaderResult;
    }

    bool writeFile(fs::FileWriterOptions fileWriterOptions) {
        json output;
        ofstream writer(fileWriterOptions.filename);
        if(!writer.is_open())
            return false;
        writer << fileWriterOptions.data;
        writer.close();
        return true;
    }
    
    fs::FileStats getStats(filesystem::path path) {
        fs::FileStats fileStats;

        error_code size_ec;
        fileStats.size = filesystem::file_size(path, size_ec);

        error_code dir_ec;
        if(filesystem::is_directory(path, dir_ec))
            fileStats.isDirectory = true;
        else
            fileStats.isFile = true;

        if(size_ec || dir_ec)
        {
            fileStats.hasError = true;
            fileStats.error = path.string() + " doesn't exists or access error";
        }

        return fileStats;
    }

namespace controllers {
    json createDirectory(json input) {
        json output;
        string path = input["path"];
        if(fs::createDirectory(path)) {
            output["success"] = true;
            output["message"] = "Directory " + path + " was created";
        }
        else{
            output["error"] = helpers::makeErrorPayload("NE_FS_DIRCRER", 
                                "Cannot create a directory in " + path);
        }
        return output;
    }

    json removeDirectory(json input) {
        json output;
        string path = input["path"];
        if(filesystem::remove(path)){
            output["success"] = true;
            output["message"] = "Directory " + path + " was removed";
        }
        else{
            output["error"] = helpers::makeErrorPayload("NE_FS_RMDIRER", 
                                "Cannot remove " + path);
        }
        return output;
    }

    json readFile(json input) {
        json output;
        fs::FileReaderResult fileReaderResult;
        fileReaderResult = fs::readFile(input["path"].get<std::string>());
        if(fileReaderResult.hasError) {
            output["error"] = helpers::makeErrorPayload("NE_FS_FILRDER", fileReaderResult.error);
        }
        else {
            output["returnValue"] = fileReaderResult.data;
            output["success"] = true;
        }
        return output;
    }
    
    json readBinaryFile(json input) {
        json output;
        fs::FileReaderResult fileReaderResult;
        fileReaderResult = fs::readFile(input["path"].get<std::string>());
        if(fileReaderResult.hasError) {
            output["error"] = helpers::makeErrorPayload("NE_FS_FILRDER", fileReaderResult.error);
        }
        else {
            output["returnValue"] = base64::to_base64(fileReaderResult.data);
            output["success"] = true;
        }
        return output;
    }

    json writeFile(json input) {
        json output;
        fs::FileWriterOptions fileWriterOptions;
        fileWriterOptions.filename = input["path"];
        fileWriterOptions.data = input["data"];
        if(fs::writeFile(fileWriterOptions))
            output["success"] = true;
        else
            output["error"] = helpers::makeErrorPayload("NE_FS_FILWRER", 
                                "Unable to write file: " + fileWriterOptions.filename);
        return output;
    }
    
    json writeBinaryFile(json input) {
        json output;
        fs::FileWriterOptions fileWriterOptions;
        fileWriterOptions.filename = input["path"];
        fileWriterOptions.data = base64::from_base64(input["data"].get<std::string>());
        if(fs::writeFile(fileWriterOptions))
            output["success"] = true;
        else
            output["error"] = helpers::makeErrorPayload("NE_FS_FILWRER", 
                                "Unable to write file: " + fileWriterOptions.filename);
        return output;
    }

    json removeFile(json input) {
        json output;
        string filename = input["path"];
        if(fs::removeFile(filename)) {
            output["success"] = true;
            output["message"] = filename + " was deleted";
        }
        else{
            output["error"] = helpers::makeErrorPayload("NE_FS_FILRMER", 
                                "Cannot remove " + filename);
        }
        return output;
    }

    json readDirectory(json input) {
        json output;
        string path = input["path"];
        fs::FileStats fileStats = fs::getStats(path);
        if(fileStats.hasError) {
            output["error"] = helpers::makeErrorPayload("NE_FS_NOPATHE", fileStats.error);
            return output;
        }

        if(filesystem::is_directory(path))
        {
            for(const auto& It : filesystem::directory_iterator(path))
            {
                const filesystem::path SubPath = It.path();
                string type = "OTHER";
                if(filesystem::is_directory(SubPath))
                    type = "DIRECTORY";
                else if(filesystem::is_regular_file(SubPath))
                    type = "FILE";
                
                json file = {
                    {"entry", SubPath.filename().string()},
                    {"type", type},
                };
                output["returnValue"].push_back(file);
            }
            output["success"] = true;
        }
        
        return output;
    }
   
    json copyFile(json input) {
        json output;
        string source = input["source"];
        string destination = input["destination"];

        error_code copy_file_ec;
        if(filesystem::copy_file(source, destination, copy_file_ec))
        {
            output["success"] = true;
            output["message"] = "File copy operation was successful";
        }
        else{
            output["error"] = helpers::makeErrorPayload("NE_FS_COPYFER",
                                "Cannot copy " + source + " to " + destination);
        }
        return output;
    } 
    
    json moveFile(json input) {
        json output;
        string source = input["source"];
        string destination = input["destination"];
        #if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
        string command = "mv \"" + source + "\" \"" + destination + "\"";
        string commandOutput = os::execCommand(command, true);
        if(commandOutput.empty()) {

        #elif defined(_WIN32)
        if(MoveFile(source.c_str(), destination.c_str()) == 1) {
        #endif
            output["success"] = true;
            output["message"] = "File move operation was successful";
        }
        else{
            output["error"] = helpers::makeErrorPayload("NE_FS_MOVEFER",
                                "Cannot move " + source + " to " + destination);
        }
        return output;
    } 
    
    json getStats(json input) {
        json output;
        string path = input["path"];
        fs::FileStats fileStats = fs::getStats(path);
        if(!fileStats.hasError) {
            output["success"] = true;
            output["size"] = fileStats.size;
            output["isFile"] = fileStats.isFile;
            output["isDirectory"] = fileStats.isDirectory;
        }
        else{
            output["error"] = helpers::makeErrorPayload("NE_FS_NOPATHE", fileStats.error);
        }
        return output;
    } 
    
} // namespace controllers
} // namespace fs
