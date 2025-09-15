#pragma once
#include <string>
#include <unordered_map>
#include <algorithm>
#include <queue>
#include "imgui.h"
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <future>
#include <cstring>
#include <cstdlib>
#include "Crypt.h"

struct Arquivo
{
    int FileID;
    std::string FileHash;
    std::string FilePath;
    bool ToUpdate = false;
};

class Helper
{
    public:
        Helper();
        bool isMaintenance;
        bool isWorkerDone;
        int g_PopupPage;
        std::string g_Message;
        std::vector<Arquivo> ListaArquivos;
        int updateCount;
        std::thread workerThread;
        std::atomic<bool> isRunning{ false };
        std::string GetFileFromURL( int iType );
        void ParseFileList();
        bool iequals(const std::string& a, const std::string& b);
        void FileCheckUpdate();
        void CheckWorker();
        std::string GetDirectoryFromPath(const std::string& filepath);
        bool CreateDirectoryIfNotExists(const std::string& dirPath);
        void WorkerUpdating(int updateCount);
        bool DownloadFile(const std::string& remoteFile, const std::string& localPath, int fileIndex, int totalFiles);
        bool InjectDLL(HANDLE hProcess, const std::string& dllPath);
        void ClickPlayButton();
        void UpdateLauncher();
        std::string GetGameExePath();
};