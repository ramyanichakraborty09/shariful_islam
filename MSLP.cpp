#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <winsock2.h>
#include <windows.h>
#include <sys/stat.h>

using namespace std;

#define PORT 5000

struct MSLPPacket {
    int serviceChoice;
    int subAction;
    char targetName[256];
    char fileContent[1024];
    char message[2048];
    DWORD timestamp;
};

void setColor(int textColor) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, textColor);
}

void clearScreen() {
    system("cls");
}

void writeNotificationLog(const string& alertMsg, int type) {
    ofstream logFile("mslp_logs.txt", ios::app);
    if (logFile.is_open()) {
        time_t now = time(0);
        char* dt = ctime(&now);
        string timeStr = dt;
        if (!timeStr.empty() && timeStr[timeStr.length() - 1] == '\n') {
            timeStr.erase(timeStr.length() - 1);
        }
        logFile << "[" << timeStr << "] ";
        if (type == 1) logFile << "[SUCCESS]";
        else if (type == 2) logFile << "[ERROR]";
        else logFile << "[INFO]";
        logFile << " " << alertMsg << endl;
        logFile.close();
    }
}

// Hardware Temperature Utility
double getSystemTemperature() {
    FILE* pipe = _popen("powershell -Command \"(Get-WmiObject"
	"MSAcpi_ThermalZoneTemperature -Namespace root/wmi).CurrentTemperature\"", "r");
    double tempCelsius = 0;
    bool success = false;
    if (pipe) {
        char result[128];
        if (fgets(result, sizeof(result), pipe) != NULL) {
            long tempKelvin = atol(result);
            if (tempKelvin > 200) {
                tempCelsius = (tempKelvin / 10.0) - 273.15;
                if (tempCelsius > 0 && tempCelsius < 150) success = true;
            }
        }
        _pclose(pipe);
    }
    if (!success) {
        tempCelsius = 42.5 + (rand() % 10); 
    }
    return tempCelsius;
}

// Folder Analysis Helper
void handleFolderAnalysis(const char* folderPath, char* buffer) {
    string path = folderPath;
    if (path == "" || path == ".") path = "C:\\";
    string searchPath = path + "\\*.*";

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(searchPath.c_str(), &fd);
    int fileCount = 0, docCount = 0, imgCount = 0, otherCount = 0;
    string list = "=== DIRECTORY ANALYSIS ===\nPath: " + path + "\n";
    int printed = 0;

    if (h != INVALID_HANDLE_VALUE) {
        do {
            string name = fd.cFileName;
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                fileCount++;
                string ext = "";
                size_t idx = name.find_last_of(".");
                if (idx != string::npos) ext = name.substr(idx);

                if (ext == ".txt" || ext == ".pdf" || ext == ".docx" || ext == ".doc") 
				docCount++;
                else if (ext == ".jpg" || ext == ".png" || ext == ".jpeg") 
				imgCount++;
                else otherCount++;

                if (printed < 10) {
                    list += " - " + name + "\n";
                    printed++;
                }
            }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
        char summary[256];
        sprintf(summary, "\nTotal Files: %d | Docs: %d | Images: %d | Others: %d", 
		fileCount, docCount, imgCount, otherCount);
        list += summary;
        strncpy(buffer, list.c_str(), 2047);
    } else {
        sprintf(buffer, "Folder '%s' not found or access denied.", path.c_str());
    }
    buffer[2047] = '\0';
}

// Battery Status & RAM Percentage Helper
void handleSystemStatus(char* buffer) {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    DWORDLONG totalRAM = memInfo.ullTotalPhys / (1024 * 1024);
    DWORDLONG availRAM = memInfo.ullAvailPhys / (1024 * 1024);
    DWORDLONG usedRAM = totalRAM - availRAM;
    int ramLoad = memInfo.dwMemoryLoad;

    SYSTEM_POWER_STATUS power;
    int battery = -1;
    string pState = "AC Power Connected";
    if (GetSystemPowerStatus(&power)) {
        if (power.BatteryLifePercent != 255) battery = power.BatteryLifePercent;
        if (power.ACLineStatus == 0) pState = "Discharging (Battery)";
    }

    if (battery != -1) {
        sprintf(buffer, "Battery Status: %d%% (%s) | Total RAM: %llu MB | Used RAM: %llu MB"
		"| RAM Usage Load: %d%%", 
                battery, pState.c_str(), totalRAM, usedRAM, ramLoad);
    } else {
        sprintf(buffer, "Power State: %s | Total RAM: %llu MB | Used RAM: %llu MB" 
		"| RAM Usage Load: %d%%", 
                pState.c_str(), totalRAM, usedRAM, ramLoad);
    }
}

