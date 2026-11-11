MSLP – Multi Service Local Protocol

A C++ Based Multi-Service Client–Server Communication System

MSLP (Multi Service Local Protocol) is a C++ based networking project developed to integrate multiple computing services into a single client–server communication framework. Using TCP/IP networking and Windows Socket Programming (Winsock2), it demonstrates how different operations can be requested, processed and delivered through a common communication structure.

About the Project

MSLP establishes communication between a client and a server through TCP sockets. Both operating modes are available within the same application, allowing users to start the program as either a server or a client.

A custom-designed data structure, "MSLPPacket", organises service requests and exchanges information between the communicating systems. The server receives requests, identifies the selected service, processes the operation and sends the appropriate response.

Core Services

MSLP integrates five main services:

1. Text Messaging: Enables two-way communication between the client and server.
2. Folder and Directory Analysis: Examines folder-related information and directory details.
3. Temperature Checker: Performs temperature-related checking.
4. System Status Monitoring: Provides information about system resources, including RAM and battery status.
5. Secure Password Hashing: Implements the DJB2 hashing algorithm to demonstrate password hashing.

What Makes MSLP Unique?

The uniqueness of MSLP lies in bringing multiple independent services together under one communication framework. Instead of developing separate applications for different operations, it provides a common platform where services are organised through a custom packet structure.

Its single-application client–server design, bidirectional communication and integration of networking, file handling, system monitoring and hashing make it an interesting practical implementation of computer science concepts.

Technologies Used

C++, TCP/IP, Winsock2, Windows API, File Handling, DJB2 Hashing Algorithm, PowerShell and Dev-C++.

Future Scope

MSLP can be further enhanced with multiple client connections, advanced security, a graphical interface, improved file transfer and additional services.

One Protocol. Multiple Services. Unified Communication.    