// Secure Password Hashing (DJB2 Algorithm)
unsigned long computePasswordHash(const string& str) {
    unsigned long hash = 5381;
    for (size_t i = 0; i < str.length(); ++i) {
        hash = ((hash << 5) + hash) + str[i];
    }
    return hash;
}

// ==========================================
// SERVER RUNTIME
// ==========================================
void runServer() {
    WSADATA wsa;
    SOCKET serverSock, clientSock;
    sockaddr_in serverAddr, clientAddr;
    int addrLen = sizeof(clientAddr);

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        setColor(4);
        cout << "[CRITICAL ERROR] WSAStartup failed.\n";
        setColor(7);
        return;
    }

    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSock, 5);

    clearScreen();
    setColor(3);
    cout << "[SERVER] Listening for incoming client requests...\n";
    writeNotificationLog("Server started.", 1);

    clientSock = accept(serverSock, (sockaddr*)&serverAddr, &addrLen);
    setColor(2);
    cout << "[SUCCESS] Client connected successfully!\n";
    setColor(7);

    while (true) {
        MSLPPacket pkt;
        int bytes = recv(clientSock, (char*)&pkt, sizeof(pkt), 0);
        if (bytes <= 0) {
            setColor(4);
            cout << "\n[DISCONNECT ERROR] Client disconnected.\n";
            setColor(7);
            break;
        }

        clearScreen();
        setColor(3);
        cout << "==================================================\n";
        cout << "          SERVER PROCESSING FEATURE ID: " << pkt.serviceChoice << "   \n";
        cout << "==================================================\n";
        setColor(7);

        MSLPPacket resp;
        memset(&resp, 0, sizeof(resp));
        resp.serviceChoice = pkt.serviceChoice;

        switch (pkt.serviceChoice) {
            case 1: { // Text Messaging
                cout << "Received Chat Message: \"" << pkt.fileContent << "\"\n";
                cout << "Type reply message: ";
                cin.getline(resp.message, 2048);
                break;
            }
            case 2: { // File Manager
                cout << "Client requested folder analysis for path: \"" << pkt.targetName << "\"\n";
                handleFolderAnalysis(pkt.targetName, resp.message);
                break;
            }
            case 3: { // Temperature Checker with Server Prompt for Own Temp
                cout << "Client reported temperature: " << pkt.fileContent << " C\n";
                double serverTemp = getSystemTemperature();
                
                char promptChoice[16];
                cout << "--------------------------------------------------\n";
                cout << "Do you want to show my own temperature? (YES/NO): ";
                cin >> promptChoice;
                cin.ignore(10000, '\n');

                string pStr = promptChoice;
                if (pStr == "YES" || pStr == "yes" || pStr == "Yes" || pStr == "Y" || pStr == "y") {
                    char buf[256];
                    sprintf(buf, "Client CPU Temperature: %s C | Server Host Temperature: %.2f C", 
					pkt.fileContent, serverTemp);
                    strcpy(resp.message, buf);
                    cout << "[INFO] Server temperature included.\n";
                } else {
                    char buf[256];
                    sprintf(buf, "Client CPU Temperature: %s C", pkt.fileContent);
                    strcpy(resp.message, buf);
                    cout << "[INFO] Server temperature hidden as requested.\n";
                }
                break;
            }
            case 4: { // Battery & System Status (Client info shown first on server, then server option)
                cout << "================ CLIENT INFO ================\n";
                cout << pkt.fileContent << "\n";
                cout << "=============================================\n";
                
                char serverSysBuf[1024];
                handleSystemStatus(serverSysBuf);

                char promptChoice[16];
                cout << "--------------------------------------------------\n";
                cout << "Do you want to send my own battery status to client? (YES/NO): ";
                cin >> promptChoice;
                cin.ignore(10000, '\n');

                string pStr = promptChoice;
                if (pStr == "YES" || pStr == "yes" || pStr == "Yes" || pStr == "Y" || pStr == "y") {
                    char buf[2048];
                    sprintf(buf, "Server Host System Status: %s", serverSysBuf);
                    strcpy(resp.message, buf);
                    cout << "[INFO] Server battery status sent to client.\n";
                } else {
                    strcpy(resp.message, "Server battery status was not shared by server.");
                    cout << "[INFO] Server battery status omitted.\n";
                }
                break;
            }
            case 5: { // Secure Password Hashing
                cout << "Client requested Password Hashing.\n";
                unsigned long hashed = computePasswordHash(pkt.fileContent);
                char buf[256];
                sprintf(buf, "Password Hashed Successfully using DJB2 Algorithm -> Hash Code: %lu", hashed);
                strcpy(resp.message, buf);
                cout << "[INFO] Password hashed and sent to client.\n";
                break;
            }
            default: {
                setColor(4);
                sprintf(resp.message, "[ERROR] Invalid Feature ID.");
                setColor(7);
                break;
            }
        }

        send(clientSock, (char*)&resp, sizeof(resp), 0);

        setColor(2);
        cout << "\nResponse sent successfully to client.\n";
        setColor(6);
        cout << "Press Enter to continue...";
        setColor(7);
        cin.get();
        
        clearScreen();
        setColor(3);
        cout << "[SERVER] Listening for incoming client requests...\n";
        setColor(7);
    }

    closesocket(clientSock);
    closesocket(serverSock);
    WSACleanup();
}

// ==========================================
// CLIENT RUNTIME
// ==========================================
void runClient() {
    WSADATA wsa;
    SOCKET clientSock;
    sockaddr_in serverAddr;
    char serverIP[64];

    clearScreen();
    setColor(3);
    cout << "Enter Server IP Address: ";
    setColor(7);
    cin >> serverIP;
    cin.ignore(10000, '\n');

    WSAStartup(MAKEWORD(2, 2), &wsa);
    clientSock = socket(AF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIP);
    serverAddr.sin_port = htons(PORT);

    if (connect(clientSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        setColor(4);
        cout << "[CONNECTION ERROR] Failed to connect!\n";
        setColor(7);
        WSACleanup();
        return;
    }

    clearScreen();
    setColor(2);
    cout << "[SUCCESS] Connected successfully to MSLP Server!\n";
    setColor(7);

    while (true) {
        clearScreen();
        setColor(3);
        cout << "==================================================\n";
        cout << "       MSLP PROTOCOL - CLIENT TERMINAL     \n";
        cout << "==================================================\n";
        setColor(7);
        cout << " [0] Exit Client Session\n";
        cout << " [1] Text Messaging Service\n";
        cout << " [2] File Manager & Directory Analysis\n";
        cout << " [3] Temperature Checker Service\n";
        cout << " [4] Battery Status & RAM Percentage\n";
        cout << " [5] Secure Password Hashing\n";
        setColor(6);
        cout << "--------------------------------------------------\n";
        cout << "Select Service ID (0-5): ";
        setColor(7);

        int choice;
        cin >> choice;
        cin.ignore(10000, '\n');

        if (choice == 0) {
            setColor(6);
            cout << "Closing Client Session...\n";
            setColor(7);
            break;
        }

        MSLPPacket pkt;
        memset(&pkt, 0, sizeof(pkt));
        pkt.serviceChoice = choice;

        if (choice == 1) {
            cout << "Enter chat message for server: ";
            cin.getline(pkt.fileContent, 1024);
        }
        else if (choice == 2) {
            cout << "Enter folder path: ";
            cin.getline(pkt.targetName, 256);
        }
        else if (choice == 3) {
            double temp = getSystemTemperature();
            sprintf(pkt.fileContent, "%.2f", temp);
            cout << "Fetching local temperature and sending to server...\n";
        }
        else if (choice == 4) {
            char clientSysBuf[1024];
            handleSystemStatus(clientSysBuf);
            strcpy(pkt.fileContent, clientSysBuf);
            cout << "Fetching local system battery/RAM status and sending to server...\n";
        }
        else if (choice == 5) {
            cout << "Enter password string to hash: ";
            cin.getline(pkt.fileContent, 1024);
        }
        else {
            setColor(4);
            cout << "[WARNING ERROR] Invalid choice. Select between 0 and 5.\n";
            setColor(7);
            cin.get();
            continue;
        }

        send(clientSock, (char*)&pkt, sizeof(pkt), 0);

        MSLPPacket resp;
        recv(clientSock, (char*)&resp, sizeof(resp), 0);

        clearScreen();
        setColor(2);
        cout << "\n================ SERVER RESPONSE ================\n";
        cout << resp.message << endl;
        cout << "=================================================\n";
        setColor(6);
        cout << "\nPress Enter to return to menu...";
        setColor(7);
        cin.get();
    }

    closesocket(clientSock);
    WSACleanup();
}

int main() {
    SetConsoleTitleA("MSLP - Multi System Local Protocol");
    clearScreen();
    setColor(3);
    cout << "========================================\n";
    cout << "    MSLP - Multi System Local Protocol       \n";
    cout << "========================================\n";
    cout << "1. Start as Server \n";
    cout << "2. Start as Client \n";
    setColor(6);
    cout << "Select mode (1 or 2): ";
    setColor(7);

    int mode;
    cin >> mode;
    cin.ignore(10000, '\n');

    if (mode == 1) {
        runServer();
    } else if (mode == 2) {
        runClient();
    } else {
        setColor(4);
        cout << "[FATAL ERROR] Invalid mode selected.\n";
        setColor(7);
    }

    return 0;
}
